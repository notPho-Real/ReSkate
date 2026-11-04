#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class GroundGrab : std::uint8_t { none, frontside, backside, double_grab, coffin };
struct GroundGrabObservation {
    bool valid{}, can_grab{}, left{}, right{}, coffin{};
    unsigned state{};
    std::uint64_t sampled_at{},step{};
    bool fresh(std::uint64_t now) const noexcept {
        return valid && step && now>=sampled_at && now-sampled_at<=150;
    }
};
struct GroundGrabControl {
    bool owned{};
    std::uint16_t buttons{};
    std::uint8_t left_trigger{},right_trigger{};
};
// These are original controller inputs, never animation or trick-state writes.
// Hand acknowledgment is a prerequisite, not proof of animation or scoring.
class GroundGrabInput {
public:
    bool start(GroundGrab kind,std::uint64_t now,const GroundGrabObservation& native) noexcept {
        if (active() || kind<GroundGrab::frontside || kind>GroundGrab::coffin ||
            !native.fresh(now) || native.state!=100 || native.coffin) return false;
        *this={};kind_=kind;started_=now;last_step_=native.step;return true;
    }
    GroundGrabControl poll(std::uint64_t now,const GroundGrabObservation& native,bool held) noexcept {
        if (!active()) return {};
        if (!held || !native.fresh(now) || native.step<last_step_ || native.state!=100 ||
            now<started_ || (!acknowledged_ && now-started_>1200)) {cancel();return {};}
        last_step_=native.step;
        const bool left=kind_!=GroundGrab::backside;
        const bool right=kind_!=GroundGrab::frontside;
        GroundGrabControl out{true,0,static_cast<std::uint8_t>(left?255:0),static_cast<std::uint8_t>(right?255:0)};
        if (!prepared_) {prepared_=true;prepared_at_=now;prepared_step_=native.step;return out;}
        const bool hands=native.can_grab && native.left==left && native.right==right;
        if (native.sampled_at>prepared_at_ && native.step>prepared_step_ && hands) {
            if (kind_==GroundGrab::coffin) issued_=true;
            else acknowledged_=true;
        }
        if (kind_==GroundGrab::coffin && issued_) {
            // Input_Coffin is the original A+X+both triggers producer. Delaying
            // feet until both native hands exist avoids an ordinary hippy jump.
            out.buttons=0x5000;
            if(native.coffin)acknowledged_=true;
        }
        return out;
    }
    bool active() const noexcept {return kind_!=GroundGrab::none;}
    GroundGrab kind() const noexcept {return kind_;}
    bool acknowledged() const noexcept {return acknowledged_;}
    void cancel() noexcept {*this={};}
private:
    GroundGrab kind_{};
    std::uint64_t started_{},prepared_at_{},prepared_step_{},last_step_{};
    bool prepared_{},issued_{},acknowledged_{};
};
}
