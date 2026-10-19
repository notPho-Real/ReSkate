#pragma once
#include "arcade_trick_table.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Candidate input recipes are separate from native recognition. No recipe awards
// points or asserts a trick identity. Timings below await controlled Deck trials.
enum class ActionReadiness : std::uint8_t {
    candidate, method_missing, geometry_required, context_unverified, invalid
};
struct NativeActionPad {
    std::int16_t right_x{}, right_y{};
    std::uint8_t left_trigger{}, right_trigger{};
    std::uint16_t buttons{};
};
struct NativeActionStep { unsigned duration_ms{}; NativeActionPad pad{}; };
struct NativeActionPlan {
    ArcadeTrick requested{ArcadeTrick::none};
    ActionReadiness readiness{ActionReadiness::invalid};
    std::array<NativeActionStep, 4> steps{};
    unsigned count{};
};

// Source: installed game's grab guide, research/native-grab-guide.json.
// Only the inspected stance is described. Mirroring remains a separate experiment.
constexpr NativeActionPlan grab_plan(ArcadeTrick trick, bool inspected_stance) noexcept {
    NativeActionPlan plan{trick, ActionReadiness::method_missing};
    if (!inspected_stance) { plan.readiness = ActionReadiness::context_unverified; return plan; }
    NativeActionPad first{}, second{}, third{};
    bool extra{};
    switch (trick) {
    case ArcadeTrick::basic_bs_grab:
        // Native OFF control1792210929031828000: RT alone establishes BS Grab.
        // Keep this distinct from named modifiers that returned a different label.
        plan.readiness = ActionReadiness::candidate;
        plan.steps[0] = {500, {0, 0, 0, 255, 0}};
        plan.steps[1] = {34, {}};
        plan.count = 2;
        return plan;
    case ArcadeTrick::melon: first.right_trigger = 255; second = first; second.right_y = 32767; break;
    case ArcadeTrick::method: first.right_trigger = 255; second = first; second.right_x = 32767; break;
    case ArcadeTrick::stiffy: first.left_trigger = 255; second = first; second.right_x = -32767; break;
    case ArcadeTrick::nosegrab:
    case ArcadeTrick::airwalk:
        first.right_y = -32767; second = first; second.right_trigger = 255;
        if (trick == ArcadeTrick::airwalk) { third = second; third.buttons = 0x2000; extra = true; }
        break;
    case ArcadeTrick::tailgrab:
    case ArcadeTrick::benihana:
    case ArcadeTrick::one_foot_tailgrab:
        first.right_y = 32767; second = first; second.left_trigger = 255;
        if (trick != ArcadeTrick::tailgrab) {
            third = second; third.buttons = trick == ArcadeTrick::benihana ? 0x4000 : 0x1000; extra = true;
        }
        break;
    case ArcadeTrick::rocket_air:
        first.right_y = -32767; second = first; second.left_trigger = second.right_trigger = 255; break;
    case ArcadeTrick::japan:
        first.right_x = 32767; second = first; second.right_trigger = 255;
        third = second; third.right_x = -32767; extra = true; break;
    case ArcadeTrick::judo:
    case ArcadeTrick::christ_air:
        first.right_trigger = 255; second = first;
        second.buttons = trick == ArcadeTrick::judo ? 0x4000 : 0x2000; break;
    default: return plan;
    }
    plan.readiness = ActionReadiness::candidate;
    plan.steps[0] = {50, first};
    plan.steps[1] = {extra ? 50u : 300u, second};
    plan.count = 2;
    if (extra) plan.steps[plan.count++] = {300, third};
    plan.steps[plan.count++] = {34, {}}; // Explicit release, including synthetic face buttons.
    return plan;
}

// Rail-relative selection already exists in select_grind(). Do not invent a
// native actuator or label until approach geometry and a method are established.
constexpr ActionReadiness grind_readiness(ArcadeTrick trick, RailRelationship rail) noexcept {
    if (trick < ArcadeTrick::grind_50_50 || trick > ArcadeTrick::darkslide) return ActionReadiness::invalid;
    return rail == RailRelationship::unknown ? ActionReadiness::geometry_required : ActionReadiness::method_missing;
}

class NativeActionPlayback {
public:
    struct Sample { bool owns_pad{}, finished{}, cancelled{}; unsigned phase{}; NativeActionPad pad{}; };
    bool start(const NativeActionPlan& plan, std::uint64_t now, bool experimental_enabled,
               bool observed_airborne, bool held = false) noexcept {
        if (active_ || !experimental_enabled || !observed_airborne ||
            plan.readiness != ActionReadiness::candidate || !plan.count || plan.count > plan.steps.size()) return false;
        unsigned total{};
        for (unsigned i = 0; i < plan.count; ++i) {
            if (!plan.steps[i].duration_ms || plan.steps[i].duration_ms > 1000) return false;
            total += plan.steps[i].duration_ms;
        }
        if (total > 1500) return false;
        // Only the established trigger-only native grab has a proven hold pad.
        // Named modifier candidates retain their existing bounded recipes.
        if (held && (plan.requested != ArcadeTrick::basic_bs_grab || plan.count != 2)) return false;
        plan_ = plan; began_ = last_ = now; active_ = true; held_ = held; released_ = emitted_ = false;
        return true;
    }
    void cancel() noexcept { active_ = held_ = released_ = emitted_ = false; }
    // Acknowledges only the existing pad recipe, never native minimum-init.
    // A tap released before its first pad cannot skip straight to an out pad.
    bool release(std::uint64_t now) noexcept {
        if (!active_ || !held_) return false;
        if (now < last_ || !emitted_) { cancel(); return false; }
        if (released_) return true;
        released_ = true; released_at_ = last_ = now;
        return true;
    }
    bool busy() const noexcept { return active_; }
    Sample sample(std::uint64_t now, bool gameplay_fresh, bool observed_airborne, bool conflict = false) noexcept {
        if (!active_) return {};
        if (!gameplay_fresh || !observed_airborne || conflict || now < last_) {
            cancel(); return {false, false, true};
        }
        last_ = now;
        if (held_) {
            if (!released_) { emitted_ = true; return {true, false, false, 0, plan_.steps[0].pad}; }
            if (now - released_at_ < plan_.steps[1].duration_ms)
                return {true, false, false, 1, plan_.steps[1].pad};
            cancel(); return {false, true, false};
        }
        auto elapsed = now - began_;
        for (unsigned i = 0; i < plan_.count; ++i) {
            if (elapsed < plan_.steps[i].duration_ms) return {true, false, false, i, plan_.steps[i].pad};
            elapsed -= plan_.steps[i].duration_ms;
        }
        cancel(); return {false, true, false};
    }
private:
    NativeActionPlan plan_{};
    std::uint64_t began_{}, last_{}, released_at_{};
    bool active_{}, held_{}, released_{}, emitted_{};
};
}
