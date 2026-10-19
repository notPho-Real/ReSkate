#pragma once
#include "arcade_gameplay_profile.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class SpinMode : std::uint8_t { held, tap };
enum class SpinFact : std::uint8_t { unknown, clear, set };
struct SpinFacts {
    SpinFact no_spin{}, spine{}, orientation_disabled{};
    constexpr unsigned encode() const noexcept {
        return static_cast<unsigned>(no_spin) | (static_cast<unsigned>(spine)<<2) | (static_cast<unsigned>(orientation_disabled)<<4);
    }
    static constexpr SpinFacts decode(unsigned bits) noexcept {
        const auto fact=[](unsigned value){return value<=2 ? static_cast<SpinFact>(value) : SpinFact::unknown;};
        return {fact(bits&3u),fact((bits>>2)&3u),fact((bits>>4)&3u)};
    }
};
// Physical edge counters survive repeated polls. The physics consumer alone
// advances pending angle, using its actual step. Epoch changes revoke old work.
struct SpinCommand {
    SpinMode mode{};
    std::uint64_t epoch{}, serial{};
    std::int64_t balance{};
    SpinFacts facts{};
};
class TapSpin {
public:
    static constexpr double pi = 3.14159265358979323846;
    void baseline(const SpinCommand& command) noexcept {
        command_=command;remaining_=0;primed_=true;tap_step_=false;
    }
    double remaining() const noexcept { return remaining_; }
    bool tap_step() const noexcept { return tap_step_; }
    float step(const SpinCommand& command,float directional_rate,float dt,float rate) noexcept {
        tap_step_=false;
        if(command.facts.no_spin==SpinFact::set)return 0;
        if(!primed_ || command.epoch!=command_.epoch || command.mode!=command_.mode){baseline(command);return directional_rate;}
        if(command.serial<command_.serial ||
            !std::isfinite(dt)||dt<=0||!std::isfinite(rate)||rate<=0)return 0;
        const long double delta=static_cast<long double>(command.balance)-static_cast<long double>(command_.balance);
        const auto events=command.serial-command_.serial;
        if(std::abs(delta)>static_cast<long double>(events) || std::abs(delta)>1000000){baseline(command);return 0;}
        command_=command;
        if(command.mode==SpinMode::held){remaining_=0;return directional_rate;}
        remaining_+=static_cast<double>(delta)*pi;
        if(remaining_==0)return directional_rate;
        tap_step_=true;
        const double signed_rate=remaining_>0 ? rate : -rate;
        const double increment=signed_rate*dt;
        // Deliberately retain the full final step: the source clears pending
        // on strict overshoot, and still applies this step's selected rate.
        if(std::abs(increment)>std::abs(remaining_))remaining_=0;
        else remaining_-=increment;
        // Existing native local-Y adapter has the opposite semantic handedness.
        return static_cast<float>(-signed_rate);
    }
private:
    SpinCommand command_{};
    double remaining_{};
    bool primed_{},tap_step_{};
};
}
