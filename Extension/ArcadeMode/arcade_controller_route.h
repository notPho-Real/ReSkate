#pragma once
#include "arcade_gesture.h"
#include "arcade_controller_route_observation.h"
#include <array>

namespace dingosdk::arcade_mode {
// The native facade has one logical local-player session (0). Physical XInput
// indices are transport addresses, not native skater identities. Only a sole
// connected XInput source may feed that session; never choose from merged pads.
// Caller serializes this adapter with the existing input-owner mutex.
class NativeControllerRoute {
public:
    static constexpr unsigned unavailable = 4;
    static constexpr unsigned session = 0;
    void observe(unsigned user, ArcadeInput::Physical pad) noexcept {
        if (user < pads_.size()) pads_[user] = pad;
    }
    bool select(std::uint64_t now, unsigned connected, bool available) noexcept {
        unsigned next = unavailable;
        if (available && connected && connected < 16 && !(connected & (connected-1))) {
            for (unsigned user = 0; user < pads_.size(); ++user)
                if (connected == (1u << user) && fresh(pads_[user],now)) next = user;
        }
        const bool changed = next != selected_ || connected != connected_ ||
            (selected_ != unavailable && !current(now));
        if (changed) ++generation_;
        selected_ = next; connected_ = connected; selected_at_ = now;
        return changed;
    }
    bool accepts(unsigned user, std::uint64_t now) const noexcept {
        return user < pads_.size() && user == selected_ && current(now) && fresh(pads_[user],now);
    }
    ArcadeInput::Physical physical(std::uint64_t now) const noexcept {
        return accepts(selected_,now) ? pads_[selected_] : ArcadeInput::Physical{};
    }
    ControllerRouteEvidence observation(std::uint64_t now,bool enabled,bool source_available) const noexcept {
        ControllerRouteEvidence r{now,generation_,0,selected_,unavailable,
            source_available && connected_<16,enabled,source_available};
        if(r.bitmap_known)r.bitmap=connected_;
        if(!enabled)r.reason=ControllerRouteReason::off;
        else if(!source_available)r.reason=ControllerRouteReason::source_unavailable;
        else if(connected_>=16)r.reason=ControllerRouteReason::unsupported_source;
        else if(!connected_)r.reason=ControllerRouteReason::no_connected_xinput;
        else if(connected_&(connected_-1))r.reason=ControllerRouteReason::multiple_connected;
        else {
            unsigned candidate=0;
            while(connected_!=(1u<<candidate))++candidate;
            const auto& pad=pads_[candidate];
            if(!pad.valid)r.reason=ControllerRouteReason::no_physical_sample;
            else if(now<pad.at)r.reason=ControllerRouteReason::physical_clock_future;
            else if(now-pad.at>150)r.reason=ControllerRouteReason::physical_sample_stale;
            else if(selected_!=candidate)r.reason=ControllerRouteReason::selection_unavailable;
            else if(now<selected_at_)r.reason=ControllerRouteReason::selection_clock_future;
            else if(now-selected_at_>150)r.reason=ControllerRouteReason::selection_stale;
            else {r.reason=ControllerRouteReason::ready;r.logical_actor=session;}
        }
        return r;
    }
    std::uint64_t generation() const noexcept { return generation_; }
    void reset() noexcept { pads_ = {}; selected_ = unavailable; connected_ = 0; selected_at_ = 0; ++generation_; }
private:
    static bool fresh(const ArcadeInput::Physical& pad,std::uint64_t now) noexcept {
        return pad.valid && now >= pad.at && now-pad.at <= 150;
    }
    bool current(std::uint64_t now) const noexcept {
        return selected_ != unavailable && now >= selected_at_ && now-selected_at_ <= 150;
    }
    std::array<ArcadeInput::Physical,4> pads_{};
    unsigned selected_{unavailable}, connected_{};
    std::uint64_t selected_at_{}, generation_{};
};
}
