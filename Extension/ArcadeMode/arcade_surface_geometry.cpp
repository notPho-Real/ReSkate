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
    // Wallplant consumers opt into static identity for either native owner
    // representation. The independently read entity must match the pool value.
    // Local self shapes retain the existing exact-entity exclusion path.
    const bool static_identity=world_owner && api.static_world_owner && value[0]!=ctx.self_entity;
    if (static_identity) {
        if (!api.static_world_owner(api.user,body,native_owner))return fail("static_owner_reader");
        if (!native_owner.value || native_owner.kind!=(value[0]?1u:0u) ||
            (value[0] && native_owner.value!=value[0]))return fail("static_owner_entity_mismatch");
    } else if (!value[0]) return fail("entity_zero");
    if (!api.picking.body_valid(&body)) return fail("shape_changed");
    if (api.picking.body_entity(after.data(),&body) != after.data()) return fail("entity_reader_again");
    if (value != after) return fail("entity_changed");
    if (!api.picking.body_valid(&body)) return fail("shape_changed_after_entity");
    if (!current(api,ctx,trace)) return fail("owner_context_changed");
    if (static_identity) {
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
struct Hit {
    SurfaceGeometryVector point; SurfaceGeometryBody body; std::uint64_t owner;
    SurfaceGeometryWorldOwner world_owner; bool local_component{};
    SurfaceGeometryVector normal{};
};
bool retain_adjacent(SurfaceGeometrySample& sample,const Hit& hit) noexcept {
    if(hit.body==sample.body)
        return hit.owner==sample.body_owner && hit.world_owner==sample.world_owner;
    // Equal entity IDs alone never establish static ownership. Independent
    // pieces need their own qualified shape/body/owner incarnation.
    if(!sample.world_owner.value || !hit.world_owner.value)return false;
    for(unsigned i=0;i<sample.adjacent_shape_count;++i)
        if(sample.adjacent_shapes[i]==hit.body)return sample.adjacent_world_owners[i]==hit.world_owner;
    if(sample.adjacent_shape_count==sample.adjacent_shapes.size())return false;
    const auto index=sample.adjacent_shape_count++;
    sample.adjacent_shapes[index]=hit.body;sample.adjacent_world_owners[index]=hit.world_owner;
    return true;
}
struct Exclusions {
    std::array<SurfaceGeometryBody,8> bodies{};
    std::array<SurfaceGeometryBody,8> query_handles{};
    std::array<SurfaceGeometryWorldOwner,8> local_components{};
    std::size_t count{};
};
bool exclusions_current(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                        const Exclusions& exclusions,SurfaceGeometryTrace* trace=nullptr) {
    if (!current(api,ctx,trace)) return false;
    for (std::size_t i = 0; i < exclusions.count; ++i) {
        if(exclusions.local_components[i].value) {
            SurfaceGeometryWorldOwner again{};
            if(!api.local_component_owner ||
               !api.local_component_owner(api.user,ctx,exclusions.bodies[i],again) ||
               again!=exclusions.local_components[i] || !current(api,ctx,trace))return false;
            continue;
        }
        std::uint64_t owner{};
        if (!body_owner(api,ctx,exclusions.bodies[i],owner,trace) || owner != ctx.self_entity) return false;
    }
    return true;
}
std::optional<Hit> cast(const SurfaceGeometryNativeApi& api, const SurfaceGeometryContext& ctx,
                       const SurfaceGeometrySegment& segment, Exclusions& excluded, SurfaceGeometryTrace* trace = nullptr,
                       bool* clear = nullptr, bool measure_contact = false,
                       const SurfaceGeometryWorldOwner* patch_owner = nullptr) {
    if(clear)*clear=false;
    const auto fail = [&](const char* reason) -> std::optional<Hit> {if(trace)trace->reason=reason;return {};};
    if(trace){trace->start=segment.start;trace->end=segment.end;}
    for (unsigned attempt = 0; attempt <= 8; ++attempt) {
        if(trace)trace->attempt=attempt;
        if (!exclusions_current(api,ctx,excluded,trace)) return fail("exclusions_current");
        editor::NativeSurfaceRay ray;
        const bool all_categories=clear || api.static_world_owner;
        if(all_categories) {
            // Wallplant's static-owner opt-in qualifies the actual nearest
            // shape. The camera filter can omit an otherwise eligible wall or
            // its supporting floor based on how that map authored categories.
            // Include those shapes and obstructions, then validate their owners.
            const auto filter=editor::surface_query_filter(body_query::QueryPreset::all);
            if(!filter)return fail("clearance_filter");
            ray.filter=*filter;
        }
        std::copy(segment.start.begin(),segment.start.end(),ray.start.begin());
        std::copy(segment.end.begin(),segment.end.end(),ray.end.begin());
        if (excluded.count) {
            ray.ignored_bodies[0] = reinterpret_cast<std::uintptr_t>(excluded.query_handles.data());
            ray.ignored_bodies[1] = reinterpret_cast<std::uintptr_t>(excluded.query_handles.data()+excluded.count);
            ray.ignored_bodies[2] = ray.ignored_bodies[1];
        }
        editor::NativeSurfaceResult result;
        bool released = false;
        std::optional<Hit> hit;
        bool empty=false;
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
                if (!read(api,result.data+8,fractions)) return fail("fractions");
                // Same counted-empty contract as Park Editor cast_surface_once:
                // native 0x26afdc0 -> 0x2664c00 publishes first=0,count=0 with
                // valid world/data even when the readable fraction lane is null.
                // An unavailable/malformed query is never clear.
                if(clear && !result.count) {
                    if(result.first)return fail("empty_result_first");
                    empty=true;return {};
                }
                if(!fractions)return fail("fractions");
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
                bool local_component=false;
                if(all_categories && api.local_component_owner && body.world==ctx.world &&
                   api.picking.body_valid(&body)) {
                    SurfaceGeometryWorldOwner first{},again{};
                    if(api.local_component_owner(api.user,ctx,body,first)) {
                        if(!first.value || !api.local_component_owner(api.user,ctx,body,again) ||
                           first!=again || !api.picking.body_valid(&body) || !current(api,ctx,trace))
                            return fail("local_component_changed");
                        world_owner=first;local_component=true;
                    }
                }
                if (!local_component && !body_owner(api,ctx,body,owner,trace,&world_owner)) return {};
                if(trace)trace->entity=owner;
                SurfaceGeometryVector point{};
                for (std::size_t i = 0; i < 3; ++i) point[i] = segment.start[i]+(segment.end[i]-segment.start[i])*closest;
                if(trace)trace->point=point;
                if (!finite(point)) return fail("point_invalid");
                Hit found{point,body,owner,world_owner,local_component};
                // A reciprocally authenticated local component is excluded by
                // its native owner below. Its hit vectors never describe a
                // destination, including when the ray starts inside that body.
                // Its original hit position must still match the ray fraction.
                if(local_component && measure_contact) {
                    const auto read_position=[&]<class T>(std::uintptr_t at,T& value) {return read(api,at,value);};
                    if(!api.surface.contact_ready ||
                       !body_contact_read::position(result.data,selected,point,found.point,read_position))
                        return fail("self_hit_position");
                }
                // Requested nonlocal measurements retain their validation.
                if(!local_component && (measure_contact || (patch_owner && owner!=ctx.self_entity && world_owner!=*patch_owner))) {
                    body_contact_read::Vectors vectors;
                    const auto read_vector=[&]<class T>(std::uintptr_t at,T& value) {
                        return read(api,at,value);
                    };
                    // Reuse the counted world-probe reader while this result
                    // still owns its native point and normal arrays.
                    if(!api.surface.contact_ready ||
                       !body_contact_read::vectors(result.data,selected,point,segment.start,segment.end,
                                                   vectors,read_vector))return fail("motion_hit_vectors");
                    found.point=vectors.position;found.normal=vectors.normal;
                }
                return found;
            };
            try { hit = inspect(); } catch (...) { hit = fail("query_exception"); }
        }
        if (!released) return fail("cleanup");
        if(empty) {
            if(!exclusions_current(api,ctx,excluded,trace))return fail("exclusions_changed");
            *clear=true;if(trace)trace->reason="clear";return {};
        }
        if (!hit) return {};
        if (!exclusions_current(api,ctx,excluded,trace)) return fail("exclusions_changed");
        if (!hit->local_component && hit->owner != ctx.self_entity) {if(trace)trace->reason="hit";return hit;} // unknown nearer owner never skipped
        // An entity identity alone cannot supply the native exclusion owner.
        if (all_categories && !hit->local_component) return fail("self_owner_unavailable");
        if (excluded.count == excluded.bodies.size() ||
            std::find(excluded.bodies.begin(),excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count),hit->body) !=
                excluded.bodies.begin()+static_cast<std::ptrdiff_t>(excluded.count)) return fail("self_exclusion_limit");
        excluded.query_handles[excluded.count]=hit->body;
        if(hit->local_component) {
            excluded.local_components[excluded.count]=hit->world_owner;
            // Native ray exclusions compare physics owner indices, not shape
            // indices. Keep the original shape separately for revalidation.
            excluded.query_handles[excluded.count]={hit->body.world,
                hit->world_owner.index,hit->world_owner.generation};
        }
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
        if (!(current(api,ctx,trace) && sample.fresh_for(ctx,api.now(api.user)) &&
            body_owner(api,ctx,sample.body,owner,trace,&world_owner) && owner == sample.body_owner &&
            world_owner == sample.world_owner)) return false;
        if(sample.adjacent_shape_count>sample.adjacent_shapes.size() ||
           (sample.adjacent_shape_count && !sample.world_owner.value))return false;
        for(unsigned i=0;i<sample.adjacent_shape_count;++i) {
            const auto& expected=sample.adjacent_world_owners[i];
            if(!expected.value || expected.kind>1)return false;
            if(!body_owner(api,ctx,sample.adjacent_shapes[i],owner,trace,&world_owner) ||
               owner!=(expected.kind?expected.value:0) || world_owner!=expected)return false;
        }
        return true;
    } catch (...) { return false; }
}
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
                                 const SurfaceGeometrySample& sample) noexcept {
    return revalidate(api,ctx,sample,nullptr);
}
VertPatch query_vert_patch(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryVector point,SurfaceGeometryVector normal,float reach,SurfaceGeometryTrace* trace) noexcept {
    using namespace vert_math;
    VertPatch patch;
    if(trace)*trace={};
    const auto fail=[&](const char* reason)->VertPatch {if(trace)trace->reason=reason;return {};};
    try {
        if(!current(api,ctx,trace) || !api.static_world_owner || !api.surface.contact_ready || !finite(point) ||
           !finite(normal) || !unit(normal) || normal[1]<-.01f || !std::isfinite(reach) || reach<=0 || reach>12)return fail("vert_patch_context");
        auto right=cross(normal,{0,1,0});
        if(!unit(right))right={1,0,0};
        const auto forward=cross(right,normal);
        constexpr std::array<std::array<float,2>,9> offsets{{{0,0},{-.3f,0},{.3f,0},{0,-.4f},{0,.4f},
            {-.3f,-.4f},{.3f,-.4f},{-.3f,.4f},{.3f,.4f}}};
        Exclusions excluded;
        for(std::size_t i=0;i<offsets.size();++i) {
            const auto center=add(point,add(scale(right,offsets[i][0]),scale(forward,offsets[i][1])));
            const SurfaceGeometrySegment ray{add(center,scale(normal,.5f)),add(center,scale(normal,-reach))};
            if(trace)trace->ray=static_cast<unsigned>(i);
            const auto hit=cast(api,ctx,ray,excluded,trace,nullptr,true);
            if(!hit)return {};
            if(!hit->world_owner.value || dot(hit->normal,normal)<.85f || hit->normal[1]<-.01f)return fail("vert_patch_normal");
            auto& sample=patch.samples[i];
            sample.context=ctx;sample.body=hit->body;sample.body_owner=hit->owner;sample.world_owner=hit->world_owner;
            sample.point=hit->point;sample.query_normal=hit->normal;sample.queried_at=api.now(api.user);sample.valid=true;
            if(i && (std::abs(dot(sub(sample.point,patch.center().point),patch.center().query_normal))>.12f ||
                     dot(sample.query_normal,patch.center().query_normal)<.85f))return fail("vert_patch_discontinuous");
        }
        for(const auto& sample:patch.samples)if(!revalidate(api,ctx,sample,trace))return fail("vert_patch_revalidate");
        patch.valid=exclusions_current(api,ctx,excluded,trace);if(trace)trace->reason=patch.valid?"vert_patch_valid":"vert_patch_exclusions";return patch;
    } catch(...) {return fail("vert_patch_exception");}
}
bool query_vert_clearance(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryVector point,SurfaceGeometryVector velocity,float gravity,float seconds,SurfaceGeometryVector direction) noexcept {
    using namespace vert_math;
    try {
        if(!current(api,ctx) || !finite(point) || !finite(velocity) || !std::isfinite(seconds) || seconds<=0 ||
           seconds>2.5f || !std::isfinite(gravity) || gravity<0 || gravity>60)return false;
        auto heading=horizontal(velocity);
        // At the apex use the owned ramp/intent direction to orient the same
        // footprint. It supplies no velocity and cannot make a zero ray clear.
        if(length(heading)<.9f)heading=horizontal(direction);
        auto right=cross(heading,{0,1,0});if(!unit(right))return false;
        // A finite swept ray bundle is conservative admission, not a claim of
        // capsule collision coverage. Native articulated collisions stay enabled.
        constexpr std::array<std::array<float,2>,6> offsets{{{0,.15f},{-.3f,.15f},{.3f,.15f},{0,1.6f},{-.3f,1.6f},{.3f,1.6f}}};
        Exclusions excluded;
        constexpr unsigned steps=12;
        for(unsigned step=0;step<steps;++step)for(const auto offset:offsets) {
            const auto shift=add(scale(right,offset[0]),SurfaceGeometryVector{0,offset[1],0});
            const auto start=add(position(point,velocity,gravity,seconds*static_cast<float>(step)/steps),shift);
            const auto end=add(position(point,velocity,gravity,seconds*static_cast<float>(step+1)/steps),shift);
            const auto delta=sub(end,start);if(dot(delta,delta)<=1e-12f)return false;
            bool clear{};
            (void)cast(api,ctx,{start,end},excluded,nullptr,&clear);
            if(!clear)return false;
        }
        return exclusions_current(api,ctx,excluded);
    } catch(...) {return false;}
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
        std::array<SurfaceGeometryVector,5> independent_normals{};
        for (std::size_t i = 0; i < points.size(); ++i) {
            if(trace)trace->ray=static_cast<unsigned>(i);
            const auto hit = cast(api,ctx,request.segments[i],excluded,trace,nullptr,false,i?&sample.world_owner:nullptr);
            if (!hit) return {};
            if(!i) {
                sample.body=hit->body;sample.body_owner=hit->owner;sample.world_owner=hit->world_owner;
            } else if(!retain_adjacent(sample,*hit))return fail("ground_shape_seam");
            if(hit->world_owner!=sample.world_owner)independent_normals[i]=hit->normal;
            points[i]=hit->point;
        }
        if (!plane(points,sample.query_normal)) return fail("ground_plane");
        for(auto normal:independent_normals)
            if(normal!=SurfaceGeometryVector{} && (!unit(normal) || dot(normal,sample.query_normal)<.995f))
                return fail("ground_piece_normal");
        sample.point = points[0]; sample.valid = true;
        if (!exclusions_current(api,ctx,excluded,trace) || !revalidate(api,ctx,sample,trace)) return fail("ground_revalidation");
        if(trace)trace->reason="ground_valid";
        return sample;
    } catch (...) { return fail("ground_exception"); }
}
SurfaceGeometrySample query_motion_wall(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryVector board,SurfaceGeometryVector up,SurfaceGeometryVector velocity,float seconds,
        SurfaceGeometryTrace* trace) noexcept {
    const auto fail=[&](const char* reason)->SurfaceGeometrySample {if(trace)trace->reason=reason;return {};};
    if(trace)*trace={};
    try {
        if(!current(api,ctx,trace) || !ctx.native.airborne || ctx.native.rail_family ||
           !finite(board) || !finite(up) || !finite(velocity) ||
           std::abs(dot(up,up)-1.f)>.02f || up[1]<.1f ||
           !std::isfinite(seconds) || seconds<.001f || seconds>.1f)return fail("motion_query_context");
        auto direction=velocity;
        if(!unit(direction))return fail("motion_query_velocity");
        SurfaceGeometryVector right{-direction[2],0,direction[0]};
        if(!unit(right))return fail("motion_query_vertical");
        constexpr float height=8.1f*.0254f,extension=10.f*.0254f;
        SurfaceGeometryVector travel{};
        for(unsigned axis=0;axis<3;++axis)travel[axis]=velocity[axis]*seconds+direction[axis]*extension;
        if(!finite(travel) || dot(travel,travel)>400.f)return fail("motion_query_reach");
        const auto queried_at=api.now(api.user);
        Exclusions excluded;
        SurfaceGeometrySegment segment;
        for(unsigned axis=0;axis<3;++axis) {
            segment.start[axis]=board[axis]+up[axis]*height;
            segment.end[axis]=segment.start[axis]+travel[axis];
        }
        const auto hit=cast(api,ctx,segment,excluded,trace,nullptr,true);
        if(!hit)return fail("motion_query_miss");
        // Travel-aligned side rays change their distance to an oblique wall.
        // First find the wall with the unchanged bounded motion ray, then
        // corroborate its full patch in the measured wall plane.
        auto result=query_wall_geometry(api,ctx,hit->body,hit->point,hit->normal,velocity,seconds,trace);
        if(!result.valid || result.body_owner!=hit->owner || result.world_owner!=hit->world_owner)
            return fail("motion_query_patch");
        result.queried_at=queried_at;
        if(!exclusions_current(api,ctx,excluded,trace) || !revalidate(api,ctx,result,trace))
            return fail("motion_query_changed");
        return result;
    } catch(...) {return fail("motion_query_exception");}
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
            const auto hit=cast(api,ctx,segment,excluded,trace,nullptr,false,i?&sample.world_owner:nullptr);
            if(!hit)return {};
            // Center remains the contacted shape. Other static pieces must
            // also present an actual facing native normal, not only ray points
            // which happen to line up across a railing or corner.
            if(!i) {
                if(hit->body!=body)return fail("wall_shape_seam");
                sample.body_owner=hit->owner;sample.world_owner=hit->world_owner;
            } else if(!retain_adjacent(sample,*hit))return fail("wall_shape_seam");
            if(hit->world_owner!=sample.world_owner) {
                auto measured_normal=hit->normal;
                if(!unit(measured_normal) || dot(measured_normal,normal)<.995f)return fail("wall_piece_normal");
            }
            points[i]=hit->point;
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
SurfaceGeometryAnchor query_wallplant_anchor(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        SurfaceGeometryBody shape,SurfaceGeometryVector point,SurfaceGeometryVector normal,
        SurfaceGeometryVector root,SurfaceGeometryVector board_offset,
        std::span<const SurfaceGeometryVector> body_offsets,
        SurfaceGeometryTrace* trace) noexcept {
    const auto fail=[&](const char* reason)->SurfaceGeometryAnchor {if(trace)trace->reason=reason;return {};};
    if(trace)*trace={};
    try {
        if(!current(api,ctx,trace) || !finite(point) || !finite(root) || !finite(board_offset) || !finite(normal) ||
           std::abs(dot(normal,normal)-1.f)>.02f || !unit(normal) || std::abs(normal[1])>.1f)
            return fail("anchor_context_normal");
        if(body_offsets.empty() || body_offsets.size()>32)return fail("anchor_supports");
        for(const auto& offset:body_offsets)if(!finite(offset))return fail("anchor_support_invalid");
        // The contact belongs to the board. The entity origin is a different
        // point in the articulated skater. Translate both by the same measured
        // displacement instead of pulling the entity origin onto a board anchor.
        // Keep the existing bound on the complete adjustment, not a shape radius.
        constexpr float distance=27.6f*.0254f;
        constexpr float plane_tolerance=.005f; // Existing plane corroboration tolerance.
        SurfaceGeometryAnchor result;
        SurfaceGeometryVector body{},body_target{};
        for(unsigned axis=0;axis<3;++axis) {
            body[axis]=root[axis]+board_offset[axis];
            body_target[axis]=point[axis]+normal[axis]*distance;
            result.target[axis]=body_target[axis]-board_offset[axis];
        }
        const auto delta=sub(result.target,root);
        result.displacement=std::sqrt(dot(delta,delta));
        if(trace) {
            trace->shape=shape;trace->point=point;trace->anchor_root=root;
            trace->anchor_body=body;trace->anchor_target=result.target;
            trace->anchor_displacement=result.displacement;
        }
        if(!finite(body) || !finite(body_target) || !finite(result.target) || !std::isfinite(result.displacement) ||
           result.displacement>distance)return fail("anchor_adjustment");
        SurfaceGeometrySample source;source.context=ctx;source.body=shape;source.queried_at=api.now(api.user);
        if(!body_owner(api,ctx,shape,source.body_owner,trace,&source.world_owner) ||
           source.body_owner==ctx.self_entity)return fail("anchor_shape");
        source.valid=true;
        Exclusions excluded;
        const auto clear_segment=[&](const SurfaceGeometryVector& start,const SurfaceGeometryVector& end) {
            if(!finite(start) || !finite(end))return false;
            if(trace)trace->ray=result.checked_rays;
            bool clear=false;
            const auto hit=cast(api,ctx,{start,end},excluded,trace,&clear);
            if(hit || !clear)return false;
            ++result.checked_rays;return true;
        };
        SurfaceGeometryVector outside{};
        for(unsigned axis=0;axis<3;++axis)outside[axis]=point[axis]+normal[axis]*plane_tolerance;
        // Never exclude the contacted wall or another obstacle. Only existing
        // revalidated local-player shape exclusions are permitted.
        if(!clear_segment(outside,body_target))return fail("anchor_outward_blocked");
        // An already stationary root introduces no translated support path.
        // The nonzero wall-outward segment and unchanged owner stamp remain
        // mandatory; a zero-length native ray is never taken as clear evidence.
        if(result.displacement>0 && !clear_segment(root,result.target))return fail("anchor_root_blocked");
        // The explicitly observed board must have a clearance path even if a
        // caller's additional support list does not include its origin.
        const bool board_in_supports=std::find(body_offsets.begin(),body_offsets.end(),board_offset)!=body_offsets.end();
        if(result.displacement>0 && board_offset!=SurfaceGeometryVector{} && !board_in_supports &&
           !clear_segment(body,body_target))return fail("anchor_board_blocked");
        if(result.displacement>0)for(const auto& offset:body_offsets) {
            SurfaceGeometryVector start{},end{};
            for(unsigned axis=0;axis<3;++axis){start[axis]=root[axis]+offset[axis];end[axis]=result.target[axis]+offset[axis];}
            if(!clear_segment(start,end))return fail("anchor_support_blocked");
        }
        if(!exclusions_current(api,ctx,excluded,trace) || !revalidate(api,ctx,source,trace))
            return fail("anchor_revalidation");
        result.valid=true;if(trace)trace->reason="anchor_rays_clear";return result;
    } catch(...) {return fail("anchor_exception");}
}

}
