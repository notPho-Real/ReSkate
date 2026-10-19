#pragma once
#include "arcade_manual_input.h"
#include "arcade_native_input_state.h"
#include "arcade_manual_observation.h"

namespace dingosdk::arcade_mode {
// Physical classification shared by the production acquisition/release join.
inline SkatePose manual_acquisition_pose(const NativeInputFrame& frame, bool bail = false) noexcept {
    if (!frame.valid) return SkatePose::unknown;
    return bail ? SkatePose::bail :
        (frame.state == 200 || frame.state == 201) ? SkatePose::air :
        frame.rail_family ? SkatePose::grind :
        frame.state == 701 ? SkatePose::grind_exit_transition :
        manual_contact_pose({frame.valid,false,false,bail,frame.state,frame.mode,frame.contacts},false);
}
// Bounded cleanup for a delivered basic Manual request that acquired native
// contact without an independently admitted balance lifecycle. Contact cannot
// create identity or a meter; expiry requests the existing putdown adapter.
class ManualAcquisition {
public:
    void reset() noexcept { active_ = grounded_ = acquired_ = exiting_ = delivered_ = false; }
    void observe(std::uint64_t now, const NativeInputFrame& frame,
                 const ManualInputDiagnostic& command, bool eligible) noexcept {
        if (!eligible || !fresh(now,frame) || nonmanual_boundary(frame)) { reset(); return; }
        if (active_ && !same_owner(frame)) reset();
        delivered_ = command.owns;
        if (!command.owns && !acquired_) { reset(); return; }
        if (!command.owns || (command.request != -1 && command.request != 1) || command.request_at > now) return;
        if (!active_) {
            active_ = true; owner_ = frame.owner; generation_ = frame.owner_generation;
            began_ = command.request_at; requested_ = command.request;
            grounded_ = !frame.airborne;
        }
        // Existing ManualActuation starts its separate1200ms acquisition lease
        // at the first grounded execution. Adopt that transition once, never a
        // newer poll time during unknown contact or a landing rebound.
        if (!grounded_ && frame.state == 100) {
            grounded_ = true;
            began_ = command.request_at;
        }
    }
    bool step(std::uint64_t now, const NativeInputFrame& frame, SkatePose physical,
              bool balance_admitted, bool valid, bool pushing = false) noexcept {
        if (!active_) return false;
        if (!valid || !fresh(now,frame) || !same_owner(frame) || nonmanual_boundary(frame) || pushing || now < began_) { reset(); return false; }
        const auto wanted = requested_ < 0 ? SkatePose::manual : SkatePose::nose_manual;
        if (physical == wanted) acquired_ = true;
        else if (acquired_ && (physical == SkatePose::riding || physical == SkatePose::air ||
                physical == SkatePose::bail || physical == SkatePose::grind ||
                physical == SkatePose::manual || physical == SkatePose::nose_manual || frame.state == 103)) {
            reset(); return false;
        }
        if (acquired_ && !balance_admitted && now-began_ >= ManualActuation::acquisition_ms) exiting_ = true;
        return exiting_;
    }
    bool preserves_history(std::uint64_t now, const NativeInputFrame& frame) const noexcept {
        return active_ && delivered_ && !exiting_ && same_owner(frame) && fresh(now,frame) &&
            frame.state == 100 && frame.mode == 1 && now >= began_ && now-began_ < ManualActuation::acquisition_ms;
    }
private:
    static bool nonmanual_boundary(const NativeInputFrame& frame) noexcept {
        // Positive native rail/exit evidence ends this acquired contact even
        // when a withdrawn input command no longer supplies a physical pose.
        return frame.rail_family || frame.state == 701;
    }
    static bool fresh(std::uint64_t now, const NativeInputFrame& f) noexcept {
        return f.valid && f.owner.valid() && f.owner_generation && f.physics_step &&
            now >= f.sampled_at && now-f.sampled_at <= 150;
    }
    bool same_owner(const NativeInputFrame& frame) const noexcept {
        return owner_ == frame.owner && generation_ == frame.owner_generation;
    }
    NativeInputOwner owner_{};
    std::uint64_t generation_{}, began_{};
    int requested_{};
    bool active_{}, grounded_{}, acquired_{}, exiting_{}, delivered_{};
};
}
