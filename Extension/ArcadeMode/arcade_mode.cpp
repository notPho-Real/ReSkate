#include "arcade_mode.h"
#include "arcade_observation.h"
#include "arcade_native_probe.h"
#include "arcade_gesture.h"
#include "arcade_rotation.h"
#include "arcade_score_probe.h"
#include "arcade_native_score.h"
#include "arcade_native_events.h"
#include "arcade_native_trick.h"
#include "arcade_authored_gestures.h"
#include "arcade_playback_packet.h"
#include "arcade_camera.h"
#include "arcade_balance_input.h"
#include "arcade_balance_hud.h"
#include "arcade_grind_runtime.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Build/20260929/skater_state.h"
#include "Engine/Game/Build/20260929/no_bail.h"
#include "Engine/Game/Build/20260929/grind_exit.h"
#include "Extension/Skater/no_bail.h"
#include "Extension/Skater/client_source_spawn.h"
#include "Extension/Multiplayer/Remote/native_skater.h"
#include "Extension/Multiplayer/Hud/native_party.h"
#include "Extension/Multiplayer/Hud/follow_camera.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Extension/UI/Overlay/overlay.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Multiplayer/Hud/game_ui_state.h"
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>
#include <string>
#include <stdexcept>
#include <memory>
#include <thread>
#include <Windows.h>

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
std::atomic<std::uint64_t> push_eligible_until{};
std::atomic<std::uint64_t> board_eligible_until{};
std::atomic<std::uint64_t> manual_eligible_until{};
std::atomic<std::uint64_t> action_eligible_until{};
std::atomic<SkatePose> manual_observed{SkatePose::unknown};
std::atomic<float> push_speed{};
std::atomic<bool> authored_mirror_x{};
ArcadeInput arcade_input;
std::mutex ollie_mutex;
std::atomic<PlaybackExperiment> playback_mode{PlaybackExperiment::delivery};
bool playback_diagnostics{}; // Protected by ollie_mutex; off in ordinary gameplay.
std::array<PlaybackPacket, 4> playback_packets;
std::array<std::uint64_t, 4> playback_polls{};
std::atomic<ActionExperiment> action_mode{ActionExperiment::off};
std::array<ActionRuntime, 4> action_inputs;
std::array<AirRotation, 4> rotations;
// Bounded edge evidence at the game-facing input boundary, not requested lab input.
struct PadEvidence {
    std::uint64_t sequence{}, ms{};
    unsigned user{}, buttons{}, lt{}, rt{}, output{}, phase{};
    bool source{};
    std::uint64_t tick{};
    short right_x{}, right_y{};
    bool playback_sample{};
    unsigned playback_mode{}, input_packet{}, output_packet{};
    std::uint64_t poll{};
    bool action_sample{}, action_requested{}, action_owned{};
    unsigned action_mode{}, action_kind{}, action_trick{}, action_result{};
    short left_x{}, left_y{};
    unsigned output_lt{}, output_rt{};
};
std::array<PadEvidence, 64> pad_evidence{};
std::array<PadEvidence, 4> previous_pad{};
std::array<PadEvidence, 4> previous_source_pad{};
std::size_t pad_head{}, pad_count{};
std::uint64_t pad_sequence{}, pad_dropped{};
std::once_flag authored_started;
std::atomic<std::shared_ptr<const std::vector<AuthoredGesture>>> authored_catalog;
std::atomic<std::shared_ptr<const std::vector<PreparedGesture>>> prepared_catalog;
std::atomic<std::uint64_t> input_game_tick{};
CameraOrbit camera_orbit;
std::uint64_t camera_at{};
bool camera_lease_requested{};
std::atomic<unsigned> authored_status{};
std::atomic<unsigned> authored_stage{}, authored_field{}, authored_error{}, authored_rejected{};
std::size_t authored_reported{};
void prepare_authored_catalog() noexcept {
    try {
        std::call_once(authored_started, [] {
            authored_status.store(1, std::memory_order_release);
            std::thread([] {
                AuthoredLoadDiagnostics diagnostic{};
                bool loaded{};
                try {
                    std::vector<wchar_t> path(32768);
                    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
                    if (!length || length >= path.size()) throw std::runtime_error("Game folder unavailable");
                    const auto root = std::filesystem::path(std::wstring(path.data(), length)).parent_path();
                    auto catalog = std::make_shared<const std::vector<AuthoredGesture>>(read_authored_gestures(root, &diagnostic));
                    auto prepared = std::make_shared<const std::vector<PreparedGesture>>(prepare_gestures(*catalog));
                    if (prepared->empty()) throw std::runtime_error("No playable authored gestures");
                    prepared_catalog.store(std::move(prepared), std::memory_order_release);
                    authored_catalog.store(std::move(catalog), std::memory_order_release);
                    loaded = true;
                } catch (...) {
                    if (!diagnostic.error) diagnostic.error = 3;
                }
                authored_stage.store(diagnostic.stage, std::memory_order_release);
                authored_field.store(diagnostic.field, std::memory_order_release);
                authored_error.store(diagnostic.error, std::memory_order_release);
                authored_rejected.store(diagnostic.rejected, std::memory_order_release);
                authored_status.store(loaded ? 2u : 3u, std::memory_order_release);
            }).detach();
        });
    } catch (...) {
        authored_status.store(3, std::memory_order_release);
    }
}
ComboSession combo_session;
std::uint64_t combo_tick_at{};
std::uint64_t observed_wipeouts{};
bool had_combo_owner{};
NoBailSkater combo_owner{};
NativeBailStatus previous_balance_bail{};
SkatePose previous_pose{SkatePose::unknown};
std::uint64_t next_balance_sample{};
void update_combo(const NativeRequestSample& native, const NoBailSkater& owner, const ControllerInput& input,
                  bool watch_valid, std::uint64_t wipeouts, std::uint64_t now,
                  const ScoreLabelOccurrence& label) noexcept {
    const auto score = score_activity();
    const bool valid = native.valid && input.available && watch_valid;
    const bool owner_changed = valid && had_combo_owner && owner != combo_owner;
    if (!valid || owner_changed) {
        cancel_balance_bail();
        combo_session.invalidate();
        combo_tick_at = 0;
    }
    const bool bail = valid && had_combo_owner && !owner_changed && wipeouts != observed_wipeouts;
    had_combo_owner = valid;
    combo_owner = valid ? owner : NoBailSkater{};
    observed_wipeouts = wipeouts;
    const auto pose = observed_pose({valid,
        score.valid && now >= score.sampled_at && now - score.sampled_at <= 150,
        score.active, bail, native.state, native.crouch_mode, native.words[1] & 0xc000u});
    manual_observed.store(pose, std::memory_order_relaxed);
    const bool manual_safe = valid && !bail &&
        ((native.state == 100 && native.crouch_mode == 1) || (native.state >= 200 && native.state < 300));
    manual_eligible_until.store(manual_safe ? now + 150 : 0, std::memory_order_release);
    action_eligible_until.store(action_input_eligible({valid, false, false, bail, native.state,
        native.crouch_mode, 0}) ? now + 150 : 0, std::memory_order_release);
    const float dt = combo_tick_at && now >= combo_tick_at ? static_cast<float>(now - combo_tick_at) / 1000.0f : 0;
    combo_tick_at = now;
    const auto kind = combo_session.model().state().balance_kind;
    const auto correction = balance_correction(kind, input.left_x, input.left_y, input.available);
    const bool was_active = combo_session.model().state().combo_active;
    const bool had_revert = combo_session.model().state().revert_pending;
    const bool native_revert = valid && !owner_changed && !bail && pose == SkatePose::riding &&
        native.state == 100 && score.valid && now >= score.sampled_at && now - score.sampled_at <= 150 &&
        observed_revert_label(label, now);
    const bool failed = combo_session.step(pose, dt, correction, native_revert);
    if (native_revert) {
        const auto& linked = combo_session.model().state();
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE REVERT LINK: ms={} sequence={} labels={} accepted={} remaining_ms={}.",
            now, label.sequence, label.labels, !had_revert && linked.revert_pending,
            static_cast<unsigned>(std::lround(linked.link_grace_s * 1000.0f)));
    }
    if (failed) {
        const auto serial = request_balance_bail(owner);
        {
            std::lock_guard lock(ollie_mutex);
            arcade_input.cancel_commands();
            for (auto& action : action_inputs) action.reset();
            for (auto& rotation : rotations) rotation.reset();
        }
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE BALANCE FAILURE: ms={} kind={} request={}.", now, static_cast<unsigned>(kind), serial);
    }
    const auto& balance = combo_session.model().state();
    // Rate-bound observations continue through a hot session; a lifetime cap
    // would silently remove the evidence needed by later fatigue trials.
    if (balance.balance_kind != BalanceKind::none && now >= next_balance_sample) {
        next_balance_sample = now + 250;
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE BALANCE SAMPLE: ms={} kind={} x={} v={} input={} difficulty={} raw_x={} raw_y={}.",
            now, static_cast<unsigned>(balance.balance_kind), balance.balance, balance.balance_velocity,
            correction, combo_session.model().difficulty(), input.left_x, input.left_y);
    }
    if (pose != previous_pose || failed || was_active != balance.combo_active) {
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
struct ComboSample { NativeRequestSample sample; std::uint64_t ms{}; NativeFlipSelection flip; };
std::array<ComboSample, 64> combo_samples{};
std::mutex combo_samples_mutex;
std::size_t combo_head{}, combo_count{};
std::uint64_t combo_dropped{};
NativeRequestSample combo_previous{};
NativeFlipSelection flip_previous{};
void observe_combo_step(const NoBailSkater& owner, float, bool) noexcept {
    observe_grind_step(owner);
    using namespace game::build::v20260929::arcade_native_probe;
    NativeRequestSample sample{};
    if (!memory::peek(owner.context + request_words, sample.words) ||
        !memory::peek(owner.context + selected_state, sample.state) ||
        !memory::peek(owner.rig + crouch_mode, sample.crouch_mode)) return;
    const auto flip = read_flip_selection([](auto address, auto& value) { return memory::peek(address, value); }, owner.core);
    NoBailSkater after{};
    if (!no_bail_skater(after) || owner.core != after.core || owner.entity != after.entity ||
        owner.context != after.context || owner.rig != after.rig) return;
    sample.valid = true;
    std::lock_guard lock(combo_samples_mutex);
    if (sample == combo_previous && flip == flip_previous) return;
    combo_previous = sample;
    flip_previous = flip;
    if (combo_count == combo_samples.size()) { ++combo_dropped; return; }
    combo_samples[(combo_head + combo_count) % combo_samples.size()] = {sample, GetTickCount64(), flip};
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

NativeRequestSample native_request_sample(NoBailSkater& validated_owner) noexcept {
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
    validated_owner = after;
    return result; // Independent guarded reads, NOT an atomic physics snapshot.
}

// Reuse the board-root layout and owner/vtable checks used by Grind Exit and
// Hall of Meat. This observer never writes physics or scales native velocity.
bool native_board_speed(float& speed) noexcept {
    namespace layout = game::build::v20260929::skater_state;
    NoBailSkater skater{}, after{};
    std::uintptr_t holder{}, physics{}, type{}, bodies{}, owner{};
    std::uint32_t count{};
    std::array<float, 3> velocity{};
    if (!no_bail_skater(skater) ||
        !memory::peek(skater.core + layout::board_holder_offset, holder) ||
        !memory::peek(holder + layout::board_physics_offset, physics) ||
        !memory::peek(physics, type) || type != skater.base + layout::board_physics_vtable ||
        !memory::peek(physics + layout::physics_bodies_offset, bodies) ||
        !memory::peek(bodies, count) || count != layout::board_body_count) return false;
    const auto body = bodies + layout::board_root_body * layout::physics_body_size;
    if (!memory::peek(body + layout::body_owner_offset, owner) || owner != physics ||
        !memory::peek(body + layout::body_velocity_offset, velocity) ||
        !no_bail_skater(after) || skater.core != after.core || skater.entity != after.entity ||
        skater.context != after.context || skater.rig != after.rig) return false;
    for (const auto value : velocity) if (!std::isfinite(value) || std::abs(value) >= 1000) return false;
    speed = std::hypot(velocity[0], velocity[2]);
    return true;
}

bool native_board_toggle_safe(const NativeRequestSample& sample) noexcept {
    namespace layout = game::build::v20260929::skater_state;
    if (!sample.valid) return false;
    if (sample.state == 100 && sample.crouch_mode == 1 && (sample.words[1] & 0xc000u) == 0xc000u) return true;
    if (sample.state != game::build::v20260929::no_bail::offboard_physics_state) return false;
    // Reuse Hall of Meat's native offboard flags to distinguish standing from a bail.
    NoBailSkater owner{}, after{};
    std::uintptr_t trick{}, offboard{};
    std::uint8_t ragdoll{}, airborne{};
    std::uint32_t current{};
    return no_bail_skater(owner) &&
        memory::peek(owner.core + layout::trick_state_offset, trick) &&
        memory::peek(trick + layout::offboard_state_offset, offboard) &&
        memory::peek(offboard + layout::ragdoll_offset, ragdoll) && ragdoll == 0 &&
        memory::peek(offboard + layout::in_the_air_offset, airborne) && airborne == 0 &&
        memory::peek(owner.context + layout::physics_state_offset, current) && current == sample.state &&
        no_bail_skater(after) && owner.core == after.core && owner.context == after.context &&
        owner.rig == after.rig && owner.entity == after.entity;
}
}

bool enabled() noexcept { return mode_enabled.load(std::memory_order_acquire); }

void on_camera_tick(std::uintptr_t base, std::uintptr_t client, bool ready_for_camera, bool phase) noexcept {
    const auto restore = [&]() noexcept {
        camera_orbit.reset(); camera_at = 0;
        try {
            if (camera_lease_requested) {
                std::string detail;
                if (update_arcade_camera(base, client, false, phase, nullptr, detail)) camera_lease_requested = false;
            }
        } catch (...) {}
    };
    try {
        const auto ui = multiplayer::sample_game_ui_state(base);
        if (!enabled() || !prepared_catalog.load(std::memory_order_acquire) || !ready_for_camera ||
            !ui.available || ui.in_menu || multiplayer::party_camera_requested()) {
            restore(); return;
        }
        ControllerInput input{};
        DingoSDKOverlayReadControllerInput(&input);
        if (!input.available || (input.buttons & 0x0010u)) { restore(); return; }
        const auto local = multiplayer::capture_local(base, client, false);
        if (!local.ready) { restore(); return; }
        float fov{};
        const auto follow = multiplayer::follow_camera(UINT64_MAX, local.pose.root, fov);
        auto target = local.pose.root.position;
        target[1] += 1.0f;
        const auto now = GetTickCount64();
        const float dt = camera_at && now >= camera_at ? static_cast<float>(std::min<std::uint64_t>(now - camera_at, 50)) / 1000 : 0;
        camera_at = now;
        const auto transform = camera_orbit.update(follow, target,
            static_cast<float>(input.right_x) / 32767, static_cast<float>(input.right_y) / 32767, dt);
        if (!transform) { restore(); return; }
        std::string detail;
        // Activation can succeed before a subsequent verification fails: always retain restoration intent.
        camera_lease_requested = true;
        if (!update_arcade_camera(base, client, true, phase, &*transform, detail, fov)) restore();
    } catch (...) { restore(); }
}

PlaybackExperiment playback_experiment() noexcept {
    return playback_mode.load(std::memory_order_acquire);
}
ActionExperiment action_experiment() noexcept { return action_mode.load(std::memory_order_acquire); }
bool set_action_experiment(ActionExperiment value) noexcept {
    if (value > ActionExperiment::revert_backside) return false;
    std::lock_guard lock(ollie_mutex);
    if (mode_enabled.load(std::memory_order_acquire)) return false;
    for (auto& input : action_inputs) input.reset();
    action_mode.store(value, std::memory_order_release);
    return true;
}
bool set_playback_experiment(PlaybackExperiment value, bool diagnostics) noexcept {
    if (value != PlaybackExperiment::delivery && value != PlaybackExperiment::timeline &&
        value != PlaybackExperiment::reference) return false;
    std::lock_guard lock(ollie_mutex);
    if (mode_enabled.load(std::memory_order_acquire)) return false;
    arcade_input.set_playback_schedule(value == PlaybackExperiment::delivery ?
        AuthoredPlaybackSchedule::delivery_relative : AuthoredPlaybackSchedule::absolute_ticks);
    playback_mode.store(value, std::memory_order_release);
    playback_diagnostics = diagnostics;
    return true;
}

void set_enabled(bool value, bool persist) noexcept {
    cancel_balance_bail();
    invalidate_grind_observer();
    if (value) prepare_authored_catalog();
    {
        std::lock_guard lock(ollie_mutex);
        arcade_input.enable(value);
        for (auto& input : action_inputs) input.reset();
        for (auto& rotation : rotations) rotation.reset();
        mode_enabled.store(value, std::memory_order_release);
        ollie_eligible_until.store(0, std::memory_order_release);
        gameplay_valid_until.store(0, std::memory_order_release);
        push_eligible_until.store(0, std::memory_order_release);
        board_eligible_until.store(0, std::memory_order_release);
        manual_eligible_until.store(0, std::memory_order_release);
        action_eligible_until.store(0, std::memory_order_release);
        manual_observed.store(SkatePose::unknown, std::memory_order_relaxed);
        previous_pad = {};
        previous_source_pad = {};
    }
    // A toggle never banks an unfinished combo or inherits a held command.
    combo_session.invalidate();
    reset_score_label_occurrences();
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
    input_game_tick.fetch_add(1, std::memory_order_acq_rel);
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
        if (const auto catalog = authored_catalog.load(std::memory_order_acquire)) {
            if (authored_reported < catalog->size()) {
                const auto index = authored_reported++;
                const auto& gesture = (*catalog)[index];
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE AUTHORED GESTURE: index={} total={} id={} nollie={} context={} arcs={}.",
                    index, catalog->size(), gesture.native_id, gesture.nollie,
                    static_cast<unsigned>(gesture.context), gesture.arcs.size());
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE AUTHORED LABEL: index={} tokens={}.", index, gesture.source_tokens);
                // Quantized, bounded game data only; no asset names or memory dumps.
                for (std::size_t arc_index = 0; arc_index < gesture.arcs.size(); ++arc_index) {
                    const auto& arc = gesture.arcs[arc_index];
                    logging::log(logging::Level::info, logging::Channel::skater,
                        "ARCADE AUTHORED ARC: index={} arc={} start={} end={} min={} max={} wait={}.",
                        index, arc_index, static_cast<int>(std::lround(arc.start * 1000)),
                        static_cast<int>(std::lround(arc.end * 1000)),
                        static_cast<int>(std::lround(arc.minimum * 1000)),
                        static_cast<int>(std::lround(arc.maximum * 1000)),
                        static_cast<int>(std::lround(arc.wait * 1000)));
                }
            }
        }
        std::array<PadEvidence, 16> pad_rows{};
        std::size_t pad_rows_count{};
        std::uint64_t pad_lost{};
        {
            std::lock_guard lock(ollie_mutex);
            pad_rows_count = std::min(pad_rows.size(), pad_count);
            for (std::size_t i = 0; i < pad_rows_count; ++i)
                pad_rows[i] = pad_evidence[(pad_head + i) % pad_evidence.size()];
            pad_head = (pad_head + pad_rows_count) % pad_evidence.size();
            pad_count -= pad_rows_count;
            pad_lost = pad_dropped; pad_dropped = 0;
        }
        if (pad_lost) logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE PAD GAP: dropped={}.", pad_lost);
        for (std::size_t i = 0; i < pad_rows_count; ++i) {
            const auto& p = pad_rows[i];
            if (p.source) {
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE PAD SOURCE: seq={} ms={} user={} buttons={} lt={} rt={}.",
                    p.sequence, p.ms, p.user, p.buttons, p.lt, p.rt);
                continue;
            }
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PAD SAMPLE: seq={} ms={} user={} buttons={} lt={} rt={} output={} phase={}.",
                p.sequence, p.ms, p.user, p.buttons, p.lt, p.rt, p.output, p.phase);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE STICK SAMPLE: seq={} ms={} tick={} user={} x={} y={} phase={}.",
                p.sequence, p.ms, p.tick, p.user, p.right_x, p.right_y, p.phase);
            if (p.poll) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PLAYBACK SAMPLE: seq={} poll={} tick={} mode={} input_packet={} output_packet={}.",
                p.sequence, p.poll, p.tick, p.playback_mode, p.input_packet, p.output_packet);
            if (p.action_sample) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE ACTION SAMPLE: seq={} ms={} tick={} user={} mode={} requested={} kind={} trick={} result={} owned={} lx={} ly={} lt={} rt={} packet={}.",
                p.sequence, p.ms, p.tick, p.user, p.action_mode, p.action_requested, p.action_kind,
                p.action_trick, p.action_result, p.action_owned, p.left_x, p.left_y, p.output_lt, p.output_rt, p.output_packet);
        }
        const auto ui = multiplayer::sample_game_ui_state(score_probe_base);
        const bool gameplay = enabled() && ui.available && !ui.in_menu && input.available;
        gameplay_valid_until.store(gameplay ? now + 150 : 0, std::memory_order_release);
        observe_score_hud(score_probe_base, now);
        native_score_tick(score_probe_base, now, gameplay);
        float grind_speed{};
        const bool grind_speed_valid = native_board_speed(grind_speed);
        grind_observer_tick(score_probe_base, now, request_probe_ready && ui.available && !ui.in_menu && input.available,
            enabled(), grind_speed_valid, grind_speed);
        const auto score_edge = take_score_label_occurrence(now);
        if (gameplay && score_edge.valid && score_edge.labels) {
            // History base-name occurrences are independently observed native
            // actions. Amounts and FS/BS context remain separate evidence.
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE EDGE: seq={} observed={} sampled={} labels={} uncertainty={}.",
                score_edge.sequence, score_edge.observed_at, score_edge.sampled_at, score_edge.labels,
                static_cast<unsigned>(score_edge.uncertainty));
        }
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
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE FLIP OBSERVATION: ms={} valid={} id={}.", s.ms, s.flip.valid, s.flip.id);
        }
        NoBailSkater native_owner{};
        const auto native = input.available && request_probe_ready ? native_request_sample(native_owner) : NativeRequestSample{};
        // Reuse the existing guarded stance flag in request_words[0]. On this
        // Deck, the clear-bit stance needs lateral reflection: authored 2/3
        // otherwise produced native 3/2. Other stance contexts remain HIL gates.
        if (native.valid) authored_mirror_x.store(
            !(native.words[0] & game::build::v20260929::grind_exit::stance_switch_mask), std::memory_order_release);
        float speed{};
        // Both wheel contacts exclude the measured manual poses. Unknown state
        // or speed means no push; native down-stick sliding also remains intact.
        const bool speed_valid = native.valid && native.state == 100 && native.crouch_mode == 1 && native_board_speed(speed);
        const bool push_valid = gameplay && watch.valid && speed_valid && (native.words[1] & 0xc000u) == 0xc000u;
        if (push_valid) push_speed.store(speed, std::memory_order_relaxed);
        push_eligible_until.store(push_valid ? now + 150 : 0, std::memory_order_release);
        board_eligible_until.store(gameplay && watch.valid && native_board_toggle_safe(native) ? now + 150 : 0,
            std::memory_order_release);
        if (gameplay) update_combo(native, native_owner, input, watch.valid, watch.wipeouts, now, score_edge);
        else {
            cancel_balance_bail();
            had_combo_owner = false;
            combo_owner = {};
            manual_eligible_until.store(0, std::memory_order_release);
            action_eligible_until.store(0, std::memory_order_release);
            manual_observed.store(SkatePose::unknown, std::memory_order_relaxed);
            combo_session.invalidate();
            combo_tick_at = 0;
            publish_balance_hud({}, 0);
        }
        const auto failure_status = balance_bail_status();
        if (failure_status.serial != previous_balance_bail.serial || failure_status.result != previous_balance_bail.result) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BALANCE BAIL: ms={} request={} result={}.", now, failure_status.serial,
                static_cast<unsigned>(failure_status.result));
            previous_balance_bail = failure_status;
        }
        // Bounded numeric read-only HIL heartbeat. Enables fresh reset/state
        // checks without injecting an A press merely to arm a capture.
        if (now >= hil_heartbeat_at && now - hil_heartbeat_at >= 250) {
            hil_heartbeat_at = now;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE HIL STATE: seq={} valid={} state={} mode={}.",
                ++hil_heartbeat_id, native.valid, native.state, native.crouch_mode);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PUSH OBSERVATION: seq={} ms={} valid={} eligible={} speed={} contacts={}.",
                hil_heartbeat_id, now, speed_valid, push_valid, speed, native.words[1] & 0xc000u);
            const auto score_numbers = score_hud_numbers();
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE NUMBERS: ms={} sampled={} score_valid={} score={} multiplier_valid={} multiplier={} owner_verified=false.",
                now, score_numbers.sampled_at, score_numbers.score_valid, score_numbers.score,
                score_numbers.multiplier_valid, score_numbers.multiplier);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE AUTHORED STATUS: state={}.", authored_status.load(std::memory_order_acquire));
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE AUTHORED DIAGNOSTIC: stage={} field={} error={} rejected={}.",
                authored_stage.load(std::memory_order_acquire), authored_field.load(std::memory_order_acquire),
                authored_error.load(std::memory_order_acquire), authored_rejected.load(std::memory_order_acquire));
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
        push_eligible_until.store(0, std::memory_order_release);
        board_eligible_until.store(0, std::memory_order_release);
    }
}

