// SPDX-License-Identifier: GPL-3.0-only
// Read-only selected traffic-surface contracts adapted from ReSkate. See LICENSE.
#pragma once
#include "arcade_surface_motion.h"
#include "arcade_surface_motion_math.h"

namespace dingosdk::arcade_mode {
// Supplied by the existing runtime owner: no second incarnation/removal journal.
struct TrafficSurfaceSelection {
    std::uint64_t id{}, generation{}, removals{};
    bool removal_free{};
    bool operator==(const TrafficSurfaceSelection&) const = default;
};
struct TrafficSurfaceNativeApi {
    std::uintptr_t base{};
    bool ready{}, parent_ready{}, mapping_ready{}, peer_ready{};
    bool (*valid)(const void*){};
    void* (*query)(void*,std::uint64_t){};
    float* (*pose)(float*,const void*){};
    bool (*body_valid)(const void*){};
    void* (*body_entity)(void*,const void*){};
    void* (*parent)(const void*,void*){};
    std::uintptr_t (*mapping)(){};
    void* (*map_entity)(std::uintptr_t,void*,std::uint64_t,int){};
    std::uint64_t (*component)(const void*,const void*,void*,std::uint32_t){};
    std::uintptr_t peer_vtable{}, peer_type{}, peer_type_vtable{};
    bool (*read_bytes)(std::uintptr_t,void*,std::size_t) noexcept{};
    void* user{};
    double (*seconds)(void*) noexcept{}; // original observation clock, never poll time
    std::uint64_t (*milliseconds)(void*) noexcept{}; // native/P2 clock domain
    // Existing callback owner proves lock/thread/phase, world/client/server/
    // population membership, selected incarnation and no in-flight removal.
    bool (*current)(void*,const SurfaceGeometryContext&,TrafficSurfaceSelection) noexcept{};
};
enum class TrafficSurfaceOwner : std::uint8_t { unavailable, direct, mapped, peer, parent_direct, parent_mapped, parent_peer };
struct TrafficSurfacePose {
    SurfaceGeometryContext context;
    TrafficSurfaceSelection selected;
    std::array<std::uint64_t,3> query{};
    std::array<float,12> transform{};
    double sampled_at{};
    std::uint64_t sampled_ms{};
    bool valid{};
};
TrafficSurfacePose capture_traffic_surface_pose(const TrafficSurfaceNativeApi&,const SurfaceGeometryContext&,
                                               TrafficSurfaceSelection) noexcept;
TrafficSurfaceOwner traffic_surface_owner(const TrafficSurfaceNativeApi&,const SurfaceGeometryContext&,
                                          TrafficSurfaceSelection,SurfaceGeometryBody) noexcept;
struct TrafficSurfaceMotion {
    SurfaceGeometryContext context;
    TrafficSurfaceSelection selected;
    SurfaceGeometryBody body;
    SurfaceGeometryVector local_point{}, point{}, normal{}, velocity{};
    std::uint64_t surface_serial{}, queried_at{}, observed_ms{};
    double sampled_at{}, observed_at{};
    bool valid{};
};
class TrafficSurfaceProvider {
public:
    // Capture pose before the P2 query in the SAME qualified callback. Neither
    // query nor pose is claimed to be an atomic physics/contact snapshot.
    bool bind(const TrafficSurfaceNativeApi&,const TrafficSurfacePose&,const SurfaceGeometrySample&) noexcept;
    TrafficSurfaceMotion observe(const TrafficSurfaceNativeApi&,const SurfaceGeometryContext&,TrafficSurfaceSelection) noexcept;
    TrafficSurfaceMotion snapshot(const SurfaceGeometryContext&,TrafficSurfaceSelection,double now,
                                  std::uint64_t now_ms) const noexcept;
    void invalidate() noexcept;
    std::uint64_t surface_serial() const noexcept { return serial_; }
private:
    TrafficSurfacePose anchor_;
    SurfaceGeometryBody body_;
    SurfaceGeometryVector local_point_{}, local_normal_{};
    SurfacePointMotion motion_;
    TrafficSurfaceMotion output_;
    std::uint64_t serial_{}, next_serial_{1}, queried_at_{};
};
}
