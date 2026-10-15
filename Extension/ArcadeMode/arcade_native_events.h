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
    // Fixture evidence 1792042748530552000: 52 ms between rail exit and pop.
    // ComboSession admits this only immediately after an observed grind, bounded
    // to 150 ms. It grants neither input eligibility nor a score event.
    if (sample.state == 701) return SkatePose::grind_exit_transition;
    // Confirmed FS/BS revert trials pass through101 on the ground. Preserve
    // existing grace here; only a separate native label can grant a revert link.
    if (sample.state == 101 && sample.mode <= 1) return SkatePose::riding;
    // Two Deck Ollie traces enter 100/0 then 103/0 before 200/0. This
    // grounded preparation must preserve link grace and accumulated fatigue.
    // It proves neither a manual nor permission to start another native action.
    if ((sample.state == 100 || sample.state == 103) && sample.mode == 0)
        return SkatePose::riding;
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
