#pragma once
#include "arcade_trigger_input.h"
#include "Engine/Game/Build/20260929/launches.h"

namespace dingosdk::arcade_mode {
// Ownership and read-only completion proof for a native stance request. This
// does not select a controller gesture: the exact native processor must supply
// that contract before a runtime adapter can issue any button output.
class StanceInputLease {
public:
    // Observation ceiling only: expiring this lease never stops or rewrites a
    // native actor that has already received its input.
    static constexpr std::uint64_t completion_limit_ms=1000;
    bool start(const TriggerPress& trigger,const InputNativeContext& native,std::uint64_t now) noexcept {
        if(active_ || !trigger || trigger.press.sequence<=retired_sequence_)return false;
        retired_sequence_=trigger.press.sequence;
        if(trigger.owner!=TriggerOwner::switch_stance || trigger.context!=InputContext::ground ||
           trigger.press.token!=InputToken{InputSource::triggers,1,InputEdge::press} ||
           now<trigger.press.at || now>trigger.deadline || trigger.deadline<trigger.press.at ||
           trigger.deadline-trigger.press.at>TriggerJournal::context_window_ms ||
           !ground(trigger.native,trigger.press.at) || !ground(native,now) ||
           trigger.native.generations!=native.generations ||
           trigger.native.flip.object!=native.flip.object || trigger.native.flip.stance!=native.flip.stance ||
           native.physics_step<trigger.native.physics_step)return false;
        if(trigger.released && (trigger.release.token!=InputToken{InputSource::triggers,1,InputEdge::release} ||
           trigger.release.sequence<=trigger.press.sequence || trigger.release.at<trigger.press.at ||
           trigger.release.at>now))return false;
        request_=trigger;before_=native;last_=native;last_poll_=now;
        active_=true;issued_=changed_=false;return true;
    }
    // The adapter calls this only after its exact native gesture was supplied.
    // A queued request or a pre-existing stance observation is not delivery.
    bool issued(const InputNativeContext& native,std::uint64_t now) noexcept {
        if(!active_ || issued_ || !observe(native,now,true) || now>request_.deadline ||
           native.flip.stance!=before_.flip.stance)return false;
        issued_=true;issued_at_=now;issued_step_=native.physics_step;return true;
    }
    bool observe(const InputNativeContext& native,std::uint64_t now,bool allowed) noexcept {
        if(!active_)return false;
        if(!allowed || !ground(native,now) || now<last_poll_ ||
           now-last_poll_>InputTiming::native_sample_lease_ms ||
           native.physics_step<last_.physics_step || native.sampled_at<last_.sampled_at ||
           native.generations.owner!=before_.generations.owner ||
           native.generations.contact!=before_.generations.contact ||
           native.flip.object!=before_.flip.object ||
           (issued_ && (now<issued_at_ || now-issued_at_>completion_limit_ms)) ||
           (!issued_ && (now>request_.deadline || native.generations.action!=before_.generations.action)) ||
           (native.flip.stance&~stance_mask)!=(before_.flip.stance&~stance_mask)) {
            cancel();return false;
        }
        const bool different=(native.flip.stance&stance_mask)!=(before_.flip.stance&stance_mask);
        if(different && (!issued_ || native.physics_step<=issued_step_ || native.sampled_at<=issued_at_)) {
            cancel();return false;
        }
        last_=native;last_poll_=now;
        if(different){changed_=true;active_=false;}
        return active_;
    }
    // Cancellation retires the edge even if the adapter was never ready. A
    // held RT cannot acquire another lease after a source/owner interruption.
    void cancel() noexcept {active_=issued_=changed_=false;request_={};before_=last_={};}
    bool active() const noexcept {return active_;}
    bool was_issued() const noexcept {return issued_;}
    bool observed_change() const noexcept {return changed_;}
    std::uint64_t sequence() const noexcept {return request_.press.sequence;}
private:
    static constexpr auto stance_mask=game::build::v20260929::launches::stance_switch_mask;
    static bool ground(const InputNativeContext& native,std::uint64_t now) noexcept {
        return native.required && native.fresh(now) && native.flip.valid && native.flip.object &&
            !native.airborne && !native.rail_family && !(native.flip.stance&0x40u) &&
            native.flip.state==100 && native.flip.mode<=1;
    }
    TriggerPress request_{};
    InputNativeContext before_{},last_{};
    std::uint64_t retired_sequence_{},last_poll_{},issued_at_{},issued_step_{};
    bool active_{},issued_{},changed_{};
};
}
