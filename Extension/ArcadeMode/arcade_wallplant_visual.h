#pragma once
#include "arcade_wallplant.h"

namespace dingosdk::arcade_mode {
// A completed body's visual receipt grants no input, motion or scoring rights.
// Native100 may reconstruct the body before the observed rebound returns to AIR.
// Preserve only this receipt, within its original lease, across that one handoff.
class WallplantVisual {
public:
    bool pending() const noexcept { return static_cast<bool>(command_); }
    const WallplantCommand& command() const noexcept { return command_; }
    const char* reason() const noexcept { return reason_; }
    void cancel(const char* reason="cancel") noexcept {
        command_={};last_={};contact_=resumed_=ready_=false;reason_=reason;
    }
    void completed(const WallplantCommand& command,bool applied) noexcept {
        cancel();
        if(!applied || !command || !command.input_sequence || !command.native.valid ||
           !command.native.airborne || (command.native.state!=200 && command.native.state!=201) ||
           !command.native.owner.valid() || !command.native.owner_generation ||
           !command.native.airborne_episode || !command.native.physics_step ||
           command.issued_at<command.native.sampled_at ||
           command.issued_at-command.native.sampled_at>InputTiming::native_sample_lease_ms)return;
        command_=command;last_=command.native;reason_="queued";
    }
    void observe(const NativeInputFrame& frame,std::uint64_t now) noexcept {
        if(!pending() || !current(frame,now))return;
        if(frame.physics_step<=last_.physics_step || frame.sampled_at<last_.sampled_at) {
            cancel("observation_order");return;
        }
        const auto& origin=command_.native;
        if(frame.state==100 && !frame.airborne && !resumed_) {
            if(origin.state!=201 || origin.air_departure==UINT64_MAX ||
               frame.airborne_episode!=origin.airborne_episode || frame.air_departure!=origin.air_departure+1) {
                cancel("contact_provenance");return;
            }
            contact_=true;ready_=false;reason_="contact_wait";
        } else if(frame.airborne && (frame.state==200 || frame.state==201)) {
            if(contact_) {
                if(origin.airborne_episode==UINT64_MAX || origin.air_departure==UINT64_MAX ||
                   frame.airborne_episode!=origin.airborne_episode+1 || frame.air_departure!=origin.air_departure+1) {
                    cancel("resume_provenance");return;
                }
                resumed_=true;reason_="resumed_air";
            } else if(frame.airborne_episode!=origin.airborne_episode || frame.air_departure!=origin.air_departure) {
                cancel("flight_changed");return;
            } else reason_="same_air";
            ready_=true;
        } else {cancel("native_state");return;}
        last_=frame;
    }
    WallplantCommand take(const NativeInputFrame& frame,std::uint64_t now) noexcept {
        if(!pending() || !current(frame,now) || !ready_)return {};
        if(!wallplant_same_observation(last_,frame)) {cancel("observation_changed");return {};}
        auto result=command_;cancel("requested");return result;
    }
private:
    bool current(const NativeInputFrame& frame,std::uint64_t now) noexcept {
        if(now<command_.issued_at || now-command_.issued_at>InputTiming::native_sample_lease_ms ||
           now<frame.sampled_at || now-frame.sampled_at>InputTiming::native_sample_lease_ms) {
            cancel("expired");return false;
        }
        if(!frame.valid || !frame.owner.valid() || frame.owner!=command_.native.owner ||
           frame.owner_generation!=command_.native.owner_generation || frame.rail_family) {
            cancel("owner_or_rail");return false;
        }
        return true;
    }
    WallplantCommand command_{};
    NativeInputFrame last_{};
    bool contact_{},resumed_{},ready_{};
    const char* reason_{"empty"};
};
}
