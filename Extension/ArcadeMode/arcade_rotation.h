#pragma once
#include "arcade_board.h"
#include "arcade_combo_session.h"
#include "arcade_gameplay_profile.h"
#include <algorithm>
#include <cmath>

namespace dingosdk::arcade_mode {
// Native LS spin/prewind method adapted from GoalTrains/reskatedualstick,
// src/translate.h at1181aa819bea2eb49042b3f67e6ff5c7c9c4e348 (GPL-3.0).
// Only controller translation is reused; angular motion remains game-owned.
class AirRotation {
public:
    struct Output { bool owns_left{}; std::int16_t x{}, y{}; float yaw_rate{}; };
    void reset() noexcept { *this = {}; }
    Output poll(std::uint64_t now, std::uint16_t buttons, bool gameplay,
                SkatePose pose, bool crouched, bool native_nollie, bool conflict = false,
                bool native_rate_control = false) noexcept {
        if (!gameplay || (clocked_ && now < previous_time_)) { reset(); return {}; }
        clocked_ = true; previous_time_ = now;
        const auto held = static_cast<std::uint16_t>(buttons & BoardChord::pair);
        if (!held) { armed_ = true; blocked_ = waiting_ = active_ = was_air_ = false; return {}; }
        if (!armed_ || blocked_) return {};
        // Both shoulders always defer to BoardChord, including a late second
        // press. Neither partial release nor a held pair can restart rotation.
        if (held == BoardChord::pair || conflict ||
            (pose != SkatePose::riding && pose != SkatePose::air)) {
            blocked_ = true; active_ = false; return {};
        }
        if (!waiting_) { waiting_ = true; pressed_at_ = now; }
        if (now - pressed_at_ <= BoardChord::window_ms) return {};
        const bool air = pose == SkatePose::air;
        if (was_air_ && !air) { blocked_ = true; active_ = false; return {}; }
        if (!active_) {
            if (!air && !crouched) return {};
            active_ = true; began_ = now; prewind_ = !air; nollie_ = native_nollie;
        }
        if (air) was_air_ = true;
        if (crouched || air) context_at_ = now;
        if (!air && !crouched && now - context_at_ > 250) {
            blocked_ = true; active_ = false; return {};
        }
        const float side = held == BoardChord::lb ? -1.0f : 1.0f;
        if (air && native_rate_control) return {true, 0, 0, side * GameplayProfile{}.angular_speed()};
        float x = side * .97f, y = 0;
        const auto elapsed = now - began_;
        if (prewind_ && elapsed < 90) {
            const float start_y = nollie_ ? 1.0f : -1.0f;
            if (elapsed < 24) { x = 0; y = start_y * static_cast<float>(elapsed) / 24.0f; }
            else {
                const float u = std::min(1.0f, static_cast<float>(elapsed - 24) / 36.0f);
                x = side * .8f * u; y = start_y * (1.0f - .4f * u);
                const float scale = .97f / std::sqrt(x * x + y * y);
                x *= scale; y *= scale;
            }
        }
        return {true, static_cast<std::int16_t>(x * 32767), static_cast<std::int16_t>(y * 32767)};
    }
private:
    bool armed_{}, clocked_{}, blocked_{}, waiting_{}, active_{}, was_air_{}, prewind_{}, nollie_{};
    std::uint64_t previous_time_{}, pressed_at_{}, began_{}, context_at_{};
};
}
