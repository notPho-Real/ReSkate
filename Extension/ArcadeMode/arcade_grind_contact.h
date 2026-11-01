#pragma once
#include "Engine/Game/Build/20260929/arcade_grind_transition.h"
#include <cmath>

namespace dingosdk::arcade_mode {
// Original native contact/segment evidence, independent of a requested name.
// This is an admission sample, not an atomic physics frame or acknowledgment.
struct NativeGrindContact {
    bool valid{};
    std::uintptr_t actor{},output{};
    std::uint32_t state{},identity{},geometry{};
    std::array<float,3> begin{},end{};
    std::array<std::uint64_t,2> segment_identity{};
    bool operator==(const NativeGrindContact&) const = default;
};

// Valid freshly resolves the complete local owner and its gameplay lease.
// Read copies guarded memory from the supported executable. No native field
// is written, and 406's temporary lift is deliberately not an initial contact.
template<class Read,class Valid>
NativeGrindContact read_grind_contact(std::uintptr_t base,std::uintptr_t core,
    std::uintptr_t context,Read&& read,Valid&& valid) noexcept {
    namespace c=game::build::v20260929::arcade_grind_transition;
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    const auto safe=[](std::uintptr_t p) { return p>0x10000 && p<highest-0x8000 && !(p&7); };
    if(!safe(base) || base>highest-c::transition_vtable || !safe(core) || !safe(context) || !valid())return {};
    NativeGrindContact result{};
    std::uintptr_t type{},owner{},selector{},pages{};
    if(!read(core,type) || type!=base+c::core_vtable ||
        !read(core+c::core_context,owner) || owner!=context ||
        !read(context+c::selected_state,result.state) || result.state<400 || result.state>405 ||
        !read(core+c::active_actor,result.actor) || !safe(result.actor) ||
        !read(result.actor,type) || type!=base+c::contact_vtables[result.state-400] ||
        !read(result.actor+c::actor_context,owner) || owner!=context ||
        !read(core+0x440,selector) || !safe(selector) || !read(selector,pages) || !safe(pages) ||
        !read(pages+32,result.output) || !safe(result.output))return {};
    const auto contact=[&] {
        std::uint8_t physical{},observed{},missing{};
        return read(context+4016,physical) && physical==1 &&
            read(result.output+400,observed) && observed==1 &&
            read(result.actor+c::missing_segment,missing) && missing==0;
    };
    if(!contact() || !read(result.output+188,result.geometry) ||
        !read(result.output+192,result.identity) || !result.identity || result.identity>68 ||
        !read(result.actor+c::segment_begin,result.begin) || !read(result.actor+c::segment_end,result.end) ||
        !read(result.actor+c::segment_identity,result.segment_identity))return {};
    double length_squared{};
    for(std::size_t i=0;i<3;++i) {
        if(!std::isfinite(result.begin[i]) || !std::isfinite(result.end[i]))return {};
        const double delta=static_cast<double>(result.end[i])-result.begin[i];
        length_squared+=delta*delta;
    }
    if(!(length_squared>0) || !std::isfinite(length_squared))return {};
    // A replacement actor, segment, selector, identity or owner invalidates
    // the whole sample. Native missing-segment output remains authoritative;
    // do not substitute fixture coordinates or invent geometry tolerances.
    std::uint32_t state{},identity{},geometry{};
    std::array<float,3> begin{},end{};
    std::array<std::uint64_t,2> segment_identity{};
    if(!read(context+c::selected_state,state) || state!=result.state ||
        !read(core+c::active_actor,owner) || owner!=result.actor ||
        !read(result.actor,type) || type!=base+c::contact_vtables[result.state-400] ||
        !read(result.actor+c::actor_context,owner) || owner!=context ||
        !read(core+c::core_context,owner) || owner!=context ||
        !read(core,type) || type!=base+c::core_vtable ||
        !read(core+0x440,owner) || owner!=selector || !read(selector,owner) || owner!=pages ||
        !read(pages+32,owner) || owner!=result.output ||
        !read(result.output+188,geometry) || geometry!=result.geometry ||
        !read(result.output+192,identity) || identity!=result.identity || !contact() ||
        !read(result.actor+c::segment_begin,begin) || begin!=result.begin ||
        !read(result.actor+c::segment_end,end) || end!=result.end ||
        !read(result.actor+c::segment_identity,segment_identity) || segment_identity!=result.segment_identity ||
        !valid())return {};
    result.valid=true;
    return result;
}
}
