#pragma once
#include "arcade_classic_input.h"
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
// Preparation is input recognition only; state101 never authorizes actuation.
inline bool manual_revert_preparation(bool valid, bool bail, unsigned state,
        unsigned mode, bool revert_pending) noexcept {
    return valid && !bail && state == 101 && mode == 1 && revert_pending;
}
// The measured native method holds a partial vertical right-stick deflection.
// This class requests that method; contact/score observations prove the pose.
class ManualGesture {
public:
    // confirmed remembers a native match for this request; it is not a fresh
    // pose event and must never replace the combo's independent native observer.
    struct Output { bool owns_stick{}, confirmed{}; std::int16_t y{}; };
    void cancel() noexcept {
        clocked_ = confirmed_ = air_prepared_ = landed_ = wrong_contact_ = false;
        requested_ = 0; stick_pair_ = {}; pad_pair_ = {};
    }
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t lx, std::int16_t ly,
                bool game_input, bool safe_state, SkatePose observed, bool conflicting,
                bool prepare_only = false) noexcept {
        const bool command_conflict = (buttons & 0xf030u) || (buttons & 0x0300u) == 0x0300u;
        const bool can_prepare = prepare_only && !requested_ && observed == SkatePose::riding;
        if (!game_input || (!safe_state && !can_prepare) || conflicting || command_conflict ||
            observed == SkatePose::bail || observed == SkatePose::grind ||
            (clocked_ && now < last_)) {
            cancel(); return {};
        }
        clocked_ = true; last_ = now;
        // A native manual already in progress uses Up/Down for correction.
        // Unknown pose cannot establish that a fresh command is appropriate.
        if (!requested_ && observed != SkatePose::riding && observed != SkatePose::air) {
            cancel(); return {};
        }
        if (requested_) {
            // A command near the apex must retain the proven native hold until
            // landing. Give the native landing actuation its own bounded window;
            // a later rebound must never restart that deadline.
            if (air_prepared_ && !landed_ && !confirmed_) {
                requested_at_ = now;
                landed_ = observed != SkatePose::air;
            }
            const auto wanted = requested_ < 0 ? SkatePose::manual : SkatePose::nose_manual;
            const auto opposite = requested_ < 0 ? SkatePose::nose_manual : SkatePose::manual;
            // A prepared landing can briefly touch one wheel then rebound. The
            // fixture measured17ms contact followed by150ms air. Retain only
            // that bounded initial landing request; intentional trick commands
            // still cancel above, and established manual takeoffs release.
            const bool settling_air = confirmed_ && air_prepared_ && observed == SkatePose::air &&
                now - first_confirmed_at_ < 250;
            // The Nose Manual fixture first touched the opposite wheel17ms,
            // rebounded200ms, then reached the requested pose. Keep only that
            // initial unconfirmed preparation; never call the wrong pose a match.
            if (observed == opposite && air_prepared_ && !confirmed_ && !wrong_contact_) {
                wrong_contact_ = true; wrong_contact_at_ = now;
            }
            const bool brief_wrong_contact = air_prepared_ && !confirmed_ && wrong_contact_ &&
                now - wrong_contact_at_ < 50;
            if ((observed == opposite && !brief_wrong_contact) ||
                (wrong_contact_ && !confirmed_ && now - wrong_contact_at_ >= 250) ||
                (confirmed_ && observed == SkatePose::air && !settling_air)) {
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
        const auto pad = buttons & 15u;
        if ((pad & 3u) == 3u || (pad & 12u) == 12u) {
            stick_pair_ = {}; pad_pair_ = {}; return {};
        }
        // A held steering stick cannot swallow a fresh D-pad Up edge, and
        // switching input devices cannot complete half of an unrelated pair.
        const int pad_request = pad_pair_.poll(now, classic_direction(buttons, 0, 0));
        if (pad) stick_pair_ = {};
        const int stick_request = pad ? 0 : stick_pair_.poll(now, classic_direction(0, lx, ly));
        const int request = pad_request ? pad_request : stick_request;
        if (request) {
            if (!safe_state) { cancel(); return {}; }
            requested_ = request; requested_at_ = now;
            air_prepared_ = observed == SkatePose::air; landed_ = !air_prepared_;
            return {true, false, static_cast<std::int16_t>(requested_ * 20000)};
        }
        return {};
    }
private:
    struct DirectionPair {
        bool armed{};
        int first{}, previous{};
        std::uint64_t at{};
        int poll(std::uint64_t now, TrickDirection direction) noexcept {
            const int vertical = direction == TrickDirection::up ? 1 : direction == TrickDirection::down ? -1 : 0;
            if (!armed) {
                if (direction == TrickDirection::none) armed = true;
                previous = vertical; return 0;
            }
            if (first && now - at > 350) first = 0;
            int request{};
            if (vertical && vertical != previous) {
                if (first == -vertical && now - at <= 350) request = vertical;
                first = vertical; at = now;
            } else if (!vertical && direction != TrickDirection::none) first = 0;
            previous = vertical;
            return request;
        }
    };
    DirectionPair stick_pair_{}, pad_pair_{};
    bool clocked_{}, confirmed_{}, air_prepared_{}, landed_{}, wrong_contact_{};
    int requested_{};
    std::uint64_t last_{}, requested_at_{}, confirmed_at_{}, first_confirmed_at_{}, wrong_contact_at_{};
};
}
