#pragma once
#include "arcade_input_policy.h"

namespace dingosdk::arcade_mode {
// The pinned native selector routes 400..406 through the same rail-exit
// admission at RVA 0x47466f3. These are physical grind classes, not successive
// takeoff phases: Boardslide, 5050, Tipslide, 5_O, Backslash, Darkslide and
// Transition (vtable state getters 0x47c8830..0x47c8890). Unknown 400-family
// values are not sufficient evidence. This admits only the _fromgrind input
// recipe; native contact/geometry checks still decide whether it can pop.
inline bool rail_pop_physics_state(std::uint32_t state) noexcept {
    return state>=400 && state<=406;
}
inline bool rail_pop_input_ready(const InputNativeContext& native, std::uint64_t now) noexcept {
    return native.required && native.fresh(now) && native.rail_family && !native.airborne &&
        rail_pop_physics_state(native.flip.state) && native.flip.mode<=2;
}
inline bool rail_pop_input_continues(const InputNativeContext& native, std::uint64_t now) noexcept {
    return rail_pop_input_ready(native,now);
}
// Rail exits may select ordinary Ollie or the native Quick Ollie helper.
inline bool rail_pop_jump(std::int32_t id) noexcept { return id==1 || id==31; }
// Continuity of a released rail Ollie, observed at every native physics step.
// This is a request lease only: native StateFlow/PTM still owns selection,
// ascent admission, actor initialization and cleanup. No ground launch ticket.
inline bool rail_pop_followup_continues(const InputNativeContext& current,
        const InputNativeContext& before, std::uint64_t now) noexcept {
    const auto phase=[](const InputNativeContext& n) {
        return n.rail_family && !n.airborne && rail_pop_physics_state(n.flip.state) ? 0 :
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
        // A previous approach trick can remain selected during native rail
        // setup. Retain only that unchanged selection until the new Ollie
        // takes over; it never grants airborne flip admission by itself.
        (!rail_pop_jump(current.flip.id) && current.flip.id!=-1 &&
            (current.airborne || current.flip.id!=before.flip.id)) ||
        (rail_pop_jump(before.flip.id) && current.flip.id==-1 && !current.airborne) ||
        current.physics_step-before.physics_step>1) return false;
    const unsigned action_change=current.flip.state!=before.flip.state || current.flip.id!=before.flip.id;
    return current.generations.action-before.generations.action==action_change &&
        current.flip.mode_generation-before.flip.mode_generation==unsigned(current.flip.mode!=before.flip.mode) &&
        current.generations.contact-before.generations.contact<=1 &&
        current.air_departure==before.air_departure &&
        current.airborne_episode==before.airborne_episode+(current.airborne&&!before.airborne?1u:0u);
}
// Native Quick Ollie can leave the rail before the physical A release. Keep
// only the observation of that admitted setup after its stick owner cancels.
// A nearby release and actual Quick Ollie selection are both required; this
// proof cannot issue a pop, retain a held stick or qualify ordinary run-off.
struct RailPopReleaseProof {
    InputNativeContext native{};
    std::uint64_t pressed_at{}, released_at{}, departure_at{};
    void reset() noexcept { *this={}; }
    void begin(const InputNativeContext& frame,std::uint64_t pressed) noexcept {
        reset();
        if (pressed && rail_pop_input_ready(frame,frame.sampled_at)) {native=frame;pressed_at=pressed;}
    }
    void observe(const InputNativeContext& frame) noexcept {
        if (!pressed_at) return;
        if (!rail_pop_followup_continues(frame,native,frame.sampled_at)) {reset();return;}
        if (!frame.rail_family && !departure_at) departure_at=frame.sampled_at;
        native=frame;
        if (departure_at && frame.sampled_at-departure_at>150) reset();
    }
    void release(std::uint64_t now,bool a_released) noexcept {
        if (!pressed_at) return;
        if (!native.fresh(now) || now<pressed_at ||
            (departure_at && (now<departure_at || now-departure_at>150))) {reset();return;}
        if (a_released && !released_at) released_at=now;
    }
    bool confirmed(std::uint64_t now) const noexcept {
        return pressed_at && released_at && departure_at && native.fresh(now) &&
            now>=released_at && now>=departure_at && now-departure_at<=150 &&
            native.flip.id==31 && (native.flip.state==701 || native.airborne);
    }
};

}
