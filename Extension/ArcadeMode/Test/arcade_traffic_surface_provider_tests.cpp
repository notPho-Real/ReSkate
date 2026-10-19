#include "Extension/ArcadeMode/arcade_traffic_surface_provider.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value){std::cerr<<message<<'\n';std::exit(1);}}
struct Fixture {
    static inline Fixture* active{};
    static constexpr std::uint64_t root=(2ULL<<32)|101, child=(1ULL<<32)|202, ancestor=(1ULL<<32)|303;
    SurfaceGeometryContext ctx{{1,2,3,4,5},{true,true,{1,2,3},1,1000,false,false,1,1},6,7,1,90,1,SurfaceGeometryPhase::park};
    TrafficSurfaceSelection selected{root,1,0,true};
    std::uint64_t raw{root};
    double now{1};std::uint64_t ms{1000};
    std::array<float,12> transform{1,1,1,0,0,0,0,1,10,0,0,0};
    std::uint32_t body_generation{9};std::uint64_t record_version{};
    unsigned queries{},poses{},body_calls{},peer_calls{};
    bool phase{true}, member{true}, removed{}, mismatch{}, query_changed{}, record_bad{}, body_changed{}, owner_changed{},
        mapping{}, parent_mapping{}, peers{}, parents{}, parent_direct{}, parent_peer{}, cycle{}, peer_changed{}, throw_pose{}, read_failed{};
    Fixture(){active=this;}
    static std::uintptr_t context(std::uint64_t id){return 0x10000+static_cast<std::uintptr_t>(id>>32)*0x1000;}
    static std::uintptr_t record(std::uint64_t id){return 0x20000+static_cast<std::uintptr_t>(static_cast<std::uint32_t>(id))*0x10+active->record_version*0x10000;}
    static bool valid(const void* ref){const auto id=*static_cast<const std::uint64_t*>(ref);return !active->removed && (id==active->selected.id || id==child || id==ancestor);}
    static void* query(void* out,std::uint64_t id){
        auto& f=*active;++f.queries;auto& q=*static_cast<std::array<std::uint64_t,3>*>(out);
        q={id,context(id),record(id)+(f.query_changed && f.queries%2==0?8u:0u)};return out;
    }
    static float* pose(float* out,const void*){++active->poses;if(active->throw_pose)throw std::runtime_error("pose unavailable");std::copy(active->transform.begin(),active->transform.end(),out);return out;}
    static bool body_valid(const void* p){const auto& b=*static_cast<const SurfaceGeometryBody*>(p);return !active->body_changed && b==SurfaceGeometryBody{active->ctx.world,1,active->body_generation};}
    static void* body_entity(void* out,const void*){auto& f=*active;++f.body_calls;*static_cast<std::array<std::uint64_t,2>*>(out)={f.raw+(f.owner_changed && f.body_calls%2==0?1u:0u),0};return out;}
    static void* parent(const void* in,void* out){
        const auto id=*static_cast<const std::uint64_t*>(in);auto& q=*static_cast<std::array<std::uint64_t,3>*>(out);std::uint64_t next{};
        if(active->cycle)next=id;else if(id==child)next=active->parent_direct?active->selected.id:ancestor;
        q={next,next?context(next):0,next?record(next):0};return out;
    }
    static std::uintptr_t manager(){return 0x60000;}
    static void* map(std::uintptr_t,void* out,std::uint64_t id,int direction){
        check(direction==1,"only published client-to-authoritative mapping direction");
        *static_cast<std::uint64_t*>(out)=active->mapping && ((id==child && !active->parent_mapping) || id==ancestor)?active->selected.id:0;return out;
    }
    static std::uint64_t component(const void* in,const void*,void* out,std::uint32_t){
        auto& f=*active;++f.peer_calls;const auto id=*static_cast<const std::uint64_t*>(in);auto* value=static_cast<std::uintptr_t*>(out);
        const auto owner=f.parent_peer?ancestor:child;
        if(!f.peers || (id!=owner && id!=f.selected.id))return 0;
        value[1]=f.peer_changed?f.peer_calls:1;value[2]=0x50000+static_cast<std::uint32_t>(id)*0x10;return 0;
    }
    template<class T> static bool copy(void* out,std::size_t size,T value){if(size!=sizeof value)return false;std::memcpy(out,&value,size);return true;}
    static bool read(std::uintptr_t at,void* out,std::size_t size)noexcept {
        auto& f=*active;if(f.read_failed)return false;
        for(const auto id:{f.selected.id,child,ancestor}){
            if(at==context(id)+8)return copy(out,size,static_cast<std::uint32_t>(id>>32));
            if(at==record(id))return copy(out,size,static_cast<std::uint32_t>(id)+(f.record_bad?1u:0u));
            if(at==0x50000+static_cast<std::uint32_t>(id)*0x10){const auto owner=f.parent_peer?ancestor:child;return copy(out,size,f.mismatch?std::uint64_t{123}:(id==f.selected.id?owner:f.selected.id));}
        }
        if(at==0x70000)return copy(out,size,std::uintptr_t{0x71000});
        if(at==0x60070)return copy(out,size,std::uintptr_t{0x61000});
        if(at==0x60078)return copy(out,size,std::uint32_t{8});
        if(at==0x6007c)return copy(out,size,std::uint32_t{1});
        return false;
    }
    TrafficSurfaceNativeApi api(){
        TrafficSurfaceNativeApi a;a.base=1;a.ready=true;a.parent_ready=parents;a.mapping_ready=mapping;a.peer_ready=peers;
        a.valid=&valid;a.query=&query;a.pose=&pose;a.body_valid=&body_valid;a.body_entity=&body_entity;a.parent=&parent;
        a.mapping=&manager;a.map_entity=&map;a.component=&component;a.peer_vtable=0x72000;a.peer_type=0x70000;a.peer_type_vtable=0x71000;a.read_bytes=&read;
        a.user=this;a.seconds=[](void* p)noexcept{return static_cast<Fixture*>(p)->now;};a.milliseconds=[](void* p)noexcept{return static_cast<Fixture*>(p)->ms;};
        a.current=[](void* p,const SurfaceGeometryContext& c,TrafficSurfaceSelection s)noexcept{const auto& f=*static_cast<Fixture*>(p);return f.phase && f.member && !f.removed && c==f.ctx && s==f.selected;};return a;
    }
    SurfaceGeometrySample geometry(){return {ctx,{7,1,9},raw,ms,{12,0,0},{0,1,0},true};}
    void advance(double seconds){now+=seconds;ms=static_cast<std::uint64_t>(std::llround(now*1000));ctx.native.sampled_at=ms;++ctx.native.physics_step;}
    bool bind(TrafficSurfaceProvider& provider){auto a=api();const auto pose=capture_traffic_surface_pose(a,ctx,selected);return provider.bind(a,pose,geometry());}
};
void ownership(){
    for(unsigned kind=0;kind<6;++kind){Fixture f;
        auto expected=TrafficSurfaceOwner::direct;
        if(kind){f.raw=Fixture::child;if(kind==1){f.mapping=true;expected=TrafficSurfaceOwner::mapped;}
            if(kind==2){f.peers=true;expected=TrafficSurfaceOwner::peer;}
            if(kind>=3){f.parents=true;if(kind==3){f.parent_direct=true;f.selected.id=(1ULL<<32)|101;expected=TrafficSurfaceOwner::parent_direct;}
                if(kind==4){f.mapping=true;f.parent_mapping=true;expected=TrafficSurfaceOwner::parent_mapped;}
                if(kind==5){f.peers=true;f.parent_peer=true;expected=TrafficSurfaceOwner::parent_peer;}}
        }
        check(traffic_surface_owner(f.api(),f.ctx,f.selected,{7,1,9})==expected,"published direct/mapped/peer/actual-parent ownership routes");
    }
    for(unsigned failure=0;failure<9;++failure){Fixture f;f.raw=Fixture::child;
        if(failure==0)f.owner_changed=true;
        if(failure==1){f.peers=true;f.mismatch=true;}
        if(failure==2){f.peers=true;f.peer_changed=true;}
        if(failure==3){f.parents=true;f.cycle=true;}
        if(failure==4)f.record_bad=true;
        if(failure==5)f.body_changed=true;
        if(failure==6)f.member=false;
        if(failure==7)f.selected.removal_free=false;
        if(failure==8)f.phase=false;
        check(traffic_surface_owner(f.api(),f.ctx,f.selected,{7,1,9})==TrafficSurfaceOwner::unavailable,"missing/changed/mismatched ownership never becomes a nearby target");
    }
}
void lifecycle(){
    Fixture f;TrafficSurfaceProvider p;
    check(f.bind(p) && p.surface_serial()==1,"verified pose and measured body bind a stable local point");
    check(!p.snapshot(f.ctx,f.selected,f.now,f.ms).valid,"bound point is not warm motion");
    f.advance(.017);f.transform[8]+=.017f;const auto a=p.observe(f.api(),f.ctx,f.selected);
    check(a.valid && std::abs(a.velocity[0]-1)<.001f && a.local_point==SurfaceGeometryVector{2,0,0},"transport same locked material point");
    for(unsigned i=0;i<20;++i){const auto cached=p.snapshot(f.ctx,f.selected,f.now+.001*i,f.ms+i);check(cached.valid && cached.sampled_at==a.sampled_at && cached.point==a.point && cached.surface_serial==1,"polls retain original point/time/serial");}
    f.advance(.001);f.transform[8]+=.001f;const auto held=p.observe(f.api(),f.ctx,f.selected);
    check(held.valid && held.sampled_at==a.sampled_at && held.observed_ms==a.observed_ms && held.normal==a.normal,"sub240Hz provider result keeps whole cached observation");
    check(!p.snapshot(f.ctx,f.selected,a.sampled_at+.101,f.ms+101).valid && p.surface_serial()==1,"expired motion denies output without replacing identity");
    f.advance(.151);check(!p.observe(f.api(),f.ctx,f.selected).valid && p.surface_serial()==1,"cadence gap resets motion not selected identity");
    f.advance(.017);f.transform[8]+=.017f;check(p.observe(f.api(),f.ctx,f.selected).valid,"same identity warms after gap");
    ++f.selected.removals;check(!p.snapshot(f.ctx,f.selected,f.now,f.ms).valid,"removal journal change revokes copied motion");
    check(!p.observe(f.api(),f.ctx,f.selected).valid && p.surface_serial()==0,"removal change retires binding");
    check(f.bind(p) && p.surface_serial()==2,"new admitted binding gets a new serial");p.invalidate();check(p.surface_serial()==0,"existing runtime invalidation is immediate");
}
void negatives(){
    for(unsigned failure=0;failure<9;++failure){Fixture f;TrafficSurfaceProvider p;
        if(failure==0)f.query_changed=true;
        if(failure==1)f.record_bad=true;
        if(failure==2)f.transform[8]=std::numeric_limits<float>::quiet_NaN();
        if(failure==3)f.transform[8]=0;
        if(failure==4)f.throw_pose=true;
        if(failure==5)f.removed=true;
        if(failure==6)f.read_failed=true;
        if(failure==7)f.member=false;
        if(failure==8)f.now=std::numeric_limits<double>::infinity();
        check(!f.bind(p) && p.surface_serial()==0,"unavailable/unstable/default pose cannot bind");
    }
    for(unsigned failure=0;failure<8;++failure){Fixture f;TrafficSurfaceProvider p;check(f.bind(p),"replacement fixture bound");
        if(failure==0)++f.selected.generation;
        if(failure==1)++f.ctx.world_generation;
        if(failure==2)++f.body_generation;
        if(failure==3)++f.record_version;
        if(failure==4)++f.ctx.native.generations.owner;
        if(failure==5)++f.ctx.native.airborne_episode;
        if(failure==6)f.selected.removal_free=false;
        if(failure==7)f.throw_pose=true;
        f.advance(.017);check(!p.observe(f.api(),f.ctx,f.selected).valid && p.surface_serial()==0,"incarnation/body/world/owner/flight/removal replacement revokes");
    }
    {Fixture f;TrafficSurfaceProvider p;const auto pose=capture_traffic_surface_pose(f.api(),f.ctx,f.selected);auto geometry=f.geometry();geometry.queried_at+=26;f.now+=.026;f.ms+=26;check(!p.bind(f.api(),pose,geometry),"late discovery cannot retime captured pose");}
    {Fixture f;TrafficSurfaceProvider p;const auto pose=capture_traffic_surface_pose(f.api(),f.ctx,f.selected);auto geometry=f.geometry();geometry.queried_at+=25;f.now+=.025;f.ms+=25;check(p.bind(f.api(),pose,geometry),"source discovery budget includes25ms");check(p.bind(f.api(),pose,geometry) && p.surface_serial()==1,"repeated same measured surface does not recenter or renew serial");}
}
void rotating_provider(){
    for(const double step:{.017,.034}){
        Fixture f;TrafficSurfaceProvider p;f.transform[0]=2;f.transform[1]=3;f.transform[2]=4;
        const auto pose=capture_traffic_surface_pose(f.api(),f.ctx,f.selected);auto geometry=f.geometry();
        const float n=std::sqrt(.5f);geometry.query_normal={n,n,0};
        check(p.bind(f.api(),pose,geometry),"nonuniform traffic surface bound");
        for(unsigned i=1;i<=40;++i){
            f.advance(step);const double angle=.2*(f.now-1);
            f.transform[5]=static_cast<float>(std::sin(angle/2));f.transform[7]=static_cast<float>(std::cos(angle/2));
            const auto sample=p.observe(f.api(),f.ctx,f.selected);
            check(sample.valid && std::abs(std::hypot(sample.velocity[0],sample.velocity[2])-.4)<.002,"verified rotating body produces tangential point velocity");
            check(sample.local_point==SurfaceGeometryVector{1,0,0} && sample.surface_serial==1 && sample.queried_at==1000,"nonuniform locked local point and source query remain immutable");
            const SurfaceGeometryVector expected{n*static_cast<float>(std::cos(angle)),n,-n*static_cast<float>(std::sin(angle))};
            for(std::size_t j=0;j<3;++j)check(std::abs(sample.normal[j]-expected[j])<.0001f,"inverse-transpose normal follows original material plane");
        }
    }
}
}
int main(){ownership();lifecycle();negatives();rotating_provider();std::cout<<checks<<" selected traffic-surface provider checks passed\n";}
