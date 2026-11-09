#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace dingosdk::arcade_mode::wallplant_animation {
inline constexpr std::uint64_t hold_duration_ms=160;
// The physical entry owns this clock. A late client tick has less time to show
// the held native sample; it cannot restart or extend the physical phase.
inline std::optional<std::uint64_t> hold_remaining_ms(std::uint64_t start,std::uint64_t now) noexcept {
    if(!start || now<start || now-start>=hold_duration_ms)return {};
    return hold_duration_ms-(now-start);
}
struct FootWallMeasurement { float signed_plane_distance{},tangent_distance{}; };
struct PlantFrame {float time{},reach{};std::uint16_t joint{};};
inline bool consider_plant_frame(std::optional<PlantFrame>& best,float time,std::uint16_t joint,
        const std::array<float,3>& relative,const std::array<float,3>& direction) noexcept {
    if(!std::isfinite(time) || time<0)return false;
    float reach{};
    for(std::size_t i=0;i<3;++i) {
        if(!std::isfinite(relative[i]) || !std::isfinite(direction[i]))return false;
        reach+=relative[i]*direction[i];
    }
    if(!std::isfinite(reach))return false;
    if(!best || reach>best->reach)best=PlantFrame{time,reach,joint};
    return true;
}
// Express the real wall-facing direction in an upright outgoing target basis.
// Native pitch/roll and the player's bind pose are not inferred from this;
// actual rendered-foot measurements remain the independent evidence.
inline std::optional<std::array<float,3>> hold_wall_direction(const std::array<float,3>& normal,
        const std::array<float,3>& forward) noexcept {
    for(std::size_t i=0;i<3;++i)if(!std::isfinite(normal[i]) || !std::isfinite(forward[i]))return {};
    const auto n=std::hypot(normal[0],normal[1],normal[2]);
    const auto f=std::hypot(forward[0],forward[2]);
    if(n<.99f || n>1.01f || f<.99f || f>1.01f)return {};
    return std::array<float,3>{(-normal[0]*forward[2]+normal[2]*forward[0])/(n*f),
        -normal[1]/n,(-normal[0]*forward[0]-normal[2]*forward[2])/(n*f)};
}
// Joint origins and collision point share world space. Report distances only:
// joint proximity does not prove the sole touches the wall or physical anchoring.
inline std::optional<FootWallMeasurement> measure_foot_wall(const std::array<float,3>& foot,
        const std::array<float,3>& point,const std::array<float,3>& normal) noexcept {
    float norm{},distance{},length{};
    for(std::size_t i=0;i<3;++i) {
        if(!std::isfinite(foot[i]) || !std::isfinite(point[i]) || !std::isfinite(normal[i]))return {};
        const auto d=foot[i]-point[i];
        norm+=normal[i]*normal[i];distance+=d*normal[i];length+=d*d;
    }
    if(!std::isfinite(length) || !std::isfinite(distance) || std::abs(norm-1.f)>.001f)return {};
    return FootWallMeasurement{distance,std::sqrt(std::fmax(0.f,length-distance*distance))};
}
}
