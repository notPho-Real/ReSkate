#include "arcade_mode.h"
#include "arcade_observation.h"
#include "arcade_native_probe.h"
#include "arcade_gesture.h"
#include "arcade_score_probe.h"
#include "arcade_native_events.h"
#include "arcade_balance_input.h"
#include "arcade_balance_hud.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Extension/Skater/no_bail.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Extension/UI/Overlay/overlay.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Multiplayer/Hud/game_ui_state.h"
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <string>

namespace dingosdk::arcade_mode {
namespace {
std::atomic<bool> ready{};
std::atomic<bool> mode_enabled{};
bool preference_loaded{};
std::uintptr_t score_probe_base{};
// Only touched by the client thread, after initialization has been published.
std::uint32_t previous_buttons{};
ObservationTracker observations;
std::array<std::uint64_t, 4> press_counts{};
constexpr std::array<std::uint32_t, 4> face_buttons{0x1000u, 0x2000u, 0x4000u, 0x8000u};
constexpr std::array<char, 4> button_names{'A', 'B', 'X', 'Y'};
NativeRequestCapture request_capture;
bool request_probe_ready{};
std::uint64_t capture_started{}, capture_id{};
std::uint64_t hil_heartbeat_at{}, hil_heartbeat_id{};
std::atomic<std::uint64_t> ollie_eligible_until{};
std::atomic<std::uint64_t> gameplay_valid_until{};
ArcadeInput arcade_input;
std::mutex ollie_mutex;
ComboSession combo_session;
std::uint64_t combo_tick_at{};
std::uint64_t observed_wipeouts{};
bool had_combo_owner{};
SkatePose previous_pose{SkatePose::unknown};
std::uint64_t next_balance_sample{};
unsigned balance_sample_count{};
void update_combo(const NativeRequestSample& native, const ControllerInput& input,
                  bool watch_valid, std::uint64_t wipeouts, std::uint64_t now) noexcept {
    const auto score = score_activity();
    const bool valid = native.valid && input.available && watch_valid;
    const bool bail = valid && had_combo_owner && wipeouts != observed_wipeouts;
    had_combo_owner = valid;
    observed_wipeouts = wipeouts;
    const auto pose = observed_pose({valid,
        score.valid && now >= score.sampled_at && now - score.sampled_at <= 150,
        score.active, bail, native.state, native.crouch_mode, native.words[1] & 0xc000u});
    const float dt = combo_tick_at && now >= combo_tick_at ? static_cast<float>(now - combo_tick_at) / 1000.0f : 0;
    combo_tick_at = now;
    const auto kind = combo_session.model().state().balance_kind;
    const auto correction = balance_correction(kind, input.left_x, input.left_y, input.available);
    const bool failed = combo_session.step(pose, dt, correction);
    const auto& balance = combo_session.model().state();
    if (balance.balance_kind != BalanceKind::none && now >= next_balance_sample && balance_sample_count < 4096) {
        next_balance_sample = now + 250;
        ++balance_sample_count;
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE BALANCE SAMPLE: ms={} kind={} x={} v={} input={} difficulty={} raw_x={} raw_y={}.",
            now, static_cast<unsigned>(balance.balance_kind), balance.balance, balance.balance_velocity,
            correction, combo_session.model().difficulty(), input.left_x, input.left_y);
    }
    if (pose != previous_pose || failed) {
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE COMBO EVENT: ms={} pose={} active={} failed={} fatigue={} grind={} manual={} native_failure_verified=false.",
            now, static_cast<unsigned>(pose), combo_session.model().state().combo_active, failed,
            combo_session.model().state().fatigue, combo_session.model().state().grind_fatigue,
            combo_session.model().state().manual_fatigue);
        previous_pose = pose;
    }
    publish_balance_hud(combo_session.model().state(), combo_session.model().danger());
}
// Read-only physics-step evidence. A short revert can disappear between client
// ticks; keep bounded transitions, not full memory dumps or guessed event IDs.
struct ComboSample { NativeRequestSample sample; std::uint64_t ms{}; };
std::array<ComboSample, 64> combo_samples{};
std::mutex combo_samples_mutex;
std::size_t combo_head{}, combo_count{};
std::uint64_t combo_dropped{};
NativeRequestSample combo_previous{};
void observe_combo_step(const NoBailSkater& owner, float, bool) noexcept {
    using namespace game::build::v20260929::arcade_native_probe;
    NativeRequestSample sample{};
    if (!memory::peek(owner.context + request_words, sample.words) ||
        !memory::peek(owner.context + selected_state, sample.state) ||
        !memory::peek(owner.rig + crouch_mode, sample.crouch_mode)) return;
    sample.valid = true;
    std::lock_guard lock(combo_samples_mutex);
    if (sample == combo_previous) return;
    combo_previous = sample;
    if (combo_count == combo_samples.size()) { ++combo_dropped; return; }
    combo_samples[(combo_head + combo_count) % combo_samples.size()] = {sample, GetTickCount64()};
    ++combo_count;
}

