#pragma once
#include "arcade_trick_table.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode::native_grab {
// These are Enum.DetailedGrabType values emitted by the original loop actors,
// not Enum.GrabType values and not requested animation IDs.
struct Control {
    unsigned hands{};
    std::int16_t x{}, y{};
    std::uint16_t feet{}; // Original face-button foot input, only after native grab entry.
    bool selector() const noexcept {return x!=0 || y!=0;}
};
struct Pilot {
    ArcadeTrick trick; std::int32_t detail; float x, y; std::int32_t family{2}; unsigned hands{2};
    std::int16_t selector_x{}, selector_y{};
    std::int32_t one_footer{};
    unsigned foot{}; // 1 left, 2 right, 3 both; not an animation identity.
    constexpr Control control() const noexcept {return {hands,selector_x,selector_y};}
};
// Selector RS coordinates preserve the original qualified-stance producer.
// They select the family before the original blendspace applies tweak X/Y;
// no hand, family enum, animation ID or score is written by this control map.
inline constexpr Pilot pilots[]{{ArcadeTrick::basic_bs_grab,1,0,0},
    {ArcadeTrick::melon,4,0,1},{ArcadeTrick::method,2,1,0},
    {ArcadeTrick::japan,3,0,-1},{ArcadeTrick::crossbone,5,-1,0},
    {ArcadeTrick::indy,6,0,0,1,1},{ArcadeTrick::fs_nosebone,7,0,1,1,1},
    {ArcadeTrick::fs_tailbone,8,0,-1,1,1},{ArcadeTrick::tuck_knee,9,1,0,1,1},
    {ArcadeTrick::stiffy,10,-1,0,1,1},
    {ArcadeTrick::double_grab,33,0,0,9,3},{ArcadeTrick::double_grab_front,36,0,1,9,3},
    {ArcadeTrick::double_grab_back,37,0,-1,9,3},{ArcadeTrick::double_grab_left,34,-1,0,9,3},
    {ArcadeTrick::double_grab_right,35,1,0,9,3},
    {ArcadeTrick::tail_bs_shifty,14,-1,-0.01f,3,1,0,28000},
    {ArcadeTrick::tail_fs_shifty,13,1,0,3,1,0,28000},
    {ArcadeTrick::tailgrab,11,0,0,3,1,0,28000},
    {ArcadeTrick::tail_nosebone,12,0,1,3,1,0,28000},
    {ArcadeTrick::nose_bs_shifty,24,-1,0,6,2,0,-28000},
    {ArcadeTrick::nose_fs_shifty,23,1,0,6,2,0,-28000},
    {ArcadeTrick::nosegrab,21,0,0,6,2,0,-28000},
    {ArcadeTrick::nose_tailbone,22,0,-1,6,2,0,-28000},
    {ArcadeTrick::nosegrab_alternate,21,0,-0.49f,6,2,0,-28000},
    {ArcadeTrick::nose_crail_fs_shifty,19,0.5f,0,6,2,0,-28000},
    {ArcadeTrick::seatbelt_fs_shifty,16,-1,0,4,2,0,28000},
    {ArcadeTrick::seatbelt,15,0,0,4,2,0,28000},
    {ArcadeTrick::crail,18,0,0,5,1,0,-28000},
    {ArcadeTrick::crail_fs_shifty,19,1,0,5,1,0,-28000},
    {ArcadeTrick::crail_bs_shifty,20,-1,0,5,1,0,-28000},
    {ArcadeTrick::crail_bs_shifty_alternate,20,-0.5f,0,5,1,0,-28000},
    {ArcadeTrick::stale_nosebone,27,0.01f,1,7,1,28000,0},
    {ArcadeTrick::stale_tailbone,26,0,-1,7,1,28000,0},
    {ArcadeTrick::stale_tuck_knee_right,28,1,0,7,1,28000,0},
    {ArcadeTrick::stale_tuck_knee,28,-1,0,7,1,28000,0},
    {ArcadeTrick::stale_tuck_knee_middle,28,-0.51f,0,7,1,28000,0},
    {ArcadeTrick::stale,25,0,0,7,1,28000,0},
    {ArcadeTrick::weddle_japan,32,-1,0,8,2,28000,0},
    {ArcadeTrick::weddle,29,0,0,8,2,28000,0},
    {ArcadeTrick::weddle_alternate,29,1,0,8,2,28000,0},
    {ArcadeTrick::weddle_nose,31,0,1,8,2,28000,0},
    {ArcadeTrick::weddle_tail,30,0,-1,8,2,28000,0},
    {ArcadeTrick::rocket_air,38,0,0,10,3,0,-28000},
    {ArcadeTrick::fs_one_foot,0,0,0,1,1,0,0,1,1},
    {ArcadeTrick::frigid_air,0,0,0,1,1,0,0,2,2},
    {ArcadeTrick::no_foot,0,0,0,1,1,0,0,3,3},
    {ArcadeTrick::judo,0,0,0,2,2,0,0,4,1},
    {ArcadeTrick::bs_one_foot,0,0,0,2,2,0,0,5,2},
    {ArcadeTrick::christ_air,0,0,0,2,2,0,0,6,3},
    {ArcadeTrick::benihana,0,0,0,3,1,0,28000,7,1},
    {ArcadeTrick::one_foot_tailgrab,0,0,0,3,1,0,28000,8,2},
    {ArcadeTrick::tailwalk,0,0,0,3,1,0,28000,9,3},
    {ArcadeTrick::dog_piss,0,0,0,6,2,0,-28000,10,1},
    {ArcadeTrick::nose_judo,0,0,0,6,2,0,-28000,11,2},
    {ArcadeTrick::airwalk,0,0,0,6,2,0,-28000,12,3},
    {ArcadeTrick::superdude,0,0,0,9,3,0,0,13,3}};
