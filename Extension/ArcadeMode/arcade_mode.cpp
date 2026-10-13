#include "arcade_mode.h"
#include "arcade_observation.h"
#include "arcade_native_probe.h"
#include "arcade_ollie.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Extension/Skater/no_bail.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Extension/UI/Overlay/overlay.h"
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <string>

namespace dingosdk::arcade_mode {
namespace {
std::atomic<bool> ready{};
// Only touched by the client thread, after initialization has been published.
std::uint32_t previous_buttons{};
ObservationTracker observations;
std::array<std::uint64_t, 4> press_counts{};
constexpr std::array<std::uint32_t, 4> face_buttons{0x1000u, 0x2000u, 0x4000u, 0x8000u};
constexpr std::array<char, 4> button_names{'A', 'B', 'X', 'Y'};
NativeRequestCapture request_capture;
bool request_probe_ready{};
std::uint64_t capture_started{}, capture_id{};
std::atomic<std::uint64_t> ollie_eligible_until{};
std::array<OllieGesture, 4> ollie_gestures{};
std::mutex ollie_mutex;

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

bool start(std::uintptr_t image_base) noexcept {
    if (ready.load(std::memory_order_acquire)) return true;
    if (!image_base) return false;
    request_probe_ready = validate_request_probe(image_base);
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
        const auto native = input.available && request_probe_ready ? native_request_sample() : NativeRequestSample{};
        // State 100/mode 1 is the verified on-board idle baseline. Expire this
        // permission quickly if client ticks, game focus, or owner reads stop.
        ollie_eligible_until.store(native.valid && native.state == 100 && native.crouch_mode == 1
            ? now + 150 : 0, std::memory_order_release);
        if (request_probe_ready && (pressed & face_buttons[0])) {
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
            try {
                overlay::notify(overlay::NoticeLevel::info, "ARCADE MODE DEV",
                    std::string(1, button_names[i]) + " press #" + std::to_string(count) +
                    " seen on ReSkate client tick.");
            } catch (...) { /* A lost debug notice must not interrupt other buttons or the tick. */ }
        }
    } catch (...) { ollie_eligible_until.store(0, std::memory_order_release); }
}

unsigned remap_xinput(unsigned user, unsigned short& buttons, short right_x,
                      short& right_y, bool game_input) noexcept {
    if (user >= ollie_gestures.size() || !ready.load(std::memory_order_acquire)) return 0;
    const auto now = GetTickCount64();
    const auto deadline = ollie_eligible_until.load(std::memory_order_acquire);
    const bool may_start = game_input && deadline >= now && deadline - now <= 150;
    const bool manual_stick = (right_x < -8689 || right_x > 8689 || right_y < -8689 || right_y > 8689);
    OllieGesture::Output result{};
    {
        std::lock_guard lock(ollie_mutex);
        result = ollie_gestures[user].poll(now, buttons, right_y,
            game_input, may_start, manual_stick);
    }
    buttons = result.buttons;
    right_y = result.right_y;
    if (result.started) logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE OLLIE: A started native right-stick gesture (experimental).");
    return result.phase;
}
}
