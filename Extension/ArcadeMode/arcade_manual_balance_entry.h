#pragma once
#include "arcade_manual_acquisition.h"

namespace dingosdk::arcade_mode {
// The basic gesture already has a native actuator. Start its physical
// balance on a NEW matching wheel contact after attributable delivery, independently
// of delayed name/history presentation. This does not identify an animation,
// advanced variant, native balance component or a monetary scoring action.
class GroundManualBalance {
public:
    void reset() noexcept { retire_air(); pending_ = confirmed_ = clocked_ = air_origin_ = held_air_ = landed_ = false; }
    void observe(std::uint64_t now, const NativeInputFrame& frame,
                 const ManualInputDiagnostic& command, bool eligible) noexcept {
        // Once acquired, physical contact owns continuation and terminal display.
        // Withdrawing the pad at a hop/endpoint must not invent physical exit.
        if (confirmed_) return;
        if (frame.airborne) {
            observe_air(now,frame,command,eligible);
            return;
        }
        if (!eligible || !fresh(now,frame) || !command.owns || !command.safe ||
            (command.request != -1 && command.request != 1) || command.request_at > now ||
            frame.state != 100 || frame.mode != 1 || (frame.stance_and_push & 0x40u)) {
            reset(); return;
        }
        const auto identity_at = command.from_air ? command.recognized_at : command.request_at;
        if (spent_ && frame.owner == spent_owner_ && frame.owner_generation == spent_generation_ &&
            identity_at == spent_at_ && command.request == spent_direction_) {
            pending_ = false; return;
        }
        if (pending_ && same_owner(frame) && command.from_air == air_origin_ &&
            identity_at == began_ && command.request == direction_) {
            delivered_at_ = now; return;
        }
        retire_air();
        pending_ = false;
        // Existing one-wheel contact, including a kick, is never a new entry.
        // Unattributed air preparations retain their independent history route.
        const auto& revert=command.revert;
        const bool linked = revert.owner && revert.owner==frame.owner_generation && revert.ack_at &&
            now>=revert.ack_at && now-revert.ack_at<=1100 && command.request_at>=revert.ack_at &&
            frame.physics_step>revert.step && frame.contact_generation>revert.contact &&
            (frame.stance_and_push&0x20u)!=revert.previous_stance &&
            frame.contacts==(command.request<0?0x4000u:0x8000u);
        if (command.from_air || (frame.contacts != 0xc000u && !linked)) return;
        owner_ = frame.owner; generation_ = frame.owner_generation;
        step_ = frame.physics_step; contact_generation_ = frame.contact_generation;
        // Already-present opposite contact is attributable only to this
        // acknowledged stance-changing link. Still require a subsequent real
        // physics step after owned RS delivery before admitting balance.
        if(linked) contact_generation_=revert.contact;
        began_ = acquisition_at_ = command.request_at; first_delivery_at_ = delivered_at_ = now;
        direction_ = command.request; pending_ = true; clocked_ = false;
        air_origin_ = false; landed_ = true;
    }
    ManualObservationResult sample(std::uint64_t now, const NativeInputFrame& frame,
                                   bool valid, bool may_begin, bool continuous = false) noexcept {
        if (!pending_ && !confirmed_) return {};
        if (pending_ && air_origin_) {
            const bool live = valid && fresh(now,frame) && same_owner(frame) &&
                now >= began_ && (landed_ || now-began_ <= ManualActuation::air_request_ms || held_air_) &&
                now >= delivered_at_ && now-delivered_at_ <= 150;
            if (!live || (frame.airborne ? landed_ || !same_air(frame) : !matching_landing(frame))) {
                retire_air(); reset(); return {};
            }
            if (frame.airborne) { air_frame_ = frame; return {}; } // Retain the latest proved phase, not a new intent.
            // Start the bounded acquisition clock once at the qualified native
            // landing, independently of the original AIR gesture's identity.
            // Cached client views and renewed delivery cannot restart it.
            if (!landed_) { landed_ = true; acquisition_at_ = frame.sampled_at; }
        }
        if (!valid || !fresh(now,frame) || !same_owner(frame) ||
            frame.state != 100 || frame.mode != 1 || (frame.stance_and_push & 0x40u) ||
            (clocked_ && (now < last_at_ || (!continuous && now-last_at_ > 150) || frame.physics_step < last_step_))) {
            reset(); return {};
        }
        clocked_ = true; last_at_ = now; last_step_ = frame.physics_step;
        const auto wanted = direction_ < 0 ? SkatePose::manual : SkatePose::nose_manual;
        const auto contacts = direction_ < 0 ? 0x4000u : 0x8000u;
        if (confirmed_) {
            if (frame.contacts == contacts) contact_at_ = frame.sampled_at;
            else if (!may_begin || frame.contacts != 0xc000u || now < contact_at_ || now-contact_at_ >= 100) {
                reset(); return {};
            }
            return {wanted,true,false};
        }
        if (!may_begin || now < acquisition_at_ || now-acquisition_at_ >= ManualActuation::acquisition_ms ||
            now < delivered_at_ || now-delivered_at_ > 150) {
            reset(); return {};
        }
        if (frame.contacts == 0xc000u) return {};
        if (frame.contacts != contacts) { reset(); return {}; }
        // A later client poll of the same physics frame is not a new contact.
        if (frame.physics_step <= step_ || frame.contact_generation <= contact_generation_ ||
            frame.sampled_at < first_delivery_at_) return {};
        confirmed_ = true; pending_ = false; contact_at_ = frame.sampled_at;
        spent_ = true; spent_owner_ = owner_; spent_generation_ = generation_;
        spent_at_ = began_; spent_direction_ = direction_;
        return {wanted,true,true};
    }
private:
    bool same_air(const NativeInputFrame& frame) const noexcept {
        const bool next_phase = air_frame_.state==200 && frame.state==201 &&
            air_frame_.action_generation!=UINT64_MAX && air_frame_.contact_generation!=UINT64_MAX;
        return frame.airborne && frame.contacts == 0 && (frame.state == air_frame_.state || next_phase) &&
            frame.mode == air_frame_.mode && frame.airborne_episode == air_frame_.airborne_episode &&
            frame.air_departure == air_frame_.air_departure && frame.action_generation == air_frame_.action_generation+((next_phase || frame.flip_id!=air_frame_.flip_id)?1u:0u) &&
            frame.contact_generation == air_frame_.contact_generation+(next_phase?1u:0u) && frame.mode_generation == air_frame_.mode_generation && same_selection(frame) &&
            frame.physics_step >= air_frame_.physics_step && frame.sampled_at >= air_frame_.sampled_at;
    }
    bool same_selection(const NativeInputFrame& frame) const noexcept {
        return frame.flip_known && (frame.flip_id == air_frame_.flip_id ||
            (air_frame_.flip_id >= 0 && frame.flip_id == -1)) && frame.selection_object == air_frame_.selection_object &&
            frame.stance_and_push == air_frame_.stance_and_push;
    }
    bool matching_landing(const NativeInputFrame& frame) const noexcept {
        return !frame.airborne && frame.state == 100 && frame.mode == 1 &&
            frame.airborne_episode == air_frame_.airborne_episode &&
            air_frame_.air_departure != UINT64_MAX && frame.air_departure == air_frame_.air_departure+1 &&
            air_frame_.action_generation != UINT64_MAX && frame.action_generation == air_frame_.action_generation+1 &&
            air_frame_.contact_generation != UINT64_MAX &&
            frame.contact_generation >= air_frame_.contact_generation+1 &&
            (landed_ || frame.contact_generation-air_frame_.contact_generation<=2) &&
            frame.mode_generation >= air_frame_.mode_generation &&
            frame.mode_generation-air_frame_.mode_generation == (air_frame_.mode == 1 ? 0u : 1u) &&
            frame.physics_step > air_frame_.physics_step &&
            same_selection(frame) && (frame.contacts == 0xc000u || frame.contacts == (direction_ < 0 ? 0x4000u : 0x8000u));
    }
    void retire_air() noexcept {
        if (!pending_ || !air_origin_) return;
        spent_ = true; spent_owner_ = owner_; spent_generation_ = generation_;
        spent_at_ = began_; spent_direction_ = direction_;
    }
    void observe_air(std::uint64_t now, const NativeInputFrame& frame,
                     const ManualInputDiagnostic& command, bool eligible) noexcept {
        const bool continuing_hold = pending_ && air_origin_ && !landed_ && held_air_ && command.held_air &&
            same_owner(frame) && began_ == command.recognized_at && direction_ == command.request;
        if ((pending_ && landed_) || !eligible || !fresh(now,frame) || !command.owns || !command.safe || !command.from_air ||
            !command.recognized_at || command.request_at != command.recognized_at ||
            (command.request != -1 && command.request != 1) || now < command.recognized_at ||
            (now-command.recognized_at > ManualActuation::air_request_ms && !continuing_hold) ||
            (frame.state != 200 && frame.state != 201) || (frame.mode != 1 && frame.mode != 2) ||
            !frame.airborne_episode || !frame.mode_generation || frame.contacts || (frame.stance_and_push & 0x40u) ||
            !frame.flip_known || frame.flip_id < -1) {
            retire_air(); pending_ = air_origin_ = false; return;
        }
        if (spent_ && frame.owner == spent_owner_ && frame.owner_generation == spent_generation_ &&
            command.recognized_at == spent_at_ && command.request == spent_direction_) return;
        if (pending_ && air_origin_ && same_owner(frame) && began_ == command.recognized_at && direction_ == command.request) {
            if (!same_air(frame)) { retire_air(); pending_ = air_origin_ = false; return; }
            air_frame_ = frame;
            held_air_ = held_air_ && command.held_air;
            delivered_at_ = now; return; // Only delivery freshness renews, never intent lifetime.
        }
        owner_ = frame.owner; generation_ = frame.owner_generation;
        step_ = frame.physics_step; contact_generation_ = frame.contact_generation;
        began_ = command.recognized_at; first_delivery_at_ = delivered_at_ = now;
        direction_ = command.request; air_frame_ = frame;
        pending_ = air_origin_ = true; held_air_ = command.held_air; clocked_ = landed_ = false;
    }
    static bool fresh(std::uint64_t now, const NativeInputFrame& frame) noexcept {
        return frame.valid && frame.owner.valid() && frame.owner_generation && frame.contact_generation &&
            frame.action_generation && frame.physics_step && frame.sampled_at <= now && now-frame.sampled_at <= 150;
    }
    bool same_owner(const NativeInputFrame& frame) const noexcept {
        return frame.owner == owner_ && frame.owner_generation == generation_;
    }
    NativeInputOwner owner_{}, spent_owner_{};
    NativeInputFrame air_frame_{};
    std::uint64_t generation_{}, began_{}, acquisition_at_{}, first_delivery_at_{}, delivered_at_{}, step_{}, contact_generation_{};
    std::uint64_t contact_at_{}, last_at_{}, last_step_{}, spent_generation_{}, spent_at_{};
    int direction_{}, spent_direction_{};
    bool pending_{}, confirmed_{}, clocked_{}, spent_{}, air_origin_{}, held_air_{}, landed_{};
};
inline ManualObservationResult manual_balance_observation(const ManualObservationResult& ground,
                                                         const ManualObservationResult& history) noexcept {
    return ground.confirmed ? ground : history;
}
}
