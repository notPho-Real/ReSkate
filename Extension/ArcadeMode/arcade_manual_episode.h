#pragma once
#include "arcade_manual_balance_entry.h"

namespace dingosdk::arcade_mode {
// Sole lifetime owner. Acquisition observers can admit an episode, never end
// one. Only original native frames advance contact/exit evidence. Consumers
// may suspend actuation, but cannot turn missing evidence into physical exit.
class ManualEpisode {
public:
    enum class Phase { inactive, acquiring, active, transitioning, airborne_suspended, combo_airborne, ended };
    enum class Exit { none, disabled, owner, offboard, grind, bail, putdown, incompatible, endpoint };
    void reset() noexcept { phase_=Phase::inactive; exit_=Exit::none; pose_=SkatePose::unknown; last_={}; two_at_=0; successor_={}; owned_=transition_completed_=resume_pending_=uncertain_=reported_=false;command_={}; }
    Phase phase() const noexcept { return phase_; }
    Exit exit() const noexcept { return exit_; }
    std::uint64_t serial() const noexcept { return serial_; }
    std::uint64_t integration_step() const noexcept { return integration_step_; }
    bool established() const noexcept { return phase_==Phase::active || phase_==Phase::transitioning || phase_==Phase::airborne_suspended; }
    bool grounded() const noexcept { return (phase_==Phase::active || phase_==Phase::transitioning) && last_.state!=103; }
    bool transitioning() const noexcept { return phase_==Phase::transitioning; }
    bool airborne() const noexcept { return phase_==Phase::airborne_suspended || phase_==Phase::combo_airborne; }
    bool ended() const noexcept { return phase_==Phase::ended; }
    SkatePose pose() const noexcept { return airborne()?SkatePose::air:last_.state==103?SkatePose::riding:pose_; }
    bool fresh(std::uint64_t now) const noexcept { return last_.valid && now>=last_.sampled_at && now-last_.sampled_at<=150; }
    bool actuatable(std::uint64_t now) const noexcept {
        return !uncertain_ && phase_==Phase::active && fresh(now) && last_.state==100 && last_.mode==1 &&
            last_.contacts==(pose_==SkatePose::manual?0x4000u:0x8000u);
    }
    bool owned() const noexcept { return owned_; }
    void suspend() noexcept { resume_pending_=owned_ && phase_==Phase::active; }
    ManualInputDiagnostic take_resume(std::uint64_t now) noexcept {
        if(!resume_pending_ || !actuatable(now))return {};
        resume_pending_=false;return command_;
    }
    NativeInputFrame termination_frame() const noexcept { return terminal_; }
    bool take_termination() noexcept { if(!ended() || reported_)return false;reported_=true;return true; }
    void end(Exit reason) noexcept { if(phase_!=Phase::inactive && phase_!=Phase::ended){phase_=Phase::ended;exit_=reason;terminal_=last_;reported_=false;successor_={};owned_=resume_pending_=false;} }
    void delivery(const NativeInputFrame& f,const ManualInputDiagnostic& d,bool eligible) noexcept {
        if(!eligible || !f.valid || !d.owns || !d.safe || (d.request!=-1 && d.request!=1))return;
        if(uncertain_ && d.request_at>command_.request_at){phase_=Phase::acquiring;uncertain_=false;owner_=f.owner;generation_=f.owner_generation;}
        if(!established() && phase_!=Phase::combo_airborne){phase_=Phase::acquiring;exit_=Exit::none;owner_=f.owner;generation_=f.owner_generation;}
        if(owner_!=f.owner || generation_!=f.owner_generation)return;
        owned_=true;command_=d;
        if(transitioning() && d.revert.owner==generation_ && d.revert.ack_at &&
            d.revert.step>=entry_step_ && d.request==(pose_==SkatePose::manual?1:-1)) {
            successor_=d.revert; successor_step_=f.physics_step; successor_direction_=d.request;
        }
    }
    void admit(const NativeInputFrame& f,const ManualObservationResult& admission) noexcept {
        if(uncertain_ && admission.admitted && f.owner==owner_){phase_=Phase::acquiring;uncertain_=false;}
        if(established() || !f.valid || !admission.confirmed || balance_kind(admission.pose)!=BalanceKind::manual)return;
        if(ended())return; // A new attributable delivery must first open acquisition.
        if(f.state!=100 || f.mode!=1 || (f.stance_and_push&0x40u) ||
            f.contacts!=(admission.pose==SkatePose::manual?0x4000u:0x8000u))return;
        if(serial_==UINT64_MAX)return;
        ++serial_;phase_=Phase::active;exit_=Exit::none;pose_=admission.pose;
        owner_=f.owner;generation_=f.owner_generation;entry_step_=integration_step_=f.physics_step;last_=f;two_at_=0;
    }
    // Return true only for a positive terminal boundary, including boundaries
    // observed between client ticks. Never infer a putdown from time alone.
    bool observe(const NativeInputFrame& f,bool transition,bool bail=false) noexcept {
        if(!f.valid)return false;
        if(!established() && phase_!=Phase::combo_airborne)return false;
        const auto finish=[&](Exit reason){last_=f;end(reason);return true;};
        if(owner_!=f.owner)return finish(Exit::owner);
        if(bail || f.state==300)return finish(Exit::bail);
        if(f.state==504)return finish(Exit::offboard);
        if(f.rail_family || f.state==701)return finish(Exit::grind);
        if(generation_!=f.owner_generation){
            // Observer epochs also advance on a failed read. Same complete
            // native owner plus a short, matching return can revalidate identity.
            const bool revalidated=f.sampled_at>=last_.sampled_at && f.sampled_at-last_.sampled_at<=150 &&
                f.state==100 && f.mode==1 && f.contacts==(pose_==SkatePose::manual?0x4000u:0x8000u);
            uncertain_=!revalidated;
            generation_=f.owner_generation;resume_pending_=revalidated && owned_;
            if(uncertain_)owned_=false;
        }
        if(f.physics_step<=last_.physics_step)return false;
        const bool consecutive=f.physics_step==last_.physics_step+1 && f.sampled_at>=last_.sampled_at && f.sampled_at-last_.sampled_at<=150;
        last_=f;
        if(f.airborne){
            if(!airborne()){air_at_=f.sampled_at;phase_=Phase::airborne_suspended;}
            if(f.sampled_at>=air_at_ && f.sampled_at-air_at_>=500)phase_=Phase::combo_airborne;
            two_at_=0;successor_={};return false;
        }
        if(phase_==Phase::combo_airborne){phase_=Phase::inactive;owned_=false;return false;}
        if(!transition)transition_completed_=false;
        if(transition && !transition_completed_){phase_=Phase::transitioning;two_at_=0;}
        if(transitioning()){
            const bool linked=successor_.owner==generation_ && successor_.ack_at && f.sampled_at>=successor_.ack_at &&
                f.sampled_at-successor_.ack_at<=1100 && f.physics_step>successor_step_ && f.physics_step>successor_.step &&
                f.contact_generation>successor_.contact && (f.stance_and_push&0x20u)!=successor_.previous_stance &&
                f.state==100 && f.mode==1 && f.contacts==(successor_direction_<0?0x4000u:0x8000u);
            if(linked){integration_step_=f.physics_step;pose_=successor_direction_<0?SkatePose::manual:SkatePose::nose_manual;phase_=Phase::active;successor_={};transition_completed_=true;return false;}
            if(transition)return false;
            // An expired link is not a physical exit. Fresh incompatible state
            // or a sustained two-truck observation below must establish it.
            phase_=Phase::active;
        }
        if(f.state==103){phase_=Phase::active;two_at_=0;return false;}
        if(f.state!=100 || f.mode!=1 || (f.stance_and_push&0x40u)){end(Exit::incompatible);return true;}
        const auto wanted=pose_==SkatePose::manual?0x4000u:0x8000u;
        if(f.contacts==wanted){phase_=Phase::active;two_at_=0;return false;}
        if(f.contacts==0xc000u){
            if(!two_at_ || !consecutive)two_at_=f.sampled_at;
            // Multiple consecutive ORIGINAL physics observations, no observer
            // gaps and no revert/hop, establish a physical two-truck putdown.
            if(f.sampled_at-two_at_>=100){end(Exit::putdown);return true;}
        }else {two_at_=0;if(f.contacts==0x4000u || f.contacts==0x8000u){end(Exit::incompatible);return true;}}
        return false;
    }
private:
    Phase phase_{Phase::inactive};Exit exit_{Exit::none};SkatePose pose_{SkatePose::unknown};
    NativeInputOwner owner_{};NativeInputFrame last_{};ManualRevertContact successor_{};
    std::uint64_t serial_{},generation_{},entry_step_{},integration_step_{},two_at_{},air_at_{},successor_step_{};
    int successor_direction_{};bool owned_{},transition_completed_{},resume_pending_{},uncertain_{},reported_{};
    NativeInputFrame terminal_{};
    ManualInputDiagnostic command_{};
};
}
