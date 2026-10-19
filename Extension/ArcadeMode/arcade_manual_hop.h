#pragma once
#include "arcade_combo_session.h"
#include "arcade_native_input_state.h"
#include "arcade_input_types.h"

namespace dingosdk::arcade_mode {
// Passive balance-history continuity for one admitted basic jump. It does not
// recognize input, choose native physics state, emit a pad or prolong ownership.
class ManualHopContinuity {
public:
    void reset() noexcept { active_ = released_ = authored_ = false; }
    bool active() const noexcept { return active_; }
    void command(std::uint64_t now, const NativeInputFrame& current, const NativeInputFrame& origin,
                 SkatePose pose, bool balance_admitted, const NativeGesture::Output& output,
                 bool held_a, bool eligible) noexcept {
        if (!eligible || !current.valid) { reset(); return; }
        const bool manual = pose == SkatePose::manual || pose == SkatePose::nose_manual;
        const bool nose = pose == SkatePose::nose_manual;
        if (balance_admitted && origin.valid && manual && held_a && output.authored_playback &&
            output.requested_id == (nose ? 16 : 1) && current.state == 100 && current.mode == 1 &&
            current.owner.valid() && current.owner_generation && current.contact_generation && current.action_generation &&
            current.physics_step && origin.owner == current.owner && origin.state == 100 && origin.mode == 1 &&
            origin.owner_generation == current.owner_generation && origin.contact_generation == current.contact_generation &&
            origin.action_generation == current.action_generation && now >= current.sampled_at && now-current.sampled_at <= 150 &&
            now >= origin.sampled_at && now-origin.sampled_at <= 150) {
            owner_ = current.owner; generation_ = current.owner_generation;
            began_ = now; nose_ = nose; active_ = true; released_ = false;
        }
        if (!active_) return;
        if (owner_ != current.owner || generation_ != current.owner_generation) { reset(); return; }
        if (output.jump.released) {
            if (!output.jump.setup_admitted || output.jump.from_rail || output.jump.nollie != nose_ ||
                output.jump.pressed_at != began_ || output.jump.released_at < began_ || output.jump.released_at > now) {
                reset(); return;
            }
            released_ = true;
        }
        if ((released_ && held_a) || (!released_ && (!held_a || !output.authored_playback))) {
            reset(); return;
        }
        authored_ = output.authored_playback;
    }
    bool preserve(const NativeInputFrame& frame, SkatePose pose, bool valid) noexcept {
        if (!active_) return false;
        if (!valid || !frame.valid || owner_ != frame.owner || generation_ != frame.owner_generation ||
            pose == SkatePose::unknown || pose == SkatePose::bail) { reset(); return false; }
        // Once real takeoff is observed, the existing airborne stop path owns
        // continuity. No elapsed-time prediction can assert that takeoff.
        if (frame.state == 200 || frame.state == 201) { reset(); return false; }
        if (frame.state == 103) return true;
        if (frame.state == 100 && authored_) return pose == SkatePose::riding;
        reset(); return false;
    }
private:
    NativeInputOwner owner_{};
    std::uint64_t generation_{}, began_{};
    bool active_{}, released_{}, authored_{}, nose_{};
};
inline PhysicalBalanceSample manual_physics_pose(ManualPhysics& physics, SkatePose pose,
        float dt, const PhysicalManualControl& control, bool admitted_hop = false) noexcept {
    if (balance_kind(pose) == BalanceKind::manual) {
        if (!physics.closed()) physics.begin(pose == SkatePose::nose_manual);
        return physics.sample(dt,control);
    }
    if ((pose == SkatePose::riding && !admitted_hop) || pose == SkatePose::bail || pose == SkatePose::unknown) physics.reset();
    else physics.stop();
    return {true,false};
}
}