// R3 held at the physical B press selects this layer. No existing row moves.
constexpr ArcadeTrick extended_grab(std::int16_t direction,ArcadeTrick selected=ArcadeTrick::double_grab) noexcept {
    switch(direction) {
    case 7:return ArcadeTrick::crossbone; case 9:return ArcadeTrick::fs_nosebone;
    case 1:return ArcadeTrick::fs_tailbone; case 3:return ArcadeTrick::tuck_knee;
    case 8:return ArcadeTrick::double_grab_front; case 2:return ArcadeTrick::double_grab_back;
    case 4:return ArcadeTrick::double_grab_left; case 6:return ArcadeTrick::double_grab_right;
    default:return selected;
    }
}
constexpr const Pilot* pilot(ArcadeTrick trick) noexcept {
    for (const auto& p:pilots) if (p.trick==trick) return &p;
    return nullptr;
}

// Original native grab signals, read together from the qualified local state
// block. BS here is EnumGS.GrabType=2, not DetailedGrabType=2 (Method).
struct RetargetObservation {
    bool valid{},grabbing{},can_grab{},left{},right{};
    std::int32_t family{};
    bool eligible(const Pilot* selected=nullptr) const noexcept {
        const auto hands=selected?selected->hands:2u;
        return valid && grabbing && can_grab && left==((hands&1u)!=0) &&
            right==((hands&2u)!=0) && family==(selected?selected->family:2);
    }
};

// The gamepad producer applies two user preferences to X/A. A foot lease
// temporarily requests their unambiguous physical-foot configuration, then
// waits until the existing settings getter has actually returned it. Both-feet
// uses the native slide/B input, never simultaneous X+A (the hippy-jump input).
constexpr std::uint16_t foot_button(unsigned foot) noexcept {
    return foot==1?0x4000:foot==2?0x1000:foot==3?0x2000:0;
}

// A physical request remains pending through a brief release until independent
// native actor advancement AND the native dominant-pose tag agree. The deadline
// is a failure bound, never a required input pulse or animation setup duration.
class Lease {
public:
    static constexpr std::uint64_t initialization_limit_ms=1500, heartbeat_ms=150;
    bool request(ArcadeTrick trick,std::uint64_t now,bool held) noexcept {
        const auto* p=pilot(trick);
        if (!p) return false;
        selected_=p; requested_=refreshed_=now; released_=!held; acknowledged_=false; feet_started_=false;
        return true;
    }
    void refresh(std::uint64_t now,bool held,bool same_air_owner) noexcept {
        if (!selected_) return;
        if (!same_air_owner || now<refreshed_ || now-refreshed_>heartbeat_ms) { cancel();return; }
        refreshed_=now;
        // A subsequent physical hold cannot resurrect this physical occurrence.
        released_=released_ || !held;
        if (!acknowledged_ && now-requested_>initialization_limit_ms) cancel();
    }
    bool acknowledge(std::int32_t detail,float delta,float before,float after,
                     bool initialized,bool exact_actor,bool native_grabbing,std::int32_t one_footer=0) noexcept {
        if (!selected_ || (selected_->one_footer ? !feet_started_ || one_footer!=selected_->one_footer : detail!=selected_->detail) ||
            !initialized || !exact_actor || !native_grabbing ||
            !std::isfinite(delta) || delta<=0 || !std::isfinite(before) || !std::isfinite(after) || after<=before) return false;
        acknowledged_=true;return true;
    }
    bool intent(std::uint64_t now) const noexcept {
        return selected_ && now>=refreshed_ && now-refreshed_<=heartbeat_ms &&
            (acknowledged_ || (now>=requested_ && now-requested_<=initialization_limit_ms)) &&
            !(released_ && acknowledged_);
    }
    bool acknowledged() const noexcept {return acknowledged_;}
    Control controls(const RetargetObservation& native,bool settings_applied) noexcept {
        if (!selected_) return {};
        auto out=selected_->control();
        if (selected_->one_footer && settings_applied) {
            // CanOneFoot is owned by the original foot producer and may depend
            // on this input. Require the established grab, not that result.
            if (native.eligible(selected_)) feet_started_=true;
            if (feet_started_) out.feet=foot_button(selected_->foot);
        }
        return out;
    }
    const Pilot* selected() const noexcept {return selected_;}
    void cancel() noexcept {*this={};}
private:
    const Pilot* selected_{};
    std::uint64_t requested_{},refreshed_{};
    bool released_{},acknowledged_{},feet_started_{};
};
}
