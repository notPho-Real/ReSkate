#pragma once
#include "arcade_native_probe.h"
#include "arcade_native_trick.h"
#include "Engine/Game/Build/20260929/grind_exit.h"
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
    // Generations invalidate correlation; they are not action occurrences.
    // Rail family does not establish a named grind, slide, Lip, early/established
    // phase or geometry. Grab initialization/ForceInit remains unknown.
};
// Serialized by the existing input mutex. Only a validated native physics step
// produces frames; client ticks and input polls can only consume them.
class NativeInputState {
public:
    void invalidate() noexcept { frame_.valid = false; continuity_ = false; }
    NativeInputFrame observe(std::uint64_t now, NativeInputOwner owner,
            NativeRequestSample sample, NativeFlipSelection flip, bool wipeout = false) noexcept {
        if (!owner.valid() || !sample.valid || !sample.state) { invalidate(); return {}; }
        const bool boundary = !continuity_ || owner != frame_.owner || wipeout ||
            now < frame_.sampled_at || now-frame_.sampled_at > 150;
        const auto contacts = sample.words[1] & 0xc000u;
        const auto stance_and_push = sample.words[0] & 0x60u;
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
            sample.state, sample.crouch_mode, contacts, stance_and_push, flip.valid ? flip.id : -1, flip.valid ? flip.object : 0, airborne, airborne_episode_, air_departure_,airborne_started_at_,airborne};
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
private:
    bool increment(std::uint64_t& value) noexcept {
        if (exhausted_ || value == UINT64_MAX) { exhausted_ = true; invalidate(); return false; }
        ++value; return true;
    }
    NativeInputFrame frame_{};
    std::uint64_t owner_generation_{}, contact_generation_{}, action_generation_{}, step_{}, airborne_episode_{}, air_departure_{},airborne_started_at_{};
    bool continuity_{}, exhausted_{};
};
}
