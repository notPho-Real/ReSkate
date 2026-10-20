#include "Extension/ArcadeMode/arcade_controller_route.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
static void observe(NativeControllerRoute& route,unsigned slot,ArcadeInput::Physical pad){
    route.observe_unfiltered(slot,pad);route.observe(slot,pad);
}
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
ArcadeInput::Physical physical(std::uint64_t now,short y=0){return {true,now,0,123,y,321,-456,0,0};}
bool passthrough(const NativeGesture::Output& out){return out.buttons==0&&out.right_x==321&&out.right_y==-456&&!out.phase;}
struct Pipeline {
    NativeControllerRoute route;ArcadeInput input{true};std::uint64_t step{},owner{1};
    Pipeline(){input.enable(true);}
    void client(std::uint64_t now,unsigned bitmap,bool focus=true){
        NativeInputFrame frame{};frame.valid=true;frame.owner={1,2,3,4,5};frame.owner_generation=owner;
        frame.contact_generation=frame.action_generation=1;frame.physics_step=++step;frame.sampled_at=now;
        frame.state=100;frame.mode=1;frame.contacts=0xc000;frame.flip_known=true;
        if(route.select(now,bitmap,focus,frame)){input.invalidate_authority();input.observe_physical(0,{});}
        input.publish_native_context(frame);input.publish_authority(now,owner,route.physical(now).valid?1u:0u,focus);
    }
    NativeGesture::Output poll(unsigned user,std::uint64_t now,short y=0){
        const auto pad=physical(now,y);observe(route,user,pad);
        if(!route.accepts(user,now))return {pad.buttons,pad.rx,pad.ry};
        input.observe_physical(NativeControllerRoute::session,pad);
        return input.poll(NativeControllerRoute::session,now,pad.buttons,pad.lx,pad.ly,pad.rx,pad.ry,
            true,false,false,0,false,false,true,now/17,false,{},false,true,SkatePose::riding);
    }
    void mode(bool value){route.reset();input.observe_physical(0,{});input.enable(value);}
};
void slots(){
    for(unsigned selected=0;selected<4;++selected){
        Pipeline p;p.poll(selected,1000);p.client(1000,1u<<selected);
        check(!passthrough(p.poll(selected,1000)),"sole connected physical controller reaches unchanged local-player session on any slot");
        for(unsigned other=0;other<4;++other)if(other!=selected){
            const auto before=p.input.physical(1001);
            check(passthrough(p.poll(other,1001,-25000)),"nonowner packet remains native");
            check(p.input.physical(1001).at==before.at&&p.input.physical(1001).ly==before.ly,"nonowner cannot replace shared physical state");
            check(p.input.admitted(0,1001),"nonowner does not cancel current owner's authority");
        }
        p.mode(false);for(unsigned user=0;user<4;++user)check(passthrough(p.poll(user,1017)),"OFF restores all physical slots");
        p.mode(true);check(passthrough(p.poll(selected,1034)),"OFF/ON requires renewed source and native proof");
        p.client(1034,1u<<selected);check(!passthrough(p.poll(selected,1034)),"fresh source and frame can restore the same owner after OFF");
    }
}
void ambiguous(){
    for(unsigned bitmap=0;bitmap<16;++bitmap){
        if(bitmap&&!(bitmap&(bitmap-1)))continue;
        Pipeline p;for(unsigned user=0;user<4;++user)p.poll(user,1000);p.client(1000,bitmap);
        for(unsigned user=0;user<4;++user)check(passthrough(p.poll(user,1001)),"zero/ambiguous source set never elects an arbitrary or merged owner");
        check(!p.input.admitted(0,1001),"ambiguous source has no native authority");
    }
    for(unsigned source:{16u,255u,256u,257u,0xffffffffu}){
        Pipeline p;for(unsigned user=0;user<4;++user)p.poll(user,1000);p.client(1000,source);
        for(unsigned user=0;user<4;++user)check(passthrough(p.poll(user,1001)),"unsupported/Sony source cannot masquerade as a physical XInput slot");
    }
}
void freshness(){
    for(unsigned slot=0;slot<4;++slot){
        NativeControllerRoute route;observe(route,slot,physical(1000));route.select(1000,1u<<slot,true);
        check(route.accepts(slot,1150)&&!route.accepts(slot,1151)&&!route.accepts(slot,999),"selection and physical samples retain inclusive150ms/future bounds");
        const auto generation=route.generation();observe(route,slot,physical(1151));
        check(!route.accepts(slot,1151),"fresh game poll cannot renew expired client selection");
        check(route.select(1151,1u<<slot,true)&&route.generation()!=generation,"selection after a lease gap cancels the prior owner epoch");
        check(route.accepts(slot,1151),"fresh republished source and physical sample can recover");
        check(route.select(1152,1u<<slot,false)&&!route.accepts(slot,1152),"focus loss immediately withdraws selection");
        route.reset();check(!route.accepts(slot,1152)&&!route.physical(1152).valid,"mode reset drops selected source and all caches");
        route.select(1152,1u<<slot,true);
        check(!route.accepts(slot,1152),"reset cannot reuse a pre-reset physical sample");
    }
    Pipeline p;p.poll(1,1000);p.client(1000,2);p.input.publish_native_context({});check(passthrough(p.poll(1,1001)),"routing does not bypass native proof loss");
    p.client(1017,2);p.input.publish_authority(1017,2,1,true);check(passthrough(p.poll(1,1018)),"routing does not bypass native owner mismatch");
    p.client(1034,2);check(passthrough(p.poll(1,1185)),"physical polls do not renew expired native/client proof");
}
void handoff(){
    for(unsigned from=0;from<4;++from)for(unsigned to=0;to<4;++to)if(from!=to){
        Pipeline p;p.poll(from,1000);p.client(1000,1u<<from);p.poll(from,1000);
        p.poll(from,1017,25000);p.client(1017,1u<<from);p.poll(from,1017,25000);
        p.poll(from,1034,-25000);p.client(1034,1u<<from);
        check(p.poll(from,1034,-25000).manual.owns,"manual establishes a live old source owner");
        p.poll(to,1051,-25000);const auto generation=p.route.generation();p.client(1051,1u<<to);
        check(p.route.generation()!=generation&&!p.input.busy(0),"source switch retires the old command and changes routing generation");
        check(!p.poll(to,1051,-25000).manual.owns,"held input on new source cannot inherit or fabricate a manual");
        check(passthrough(p.poll(from,1052,25000)),"old source cannot command the new source's local-player session");
        p.client(1068,(1u<<from)|(1u<<to));check(!p.input.admitted(0,1068),"adding a second source cancels instead of choosing by activity");
    }
}

