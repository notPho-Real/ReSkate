#pragma once
#include "Engine/Game/Build/20260929/arcade_grind_transition.h"

namespace dingosdk::arcade_mode {
// An internal callback scope, never an input request or a grind acknowledgment.
// The caller must revalidate its live request and complete local owner before
// and after forwarding the original native function.
struct GrindOrientationScope {
    std::uintptr_t base{}, core{}, context{}, actor{}, caller{};
};
template<class Read> bool grind_orientation_scope(Read& read,const GrindOrientationScope& s) noexcept {
    namespace c=game::build::v20260929::arcade_grind_transition;
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    const auto safe=[](std::uintptr_t p) { return p>0x10000 && p<highest-0x8000 && !(p&7); };
    if (!safe(s.base) || !safe(s.core) || !safe(s.context) || !safe(s.actor) ||
        s.base>highest-c::transition_vtable || s.caller!=s.base+c::orientation_return) return false;
    std::uintptr_t value{};std::uint32_t state{};
    return read(s.core,value) && value==s.base+c::core_vtable &&
        read(s.core+c::core_context,value) && value==s.context &&
        read(s.core+c::core_actor,value) && value==s.actor &&
        read(s.core+c::active_actor,value) && value==s.actor &&
        read(s.actor,value) && value==s.base+c::transition_vtable &&
        read(s.actor+c::actor_context,value) && value==s.context &&
        read(s.context+c::selected_state,state) && state==c::transition_state;
}

// Shared native orientation calculation overwrites actor+766 with its request
// admission result, including false for the in-flight 406 actor. Preserve only
// that actor's existing native lifetime value around this helper. Its separate
// update/complete/cleanup functions execute afterwards and remain authoritative.
// In particular, preserving zero never revives a completed transition, and a
// transient missing contact bit during native lift does not end actor ownership.
// Native position, orientation, targets, identities and result values are never
// substituted here. This helper alone does not execute a named grind.
template<class Read,class Write,class Valid,class Invoke>
std::uintptr_t preserve_grind_transition_lifetime(const GrindOrientationScope& scope,
    Read&& read,Write&& write,Valid&& valid,Invoke&& original) {
    namespace c=game::build::v20260929::arcade_grind_transition;
    std::uint8_t before{},after{};
    const bool preserve=valid() && grind_orientation_scope(read,scope) &&
        read(scope.actor+c::actor_active,before) && before<=1;
    const auto result=original();
    if (preserve && valid() && grind_orientation_scope(read,scope) &&
        read(scope.actor+c::actor_active,after) && after<=1 && after!=before)
        write(scope.actor+c::actor_active,before);
    return result;
}
}
