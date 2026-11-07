#pragma once
#include "arcade_surface_geometry.h"
#include "Engine/Game/Build/20260929/physics_bodies.h"
#include "Engine/Game/Build/20260929/physics_owner.h"
#include <limits>

namespace dingosdk::arcade_mode::static_world_owner_detail {
// The caller proves the allocated shape-to-body association and static motion
// before/after every snapshot. Native non-ECS identity is never an invented ECS ID.
template<class Read,class Current>
bool observe(SurfaceGeometryBody shape,SurfaceGeometryBody body,std::uintptr_t store,
        Read&& read,Current&& current,SurfaceGeometryWorldOwner& out) {
    namespace p=game::build::v20260929::physics_bodies;
    namespace o=game::build::v20260929::physics_owner;
    out={};
    constexpr auto last=std::numeric_limits<std::uintptr_t>::max();
    if(!shape.world || shape.world>last-o::generations_end-sizeof(std::uintptr_t) ||
       !store || store>last-p::body_owner_offset-sizeof(std::uintptr_t) ||
       shape.index>=1048576 || shape.generation>255 || body.world!=shape.world || body.index>=1048576 || !current())return false;
    const auto element=[&](std::uintptr_t table,std::uint32_t index,auto& value) {
        const auto size=sizeof(value);
        return table && table<=last-(size-1) && index<=(last-(size-1)-table)/size &&
            read(table+static_cast<std::uintptr_t>(index)*size,value);
    };
    struct Snapshot {
        std::uintptr_t owners{},kinds{},values{},begin{},end{};
        std::uint32_t index{},generation{};
        std::uint8_t kind_byte{};
        std::uint64_t value{};
        bool operator==(const Snapshot&) const = default;
    };
    const auto snapshot=[&](Snapshot& s) {
        if(!read(store+p::body_owner_offset,s.owners) || !read(store+p::owner_kind_offset,s.kinds) ||
           !read(store+p::owner_value_offset,s.values) || !read(shape.world+o::generations_begin,s.begin) ||
           !read(shape.world+o::generations_end,s.end) || !s.begin || s.end<s.begin ||
           (s.end-s.begin)%4 || (s.end-s.begin)/4>1048576 ||
           !element(s.owners,shape.index,s.index) || s.index==UINT32_MAX || s.index>=(s.end-s.begin)/4 ||
           !element(s.begin,s.index,s.generation) || !element(s.kinds,s.index>>3,s.kind_byte) ||
           !(s.kind_byte&(1u<<(s.index&7u))) || !element(s.values,s.index,s.value) || !s.value)return false;
        return current();
    };
    Snapshot before{},after{};
    if(!snapshot(before) || !snapshot(after) || before!=after || !current())return false;
    out={body,store,before.index,before.generation,0,before.value};
    return true;
}
}