NativeInputFrame binding_frame(std::uint64_t now,std::uint64_t step=1){
    NativeInputFrame f{};f.valid=true;f.owner={1,2,3,4,5};f.owner_generation=7;
    f.contact_generation=f.action_generation=10;f.physics_step=step;f.sampled_at=now;f.state=100;return f;
}
void explicit_binding(){
    for(unsigned slot=0;slot<4;++slot)for(unsigned bitmap=1;bitmap<16;++bitmap){
        if(!(bitmap&(1u<<slot)))continue;
        NativeControllerRoute r;
        check(r.arm(slot,1000,1000),"explicit slot request accepted, without admission");
        r.select(1000,bitmap,true,binding_frame(1000));
        check(!r.accepts(slot,1000),"arming clears old physical samples and cannot manufacture input");
        for(unsigned user=0;user<4;++user)observe(r,user,physical(1001,user==slot?0:25000));
        r.select(1001,bitmap,true,binding_frame(1001));
        check(r.accepts(slot,1001),"explicit neutral source binds independently of other connected or active pads");
        const auto generation=r.generation();
        for(unsigned user=0;user<4;++user)check(r.accepts(user,1001)==(user==slot),"explicit route accepts exactly the named transport");
        auto row=r.observation(1001,true,true);
        check(row.explicit_binding&&row.logical_actor==0&&controller_route_evidence_valid(row),"explicit evidence permits selected bit with multiple connected pads");
        row.explicit_binding=false;
        check(controller_route_evidence_valid(row)==(!(bitmap&(bitmap-1))),"multi-source readiness requires explicit policy evidence");
        observe(r,slot,physical(1017,25000));
        auto frame=binding_frame(1017,2);++frame.contact_generation;++frame.action_generation;frame.state=200;
        r.select(1017,15,true,frame);
        check(r.accepts(slot,1017)&&r.generation()==generation,"normal state/contact/action changes and other hotplug do not reassign owner");
        observe(r,slot,physical(1034));r.select(1034,15&~(1u<<slot),true,binding_frame(1034,3));
        check(!r.physical(1034).valid,"selected bit disappearance revokes");
        observe(r,slot,physical(1051));r.select(1051,15,true,binding_frame(1051,4));
        check(!r.physical(1051).valid,"reconnect cannot silently reuse revoked binding");
        r.reset();observe(r,slot,physical(1068));r.select(1068,15,true);
        check(!r.physical(1068).valid,"OFF/ON clears explicit policy and restores sole-source automatic default");
        r.disconnected(slot);const auto player=(slot+1)%4;observe(r,player,physical(1085));r.select(1085,1u<<player,true);
        check(r.accepts(player,1085),"after OFF and fixture disappearance physical player can acquire a fresh sole route");
    }
    for(unsigned fault=0;fault<12;++fault){
        NativeControllerRoute r;r.arm(1,1000,1000);observe(r,1,physical(1000));r.select(1000,3,true,binding_frame(1000,5));
        check(r.accepts(1,1000),"fault control starts admitted");
        auto f=binding_frame(1017,6);unsigned bitmap=3;bool available=true;
        if(fault==0)f.owner.rig++;
        if(fault==1)f.owner_generation++;
        if(fault==2)f.physics_step=4;
        if(fault==3)f.sampled_at=999;
        if(fault==4)f.valid=false;
        if(fault==5)f.sampled_at=1018;
        if(fault==6)f.contact_generation=9;
        if(fault==7)f.action_generation=9;
        if(fault==8)available=false;
        if(fault==9)bitmap=256;
        if(fault==10)r.disconnected(1);
        if(fault==11)r.captured(1);
        observe(r,1,physical(1017));r.select(1017,bitmap,available,f);
        check(!r.physical(1017).valid,"owner/state replacement, stale generations, focus, unsupported transport or source loss revokes");
        observe(r,1,physical(1034));r.select(1034,3,true,binding_frame(1034,7));
        check(!r.accepts(1,1034),"failed binding cannot recover through fresh activity alone");
    }
    NativeControllerRoute filtered;filtered.arm(1,1000,1000);
    filtered.observe(1,physical(1000));filtered.select(1000,3,true,binding_frame(1000));
    check(!filtered.accepts(1,1000),"filtered input alone cannot prove raw neutral admission");
    filtered.observe_unfiltered(1,physical(1000,25000));filtered.select(1000,3,true,binding_frame(1000));
    check(!filtered.accepts(1,1000),"an earlier controller filter cannot conceal physically held input for binding");
    filtered.observe_unfiltered(1,physical(849));filtered.select(1000,3,true,binding_frame(1000));
    check(!filtered.accepts(1,1000),"raw neutral observation retains the existing150ms limit");
    filtered.observe_unfiltered(1,physical(1000));filtered.select(1000,3,true,binding_frame(1000));
    check(filtered.accepts(1,1000),"fresh unfiltered and normal input stages can establish neutral binding");
    Pipeline p;p.route.arm(1,1000,1000);p.poll(1,1000);p.client(1000,3);
    check(!passthrough(p.poll(1,1000)),"explicit multi-connected source reaches real facade playback/movement");
    const auto before=p.input.physical(1001);
    check(passthrough(p.poll(0,1001,25000))&&p.input.physical(1001).at==before.at,
        "active wrong source cannot blend input into bound real facade");
    p.poll(1,1017,25000);p.client(1017,3);p.poll(1,1017,25000);
    p.poll(0,1020,-25000);p.poll(1,1034,-25000);p.client(1034,3);
    check(p.poll(1,1034,-25000).manual.owns,"shared native Manual adapter remains reachable from explicit source");
    p.route.arm(0,1051,1000);p.poll(0,1051,-25000);p.client(1051,3);
    check(!p.input.busy(0)&&!p.route.physical(1051).valid,"reassignment cannot inherit Manual or admit held new source");
    NativeControllerRoute r;
    check(!r.arm(4,1000,1000)&&!r.arm(0,1000,999)&&!r.arm(0,1000,300001)&&!r.arm(0,UINT64_MAX,1000),"invalid slot/duration/overflow rejected");
    r.arm(1,1000,10000);r.captured(1);r.select(1000,3,false);
    check(!r.accepts(1,1000),"console capture while pending never supplies admission or consumes request");
    observe(r,1,physical(1001,25000));r.select(1001,3,true,binding_frame(1001));
    check(!r.accepts(1,1001),"held selected stick cannot create a binding");
    auto held=physical(1002);held.buttons=0x1000;observe(r,1,held);r.select(1002,3,true,binding_frame(1002));
    check(!r.accepts(1,1002),"held face button cannot create binding");
    held=physical(1003);held.lt=31;observe(r,1,held);r.select(1003,3,true,binding_frame(1003));
    check(!r.accepts(1,1003),"trigger neutral threshold reuses input normalization");
    observe(r,1,physical(6000));r.select(6000,3,true,binding_frame(6000));
    check(r.accepts(1,6000),"pending request admits at inclusive5s boundary");
    check(!r.disconnected(0)&&r.accepts(1,6000),"nonselected disconnection cannot cancel binding");
    observe(r,1,physical(6151));r.select(6151,3,true,binding_frame(6151,2));
    check(!r.accepts(1,6151),"client/source proof lease gap revokes instead of renewing");
    r.arm(1,7000,10000);observe(r,1,physical(12001));r.select(12001,3,true,binding_frame(12001));
    check(!r.accepts(1,12001),"waiting past5s cannot admit late");
    r.arm(1,13000,1000);observe(r,1,physical(13000));r.select(13000,3,true,binding_frame(13000));
    for(std::uint64_t now=13100;now<=13900;now+=100){observe(r,1,physical(now));r.select(now,3,true,binding_frame(now,now));}
    observe(r,1,physical(13999));r.select(13999,3,true,binding_frame(13999,13999));
    check(r.accepts(1,13999)&&!r.accepts(1,14000),"finite lease cannot be renewed by polling or physics updates");
    r.select(14000,3,true,binding_frame(14000,14000));r.clear();observe(r,0,physical(14001));r.select(14001,1,true,binding_frame(14001));
    check(!r.accepts(0,14001),"clear/teardown blocks automatic physical handoff until deliberate OFF");
}

