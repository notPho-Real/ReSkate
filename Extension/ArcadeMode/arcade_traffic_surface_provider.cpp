// SPDX-License-Identifier: GPL-3.0-only
// Adapted from ReSkate's published selected-traffic identity/pose and surface
// ownership readers. No enumeration, hook, gameplay actuation or support claim.
#include "arcade_traffic_surface_provider.h"
#include <algorithm>
#include <optional>

namespace dingosdk::arcade_mode {
namespace {
template<class T> bool read(const TrafficSurfaceNativeApi& api,std::uintptr_t at,T& out) {
    return api.read_bytes && api.read_bytes(at,&out,sizeof out);
}
bool pointer(std::uintptr_t value) { return value>=0x10000 && value%8==0 && value<=0x00007fffffffffefULL; }
bool current(const TrafficSurfaceNativeApi& api,const SurfaceGeometryContext& ctx,TrafficSurfaceSelection selected) {
    return api.ready && api.base==ctx.owner[0] && api.valid && api.query && api.pose && api.body_valid && api.body_entity &&
        api.read_bytes && api.seconds && api.milliseconds && api.current && (selected.id>>32) && selected.generation &&
        selected.removal_free && ctx.fresh(api.milliseconds(api.user)) && api.current(api.user,ctx,selected);
}
bool scope(const SurfaceGeometryContext& a,const SurfaceGeometryContext& b) {
    return a.owner==b.owner && a.client_context==b.client_context && a.world==b.world &&
        a.world_generation==b.world_generation && a.self_entity==b.self_entity && a.self_generation==b.self_generation &&
        a.native.generations==b.native.generations && a.native.air_episode()==b.native.air_episode() && a.native.airborne==b.native.airborne;
}
struct alignas(16) Query {
    std::array<std::uint64_t,3> value{};
    std::uint64_t padding{};
    bool operator==(const Query& other) const { return value==other.value; }
};
static_assert(sizeof(Query)==32);
bool valid(const TrafficSurfaceNativeApi& api,std::uint64_t id) {
    alignas(16) const std::array<std::uint64_t,2> ref{id,0};
    return id && api.valid(ref.data());
}
bool identity(const TrafficSurfaceNativeApi& api,const Query& q) {
    std::uint32_t realm{}, record{};
    return (q.value[0]>>32) && pointer(q.value[1]) && pointer(q.value[2]) &&
        read(api,q.value[1]+8,realm) && realm==static_cast<std::uint32_t>(q.value[0]>>32) &&
        read(api,q.value[2],record) && record==static_cast<std::uint32_t>(q.value[0]) && valid(api,q.value[0]);
}
bool query(const TrafficSurfaceNativeApi& api,std::uint64_t id,Query& q) {
    return api.query(q.value.data(),id)==q.value.data() && q.value[0]==id && identity(api,q);
}
bool query_current(const TrafficSurfaceNativeApi& api,const Query& q) {
    Query after;
    return identity(api,q) && query(api,q.value[0],after) && after==q;
}
bool parent(const TrafficSurfaceNativeApi& api,const Query& q,Query& out) {
    return api.parent_ready && api.parent && api.parent(q.value.data(),out.value.data())==out.value.data();
}
bool chain_current(const TrafficSurfaceNativeApi& api,const std::array<Query,9>& chain,std::size_t depth) {
    for (std::size_t i=0;i<=depth;++i) {
        if (!query_current(api,chain[i])) return false;
        Query after;
        if (i<depth && (!parent(api,chain[i],after) || after!=chain[i+1])) return false;
    }
    return true;
}
std::uint64_t mapped(const TrafficSurfaceNativeApi& api,std::uint64_t id) {
    if (!api.mapping_ready || !api.mapping || !api.map_entity) return 0;
    const auto manager=api.mapping();
    std::uintptr_t buckets{}; std::uint32_t count{}, entries{}; std::uint64_t out{};
    if (!manager || !read(api,manager+0x70,buckets) || !buckets || !read(api,manager+0x78,count) ||
        !count || count>0x100000 || !read(api,manager+0x7c,entries) || !entries ||
        api.map_entity(manager,&out,id,1)!=&out) return 0;
    return out;
}
struct alignas(16) Peer {
    std::array<std::uintptr_t,5> value{};
    std::uint64_t id{};
    bool operator==(const Peer&) const = default;
};
bool peer(const TrafficSurfaceNativeApi& api,const Query& q,Peer& out) {
    std::uintptr_t type{};
    if (!api.peer_ready || !api.component || !api.peer_type || !api.peer_type_vtable || !api.peer_vtable ||
        !read(api,api.peer_type,type) || type!=api.peer_type_vtable) return false;
    out.value={api.peer_vtable,0,0,q.value[0],api.peer_type};
    api.component(q.value.data(),reinterpret_cast<const void*>(api.peer_type),out.value.data(),0);
    return out.value[0]==api.peer_vtable && out.value[3]==q.value[0] && out.value[4]==api.peer_type &&
        out.value[1] && pointer(out.value[2]) && read(api,out.value[2],out.id) && (out.id>>32);
}
bool pair(const TrafficSurfaceNativeApi& api,const Query& owner,const Query& selected,Peer& a,Peer& b) {
    return peer(api,owner,a) && peer(api,selected,b) && a.id==selected.value[0] && b.id==owner.value[0];
}
bool pair_current(const TrafficSurfaceNativeApi& api,const Query& owner,const Query& selected,const Peer& a,const Peer& b) {
    Peer after_a, after_b;
    return query_current(api,owner) && query_current(api,selected) && pair(api,owner,selected,after_a,after_b) &&
        a==after_a && b==after_b && query_current(api,owner) && query_current(api,selected);
}
bool pose_current(const TrafficSurfaceNativeApi& api,const TrafficSurfacePose& pose) {
    if (!api.seconds || !api.milliseconds) return false;
    const auto now=api.seconds(api.user);
    Query q; q.value=pose.query;
    return pose.valid && std::isfinite(now) && now>=pose.sampled_at && now-pose.sampled_at<=.1 &&
        current(api,pose.context,pose.selected) && query_current(api,q) && current(api,pose.context,pose.selected);
}
bool geometry_owner_current(const TrafficSurfaceNativeApi& api,const SurfaceGeometrySample& geometry) {
    alignas(16) const auto body=geometry.body;
    alignas(16) std::array<std::uint64_t,2> owner{};
    return geometry.body_owner && api.body_valid(&body) && api.body_entity(owner.data(),&body)==owner.data() &&
        owner[0]==geometry.body_owner;
}
}
TrafficSurfaceOwner traffic_surface_owner(const TrafficSurfaceNativeApi& api,const SurfaceGeometryContext& ctx,
                                          TrafficSurfaceSelection selected,SurfaceGeometryBody token) noexcept {
    try {
        alignas(16) const auto body=token;
        if (!current(api,ctx,selected) || body.world!=ctx.world || body.index==UINT32_MAX || !valid(api,selected.id)) return {};
        alignas(16) std::array<std::uint64_t,2> raw{};
        if (!api.body_valid(&body) || api.body_entity(raw.data(),&body)!=raw.data() || !valid(api,raw[0])) return {};
        const auto same_owner=[&] {
            alignas(16) std::array<std::uint64_t,2> after{};
            return api.body_valid(&body) && api.body_entity(after.data(),&body)==after.data() && after[0]==raw[0] &&
                valid(api,after[0]) && valid(api,selected.id) && current(api,ctx,selected);
        };
        std::array<Query,9> chain{};
        const auto accepted=[&](TrafficSurfaceOwner method,std::uint64_t mapping_id,std::size_t depth) {
            return same_owner() && (!mapping_id || (mapped(api,mapping_id)==selected.id && same_owner())) &&
                (!depth || chain_current(api,chain,depth)) ? method : TrafficSurfaceOwner::unavailable;
        };
        if (raw[0]==selected.id) return accepted(TrafficSurfaceOwner::direct,0,0);
        const bool raw_queried=query(api,raw[0],chain[0]);
        Query selected_query;
        const bool peer_available=api.peer_ready && raw_queried && query(api,selected.id,selected_query);
        const auto peer_contact=[&](std::size_t depth)->std::optional<TrafficSurfaceOwner> {
            if (!peer_available) return {};
            if (!query_current(api,chain[depth]) || !query_current(api,selected_query)) return TrafficSurfaceOwner::unavailable;
            Peer a,b;
            if (!pair(api,chain[depth],selected_query,a,b)) return {};
            if (!same_owner() || !chain_current(api,chain,depth) || !pair_current(api,chain[depth],selected_query,a,b) ||
                !same_owner() || !chain_current(api,chain,depth) || !pair_current(api,chain[depth],selected_query,a,b)) return TrafficSurfaceOwner::unavailable;
            return depth ? TrafficSurfaceOwner::parent_peer : TrafficSurfaceOwner::peer;
        };
        if (raw_queried) if (const auto p=peer_contact(0)) return *p;
        if (mapped(api,raw[0])==selected.id) return accepted(TrafficSurfaceOwner::mapped,raw[0],0);
        if (!raw_queried || !api.parent_ready || !api.parent) return {};
        for (std::size_t depth=1;depth<chain.size();++depth) {
            if (!query_current(api,chain[depth-1]) || !parent(api,chain[depth-1],chain[depth]) || !chain[depth].value[0] ||
                (chain[depth].value[0]>>32)!=(raw[0]>>32) || chain[depth].value[1]!=chain[0].value[1] || !identity(api,chain[depth])) return {};
            for (std::size_t i=0;i<depth;++i) if (chain[i].value[0]==chain[depth].value[0]) return {};
            if (chain[depth].value[0]==selected.id) return accepted(TrafficSurfaceOwner::parent_direct,0,depth);
            if (const auto p=peer_contact(depth)) return *p;
            if (mapped(api,chain[depth].value[0])==selected.id) return accepted(TrafficSurfaceOwner::parent_mapped,chain[depth].value[0],depth);
        }
    } catch (...) {}
    return {};
}
TrafficSurfacePose capture_traffic_surface_pose(const TrafficSurfaceNativeApi& api,const SurfaceGeometryContext& ctx,
                                               TrafficSurfaceSelection selected) noexcept {
    try {
        if (!current(api,ctx,selected)) return {};
        TrafficSurfacePose out;
        out.context=ctx; out.selected=selected; out.sampled_at=api.seconds(api.user); out.sampled_ms=api.milliseconds(api.user);
        if (!std::isfinite(out.sampled_at) || out.sampled_at<=0) return {};
        Query q, after;
        alignas(16) std::array<float,12> pose{};
        if (!query(api,selected.id,q) || api.pose(pose.data(),q.value.data())!=pose.data() ||
            !query(api,selected.id,after) || after!=q) return {};
        surface_motion_math::Transform transform;
        if (!surface_motion_math::transform(pose,transform) ||
            (pose[0]==1 && pose[1]==1 && pose[2]==1 && pose[4]==0 && pose[5]==0 && pose[6]==0 &&
             pose[7]==1 && pose[8]==0 && pose[9]==0 && pose[10]==0)) return {};
        out.query=q.value; out.transform=pose; out.valid=true;
        return pose_current(api,out) ? out : TrafficSurfacePose{};
    } catch (...) { return {}; }
}
void TrafficSurfaceProvider::invalidate() noexcept {
    anchor_={}; body_={}; local_point_={}; local_normal_={}; motion_.reset(); output_={}; serial_=queried_at_=0;
}
bool TrafficSurfaceProvider::bind(const TrafficSurfaceNativeApi& api,const TrafficSurfacePose& pose,const SurfaceGeometrySample& geometry) noexcept {
    try {
        if (!pose_current(api,pose) || !geometry.fresh_for(pose.context,api.milliseconds(api.user)) ||
            geometry.queried_at<pose.sampled_ms || geometry.queried_at-pose.sampled_ms>25 || !geometry_owner_current(api,geometry) ||
            traffic_surface_owner(api,pose.context,pose.selected,geometry.body)==TrafficSurfaceOwner::unavailable ||
            !geometry_owner_current(api,geometry)) { invalidate(); return false; }
        if (serial_ && body_==geometry.body && queried_at_==geometry.queried_at && anchor_.selected==pose.selected &&
            anchor_.query==pose.query && scope(anchor_.context,pose.context)) return true;
        surface_motion_math::Transform transform;
        if (!next_serial_ || !surface_motion_math::transform(pose.transform,transform)) { invalidate(); return false; }
        const auto point=surface_motion_math::local_point(geometry.point,transform);
        const auto normal=surface_motion_math::local_normal(geometry.query_normal,transform);
        if (!surface_geometry_detail::finite(point) || surface_geometry_detail::dot(normal,normal)<.99f) { invalidate(); return false; }
        invalidate(); anchor_=pose; body_=geometry.body; local_point_=point; local_normal_=normal; queried_at_=geometry.queried_at;
        serial_=next_serial_; next_serial_=next_serial_==UINT64_MAX ? 0 : next_serial_+1;
        (void)motion_.sample(pose.sampled_at,geometry.point);
        return true;
    } catch (...) { invalidate(); return false; }
}
TrafficSurfaceMotion TrafficSurfaceProvider::observe(const TrafficSurfaceNativeApi& api,const SurfaceGeometryContext& ctx,TrafficSurfaceSelection selected) noexcept {
    try {
        const auto pose=capture_traffic_surface_pose(api,ctx,selected);
        if (!serial_ || !pose.valid || !scope(anchor_.context,ctx) || anchor_.selected!=selected || anchor_.query!=pose.query ||
            traffic_surface_owner(api,ctx,selected,body_)==TrafficSurfaceOwner::unavailable) { invalidate(); return {}; }
        surface_motion_math::Transform transform;
        if (!surface_motion_math::transform(pose.transform,transform)) { invalidate(); return {}; }
        const auto point=surface_motion_math::world_point(local_point_,transform);
        const auto contact=motion_.sample(pose.sampled_at,point);
        if (!contact.valid) { output_={}; return {}; }
        if (!pose_current(api,pose) || traffic_surface_owner(api,ctx,selected,body_)==TrafficSurfaceOwner::unavailable) { invalidate(); return {}; }
        if (contact.sampled_at!=pose.sampled_at) return snapshot(ctx,selected,api.seconds(api.user),api.milliseconds(api.user));
        output_={ctx,selected,body_,local_point_,contact.point,surface_motion_math::world_normal(local_normal_,transform),
            contact.velocity,serial_,queried_at_,pose.sampled_ms,contact.sampled_at,pose.sampled_at,true};
        return output_;
    } catch (...) { invalidate(); return {}; }
}
TrafficSurfaceMotion TrafficSurfaceProvider::snapshot(const SurfaceGeometryContext& ctx,TrafficSurfaceSelection selected,double now,std::uint64_t now_ms) const noexcept {
    return serial_ && output_.valid && selected==anchor_.selected && scope(anchor_.context,ctx) && ctx.fresh(now_ms) &&
        output_.context.native.fresh(now_ms) && now_ms>=output_.observed_ms && now_ms-output_.observed_ms<=InputTiming::native_sample_lease_ms &&
        std::isfinite(now) && now>=output_.sampled_at && now-output_.sampled_at<=.1 ? output_ : TrafficSurfaceMotion{};
}
}
