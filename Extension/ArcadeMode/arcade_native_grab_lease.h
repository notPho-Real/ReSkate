#pragma once
#include "arcade_trick_table.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode::native_grab {
// These are Enum.DetailedGrabType values emitted by the original loop actors,
// not Enum.GrabType values and not requested animation IDs.
struct Pilot { ArcadeTrick trick; std::int32_t detail; float x, y; std::int32_t family{2}; unsigned hands{2}; };
inline constexpr Pilot pilots[]{{ArcadeTrick::basic_bs_grab,1,0,0},
    {ArcadeTrick::melon,4,0,1},{ArcadeTrick::method,2,1,0},
    {ArcadeTrick::japan,3,0,-1},{ArcadeTrick::crossbone,5,-1,0},
    {ArcadeTrick::indy,6,0,0,1,1},{ArcadeTrick::fs_nosebone,7,0,1,1,1},
    {ArcadeTrick::fs_tailbone,8,0,-1,1,1},{ArcadeTrick::tuck_knee,9,1,0,1,1},
    {ArcadeTrick::stiffy,10,-1,0,1,1},
    {ArcadeTrick::double_grab,33,0,0,9,3},{ArcadeTrick::double_grab_front,36,0,1,9,3},
    {ArcadeTrick::double_grab_back,37,0,-1,9,3},{ArcadeTrick::double_grab_left,34,-1,0,9,3},
    {ArcadeTrick::double_grab_right,35,1,0,9,3}};
// R3 held at the physical B press selects this layer. No existing row moves.
constexpr ArcadeTrick extended_grab(std::int16_t direction) noexcept {
    switch(direction) {
    case 7:return ArcadeTrick::crossbone; case 9:return ArcadeTrick::fs_nosebone;
    case 1:return ArcadeTrick::fs_tailbone; case 3:return ArcadeTrick::tuck_knee;
    case 8:return ArcadeTrick::double_grab_front; case 2:return ArcadeTrick::double_grab_back;
    case 4:return ArcadeTrick::double_grab_left; case 6:return ArcadeTrick::double_grab_right;
    default:return ArcadeTrick::double_grab;
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

// A physical request remains pending through a brief release until independent
// native actor advancement AND the native dominant-pose tag agree. The deadline
// is a failure bound, never a required input pulse or animation setup duration.
class Lease {
public:
    static constexpr std::uint64_t initialization_limit_ms=1500, heartbeat_ms=150;
    bool request(ArcadeTrick trick,std::uint64_t now,bool held) noexcept {
        const auto* p=pilot(trick);
        if (!p) return false;
        selected_=p; requested_=refreshed_=now; released_=!held; acknowledged_=false;
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
                     bool initialized,bool exact_actor,bool native_grabbing) noexcept {
        if (!selected_ || detail!=selected_->detail || !initialized || !exact_actor || !native_grabbing ||
            !std::isfinite(delta) || delta<=0 || !std::isfinite(before) || !std::isfinite(after) || after<=before) return false;
        acknowledged_=true;return true;
    }
    bool intent(std::uint64_t now) const noexcept {
        return selected_ && now>=refreshed_ && now-refreshed_<=heartbeat_ms &&
            (acknowledged_ || (now>=requested_ && now-requested_<=initialization_limit_ms)) &&
            !(released_ && acknowledged_);
    }
    bool acknowledged() const noexcept {return acknowledged_;}
    const Pilot* selected() const noexcept {return selected_;}
    void cancel() noexcept {*this={};}
private:
    const Pilot* selected_{};
    std::uint64_t requested_{},refreshed_{};
    bool released_{},acknowledged_{};
};
}
