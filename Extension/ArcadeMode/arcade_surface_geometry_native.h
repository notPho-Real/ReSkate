#pragma once
#include "arcade_surface_geometry.h"
#include <span>
#include "Extension/Objects/ParkEditor/park_editor_surface.h"

namespace dingosdk::arcade_mode {
// The existing runtime owner supplies fingerprint-validated APIs and the live
// callback guard. No TLS resolution, hook installation or competing callback.
struct SurfaceGeometryNativeApi {
    std::uintptr_t base{}; // base whose existing API fingerprints were validated
    editor::NativeSurfaceApi surface;
    // Borrow these three functions from the existing validated NativePickingApi.
    // Entity mapping and Build Kit mutation/query functions are not needed here.
    struct Picking {
        void* (*hit_body)(const void*, void*){};
        bool (*body_valid)(const void*){};
        void* (*body_entity)(void*, const void*){};
        bool ready{};
    } picking;
    void* user{};
    std::uint64_t (*now)(void*) noexcept{}; // same millisecond domain as native.sampled_at
    // Must verify thread/phase, lock, full owner/world/self incarnations and
    // current native generations against this immutable callback context.
    bool (*current)(void*, const SurfaceGeometryContext&) noexcept{};
    // Existing bounded memory::read_bytes, limited here to query-owned result
    // storage and its published allocator. Tests supply fixture storage only.
    bool (*read_bytes)(std::uintptr_t, void*, std::size_t) noexcept{};
    // Opt-in only for wallplant geometry. Must validate an allocated shape,
    // its current static body, and a complete non-ECS native owner stamp.
    bool (*static_world_owner)(void*, const SurfaceGeometryBody&, SurfaceGeometryWorldOwner&) noexcept{};
    // Copied diagnostic from the existing current() callback, never a retry.
    const char* (*current_failure)(void*) noexcept{};
    // Clearance-only opt-in. Resolve a non-ECS shape owner to a registered
    // component of context.owner[1] or its authenticated board entity, retaining
    // the complete owner incarnation and reciprocal local association.
    bool (*local_component_owner)(void*,const SurfaceGeometryContext&,const SurfaceGeometryBody&,
        SurfaceGeometryWorldOwner&) noexcept{};
};
// Optional copied receipts from calls already made by a geometry query. Never
// issues another native query or changes its admission/cleanup behavior.
struct SurfaceGeometryTrace {
    const char* reason{"not_started"};
    const char* context_reason{"not_observed"};
    unsigned ray{}, attempt{}, hits{};
    SurfaceGeometryBody shape{};
    std::uint64_t entity{};
    SurfaceGeometryVector start{}, end{}, point{};
    float value{};
};
SurfaceGeometrySample query_surface_geometry(const SurfaceGeometryNativeApi&, const SurfaceGeometryContext&,
                                             const SurfaceGeometryRequest&, SurfaceGeometryTrace* = nullptr) noexcept;
// Corroborates the broad vertical face around an independently recorded real
// collision. This is geometry only; it must never create a wall-contact event.
// Motion must come from that retained collision's validated original step.
// It bounds the first ray's inward search, never predicts or moves a contact.
SurfaceGeometrySample query_wall_geometry(const SurfaceGeometryNativeApi&, const SurfaceGeometryContext&,
    SurfaceGeometryBody contacted_body, SurfaceGeometryVector point, SurfaceGeometryVector normal,
    SurfaceGeometryVector approach_velocity, float native_seconds,
    SurfaceGeometryTrace* = nullptr) noexcept;
// Finite native support-ray clearance, not a capsule/volume sweep. The caller
// supplies verified native support positions relative to root (at most 32),
// from the same guarded owner observation. The live caller currently proves
// deck part0 only, not articulated shape extents. Native collision remains enabled.
// No missing-support fallback and no transform/position write occurs here.
struct SurfaceGeometryAnchor {
    SurfaceGeometryVector target{};
    float displacement{};
    unsigned checked_rays{};
    bool valid{};
};
SurfaceGeometryAnchor query_wallplant_anchor(const SurfaceGeometryNativeApi&, const SurfaceGeometryContext&,
    SurfaceGeometryBody contacted_shape, SurfaceGeometryVector contact_point, SurfaceGeometryVector normal,
    SurfaceGeometryVector root, std::span<const SurfaceGeometryVector> body_offsets,
    SurfaceGeometryTrace* = nullptr) noexcept;
// Call only in the same supported client/park callback or same-thread scoped
// collision response contract. Never renews
// queried_at; physics/input consumers receive the copied publication only.
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi&, const SurfaceGeometryContext&,
                                 const SurfaceGeometrySample&) noexcept;
}
