#pragma once
#include "arcade_input_policy.h"

namespace dingosdk::arcade_mode {
// Admission for the installed game's exact _fromgrind stick recipe, not a
// velocity writer or a named-grind actor. 401 is the measured held grind;
// 403 is the independently captured fixed-rail contact/exit phase. Other
// members of the 400 family are exclusions, not verified launch admission.
inline bool rail_pop_input_ready(const InputNativeContext& native, std::uint64_t now) noexcept {
    return native.required && native.fresh(now) && native.rail_family && !native.airborne &&
        (native.flip.state==401 || native.flip.state==403) && native.flip.mode<=2;
}
inline bool rail_pop_input_continues(const InputNativeContext& native, std::uint64_t now) noexcept {
    // Native 402/404 are measured brief grind phases. They can retain an
    // already admitted hold without granting a new setup in an unknown phase.
    return native.required && native.fresh(now) && native.rail_family && !native.airborne &&
        native.flip.state>=401 && native.flip.state<=404 && native.flip.mode<=2;
}
// Continuity of a released rail Ollie, observed at every native physics step.
// This is a request lease only: native StateFlow/PTM still owns selection,
// ascent admission, actor initialization and cleanup. No ground launch ticket.
inline bool rail_pop_followup_continues(const InputNativeContext& current,
        const InputNativeContext& before, std::uint64_t now) noexcept {
    const auto phase=[](const InputNativeContext& n) {
        return n.rail_family && !n.airborne && n.flip.state>=401 && n.flip.state<=404 ? 0 :
            !n.rail_family && !n.airborne && n.flip.state==701 ? 1 :
            !n.rail_family && n.airborne && n.flip.state==200 ? 2 :
            !n.rail_family && n.airborne && n.flip.state==201 ? 3 : -1;
    };
    if (!current.required || !before.required || !current.fresh(now) || !before.fresh(now) ||
        !current.follows(before) || current.generations.owner!=before.generations.owner ||
        !current.flip.valid || !before.flip.valid || !current.flip.object ||
        current.flip.object!=before.flip.object || current.flip.stance!=before.flip.stance ||
        current.flip.mode>2 || before.flip.mode>2 || !current.flip.mode_generation ||
        !before.flip.mode_generation || phase(before)<0 || phase(current)<phase(before) ||
        (current.flip.id!=1 && current.flip.id!=-1) || (before.flip.id!=1 && before.flip.id!=-1) ||
        (before.flip.id==1 && current.flip.id==-1 && !current.airborne) ||
        current.physics_step-before.physics_step>1) return false;
    const unsigned action_change=current.flip.state!=before.flip.state || current.flip.id!=before.flip.id;
    return current.generations.action-before.generations.action==action_change &&
        current.flip.mode_generation-before.flip.mode_generation==unsigned(current.flip.mode!=before.flip.mode) &&
        current.generations.contact-before.generations.contact<=1 &&
        current.air_departure==before.air_departure &&
        current.airborne_episode==before.airborne_episode+(current.airborne&&!before.airborne?1u:0u);
}
}
