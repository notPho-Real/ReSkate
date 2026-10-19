// SPDX-License-Identifier: GPL-3.0-only
// Adapted from the published ReSkate traffic surface transform helpers. See LICENSE.
#pragma once
#include "arcade_surface_geometry.h"
#include <cmath>
namespace dingosdk::arcade_mode::surface_motion_math {
using V = SurfaceGeometryVector;
using Q = std::array<float, 4>;
inline V add(V a, V b) noexcept { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; }
inline V sub(V a, V b) noexcept { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
inline V mul(V a, float b) noexcept { return {a[0]*b, a[1]*b, a[2]*b}; }
inline float dot(V a, V b) noexcept { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
inline V cross(V a, V b) noexcept { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
inline bool finite(V a) noexcept {
    return std::isfinite(a[0]) && std::isfinite(a[1]) && std::isfinite(a[2]);
}
inline V unit(V a) noexcept {
    const auto n = dot(a, a);
    return finite(a) && std::isfinite(n) && n > 1e-10f ? mul(a, 1/std::sqrt(n)) : V{};
}
inline V rotate(V v, Q q) noexcept {
    const V u{q[0],q[1],q[2]};
    const auto t = mul(cross(u,v),2);
    return add(v,add(mul(t,q[3]),cross(u,t)));
}
struct Transform { V scale, position; Q rotation, inverse; };
inline bool transform(const std::array<float,12>& pose, Transform& out) noexcept {
    for (unsigned lane : {0U,1U,2U,4U,5U,6U,7U,8U,9U,10U})
        if (!std::isfinite(pose[lane])) return false;
    if (pose[0]<.0001f || pose[1]<.0001f || pose[2]<.0001f ||
        pose[0]>100 || pose[1]>100 || pose[2]>100 ||
        std::abs(pose[8])>100000 || std::abs(pose[9])>100000 || std::abs(pose[10])>100000)
        return false;
    const auto norm = pose[4]*pose[4]+pose[5]*pose[5]+pose[6]*pose[6]+pose[7]*pose[7];
    if (norm < .98f || norm > 1.02f) return false;
    const auto s = 1/std::sqrt(norm);
    out = {{pose[0],pose[1],pose[2]},{pose[8],pose[9],pose[10]},
        {pose[4]*s,pose[5]*s,pose[6]*s,pose[7]*s},
        {-pose[4]*s,-pose[5]*s,-pose[6]*s,pose[7]*s}};
    return true;
}
inline V local_point(V point, const Transform& t) noexcept {
    const auto p = rotate(sub(point,t.position),t.inverse);
    return {p[0]/t.scale[0],p[1]/t.scale[1],p[2]/t.scale[2]};
}
inline V world_point(V point, const Transform& t) noexcept {
    return add(t.position,rotate({point[0]*t.scale[0],point[1]*t.scale[1],point[2]*t.scale[2]},t.rotation));
}
inline V local_normal(V normal, const Transform& t) noexcept {
    const auto n = rotate(normal,t.inverse);
    return unit({n[0]*t.scale[0],n[1]*t.scale[1],n[2]*t.scale[2]});
}
inline V world_normal(V normal, const Transform& t) noexcept {
    return unit(rotate({normal[0]/t.scale[0],normal[1]/t.scale[1],normal[2]/t.scale[2]},t.rotation));
}
}
