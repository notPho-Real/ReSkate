#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_native_input_state.h"
#include "arcade_grind_contact.h"
#include "arcade_grind_transition_lifetime.h"
#include "arcade_grind_observer.h"
#include "arcade_grind_admission.h"
#include "arcade_grind_control.h"

namespace dingosdk::arcade_mode {
// Bounded input executor for independently observed 401/20 ->406 ->403
// held sweeps ending in native23 (Nosegrind),32 (Crooked) or24 (5-0), and
// the measured neutral 403/24 ->406 ->401/20 return and lateral tail
// 403/24 ->406 ->402/31 Bluntslide and 403/23 ->406 ->402/21 Noseslide
// or opposite-heading 402/30 Nosebluntslide and 403/24 ->402/22 Tailslide.
// The Tailslide pulse must return to neutral before held-input drift.
// Smith uses original403 control motion on the same actor, without a406 lease.
// The measured slide/Smith routes may begin in401/20: independently verify
// original403 preparation before admitting the lateral lease or Smith controls.
// Other source contexts have no qualified control recipe yet.
// This owns only RS and the scoped native admission lease. Native completion,
// contact and animation are observations; none is synthesized by this actor.
class GrindChange {
public:
    struct Pad { bool owns{}; std::int16_t x{},y{}; };
    struct Evidence {
        std::uint64_t serial{},first_input_at{},last_input_at{},started_at{};
        bool completed{},physics_changed{},animation_changed{},source_animation_known{};
        bool control_only{};
        bool preparation_completed{},preparation_physics_changed{};
        std::uint64_t final_leg_serial{},final_leg_started_at{};
    };
    void cancel() noexcept { active_=false; admission_={}; source_={}; frame_={}; stable_=false; source_animation_known_=false; preparing_=false; preparation_ready_=false; }
    bool active() const noexcept { return active_; }
    bool requires_transition() const noexcept { return active_ && !smith_; }
    std::uint64_t serial() const noexcept { return active_?serial_:0; }
    const Evidence& evidence() const noexcept { return evidence_; }
    InputRailState admission() const noexcept { return admission_; }
    // Repeated game-facing polls may not advance the establishment clock.
    void observe(std::uint64_t now,const NativeInputFrame& frame,const NativeGrindContact& contact,
                 bool permitted,const NativeGrindSnapshot& animation={},NativeGrindControl controls={}) noexcept {
        admission_={};
        if(!permitted || !fresh(now,frame) || (frame_.valid &&
            (frame.owner!=frame_.owner || frame.owner_generation!=frame_.owner_generation ||
             frame.physics_step<frame_.physics_step || frame.sampled_at<frame_.sampled_at ||
             frame.sampled_at-frame_.sampled_at>150))) {
            cancel();return;
        }
        frame_=frame;controls_=controls;
        if(active_) {
            if(now<leg_started_at_ || now-leg_started_at_>=duration_ms_ ||
                (smith_ && (!controls.valid || frame.state!=403 || contact.actor!=source_.actor)) ||
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
        const bool smith=source_.state==403 && source_.identity==24 &&
            request.grind_target()==GrindChangeTarget::smith;
        const bool lateral_tail=source_.state==403 && source_.identity==24 &&
            request.grind_target()==GrindChangeTarget::bluntslide;
        const bool lateral_tail_reverse=source_.state==403 && source_.identity==24 &&
            request.grind_target()==GrindChangeTarget::tailslide;
        const bool lateral_nose=source_.state==403 && source_.identity==23 &&
            request.grind_target()==GrindChangeTarget::noseslide;
        const bool lateral_nose_reverse=source_.state==403 && source_.identity==23 &&
            request.grind_target()==GrindChangeTarget::nosebluntslide;
        const bool lateral=lateral_tail || lateral_tail_reverse || lateral_nose || lateral_nose_reverse;
        const bool sweep=source_.state==401 && source_.identity==20 &&
            (request.grind_target()==GrindChangeTarget::nosegrind || request.grind_target()==GrindChangeTarget::crooked ||
             request.grind_target()==GrindChangeTarget::grind_5_0);
        const bool prepare_tail=source_.state==401 && source_.identity==20 &&
            (request.grind_target()==GrindChangeTarget::bluntslide || request.grind_target()==GrindChangeTarget::tailslide ||
             request.grind_target()==GrindChangeTarget::smith);
        const bool prepare_nose=source_.state==401 && source_.identity==20 &&
            (request.grind_target()==GrindChangeTarget::noseslide || request.grind_target()==GrindChangeTarget::nosebluntslide);
        const bool preparing=prepare_tail || prepare_nose;
        if(request.action!=InputAction::rail_change ||
            (!returning && !sweep && !lateral && !smith && !preparing) ||
            (preparing && !geometry(source_,144.)))
            return InputDisposition::unsupported;
        const std::uint64_t duration=smith?2200u:1500u;
        if(active_ || !admission_.ready || !fresh(now,frame_) || request.rail_generation!=generation_ ||
            request.generations!=admission_.generations || request.first_input_at>request.last_input_at ||
            request.last_input_at>request.recognized_at || request.recognized_at>now ||
            now-request.last_input_at>300 || request.last_input_at-request.first_input_at>300 ||
            serial_>=UINT64_MAX-(preparing?1u:0u) ||
            now>UINT64_MAX-(preparing?(request.grind_target()==GrindChangeTarget::smith?3700u:3000u):duration))
            return InputDisposition::cancelled;
        // The native reference was cleared before the measured control-only
        // sequence. Require observed neutral pitch/roll and feature-off now;
        // do not reuse an active transition lease or a held prior control.
        if(smith && (!controls_.valid || !controls_.neutral))return InputDisposition::cancelled;
        const auto next=serial_+1;
        if(!smith && !arm(next,now+1500,lateral_tail_reverse?GrindAdmissionControl::lateral_tail_reverse:
            lateral_nose_reverse?GrindAdmissionControl::lateral_nose_reverse:
            lateral_nose?GrindAdmissionControl::lateral_nose:
            lateral_tail?GrindAdmissionControl::lateral_tail:GrindAdmissionControl::ordinary))
            return InputDisposition::unsupported;
        // The measured 5-0 recipe retains Down; the other two retain Up.
        // The native geometry classifier supplies the destination identity.
        const bool crooked=request.grind_target()==GrindChangeTarget::crooked;
        const bool tail=request.grind_target()==GrindChangeTarget::grind_5_0 || prepare_tail;
        // Neutral is an owned control sample, not a fabricated 50-50 result.
        // The original 403 admission and 406 initialization select the return.
        sweep_x_=(lateral_nose_reverse || lateral_tail_reverse)?-32767:lateral?32767:returning?0:crooked?24000:12000;
        held_y_=(lateral_nose || lateral_nose_reverse)?8192:(lateral_tail || lateral_tail_reverse)?-8192:returning?0:tail?-32767:32767;
        target_identity_=smith?26u:lateral_tail_reverse?22u:lateral_nose_reverse?30u:lateral_nose?21u:lateral_tail?31u:returning?20u:tail?24u:crooked?32u:23u;
        target_state_=lateral?402u:returning?401u:403u;
        hold_ms_=smith?2200u:(returning || lateral)?1500u:1000u;
        duration_ms_=duration;smith_=smith;lateral_=lateral;tail_release_=lateral_tail_reverse;
        serial_=next;active_=true;stable_=false;
        leg_started_at_=now;preparing_=preparing;preparation_ready_=false;
        final_target_=request.grind_target();
        evidence_={serial_,request.first_input_at,request.last_input_at,now};
        evidence_.source_animation_known=source_animation_known_;
        evidence_.control_only=smith;
        admission_={};return InputDisposition::started;
    }
    // Continue an already admitted physical request; never re-date or submit a
    // synthetic input event to evade the recognizer's300ms window. The first
    // native completion and independently measured intermediate name/segment
    // are prerequisites. Arm revalidates the current native owner/contact.
    template<class Arm> void advance(std::uint64_t now,const NativeGrindContact& contact,
                                    bool native_idle,Arm&& arm) noexcept {
        if(!active_ || !preparing_ || !preparation_ready_)return;
        if(!fresh(now,frame_) || now<preparation_ready_at_ || now-preparation_ready_at_>150 ||
            now<leg_started_at_ || now-leg_started_at_>=1500 ||
            frame_.state!=403 || !contact.valid || contact!=preparation_contact_ || !same_segment(contact)) {
            cancel();return;
        }
        // A still-live original406 lease cannot be replaced by a new lease.
        // During retirement the executor owns neutral, as in the native trial.
        if(!native_idle)return;
        if(final_target_==GrindChangeTarget::smith) {
            // The measured Smith leg starts only after the original lease is
            // gone and native controls have observed neutral. No second406 is
            // requested or claimed for this control-only trajectory.
            if(!controls_.valid || !controls_.neutral)return;
            source_=contact;++serial_;leg_started_at_=now;
            preparing_=preparation_ready_=false;smith_=true;lateral_=tail_release_=false;
            target_identity_=26;target_state_=403;hold_ms_=2200;duration_ms_=2200;
            evidence_.control_only=true;evidence_.final_leg_serial=serial_;evidence_.final_leg_started_at=now;
            return;
        }
        const bool nose=final_target_==GrindChangeTarget::noseslide || final_target_==GrindChangeTarget::nosebluntslide;
        const bool reverse=final_target_==GrindChangeTarget::nosebluntslide || final_target_==GrindChangeTarget::tailslide;
        const auto control=nose?(reverse?GrindAdmissionControl::lateral_nose_reverse:GrindAdmissionControl::lateral_nose):
            (reverse?GrindAdmissionControl::lateral_tail_reverse:GrindAdmissionControl::lateral_tail);
        const auto next=serial_+1;
        if(!arm(next,now+1500,control)) { cancel();return; }
        source_=contact;serial_=next;leg_started_at_=now;
        preparing_=preparation_ready_=false;lateral_=true;tail_release_=!nose && reverse;
        sweep_x_=reverse?-32767:32767;held_y_=nose?8192:-8192;
        target_identity_=nose?(reverse?30u:21u):(reverse?22u:31u);target_state_=402;
        hold_ms_=1500;duration_ms_=1500;
        evidence_.final_leg_serial=serial_;evidence_.final_leg_started_at=now;
    }
    Pad sample(std::uint64_t now) const noexcept {
        if(!active_ || !fresh(now,frame_) || now<leg_started_at_ || now-leg_started_at_>=duration_ms_)return {};
        if(preparation_ready_)return {true,0,0};
        if(now-leg_started_at_>=hold_ms_)return {};
        if(smith_) {
            const auto elapsed=now-leg_started_at_;
            // Native Down captures the reference before the heading sweep.
            // Starting directly with diagonal input recaptures it as zero.
            if(elapsed<200)return {true,0,-32767};
            if(elapsed<500)return {true,24000,-32767};
            if(elapsed<800 || elapsed>=2000)return {true,0,0};
            constexpr std::array<std::int16_t,5> ramp{6553,13106,19660,26213,32767};
            const auto step=(elapsed-800)/100;
            return {true,0,ramp[step<4?step:4]};
        }
        // The measured lateral leg starts with neutral. Its lease remains
        // disabled until the actual native request enters the lateral band;
        // enabling it during neutral would execute the 50-50 return instead.
        if(lateral_) {
            const auto elapsed=now-leg_started_at_;
            // Actual native22 remained in contact after a1000ms lateral pulse;
            // keeping that pulse held instead drifted into Suski71. Own the
            // neutral tail even when animation evidence has already arrived.
            if(elapsed<grind_lateral_neutral_ms ||
                (tail_release_ && elapsed>=grind_lateral_neutral_ms+1000))return {true,0,0};
            return {true,sweep_x_,held_y_};
        }
        // Native XInput Y is opposite to evdev ABS_RY used in the control trial.
        // Keep the vertical input held across the 200ms heading change; neutral between the two
        // samples loses the native captured-reference path.
        return {true,static_cast<std::int16_t>(now-leg_started_at_<200?0:sweep_x_),held_y_};
    }
    void verify(std::uint64_t now,const NativeGrindCompletion& completion,
                const NativeGrindContact& contact,const NativeGrindSnapshot& animation) noexcept {
        if(!active_ || !fresh(now,frame_))return;
        if(preparing_) {
            evidence_.preparation_completed=evidence_.preparation_completed ||
                (completion.valid && completion.source==401 && completion.target==403 && completion.phase==1.f);
            if(evidence_.preparation_completed && frame_.state==403 && contact.valid && contact.state==403 &&
                same_segment(contact) && contact.identity==target_identity_) {
                evidence_.preparation_physics_changed=true;
                if(!preparation_ready_) { preparation_contact_=contact;preparation_ready_at_=now;preparation_ready_=true; }
            }
            // An observed Nosegrind/5-0 preparation is not the requested name.
            return;
        }
        evidence_.completed=evidence_.completed || (!smith_ && completion.valid && completion.source==source_.state &&
            completion.target==target_state_ && completion.phase==1.f);
        const bool executed=smith_?(controls_.valid && now>=leg_started_at_ && now-leg_started_at_>=1200):evidence_.completed;
        const bool changed=executed && frame_.state==target_state_ && contact.valid && contact.state==target_state_ &&
            (!smith_ || contact.actor==source_.actor) &&
            same_segment(contact) && contact.identity==target_identity_;
        evidence_.physics_changed=evidence_.physics_changed || changed;
        // This snapshot is supplied by the owner-bracketed native reader. A
        // lingering animation label alone cannot complete this request: the
        // matching source animation must also have been read before arming.
        if(changed && evidence_.source_animation_known && animation.valid && animation.physics_state==target_state_ &&
            animation.sampled_at>=leg_started_at_ && animation.sampled_at<=now &&
            now-animation.sampled_at<=150 && animation.physics_grind_type_valid && animation.physics_grind_type==target_identity_ &&
            animation.grind_type_valid && animation.grind_type==target_identity_) evidence_.animation_changed=true;
        // Preserve the measured held input through its bounded duration even
        // after identity publication; do not cut the control trajectory short.
        if(evidence_.animation_changed && now-leg_started_at_>=(smith_?2200u:tail_release_?1500u:1000u))cancel();
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
        return (sweep || from_single_truck) && geometry(c,from_single_truck?144.:36.);
    }
    static bool geometry(const NativeGrindContact& c,double minimum) noexcept {
        if(!c.valid || c.geometry!=1 || c.begin[1]!=c.end[1])return false;
        double length{};
        for(unsigned i=0;i<3;++i) {
            if(!std::isfinite(c.begin[i]) || !std::isfinite(c.end[i]))return false;
            const auto d=static_cast<double>(c.end[i])-c.begin[i];length+=d*d;
        }
        // The return completed with contact on a measured twelve-metre rail;
        // its six-metre negative control ran out of geometry before completion.
        return length>=minimum;
    }
    bool same_segment(const NativeGrindContact& c) const noexcept {
        return c.output==source_.output && c.segment_identity==source_.segment_identity &&
            c.begin==source_.begin && c.end==source_.end && c.geometry==source_.geometry;
    }
    NativeInputFrame frame_{};
    NativeGrindControl controls_{};
    NativeGrindContact source_{};
    NativeGrindContact preparation_contact_{};
    InputRailState admission_{};
    Evidence evidence_{};
    std::int16_t sweep_x_{},held_y_{};
    std::uint32_t target_identity_{},target_state_{},hold_ms_{};
    std::uint64_t serial_{},generation_{},stable_at_{},duration_ms_{1500},leg_started_at_{},preparation_ready_at_{};
    GrindChangeTarget final_target_{};
    bool active_{},stable_{},source_animation_known_{},lateral_{},tail_release_{},smith_{},preparing_{},preparation_ready_{};
};
}
