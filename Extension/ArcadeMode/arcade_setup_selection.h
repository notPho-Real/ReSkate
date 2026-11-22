#pragma once
#include "arcade_ground_actions.h"
#include "arcade_trigger_input.h"

namespace dingosdk::arcade_mode {
struct SetupSelectionContext {
    InputContext context{};
    InputNativeContext native{};
    bool enabled{};
};
// Physical setup preference only. This supplies neither a native Pressure
// actor nor permission to launch. TriggerJournal owns every physical edge.
class PhysicalSetupSelection {
public:
    // Explicit implementation tuning, not a historical PS2 timing claim.
    // Classification never postpones A or changes its physical timestamps.
    static constexpr std::uint64_t pressure_hold_ms=200;
    void reset() noexcept { *this={}; }
    void sample(std::uint64_t now,const TriggerPress& trigger,SetupSelectionContext frame) noexcept {
        const auto sequence=trigger.press.sequence;
        // Native0x40 means pushing, not bail. A riding push cannot erase a
        // remembered setup; native launch admission remains the caller's job.
        if(!frame.enabled || !frame.native.required || !frame.native.fresh(now) ||
           (clocked_&&now<now_)) {
            reset();cursor_=sequence;return;
        }
        const auto owner=frame.native.generations.owner;
        if(owner_&&owner_!=owner) {
            reset();cursor_=sequence;owner_=owner;epoch_at_=now;
        }
        if(!owner_) {owner_=owner;epoch_at_=now;}
        now_=now;clocked_=true;
        const bool eligible=setup_context(frame.context)&&!frame.native.airborne;
        const bool air=frame.context==InputContext::air&&frame.native.airborne;
        if(!eligible&&!air) {
            base_=SetupStance::normal;launch_=SetupStance::unknown;clear_press();
            cursor_=sequence;context_=frame.context;epoch_at_=now;ready_=false;return;
        }
        ready_=true;
        if(air) {
            if(launch_!=SetupStance::unknown) {
                if(!flight_.number)flight_=frame.native.air_episode();
                else if(flight_!=frame.native.air_episode()) {launch_=SetupStance::unknown;clear_press();}
            }
            // Only a release of the original setup press can finish in AIR.
            // A fresh airborne LT belongs to another context, never setup.
            if(launch_==SetupStance::unknown || sequence!=press_sequence_) {cursor_=sequence;clear_press();}
        } else if(context_!=frame.context) {
            if(context_!=InputContext::unknown) {
                clear_press();cursor_=sequence;epoch_at_=now;
                if(context_==InputContext::air) {launch_=SetupStance::unknown;flight_={};}
            }
        }
        context_=frame.context;
        if(trigger.owner!=TriggerOwner::setup || trigger.channel()!=0 ||
           trigger.press.token!=InputToken{InputSource::triggers,0,InputEdge::press}) {
            if(sequence==press_sequence_)clear_press();
            cursor_=std::max(cursor_,sequence);return;
        }
        if(sequence>cursor_) {
            cursor_=sequence;
            if(!eligible || trigger.context!=frame.context || trigger.press.at<epoch_at_ ||
               trigger.press.at>now || !trigger.native.required || !trigger.native.fresh(trigger.press.at) ||
               trigger.native.generations!=frame.native.generations) {clear_press();return;}
            press_sequence_=sequence;pressed_at_=trigger.press.at;down_=true;
        }
        if(sequence!=press_sequence_ || !down_)return;
        if(trigger.released) {
            if(trigger.release.token!=InputToken{InputSource::triggers,0,InputEdge::release} ||
               trigger.release.sequence<=sequence || trigger.release.at<pressed_at_ || trigger.release.at>now) {
                clear_press();return;
            }
            if(trigger.release.at-pressed_at_<pressure_hold_ms)
                base_=base_==SetupStance::nollie?SetupStance::normal:SetupStance::nollie;
            clear_press();
        }
    }
    SetupStance desired(bool a_held=false) const noexcept {
        if(!ready_)return SetupStance::unknown;
        if(launch_!=SetupStance::unknown)return launch_;
        return desired_setup(a_held);
    }
    // A fresh grounded A may replace a pop which never reached AIR. Its
    // prospective setup is independent of that previous launch snapshot.
    SetupStance desired_setup(bool a_held=false) const noexcept {
        if(!ready_)return SetupStance::unknown;
        if(context_==InputContext::air)return SetupStance::normal;
        if(down_) {
            if(now_-pressed_at_>=pressure_hold_ms)return SetupStance::pressure;
            if(a_held)return base_==SetupStance::nollie?SetupStance::normal:SetupStance::nollie;
        }
        return base_;
    }
    // Call only for an actually admitted A release, at its physical timestamp.
    // A delayed callback cannot retroactively classify a later LT hold as the
    // released launch. The caller retains its original launch snapshot instead.
    bool freeze_launch(std::uint64_t released_at,SetupStance selected=SetupStance::unknown) noexcept {
        if(!ready_ || released_at!=now_ || launch_!=SetupStance::unknown ||
           !setup_context(context_))return false;
        launch_=selected==SetupStance::unknown?desired(true):selected;flight_={};return true;
    }
    SetupStance launch() const noexcept {return launch_;}
    SetupStance remembered() const noexcept {return base_;}
    bool trigger_held() const noexcept {return down_;}
    void cancel_launch() noexcept {launch_=SetupStance::unknown;flight_={};}
    void landed() noexcept {launch_=SetupStance::unknown;flight_={};clear_press();}
private:
    static constexpr bool setup_context(InputContext context) noexcept {
        return context==InputContext::ground || context==InputContext::balance || context==InputContext::rail;
    }
    void clear_press() noexcept {press_sequence_=pressed_at_=0;down_=false;}
    SetupStance base_{SetupStance::normal},launch_{};
    InputContext context_{};
    InputAirEpisode flight_{};
    std::uint64_t owner_{},epoch_at_{},cursor_{},press_sequence_{},pressed_at_{},now_{};
    bool clocked_{},ready_{},down_{};
};
}
