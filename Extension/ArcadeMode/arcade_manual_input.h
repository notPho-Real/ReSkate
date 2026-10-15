#pragma once
#include "arcade_classic_input.h"
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
// The measured native method holds a partial vertical right-stick deflection.
// This class requests that method; contact/score observations prove the pose.
class ManualGesture {
public:
    // confirmed remembers a native match for this request; it is not a fresh
    // pose event and must never replace the combo's independent native observer.
    struct Output { bool owns_stick{}, confirmed{}; std::int16_t y{}; };
    void cancel() noexcept {
        armed_ = clocked_ = confirmed_ = air_prepared_ = false;
        first_ = previous_ = requested_ = 0;
    }
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t lx, std::int16_t ly,
                bool game_input, bool safe_state, SkatePose observed, bool conflicting) noexcept {
        const bool command_conflict = (buttons & 0xf030u) || (buttons & 0x0300u) == 0x0300u;
        if (!game_input || !safe_state || conflicting || command_conflict ||
            observed == SkatePose::bail || observed == SkatePose::grind ||
            (clocked_ && now < last_)) {
            cancel(); return {};
        }
        clocked_ = true; last_ = now;
        const auto direction = classic_direction(buttons, lx, ly);
        const int vertical = direction == TrickDirection::up ? 1 : direction == TrickDirection::down ? -1 : 0;
        // A native manual already in progress uses Up/Down for correction.
        // Unknown pose cannot establish that a fresh command is appropriate.
        if (!requested_ && observed != SkatePose::riding && observed != SkatePose::air) {
            cancel(); return {};
        }
        if (!armed_) {
            if (direction == TrickDirection::none) armed_ = true;
            previous_ = vertical;
            return {};
        }
        if (requested_) {
            const auto wanted = requested_ < 0 ? SkatePose::manual : SkatePose::nose_manual;
            const auto opposite = requested_ < 0 ? SkatePose::nose_manual : SkatePose::manual;
            // A prepared landing can briefly touch one wheel then rebound. The
            // fixture measured17ms contact followed by150ms air. Retain only
            // that bounded initial landing request; intentional trick commands
            // still cancel above, and established manual takeoffs release.
            const bool settling_air = confirmed_ && air_prepared_ && observed == SkatePose::air &&
                now - first_confirmed_at_ < 250;
            if (observed == opposite || (confirmed_ && observed == SkatePose::air && !settling_air)) {
                cancel(); return {};
            }
            if (observed == wanted) {
                if (!confirmed_) first_confirmed_at_ = now;
                confirmed_ = true; confirmed_at_ = now;
            }
            // Brief observer gaps must not drop a held native manual. A failed
            // request or an observed exit always has a bounded release path.
            if ((!confirmed_ && now - requested_at_ >= 1200) ||
                (confirmed_ && now - confirmed_at_ >= (settling_air ? 200u : 150u))) {
                cancel(); return {};
            }
            return {true, confirmed_, static_cast<std::int16_t>(requested_ * 20000)};
        }
        if (first_ && now - first_at_ > 350) first_ = 0;
        if (vertical && vertical != previous_) {
            if (first_ == -vertical && now - first_at_ <= 350) {
                requested_ = vertical; requested_at_ = now; first_ = 0;
                air_prepared_ = observed == SkatePose::air;
                previous_ = vertical;
                return {true, false, static_cast<std::int16_t>(requested_ * 20000)};
            }
            first_ = vertical; first_at_ = now;
        } else if (!vertical && direction != TrickDirection::none) first_ = 0;
        previous_ = vertical;
        return {};
    }
private:
    bool armed_{}, clocked_{}, confirmed_{}, air_prepared_{};
    int first_{}, previous_{}, requested_{};
    std::uint64_t last_{}, first_at_{}, requested_at_{}, confirmed_at_{}, first_confirmed_at_{};
};
}
