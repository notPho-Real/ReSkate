#pragma once
#include "arcade_input_events.h"

namespace dingosdk::arcade_mode {
enum class TriggerOwner : std::uint8_t {
    none, setup, switch_stance, manual_pivot, landing_revert, transfer, cancelled
};
// This is provenance from the existing physical journal, not another edge
// sampler. The original context and native observation never follow the player
// into a later context. A retained edge may be qualified only by its own owner.
struct TriggerPress {
    InputEvent press{}, release{};
    InputContext context{};
    InputNativeContext native{};
    TriggerOwner owner{};
    std::uint64_t deadline{};
    bool released{};
    bool operator==(const TriggerPress&) const = default;
    explicit operator bool() const noexcept { return press.sequence!=0; }
    unsigned channel() const noexcept { return static_cast<unsigned>(press.token.code); }
    std::uint8_t mask() const noexcept { return *this && channel()<2 ? static_cast<std::uint8_t>(1u<<channel()) : 0; }
    bool held_for(std::uint64_t now,std::uint64_t duration) const noexcept {
        const auto end=released?release.at:now;
        return *this && end>=press.at && end-press.at>=duration;
    }
};
struct TriggerDecision {
    TriggerOwner owner{};
    // Original-edge deadline, never a polling-relative renewal. A reservation
    // consumes the edge too; the action adapter may wait for actual contact.
    std::uint64_t deadline{};
    bool ready{}; // False reserves the edge while awaiting actual native contact.
};
struct TriggerRouting {
    void* context{};
    TriggerDecision (*qualify)(void*,const TriggerPress&,const InputNativeContext&,std::uint64_t) noexcept{};
};
class TriggerJournal {
public:
    static constexpr std::uint64_t context_window_ms=500;
    void reset() noexcept { presses_={}; cursor_=0; }
    void observe(InputHistory& history,InputContext context,const InputNativeContext& native) noexcept {
        for(const auto& event:history.events()) {
            if(event.sequence<=cursor_)continue;
            cursor_=event.sequence;
            if(event.token.source!=InputSource::triggers || event.token.code<0 || event.token.code>1)continue;
            auto& value=presses_[static_cast<unsigned>(event.token.code)];
            if(event.token.edge==InputEdge::press) {
                // InputEvents is the sole edge producer. Never reconstruct an
                // edge from a held trigger after reset, takeoff or touchdown.
                value={event,{},context,native,event.owners?TriggerOwner::cancelled:TriggerOwner::none};
            } else if(value && !value.released && event.at>=value.press.at) {
                value.release=event;value.released=true;
                if(value.owner!=TriggerOwner::none)
                    history.consume(InputMatch::single(event),InputConsumption::trigger);
            }
        }
    }
    const TriggerPress& get(unsigned channel) const noexcept {
        static const TriggerPress empty{};
        return channel<2?presses_[channel]:empty;
    }
    bool claim(unsigned channel,TriggerDecision decision,InputHistory& history,std::uint64_t now) noexcept {
        if(channel>=2 || decision.owner==TriggerOwner::none)return false;
        auto& value=presses_[channel];
        if(!value || value.owner!=TriggerOwner::none || now<value.press.at)return false;
        if((decision.owner==TriggerOwner::setup && (channel!=0 ||
                (value.context!=InputContext::ground && value.context!=InputContext::balance && value.context!=InputContext::rail))) ||
           (decision.owner==TriggerOwner::switch_stance && (channel!=1 || value.context!=InputContext::ground)) ||
           (decision.owner==TriggerOwner::manual_pivot && (channel!=1 || value.context!=InputContext::balance)) ||
           (decision.owner==TriggerOwner::transfer && value.context!=InputContext::air) ||
           (decision.owner==TriggerOwner::landing_revert && value.context!=InputContext::air && value.context!=InputContext::ground))return false;
        if(decision.owner!=TriggerOwner::setup && decision.owner!=TriggerOwner::cancelled &&
           (now-value.press.at>context_window_ms || decision.deadline<now ||
            decision.deadline-value.press.at>context_window_ms))return false;
        auto match=InputMatch::single(value.press);
        if(value.released) {
            match.events[1]=value.release.sequence;match.count=2;match.last_at=value.release.at;
        }
        if(!history.consume(match,InputConsumption::trigger))return false;
        value.owner=decision.owner;value.deadline=decision.deadline;
        return true;
    }
    std::uint8_t owned_held() const noexcept {
        std::uint8_t mask{};
        for(const auto& value:presses_)if(value && !value.released && value.owner!=TriggerOwner::none)mask|=value.mask();
        return mask;
    }
private:
    std::array<TriggerPress,2> presses_{};
    std::uint64_t cursor_{};
};
}
