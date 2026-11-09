#pragma once
#include "body_hit_identity.h"
#include "Engine/Game/Build/20260929/physics_owner.h"
#include <limits>

namespace dingosdk::physics_component_owner {
// Read-only qualification of a previously resolved non-ECS owner. The caller
// fingerprints the supported layout and revalidates the shape/body/owner pool.
// A matching category or nearby body is never evidence of entity ownership.
template<class Read,class Current>
bool belongs_to(std::uintptr_t base,std::uintptr_t entity,std::uintptr_t component,
        body_hit_identity::NativeHandle owner,Read&& read,Current&& current) {
    namespace p=game::build::v20260929::physics_owner;
    constexpr auto last=std::numeric_limits<std::uintptr_t>::max();
    if(!base || base>last-p::component_vtable || !entity ||
       entity>last-p::entity_collection-sizeof(std::uintptr_t) || !component ||
       component>last-p::component_owner_handle-sizeof(owner) || !owner.world ||
       owner.index==UINT32_MAX || !current())return false;
    struct Snapshot {
        std::uintptr_t table{},collection{},entity_collection{},registered{},world{};
        body_hit_identity::NativeHandle owner{};
        std::uint8_t slot{};
        bool operator==(const Snapshot&) const = default;
    };
    const auto snapshot=[&](Snapshot& s) {
        if(!read(component,s.table) || s.table!=base+p::component_vtable ||
           !read(component+p::component_collection,s.collection) || !s.collection ||
           !read(entity+p::entity_collection,s.entity_collection) || s.collection!=s.entity_collection ||
           !read(component+p::component_slot,s.slot))return false;
        const auto offset=p::collection_entries+std::uintptr_t(s.slot)*p::collection_entry_size;
        return s.collection<=last-offset-sizeof(std::uintptr_t) &&
            read(s.collection+offset,s.registered) && s.registered==component &&
            read(component+p::component_world,s.world) && s.world==owner.world &&
            read(component+p::component_owner_handle,s.owner) && s.owner==owner && current();
    };
    Snapshot before{},after{};
    return snapshot(before) && snapshot(after) && before==after && current();
}
}