bool validate_request_probe(std::uintptr_t base) noexcept {
    using namespace game::build::v20260929::arcade_native_probe;
    for (const auto& contract : {crouch_predicate, crouch_producer}) {
        decltype(contract.bytes) actual{};
        if (!memory::read(base + contract.rva, actual) || actual != contract.bytes) return false;
    }
    return true;
}

NativeRequestSample native_request_sample() noexcept {
    using namespace game::build::v20260929::arcade_native_probe;
    NoBailSkater owner{}, after{};
    NativeRequestSample result;
    if (!request_probe_ready || !no_bail_skater(owner) ||
        !memory::peek(owner.context + request_words, result.words) ||
        !memory::peek(owner.context + selected_state, result.state) ||
        !memory::peek(owner.rig + crouch_mode, result.crouch_mode) ||
        !no_bail_skater(after) || owner.core != after.core || owner.context != after.context ||
        owner.rig != after.rig || owner.entity != after.entity) return {};
    result.valid = true;
    return result; // Independent guarded reads, NOT an atomic physics snapshot.
}
}

bool enabled() noexcept { return mode_enabled.load(std::memory_order_acquire); }

void set_enabled(bool value, bool persist) noexcept {
    {
        std::lock_guard lock(ollie_mutex);
        arcade_input.enable(value);
        mode_enabled.store(value, std::memory_order_release);
        ollie_eligible_until.store(0, std::memory_order_release);
        gameplay_valid_until.store(0, std::memory_order_release);
    }
    // A toggle never banks an unfinished combo or inherits a held command.
    combo_session.invalidate();
    combo_tick_at = 0;
    previous_pose = SkatePose::unknown;
    had_combo_owner = false;
    request_capture = {};
    publish_balance_hud({}, 0);
    if (persist) {
        preference_loaded = true;
        profile_runtime::set_local_preference("ArcadeMode", value);
    }
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE MODE: enabled={} synthetic_reset=true.", value);
}

bool start(std::uintptr_t image_base) noexcept {
    if (ready.load(std::memory_order_acquire)) return true;
    if (!image_base) return false;
    score_probe_base = image_base;
    request_probe_ready = validate_request_probe(image_base);
    const bool combo_observer = request_probe_ready && add_no_bail_step_listener(observe_combo_step);
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE COMBO OBSERVER: available={} read_only=true queue_limit=64.", combo_observer);
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE NATIVE PROBE: available={} read_only=true duration_ms=5000 limit=128.", request_probe_ready);
    logging::log(logging::Level::info, logging::Channel::skater,
        "Arcade Mode ready (controller observer and experimental A-to-native-input ollie).");
    ready.store(true, std::memory_order_release);
    return true;
}

