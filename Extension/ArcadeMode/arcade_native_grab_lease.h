#pragma once
#include "arcade_trick_table.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode::native_grab {
// These are Enum.DetailedGrabType values emitted by the original loop actors,
// not Enum.GrabType values and not requested animation IDs.
struct Pilot { ArcadeTrick trick; std::int32_t detail; float x, y; };
inline constexpr Pilot pilots[]{{ArcadeTrick::basic_bs_grab,1,0,0},
    {ArcadeTrick::melon,4,0,1},{ArcadeTrick::method,2,1,0}};
constexpr const Pilot* pilot(ArcadeTrick trick) noexcept {
    for (const auto& p:pilots) if (p.trick==trick) return &p;
    return nullptr;
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
