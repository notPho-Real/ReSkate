#pragma once
#include "arcade_trigger_input.h"
#include "Engine/Game/Build/20260929/launches.h"
#include "Engine/Game/Build/20260929/arcade_stance.h"

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
struct StanceControl {
    bool owned{};
    std::uint16_t buttons{};
    std::uint64_t serial{};
    std::uint8_t phase{};
    bool operator==(const StanceControl&) const = default;
};
// One original RT owns one finite native B double tap. A phase advances only
// after its exact final packet was published and a newer native step arrived.
// Neither input polls nor callback acceptance count as native stance change.
class StanceInput {
public:
    bool start(const TriggerPress& trigger,const InputNativeContext& native,std::uint64_t now) noexcept {
        if(active_ || !lease_.start(trigger,native,now))return false;
        ++serial_;sequence_=trigger.press.sequence;phase_=0;published_=false;active_=true;started_=now;
        baseline_=native;
        first_down_=last_tick_=published_at_=published_step_=published_sample_=0;
        changed_=second_down_=false;return true;
    }
    StanceControl controls(const InputNativeContext& native,std::uint64_t now,std::uint64_t tick) noexcept {
        if(!active_)return {};
        if(!valid_frame(native,now) || tick<last_tick_ || now<started_ ||
           (first_down_ && now-first_down_>StanceInputLease::completion_limit_ms)) {cancel();return {};}
        if(!lease_.observe(native,now,true)) {
            if(lease_.observed_change() && second_down_) {
                // Even a very fast native acknowledgment must publish B up.
                if(phase_==5 && changed_now(native)){changed_=true;active_=false;return {};}
            } else {cancel();return {};}
        }
        if(!second_down_ && first_down_ && (now<first_down_ || now-first_down_>=contract::double_press_ms)) {
            cancel();return {};
        }
        if(published_ && tick>last_tick_ && now>published_at_ &&
           native.physics_step>published_step_ && native.sampled_at>published_sample_) {
            ++phase_;published_=false;
        }
        if(phase_>=5) {
            if(lease_.observed_change() && changed_now(native)){changed_=true;active_=false;}
            return {};
        }
        return {true,static_cast<std::uint16_t>((phase_==1 || phase_==3)?contract::button:0),serial_,phase_};
    }
    bool delivered(const InputNativeContext& native,StanceControl control,std::uint16_t final_buttons,
                   std::uint64_t now,std::uint64_t tick) noexcept {
        if(!active_ || !control.owned || control.serial!=serial_ || control.phase!=phase_ || phase_>=5 ||
           control.buttons!=((phase_==1 || phase_==3)?contract::button:0) ||
           (final_buttons&contract::button)!=control.buttons || tick<last_tick_ ||
           !valid_frame(native,now)) {cancel();return false;}
        if(published_)return true; // Same delivered phase never renews its clocks.
        if(phase_==1) {
            if(!lease_.issued(native,now)){cancel();return false;}
            first_down_=now;
        }
        if(phase_==3) {
            if(now<first_down_ || now-first_down_>=contract::double_press_ms){cancel();return false;}
            second_down_=true;
        }
        published_=true;last_tick_=tick;published_at_=now;
        published_step_=native.physics_step;published_sample_=native.sampled_at;
        return true;
    }
    void cancel() noexcept {lease_.cancel();active_=published_=changed_=false;}
    bool active() const noexcept {return active_;}
    bool same_owner(const InputNativeContext& native) const noexcept {
        return native.generations.owner==baseline_.generations.owner &&
            native.generations.contact==baseline_.generations.contact && native.flip.object==baseline_.flip.object;
    }
    bool observed_change() const noexcept {return changed_;}
    bool double_tap_delivered() const noexcept {return second_down_;}
    std::uint64_t sequence() const noexcept {return sequence_;}
private:
    struct contract {
        static constexpr auto button=game::build::v20260929::arcade_stance::button;
        static constexpr auto double_press_ms=game::build::v20260929::arcade_stance::double_press_ms;
    };
    bool changed_now(const InputNativeContext& native) const noexcept {
        // An early acknowledgment is only provisional until final B release.
        // Do not turn a transient foot change into current completion proof.
        return ((native.flip.stance^baseline_.flip.stance)&
            game::build::v20260929::launches::stance_switch_mask)!=0;
    }
    bool valid_frame(const InputNativeContext& native,std::uint64_t now) const noexcept {
        return native.required && native.fresh(now) && native.flip.valid && !native.airborne && !native.rail_family &&
            !(native.flip.stance&0x40u) && native.flip.state==100 && native.flip.mode<=1 &&
            same_owner(native);
    }
    StanceInputLease lease_{};
    InputNativeContext baseline_{};
    std::uint64_t serial_{},sequence_{},started_{},first_down_{},last_tick_{},published_at_{},published_step_{},published_sample_{};
    std::uint8_t phase_{};
    bool active_{},published_{},changed_{},second_down_{};
};
}
