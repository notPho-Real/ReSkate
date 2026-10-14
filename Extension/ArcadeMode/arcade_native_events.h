#pragma once
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
// Build-specific adapter, separate from combo dynamics. The scoring activity
// flag alone also fires for ollies; one-wheel contact alone also occurs in turns.
// Their conjunction in riding state is the measured manual observation.
struct NativeComboObservation {
    bool valid{}, score_valid{}, score_active{}, bailed{};
    unsigned state{}, mode{}, contacts{};
};
inline bool action_input_eligible(const NativeComboObservation& sample) noexcept {
    if (!sample.valid || sample.bailed) return false;
    // Revert setup may enter native crouch before its rotation finishes.
    return ((sample.state == 100 || sample.state == 103) && sample.mode == 1) ||
        (sample.state >= 200 && sample.state < 300);
}
inline SkatePose observed_pose(const NativeComboObservation& sample) noexcept {
    if (!sample.valid) return SkatePose::unknown;
    if (sample.bailed) return SkatePose::bail;
    if (sample.state >= 200 && sample.state < 300) return SkatePose::air;
    if (sample.state >= 400 && sample.state < 500) return SkatePose::grind;
    if (sample.state == 100 && sample.mode == 1) {
        if (!sample.score_valid) return SkatePose::unknown;
        if (sample.score_active) {
            if (sample.contacts == 0x4000) return SkatePose::manual;
            if (sample.contacts == 0x8000) return SkatePose::nose_manual;
        }
        return SkatePose::riding;
    }
    // Crouch is grounded: leaving a manual to crouch opens link grace, then
    // the observed takeoff consumes it. It is not itself a new trick.
    if (sample.state == 103 && sample.mode == 1) return SkatePose::riding;
    return SkatePose::unknown;
}
}
