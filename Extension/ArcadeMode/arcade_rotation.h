#pragma once
#include "arcade_board.h"
#include "arcade_spin_control.h"
#include "arcade_combo_session.h"
#include "arcade_gameplay_profile.h"
#include "arcade_native_input_state.h"
#include <algorithm>
#include <cmath>

namespace dingosdk::arcade_mode {
// Reconcile only verified physical classes for the native spin consumer.
// A client pose from the preceding tick cannot prolong airborne rotation.
inline SkatePose native_rotation_pose(const NativeInputFrame& frame, SkatePose observed, bool balance_admitted) noexcept {
    if (!frame.valid) return SkatePose::unknown;
    if (frame.airborne) return SkatePose::air;
    if (frame.rail_family) return SkatePose::grind;
    if (frame.state == 100 || frame.state == 103) {
        if (frame.state == 100 && balance_admitted && (observed == SkatePose::manual || observed == SkatePose::nose_manual)) return observed;
        return SkatePose::riding;
    }
    return SkatePose::unknown;
}
// Native LS spin/prewind method adapted from GoalTrains/reskatedualstick,
// src/translate.h at1181aa819bea2eb49042b3f67e6ff5c7c9c4e348 (GPL-3.0).
// Only controller translation is reused; angular motion remains game-owned.
class AirRotation {
public:
    struct Output { bool owns_left{}; std::int16_t x{}, y{}; float yaw_rate{}; SpinCommand spin{}; };
    // Recover the established native LS body-spin consumer for held shoulders.
    // Qualification above the actuator still owns episode, freshness and conflicts.
    // Do not simultaneously command native stick spin and an SDK body rate.
    static Output native_shoulder_output(Output output, SpinMode mode,
            std::uint16_t buttons, bool allowed) noexcept {
        const auto held = static_cast<std::uint16_t>(buttons & BoardChord::pair);
        if (allowed && mode == SpinMode::held && output.owns_left && output.yaw_rate != 0 &&
            (held == BoardChord::lb || held == BoardChord::rb)) {
            output.x = static_cast<std::int16_t>((output.yaw_rate < 0 ? -.97f : .97f) * 32767);
            output.y = 0;
            output.yaw_rate = 0; // Native body-spin owns angular speed and constraints.
        }
        return output;
    }
    // Separate preparation actor: never relax the airborne proof or own RS.
    Output poll_preparation(std::uint64_t now, std::uint16_t buttons, bool gameplay,
            const NativeInputFrame& frame, bool crouched, bool nollie, bool conflict, SpinMode mode) noexcept {
        const auto held=static_cast<std::uint16_t>(buttons & BoardChord::pair);
        const bool fresh=frame.valid && frame.owner.valid() && frame.owner_generation &&
            frame.contact_generation && frame.action_generation && frame.physics_step &&
            now>=frame.sampled_at && now-frame.sampled_at<=150;
        const bool safe=gameplay && mode==SpinMode::held && fresh && !conflict &&
            !(buttons&0x30u) && !frame.rail_family && (held==BoardChord::lb || held==BoardChord::rb);
        const bool same=prepared_.valid && prepared_.owner==frame.owner &&
            prepared_.owner_generation==frame.owner_generation && prepared_buttons_==held;
        if (!safe || (prepared_.valid && !same)) { prepared_={}; prepared_air_=0; }
        if (safe && same && frame.airborne && (frame.state==200 || frame.state==201) &&
            frame.airborne_episode && frame.airborne_started_known) {
            if (!prepared_air_ && now>=prepared_.sampled_at && now-prepared_.sampled_at<=250 &&
                frame.airborne_started_at>=prepared_.sampled_at && frame.air_departure==prepared_.air_departure)
                prepared_air_=frame.airborne_episode;
            if (prepared_air_==frame.airborne_episode && frame.air_departure==prepared_.air_departure)
                return {true,static_cast<std::int16_t>((held==BoardChord::lb?-.97f:.97f)*32767),0};
            prepared_={}; prepared_air_=0;
        }
        const bool ground=fresh && !frame.airborne && !frame.rail_family &&
            (frame.state==100 || frame.state==103);
        if (prepared_air_ && ground) { prepared_={}; prepared_air_=0; }
        auto output=poll(now,buttons,gameplay && mode==SpinMode::held,
            ground?SkatePose::riding:SkatePose::unknown,crouched,nollie,conflict);
        if (safe && ground && crouched && output.owns_left) {
            prepared_=frame; prepared_buttons_=held;
        }
        // Explicit buffered AIR spin, not a claim of native ground wind-up.
        // Only a fresh crouch-owned shoulder can bridge the bounded pop gap.
        if (safe && ground && prepared_.valid && now>=prepared_.sampled_at && now-prepared_.sampled_at<=250)
            output.owns_left=true;
        if (output.owns_left) output.x=output.y=0;
        return output;
    }
    void reset() noexcept {
        const auto epoch=qualified_.epoch;
        *this = {};
        qualified_.epoch=epoch==UINT64_MAX ? UINT64_MAX : epoch+1;
    }
    Output poll_qualified(std::uint64_t now,std::uint16_t buttons,bool left,bool right,
            bool gameplay,SkatePose pose,bool ready,bool conflict,SpinMode mode,SpinFacts facts={},
            const NativeInputFrame* native=nullptr) noexcept {
        if(!gameplay || (buttons&0x30u) || pose==SkatePose::unknown || pose==SkatePose::bail ||
            (qualified_clocked_ && now<qualified_at_)){reset();return {};}
        const bool mode_changed=qualified_.mode!=mode;
        if(mode_changed){reset();qualified_.mode=mode;}
        qualified_clocked_=true;qualified_at_=now;qualified_.facts=facts;
        const auto held=static_cast<std::uint16_t>(buttons&BoardChord::pair);
        if(!held&&!left&&!right)qualified_armed_=true;
        if(left!=left_held_){left_held_=left;left_at_=now;}
        if(right!=right_held_){right_held_=right;right_at_=now;}
        qualified_.left_pressed_at=left_at_;qualified_.right_pressed_at=right_at_;
        qualified_.physical_armed=qualified_armed_;
        const bool air=pose==SkatePose::air;
        const auto triggered=static_cast<std::uint16_t>(held&~qualified_buttons_);
        const bool entering=air&&!qualified_air_;
        qualified_buttons_=held;qualified_air_=air;
        const bool proof_ready=!native || (native->valid && native->owner.valid() && native->owner_generation &&
            native->contact_generation && native->action_generation && native->physics_step && native->airborne && native->airborne_episode && native->airborne_started_known &&
            (native->state==200 || native->state==201) && native->airborne_started_at<=native->sampled_at &&
            now>=native->sampled_at && now-native->sampled_at<=150);
        const bool native_held_ready = mode==SpinMode::held && native && proof_ready;
        if(!air || (!ready && !native_held_ready) || !proof_ready || conflict || !qualified_armed_ || facts.no_spin==SpinFact::set)return {false,0,0,0,qualified_};
        // A new physical edge may follow actual native AIR before the first
        // AIR input poll. A preheld or transition-coincident edge is distinct.
        const bool after_native_entry=native && proof_ready && now>native->airborne_started_at;
        bool tap=false;
        if(mode==SpinMode::tap && (!entering || after_native_entry) && !mode_changed){
            int change=0;
            if((triggered&BoardChord::lb)&&!(held&BoardChord::rb))++change;
            if((triggered&BoardChord::rb)&&!(held&BoardChord::lb))--change;
            if(change && qualified_.serial!=UINT64_MAX &&
                qualified_.balance>INT64_MIN && qualified_.balance<INT64_MAX){
                ++qualified_.serial;qualified_.balance+=change;tap=true;
            }
        }
        float rate=0;
        if(mode==SpinMode::held && held!=BoardChord::pair){
            if(held&BoardChord::lb)rate=-GameplayProfile{}.angular_speed();
            if(held&BoardChord::rb)rate=GameplayProfile{}.angular_speed();
        }
        if((mode==SpinMode::tap || !held) && facts.orientation_disabled!=SpinFact::set){
            // Right wins when both directional controls are supplied. Each
            // physical hold uses strict >100ms, with no later ramp interval.
            if(left && now-left_at_>100)rate=-GameplayProfile{}.angular_speed();
            if(right)rate=now-right_at_>100 ? GameplayProfile{}.angular_speed() : 0;
        }
        return {held!=0||left||right||tap,0,0,rate,qualified_};
    }
    Output poll(std::uint64_t now, std::uint16_t buttons, bool gameplay,
                SkatePose pose, bool crouched, bool native_nollie, bool conflict = false,
                bool native_rate_control = false) noexcept {
        if (!gameplay || (buttons & 0x0030u) || pose == SkatePose::unknown || pose == SkatePose::bail ||
            (clocked_ && now < previous_time_)) { reset(); return {}; }
        clocked_ = true; previous_time_ = now;
        const auto held = static_cast<std::uint16_t>(buttons & BoardChord::pair);
        if (!held) { armed_ = true; blocked_ = waiting_ = active_ = was_air_ = false; return {}; }
        if (native_rate_control && pose == SkatePose::air) {
            // Continuous rate control uses the current eligible held sample.
            // Opposing shoulders and temporary reservations produce zero only
            // while present; they do not consume a later held direction.
            blocked_ = waiting_ = active_ = false; was_air_ = true;
            if (!armed_ || conflict || held == BoardChord::pair) return {};
            const float side = held == BoardChord::lb ? -1.0f : 1.0f;
            return {true,0,0,side * GameplayProfile{}.angular_speed()};
        }
        if (native_rate_control && was_air_) {
            blocked_ = waiting_ = active_ = was_air_ = false;
        }
        if (!armed_ || blocked_) return {};
        // The stick/prewind fallback retains its existing chord arbitration.
        if (held == BoardChord::pair || conflict ||
            (pose != SkatePose::riding && pose != SkatePose::air)) {
            blocked_ = true; active_ = false; return {};
        }
        if (!waiting_) { waiting_ = true; pressed_at_ = now; }
        const bool air = pose == SkatePose::air;
        if (was_air_ && !air) { blocked_ = true; active_ = false; return {}; }
        if (now - pressed_at_ <= BoardChord::window_ms) return {};
        if (!active_) {
            if (!air && !crouched) return {};
            active_ = true; began_ = now; prewind_ = !air; nollie_ = native_nollie;
        }
        if (air) was_air_ = true;
        if (crouched || air) context_at_ = now;
        if (!air && !crouched && now - context_at_ > 250) {
            blocked_ = true; active_ = false; return {};
        }
        const float side = held == BoardChord::lb ? -1.0f : 1.0f;
        float x = side * .97f, y = 0;
        const auto elapsed = now - began_;
        if (prewind_ && elapsed < 90) {
            const float start_y = nollie_ ? 1.0f : -1.0f;
            if (elapsed < 24) { x = 0; y = start_y * static_cast<float>(elapsed) / 24.0f; }
            else {
                const float u = std::min(1.0f, static_cast<float>(elapsed - 24) / 36.0f);
                x = side * .8f * u; y = start_y * (1.0f - .4f * u);
                const float scale = .97f / std::sqrt(x * x + y * y);
                x *= scale; y *= scale;
            }
        }
        return {true, static_cast<std::int16_t>(x * 32767), static_cast<std::int16_t>(y * 32767)};
    }
private:
    SpinCommand qualified_{};
    NativeInputFrame prepared_{};
    std::uint64_t prepared_air_{};
    std::uint16_t prepared_buttons_{};
    bool qualified_clocked_{},qualified_armed_{},qualified_air_{},left_held_{},right_held_{};
    std::uint16_t qualified_buttons_{};
    std::uint64_t qualified_at_{},left_at_{},right_at_{};
    bool armed_{}, clocked_{}, blocked_{}, waiting_{}, active_{}, was_air_{}, prewind_{}, nollie_{};
    std::uint64_t previous_time_{}, pressed_at_{}, began_{}, context_at_{};
};
}
