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
bool current(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,SurfaceGeometryTrace* trace=nullptr) {
    const auto fail=[&](const char* reason){if(trace)trace->context_reason=reason;return false;};
    if(!(api.base && api.base == ctx.owner[0] && api.surface.ready && api.picking.ready && api.now && api.current && api.surface.world &&
        api.read_bytes && api.surface.ray && api.surface.scope_allocator && api.surface.release && api.picking.hit_body &&
        api.picking.body_valid && api.picking.body_entity))return fail("query_api");
    if(!ctx.fresh(api.now(api.user)))return fail("query_context_stale");
    if(!api.current(api.user,ctx))return fail(api.current_failure?api.current_failure(api.user):"callback_before_world");
    if(api.surface.world(ctx.client_context)!=ctx.world)return fail("query_world_changed");
    if(!api.current(api.user,ctx))return fail(api.current_failure?api.current_failure(api.user):"callback_after_world");
    if(trace)trace->context_reason="current";
    return true;
}
bool body_owner(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                const SurfaceGeometryBody& body, std::uint64_t& owner, SurfaceGeometryTrace* trace = nullptr,
                SurfaceGeometryWorldOwner* world_owner = nullptr) {
    alignas(16) std::array<std::uint64_t,2> value{}, after{};
    SurfaceGeometryWorldOwner native_owner{};
    const auto fail = [&](const char* reason) { if (trace) {trace->reason=reason;trace->shape=body;trace->entity=value[0];} return false; };
    if (body.world != ctx.world || body.index == UINT32_MAX || !api.picking.body_valid(&body)) return fail("shape_invalid");
    if (api.picking.body_entity(value.data(),&body) != value.data()) return fail("entity_reader");
    if (!value[0] && (!world_owner || !api.static_world_owner ||
        !api.static_world_owner(api.user,body,native_owner))) return fail("entity_zero");
    if (!api.picking.body_valid(&body)) return fail("shape_changed");
    if (api.picking.body_entity(after.data(),&body) != after.data()) return fail("entity_reader_again");
    if (value != after) return fail("entity_changed");
    if (!api.picking.body_valid(&body)) return fail("shape_changed_after_entity");
    if (!current(api,ctx,trace)) return fail("owner_context_changed");
    if (!value[0]) {
        SurfaceGeometryWorldOwner again{};
        if (!api.static_world_owner(api.user,body,again))return fail("static_owner_reader_again");
        if (again!=native_owner)return fail("static_owner_stamp_changed");
        if (!current(api,ctx,trace))return fail("static_owner_context_changed");
    }
    owner = value[0];
    if(world_owner)*world_owner=native_owner;
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
struct Hit { SurfaceGeometryVector point; SurfaceGeometryBody body; std::uint64_t owner; SurfaceGeometryWorldOwner world_owner; };
struct Exclusions { std::array<SurfaceGeometryBody,8> bodies{}; std::size_t count{}; };
bool exclusions_current(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                        const Exclusions& exclusions,SurfaceGeometryTrace* trace=nullptr) {
    if (!current(api,ctx,trace)) return false;
    for (std::size_t i = 0; i < exclusions.count; ++i) {
        std::uint64_t owner{};
        if (!body_owner(api,ctx,exclusions.bodies[i],owner,trace) || owner != ctx.self_entity) return false;
    }
    return true;
}
std::optional<Hit> cast(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                       const SurfaceGeometrySegment& segment, Exclusions& excluded, SurfaceGeometryTrace* trace = nullptr) {
    const auto fail = [&](const char* reason) -> std::optional<Hit> {if(trace)trace->reason=reason;return {};};
    if(trace){trace->start=segment.start;trace->end=segment.end;}
    for (unsigned attempt = 0; attempt <= 8; ++attempt) {
        if(trace)trace->attempt=attempt;
        if (!exclusions_current(api,ctx,excluded,trace)) return fail("exclusions_current");
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
                if(trace)trace->hits=result.count;
                if (result.world != ctx.world || !result.data || result.count > 4096 ||
                    result.first > UINT32_MAX-result.count) return fail("ray_result");
                std::uintptr_t fractions{};
                if (!read(api,result.data+8,fractions) || !fractions) return fail("fractions");
                float closest = 2;
                std::uint32_t selected{};
                for (std::uint32_t i = 0; i < result.count; ++i) {
                    float fraction{};
                    if (!read(api,fractions+static_cast<std::uintptr_t>(result.first+i)*sizeof(float),fraction) ||
                        !std::isfinite(fraction) || fraction < 0 || fraction > 1) return fail("fraction_invalid");
                    if (fraction < closest) { closest = fraction; selected = result.first+i; }
                }
                if (closest > 1) return fail("ray_miss");
                if(trace)trace->value=closest;
                alignas(16) const std::array<std::uint64_t,3> descriptor{ctx.world,selected,result.data};
                alignas(16) SurfaceGeometryBody body;
                std::uint64_t owner{};
                SurfaceGeometryWorldOwner world_owner{};
                if (api.picking.hit_body(descriptor.data(),&body) != &body) return fail("hit_shape");
                if(trace)trace->shape=body;
                if (!body_owner(api,ctx,body,owner,trace,&world_owner)) return {};
                if(trace)trace->entity=owner;
                SurfaceGeometryVector point{};
                for (std::size_t i = 0; i < 3; ++i) point[i] = segment.start[i]+(segment.end[i]-segment.start[i])*closest;
                if(trace)trace->point=point;
                if (!finite(point)) return fail("point_invalid");
                return Hit{point,body,owner,world_owner};
            };
            try { hit = inspect(); } catch (...) { hit = fail("query_exception"); }
        }
        if (!released) return fail("cleanup");
        if (!hit) return {};
        if (!exclusions_current(api,ctx,excluded,trace)) return fail("exclusions_changed");
        if (hit->owner != ctx.self_entity) {if(trace)trace->reason="hit";return hit;} // unknown nearer owner never skipped
        if (excluded.count == excluded.bodies.size() ||
            std::find(excluded.bodies.begin(),excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count),hit->body) !=
                excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count)) return fail("self_exclusion_limit");
        excluded.bodies[excluded.count++] = hit->body;
    }
    return {};
}
}
static bool revalidate(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                       const SurfaceGeometrySample& sample,SurfaceGeometryTrace* trace) noexcept {
    try {
        std::uint64_t owner{};
        SurfaceGeometryWorldOwner world_owner{};
        return current(api,ctx,trace) && sample.fresh_for(ctx,api.now(api.user)) &&
            body_owner(api,ctx,sample.body,owner,trace,&world_owner) && owner == sample.body_owner &&
            world_owner == sample.world_owner;
    } catch (...) { return false; }
}
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
                                 const SurfaceGeometrySample& sample) noexcept {
    return revalidate(api,ctx,sample,nullptr);
}
SurfaceGeometrySample query_surface_geometry(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                                             const SurfaceGeometryRequest& request, SurfaceGeometryTrace* trace) noexcept {
    const auto fail=[&](const char* reason) -> SurfaceGeometrySample {if(trace)trace->reason=reason;return {};};
    if(trace)*trace={};
    try {
        if (!current(api,ctx,trace)) return fail("query_context");
        SurfaceGeometrySample sample;
        sample.context = ctx; sample.queried_at = api.now(api.user);
        for (const auto& segment : request.segments) {
            const auto delta = sub(segment.end,segment.start);
            if (!finite(segment.start) || !finite(segment.end) || delta[1] >= 0 || delta[1] < -20 ||
                std::abs(delta[0]) > .00001f || std::abs(delta[2]) > .00001f) return fail("ground_segment");
            const auto offset = sub(segment.start,request.segments[0].start);
            if (offset[0]*offset[0]+offset[2]*offset[2] > 4) return fail("ground_footprint");
        }
        Exclusions excluded;
        std::array<SurfaceGeometryVector,5> points{};
        for (std::size_t i = 0; i < points.size(); ++i) {
            if(trace)trace->ray=static_cast<unsigned>(i);
            const auto hit = cast(api,ctx,request.segments[i],excluded,trace);
            if (!hit) return {};
            if (i && (hit->body != sample.body || hit->owner != sample.body_owner || hit->world_owner!=sample.world_owner))
                return fail("ground_shape_seam");
            sample.body = hit->body; sample.body_owner = hit->owner; sample.world_owner=hit->world_owner; points[i] = hit->point;
        }
        if (!plane(points,sample.query_normal)) return fail("ground_plane");
        sample.point = points[0]; sample.valid = true;
        if (!exclusions_current(api,ctx,excluded,trace) || !revalidate(api,ctx,sample,trace)) return fail("ground_revalidation");
        if(trace)trace->reason="ground_valid";
        return sample;
    } catch (...) { return fail("ground_exception"); }
}
SurfaceGeometrySample query_wall_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryBody body,SurfaceGeometryVector point,SurfaceGeometryVector normal,
        SurfaceGeometryVector approach_velocity,float native_seconds,SurfaceGeometryTrace* trace) noexcept {
    const auto fail=[&](const char* reason) -> SurfaceGeometrySample {if(trace)trace->reason=reason;return {};};
    if(trace)*trace={};
    try {
        if(!current(api,ctx,trace) || !finite(point) || !finite(normal) ||
            std::abs(dot(normal,normal)-1.f)>.02f || !unit(normal) || std::abs(normal[1])>.1f) return fail("wall_context_normal");
        // A broad patch excludes narrow rails/poles and corner contacts. Half
        // width/height use the retained classic arcade minimum plant height (24 inches).
        constexpr float extent=24.f*.0254f;
        constexpr float depth=.1f; // Query reach only, never a displacement.
        // Match the existing NativeInputState timestep admission. The runtime
        // additionally matches this value to the retained original physics frame.
        if(!finite(approach_velocity) || !std::isfinite(native_seconds) ||
           native_seconds<.001f || native_seconds>.1f)return fail("wall_step_motion");
        const float approach=-dot(approach_velocity,normal);
        if(!std::isfinite(approach) || approach<=0)return fail("wall_step_approach");
        const float inward=std::max(depth,approach*native_seconds);
        // Keep the existing surface-query maximum segment length; malformed
        // native motion must not turn corroboration into an unbounded search.
        if(!std::isfinite(inward) || inward+depth>20.f)return fail("wall_step_reach");
        const SurfaceGeometryVector right{-normal[2],0,normal[0]};
        auto up=cross(right,normal);
        if(!unit(up))return fail("wall_basis");
        constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-extent,0},{extent,0},{0,-extent},{0,extent}}};
        SurfaceGeometrySample sample;sample.context=ctx;sample.body=body;sample.queried_at=api.now(api.user);
        Exclusions excluded;std::array<SurfaceGeometryVector,5> points{};
        for(std::size_t i=0;i<points.size();++i) {
            SurfaceGeometrySegment segment;
            // Solver contact coordinates are not a triangle-plane promise:
            // the pinned AIR contact can precede the same shape's ray plane.
            // Search inward by at most the observed step's normal travel, then
            // measure the broad patch around the actual same-shape hit. This
            // does not assert a speculative-contact or separation-field meaning.
            const auto anchor=i?points[0]:point;
            for(unsigned axis=0;axis<3;++axis) {
                const float center=anchor[axis]+right[axis]*offsets[i][0]+up[axis]*offsets[i][1];
                segment.start[axis]=center+normal[axis]*depth;
                segment.end[axis]=center-normal[axis]*(i?depth:inward);
            }
            if(trace)trace->ray=static_cast<unsigned>(i);
            const auto hit=cast(api,ctx,segment,excluded,trace);
            if(!hit)return {};
            if(hit->body!=body || (i && (hit->owner!=sample.body_owner || hit->world_owner!=sample.world_owner)))return fail("wall_shape_seam");
            sample.body_owner=hit->owner;sample.world_owner=hit->world_owner;points[i]=hit->point;
            const auto separation=std::abs(dot(sub(hit->point,points[0]),normal));
            if(trace)trace->value=separation;
            if(separation>.005f)return fail("wall_patch_plane_offset");
        }
        auto measured=cross(sub(points[4],points[3]),sub(points[2],points[1]));
        if(!unit(measured) || dot(measured,normal)<.995f)return fail("wall_plane_normal");
        sample.point=points[0];sample.query_normal=measured;sample.valid=true;
        if(!exclusions_current(api,ctx,excluded,trace) || !revalidate(api,ctx,sample,trace))return fail("wall_revalidation");
        if(trace)trace->reason="wall_valid";
        return sample;
    } catch(...) {return fail("wall_exception");}
}
}