void on_client_tick() noexcept {
    if (!ready.load(std::memory_order_acquire)) return;
    try {
        if (!preference_loaded && profile_runtime::local_profile_store_ready()) {
            set_enabled(profile_runtime::local_preference("ArcadeMode").value_or(false), false);
            preference_loaded = true;
        }
        const auto watch = watched_physics_state();
        if (observations.update({watch.valid, watch.state, watch.changes, watch.wipeouts})) {
            const auto& sample = observations.state();
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE STATE: available={} native={} selector_changes={} wipeouts={} (diagnostic sample).",
                sample.valid, sample.state, sample.changes, sample.wipeouts);
        }
        ControllerInput input{};
        DingoSDKOverlayReadControllerInput(&input);
        const auto buttons = input.available ? input.buttons : 0u;
        const auto pressed = buttons & ~previous_buttons;
        previous_buttons = buttons; // Reporting failures must never replay an edge.
        const auto now = GetTickCount64();
        const auto ui = multiplayer::sample_game_ui_state(score_probe_base);
        const bool gameplay = enabled() && ui.available && !ui.in_menu && input.available;
        gameplay_valid_until.store(gameplay ? now + 150 : 0, std::memory_order_release);
        observe_score_hud(score_probe_base, now);
        // Drain a fixed maximum per tick; explicitly report lost evidence.
        std::array<ComboSample, 16> samples{};
        std::size_t sample_count{};
        std::uint64_t dropped{};
        {
            std::lock_guard lock(combo_samples_mutex);
            sample_count = std::min(samples.size(), combo_count);
            for (std::size_t i = 0; i < sample_count; ++i)
                samples[i] = combo_samples[(combo_head + i) % combo_samples.size()];
            combo_head = (combo_head + sample_count) % combo_samples.size();
            combo_count -= sample_count;
            dropped = combo_dropped;
            combo_dropped = 0;
        }
        if (dropped) logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE COMBO GAP: dropped={}.", dropped);
        for (std::size_t i = 0; i < sample_count; ++i) {
            const auto& s = samples[i];
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE COMBO SAMPLE: ms={} state={} mode={} words={:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x}.",
                s.ms, s.sample.state, s.sample.crouch_mode,
                s.sample.words[0], s.sample.words[1], s.sample.words[2], s.sample.words[3],
                s.sample.words[4], s.sample.words[5], s.sample.words[6], s.sample.words[7]);
        }
        const auto native = input.available && request_probe_ready ? native_request_sample() : NativeRequestSample{};
        if (gameplay) update_combo(native, input, watch.valid, watch.wipeouts, now);
        else {
            combo_session.invalidate();
            combo_tick_at = 0;
            publish_balance_hud({}, 0);
        }
        // Bounded numeric read-only HIL heartbeat. Enables fresh reset/state
        // checks without injecting an A press merely to arm a capture.
        if (now >= hil_heartbeat_at && now - hil_heartbeat_at >= 250) {
            hil_heartbeat_at = now;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE HIL STATE: seq={} valid={} state={} mode={}.",
                ++hil_heartbeat_id, native.valid, native.state, native.crouch_mode);
        }
        // State 100/mode 1 is the verified on-board idle baseline. Expire this
        // permission quickly if client ticks, game focus, or owner reads stop.
        ollie_eligible_until.store(gameplay && native.valid && native.state == 100 && native.crouch_mode == 1
            ? now + 150 : 0, std::memory_order_release);
        if (enabled() && request_probe_ready && (pressed & face_buttons[0])) {
            capture_started = now;
            ++capture_id;
            request_capture.arm(now);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE CAPTURE: id={} begin=true.", capture_id);
        }
        const bool capturing = request_capture.active();
        if (capturing && request_capture.sample(now, native)) {
            const auto& sample = request_capture.last();
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE SAMPLE: id={} ms={} valid={} state={} crouch_mode={} words={:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x}.",
                capture_id, now - capture_started, sample.valid, sample.state, sample.crouch_mode,
                sample.words[0], sample.words[1], sample.words[2], sample.words[3],
                sample.words[4], sample.words[5], sample.words[6], sample.words[7]);
        }
        if (capturing && !request_capture.active())
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE CAPTURE: id={} end=true samples={}.", capture_id, request_capture.count());
        for (std::size_t i = 0; i < face_buttons.size(); ++i) {
            if (!(pressed & face_buttons[i])) continue;
            auto& count = press_counts[i];
            if (count < std::numeric_limits<std::uint64_t>::max()) ++count;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE DEV: {} press #{} seen on ReSkate client tick.", button_names[i], count);
            const auto& sample = observations.state();
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE INPUT: {} #{} native_state_available={} native_state={} selector_changes={} wipeouts={}.",
                button_names[i], count, sample.valid, sample.state, sample.changes, sample.wipeouts);
        }
    } catch (...) {
        ollie_eligible_until.store(0, std::memory_order_release);
        gameplay_valid_until.store(0, std::memory_order_release);
    }
}

unsigned remap_xinput(unsigned user, unsigned short& buttons, short& right_x,
                      short& right_y, bool game_input, short left_x, short left_y) noexcept {
    if (user >= 4 || !ready.load(std::memory_order_acquire)) return 0;
    const auto physical_buttons = buttons;
    const auto now = GetTickCount64();
    const auto deadline = ollie_eligible_until.load(std::memory_order_acquire);
    const auto gameplay_deadline = gameplay_valid_until.load(std::memory_order_acquire);
    game_input = game_input && gameplay_deadline >= now && gameplay_deadline - now <= 150 && !(buttons & 0x0010u);
    const bool may_start = game_input && deadline >= now && deadline - now <= 150;
    NativeGesture::Output result{};
    {
        std::lock_guard lock(ollie_mutex);
        result = arcade_input.poll(user, now, buttons, left_x, left_y, right_x, right_y,
            game_input, may_start);
    }
    buttons = result.buttons;
    right_x = result.right_x;
    right_y = result.right_y;
    if (result.started == NativeTrick::ollie) logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE OLLIE: A started native right-stick gesture (experimental).");
    if (result.started == NativeTrick::kickflip || result.started == NativeTrick::heelflip)
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE FLIP: X started candidate={} (experimental).", static_cast<unsigned>(result.started));
    return synthetic_packet_mask(result.phase, buttons != physical_buttons);
}
}
