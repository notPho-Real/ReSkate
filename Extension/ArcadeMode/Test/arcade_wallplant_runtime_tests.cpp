// Offline boundary fixture: includes the real runtime implementation unchanged.
#include "Extension/ArcadeMode/arcade_wallplant_runtime.cpp"
#include <cstdlib>
#include <iostream>
namespace fixture {
std::uint64_t now=1000;
bool hit_current=true,race_cancel{};
bool collision_scope{};
dingosdk::arcade_mode::native_wall_contact::Snapshot collision{};
unsigned begins{},releases{},cancels{},checks{};
std::uint64_t released_identity{};
std::uint64_t visual_expiry{};
dingosdk::arcade_mode::WallplantCommand active{};
std::array<float,3> visual_normal{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
}
std::uint64_t GetTickCount64(){return fixture::now;}
DWORD GetLastError(){return 0;}
void SetLastError(DWORD){}
namespace dingosdk::logging {
bool enabled(Level) noexcept{return false;}
void write(Level,Context,Channel,std::string_view) noexcept{}
void write(Level,Channel,std::string_view) noexcept{}
}
namespace dingosdk::arcade_mode::native_wall_contact {
bool start(std::uintptr_t) noexcept{return true;}
void enable(bool) noexcept{}
Snapshot observe(const NoBailSkater&,std::uint64_t) noexcept{return fixture::collision;}
bool collision_current(std::uint64_t serial,const NoBailSkater& owner) noexcept {
    return fixture::collision_scope && serial && serial==fixture::collision.serial && owner==fixture::collision.owner;
}
bool current(const Contact&,std::uintptr_t) noexcept{return fixture::hit_current;}
bool static_world_owner(const SurfaceGeometryBody&,SurfaceGeometryWorldOwner&) noexcept{return false;}
bool local_component_owner(const SurfaceGeometryBody&,std::uintptr_t,SurfaceGeometryWorldOwner&) noexcept{return false;}
}
namespace dingosdk::arcade_mode::wallplant_animation {
void tick(bool) noexcept{}
bool available(const NoBailSkater&) noexcept{return true;}
void cancel() noexcept{++fixture::cancels;}
void release(const NoBailSkater&,std::uint64_t identity) noexcept{++fixture::releases;fixture::released_identity=identity;}
bool begin_hold(const NoBailSkater&,std::uint64_t,std::uint64_t,std::uint64_t expires,const std::array<float,3>&,
        const std::array<float,3>& normal,const std::array<float,3>&) noexcept {
    ++fixture::begins;fixture::visual_normal=normal;fixture::visual_expiry=expires;
    if(fixture::race_cancel)wallplant_runtime::finish_hold(fixture::active,false,fixture::now);
    return true;
}
}
namespace dingosdk::arcade_mode {
SurfaceGeometrySample query_surface_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        const SurfaceGeometryRequest&,SurfaceGeometryTrace*) noexcept{return {};}
SurfaceGeometrySample query_wall_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        SurfaceGeometryBody,SurfaceGeometryVector,SurfaceGeometryVector,SurfaceGeometryVector,float,
        SurfaceGeometryTrace*) noexcept{return {};}
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        const SurfaceGeometrySample&) noexcept{return false;}
SurfaceGeometryAnchor query_wallplant_anchor(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        SurfaceGeometryBody,SurfaceGeometryVector,SurfaceGeometryVector,SurfaceGeometryVector,
        std::span<const SurfaceGeometryVector>,SurfaceGeometryTrace*) noexcept{return {};}
}
using namespace dingosdk::arcade_mode;
using namespace dingosdk::arcade_mode::wallplant_runtime;
using fixture::check;
WallplantCommand setup() {
    cancel();fixture::now=1000;fixture::hit_current=true;fixture::race_cancel=false;
    fixture::begins=fixture::releases=fixture::cancels=0;fixture::released_identity=0;
    std::lock_guard lock(mutex);
    interrupt.store(false);stop_pose.store(false);hold_stop={};hold_play={};
    WallplantCommand c{};c.motion.ready=true;c.input_sequence=7;c.contact=3;c.issued_at=1000;
    c.before={-12,-2,5};c.after={6.72f,12.7f,2.8f};c.incoming=c.before;
    c.reflected={12,0,5};c.hold_forward={.92f,0,.38f};c.normal={1,0,0};
    c.contact_normal={.9987492f,.05f,0};c.anchor={1,2,3};c.anchor_verified=true;
    NativeInputState tracker;NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
    c.native=tracker.observe(990,{1,2,3,4,5},sample,{},false,1.f/30);
    const auto& f=c.native;
    observed=f;permitted=takeoff=true;candidate.valid=true;in_flight=c;
    fixture::active=c;return c;
}
NativeInputFrame ground(const WallplantCommand& c,std::uint64_t at) {
    NativeInputState tracker;NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
    tracker.observe(c.native.sampled_at,c.native.owner,sample,{},false,1.f/30);
    sample.state=100;sample.crouch_mode=1;sample.words[1]=0x8000;
    for(auto stamp=c.native.sampled_at+33;stamp<at;stamp+=33)
        tracker.observe(stamp,c.native.owner,sample,{},false,1.f/30);
    return tracker.observe(at,c.native.owner,sample,{},false,1.f/30);
}
bool enter(const WallplantCommand& c,std::uint64_t at){fixture::now=at;return begin_hold(c,at);}
bool validate(const WallplantCommand& c,std::uint64_t at,const NativeInputFrame& f){fixture::now=at;return hold_current(c,at,f);}
void observe_at(const NativeInputFrame& f){fixture::now=std::max(fixture::now,f.sampled_at);observe_physics(f);}
void finish_at(const WallplantCommand& c,bool ok,std::uint64_t at){fixture::now=at;finish_hold(c,ok,at);}
bool preserve_at(const NativeInputFrame& f,std::uint64_t at){fixture::now=at;return preserves_combo(f,at);}
int main(){
    for(unsigned fault=0;fault<13;++fault) {
        const auto command=setup();const auto origin=command.native;
        in_flight={};candidate={};contact_active=false;last_raw=0;
        auto& raw_contact=fixture::collision;raw_contact={};
        raw_contact.valid=raw_contact.after_valid=true;raw_contact.serial=1;raw_contact.sampled_at=1000;
        raw_contact.owner={1,2,3,4,5};raw_contact.state=raw_contact.state_after=201;
        raw_contact.source=native_wall_contact::Source::board;raw_contact.seconds=origin.physics_seconds;
        raw_contact.count=1;auto& hit=raw_contact.contacts[0];
        hit.identity_valid=hit.static_body=hit.categories_known=true;hit.normal={1,0,0};
        fixture::collision_scope=true;
        auto frame=origin;
        if(fault==1)fixture::collision_scope=false;
        if(fault==2)++frame.owner_generation;
        if(fault==3)takeoff=false;
        if(fault==4)raw_contact.state_after=504;
        if(fault==5)raw_contact.seconds=0;
        if(fault==6)raw_contact.sampled_at=989;
        if(fault==7)raw_contact.sampled_at=1001;
        if(fault==8)hit.static_body=false;
        if(fault==9)hit.normal={0,1,0};
        if(fault==10)raw_contact.source=native_wall_contact::Source::rig;
        if(fault==11)permitted=false;
        if(fault==12)last_raw=raw_contact.serial;
        check(retain_collision_before_response(frame,1000)==(fault==0),
              "first contact requires the exact active native collision, prior AIR authority and genuine wall identity");
        check(wallplant_same_observation(observed,origin),
              "early contact retention never renews a native frame or manufactures a physics step");
        check(!surface(1000,origin).can_plant && !pending.sequence && !held_command(),
              "early raw contact alone cannot bypass geometry, fresh A or hold entry");
    }
    fixture::collision={};fixture::collision_scope=false;
    for(bool missing:{false,true}) {
        auto c=setup();if(missing)c.anchor_verified=false;else ++c.anchor[0];
        check(!enter(c,1000) && !held_command(),"changed or unverified entry anchor cannot borrow consumed geometry authority");
    }
    {
        auto c=setup();check(enter(c,1000),"successful entry starts physical hold");
        check(!latest_completed(),"entry is not a completed rebound");
        check(!enter(c,1001),"entry cannot duplicate active phase");
        const auto f=ground(c,1033);observe_at(f);fixture::now=1035;tick(1,true);
        check(fixture::begins==1 && fixture::visual_normal==c.contact_normal,"visual begins during contact100 using actual wall plane");
        check(fixture::visual_expiry==1194,"visual shares the original physical dispatch expiry without renewing its clock");
        fixture::now=1040;tick(1,true);check(fixture::begins==1,"same hold does not queue another pose");
        finish_at(c,false,1040);fixture::now=1041;tick(1,true);
        check(fixture::releases==1 && fixture::released_identity==c.input_sequence,"cancellation retains stop identity for client");
        check(!latest_completed(),"cancelled hold creates no rebound occurrence");
    }
    {
        auto c=setup();check(enter(c,1000),"release test enters");
        observe_at(ground(c,1033));
        auto f=ground(c,1165);observe_at(f);fixture::now=1175;
        check(validate(c,1175,f),"release revalidates original hold with current native sample");
        finish_at(c,true,1175);
        check(latest_completed().input_sequence==7 && !held_command(),"actual release publishes once and removes motion authority");
        check(preserve_at(f,1200),"existing contact100 has one native interval from actual release");
        auto expired=combo_bridge;
        check(!expired.observe(f,1210),"release bridge does not grant a second native interval");
        check(!combo_bridge.target_active(1180) && !combo_bridge.release_due(1180),"combo bridge grants no target or release rights");
        auto air=f;air.state=200;air.airborne=true;++air.airborne_episode;++air.physics_step;air.sampled_at=1205;
        ++air.action_generation;++air.contact_generation;air.mode=2;air.contacts=0;
        observe_at(air);check(!combo_bridge.active(),"first genuine resumedAIR retires bridge");
        auto landed=air;landed.state=100;landed.airborne=false;++landed.air_departure;++landed.physics_step;landed.sampled_at=1208;
        check(!preserve_at(landed,1208),"real landing after resumedAIR cannot inherit suppression");
        finish_at(c,true,1208);check(latest_completed().issued_at==1000,"duplicate finish cannot renew completed occurrence");
    }
    {
        auto c=setup();check(enter(c,1000),"air release test enters");
        auto air=c.native;air.sampled_at=1050;++air.physics_step;observe_at(air);
        air.sampled_at=1160;++air.physics_step;observe_at(air);
        finish_at(c,true,1160);check(!combo_bridge.active(),"release already inAIR cannot suppress a later real ground contact");
    }
    for(unsigned fault=0;fault<14;++fault){
        auto c=setup();check(enter(c,1000),"immutable identity fixture enters");auto changed=c;
        switch(fault){
        case 0:++changed.native.owner_generation;break;case 1:++changed.native.physics_step;break;
        case 2:++changed.native.sampled_at;break;case 3:++changed.contact;break;
        case 4:++changed.geometry.world;break;case 5:++changed.body.generation;break;
        case 6:++changed.point[0];break;case 7:++changed.contact_normal[1];break;
        case 8:++changed.before[0];break;case 9:++changed.after[0];break;
        case 10:++changed.incoming[0];break;case 11:++changed.reflected[0];break;
        case 12:++changed.hold_forward[0];break;case 13:++changed.anchor[0];break;
        }
        check(!validate(changed,1033,ground(c,1033)) && bool(held_command()),"changed command cannot borrow or cancel legitimate held authority");
        finish_at(changed,true,1160);check(!latest_completed() && bool(held_command()),"changed completion cannot manufacture release");
    }
    for(unsigned fault=0;fault<3;++fault){
        auto c=setup();check(enter(c,1000),"invalid continuity fixture enters");auto f=ground(c,1033);
        if(fault==0)++f.owner_generation;
        if(fault==1)f.state=504;
        if(fault==2)fixture::hit_current=false;
        check(!validate(c,1033,f) && !held_command(),"invalid live ownership/state/contact cancels hold");
        check(hold_stop.input_sequence==7,"destructive continuity rejection preserves original visual stop receipt");
        fixture::now=1034;tick(1,true);
        check(fixture::released_identity==7 && fixture::begins==0,"cancelled queued visual never starts late");
    }
    {
        auto c=setup();check(enter(c,1000),"physics cancellation fixture enters");
        auto f=ground(c,1033);f.state=504;observe_at(f);
        check(!held_command() && hold_stop.input_sequence==7,"observer rejection preserves stop before helper clears command");
    }
    {
        auto c=setup();check(enter(c,1000),"request race fixture enters");
        fixture::race_cancel=true;fixture::now=1030;tick(1,true);
        check(fixture::begins==1 && fixture::cancels==1 && !held_command(),"release during visual request cancels accepted pose immediately after request");
    }
    {
        auto c=setup();check(enter(c,1000),"delayed publication fixture enters");
        const auto f=ground(c,1033);std::uint64_t stale_now=1033;fixture::now=1040;
        check(hold_current(c,stale_now,f) && stale_now==1040,"target validation returns clock refreshed inside runtime lock");
        fixture::now=1045;observe_physics(f);
        check(bool(held_command()) && observed.sampled_at==1033,"delayed native publication does not backdate validation or renew observation");
        auto late=ground(c,1165);fixture::now=1180;stale_now=1165;
        check(hold_current(c,stale_now,late) && stale_now==1180,"late callback validates actual current clock");
        fixture::now=1181;finish_hold(c,true,1165);
        check(latest_completed().input_sequence==7,"completion samples current clock rather than stale caller time");
    }
    {
        auto c=setup();check(enter(c,1000),"late dispatch fixture enters");
        check(!validate(c,1195,ground(c,1195)),"expired physical phase cannot release late");
        finish_at(c,true,1195);check(!latest_completed(),"expired release cannot publish success");
    }
    {
        auto c=setup();check(enter(c,1000),"interleaved reader fixture enters");
        const auto reader_frame=ground(c,1033);observe_at(reader_frame);
        const auto newer=ground(c,1066);observe_at(newer);
        fixture::now=1070;std::uint64_t at=1033;bool stale=false;
        const bool authorized=hold_current(c,at,reader_frame,&stale);
        check(bool(held_command()) && hold.started_at()==1000 && hold.deadline()==1160,
            "stale query preserves newer hold without extending its deadline");
        check(!authorized && stale,"reader overtaken by original physics cannot authorize a target");
        check(hold_current(c,at,newer,&stale) && !stale,
            "fresh reader can use the independently published newer frame");
        fixture::now=1195;
        check(!hold_current(c,at,reader_frame,&stale) && !stale && !held_command(),
            "expired hold cancels even when reader is stale");
    }
    for(unsigned fault=0;fault<3;++fault) {
        auto c=setup();check(enter(c,1000),"stale query safety fixture enters");
        auto older=ground(c,1033);observe_at(ground(c,1066));
        if(fault==0)++older.owner_generation;
        if(fault==1)older.state=504;
        if(fault==2)fixture::hit_current=false;
        fixture::now=1070;std::uint64_t at=1070;bool stale=true;
        check(!hold_current(c,at,older,&stale) && !stale && !held_command(),
            "owner, bail and collision loss cannot hide behind stale-query deferral");
    }
    std::cout<<fixture::checks<<" actual runtime hold lifecycle checks passed\n";
}
