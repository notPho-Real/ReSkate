#pragma once
#include "arcade_input_policy.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
using SurfaceGeometryVector = std::array<float, 3>;
struct SurfaceGeometryBody {
    std::uintptr_t world{};
    std::uint32_t index{UINT32_MAX}, generation{};
    bool operator==(const SurfaceGeometryBody&) const = default;
};
static_assert(sizeof(SurfaceGeometryBody) == 16);
enum class SurfaceGeometryPhase : std::uint8_t { unavailable, client, park };
struct SurfaceGeometryContext {
    std::array<std::uintptr_t, 5> owner{};
    InputNativeContext native;
    std::uintptr_t client_context{}, world{};
    std::uint64_t world_generation{}, self_entity{}, self_generation{};
    SurfaceGeometryPhase phase{};
    bool operator==(const SurfaceGeometryContext&) const = default;
    bool fresh(std::uint64_t now) const noexcept {
        return native.required && native.fresh(now) && client_context && world && world_generation &&
            self_entity && self_generation &&
            (phase == SurfaceGeometryPhase::client || phase == SurfaceGeometryPhase::park) &&
            owner[0] && owner[1] && owner[2] && owner[3] && owner[4];
    }
};
struct SurfaceGeometrySegment { SurfaceGeometryVector start{}, end{}; };
// Center, left, right, back, front. Only downward segment intersections count.
struct SurfaceGeometryRequest { std::array<SurfaceGeometrySegment, 5> segments; };
struct SurfaceGeometrySample {
    SurfaceGeometryContext context;
    SurfaceGeometryBody body;
    std::uint64_t body_owner{}, queried_at{};
    SurfaceGeometryVector point{}, query_normal{};
    bool valid{};
    // This is query geometry, never actual board support or prediction normal.
    // Revalidation of the live body is also required in the authorized callback.
    bool fresh_for(const SurfaceGeometryContext& current, std::uint64_t now) const noexcept {
        return valid && current.fresh(now) && context.native.fresh(now) && now >= queried_at &&
            now - queried_at <= InputTiming::native_sample_lease_ms && context.owner == current.owner &&
            context.client_context == current.client_context && context.world == current.world &&
            context.world_generation == current.world_generation && context.self_entity == current.self_entity &&
            context.self_generation == current.self_generation && context.native.generations == current.native.generations &&
            context.native.air_episode() == current.native.air_episode() &&
            context.native.airborne == current.native.airborne;
    }
};
namespace surface_geometry_detail {
inline SurfaceGeometryVector sub(SurfaceGeometryVector a, SurfaceGeometryVector b) noexcept {
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}
inline float dot(SurfaceGeometryVector a, SurfaceGeometryVector b) noexcept {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
inline SurfaceGeometryVector cross(SurfaceGeometryVector a, SurfaceGeometryVector b) noexcept {
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
inline bool finite(SurfaceGeometryVector v) noexcept {
    for (const float x : v) if (!std::isfinite(x) || std::abs(x)>100000) return false;
    return true;
}
inline bool unit(SurfaceGeometryVector& v) noexcept {
    const float length = std::sqrt(dot(v,v));
    if (!std::isfinite(length) || length < .00001f) return false;
    for (float& x : v) x /= length;
    return true;
}
inline bool plane(const std::array<SurfaceGeometryVector, 5>& points, SurfaceGeometryVector& normal) noexcept {
    for (const auto p : points) if (!finite(p)) return false;
    normal = cross(sub(points[4],points[3]),sub(points[2],points[1]));
    if (!unit(normal) || normal[1] <= 0) return false;
    constexpr std::array<unsigned, 5> ring{1,4,2,3,1};
    for (unsigned i = 0; i < 4; ++i) {
        auto face = cross(sub(points[ring[i]],points[0]),sub(points[ring[i+1]],points[0]));
        if (!unit(face) || dot(face,normal) < .995f ||
            std::abs(dot(sub(points[i+1],points[0]),normal)) > .005f) return false;
    }
    return true;
}
}
}
