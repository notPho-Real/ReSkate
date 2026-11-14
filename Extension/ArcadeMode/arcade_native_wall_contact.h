#pragma once
#include "Extension/Skater/no_bail.h"
#include "Engine/Game/World/body_hit_identity.h"
#include "arcade_surface_geometry.h"
#include <array>

namespace dingosdk::arcade_mode::native_wall_contact {
using Vec=std::array<float,3>;
enum class Source : std::uint8_t { rig, board };
struct Contact {
    body_hit_identity::NativeHandle shape{},body{};
    Vec point{},normal{};
    std::uintptr_t other_entity{},world_store{};
    std::uint32_t bone{},kind{},categories{};
    bool identity_valid{},categories_known{},static_body{};
    // Static body is a native mobility fact. It does not exclude rails, prove
    // a wall-sized surface, or authorize a rebound by itself.
};
struct Snapshot {
    NoBailSkater owner{};
    std::uint64_t serial{},sampled_at{},source_serial{};
    std::uint32_t state{};
    std::uint32_t physics_thread{};
    Source source{Source::rig};
    std::uint32_t state_after{};
    Vec velocity_after{};
    bool after_valid{};
    float seconds{};
    Vec velocity{};
    std::array<Contact,64> contacts{};
    std::size_t count{};
    bool valid{};
    // Rig summaries are reset and rebuilt by the original contact processor.
    // Link its completed publication to the board publication it followed.
    std::uint64_t board_serial{};
    std::uintptr_t summaries{};
    std::array<float,8> impacts{};
    bool processed{};
};
// A plant consumes only its own complete collision response. An articulated
// skater can touch that wall with several body parts in the admitted step.
// Unknown contacts and other surfaces retain the native bail decision.
inline bool same_wall_contacts(const Snapshot& board,const Snapshot& rig,
        const SurfaceGeometryBody& shape,const Vec& normal) noexcept {
    if(!board.valid || !board.after_valid || !board.serial || !rig.valid || !rig.processed ||
       rig.board_serial!=board.serial || board.source!=Source::board || rig.source!=Source::rig ||
       board.owner!=rig.owner || !board.physics_thread || board.physics_thread!=rig.physics_thread ||
       (board.state!=200 && board.state!=201) || board.state_after!=board.state || rig.state!=board.state ||
       board.seconds!=rig.seconds || rig.sampled_at<board.sampled_at ||
       !board.count || board.count>board.contacts.size() || !rig.count || rig.count>rig.contacts.size() ||
       !rig.summaries)return false;
    const auto& wall=board.contacts[0];
    if(wall.shape.world!=shape.world || wall.shape.index!=shape.index || wall.shape.generation!=shape.generation)
        return false;
    for(const auto* sample:{&board,&rig})for(std::size_t i=0;i<sample->count;++i) {
        const auto& hit=sample->contacts[i];
        if(!hit.identity_valid || !hit.static_body || !hit.categories_known ||
           hit.shape!=wall.shape || hit.body!=wall.body || hit.world_store!=wall.world_store ||
           hit.categories!=wall.categories || hit.kind!=wall.kind || hit.normal!=normal)return false;
        // Bone 0 is the board root, not a rig contact. Every actual rig body
        // belongs to the same transaction when its full wall identity agrees.
        if(sample==&rig && (hit.bone==0 || hit.bone>=24))return false;
    }
    for(float impact:rig.impacts)if(!std::isfinite(impact) || impact<0)return false;
    return true;
}
bool start(std::uintptr_t base) noexcept;
void enable(bool enabled) noexcept;
Snapshot observe(const NoBailSkater& owner,std::uint64_t now) noexcept;
// True only inside the exact local collision predicate invocation, after the
// native predicate ran. Neither a timer nor a cached query can create this scope.
bool collision_current(std::uint64_t serial,const NoBailSkater&) noexcept;
// Caller must independently validate its current local-world incarnation and
// the same observed physics step. This only rechecks the native target handle.
bool current(const Contact& contact,std::uintptr_t expected_world) noexcept;
// A valid static native owner can have no ECS entity. Never admits an unknown
// owner or moving body; returns the complete source-backed owner incarnation.
bool static_world_owner(const SurfaceGeometryBody&,SurfaceGeometryWorldOwner&) noexcept;
bool local_component_owner(const SurfaceGeometryBody&,std::uintptr_t entity,SurfaceGeometryWorldOwner&) noexcept;
}
