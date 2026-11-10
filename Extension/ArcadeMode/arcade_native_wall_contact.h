#pragma once
#include "Extension/Skater/no_bail.h"
#include "Engine/Game/World/body_hit_identity.h"
#include "Engine/Game/World/contact_material_read.h"
#include <array>

namespace dingosdk::arcade_mode::native_wall_contact {
using Vec=std::array<float,3>;
enum class Source : std::uint8_t { rig, board };
struct Contact {
    body_hit_identity::NativeHandle shape{},body{};
    Vec point{},normal{};
    std::uintptr_t other_entity{},world_store{};
    std::uint32_t bone{},kind{},categories{},packed_material{};
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
struct SupportMaterial {
    NoBailSkater owner{};
    contact_material_read::Material material{};
    std::uint64_t serial{},sampled_at{};
    float seconds{};
    bool valid{};
};
// Same physics thread only, after the original board processor. Names the
// actual static flat support; no ray, cached fallback, or native material write.
SupportMaterial board_material(const NoBailSkater& owner,std::uint64_t now) noexcept;
bool start(std::uintptr_t base) noexcept;
void enable(bool enabled) noexcept;
Snapshot observe(const NoBailSkater& owner,std::uint64_t now) noexcept;
// Caller must independently validate its current local-world incarnation and
// the same observed physics step. This only rechecks the native target handle.
bool current(const Contact& contact,std::uintptr_t expected_world) noexcept;
}
