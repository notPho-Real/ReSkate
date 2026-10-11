#include "arcade_mode.h"
#include "arcade_observation.h"
#include "Extension/Skater/no_bail.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Extension/UI/Overlay/overlay.h"
#include <array>
#include <atomic>
#include <limits>
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
}

bool start(std::uintptr_t image_base) noexcept {
    if (ready.load(std::memory_order_acquire)) return true;
    if (!image_base) return false;
    logging::log(logging::Level::info, logging::Channel::skater,
        "Arcade Mode ready (development controller and native-state observer only).");
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
    } catch (...) { /* Debug observation is never worth the client tick. */ }
}
}
