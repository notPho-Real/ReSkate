#pragma once
#include "Engine/Game/Build/20260929/arcade_grind_transition.h"
#include <cmath>

namespace dingosdk::arcade_mode {
// An internal callback scope, never an input request or a grind acknowledgment.
// The caller must revalidate its live request and complete local owner before
// and after forwarding the original native function.
struct GrindOrientationScope {
    std::uintptr_t base{}, core{}, context{}, actor{}, caller{};
};
template<class Read> bool grind_orientation_scope(Read& read,const GrindOrientationScope& s,
    std::uintptr_t expected_return=game::build::v20260929::arcade_grind_transition::orientation_return) noexcept {
    namespace c=game::build::v20260929::arcade_grind_transition;
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    const auto safe=[](std::uintptr_t p) { return p>0x10000 && p<highest-0x8000 && !(p&7); };
    if (!safe(s.base) || !safe(s.core) || !safe(s.context) || !safe(s.actor) ||
        s.base>highest-c::transition_vtable || s.caller!=s.base+expected_return) return false;
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

struct NativeGrindCompletion {
    bool valid{};
    std::uint32_t source{},target{};
    float phase{};
    std::array<float,3> orientation{};
};
// Observe the original transition update's active 1->0 edge, independently of
// the orientation repair. The pinned update contains the native completion
// writer; cleanup and a shared admission clear cannot supply this evidence.
// This proves actor completion only, never a named grind or score occurrence.
template<class Read,class Valid,class Invoke>
NativeGrindCompletion observe_grind_transition_completion(const GrindOrientationScope& scope,
    Read&& read,Valid&& valid,Invoke&& original) {
    namespace c=game::build::v20260929::arcade_grind_transition;
    std::uint8_t before{},after{};
    const bool eligible=valid() && grind_orientation_scope(read,scope,c::update_return) &&
        read(scope.actor+c::actor_active,before) && before==1;
    original();
    NativeGrindCompletion result{};
    if (!eligible || !valid() || !grind_orientation_scope(read,scope,c::update_return) ||
        !read(scope.actor+c::actor_active,after) || after!=0 ||
        !read(scope.actor+1040,result.source) || !read(scope.actor+1044,result.target) ||
        !read(scope.actor+1424,result.phase) || !read(scope.actor+72,result.orientation) ||
        result.source<400 || result.source>405 || result.target<400 || result.target>405 ||
        !std::isfinite(result.phase) || result.phase<.99f || result.phase>1.f) return {};
    for(const auto angle:result.orientation)if(!std::isfinite(angle))return {};
    result.valid=valid() && grind_orientation_scope(read,scope,c::update_return);
    return result.valid?result:NativeGrindCompletion{};
}
}
