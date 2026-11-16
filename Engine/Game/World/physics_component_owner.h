#pragma once
#include "body_hit_identity.h"
#include "Engine/Game/Build/20260929/physics_owner.h"
#include "Engine/Game/Build/20260929/engine.h"
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

// The local board has its own entity and physics component. Follow the same
// reciprocal association used by native pose capture; proximity is not ownership.
template<class Read,class Current>
bool belongs_to_board(std::uintptr_t base,std::uintptr_t skater,std::uintptr_t component,
        body_hit_identity::NativeHandle owner,Read&& read,Current&& current) {
    namespace p=game::build::v20260929::physics_owner;
    namespace e=game::build::v20260929::engine;
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    if(!base || base>highest-e::board_entity_vtable || !current())return false;
    const auto at=[&]<class T>(std::uintptr_t object,std::uintptr_t offset,T& value) {
        return object>=0x10000 && offset<=highest-sizeof(T) &&
            object<=highest-offset-sizeof(T) && read(object+offset,value);
    };
    struct Snapshot {
        std::uintptr_t skater_collection{},skater_component{},board_component{},board_collection{},board{},context{};
        bool operator==(const Snapshot&) const = default;
    };
    const auto registered=[&](std::uintptr_t entity,std::uintptr_t collection,std::uintptr_t value) {
        std::uintptr_t backlink{},entry{},owner_entity{};std::uint8_t slot{};
        return at(collection,0,owner_entity) && owner_entity==entity &&
            at(value,p::component_collection,backlink) && backlink==collection &&
            at(value,p::component_slot,slot) &&
            at(collection,p::collection_entries+std::uintptr_t(slot)*p::collection_entry_size,entry) && entry==value;
    };
    const auto snapshot=[&](Snapshot& s) {
        std::uintptr_t table{},backlink{},collection{},context{};
        return current() && at(skater,0,table) && table==base+e::skater_entity_vtable &&
            at(skater,p::entity_context,s.context) && s.context &&
            at(skater,p::entity_collection,s.skater_collection) &&
            at(skater,p::skater_component,s.skater_component) &&
            at(s.skater_component,0,table) && table==base+e::skater_component_vtable &&
            registered(skater,s.skater_collection,s.skater_component) &&
            at(s.skater_component,p::skater_board_component,s.board_component) &&
            at(s.board_component,0,table) && table==base+e::board_component_vtable &&
            at(s.board_component,p::board_skater_component,backlink) && backlink==s.skater_component &&
            at(s.board_component,p::component_collection,s.board_collection) &&
            at(s.board_collection,0,s.board) && s.board!=skater &&
            at(s.board,0,table) && table==base+e::board_entity_vtable &&
            at(s.board,p::entity_context,context) && context==s.context &&
            at(s.board,p::entity_collection,collection) && collection==s.board_collection &&
            registered(s.board,s.board_collection,s.board_component) && current();
    };
    Snapshot before{},after{};
    return snapshot(before) && belongs_to(base,before.board,component,owner,read,current) &&
        snapshot(after) && before==after && current();
}
}
