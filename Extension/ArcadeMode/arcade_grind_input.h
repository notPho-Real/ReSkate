#pragma once
#include "arcade_classic_input.h"
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
// Native rail contact decides engagement. Y only holds board alignment/tilt;
// it never writes a rail, physics state, score, or named grind identity.
// Vertical tilt follows the installed native guide and the pinned community
// air-to-rail continuity method recorded in upstream-method-survey.json.
class GrindInput {
public:
    static constexpr std::uint16_t y_button = 0x8000;
    struct Output { bool owns_stick{}, unsupported{}; std::int16_t x{}, y{}; };
    void reset() noexcept { *this = {}; }
    Output poll(std::uint16_t buttons, short lx, short ly, bool gameplay,
                SkatePose pose, bool other_owner) noexcept {
        if (!gameplay) { reset(); return {}; }
        const bool held = (buttons & y_button) != 0;
        if (!held) { armed_ = true; pressed_ = blocked_ = reported_ = false; return {}; }
        if (!armed_ || blocked_) return {};
        if (pose == SkatePose::bail || pose == SkatePose::unknown ||
            pose == SkatePose::manual || pose == SkatePose::nose_manual) {
            blocked_ = true; return {};
        }
        if (!pressed_) {
            pressed_ = true;
            direction_ = classic_direction(buttons, lx, ly);
        }
        const bool supported = direction_ == TrickDirection::none ||
            direction_ == TrickDirection::up || direction_ == TrickDirection::down;
        if (!supported) { const bool report = !reported_; reported_ = true; return {false, report}; }
        if (other_owner || (pose != SkatePose::air && pose != SkatePose::grind)) return {};
        // Hold unchanged across native rail engagement. Full vertical stick is
        // a pop setup; partial pressure supplies board tilt without requesting it.
        const short tilt = direction_ == TrickDirection::up ? 20000 :
            direction_ == TrickDirection::down ? -20000 : 0;
        return {true, false, 0, tilt};
    }
private:
    bool armed_{}, pressed_{}, blocked_{}, reported_{};
    TrickDirection direction_{TrickDirection::none};
};
}