void diagnostics(){
    using Reason=ControllerRouteReason;
    NativeControllerRoute route;
    auto sample=[&](std::uint64_t now,bool enabled=true,bool available=true){
        const auto generation=route.generation();const auto before=route.physical(now);
        const auto row=route.observation(now,enabled,available);
        check(client_telemetry_valid(ClientTelemetry{row}),"route diagnostic fits existing sanitized client telemetry");
        check(route.generation()==generation&&route.physical(now).valid==before.valid&&route.physical(now).at==before.at,
            "observing route diagnostics cannot change selection, timestamps or authority");
        return row;
    };
    route.select(1000,0,true);auto row=sample(1000);
    check(row.bitmap_known&&!row.bitmap&&row.reason==Reason::no_connected_xinput&&row.logical_actor==4,"observed zero bitmap is explicit no-connected denial");
    for(unsigned slot=0;slot<4;++slot){
        route.reset();observe(route,slot,physical(1000));route.select(1000,1u<<slot,true);row=sample(1000);
        check(row.bitmap_known&&row.bitmap==(1u<<slot)&&row.physical_slot==slot&&row.logical_actor==0&&row.generation==route.generation()&&row.reason==Reason::ready,
            "one observed source reports actual physical slot and single logical actor");
        row=sample(1151);check(row.reason==Reason::physical_sample_stale&&row.logical_actor==4,"old physical sample never reports route ready");
        observe(route,slot,physical(1151));row=sample(1151);
        check(row.reason==Reason::selection_stale,"new physical packet cannot conceal expired client selection");
        observe(route,slot,physical(999));row=sample(999);
        check(row.reason==Reason::selection_clock_future,"selection clock rollback has an explicit denial");
        observe(route,slot,physical(1001));row=sample(1000);
        check(row.reason==Reason::physical_clock_future,"future physical sample has an explicit denial");
    }
    for(unsigned bitmap=1;bitmap<16;++bitmap)if(bitmap&(bitmap-1)){
        route.select(1000,bitmap,true);row=sample(1000);
        check(row.bitmap_known&&row.bitmap==bitmap&&row.physical_slot==4&&row.logical_actor==4&&row.reason==Reason::multiple_connected,
            "every ambiguous source set reports exact bitmap and unselected denial");
    }
    route.reset();route.select(1000,2,true);row=sample(1000);
    check(row.reason==Reason::no_physical_sample,"sole bitmap alone is not a physical poll or admission");
    observe(route,1,physical(1000));row=sample(1000);
    check(row.reason==Reason::selection_unavailable,"cached packet does not imply a new client selection");
    for(unsigned source:{16u,255u,256u,257u,0xffffffffu}){
        route.select(1000,source,true);row=sample(1000);
        check(!row.bitmap_known&&!row.bitmap&&row.reason==Reason::unsupported_source,"unsupported device values and generation tokens never reach diagnostics");
    }
    route.select(1000,3,false);row=sample(1000,true,false);
    check(!row.bitmap_known&&!row.bitmap&&row.reason==Reason::source_unavailable,"unavailable SDK reads are unknown bitmap rather than an observed zero");
    row=sample(1000,false,false);check(row.reason==Reason::off&&row.logical_actor==4,"OFF stays unadmitted and has an explicit reason");
    ControllerRouteReporting reporting;row=route.observation(1000,false,false);
    check(reporting.due(row),"first diagnostic emits immediately");
    row.ms=1999;row.generation++;check(!reporting.due(row),"changes cannot flood telemetry inside one second");
    row.ms=2000;check(reporting.due(row),"latest changed route emits at the one-second boundary");
    row.ms=6999;check(!reporting.due(row),"unchanged status does not emit every client tick");
    row.ms=7000;check(reporting.due(row),"unchanged status refreshes after five seconds");
    row.ms=6000;row.generation++;check(!reporting.due(row),"clock rollback cannot flood diagnostic output");
    row.ms=8000;row.bitmap=16;check(!client_telemetry_valid(ClientTelemetry{row})&&!reporting.due(row),"out-of-schema bitmap is rejected by queue and throttle");
    row=route.observation(9000,false,false);row.logical_actor=1;check(!client_telemetry_valid(ClientTelemetry{row}),"no second logical actor may be claimed by telemetry");
    row=route.observation(9000,false,false);row.reason=Reason::ready;check(!client_telemetry_valid(ClientTelemetry{row}),"denied or unknown source cannot claim a ready telemetry row");
}

}
void run_controller_publication_tests();
int main(){slots();ambiguous();freshness();handoff();explicit_binding();diagnostics();run_controller_publication_tests();std::cout<<checks<<" controller route checks passed\n";}
