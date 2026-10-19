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
    unsigned calls{}, pops{}, frees{}, entities{}, first{3}, delay{};
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
    Fixture() { running=this; table[1]=reinterpret_cast<std::uintptr_t>(&pop); allocator=reinterpret_cast<std::uintptr_t>(table.data()); }
    static std::uint64_t now(void* user) noexcept { return static_cast<Fixture*>(user)->time; }
    static bool current(void* user,const SurfaceGeometryContext& context) noexcept {
        const auto& f=*static_cast<Fixture*>(user); return !f.no_context && context==f.live;
    }
    static std::uintptr_t world(std::uintptr_t context) { return context==running->live.client_context ? running->live.world : 0; }
    static void pop(std::uintptr_t,void*) { ++running->pops; if (running->throw_pop) throw std::runtime_error("pop failed"); }
    static void release(void*,std::uintptr_t) {
        check(running->pops==running->frees+1,"scope must pop before allocation release"); ++running->frees;
        if (running->throw_free) throw std::runtime_error("release failed");
    }
    static bool valid(const void* ptr) {
        const auto& b=*static_cast<const SurfaceGeometryBody*>(ptr);
        return !running->bad_body && b.world==running->live.world && b.index!=UINT32_MAX && b.generation==running->generation;
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
        f.fractions[f.first]=f.self_hit ? .1f : (ray_value->start[1]-height)/(ray_value->start[1]-ray_value->end[1]);
        f.fractions[f.first+1]=.9f;
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
    std::cout<<checks<<" surface geometry/native-query contract checks passed\n";
}
