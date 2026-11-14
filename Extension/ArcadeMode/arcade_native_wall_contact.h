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
};
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
