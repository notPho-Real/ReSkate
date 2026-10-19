#pragma once
#include "arcade_manual_acquisition.h"

namespace dingosdk::arcade_mode {
// The basic ground gesture already has a native actuator. Start its physical
// balance on a NEW matching wheel contact after actual delivery, independently
// of delayed name/history presentation. This does not identify an animation,
// advanced variant, native balance component or a monetary scoring action.
class GroundManualBalance {
public:
    void reset() noexcept { pending_ = confirmed_ = clocked_ = false; }
    void observe(std::uint64_t now, const NativeInputFrame& frame,
                 const ManualInputDiagnostic& command, bool eligible) noexcept {
        // Once acquired, physical contact owns continuation and terminal display.
        // Withdrawing the pad at a hop/endpoint must not invent physical exit.
        if (confirmed_) return;
        if (!eligible || !fresh(now,frame) || !command.owns || !command.safe ||
            (command.request != -1 && command.request != 1) || command.request_at > now ||
            frame.state != 100 || frame.mode != 1 || (frame.stance_and_push & 0x40u)) {
            pending_ = false; return;
        }
        if (spent_ && frame.owner == spent_owner_ && frame.owner_generation == spent_generation_ &&
            command.request_at == spent_at_ && command.request == spent_direction_) {
            pending_ = false; return;
        }
        if (pending_ && same_owner(frame) && command.request_at == began_ && command.request == direction_) {
            delivered_at_ = now; return;
        }
        pending_ = false;
        // Existing one-wheel contact, including a kick, is never a new entry.
        // An air preparation needs observed two-wheel ground delivery first;
        // direct airborne-to-one-wheel entries retain their history route.
        if (frame.contacts != 0xc000u) return;
        owner_ = frame.owner; generation_ = frame.owner_generation;
        step_ = frame.physics_step; contact_generation_ = frame.contact_generation;
        began_ = command.request_at; first_delivery_at_ = delivered_at_ = now;
        direction_ = command.request; pending_ = true; clocked_ = false;
    }
    ManualObservationResult sample(std::uint64_t now, const NativeInputFrame& frame,
                                   bool valid, bool may_begin) noexcept {
        if (!pending_ && !confirmed_) return {};
        if (!valid || !fresh(now,frame) || !same_owner(frame) ||
            frame.state != 100 || frame.mode != 1 || (frame.stance_and_push & 0x40u) ||
            (clocked_ && (now < last_at_ || now-last_at_ > 150 || frame.physics_step < last_step_))) {
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
        if (!may_begin || now < began_ || now-began_ >= ManualActuation::acquisition_ms ||
            now < delivered_at_ || now-delivered_at_ > 150) {
            pending_ = false; return {};
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
    static bool fresh(std::uint64_t now, const NativeInputFrame& frame) noexcept {
        return frame.valid && frame.owner.valid() && frame.owner_generation && frame.contact_generation &&
            frame.action_generation && frame.physics_step && frame.sampled_at <= now && now-frame.sampled_at <= 150;
    }
    bool same_owner(const NativeInputFrame& frame) const noexcept {
        return frame.owner == owner_ && frame.owner_generation == generation_;
    }
    NativeInputOwner owner_{}, spent_owner_{};
    std::uint64_t generation_{}, began_{}, first_delivery_at_{}, delivered_at_{}, step_{}, contact_generation_{};
    std::uint64_t contact_at_{}, last_at_{}, last_step_{}, spent_generation_{}, spent_at_{};
    int direction_{}, spent_direction_{};
    bool pending_{}, confirmed_{}, clocked_{}, spent_{};
};
inline ManualObservationResult manual_balance_observation(const ManualObservationResult& ground,
                                                         const ManualObservationResult& history) noexcept {
    return ground.confirmed ? ground : history;
}
}
