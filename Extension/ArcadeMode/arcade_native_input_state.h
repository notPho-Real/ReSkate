#pragma once
#include "arcade_native_probe.h"
#include "arcade_native_trick.h"
#include "Engine/Game/Build/20260929/grind_exit.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Internal owner identity, never serialized. Each field comes from the existing
// local owner resolver before and after the native reads.
struct NativeInputOwner {
    std::uintptr_t base{}, entity{}, core{}, context{}, rig{};
    bool operator==(const NativeInputOwner&) const = default;
    bool valid() const noexcept { return base && entity && core && context && rig; }
};
struct NativeInputFrame {
    bool valid{}, rail_family{}, flip_known{};
    NativeInputOwner owner{};
    std::uint64_t owner_generation{}, contact_generation{}, action_generation{}, physics_step{}, sampled_at{};
    std::uint32_t state{}, mode{}, contacts{}, stance_and_push{};
    std::int32_t flip_id{-1};
    std::uintptr_t selection_object{}; // Internal identity, not a native action occurrence.
    // Continuous verified flight identity, independent of selection changes.
    // Departure means leaving verified AIR, not proof of landing/completion.
    bool airborne{};
    std::uint64_t airborne_episode{}, air_departure{};
    // Original first observed physical AIR step, never animation time.
    std::uint64_t airborne_started_at{};
    bool airborne_started_known{};
    // Software continuity for observed mode changes, independent of wheel contacts.
    std::uint64_t mode_generation{};
    // Continuity of the ordinary airborne jump-to-idle wait. Every physics
    // observation can revoke it, including excursions missed by input polls.
    std::uint64_t flip_wait_generation{};
    // Copied from this original physics callback, never a cached body clock
    // or a timestamp-derived interval. Zero means no qualified clock supplied.
    float physics_seconds{};
    // Generations invalidate correlation; they are not action occurrences.
    // Rail family does not establish a named grind, slide, Lip, early/established
    // phase or geometry. Grab initialization/ForceInit remains unknown.
};
// Cleanup-only physical continuity. This wrapper cannot be passed as input
// admission or an ordinary motion intent; OFF never revives command authority.
struct MotionCleanupProof {
    NativeInputFrame observed{};
    bool inspected{}; // False means lock contention, not a new observation.
    MotionCleanupProof() = default;
    explicit MotionCleanupProof(NativeInputFrame frame) noexcept : observed(frame), inspected(true) {}
};
// Serialized by the existing input mutex. Only a validated native physics step
// produces frames; client ticks and input polls can only consume them.
class NativeInputState {
public:
    void invalidate() noexcept { frame_.valid = false; continuity_ = false; }
    NativeInputFrame observe(std::uint64_t now, NativeInputOwner owner,
            NativeRequestSample sample, NativeFlipSelection flip, bool wipeout = false,
            float seconds = 0) noexcept {
        if (!owner.valid() || !sample.valid || !sample.state) { invalidate(); return {}; }
        const bool boundary = !continuity_ || owner != frame_.owner || wipeout ||
            now < frame_.sampled_at || now-frame_.sampled_at > 150;
        const auto contacts = sample.words[1] & 0xc000u;
        const auto stance_and_push = sample.words[0] & 0x60u;
        const bool same_selection = flip.valid && frame_.flip_known && flip.object &&
            flip.object == frame_.selection_object &&
            (flip.id == frame_.flip_id || (frame_.flip_id == 1 && flip.id == -1));
        const bool waiting_selection = (frame_.flip_id == 1 || frame_.flip_id == -1) &&
            (flip.id == 1 || flip.id == -1);
        const bool forward_air = frame_.airborne && (sample.state == frame_.state ||
            (frame_.state == 200 && sample.state == 201));
        const bool continues_wait = !boundary && forward_air && same_selection && waiting_selection &&
            sample.crouch_mode == 2 && frame_.mode == 2 && stance_and_push == frame_.stance_and_push;
        const bool changed = boundary || sample.state != frame_.state || sample.crouch_mode != frame_.mode ||
            stance_and_push != frame_.stance_and_push || flip.valid != frame_.flip_known ||
            (flip.valid && (flip.id != frame_.flip_id || flip.object != frame_.selection_object));
        if (changed && !continues_wait && !increment(flip_wait_generation_)) return {};
        if ((boundary || sample.crouch_mode != frame_.mode) && !increment(mode_generation_)) return {};
        if (boundary) {
            if (!increment(owner_generation_) || !increment(contact_generation_) || !increment(action_generation_)) return {};
        } else {
            if ((sample.state != frame_.state || sample.crouch_mode != frame_.mode || contacts != frame_.contacts) &&
                !increment(contact_generation_)) return {};
            if ((sample.state != frame_.state || stance_and_push != frame_.stance_and_push || flip.valid != frame_.flip_known ||
                 (flip.valid && (flip.id != frame_.flip_id || flip.object != frame_.selection_object))) && !increment(action_generation_)) return {};
        }
        const bool airborne = sample.state == 200 || sample.state == 201;
        if (airborne && (boundary || !frame_.airborne)) {
            if(!increment(airborne_episode_))return {};
            airborne_started_at_=now;
        }
        // Invalid reads/owner changes already break owner continuity. They do
        // not manufacture a physical departure or landing observation.
        if (!boundary && frame_.airborne && !airborne && !increment(air_departure_)) return {};
        if (!increment(step_)) return {};
        namespace rail = game::build::v20260929::grind_exit;
        frame_ = {true, sample.state >= rail::grind_states_first && sample.state < rail::grind_states_end,
            flip.valid, owner, owner_generation_, contact_generation_, action_generation_, step_, now,
            sample.state, sample.crouch_mode, contacts, stance_and_push, flip.valid ? flip.id : -1, flip.valid ? flip.object : 0, airborne, airborne_episode_, air_departure_,airborne_started_at_,airborne,mode_generation_,flip_wait_generation_};
        if (std::isfinite(seconds) && seconds >= .001f && seconds <= .1f) frame_.physics_seconds = seconds;
        continuity_ = true;
        return frame_;
    }
    NativeInputFrame correlate(std::uint64_t now, NativeInputOwner owner, NativeRequestSample sample) const noexcept {
        if (!frame_.valid || !sample.valid || owner != frame_.owner || now < frame_.sampled_at ||
            now-frame_.sampled_at > 150 || sample.state != frame_.state || sample.crouch_mode != frame_.mode ||
            (sample.words[1] & 0xc000u) != frame_.contacts ||
            (sample.words[0] & 0x60u) != frame_.stance_and_push) return {};
        return frame_;
    }
    // Controller transport is bound to an original-physics owner episode. A
    // client read may already show the next native phase before that callback
    // observes it; this cannot grant gameplay authority for the newer phase.
    // Return the unchanged original frame only while its actual owner remains
    // current and fresh. Gameplay continues to use full correlate() above.
    NativeInputFrame binding_owner_proof(std::uint64_t now, NativeInputOwner owner,
            NativeRequestSample sample) const noexcept {
        if (!frame_.valid || !sample.valid || !sample.state || !owner.valid() || owner != frame_.owner ||
            now < frame_.sampled_at || now-frame_.sampled_at > 150) return {};
        return frame_;
    }
    // A client read can lead the original callback by one native phase. Keep
    // an already admitted session in that gap, without publishing this newer
    // phase or renewing any permission/clock. Hard withdrawal still wins.
    bool defer_client_phase(std::uint64_t now, NativeInputOwner owner,
            NativeRequestSample sample, const NativeInputFrame& admitted, bool permitted) const noexcept {
        const auto original = binding_owner_proof(now,owner,sample);
        const auto ordinary=[](unsigned state,unsigned mode) {
            return (state==101 && mode<=1) ||
                (mode<=2 && (state==100 || state==103 || state==200 || state==201));
        };
        return permitted && original.valid && admitted.valid && admitted.owner == original.owner &&
            ordinary(original.state,original.mode) && ordinary(sample.state,sample.crouch_mode) &&
            admitted.owner_generation == original.owner_generation && now >= admitted.sampled_at &&
            now-admitted.sampled_at <= 150 && !correlate(now,owner,sample).valid;
    }
private:
    bool increment(std::uint64_t& value) noexcept {
        if (exhausted_ || value == UINT64_MAX) { exhausted_ = true; invalidate(); return false; }
        ++value; return true;
    }
    NativeInputFrame frame_{};
    std::uint64_t owner_generation_{}, contact_generation_{}, action_generation_{}, step_{}, airborne_episode_{}, air_departure_{},airborne_started_at_{};
    std::uint64_t mode_generation_{}, flip_wait_generation_{};
    bool continuity_{}, exhausted_{};
};
}
