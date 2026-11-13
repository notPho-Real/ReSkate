#pragma once
#include "arcade_wallplant.h"

namespace dingosdk::arcade_mode {
// A release from the observed wall-contact state may be followed by the
// game's own pop query. This proof covers that first transition, not another
// launch. Its lifetime is the existing observation lease, never renewed.
inline bool wallplant_release_departure_current(const WallplantCommand& command,
        const NativeInputFrame& released,const NativeInputFrame& live,
        std::uint64_t released_at,std::uint64_t now,
        std::uint32_t selected,std::uint32_t previous) noexcept {
    const auto& origin=command.native;
    if(!command || !command.input_sequence || !command.contact || !origin.valid || !origin.owner.valid() ||
       !origin.airborne || origin.state!=201 || !origin.owner_generation || !origin.airborne_episode ||
       origin.air_departure==UINT64_MAX || origin.airborne_episode==UINT64_MAX ||
       !released.valid || released.state!=100 || released.airborne || released.rail_family ||
       released.owner!=origin.owner || released.owner_generation!=origin.owner_generation ||
       released.airborne_episode!=origin.airborne_episode || released.air_departure!=origin.air_departure+1 ||
       origin.action_generation==UINT64_MAX || released.action_generation!=origin.action_generation+1 ||
       released.stance_and_push!=origin.stance_and_push || released.flip_known!=origin.flip_known ||
       released.flip_id!=origin.flip_id || released.selection_object!=origin.selection_object ||
       !released.physics_step || now<released_at || now-released_at>InputTiming::native_sample_lease_ms ||
       released_at<released.sampled_at || released_at-released.sampled_at>InputTiming::native_sample_lease_ms ||
       selected!=200 || previous!=100 || !live.valid || live.owner!=released.owner ||
       live.owner_generation!=released.owner_generation || live.rail_family ||
       now<live.sampled_at || now-live.sampled_at>InputTiming::native_sample_lease_ms ||
       live.physics_step<released.physics_step || live.sampled_at<released.sampled_at ||
       live.stance_and_push!=released.stance_and_push || live.flip_known!=released.flip_known ||
       live.flip_id!=released.flip_id || live.selection_object!=released.selection_object)return false;
    if(live.physics_step==released.physics_step &&
       (live.sampled_at!=released.sampled_at || live.state!=released.state))return false;
    // The original setter can run before the input observer publishes state200.
    // Accept the retained100 or that exact next episode, never a later air/landing.
    if(live.state==100)return !live.airborne && live.airborne_episode==released.airborne_episode &&
        live.air_departure==released.air_departure && live.action_generation==released.action_generation;
    return live.state==200 && live.airborne && released.action_generation!=UINT64_MAX &&
        live.action_generation==released.action_generation+1 &&
        live.airborne_episode==released.airborne_episode+1 && live.air_departure==released.air_departure;
}
inline bool wallplant_release_velocity_current(const WallplantCommand& command,
        const std::array<float,3>& velocity) noexcept {
    for(unsigned axis=0;axis<3;++axis)
        if(!std::isfinite(velocity[axis]) || !std::isfinite(command.after[axis]) ||
           !std::isfinite(command.normal[axis]))return false;
    const float outward=velocity[0]*command.normal[0]+velocity[2]*command.normal[2];
    return velocity[1]>0 && velocity[1]<=command.after[1] && outward>0 &&
        std::hypot(velocity[0],velocity[2])<=std::hypot(command.after[0],command.after[2]);
}
inline bool wallplant_hold_anchor_current(const WallplantCommand& command,
        const std::array<float,3>& root) noexcept {
    if(!command.anchor_verified)return false;
    for(unsigned axis=0;axis<3;++axis)
        if(!std::isfinite(root[axis]) || !std::isfinite(command.anchor[axis]))return false;
    return std::hypot(root[0]-command.anchor[0],root[1]-command.anchor[1],
        root[2]-command.anchor[2])<=.70104f;
}
// A consumed collision command owns one short physical-target phase. This
// receipt never admits an input or changes a native state/contact bit.
class WallplantHoldContinuity {
public:
    static constexpr std::uint64_t duration_ms=160;
    bool active() const noexcept { return static_cast<bool>(command_); }
    const WallplantCommand& command() const noexcept { return command_; }
    std::uint64_t started_at() const noexcept { return started_; }
    std::uint64_t deadline() const noexcept { return deadline_; }
    std::uint64_t target_expiry() const noexcept { return active() && !bridge_until_ ? deadline_+grace_ : 0; }
    const char* reason() const noexcept { return reason_; }
    // A reader may lose a race with the original physics publisher. This is
    // not permission to write: leave the newer hold untouched and defer.
    bool stale_query(const NativeInputFrame& frame,std::uint64_t now) const noexcept {
        return active() && now>=last_now_ && now<=deadline_+grace_ &&
            frame.valid && frame.owner==last_.owner && frame.owner_generation==last_.owner_generation &&
            !frame.rail_family && (frame.state==100 || frame.state==200 || frame.state==201) &&
            now>=frame.sampled_at && now-frame.sampled_at<=InputTiming::native_sample_lease_ms &&
            frame.physics_step<last_.physics_step && frame.sampled_at<=last_.sampled_at;
    }
    void cancel(const char* reason="cancel") noexcept {
        command_={};last_={};started_=deadline_=grace_=last_now_=bridge_until_=0;
        contact_=resumed_=false;reason_=reason;
    }
    bool begin(const WallplantCommand& command,std::uint64_t now) noexcept {
        cancel();
        const auto& origin=command.native;
        if(!command || !command.input_sequence || !command.contact || !origin.valid ||
           !origin.owner.valid() || !origin.owner_generation || !origin.airborne ||
           (origin.state!=200 && origin.state!=201) || !origin.airborne_episode ||
           !origin.physics_step || now<command.issued_at ||
           now-command.issued_at>InputTiming::native_sample_lease_ms ||
           now<origin.sampled_at || now-origin.sampled_at>InputTiming::native_sample_lease_ms ||
           !std::isfinite(origin.physics_seconds) || origin.physics_seconds<=0 ||
           origin.physics_seconds>.1f || now>UINT64_MAX-duration_ms-100)return false;
        command_=command;last_=origin;started_=last_now_=now;deadline_=now+duration_ms;
        grace_=static_cast<std::uint64_t>(std::ceil(origin.physics_seconds*1000.0));
        reason_="entered";return true;
    }
    bool observe(const NativeInputFrame& frame,std::uint64_t now) noexcept {
        if(!active())return false;
        if(now<last_now_ || now>(bridge_until_?bridge_until_:deadline_+grace_) || now<frame.sampled_at ||
           now-frame.sampled_at>InputTiming::native_sample_lease_ms) {
            cancel("clock_or_expiry");return false;
        }
        const auto& origin=command_.native;
        if(!frame.valid || !frame.owner.valid() || frame.owner!=origin.owner ||
           frame.owner_generation!=origin.owner_generation ||
           frame.rail_family ||
           frame.physics_step<last_.physics_step || frame.sampled_at<last_.sampled_at) {
            cancel("owner_or_observation");return false;
        }
        // NativeInputState increments action_generation on every state change,
        // even for the original wall-contact reconstruction. Preserve exactly
        // that increment; a new stance or selected trick is a different owner.
        const bool state_changed=frame.state!=last_.state;
        const bool forward_state=!state_changed || (last_.state==201 && frame.state==100 && !resumed_) ||
            (last_.state==100 && (frame.state==200 || frame.state==201)) ||
            (last_.state==200 && frame.state==201);
        if(!forward_state || frame.stance_and_push!=last_.stance_and_push || frame.flip_known!=last_.flip_known ||
           frame.flip_id!=last_.flip_id || frame.selection_object!=last_.selection_object ||
           (state_changed && last_.action_generation==UINT64_MAX) ||
           frame.action_generation!=last_.action_generation+static_cast<unsigned>(state_changed)) {
            cancel("action_changed");return false;
        }
        if(frame.physics_step==last_.physics_step &&
           (frame.sampled_at!=last_.sampled_at || frame.state!=last_.state ||
            frame.airborne!=last_.airborne || frame.airborne_episode!=last_.airborne_episode ||
            frame.air_departure!=last_.air_departure)) {
            cancel("same_step_changed");return false;
        }
        if(frame.state==100 && !frame.airborne && !resumed_) {
            if(origin.state!=201 || origin.air_departure==UINT64_MAX ||
               frame.airborne_episode!=origin.airborne_episode || frame.air_departure!=origin.air_departure+1) {
                cancel("contact_provenance");return false;
            }
            contact_=true;reason_="contact_hold";
        } else if(frame.airborne && (frame.state==200 || frame.state==201)) {
            if(contact_) {
                if(origin.airborne_episode==UINT64_MAX || origin.air_departure==UINT64_MAX ||
                   frame.airborne_episode!=origin.airborne_episode+1 || frame.air_departure!=origin.air_departure+1) {
                    cancel("resume_provenance");return false;
                }
                resumed_=true;reason_="resumed_hold";
            } else if(frame.airborne_episode!=origin.airborne_episode || frame.air_departure!=origin.air_departure) {
                cancel("flight_changed");return false;
            } else reason_="air_hold";
        } else {cancel("native_state");return false;}
        last_=frame;last_now_=now;return true;
    }
    bool release_due(std::uint64_t now) const noexcept {
        return active() && !bridge_until_ && now>=last_now_ && now>=deadline_ && now<=deadline_+grace_;
    }
    bool target_active(std::uint64_t now) const noexcept {
        // The release runs after the original motion callback. Keep its target
        // through that callback, including the existing one-step dispatch
        // grace, until the release consumes this receipt. Dropping it at the
        // deadline lets ground motion return before the body launch.
        return active() && !bridge_until_ && now>=last_now_ && now<=deadline_+grace_;
    }
    // Apply only to a copy after the actual release write. It grants one
    // original physics interval for an already observed wall-contact100 to
    // return to AIR, never a new ground episode, target write, or second kick.
    bool release_bridge(std::uint64_t now) noexcept {
        if(!release_due(now) || !contact_ || resumed_ || last_.state!=100 || last_.airborne ||
           now>UINT64_MAX-grace_) {cancel("release_bridge_unavailable");return false;}
        last_now_=now;bridge_until_=now+grace_;reason_="released_contact";return true;
    }
private:
    WallplantCommand command_{};
    NativeInputFrame last_{};
    std::uint64_t started_{},deadline_{},grace_{},last_now_{},bridge_until_{};
    bool contact_{},resumed_{};
    const char* reason_{"empty"};
};
}
