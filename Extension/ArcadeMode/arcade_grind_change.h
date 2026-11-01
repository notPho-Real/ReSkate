#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_native_input_state.h"
#include "arcade_grind_contact.h"
#include "arcade_grind_transition_lifetime.h"
#include "arcade_grind_observer.h"
#include "arcade_grind_admission.h"

namespace dingosdk::arcade_mode {
// Bounded input executor for independently observed 401/20 ->406 ->403
// held sweeps ending in native23 (Nosegrind),32 (Crooked) or24 (5-0), and
// the measured neutral 403/24 ->406 ->401/20 return and lateral tail
// 403/24 ->406 ->402/31 Bluntslide and 403/23 ->406 ->402/21 Noseslide. Other named bindings
// and source contexts have no qualified control recipe yet.
// This owns only RS and the scoped native admission lease. Native completion,
// contact and animation are observations; none is synthesized by this actor.
class GrindChange {
public:
    struct Pad { bool owns{}; std::int16_t x{},y{}; };
    struct Evidence {
        std::uint64_t serial{},first_input_at{},last_input_at{},started_at{};
        bool completed{},physics_changed{},animation_changed{},source_animation_known{};
    };
    void cancel() noexcept { active_=false; admission_={}; source_={}; frame_={}; stable_=false; source_animation_known_=false; }
    bool active() const noexcept { return active_; }
    std::uint64_t serial() const noexcept { return active_?serial_:0; }
    const Evidence& evidence() const noexcept { return evidence_; }
    InputRailState admission() const noexcept { return admission_; }
    // Repeated game-facing polls may not advance the establishment clock.
    void observe(std::uint64_t now,const NativeInputFrame& frame,const NativeGrindContact& contact,
                 bool permitted,const NativeGrindSnapshot& animation={}) noexcept {
        admission_={};
        if(!permitted || !fresh(now,frame) || (frame_.valid &&
            (frame.owner!=frame_.owner || frame.owner_generation!=frame_.owner_generation ||
             frame.physics_step<frame_.physics_step || frame.sampled_at<frame_.sampled_at ||
             frame.sampled_at-frame_.sampled_at>150))) {
            cancel();return;
        }
        frame_=frame;
        if(active_) {
            if(now<evidence_.started_at || now-evidence_.started_at>=1500 ||
                (frame.state!=source_.state && frame.state!=406 && frame.state!=target_state_) ||
                (frame.state!=406 && (!contact.valid || contact.state!=frame.state || !same_segment(contact)))) cancel();
            return;
        }
        source_animation_known_=animation.valid && animation.physics_state==contact.state &&
            animation.sampled_at>=frame.sampled_at && animation.sampled_at<=now &&
            animation.grind_type_valid && animation.grind_type==contact.identity &&
            animation.physics_grind_type_valid && animation.physics_grind_type==contact.identity;
        if(!qualified(contact) || frame.state!=contact.state) { stable_=false;return; }
        if(!stable_ || !same_segment(contact) || source_.actor!=contact.actor ||
            source_.state!=contact.state || source_.identity!=contact.identity) {
            if(generation_==UINT64_MAX)return;
            ++generation_;source_=contact;stable_at_=frame.sampled_at;stable_=true;
        }
        if(frame.sampled_at<stable_at_ || frame.sampled_at-stable_at_<80)return;
        admission_.phase=InputRailPhase::established;
        admission_.generation=generation_;admission_.gameframe=frame.physics_step;
        admission_.excluded_variant=grind_change_variant(source_.state==403?
            (source_.identity==23?GrindChangeTarget::nosegrind:GrindChangeTarget::grind_5_0):GrindChangeTarget::grind_50_50);
        admission_.fresh=admission_.ready=admission_.geometry_known=true;
        admission_.generations={frame.owner_generation,frame.contact_generation,frame.action_generation};
    }
    template<class Arm> InputDisposition request(const InputRequest& request,std::uint64_t now,Arm&& arm) noexcept {
        const bool returning=source_.state==403 && source_.identity==24 &&
            request.grind_target()==GrindChangeTarget::grind_50_50;
        const bool lateral_tail=source_.state==403 && source_.identity==24 &&
            request.grind_target()==GrindChangeTarget::bluntslide;
        const bool lateral_nose=source_.state==403 && source_.identity==23 &&
            request.grind_target()==GrindChangeTarget::noseslide;
        const bool lateral=lateral_tail || lateral_nose;
        const bool sweep=source_.state==401 && source_.identity==20 &&
            (request.grind_target()==GrindChangeTarget::nosegrind || request.grind_target()==GrindChangeTarget::crooked ||
             request.grind_target()==GrindChangeTarget::grind_5_0);
        if(request.action!=InputAction::rail_change ||
            (!returning && !sweep && !lateral))
            return InputDisposition::unsupported;
        if(active_ || !admission_.ready || !fresh(now,frame_) || request.rail_generation!=generation_ ||
            request.generations!=admission_.generations || request.first_input_at>request.last_input_at ||
            request.last_input_at>request.recognized_at || request.recognized_at>now ||
            now-request.last_input_at>300 || request.last_input_at-request.first_input_at>300 ||
            serial_==UINT64_MAX || now>UINT64_MAX-1500) return InputDisposition::cancelled;
        const auto next=serial_+1;
        if(!arm(next,now+1500,lateral_nose?GrindAdmissionControl::lateral_nose:
            lateral_tail?GrindAdmissionControl::lateral_tail:GrindAdmissionControl::ordinary))
            return InputDisposition::unsupported;
        // The measured 5-0 recipe retains Down; the other two retain Up.
        // The native geometry classifier supplies the destination identity.
        const bool crooked=request.grind_target()==GrindChangeTarget::crooked;
        const bool tail=request.grind_target()==GrindChangeTarget::grind_5_0;
        // Neutral is an owned control sample, not a fabricated 50-50 result.
        // The original 403 admission and 406 initialization select the return.
        sweep_x_=lateral?32767:returning?0:crooked?24000:12000;
        held_y_=lateral_nose?8192:lateral_tail?-8192:returning?0:tail?-32767:32767;
        target_identity_=lateral_nose?21u:lateral_tail?31u:returning?20u:tail?24u:crooked?32u:23u;
        target_state_=lateral?402u:returning?401u:403u;
        hold_ms_=(returning || lateral)?1500u:1000u;lateral_=lateral;
        serial_=next;active_=true;stable_=false;
        evidence_={serial_,request.first_input_at,request.last_input_at,now};
        evidence_.source_animation_known=source_animation_known_;
        admission_={};return InputDisposition::started;
    }
    Pad sample(std::uint64_t now) const noexcept {
        if(!active_ || !fresh(now,frame_) || now<evidence_.started_at || now-evidence_.started_at>=hold_ms_)return {};
        // The measured lateral leg starts with neutral. Its lease remains
        // disabled until the actual native request enters the lateral band;
        // enabling it during neutral would execute the 50-50 return instead.
        if(lateral_)return now-evidence_.started_at<grind_lateral_neutral_ms?Pad{true,0,0}:Pad{true,sweep_x_,held_y_};
        // Native XInput Y is opposite to evdev ABS_RY used in the control trial.
        // Keep the vertical input held across the 200ms heading change; neutral between the two
        // samples loses the native captured-reference path.
        return {true,static_cast<std::int16_t>(now-evidence_.started_at<200?0:sweep_x_),held_y_};
    }
    void verify(std::uint64_t now,const NativeGrindCompletion& completion,
                const NativeGrindContact& contact,const NativeGrindSnapshot& animation) noexcept {
        if(!active_ || !fresh(now,frame_))return;
        evidence_.completed=evidence_.completed || (completion.valid && completion.source==source_.state &&
            completion.target==target_state_ && completion.phase==1.f);
        const bool changed=evidence_.completed && frame_.state==target_state_ && contact.valid && contact.state==target_state_ &&
            same_segment(contact) && contact.identity==target_identity_;
        evidence_.physics_changed=evidence_.physics_changed || changed;
        // This snapshot is supplied by the owner-bracketed native reader. A
        // lingering animation label alone cannot complete this request: the
        // matching source animation must also have been read before arming.
        if(changed && evidence_.source_animation_known && animation.valid && animation.physics_state==target_state_ &&
            animation.sampled_at>=evidence_.started_at && animation.sampled_at<=now &&
            now-animation.sampled_at<=150 && animation.physics_grind_type_valid && animation.physics_grind_type==target_identity_ &&
            animation.grind_type_valid && animation.grind_type==target_identity_) evidence_.animation_changed=true;
        // Preserve the measured held input through its bounded duration even
        // after identity publication; do not cut the control trajectory short.
        if(evidence_.animation_changed && now-evidence_.started_at>=1000)cancel();
    }
private:
    static bool fresh(std::uint64_t now,const NativeInputFrame& f) noexcept {
        return f.valid && f.owner.valid() && f.owner_generation && f.physics_step &&
            now>=f.sampled_at && now-f.sampled_at<=150;
    }
    static bool qualified(const NativeGrindContact& c) noexcept {
        // Qualification scope of the native control receipt, not a promise
        // that enough forward travel remains. Native geometry still decides.
        const bool sweep=c.state==401 && c.identity==20;
        const bool from_single_truck=c.state==403 && (c.identity==24 || c.identity==23);
        if(!c.valid || (!sweep && !from_single_truck) || c.geometry!=1 || c.begin[1]!=c.end[1])return false;
        double length{};
        for(unsigned i=0;i<3;++i) {
            if(!std::isfinite(c.begin[i]) || !std::isfinite(c.end[i]))return false;
            const auto d=static_cast<double>(c.end[i])-c.begin[i];length+=d*d;
        }
        // The return completed with contact on a measured twelve-metre rail;
        // its six-metre negative control ran out of geometry before completion.
        return length>=(from_single_truck?144.:36.);
    }
    bool same_segment(const NativeGrindContact& c) const noexcept {
        return c.output==source_.output && c.segment_identity==source_.segment_identity &&
            c.begin==source_.begin && c.end==source_.end && c.geometry==source_.geometry;
    }
    NativeInputFrame frame_{};
    NativeGrindContact source_{};
    InputRailState admission_{};
    Evidence evidence_{};
    std::int16_t sweep_x_{},held_y_{};
    std::uint32_t target_identity_{},target_state_{},hold_ms_{};
    std::uint64_t serial_{},generation_{},stable_at_{};
    bool active_{},stable_{},source_animation_known_{},lateral_{};
};
}
