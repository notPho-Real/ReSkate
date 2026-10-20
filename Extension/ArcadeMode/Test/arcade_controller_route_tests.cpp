#include "Extension/ArcadeMode/arcade_controller_route.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
ArcadeInput::Physical physical(std::uint64_t now,short y=0){return {true,now,0,123,y,321,-456,0,0};}
bool passthrough(const NativeGesture::Output& out){return out.buttons==0&&out.right_x==321&&out.right_y==-456&&!out.phase;}
struct Pipeline {
    NativeControllerRoute route;ArcadeInput input{true};std::uint64_t step{},owner{1};
    Pipeline(){input.enable(true);}
    void client(std::uint64_t now,unsigned bitmap,bool focus=true){
        if(route.select(now,bitmap,focus)){input.invalidate_authority();input.observe_physical(0,{});}
        NativeInputFrame frame{};frame.valid=true;frame.owner={1,2,3,4,5};frame.owner_generation=owner;
        frame.contact_generation=frame.action_generation=1;frame.physics_step=++step;frame.sampled_at=now;
        frame.state=100;frame.mode=1;frame.contacts=0xc000;frame.flip_known=true;
        input.publish_native_context(frame);input.publish_authority(now,owner,route.physical(now).valid?1u:0u,focus);
    }
    NativeGesture::Output poll(unsigned user,std::uint64_t now,short y=0){
        const auto pad=physical(now,y);route.observe(user,pad);
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
        NativeControllerRoute route;route.observe(slot,physical(1000));route.select(1000,1u<<slot,true);
        check(route.accepts(slot,1150)&&!route.accepts(slot,1151)&&!route.accepts(slot,999),"selection and physical samples retain inclusive150ms/future bounds");
        const auto generation=route.generation();route.observe(slot,physical(1151));
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
        route.reset();route.observe(slot,physical(1000));route.select(1000,1u<<slot,true);row=sample(1000);
        check(row.bitmap_known&&row.bitmap==(1u<<slot)&&row.physical_slot==slot&&row.logical_actor==0&&row.generation==route.generation()&&row.reason==Reason::ready,
            "one observed source reports actual physical slot and single logical actor");
        row=sample(1151);check(row.reason==Reason::physical_sample_stale&&row.logical_actor==4,"old physical sample never reports route ready");
        route.observe(slot,physical(1151));row=sample(1151);
        check(row.reason==Reason::selection_stale,"new physical packet cannot conceal expired client selection");
        route.observe(slot,physical(999));row=sample(999);
        check(row.reason==Reason::selection_clock_future,"selection clock rollback has an explicit denial");
        route.observe(slot,physical(1001));row=sample(1000);
        check(row.reason==Reason::physical_clock_future,"future physical sample has an explicit denial");
    }
    for(unsigned bitmap=1;bitmap<16;++bitmap)if(bitmap&(bitmap-1)){
        route.select(1000,bitmap,true);row=sample(1000);
        check(row.bitmap_known&&row.bitmap==bitmap&&row.physical_slot==4&&row.logical_actor==4&&row.reason==Reason::multiple_connected,
            "every ambiguous source set reports exact bitmap and unselected denial");
    }
    route.reset();route.select(1000,2,true);row=sample(1000);
    check(row.reason==Reason::no_physical_sample,"sole bitmap alone is not a physical poll or admission");
    route.observe(1,physical(1000));row=sample(1000);
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
int main(){slots();ambiguous();freshness();handoff();diagnostics();run_controller_publication_tests();std::cout<<checks<<" controller route checks passed\n";}