void observe_xinput_source(unsigned user, unsigned short buttons,
                           unsigned char left_trigger, unsigned char right_trigger) noexcept {
    if (user >= previous_source_pad.size() || !ready.load(std::memory_order_acquire) ||
        !mode_enabled.load(std::memory_order_acquire)) return;
    std::lock_guard lock(ollie_mutex);
    auto& old = previous_source_pad[user];
    if (old.buttons == buttons && old.lt == left_trigger && old.rt == right_trigger) return;
    const PadEvidence row{++pad_sequence, GetTickCount64(), user, buttons,
        left_trigger, right_trigger, 0, 0, true};
    old = row;
    if (pad_count == pad_evidence.size()) ++pad_dropped;
    else {
        pad_evidence[(pad_head + pad_count) % pad_evidence.size()] = row;
        ++pad_count;
    }
}

unsigned remap_xinput(unsigned user, unsigned short& buttons, short& right_x,
                      short& right_y, bool game_input, short& left_x, short& left_y,
                      unsigned char& left_trigger, unsigned char& right_trigger,
                      std::uint32_t physical_packet) noexcept {
    if (user >= 4 || !ready.load(std::memory_order_acquire)) return 0;
    const auto physical_buttons = buttons;
    const auto physical_right_x = right_x, physical_right_y = right_y;
    const auto physical_left_trigger = left_trigger, physical_right_trigger = right_trigger;
    const ActionPhysicalPad action_physical{buttons, left_x, left_y, left_trigger, right_trigger};
    const auto now = GetTickCount64();
    const auto deadline = ollie_eligible_until.load(std::memory_order_acquire);
    const auto gameplay_deadline = gameplay_valid_until.load(std::memory_order_acquire);
    game_input = game_input && gameplay_deadline >= now && gameplay_deadline - now <= 150 && !(buttons & 0x0010u);
    const bool may_start = game_input && deadline >= now && deadline - now <= 150;
    const auto push_deadline = push_eligible_until.load(std::memory_order_acquire);
    const bool may_push = game_input && push_deadline >= now && push_deadline - now <= 150;
    const auto board_deadline = board_eligible_until.load(std::memory_order_acquire);
    const bool may_toggle_board = game_input && board_deadline >= now && board_deadline - now <= 150;
    const auto manual_deadline = manual_eligible_until.load(std::memory_order_acquire);
    const bool manual_safe = game_input && manual_deadline >= now && manual_deadline - now <= 150;
    const auto action_deadline = action_eligible_until.load(std::memory_order_acquire);
    const bool action_safe = game_input && action_deadline >= now && action_deadline - now <= 150;
    NativeGesture::Output result{};
    const auto prepared = prepared_catalog.load(std::memory_order_acquire);
    const bool authored = prepared && !prepared->empty();
    const auto tick = input_game_tick.load(std::memory_order_acquire);
    std::uint32_t packet_mask{};
    {
        std::lock_guard lock(ollie_mutex);
        const bool auxiliary_busy = action_inputs[user].busy();
        const auto action_experiment = action_mode.load(std::memory_order_relaxed);
        const auto observed = manual_observed.load(std::memory_order_relaxed);
        const bool may_exit_grind = game_input && observed == SkatePose::grind;
        result = arcade_input.poll(user, now, buttons, left_x, left_y, right_x, right_y,
            game_input, may_start || may_exit_grind, may_push && !auxiliary_busy, push_speed.load(std::memory_order_relaxed),
            left_trigger > 30 || right_trigger > 30, may_toggle_board, authored, tick,
            left_trigger > 30, prepared ? std::span<const PreparedGesture>(*prepared) : std::span<const PreparedGesture>{},
            authored_mirror_x.load(std::memory_order_acquire), manual_safe && !auxiliary_busy,
            manual_observed.load(std::memory_order_relaxed), action_experiment == ActionExperiment::grabs);
        // The inspected guide stance has a clear native switch bit, also the
        // stance needing authored flip reflection. Do not invert this for grabs.
        // Revert mirroring outside that baseline remains a separate native test.
        const auto action = action_inputs[user].poll(action_experiment, now, action_physical,
            mode_enabled.load(std::memory_order_acquire), action_safe,
            manual_observed.load(std::memory_order_relaxed), authored_mirror_x.load(std::memory_order_acquire),
            result.command_busy);
        result.buttons &= ~action.consume_buttons;
        if (action.consume_rt) right_trigger = 0;
        if (action.pad.owns_right_stick) { result.right_x = action.pad.right_x; result.right_y = action.pad.right_y; }
        if (action.pad.owns_left_stick) { left_x = action.pad.left_x; left_y = action.pad.left_y; }
        if (action.pad.owns_triggers) { left_trigger = action.pad.left_trigger; right_trigger = action.pad.right_trigger; }
        if (action.pad.owns_face_buttons) result.buttons = (result.buttons & ~0xf000u) | action.pad.face_buttons;
        const auto experiment = playback_mode.load(std::memory_order_relaxed);
        const bool reference = experiment == PlaybackExperiment::reference && result.authored_playback;
        const bool consume_nollie = authored && mode_enabled.load(std::memory_order_acquire) && game_input && left_trigger != 0;
        // The reference arm matches upstream's complete scripted pad during the
        // active flip. The timeline arm isolates scheduling with ordinary masking.
        if (reference) { result.buttons = 0; left_x = left_y = 0; left_trigger = right_trigger = 0; }
        else if (consume_nollie && !action.pad.owns_triggers) left_trigger = 0;
        const auto pose = manual_observed.load(std::memory_order_relaxed);
        const bool rotation_enabled = mode_enabled.load(std::memory_order_acquire) && game_input;
        const bool crouched = pose == SkatePose::riding && (physical_buttons & ClassicGesture::a) &&
            result.authored_playback && (result.right_y > 27850 || result.right_y < -27850);
        const auto rotation = rotations[user].poll(now, physical_buttons, rotation_enabled,
            pose, crouched, result.right_y > 0, reference || action.owned || result.phase == 3);
        // BoardChord has already consumed a valid pair and emitted native Y.
        // Single shoulders must not leak to native catch/grab bindings.
        if (rotation_enabled) result.buttons &= ~BoardChord::pair;
        if (rotation.owns_left) { left_x = rotation.x; left_y = rotation.y; }
        observe_grind_pad(user, now, (physical_buttons & GrindInput::y_button) != 0,
            result.right_x, result.right_y, mode_enabled.load(std::memory_order_acquire));
        const auto normal_mask = synthetic_packet_mask(result.phase, result.buttons != physical_buttons || consume_nollie ||
            result.right_x != physical_right_x || result.right_y != physical_right_y) ^
            (authored && result.phase == 1 ? static_cast<unsigned>(tick & 0x00ffffffu) : 0u);
        const auto output_packet = playback_packets[user].sample(physical_packet, physical_packet ^ normal_mask,
            reference || action.owned || action.consume_rt || action.consume_buttons || rotation.owns_left);
        packet_mask = physical_packet ^ output_packet;
        const auto poll = ++playback_polls[user];
        const auto& old = previous_pad[user];
        const bool rotation_sample = rotation.owns_left || (rotation_enabled && (physical_buttons & BoardChord::pair)) || old.action_kind == 5;
        const bool action_owned = action.owned || rotation.owns_left;
        const bool diagnostic = playback_diagnostics && (result.authored_playback || old.playback_sample);
        // Keep one cadence row per game tick plus every changed output. The
        // cumulative poll count exposes extra reads without logging each one.
        // Action cadence is useful only while owning input. Idle rows from
        // multiple controller slots otherwise evict the actual trial evidence.
        if (mode_enabled.load(std::memory_order_acquire) &&
            ((diagnostic && old.tick != tick) || old.buttons != physical_buttons || old.lt != physical_left_trigger || old.rt != physical_right_trigger ||
             old.output != result.buttons || old.phase != result.phase ||
             old.right_x != result.right_x || old.right_y != result.right_y || action.requested ||
             ((action_experiment != ActionExperiment::off || old.action_sample || rotation_sample) &&
              (((action_owned || old.action_owned) && old.tick != tick) || old.left_x != left_x || old.left_y != left_y || old.output_lt != left_trigger ||
               old.output_rt != right_trigger || old.action_owned != action_owned)))) {
            PadEvidence row{++pad_sequence, now, user, physical_buttons, physical_left_trigger,
                physical_right_trigger, result.buttons, result.phase, false, tick, result.right_x, result.right_y,
                playback_diagnostics && result.authored_playback, static_cast<unsigned>(experiment),
                physical_packet, output_packet, diagnostic ? poll : 0};
            row.action_sample = action_experiment != ActionExperiment::off || rotation_sample || old.action_owned;
            row.action_requested = action.requested; row.action_owned = action_owned;
            row.action_mode = static_cast<unsigned>(action_experiment);
            row.action_kind = rotation_enabled && (physical_buttons & BoardChord::pair) ? 5 : action.request_kind;
            row.action_trick = static_cast<unsigned>(action.trick); row.action_result = static_cast<unsigned>(action.result);
            row.left_x = left_x; row.left_y = left_y; row.output_lt = left_trigger; row.output_rt = right_trigger;
            previous_pad[user] = row;
            if (pad_count == pad_evidence.size()) ++pad_dropped;
            else {
                pad_evidence[(pad_head + pad_count) % pad_evidence.size()] = row;
                ++pad_count;
            }
        }
    }
    buttons = result.buttons;
    right_x = result.right_x;
    right_y = result.right_y;
    if (result.started == NativeTrick::ollie) logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE OLLIE: A started native right-stick gesture (experimental).");
    if (result.started == NativeTrick::kickflip || result.started == NativeTrick::heelflip)
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE FLIP: X started candidate={} (experimental).", static_cast<unsigned>(result.started));
    if (result.requested_id || result.unsupported)
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE AUTHORED REQUEST: id={} unsupported={}.", result.requested_id, result.unsupported);
    return packet_mask;
}
}
