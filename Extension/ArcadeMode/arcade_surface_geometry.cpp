// SPDX-License-Identifier: GPL-3.0-only
// Native result lifecycle and closest-hit/body validation adapted from ReSkate
// Park Editor and the experimental traffic-surface consumer. See LICENSE.
#include "arcade_surface_geometry_native.h"
#include <algorithm>
#include <optional>
#include <span>

namespace dingosdk::arcade_mode {
namespace {
using namespace surface_geometry_detail;
template<class T> bool read(const SurfaceGeometryNativeApi& api, std::uintptr_t at, T& value) noexcept {
    return api.read_bytes && api.read_bytes(at,&value,sizeof value);
}
bool current(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx) {
    return api.base && api.base == ctx.owner[0] && api.surface.ready && api.picking.ready && api.now && api.current && api.surface.world &&
        api.read_bytes && api.surface.ray && api.surface.scope_allocator && api.surface.release && api.picking.hit_body &&
        api.picking.body_valid && api.picking.body_entity && ctx.fresh(api.now(api.user)) &&
        api.current(api.user,ctx) && api.surface.world(ctx.client_context) == ctx.world && api.current(api.user,ctx);
}
bool body_owner(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                const SurfaceGeometryBody& body, std::uint64_t& owner) {
    alignas(16) std::array<std::uint64_t,2> value{}, after{};
    if (body.world != ctx.world || body.index == UINT32_MAX || !api.picking.body_valid(&body) ||
        api.picking.body_entity(value.data(),&body) != value.data() || !value[0] ||
        !api.picking.body_valid(&body) || api.picking.body_entity(after.data(),&body) != after.data() ||
        value != after || !api.picking.body_valid(&body) || !current(api,ctx)) return false;
    owner = value[0];
    return true;
}
bool cleanup(const SurfaceGeometryNativeApi& api, editor::NativeSurfaceResult& result) noexcept {
    bool ok = true;
    try {
        if (result.scope[2]) {
            const auto allocator = api.surface.scope_allocator();
            std::uintptr_t table{}, pop{};
            if (!read(api,allocator,table) || !read(api,table+8,pop) || !pop) ok = false;
            else reinterpret_cast<void (*)(std::uintptr_t,void*)>(pop)(allocator,result.scope.data());
        }
    } catch (...) { ok = false; }
    try { if (result.allocator) api.surface.release(&result.allocator_vtable,result.data); }
    catch (...) { ok = false; }
    return ok;
}
struct Hit { SurfaceGeometryVector point; SurfaceGeometryBody body; std::uint64_t owner; };
struct Exclusions { std::array<SurfaceGeometryBody,8> bodies{}; std::size_t count{}; };
bool exclusions_current(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                        const Exclusions& exclusions) {
    if (!current(api,ctx)) return false;
    for (std::size_t i = 0; i < exclusions.count; ++i) {
        std::uint64_t owner{};
        if (!body_owner(api,ctx,exclusions.bodies[i],owner) || owner != ctx.self_entity) return false;
    }
    return true;
}
std::optional<Hit> cast(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                       const SurfaceGeometrySegment& segment, Exclusions& excluded) {
    for (unsigned attempt = 0; attempt <= 8; ++attempt) {
        if (!exclusions_current(api,ctx,excluded)) return {};
        editor::NativeSurfaceRay ray;
        std::copy(segment.start.begin(),segment.start.end(),ray.start.begin());
        std::copy(segment.end.begin(),segment.end.end(),ray.end.begin());
        if (excluded.count) {
            ray.ignored_bodies[0] = reinterpret_cast<std::uintptr_t>(excluded.bodies.data());
            ray.ignored_bodies[1] = reinterpret_cast<std::uintptr_t>(excluded.bodies.data()+excluded.count);
            ray.ignored_bodies[2] = ray.ignored_bodies[1];
        }
        editor::NativeSurfaceResult result;
        bool released = false;
        std::optional<Hit> hit;
        {
            struct Release {
                const SurfaceGeometryNativeApi& api; editor::NativeSurfaceResult& result; bool& ok;
                ~Release() { ok = cleanup(api,result); }
            } release{api,result,released};
            // Install cleanup before the call: partial query failure owns scope too.
            const auto inspect = [&]() -> std::optional<Hit> {
                api.surface.ray(ctx.world,&result,&ray,"ReSkate_SurfaceGeometry");
                if (result.world != ctx.world || !result.data || result.count > 4096 ||
                    result.first > UINT32_MAX-result.count) return {};
                std::uintptr_t fractions{};
                if (!read(api,result.data+8,fractions) || !fractions) return {};
                float closest = 2;
                std::uint32_t selected{};
                for (std::uint32_t i = 0; i < result.count; ++i) {
                    float fraction{};
                    if (!read(api,fractions+static_cast<std::uintptr_t>(result.first+i)*sizeof(float),fraction) ||
                        !std::isfinite(fraction) || fraction < 0 || fraction > 1) return {};
                    if (fraction < closest) { closest = fraction; selected = result.first+i; }
                }
                if (closest > 1) return {};
                alignas(16) const std::array<std::uint64_t,3> descriptor{ctx.world,selected,result.data};
                alignas(16) SurfaceGeometryBody body;
                std::uint64_t owner{};
                if (api.picking.hit_body(descriptor.data(),&body) != &body || !body_owner(api,ctx,body,owner)) return {};
                SurfaceGeometryVector point{};
                for (std::size_t i = 0; i < 3; ++i) point[i] = segment.start[i]+(segment.end[i]-segment.start[i])*closest;
                if (!finite(point)) return {};
                return Hit{point,body,owner};
            };
            try { hit = inspect(); } catch (...) { hit = {}; }
        }
        if (!released || !hit || !exclusions_current(api,ctx,excluded)) return {};
        if (hit->owner != ctx.self_entity) return hit; // unknown nearer owner never skipped
        if (excluded.count == excluded.bodies.size() ||
            std::find(excluded.bodies.begin(),excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count),hit->body) !=
                excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count)) return {};
        excluded.bodies[excluded.count++] = hit->body;
    }
    return {};
}
}
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                                 const SurfaceGeometrySample& sample) noexcept {
    try {
        std::uint64_t owner{};
        return current(api,ctx) && sample.fresh_for(ctx,api.now(api.user)) &&
            body_owner(api,ctx,sample.body,owner) && owner == sample.body_owner;
    } catch (...) { return false; }
}
SurfaceGeometrySample query_surface_geometry(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                                             const SurfaceGeometryRequest& request) noexcept {
    try {
        if (!current(api,ctx)) return {};
        SurfaceGeometrySample sample;
        sample.context = ctx; sample.queried_at = api.now(api.user);
        for (const auto& segment : request.segments) {
            const auto delta = sub(segment.end,segment.start);
            if (!finite(segment.start) || !finite(segment.end) || delta[1] >= 0 || delta[1] < -20 ||
                std::abs(delta[0]) > .00001f || std::abs(delta[2]) > .00001f) return {};
            const auto offset = sub(segment.start,request.segments[0].start);
            if (offset[0]*offset[0]+offset[2]*offset[2] > 4) return {};
        }
        Exclusions excluded;
        std::array<SurfaceGeometryVector,5> points{};
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto hit = cast(api,ctx,request.segments[i],excluded);
            if (!hit || (i && (hit->body != sample.body || hit->owner != sample.body_owner))) return {};
            sample.body = hit->body; sample.body_owner = hit->owner; points[i] = hit->point;
        }
        if (!plane(points,sample.query_normal)) return {};
        sample.point = points[0]; sample.valid = true;
        if (!exclusions_current(api,ctx,excluded) || !revalidate_surface_geometry(api,ctx,sample)) return {};
        return sample;
    } catch (...) { return {}; }
}
SurfaceGeometrySample query_wall_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryBody body,SurfaceGeometryVector point,SurfaceGeometryVector normal) noexcept {
    try {
        if(!current(api,ctx) || !finite(point) || !finite(normal) ||
            std::abs(dot(normal,normal)-1.f)>.02f || !unit(normal) || std::abs(normal[1])>.1f) return {};
        // A broad patch excludes narrow rails/poles and corner contacts. Half
        // width/height use the retained classic arcade minimum plant height (24 inches).
        constexpr float extent=24.f*.0254f;
        constexpr float depth=.1f; // Query reach only, never a displacement.
        const SurfaceGeometryVector right{-normal[2],0,normal[0]};
        auto up=cross(right,normal);
        if(!unit(up))return {};
        constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-extent,0},{extent,0},{0,-extent},{0,extent}}};
        SurfaceGeometrySample sample;sample.context=ctx;sample.body=body;sample.queried_at=api.now(api.user);
        Exclusions excluded;std::array<SurfaceGeometryVector,5> points{};
        for(std::size_t i=0;i<points.size();++i) {
            SurfaceGeometrySegment segment;
            for(unsigned axis=0;axis<3;++axis) {
                const float center=point[axis]+right[axis]*offsets[i][0]+up[axis]*offsets[i][1];
                segment.start[axis]=center+normal[axis]*depth;
                segment.end[axis]=center-normal[axis]*depth;
            }
            const auto hit=cast(api,ctx,segment,excluded);
            if(!hit || hit->body!=body || (i && hit->owner!=sample.body_owner))return {};
            sample.body_owner=hit->owner;points[i]=hit->point;
            if(std::abs(dot(sub(hit->point,point),normal))>.005f)return {};
        }
        auto measured=cross(sub(points[4],points[3]),sub(points[2],points[1]));
        if(!unit(measured) || dot(measured,normal)<.995f)return {};
        sample.point=points[0];sample.query_normal=measured;sample.valid=true;
        if(!exclusions_current(api,ctx,excluded) || !revalidate_surface_geometry(api,ctx,sample))return {};
        return sample;
    } catch(...) {return {};}
}
}
