#include "Extension/ArcadeMode/arcade_surface_geometry_native.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dingosdk;
using namespace dingosdk::arcade_mode;
namespace dingosdk { bool surface_geometry_test_read(std::uintptr_t,void*,std::size_t) noexcept; }
namespace {
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
struct Fixture {
    static inline Fixture* running{};
    SurfaceGeometryContext live{{1,2,3,4,5},{true,true,{1,2,3},1,1000,false,false,1,1},6,7,1,90,1,SurfaceGeometryPhase::park};
    std::uint64_t time{1000};
    unsigned calls{}, pops{}, frees{}, entities{}, first{3}, delay{}, clock_step{};
    float sx{}, sz{};
    bool roof{}, unknown{}, seam{}, fold{}, edge{}, miss{}, bad_fraction{}, bad_data{}, wrong_world{}, overflow{},
        bad_body{}, changed_owner{}, changed_context{}, throw_query{}, no_context{}, many_self{}, self{}, read_fail{},
        throw_pop{}, throw_free{};
    std::uint32_t generation{9};
    std::array<float,8> fractions{};
    std::array<std::uintptr_t,2> data{};
    std::array<std::uintptr_t,2> table{};
    std::uintptr_t allocator{};
    float last_x{};
    bool self_hit{};
    bool wall{}, upper_gap{}, lower_gap{};
    bool allow_world_owner{}, reject_world_owner{}, changing_world_owner{};
    unsigned world_owner_calls{};
    unsigned reject_owner_at{},reject_shape_at{},shape_checks{},context_failure_after_owners{};
    SurfaceGeometryWorldOwner world_owner{{7,70,50331648},77,69,50331648,0,1234};
    SurfaceGeometryVector wall_normal{0,0,1}, wall_point{0,1.5f,0};
    SurfaceGeometryVector contact_velocity{};
    float contact_seconds{1.f/30.f};
    bool explicit_contact_motion{};
    std::array<SurfaceGeometrySegment,5> wall_segments{};
    Fixture() { running=this; table[1]=reinterpret_cast<std::uintptr_t>(&pop); allocator=reinterpret_cast<std::uintptr_t>(table.data()); }
    static std::uint64_t now(void* user) noexcept {
        auto& f=*static_cast<Fixture*>(user);const auto result=f.time;f.time+=f.clock_step;return result;
    }
    static bool current(void* user,const SurfaceGeometryContext& context) noexcept {
        const auto& f=*static_cast<Fixture*>(user); return !f.no_context && context==f.live &&
            (!f.context_failure_after_owners || f.world_owner_calls<f.context_failure_after_owners);
    }
    static std::uintptr_t world(std::uintptr_t context) { return context==running->live.client_context ? running->live.world : 0; }
    static void pop(std::uintptr_t,void*) { ++running->pops; if (running->throw_pop) throw std::runtime_error("pop failed"); }
    static void release(void*,std::uintptr_t) {
        check(running->pops==running->frees+1,"scope must pop before allocation release"); ++running->frees;
        if (running->throw_free) throw std::runtime_error("release failed");
    }
    static bool valid(const void* ptr) {
        const auto& b=*static_cast<const SurfaceGeometryBody*>(ptr);
        const auto call=++running->shape_checks;
        return call!=running->reject_shape_at && !running->bad_body && b.world==running->live.world && b.index!=UINT32_MAX && b.generation==running->generation;
    }
    static void* entity(void* out,const void* ptr) {
        auto& f=*running;
        const auto& body=*static_cast<const SurfaceGeometryBody*>(ptr);
        auto& value=*static_cast<std::array<std::uint64_t,2>*>(out);
        value={body.index>=100 ? f.live.self_entity : f.unknown ? 0u : 20u,0};
        if (f.changed_owner && ++f.entities%2==0) ++value[0];
        return out;
    }
    static void* hit_body(const void* hit,void* out) {
        auto& f=*running;
        const auto& descriptor=*static_cast<const std::array<std::uint64_t,3>*>(hit);
        check(descriptor[0]==f.live.world && descriptor[1]==f.first,"complete nearest hit descriptor retained");
        auto& b=*static_cast<SurfaceGeometryBody*>(out);
        b={f.live.world, f.self_hit ? static_cast<std::uint32_t>(100+f.calls) : f.seam && f.last_x>0 ? 2u : 1u,f.generation};
        return out;
    }
    static bool static_owner(void* user,const SurfaceGeometryBody&,SurfaceGeometryWorldOwner& owner) noexcept {
        auto& f=*static_cast<Fixture*>(user);
        const auto call=++f.world_owner_calls;
        if(f.reject_world_owner || call==f.reject_owner_at)return false;
        owner=f.world_owner;
        if(f.changing_world_owner)owner.generation+=call;
        return true;
    }
    static void* ray(std::uintptr_t world_value,editor::NativeSurfaceResult* result,const editor::NativeSurfaceRay* ray_value,const char*) {
        auto& f=*running; ++f.calls; f.time+=f.delay;
        result->world=f.wrong_world ? world_value+1 : world_value;
        result->scope[2]=1; result->allocator=1;
        result->data=reinterpret_cast<std::uintptr_t>(f.data.data());
        result->first=f.overflow ? UINT32_MAX : f.first; result->count=f.miss ? 0u : 2u;
        f.last_x=ray_value->start[0];
        const float x=ray_value->start[0], z=ray_value->start[2];
        const float height=f.roof ? 1.f : f.fold ? std::abs(x)*2.f : f.sx*x+f.sz*z;
        f.self_hit=f.many_self || (f.self && !ray_value->ignored_bodies[0]);
        if (f.self && ray_value->ignored_bodies[0]) {
            const auto* b=reinterpret_cast<const SurfaceGeometryBody*>(ray_value->ignored_bodies[0]);
            check(b->world==f.live.world && b->generation==f.generation && b->index>=100,"native exclusions retain full body token");
        }
        if (f.wall) {
            SurfaceGeometryVector start{}, end{};
            for (unsigned axis=0;axis<3;++axis) { start[axis]=ray_value->start[axis]; end[axis]=ray_value->end[axis]; }
            if(f.calls<=f.wall_segments.size())f.wall_segments[f.calls-1]={start,end};
            const float distance=surface_geometry_detail::dot(surface_geometry_detail::sub(start,f.wall_point),f.wall_normal);
            const float length=surface_geometry_detail::dot(surface_geometry_detail::sub(start,end),f.wall_normal);
            f.fractions[f.first]=f.self_hit ? .1f : distance/length;
            if(f.fractions[f.first]<0 || f.fractions[f.first]>1)result->count=0;
            if ((f.upper_gap && start[1]>f.wall_point[1]+.1f) ||
                (f.lower_gap && start[1]<f.wall_point[1]-.1f)) result->count=0;
        } else f.fractions[f.first]=f.self_hit ? .1f : (ray_value->start[1]-height)/(ray_value->start[1]-ray_value->end[1]);
        f.fractions[f.first+1]=f.wall ? 1.f : .9f;
        if (f.bad_fraction) f.fractions[f.first]=std::numeric_limits<float>::quiet_NaN();
        f.data[1]=f.bad_data ? 0 : reinterpret_cast<std::uintptr_t>(f.fractions.data());
        if (f.edge && x>0) result->count=0;
        if (f.changed_context) ++f.live.world_generation;
        if (f.throw_query) throw std::runtime_error("partial result");
        return result;
    }
    SurfaceGeometryNativeApi api() {
        SurfaceGeometryNativeApi api;
        api.base=live.owner[0];
        api.surface={&world,&ray,[]() -> std::uintptr_t {return reinterpret_cast<std::uintptr_t>(&running->allocator);},&release,true};
        api.picking.hit_body=&hit_body; api.picking.body_valid=&valid; api.picking.body_entity=&entity; api.picking.ready=true;
        api.user=this; api.now=&now; api.current=&current; api.read_bytes=&surface_geometry_test_read;
        if(allow_world_owner)api.static_world_owner=&static_owner;
        api.current_failure=[](void*)noexcept{return "fixture_frame_lock_busy";};
        return api;
    }
    static SurfaceGeometryRequest request() {
        SurfaceGeometryRequest request;
        constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.25f,0},{.25f,0},{0,-.25f},{0,.25f}}};
        for (std::size_t i=0;i<5;++i) request.segments[i]={{offsets[i][0],3,offsets[i][1]},{offsets[i][0],-3,offsets[i][1]}};
        return request;
    }
    SurfaceGeometrySample query() {
        const auto context=live;
        auto result=query_surface_geometry(api(),context,request());
        check(calls==pops && calls==frees,"all result scopes released on every path");
        return result;
    }
    SurfaceGeometrySample query_wall(SurfaceGeometryVector point,SurfaceGeometryVector normal,
                                     SurfaceGeometryBody body={7,1,9}) {
        wall=true;
        const auto context=live;
        const SurfaceGeometryVector velocity=explicit_contact_motion?contact_velocity:
            SurfaceGeometryVector{-normal[0],-normal[1],-normal[2]};
        const auto result=query_wall_geometry(api(),context,body,point,normal,velocity,contact_seconds);
        check(calls==pops && calls==frees,"wall query releases every native result scope");
        return result;
    }
    SurfaceGeometrySample query_wall() {return query_wall(wall_point,wall_normal);}
};
}
namespace dingosdk {
bool surface_geometry_test_read(std::uintptr_t at,void* out,std::size_t size) noexcept {
    const auto* f=Fixture::running;
    const auto fractions=reinterpret_cast<std::uintptr_t>(f->fractions.data());
    if (f->read_fail && at>=fractions && at-fractions<sizeof f->fractions) return false;
    const auto copy=[&](const void* begin,std::size_t length) {
        const auto base=reinterpret_cast<std::uintptr_t>(begin);
        if (at<base || size>length || at-base>length-size) return false;
        std::memcpy(out,reinterpret_cast<const void*>(at),size); return true;
    };
    return copy(f->fractions.data(),sizeof f->fractions) || copy(f->data.data(),sizeof f->data) ||
        copy(f->table.data(),sizeof f->table) || copy(&f->allocator,sizeof f->allocator);
}
}
int main() {
    for(bool wall:{false,true}) {
        Fixture f;f.clock_step=1;
        const auto api=f.api();const auto admitted_at=api.now(api.user);
        const auto sample=wall?f.query_wall():f.query();
        check(sample.valid&&sample.queried_at>admitted_at,
            "native query may successfully stamp a later millisecond than its caller's admission clock");
        check(!sample.fresh_for(f.live,admitted_at),
            "reusing the pre-query clock reproduces the future-query rejection");
        const auto original_query_time=sample.queried_at;
        const auto original_frame_time=sample.context.native.sampled_at;
        const auto validated_at=api.now(api.user);
        check(sample.fresh_for(f.live,validated_at)&&revalidate_surface_geometry(api,f.live,sample),
            "actual post-query validation time admits the unchanged fresh native geometry");
        check(sample.queried_at==original_query_time&&sample.context.native.sampled_at==original_frame_time,
            "validation never renews either native query or original frame timestamp");
        f.clock_step=0;f.time=original_query_time-1;
        check(!sample.fresh_for(f.live,api.now(api.user)),"clock rollback still rejects future observations");
        f.time=original_frame_time+InputTiming::native_sample_lease_ms+1;
        check(!sample.fresh_for(f.live,api.now(api.user))&&!revalidate_surface_geometry(api,f.live,sample),
            "post-query publication cannot extend the original frame lease");
    }
    {
        Fixture f;f.clock_step=20;
        check(!f.query().valid,"a query that outlives original native authority fails rather than refreshing it");
    }
    {
        Fixture f; auto sample=f.query();
        check(sample.valid && f.calls==5,"flat floor requires five real intersections");
        check(sample.body==SurfaceGeometryBody{7,1,9} && sample.body_owner==20,"full body identity and exact owner");
        check(sample.point[1]==0 && sample.query_normal==SurfaceGeometryVector{0,1,0},"flat floor normal");
        check(sample.queried_at==1000 && sample.context==f.live,"original query timestamp and generations");
        check(revalidate_surface_geometry(f.api(),f.live,sample),"fresh callback revalidation");
        f.time=1150; check(revalidate_surface_geometry(f.api(),f.live,sample),"existing inclusive150ms lease");
        f.time=1151; check(!revalidate_surface_geometry(f.api(),f.live,sample),"stale query cannot renew");
        f.live.native.sampled_at=1151; ++f.live.native.physics_step;
        check(!revalidate_surface_geometry(f.api(),f.live,sample),"fresh physics cannot renew an old query");
        check(sample.queried_at==1000,"revalidation retains original query clock");
    }
    for (const auto slope : {std::array<float,2>{.5f,0},std::array<float,2>{0,-1},std::array<float,2>{2,1}}) {
        Fixture f; f.sx=slope[0]; f.sz=slope[1]; auto sample=f.query();
        check(sample.valid,"sloped ground is available, no vertical-face rejection copied");
        auto normal=SurfaceGeometryVector{-f.sx,1,-f.sz};
        check(surface_geometry_detail::unit(normal),"slope fixture unit vector");
        check(surface_geometry_detail::dot(normal,sample.query_normal)>.9999f,"slope independently computed from intersections");
    }
    { Fixture f; f.roof=true; const auto sample=f.query(); check(sample.valid && std::abs(sample.point[1]-1)<.00001f,"nearer roof wins over lower ground"); }
    for (unsigned failure=0;failure<16;++failure) {
        Fixture f;
        switch(failure) {
        case 0:f.miss=true;break; case 1:f.unknown=true;break; case 2:f.seam=true;break;
        case 3:f.fold=true;break; case 4:f.edge=true;break; case 5:f.bad_fraction=true;break;
        case 6:f.bad_data=true;break; case 7:f.wrong_world=true;break; case 8:f.overflow=true;break;
        case 9:f.bad_body=true;break; case 10:f.changed_owner=true;break;
        case 11:f.changed_context=true;break; case 12:f.throw_query=true;break;
        case 13:f.read_fail=true;break; case 14:f.throw_pop=true;break; case 15:f.throw_free=true;break;
        }
        check(!f.query().valid,"miss/unknown/seam/fold/edge/corruption/changed context cannot fabricate plane");
    }
    { Fixture f; f.self=true; check(f.query().valid && f.calls==6,"fresh exact self body excluded natively once"); }
    { Fixture f; f.many_self=true; check(!f.query().valid && f.calls==9,"self exclusion bounded to eight exact shapes"); }
    {
        Fixture f;f.unknown=true;f.allow_world_owner=true;
        const auto sample=f.query();
        check(sample.valid && sample.body_owner==0 && sample.world_owner==f.world_owner,
            "verified non-ECS world geometry retains its full owner stamp without inventing an entity ID");
        check(revalidate_surface_geometry(f.api(),f.live,sample),"unchanged static world owner revalidates");
        for(unsigned change=0;change<6;++change) {
            const auto original=f.world_owner;
            switch(change) {
            case 0:++f.world_owner.index;break;case 1:++f.world_owner.generation;break;
            case 2:++f.world_owner.kind;break;case 3:++f.world_owner.value;break;
            case 4:++f.world_owner.body.generation;break;case 5:++f.world_owner.store;break;
            }
            check(!revalidate_surface_geometry(f.api(),f.live,sample),"world owner/body incarnation changes revoke geometry");
            f.world_owner=original;
        }
        auto ordinary=f.api();ordinary.static_world_owner=nullptr;
        check(!revalidate_surface_geometry(ordinary,f.live,sample),"world-owner opt-in cannot escape to ordinary surface consumers");
    }
    { Fixture f;f.unknown=true;f.allow_world_owner=true;f.reject_world_owner=true;
      check(!f.query().valid,"unknown or moving owner remains rejected"); }
    { Fixture f;f.unknown=true;f.allow_world_owner=true;f.changing_world_owner=true;
      check(!f.query().valid,"world owner changing during the query is rejected"); }
    { Fixture f;f.unknown=true;f.allow_world_owner=true;
      check(f.query_wall().valid,"verified non-ECS wall supports the same broad geometry query"); }
    for(unsigned failure=0;failure<5;++failure) {
        Fixture f;f.unknown=true;f.allow_world_owner=true;
        const char* expected{};
        switch(failure) {
        case 0:f.reject_shape_at=3;expected="shape_changed_after_entity";break;
        case 1:f.context_failure_after_owners=1;expected="owner_context_changed";break;
        case 2:f.reject_owner_at=2;expected="static_owner_reader_again";break;
        case 3:f.changing_world_owner=true;expected="static_owner_stamp_changed";break;
        case 4:f.context_failure_after_owners=2;expected="static_owner_context_changed";break;
        }
        SurfaceGeometryTrace trace;
        const auto sample=query_surface_geometry(f.api(),f.live,f.request(),&trace);
        check(!sample.valid && std::strcmp(trace.reason,expected)==0,"diagnostic distinguishes shape, owner stamp, owner reader and context failures");
        check(f.calls==1&&f.pops==1&&f.frees==1,"diagnostic never retries query or changes cleanup");
        if(failure==1||failure==4)check(std::strcmp(trace.context_reason,"fixture_frame_lock_busy")==0,"callback failure copied without another context read");
        else check(std::strcmp(trace.context_reason,"fixture_frame_lock_busy")!=0,"owner failure does not borrow stale callback failure reason");
    }
    {Fixture f;f.time=1151;SurfaceGeometryTrace trace;
     check(!query_surface_geometry(f.api(),f.live,f.request(),&trace).valid&&
         std::strcmp(trace.context_reason,"query_context_stale")==0&&f.calls==0,"wrapper freshness rejection is not mislabeled as callback failure");}
    {Fixture f;auto api=f.api();api.surface.world=[](std::uintptr_t){return std::uintptr_t{8};};SurfaceGeometryTrace trace;
     check(!query_surface_geometry(api,f.live,f.request(),&trace).valid&&
         std::strcmp(trace.context_reason,"query_world_changed")==0&&f.calls==0,"wrapper world rejection is not mislabeled as callback failure");}
    for (unsigned failure=0;failure<12;++failure) {
        Fixture f; auto sample=f.query();
        switch(failure) {
        case 0:++f.live.native.generations.owner;break; case 1:++f.live.world_generation;break;
        case 2:++f.generation;break; case 3:++f.live.self_generation;break;
        case 4:++f.live.native.generations.contact;break; case 5:++f.live.native.airborne_episode;break;
        case 6:++f.live.native.air_departure;break; case 7:f.no_context=true;break;
        case 8:++f.live.world;break; case 9:++f.live.owner[1];break;
        case 10:++f.live.native.generations.action;break; case 11:++f.live.self_entity;break;
        }
        check(!revalidate_surface_geometry(f.api(),f.live,sample),"world/owner/body/self/flight/contact replacement revokes");
    }
    for (unsigned failure=0;failure<5;++failure) {
        Fixture f; auto request=f.request();
        if (failure==0) for(auto& s:request.segments) s.start[2]=s.end[2]=0;
        if (failure==1) request.segments[2].start[0]=std::numeric_limits<float>::infinity();
        if (failure==2) std::swap(request.segments[0].start,request.segments[0].end);
        if (failure==3) request.segments[0].end[1]=-30;
        if (failure==4) request.segments[4].start[2]=request.segments[4].end[2]=3;
        check(!query_surface_geometry(f.api(),f.live,request).valid,"collinear/nonfinite/reversed/unbounded rays unavailable");
        check(f.calls==f.pops && f.calls==f.frees,"invalid footprint releases all allocated scopes");
    }
    { Fixture f; f.delay=31; check(!f.query().valid,"query work cannot renew native/query lease"); }
    { Fixture f; f.delay=10; const auto sample=f.query(); check(sample.valid && sample.queried_at==1000 && f.time==1050,"all query work spends original query lease"); }
    { Fixture f; f.live.phase=SurfaceGeometryPhase::client; check(f.query().valid,"qualified client callback also supported"); }
    { Fixture f; f.no_context=true; check(!f.query().valid && f.calls==0,"no calls from unqualified callback"); }
    { Fixture f; f.live.phase=SurfaceGeometryPhase::unavailable; check(!f.query().valid && f.calls==0,"wrong phase fails closed"); }
    { Fixture f; auto api=f.api(); api.surface.ready=false; check(!query_surface_geometry(api,f.live,f.request()).valid && f.calls==0,"unknown collision API unavailable"); }
    { Fixture f; auto api=f.api(); ++api.base; check(!query_surface_geometry(api,f.live,f.request()).valid && f.calls==0,"API build owner mismatch is not callable"); }
    // Geometry corroborates an independently supplied contact. These fixtures
    // exercise actual production casts; a query hit is never contact evidence.
    for (const auto normal : {SurfaceGeometryVector{0,0,1},SurfaceGeometryVector{1,0,0},
                              SurfaceGeometryVector{-.6f,0,.8f},SurfaceGeometryVector{0,0,-1}}) {
        Fixture f; f.wall_normal=normal;
        const auto sample=f.query_wall();
        check(sample.valid && f.calls==5,"vertical walls at head-on and oblique headings need all five hits");
        check(sample.body==SurfaceGeometryBody{7,1,9} && sample.body_owner==20,"wall keeps exact collision body and owner");
        check(surface_geometry_detail::dot(sample.query_normal,normal)>.9999f,"measured wall normal retains contact orientation");
        check(sample.point==f.wall_point,"wall center agrees with real contact point");
        check(sample.context==f.live && sample.queried_at==1000,"wall preserves original context and clock");
        check(revalidate_surface_geometry(f.api(),f.live,sample),"wall can revalidate through existing native callback");
    }
    for (unsigned gap=0;gap<3;++gap) {
        Fixture f; f.upper_gap=gap!=1; f.lower_gap=gap!=0;
        check(!f.query_wall().valid,"thin rail or missing upper/lower face cannot corroborate broad wall");
    }
    { Fixture f; f.edge=true; check(!f.query_wall().valid,"wall edge missing a side sample rejects"); }
    { Fixture f; f.seam=true; check(!f.query_wall().valid,"wall seam cannot join different collision bodies"); }
    // Actual 2ce4 hardware: genuine AIR solver points precede the exact same
    // shape's measured plane. Reach is unchanged; only patch origin changes.
    { Fixture f;f.wall_point={130.50424f,1.556511f,-136.594f};
      const auto sample=f.query_wall({130.50424f,1.556511f,-136.49689f},f.wall_normal);
      check(sample.valid&&f.calls==5,"measured 97mm solver-to-plane offset admits same-shape broad patch");
      check(std::abs(sample.point[2]-f.wall_point[2])<.0001f,"published center is original ray plane, never synthetic solver plane"); }
    { Fixture f;f.wall_point={130.48776f,1.5352534f,-136.594f};
      check(!f.query_wall({130.48776f,1.5352534f,-136.46881f},f.wall_normal).valid&&f.calls==1,
          "measured earlier contact outside existing 100mm bracket still rejects"); }
    { Fixture f;f.wall_normal={-.6f,0,.8f};auto point=f.wall_point;
      for(unsigned axis=0;axis<3;++axis)point[axis]+=.09710693f*f.wall_normal[axis];
      const auto sample=f.query_wall(point,f.wall_normal);
      check(sample.valid&&f.calls==5,"oblique same-shape center establishes plane without wider reach");
      check(std::abs(surface_geometry_detail::dot(surface_geometry_detail::sub(sample.point,f.wall_point),f.wall_normal))<.00001f,
          "oblique patch remains on the measured plane"); }
    { Fixture f;auto point=f.wall_point;point[2]+=.1001f;
      check(!f.query_wall(point,f.wall_normal).valid,"nearby plane beyond original reach remains unavailable"); }
    // Actual ea89 AIR contact: the authenticated solver point is 324mm in
    // front of the same shape's plane; its original 30Hz step travels 346mm
    // inward. This query does not infer a collision from that travel.
    { Fixture f;f.wall_point={129.46672f,1.5966883f,-136.594f};
      f.explicit_contact_motion=true;f.contact_velocity={-4.8446026f,-1.2898192f,-10.376561f};
      f.contact_seconds=.03333333f;
      const SurfaceGeometryVector point{129.46672f,1.5966883f,-136.26971f};
      const auto sample=f.query_wall(point,f.wall_normal);
      check(sample.valid&&f.calls==5,"measured 324mm gap is corroborated inside original 346mm inward step");
      check(std::abs(sample.point[2]-f.wall_point[2])<.0001f,"native ray supplies actual plane, not velocity extrapolation");
      check(std::abs(f.wall_segments[0].end[2]-(point[2]-10.376561f*f.contact_seconds))<.00002f,
          "first ray reaches precisely measured normal travel without additive margin");
      check(std::abs(f.wall_segments[0].start[2]-(point[2]+.1f))<.00002f,"outward center bracket is unchanged");
      for(unsigned i=1;i<5;++i) {
          check(std::abs(f.wall_segments[i].start[2]-(f.wall_point[2]+.1f))<.00002f&&
              std::abs(f.wall_segments[i].end[2]-(f.wall_point[2]-.1f))<.00002f,
              "four broad-plane rays retain original tight bracket");
      }
    }
    { Fixture f;f.wall_normal={-.6f,0,.8f};f.explicit_contact_motion=true;
      // 10m/s toward the normal and 20m/s along the tangent.
      f.contact_velocity={22,0,4};auto point=f.wall_point;
      for(unsigned axis=0;axis<3;++axis)point[axis]+=.32f*f.wall_normal[axis];
      const auto sample=f.query_wall(point,f.wall_normal);
      check(sample.valid&&f.calls==5,"oblique inward travel corroborates the same measured plane");
      const float reach=-surface_geometry_detail::dot(surface_geometry_detail::sub(f.wall_segments[0].end,point),f.wall_normal);
      check(std::abs(reach-10.f*f.contact_seconds)<.00001f,"tangential speed never enlarges normal search");
    }
    for(float gap:{.346f,.40f}) {
        Fixture f;f.explicit_contact_motion=true;f.contact_velocity={0,0,-10.376561f};
        f.contact_seconds=.03333333f;auto point=f.wall_point;point[2]+=gap;
        check(!f.query_wall(point,f.wall_normal).valid&&f.calls==1,
            "wall beyond original inward step rejects without ray retry or margin expansion");
    }
    for(float seconds:{0.f,-.01f,.0009f,.1001f,std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::quiet_NaN()}) {
        Fixture f;f.contact_seconds=seconds;
        check(!f.query_wall().valid&&f.calls==0,"invalid native step rejects before any native query");
    }
    for(auto velocity:{SurfaceGeometryVector{0,0,0},SurfaceGeometryVector{0,0,1},
                       SurfaceGeometryVector{1,0,0},SurfaceGeometryVector{0,0,-1000},
                       SurfaceGeometryVector{0,0,-std::numeric_limits<float>::infinity()},
                       SurfaceGeometryVector{std::numeric_limits<float>::quiet_NaN(),0,-10}}) {
        Fixture f;f.explicit_contact_motion=true;f.contact_velocity=velocity;
        check(!f.query_wall().valid&&f.calls==0,"non-inward, invalid or excessive query motion rejects before native calls");
    }
    { Fixture f;f.explicit_contact_motion=true;f.contact_velocity={0,0,-10.376561f};
      auto point=f.wall_point;point[2]+=.32429f;
      check(!f.query_wall(point,f.wall_normal,{7,2,9}).valid&&f.calls==1,
          "extended center search cannot substitute another shape"); }
    { Fixture f;f.explicit_contact_motion=true;f.contact_velocity={0,0,-10.376561f};
      f.upper_gap=true;auto point=f.wall_point;point[2]+=.32429f;
      check(!f.query_wall(point,f.wall_normal).valid,"native-step center reach still rejects missing broad wall geometry"); }
    { Fixture f; f.wall_normal={.02f,0,std::sqrt(1.f-.02f*.02f)};
      check(!f.query_wall(f.wall_point,{0,0,1}).valid,"contact normal and intersected plane must agree across broad patch"); }
    for (const auto normal : {SurfaceGeometryVector{0,1,0},SurfaceGeometryVector{0,-1,0},
                              SurfaceGeometryVector{0,.6f,.8f},SurfaceGeometryVector{0,0,.9f},
                              SurfaceGeometryVector{0,0,0},
                              SurfaceGeometryVector{0,0,std::numeric_limits<float>::infinity()},
                              SurfaceGeometryVector{0,0,std::numeric_limits<float>::quiet_NaN()}}) {
        Fixture f;
        check(!f.query_wall(f.wall_point,normal).valid && f.calls==0,"floor/ceiling/ramp/nonunit/invalid normal is rejected before native calls");
    }
    { Fixture f; auto point=f.wall_point; point[0]=std::numeric_limits<float>::quiet_NaN();
      check(!f.query_wall(point,f.wall_normal).valid && f.calls==0,"invalid real contact point is rejected before native calls"); }
    for (unsigned failure=0;failure<14;++failure) {
        Fixture f;
        switch(failure) {
        case 0:f.unknown=true;break; case 1:f.changed_owner=true;break;
        case 2:f.changed_context=true;break; case 3:f.bad_body=true;break;
        case 4:f.no_context=true;break; case 5:f.live.world_generation=0;break;
        case 6:f.live.owner[2]=0;break; case 7:f.time=1151;break;
        case 8:f.bad_fraction=true;break; case 9:f.read_fail=true;break;
        case 10:f.throw_query=true;break; case 11:f.throw_pop=true;break;
        case 12:f.throw_free=true;break; case 13:f.live.phase=SurfaceGeometryPhase::unavailable;break;
        }
        check(!f.query_wall().valid,"wall requires live context, stable owner/body, readable hits and complete cleanup");
    }
    for (const auto body : {SurfaceGeometryBody{8,1,9},SurfaceGeometryBody{7,2,9},
                           SurfaceGeometryBody{7,1,10},SurfaceGeometryBody{7,UINT32_MAX,9}}) {
        Fixture f;
        check(!f.query_wall(f.wall_point,f.wall_normal,body).valid,"wall geometry cannot substitute a different contacted body incarnation");
    }
    { Fixture f; f.delay=31; check(!f.query_wall().valid,"wall work cannot renew an expired native lease"); }
    { Fixture f; f.delay=10; const auto sample=f.query_wall();
      check(sample.valid && sample.queried_at==1000 && f.time==1050,"wall casts spend original query clock"); }
    { Fixture f; f.self=true; check(f.query_wall().valid && f.calls==6,"wall excludes exact local skater body before world contact"); }
    { Fixture f; f.many_self=true; check(!f.query_wall().valid && f.calls==9,"wall self-body exclusions remain bounded"); }
    std::cout<<checks<<" surface geometry/native-query contract checks passed\n";
}
