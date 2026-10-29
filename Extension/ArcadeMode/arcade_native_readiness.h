#pragma once
#include "arcade_native_input_state.h"
#include "arcade_native_events.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"

namespace dingosdk::arcade_mode {
struct NativeInputPermissions {
    bool gameplay{}, start{}, push{}, board{}, drive{}, manual{}, prepare{}, releasing{}, action{};
    float speed{};
};
struct NativeInputAdmission : NativeInputPermissions {
    bool mirror_x{}, offboard{}, flip_ready{}, matched{};
};
// Publish/consume under the existing input mutex. A client-derived permission
// keeps the physical frame that justified it, rather than borrowing a new owner
// or selection merely because its time lease is still live.
class NativeInputReadiness {
public:
    NativeInputFrame observed_proof()const noexcept{return proof_;}
    void reset() noexcept { proof_ = {}; permissions_ = {}; }
    // Withholding mapped controls is distinct from admitting an action. An
    // original focused client proof can reserve them during a missing physics
    // observation, but cannot renew its150ms lease or cross a known owner exit.
    bool owns_controls(std::uint64_t now,std::uint64_t owner,const NativeInputFrame& current) const noexcept {
        return permissions_.gameplay && proof_.valid && proof_.owner.valid() && owner &&
            proof_.owner_generation==owner && now>=proof_.sampled_at && now-proof_.sampled_at<=150 &&
            proof_.state!=game::build::v20260929::arcade_native_probe::offboard_state &&
            (!current.valid || (current.owner==proof_.owner && current.owner_generation==owner &&
                current.state!=game::build::v20260929::arcade_native_probe::offboard_state));
    }
    void publish(NativeInputFrame proof, NativeInputPermissions permissions) noexcept {
        proof_ = proof; permissions_ = proof.valid ? permissions : NativeInputPermissions{};
    }
    NativeInputAdmission sample(std::uint64_t now, const NativeInputFrame& current) const noexcept {
        NativeInputAdmission out{};
        const auto fresh = [now](const NativeInputFrame& f) {
            return f.valid && f.owner.valid() && f.owner_generation && f.contact_generation && f.action_generation &&
                now >= f.sampled_at && now-f.sampled_at <= 150;
        };
        if (!fresh(proof_) || !fresh(current) || proof_.owner != current.owner ||
            proof_.owner_generation != current.owner_generation || !permissions_.gameplay) return out;
        out.gameplay = true;
        // Stance and immediate native exclusions always come from the same
        // current owner frame used by the facade, including between client ticks.
        out.mirror_x = !(current.stance_and_push & game::build::v20260929::grind_exit::stance_switch_mask);
        out.offboard = current.state == game::build::v20260929::arcade_native_probe::offboard_state;
        // This is the existing native action predicate, not clip readiness. It
        // can be reevaluated from the current frame so own selection/crouch
        // acknowledgments do not cancel an already admitted actor.
        out.action = permissions_.action && action_input_eligible({true,false,false,false,current.state,current.mode,0});
        // Generic physical Manual safety has no selection/stance identity. Reuse
        // the existing predicate on the current frame so a prepared landing is
        // not erased by its own contact transition. Component phase authority
        // remains independently bound by balance_admission().
        out.manual = permissions_.manual && ((current.state == 100 && current.mode == 1) ||
            (current.state >= 200 && current.state < 300));
        out.flip_ready = out.action && current.flip_known && current.flip_id == -1 && current.airborne;
        out.matched = proof_.contact_generation == current.contact_generation &&
            proof_.action_generation == current.action_generation;
        if (out.matched) {
            out.start = permissions_.start; out.push = permissions_.push;
            out.board = permissions_.board; out.drive = permissions_.drive;
            out.prepare = permissions_.prepare;
            out.releasing = permissions_.releasing; out.speed = permissions_.speed;
        }
        return out;
    }
private:
    NativeInputFrame proof_{};
    NativeInputPermissions permissions_{};
};
}
