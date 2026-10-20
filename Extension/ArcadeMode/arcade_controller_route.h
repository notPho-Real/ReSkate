#pragma once
#include "arcade_gesture.h"
#include "arcade_controller_route_observation.h"
#include "arcade_native_input_state.h"
#include <array>

namespace dingosdk::arcade_mode {
// The native facade has one logical local-player session (0). Physical XInput
// indices are transport addresses, not native skater identities. Automatic mode
// requires one connected pad. An explicit, finite user policy can name one slot;
// it does not claim the engine selected that slot. Never merge device events.
// Caller serializes this adapter with the existing input-owner mutex.
class NativeControllerRoute {
public:
    static constexpr unsigned unavailable = 4;
    static constexpr unsigned session = 0;
    // Pre-filter outer game poll, used only for neutral binding admission.
    void observe_unfiltered(unsigned user, ArcadeInput::Physical pad) noexcept {
        if(user<raw_pads_.size())raw_pads_[user]=pad;
    }
    void observe(unsigned user, ArcadeInput::Physical pad) noexcept {
        if (user < pads_.size()) pads_[user] = pad;
    }
    // Arming is a policy request, not admission. The first neutral, uncaptured
    // sample and independent original-physics owner proof must arrive within5s.
    bool arm(unsigned user, std::uint64_t now, unsigned duration_ms) noexcept {
        if(user>=4 || duration_ms<1000 || duration_ms>300000 || now>UINT64_MAX-duration_ms)return false;
        explicit_=true; revoked_=bound_=false; requested_=user; armed_at_=now;
        deadline_=now+duration_ms; proof_={}; pads_[user]=raw_pads_[user]={}; selected_=unavailable; ++generation_;
        return true;
    }
    void clear() noexcept {
        explicit_=revoked_=true;bound_=false;selected_=unavailable;proof_={};++generation_;
    }
    bool disconnected(unsigned user) noexcept {
        if(user>=4)return false;
        pads_[user]=raw_pads_[user]={};
        if((explicit_ && requested_==user && !revoked_) || selected_==user){
            if(explicit_)revoked_=true;
            selected_=unavailable;++generation_;return true;
        }
        return false;
    }
    bool captured(unsigned user) noexcept {
        if(user>=4)return false;
        pads_[user]=raw_pads_[user]={};
        if(explicit_ && bound_ && selected_==user){clear();return true;}
        return false;
    }
    bool explicit_selection() const noexcept {return explicit_;}
    bool select(std::uint64_t now, unsigned connected, bool available, NativeInputFrame proof={}) noexcept {
        unsigned next = unavailable;
        if(explicit_){
            const bool source_ok=available && connected<16 && requested_<4 && (connected&(1u<<requested_));
            if(!revoked_){
                if(now<armed_at_ || now>=deadline_ || (!bound_ && now-armed_at_>5000))revoked_=true;
                else if(bound_){
                    if(!source_ok || !fresh(pads_[requested_],now) || !current(now) ||
                        !fresh_proof(proof,now) || proof.owner!=proof_.owner ||
                        proof.owner_generation!=proof_.owner_generation || proof.physics_step<proof_.physics_step ||
                        proof.sampled_at<proof_.sampled_at || proof.contact_generation<proof_.contact_generation ||
                        proof.action_generation<proof_.action_generation)revoked_=true;
                    else {next=requested_;proof_=proof;}
                }else if(source_ok && fresh(pads_[requested_],now) && fresh(raw_pads_[requested_],now) && neutral(raw_pads_[requested_]) && neutral(pads_[requested_]) && fresh_proof(proof,now)){
                    bound_=true;proof_=proof;next=requested_;
                }
            }
        }else if (available && connected && connected < 16 && !(connected & (connected-1))) {
            for (unsigned user = 0; user < pads_.size(); ++user)
                if (connected == (1u << user) && fresh(pads_[user],now)) next = user;
        }
        const bool changed = next != selected_ || (!explicit_ && connected != connected_) ||
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
        else if(explicit_){
            if(revoked_)r.reason=ControllerRouteReason::binding_revoked;
            else if(!bound_)r.reason=ControllerRouteReason::binding_waiting;
            else if(!accepts(selected_,now))r.reason=ControllerRouteReason::selection_stale;
            else {r.reason=ControllerRouteReason::ready;r.logical_actor=session;}
        }
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
        r.explicit_binding=explicit_;
        return r;
    }
    std::uint64_t generation() const noexcept { return generation_; }
    void reset() noexcept { explicit_=revoked_=bound_=false;requested_=unavailable;proof_={}; pads_ = raw_pads_ = {}; selected_ = unavailable; connected_ = 0; selected_at_ = 0; ++generation_; }
private:
    static bool neutral(const ArcadeInput::Physical& pad) noexcept {
        // Reuse the shared digital thresholds for both sticks and triggers.
        const auto left=InputEvents::normalize({pad.buttons,pad.lx,pad.ly,pad.lt,pad.rt},pad.at);
        const auto right=InputEvents::normalize({0,pad.rx,pad.ry,0,0},pad.at);
        return !left.buttons && !left.triggers && !right.buttons;
    }
    static bool fresh_proof(const NativeInputFrame& proof,std::uint64_t now) noexcept {
        return proof.valid && proof.state && proof.owner.valid() && proof.owner_generation && proof.physics_step &&
            now>=proof.sampled_at && now-proof.sampled_at<=150;
    }
    static bool fresh(const ArcadeInput::Physical& pad,std::uint64_t now) noexcept {
        return pad.valid && now >= pad.at && now-pad.at <= 150;
    }
    bool current(std::uint64_t now) const noexcept {
        return (!explicit_ || (bound_ && !revoked_ && now>=armed_at_ && now<deadline_ && fresh_proof(proof_,now))) &&
            selected_ != unavailable && now >= selected_at_ && now-selected_at_ <= 150;
    }
    bool explicit_{},revoked_{},bound_{};
    unsigned requested_{unavailable};
    std::uint64_t armed_at_{},deadline_{};
    NativeInputFrame proof_{};
    std::array<ArcadeInput::Physical,4> pads_{},raw_pads_{};
    unsigned selected_{unavailable}, connected_{};
    std::uint64_t selected_at_{}, generation_{};
};
}
