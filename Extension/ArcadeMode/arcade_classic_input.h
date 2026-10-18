#pragma once
#include "arcade_authored_playback.h"
#include "arcade_native_mapping.h"

namespace dingosdk::arcade_mode {
struct PreparedGesture {
    std::uint8_t native_id{};
    bool nollie{};
    GestureContext context{};
    std::vector<AuthoredStickStep> steps;
    std::vector<AuthoredStickStep> air_steps;
    std::vector<AuthoredStickStep> ready_air_steps;
    PreparedGesture(std::uint8_t id, bool from_nollie, GestureContext from_context,
                    std::vector<AuthoredStickStep> ground, std::vector<AuthoredStickStep> air_path = {})
        : native_id(id), nollie(from_nollie), context(from_context), steps(std::move(ground)),
          air_steps(compile_airborne_flick(air_path.empty() ? steps : air_path)),
          ready_air_steps(compile_airborne_flick(air_path.empty() ? steps : air_path, false)) {}
};
inline std::vector<PreparedGesture> prepare_gestures(std::span<const AuthoredGesture> catalog) {
    std::vector<PreparedGesture> result;
    for (const auto& gesture : catalog) {
        const bool full_pop = gesture.context == GestureContext::ground && (gesture.native_id == 1 || gesture.native_id == 16);
        auto steps = compile_authored_flick(gesture, 24, full_pop);
        if (!steps.empty()) result.push_back({gesture.native_id, gesture.nollie, gesture.context,
            std::move(steps), compile_authored_flick(gesture, 24, true)});
    }
    return result;
}
inline constexpr std::uint16_t classic_a = 0x1000, classic_x = 0x4000;
}
