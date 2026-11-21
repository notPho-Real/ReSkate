#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void native_phase_recheck() {
    for(unsigned native_phase:{200u,201u}) {
        unsigned reads{},owners{};
        dingosdk::game::skater::PhysicsStateRead evidence{};
        const bool air=sample_held_flip_airborne([&](std::uint32_t& phase) noexcept {
            phase=reads++?native_phase:0;return true;
        },[&]() noexcept {++owners;return true;},&evidence);
        check(air && reads==2 && owners==2 && evidence.first==0 && evidence.state==native_phase,
            "held observation rereads transient zero once with exact owner verification");
    }
    for(unsigned contrary:{100u,103u,300u,403u,504u}) {
        unsigned reads{};
        check(!sample_held_flip_airborne([&](std::uint32_t& phase) noexcept {
            phase=reads++?contrary:0;return true;
        },[]() noexcept {return true;}) && reads==2,
            "zero reread preserves actual ground, preparation, rail, bail or offboard rejection");
    }
    for(unsigned native_phase:{200u,201u}) {
        unsigned reads{},owners{};
        check(sample_held_flip_airborne([&](std::uint32_t& phase) noexcept {
            ++reads;phase=native_phase;return true;
        },[&]() noexcept {++owners;return false;}) && reads==1 && owners==0,
            "already qualified nonzero airborne phase retains the original one-read path");
    }
    for(unsigned failure=0;failure<10;++failure) {
        unsigned reads{},owners{};
        const bool air=sample_held_flip_airborne([&](std::uint32_t& phase) noexcept {
            const auto index=reads++;phase=index?201u:0u;
            if(failure==0)phase=0;
            if(failure==1 || (failure==2 && index==1))return false;
            if(failure>=5)phase=failure==5?100u:failure==6?300u:failure==7?504u:failure==8?103u:403u;
            return true;
        },[&]() noexcept {return ++owners!=(failure==3?1u:failure==4?2u:99u);});
        check(!air,"held phase rejects persistent zero, read failure, owner or block change, ground, bail and rail");
        check(reads<=(failure==1 || failure==3 || failure>=5?1u:2u),
            "held phase rereads only zero once and never retries a nonzero contrary phase");
    }
}
void native_boundaries() {
    for (unsigned missing:{0u,1u,2u,3u,4u}) {
        HeldFlipObservation native{};native.available=true;native.sampled_at=1000;
        const auto sampled=sample_held_flip_state(native,[&](unsigned index,auto& value) noexcept {
            if(index==missing)return false;
            using T=std::remove_reference_t<decltype(value)>;
            value=static_cast<T>(index==0?2:0);return true;
        });
        native.valid=sampled;
        HeldFlipInput input;
        check(input.start(1000,2,native)==(missing>=1 && missing<=3),
            "first-use missing native flags permit gesture preparation; missing flip identity or rotation count still refuses input");
        check(!native.prepare && !native.holding && !input.acknowledged(),
            "unallocated flags cannot acknowledge a native held action or actor");
    }
    for (int id:{2,3}) {
        HeldFlipInput input;
        HeldFlipObservation native{true,true,true,false,false,id,0,1000};
        check(input.start(1000,id,native),"fresh original PrepareHeldFlip admits matching base ID");
        native.sampled_at=1034;native.holding=true;
        check(input.hold(1034,native,true),"original held entry continues input before extra rotation");
        native.sampled_at=1068;native.extra_rotations=1;
        check(!input.hold(1068,native,true) && !input.acknowledged(),"counter alone withdraws input but cannot acknowledge actor execution");
        native={true,true,true,false,false,id,0,2000};
        check(input.start(2000,id,native),"a new lifecycle can start independently");
        native.sampled_at=2034;native.holding=true;native.actor_advanced=true;
        check(input.hold(2034,native,true),"exact initialized advancing loop supports continued native ownership");
        native.sampled_at=2068;native.extra_rotations=1;
        check(!input.hold(2068,native,true) && input.acknowledged(),"one extra native rotation requests double catch");
        check(!input.hold(2102,native,true),"completed hold cannot restart itself");
        native={true,true,false,false,false,id,0,3000};
        check(input.start(3000,id,native),"second physical press can prepare the original gesture before its native admission window");
        native.sampled_at=3034;
        check(input.hold(3034,native,true),"preparation retains input while the original animation approaches its admission window");
        native.sampled_at=3100;native.prepare=true;native.holding=true;native.actor_advanced=true;
        check(input.hold(3100,native,true),"native window and original held actor admit the prepared gesture");
        native.sampled_at=3300;native.extra_rotations=1;
        check(!input.hold(3300,native,true) && input.acknowledged(),"prepared gesture still catches only at the independently observed extra rotation");
        native={true,true,false,false,false,id,0,4000};
        check(input.start(4000,id,native),"unadmitted preparation starts bounded input only");
        native.sampled_at=4201;
        check(!input.hold(4201,native,true) && !input.acknowledged(),"missing native window withdraws preparation at the input deadline");
        native={true,true,true,false,false,id,0,5000};
        check(input.start(5000,id,native),"late physical repetition starts only matching native input");
        native.sampled_at=5088;native.prepare=false;
        check(input.hold(5088,native,true) && !input.acknowledged(),
            "the observed short Prepare window cannot prematurely withdraw an unfinished gesture or acknowledge admission");
        native.sampled_at=5201;
        check(!input.hold(5201,native,true) && !input.acknowledged(),
            "seeing a Prepare window cannot grant an unadmitted gesture the longer held-actor lifetime");
    }
    for (unsigned failure=0;failure<9;++failure) {
        HeldFlipInput input;HeldFlipObservation native{true,true,true,false,false,2,0,1000};
        check(input.start(1000,2,native),"negative control starts a valid request");
        native.sampled_at=1034;bool flight=true;std::uint64_t now=1034;
        if(failure==0)native.valid=false;
        if(failure==1)native.id=3;
        if(failure==2)flight=false;
        if(failure==3)now=999;
        if(failure==4)native.sampled_at=800;
        if(failure==5){native.prepare=false;now=1201;native.sampled_at=1201;}
        if(failure==6)native.extra_rotations=2;
        if(failure==7)native.extra_rotations=std::nanf("");
        if(failure==8){now=2201;native.sampled_at=2201;}
        check(!input.hold(now,native,flight) && !input.acknowledged(),"unknown/stale/foreign/landed/overshot/timed-out native evidence withdraws without false double acknowledgment");
    }
}
struct Route {
    InputSession session;NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
    InputAdmission admission{};NativeInputFrame frame{};unsigned bridge_calls{},repeats{};
    std::int32_t requested{};std::vector<PreparedGesture> recipes;bool held_available{true},bridge_accept{true},switched{};
    InputRequest repeated_request{},third_request{};
    bool one_shot_bridge{};
    std::uintptr_t selection_object{16};
    bool transient_phase_zero{};
    dingosdk::NativeOrientation native_orientation{true,false};
    Route() {
        AuthoredGesture kick{2,false,GestureContext::ground,90,180,.85f,{{280,30,.5f,1,0},{90,150,.5f,1,.1f}}};
        AuthoredGesture heel{3,false,GestureContext::ground,180,270,.85f,{{330,80,.5f,1,0},{210,270,.5f,1,.1f}}};
        const std::array catalog{kick,heel};recipes=prepare_gestures(catalog);
        recipes.emplace_back(std::uint8_t{1},false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}});
        admission.gameplay=admission.authored=admission.action_fresh=true;
        admission.may_start=admission.inspected_stance=true;admission.action_mode=ActionExperiment::classic;
        admission.catalog=recipes;admission.held_flip.available=true;
        admission.normal_flip={this,[](void* p,std::int32_t id) noexcept {
            auto& self=*static_cast<Route*>(p);if(!self.bridge_accept || (self.one_shot_bridge && self.bridge_calls))return false;
            ++self.bridge_calls;self.requested=id;return true;
        }};
    }
    NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample physical={},std::uint32_t state=201,std::int32_t id=2,
                               bool prepare=true,float extra=0,bool held=false,bool actor=false,int animation_id=-999,bool advance=true) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:(state==200 || state==103)?0:2;
        sample.words[1]=state==100?0xc000u:0;
        sample.words[0]=switched?0x20u:0;
        if(advance)frame=producer.observe(now,owner,sample,{true,id,selection_object},false,0,native_orientation);
        admission.native_context=InputNativeContext::from_frame(frame);
        admission.pose=frame.airborne?SkatePose::air:SkatePose::riding;
        admission.flip_ready=id==-1;admission.tick=now*60/1000;
        unsigned phase_reads{};
        const bool held_air=sample_held_flip_airborne([&](std::uint32_t& phase) noexcept {
            phase=transient_phase_zero && phase_reads++==0?0:state;return true;
        },[]() noexcept {return true;});
        admission.held_flip={held_available,frame.airborne && held_air,prepare,held,actor,animation_id==-999?id:animation_id,extra,now};
        const auto out=session.poll(now,physical,0,0,admission);
        const auto& d=session.diagnostic();
        for(std::size_t i=0;i<d.dispatched;++i)
            if(d.dispatch[i].request.form==InputForm::held_repeat && d.dispatch[i].disposition==InputDisposition::started) {
                ++repeats;
                if(d.dispatch[i].request.presses==3)third_request=d.dispatch[i].request;
                else repeated_request=d.dispatch[i].request;
            }
        return out;
    }
    void first(std::int16_t direction=0) {
        poll(1000,{},100,-1);poll(1017,{0x1000},100,-1);poll(1051,{0x1000},100,1);
        poll(1085,{},100,1);poll(1102,{},200,1);poll(1119,{},201,1);poll(1136,{},201,1);
        poll(1153,{0x4000,direction,0},201,1);
        check(bridge_calls==1 && requested==(direction>0?3:2),"first physical X retains the original single native bridge");
        poll(1170,{},201,requested,false);
    }
};
void physical_triples() {
    for(unsigned hz:{30u,60u,120u})for(unsigned stage:{0u,1u,2u})
    for(std::int16_t direction:{std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) {
        auto h=std::make_unique<Route>();h->one_shot_bridge=true;h->bridge_accept=stage==2;h->switched=switched;
        const auto dt=(1000u+hz-1)/hz;const int id=direction>0?3:2;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        if(stage) {h->poll(1102,{},200,1,false);h->poll(1119,{},201,1,false);}
        const auto first=stage?1153u:1102u,second=first+2*dt,third=first+4*dt;
        const auto phase=stage?201u:103u;
        h->poll(first,{0x4000,direction,0},phase,1,false,0,false,false,id);
        h->poll(first+dt,{},phase,1,false,0,false,false,id);
        h->poll(second,{0x4000},phase,1,false,0,false,false,id);
        h->poll(second+dt,{},phase,1,stage==2,0,stage==2,stage==2,id);
        h->poll(third,{0x4000},phase,1,stage==2,0,stage==2,stage==2,id);
        if(!stage)h->poll(third+dt,{},200,1,false,0,false,false,id);
        h->bridge_accept=true;
        for(unsigned n=1;n<=4;++n)h->poll(third+(n+1)*dt,{},201,1,false,0,stage==2,stage==2,id);
        check(h->bridge_calls==1 && h->repeats==2 && h->third_request.presses==3,
            "third physical X upgrades the existing held owner without a second native base request");
        check(h->third_request.parent_input_sequence==h->repeated_request.input_sequence &&
            h->third_request.input_sequence>h->repeated_request.input_sequence &&
            h->third_request.physical_pressed_at==third && h->third_request.deadline==third+InputTiming::readiness_ms &&
            !h->third_request.unbounded,
            "third edge retains its exact second parent and immutable physical admission deadline");
        auto out=h->poll(third+6*dt,{},201,1,true,1,true,true,id);
        check(out.authored_playback && !out.held_input_released && !out.held_actor_acknowledged,
            "upgraded held input keeps native ownership through the first extra rotation");
        out=h->poll(third+7*dt,{},201,1,true,2,true,true,id);
        check(out.held_input_released && out.held_actor_acknowledged && !out.authored_playback,
            "triple catches at two independently observed extra rotations and original actor advancement");
        h->poll(third+8*dt,{},100,-1,false,2,false,false,id);
        check(h->bridge_calls==1 && h->repeats==2,"landing cannot replay a consumed triple request");
    }
}
void triple_boundaries() {
    for(bool entered:{false,true}) {
        HeldFlipInput input;HeldFlipObservation native{true,true,false,false,false,2,0,1000};
        check(input.start(1000,2,native),"triple boundary begins the existing double gesture");
        native.sampled_at=1050;native.holding=entered;native.actor_advanced=entered;
        check(input.hold(1050,native,true),"entry observation belongs to the original hold");
        native.sampled_at=1100;
        check(input.extend(1100,2,native,true),"third input can upgrade admitted preparation or active native hold");
        check(!input.extend(1100,2,native,true),"duplicate or fourth input cannot upgrade again");
        const auto end=entered?2201u:1201u;native.sampled_at=end;
        check(!input.hold(end,native,true) && !input.acknowledged(),
            "third input never renews the original200ms entry or1200ms held lifetime");
    }
    for(unsigned fault=0;fault<9;++fault) {
        HeldFlipInput input;HeldFlipObservation native{true,true,false,false,false,2,0,1000};
        check(input.start(1000,2,native),"third rejection preserves a valid double start");
        native.sampled_at=1034;native.holding=native.actor_advanced=true;
        check(input.hold(1034,native,true),"third rejection starts from original native actor ownership");
        auto third=native;third.sampled_at=1068;
        if(fault==0)third.available=false;
        if(fault==1)third.valid=false;
        if(fault==2)third.id=3;
        if(fault==3)third.sampled_at=900;
        if(fault==4)third.extra_rotations=1;
        if(fault==5)third.extra_rotations=std::nanf("");
        if(fault==6)third.holding=false;
        check(!input.extend(fault==7?999:1068,2,third,fault!=8),
            "unqualified third cannot extend wrong, stale, caught, ended or foreign native input");
        native.sampled_at=1102;native.extra_rotations=1;
        check(!input.hold(1102,native,true) && input.acknowledged(),
            "rejected extension leaves the original double catch unchanged");
    }
    for(float observed:{2.0f,3.0f})for(bool actor:{false,true}) {
        HeldFlipInput input;HeldFlipObservation native{true,true,false,false,false,2,0,1000};
        check(input.start(1000,2,native) && input.extend(1000,2,native,true),"native triple result has an admitted target only");
        native.sampled_at=1034;native.holding=true;native.actor_advanced=actor;native.extra_rotations=observed;
        check(!input.hold(1034,native,true) && input.acknowledged()==(actor && observed==2),
            "target count without actor or an overshot rotation cannot acknowledge an exact triple");
    }
    // The existing timely-parent/successor exception belongs to the double.
    // Its queued third has an independent half-open physical200ms deadline.
    for(unsigned delivery:{1369u,1370u}) {
        auto h=std::make_unique<Route>();h->one_shot_bridge=true;h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{0x4000,-25000},103,1,false);h->poll(1119,{},103,1,false);
        h->poll(1136,{0x4000},103,1,false);h->poll(1153,{},103,1,false);
        h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},103,1,false);
        h->poll(1250,{},200,1,false);h->poll(1286,{},201,1,false);
        h->bridge_accept=true;h->poll(1320,{},201,1,false);
        h->poll(1354,{},201,1,false,0,false,false,2);
        check(h->repeats==1 && h->bridge_calls==1,"existing double retains its one successor beyond original second deadline");
        h->poll(delivery,{},201,1,true,0,true,true,2);
        check(h->repeats==(delivery==1369?2u:1u),"third expires at its own physical200ms deadline without borrowing double dependency");
        const auto out=h->poll(delivery+17,{},201,1,true,1,true,true,2);
        check(out.held_input_released==(delivery==1370) && out.held_actor_acknowledged==(delivery==1370),
            "expired third preserves double catch while timely third keeps its original native hold");
    }
}
void triple_cancellation() {
    for(unsigned fault=0;fault<18;++fault) {
        auto h=std::make_unique<Route>();h->one_shot_bridge=true;
        h->first(fault==11?0:-25000);
        h->poll(1187,{0x4000},201,2,false,0,false,false,2);
        h->poll(1204,{},201,2,true,0,true,true,2);
        check(h->repeats==1,"third negative starts with a live original double owner");
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.gameplay=false;
        if(fault==2)h->admission.external_owner=true;
        if(fault==3)h->switched=true;
        if(fault==4)h->selection_object=32;
        if(fault==5)h->held_available=false;
        if(fault==15)h->admission.action_mode=ActionExperiment::off;
        const auto at=fault==16?1356u:1221u;
        PhysicalInputSample pad{static_cast<std::uint16_t>(fault==8?0x5000: fault==12?0x6000:0x4000)};
        if(fault==13)pad.lt=255;
        if(fault==14)pad.rt=255;
        const auto out=h->poll(at,pad,fault==6?100:fault==7?300:201,fault==17?4:2,true,
            fault==9?1.0f:0.0f,true,true,fault==10?3:2,fault!=16);
        check(!h->third_request.input_sequence && h->repeats==1,
            "third rejects changed owner, focus, conflict, stance, object, observer, landing, bail, new A, catch, wrong actor, neutral binding, buttons and stale state");
        if(fault==9)check(out.held_input_released && out.held_actor_acknowledged,
            "count1 in the third-edge poll catches the established double before any extension");
        if(fault==11) {
            const auto caught=h->poll(at+17,{},201,2,true,1,true,true,2);
            check(caught.held_input_released && caught.held_actor_acknowledged,"unmapped neutral third leaves native double unchanged");
        }
    }
    auto h=std::make_unique<Route>();h->one_shot_bridge=true;h->first(-25000);
    h->poll(1187,{0x4000},201,2,false,0,false,false,2);h->poll(1204,{},201,2,true,0,true,true,2);
    h->poll(1221,{0x4000},201,2,true,0,true,true,2);h->poll(1238,{},201,2,true,0,true,true,2);
    h->poll(1255,{0x4000},201,2,true,0,true,true,2);
    check(h->repeats==2 && h->bridge_calls==1,"fourth X cannot start another held owner or native base");
    h->poll(1272,{},201,2,true,1,true,true,2);
    const auto caught=h->poll(1289,{},201,2,true,2,true,true,2);
    check(caught.held_input_released && caught.held_actor_acknowledged,"fourth X cannot delay the exact triple catch");
}
void queued_first_repetition() {
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) for(bool physics_retired:{false,true})
    for(unsigned first_phase:{103u,200u,201u}) for(bool idle_retired:{false,true}) {
        auto h=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        h->switched=switched;h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},first_phase==103?103:200,1);h->poll(1119,{},first_phase,1,false);
        h->poll(1119+dt,{0x4000,direction,0},first_phase,1,false);
        const auto queued_selection=idle_retired?-1:1;
        h->poll(1119+2*dt,{},201,queued_selection,false);
        const auto second=1119+3*dt;
        h->poll(second,{0x4000,static_cast<std::int16_t>(direction?0:25000),0},201,queued_selection,false);
        h->poll(1119+4*dt,{},201,queued_selection,false);
        check(!h->bridge_calls && !h->repeats,"queued physical X pair cannot bypass the first native flip admission");
        h->bridge_accept=true;h->poll(1119+5*dt,{},201,queued_selection,false);
        const auto id=direction>0?3:2;
        check(h->bridge_calls==1 && h->requested==id && !h->repeats,
              "waiting pair first admits exactly the requested original single flip");
        const auto physics_id=physics_retired?id:queued_selection;
        const auto out=h->poll(1119+6*dt,{},201,physics_id,false,0,false,false,id);
        check(h->repeats==1 && out.authored_playback && h->bridge_calls==1,
              "second physical X before first admission prepares the matching held native flip");
        check(h->repeated_request.physical_pressed_at==second &&
              h->repeated_request.physical_released_at==0 &&
              h->repeated_request.deadline==second+InputTiming::readiness_ms && !h->repeated_request.unbounded,
              "queued held repeat retains its recognition-edge timestamp and immutable deadline");
        h->poll(1119+7*dt,{},201,physics_id,true,0,true,true,id);
        const auto caught=h->poll(1119+8*dt,{},201,physics_id,true,1,true,true,id);
        check(caught.held_input_released && caught.held_actor_acknowledged && !caught.authored_playback,
              "queued repetition releases only at independently supplied held actor and extra rotation");
        h->poll(1119+9*dt,{},201,physics_id,false,1,false,false,id);
        check(h->bridge_calls==1 && h->repeats==1,"queued pair never replays another pop, flip or held request");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned fault=0;fault<10;++fault) {
        auto h=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},200,1);h->poll(1119,{},201,1,false);
        h->poll(1119+dt,{0x4000},201,1,false);h->poll(1119+2*dt,{},201,1,false);
        h->poll(1119+3*dt,{0x4000},201,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.external_owner=true;
        if(fault==2)h->admission.gameplay=false;
        if(fault==5)h->switched=true;
        if(fault==6)h->selection_object=32;
        if(fault==7)h->held_available=false;
        h->poll(1119+4*dt,{},fault==3?100:fault==4?300:201,1,false);
        h->bridge_accept=true;h->poll(1119+5*dt,{},201,1,false);
        for(unsigned n=6;n<12;++n)h->poll(1119+n*dt,{},201,1,fault==8,0,false,false,fault==9?3:2);
        check(!h->repeats,
              "queued repeat rejects owner, conflict, focus, landing, bail, stance, selector, missing observer, late Prepare or wrong actor");
    }
    for(bool late_first:{false,true})for(unsigned delay:{199u,200u}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},200,1);h->poll(1119,{},201,1,false);
        h->poll(1136,{0x4000},201,1,false);h->poll(1153,{},201,1,false);
        h->poll(1170,{0x4000},201,1,false);h->poll(1187,{},201,1,false);
        if(!late_first)h->bridge_accept=true;
        h->poll(1204,{},201,1,false);h->poll(1250,{},201,1,false);
        h->poll(1300,{},201,1,false);h->bridge_accept=true;
        h->poll(1336,{},201,1,false);h->poll(1170+delay,{},201,1,false,0,false,false,2);
        check(h->repeats==(delay==199?1u:0u) && h->bridge_calls==1,
              "first admission retains its ordinary policy without renewing the queued repeat's original deadline");
    }
}
void preair_queued_repetition() {
    // The recorded native launch remains in setup until about release+220ms.
    // Both real X edges can therefore arrive after the old 160ms preload but
    // before AIR. Preserve each edge instead of recovering only the latest X.
    for(unsigned hz:{30u,60u,120u})
    for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;h->switched=switched;
        const auto dt=(1000u+hz-1)/hz;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1136,{},100,1);h->poll(1204,{},103,1);
        h->poll(1255,{0x4000,direction,0},103,1,false);
        h->poll(1255+dt,{},103,1,false);
        const auto second=1255+2*dt;h->poll(second,{0x4000},103,1,false);
        check(!h->bridge_calls && !h->repeats,"late setup X pair waits for independently observed AIR");
        h->poll(second+dt,{},200,1,false);h->bridge_accept=true;
        h->poll(second+2*dt,{},201,1,false);
        const auto id=direction>0?3:2;
        const auto out=h->poll(second+3*dt,{},201,1,false,0,false,false,id);
        check(h->bridge_calls==1 && h->requested==id && h->repeats==1 && out.authored_playback,
              "X pair after160ms retains both physical edges through the same native pop");
        check(h->repeated_request.physical_pressed_at==second &&
              h->repeated_request.deadline==second+InputTiming::readiness_ms &&
              h->repeated_request.pop_release_at==1085 && !h->repeated_request.unbounded,
              "late setup pair retains the exact original repeat deadline and released pop");
    }
    for(unsigned fault=0;fault<13;++fault) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        if(fault==12)h->admission.normal_flip={};
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1136,{},100,1);h->poll(1204,{},103,1);
        h->poll(1255,{0x4000},103,1,false);h->poll(1272,{},103,1,false);
        h->poll(1289,{0x4000},103,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.gameplay=false;
        if(fault==2)h->admission.external_owner=true;
        if(fault==3)h->switched=true;
        if(fault==4)h->selection_object=32;
        if(fault==5)h->held_available=false;
        if(fault==11) {
            for(unsigned at=1323;at<1585;at+=34)h->poll(at,{},103,1,false);
            h->poll(1585,{},103,1,false); // Original pop has expired at500ms.
        }
        const auto air=fault==11?1602u:1306u;
        h->poll(air,{},200,1,false);
        h->poll(air+17,fault==8?PhysicalInputSample{0x1000}:PhysicalInputSample{},
            (fault==6 || fault==8)?100:fault==7?300:201,1,false);
        h->bridge_accept=true;
        h->poll(air+34,{},201,1,fault==9);
        h->poll(air+51,{},201,1,fault==9,0,false,false,fault==10?3:2);
        check(!h->repeats,"late setup pair preserves owner, focus, conflict, stance, selector, native actor, landing, bail, new A, Prepare and pop expiry gates");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned phase:{100u,103u})
    for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;h->switched=switched;
        const auto dt=(1000u+hz-1)/hz;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1085+dt,{0x4000,direction,0},phase,1,false);
        h->poll(1085+2*dt,{},phase,1,false);
        const auto second=1085+3*dt;
        h->poll(second,{0x4000},phase,1,false);
        check(!h->bridge_calls && !h->repeats,"pre-AIR pair cannot execute before native takeoff");
        h->poll(second+dt,{},200,1,false);h->bridge_accept=true;
        h->poll(second+2*dt,{},201,1,false);
        const auto id=direction>0?3:2;
        const auto out=h->poll(second+3*dt,{},201,1,false,0,false,false,id);
        check(h->bridge_calls==1 && h->requested==id && h->repeats==1 && out.authored_playback,
              "two pre-AIR X edges on the same released pop prepare the original held flip after takeoff");
        check(h->repeated_request.pop_release_at==1085 &&
              h->repeated_request.physical_pressed_at==second && !h->repeated_request.unbounded &&
              h->repeated_request.deadline==second+InputTiming::readiness_ms,
              "pre-AIR repeat keeps exact pop ownership and second-edge deadline");
        h->poll(second+4*dt,{},201,1,true,0,true,true,id);
        const auto caught=h->poll(second+5*dt,{},201,1,true,1,true,true,id);
        check(caught.held_actor_acknowledged && caught.held_input_released,
              "pre-AIR repeat completion still requires independent native held actor and rotation evidence");
    }
    for(unsigned delay:{199u,200u,222u}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
        const auto second=1170u;h->poll(second,{0x4000},103,1,false);
        h->poll(1204,{},200,1,false);h->bridge_accept=true;
        h->poll(1238,{},201,1,false);h->poll(1288,{},201,1,false);
        h->poll(1338,{},201,1,false);
        h->poll(second+delay,{},201,1,false,0,false,false,2);
        check(h->bridge_calls==1 && h->repeats==(delay==199?1u:0u),
              "ground preload cannot renew the repeat at200ms or a longer222ms native base delay");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned fault=0;fault<11;++fault) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        const auto dt=(1000u+hz-1)/hz;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1085+dt,{0x4000},103,1,false);
        h->poll(1085+2*dt,{},103,1,false);
        const auto second=1085+3*dt;h->poll(second,{0x4000},103,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.gameplay=false;
        if(fault==2)h->admission.external_owner=true;
        if(fault==3)h->switched=true;
        if(fault==4)h->selection_object=32;
        if(fault==5)h->held_available=false;
        h->poll(second+dt,{},200,1,false);
        h->poll(second+2*dt,fault==8?PhysicalInputSample{0x1000}:PhysicalInputSample{},
                (fault==6 || fault==8)?100:fault==7?300:201,1,false);
        h->bridge_accept=true;
        h->poll(second+3*dt,{},201,1,fault==9);
        for(unsigned n=4;n<9;++n)h->poll(second+n*dt,{},201,1,fault==9,0,false,false,fault==10?3:2);
        check(!h->repeats,
              "pre-AIR repeat rejects owner, focus, conflict, stance, selection, observer, landing, bail, new pop, late Prepare and wrong actor");
    }
    {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);
        // A remains held throughout: the X taps cannot invent its release.
        h->poll(1102,{0x5000},103,1,false);h->poll(1119,{0x1000},103,1,false);
        h->poll(1136,{0x5000},103,1,false);h->poll(1153,{0x1000},200,1,false);
        h->bridge_accept=true;h->poll(1170,{0x1000},201,1,false);
        h->poll(1187,{0x1000},201,1,false,0,false,false,2);
        check(!h->repeats,"ground X pair cannot invent a qualified pop release");
    }
}
void preair_successor_dependency() {
    auto h=std::make_unique<Route>();h->bridge_accept=false;
    h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
    h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
    h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
    h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},200,1,false);
    h->poll(1238,{},201,1,false);h->poll(1272,{},201,1,false);
    h->poll(1304,{},201,1,false);h->poll(1336,{},201,1,false);
    h->bridge_accept=true;h->poll(1360,{},201,1,false);
    check(h->bridge_calls==1 && !h->repeats,"late qualified parent starts before the child deadline");
    const auto out=h->poll(1394,{},201,1,false,0,false,false,2);
    check(h->repeats==1 && out.authored_playback,
          "pre-AIR queued repeat survives one native successor after parent admission at190ms");
    check(h->repeated_request.physical_pressed_at==1170 && h->repeated_request.deadline==1370 &&
          !h->repeated_request.unbounded && !out.held_actor_acknowledged,
          "dependency admission retains original edge deadline and grants no native completion");
}

void preair_late_parent_readiness() {
    auto h=std::make_unique<Route>();h->bridge_accept=false;
    h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
    h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
    h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
    h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},103,1,false);
    h->admission.action_fresh=false;
    h->poll(1304,{},103,1,false);h->poll(1324,{},200,1,false);
    h->poll(1358,{},201,1,false);
    check(!h->bridge_calls && !h->repeats,"physical pair waits for independently fresh first-flip readiness");
    h->admission.action_fresh=true;h->bridge_accept=true;
    h->poll(1381,{},201,1,false);
    check(h->bridge_calls==1 && !h->repeats,"late first native readiness admits only the matching base");
    const auto out=h->poll(1415,{},201,1,false,0.0f,false,false,2);
    check(h->repeats==1 && out.authored_playback,
          "pre-AIR pair survives first native readiness after the physical repeat deadline");
    check(h->repeated_request.physical_pressed_at==1170 && h->repeated_request.deadline==1370 &&
          !h->repeated_request.unbounded && !out.held_actor_acknowledged,
          "fixed first-AIR dependency retains physical deadline and grants no completion");
}

struct DependencyQueue {
    InputRequestQueue queue;
    InputRequest parent{},child{};
    InputNativeContext native{};
    unsigned bases{},repeats{},waiting{},expired{},mutation{};
    bool wait_child{},wait_parent{};
    DependencyQueue() {
        native.required=native.valid=native.airborne=true;
        native.generations={1,2,3};native.physics_step=10;native.sampled_at=1360;
        native.airborne_episode=1;native.flip={true,1,16,201,2,0,10,1,1};
        native.orientation={true,false};
        parent.action=InputAction::flip;parent.context=InputContext::ground;parent.session=1;
        parent.form=InputForm::neutral_press;parent.air.trick=ArcadeTrick::kickflip;
        parent.physical_pressed_at=1102;parent.pop_release_at=1085;parent.input_sequence=10;
        parent.generations=native.generations;parent.generation_mask=1;parent.unbounded=true;
        child=parent;child.form=InputForm::held_repeat;child.air=air_selection(TrickFamily::flip,4,2);
        child.physical_pressed_at=1170;child.input_sequence=20;child.parent_input_sequence=10;
        child.presses=2;child.deadline=1370;child.unbounded=false;
    }
    void enqueue(bool include_child=true) {check(queue.push(parent),"queue parent");if(include_child)check(queue.push(child),"queue exact child");}
    void dispatch(std::uint64_t now) {
        queue.dispatch(now,1,InputContext::air,{this,[](void* p,const InputRequest& r) noexcept {
            auto& q=*static_cast<DependencyQueue*>(p);
            if(r.presses==1) {
                if(q.wait_parent)return InputDisposition::waiting;
                ++q.bases;
                if(q.mutation==1)q.queue.cancel(InputAction::flip);
                if(q.mutation==2)q.queue.clear_ordinary();
                if(q.mutation==3)q.queue.clear();
                if(q.mutation>=1 && q.mutation<=4)q.queue.push(q.child);
                return InputDisposition::started;
            }
            if(q.wait_child){++q.waiting;return InputDisposition::waiting;}
            ++q.repeats;return InputDisposition::started;
        }},[&](const InputDispatchEvent& e) {
            if(e.disposition==InputDisposition::expired)++expired;
            if(mutation==5 && e.disposition==InputDisposition::started && e.request.presses==1) {
                queue.cancel(InputAction::flip);queue.push(child);
            }
        },native,false,{},1085);
    }
    void next(std::uint64_t now) {++native.physics_step;++native.flip.step;native.sampled_at=now;}
};
void late_parent_dependency_boundaries() {
    const auto prepare=[](DependencyQueue& q,std::uint64_t start=1324) {
        q.native.airborne_started_known=true;q.native.airborne_started_at=start;
        q.native.sampled_at=1381;
    };
    for(unsigned age:{149u,150u,151u}) {
        DependencyQueue q;prepare(q);q.native.sampled_at=1324+age;q.enqueue();q.dispatch(1324+age);
        q.next(1324+age);q.dispatch(1324+age);
        check(q.repeats==(age==149?1u:0u),"late parent first-AIR lease is half-open150ms at acceptance and successor");
    }
    for(unsigned gap:{499u,500u}) {
        DependencyQueue q;prepare(q,1085+gap);q.native.sampled_at=1135+gap;q.enqueue();q.dispatch(1135+gap);
        q.next(1169+gap);q.dispatch(1169+gap);
        check(q.repeats==(gap==499?1u:0u),"late parent requires first AIR inside original500ms released-pop corridor");
    }
    for(unsigned successor:{1473u,1474u}) {
        DependencyQueue q;prepare(q);q.enqueue();q.dispatch(1381);
        for(unsigned now:{1382u,1400u,1440u})q.dispatch(now);
        check(!q.repeats,"same-step polling never admits or renews late dependency");
        q.next(successor);q.dispatch(successor);
        check(q.repeats==(successor==1473?1u:0u),"successor is capped by original first AIR rather than parent acceptance");
    }
    for(unsigned fault=0;fault<32;++fault) {
        DependencyQueue q;prepare(q);
        if(fault==0)q.native.airborne_started_known=false;
        if(fault==1)q.native.airborne_started_at=1382;
        if(fault==2)q.native.airborne_started_at=1084;
        if(fault==3)q.parent.physical_pressed_at=1084;
        if(fault==4)q.child.physical_pressed_at=1325;
        if(fault==5)q.child.parent_input_sequence=11;
        if(fault==6)q.child.session=2;
        if(fault==7)q.child.pop_release_at=1086;
        if(fault==8)q.child.generations.owner=2;
        if(fault==9)q.child.air_episode={2,0};
        if(fault==10)q.child.context=InputContext::air;
        if(fault==11)q.parent.context=InputContext::air;
        if(fault==12)q.native.orientation.valid=false;
        if(fault==13)q.native.airborne_started_at=0;
        if(fault==14)q.native.sampled_at=1323;
        q.enqueue(fault!=15);q.dispatch(1381);q.next(1415);
        if(fault==16)q.native.airborne_started_at=1358;
        if(fault==17)q.native.airborne_started_at=1300;
        if(fault==18)q.native.airborne_started_known=false;
        if(fault==19)++q.native.physics_step;
        if(fault==20)++q.native.generations.owner;
        if(fault==21)++q.native.airborne_episode;
        if(fault==22)++q.native.air_departure;
        if(fault==23)q.native.flip.object=32;
        if(fault==24)q.native.flip.stance=0x20;
        if(fault==25)q.native.orientation.fakie=true;
        if(fault==26)q.native.sampled_at=1380;
        if(fault==27)q.native.flip.state=300;
        if(fault==28)q.native.airborne=false;
        if(fault==29)q.native.flip.id=3;
        if(fault==30)q.native.valid=false;
        if(fault==31)q.native.orientation.valid=false;
        q.dispatch(1415);
        check(!q.repeats,"late parent rejects missing/changed timing, cancellation and foreign native ownership");
    }
    {
        DependencyQueue q;prepare(q);q.enqueue();q.wait_parent=true;q.dispatch(1381);
        q.next(1474);q.wait_parent=false;q.dispatch(1474);q.next(1508);q.dispatch(1508);
        check(q.bases==1 && !q.repeats,"waiting parent cannot reserve or renew the first-AIR lease");
    }
    {
        DependencyQueue q;prepare(q);q.enqueue();q.dispatch(1381);q.wait_child=true;q.next(1415);q.dispatch(1415);
        q.wait_child=false;q.next(1449);q.dispatch(1449);
        check(q.waiting==1 && q.expired==1 && !q.repeats,"late-parent successor waiting consumes the only chance");
    }
    {
        DependencyQueue q;prepare(q);q.enqueue();q.dispatch(1381);q.dispatch(1380);q.next(1415);q.dispatch(1415);
        check(!q.repeats,"late dependency retires on clock rollback without recovery replay");
    }
    for(unsigned mutation:{1u,2u,3u,4u,5u}) {
        DependencyQueue q;prepare(q);q.mutation=mutation;q.enqueue(mutation!=4);q.dispatch(1381);
        q.next(1415);q.dispatch(1415);
        check(!q.repeats,"late parent cannot grant from reentrant insert or cancellation");
    }
}

void successor_dependency_boundaries() {
    for(unsigned first_age:{190u,196u,199u})for(unsigned dt:{9u,17u,34u}) {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1170+first_age;q.dispatch(1170+first_age);
        q.next(1170+first_age+dt);q.dispatch(1170+first_age+dt);
        check(q.bases==1 && q.repeats==1,"only exact first successor can carry timely accepted parent dependency");
    }
    for(unsigned age:{199u,200u,222u}) {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1170+age;q.dispatch(1170+age);
        q.next(1204+age);q.dispatch(1204+age);
        check(q.repeats==(age==199?1u:0u),"parent at or beyond immutable second-edge deadline cannot grant allowance");
    }
    for(unsigned delay:{150u,151u}) {
        DependencyQueue q;q.enqueue();q.dispatch(1360);
        for(unsigned now:{1370u,1390u,1420u,1460u})q.dispatch(now);
        check(q.repeats==0,"same accepted native sample cannot execute beyond physical deadline");
        q.next(1360+delay);q.dispatch(1360+delay);
        check(q.repeats==(delay==150?1u:0u),"original native sample lease is inclusive150ms and never refreshed by polls");
    }
    for(unsigned fault=0;fault<23;++fault) {
        DependencyQueue q;
        if(fault==0)q.child.parent_input_sequence=11;
        if(fault==1)q.child.input_sequence=0;
        if(fault==2)q.child.pop_release_at=1086;
        if(fault==3)q.child.air=air_selection(TrickFamily::flip,6,2);
        if(fault==4)q.child.nollie=true;
        if(fault==5)q.child.presses=3;
        if(fault==6)q.child.context=InputContext::air;
        if(fault==7)q.parent.form=InputForm::repeated_direction;
        if(fault==20)q.native.orientation.valid=false;
        q.enqueue();q.dispatch(1360);q.next(1394);
        if(fault==8)++q.native.physics_step;
        if(fault==9)++q.native.generations.owner;
        if(fault==10)++q.native.airborne_episode;
        if(fault==11)q.native.flip.object=32;
        if(fault==12)q.native.flip.stance=0x20;
        if(fault==13)q.native.orientation.fakie=true;
        if(fault==14)q.native.sampled_at=1359;
        if(fault==15)q.native.flip.state=300;
        if(fault==16)q.native.airborne=false;
        if(fault==17)q.native.flip.id=3;
        if(fault==18)q.native.valid=false;
        if(fault==19)q.native.orientation.valid=false;
        if(fault==21)--q.native.generations.contact;
        if(fault==22)--q.native.physics_step; // A changed timestamp on the same step is not a new native sample.
        q.dispatch(1394);
        check(q.repeats==0,"invalid physical parent/native successor cannot borrow dependency admission");
    }
    {
        DependencyQueue q;q.native.orientation={};q.enqueue();q.native.sampled_at=1238;q.dispatch(1238);
        q.next(1272);q.dispatch(1272);
        check(q.repeats==1,"ordinary timely admission does not require the exceptional orientation proof");
    }
    {
        DependencyQueue q;q.enqueue();q.dispatch(1360);q.next(1394);q.wait_child=true;q.dispatch(1394);
        check(q.waiting==1 && q.expired==1,"waiting at the one successor opportunity consumes it without native credit");
        q.wait_child=false;q.dispatch(1395);q.next(1428);q.dispatch(1428);
        check(!q.repeats,"waiting successor never retries even with newly ready actor");
    }
    {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1238;q.dispatch(1238);
        q.wait_child=true;q.next(1272);q.dispatch(1272);
        q.wait_child=false;q.native.sampled_at=1370;q.dispatch(1370);
        check(q.waiting==1 && !q.repeats,"early parent consumes its successor before deadline and cannot renew allowance later");
    }
    {
        DependencyQueue q;q.enqueue();q.dispatch(1360);q.wait_child=true;q.dispatch(1359);
        q.wait_child=false;q.next(1394);q.dispatch(1394);
        check(!q.repeats,"clock rollback retires dependency even if original request later becomes current");
    }
    for(unsigned mutation:{1u,2u,3u,4u,5u}) {
        DependencyQueue q;q.mutation=mutation;q.enqueue(mutation!=4);q.dispatch(1360);
        q.next(1394);q.dispatch(1394);
        check(!q.repeats,"adapter/report cancellation, clear or reentrant insertion cannot fabricate a preexisting child");
    }
}
void composed_successor_negatives() {
    for(bool late:{false,true})for(unsigned fault=0;fault<15;++fault) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
        h->poll(1085,{},100,1);h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
        h->poll(1170,{0x4000},103,1,false);
        if(late) {
            h->poll(1204,{},103,1,false);h->admission.action_fresh=false;
            h->poll(1304,{},103,1,false);h->poll(1324,{},200,1,false);h->poll(1358,{},201,1,false);
            h->admission.action_fresh=true;h->bridge_accept=true;h->poll(1381,{},201,1,false);
        } else {
            h->poll(1204,{},200,1,false);h->poll(1238,{},201,1,false);
            h->poll(1272,{},201,1,false);h->poll(1304,{},201,1,false);
            h->poll(1336,{},201,1,false);h->bridge_accept=true;h->poll(1360,{},201,1,false);
        }
        const auto successor=late?1415u:1394u;
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.external_owner=true;
        if(fault==2)h->admission.gameplay=false;
        if(fault==5)h->switched=true;
        if(fault==6)h->selection_object=32;
        if(fault==7)h->held_available=false;
        if(fault==13)h->native_orientation.fakie=true;
        if(fault==14)h->native_orientation.valid=false;
        PhysicalInputSample input{};if(fault==11)input.lt=255;if(fault==12)input.buttons=0x8000;
        const auto out=h->poll(successor,input,fault==3?100:fault==4?300:201,1,fault==8,fault==10?1.0f:0.0f,false,false,fault==9?3:2);
        check(!h->repeats && !out.held_actor_acknowledged,
              "successor still rejects foreign owner, focus, conflict, landing, bail, stance, object, missing observer, Prepare, wrong actor, count and buttons");
        h->poll(successor+34,{},201,1,false,0.0f,false,false,2);
        check(!h->repeats,"failed successor opportunity cannot replay on recovery");
    }
    auto h=std::make_unique<Route>();h->bridge_accept=false;
    h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
    h->poll(1085,{},100,1);h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
    h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},200,1,false);
    h->poll(1238,{},201,1,false);h->poll(1272,{},201,1,false);h->poll(1304,{},201,1,false);
    h->poll(1336,{},201,1,false);h->bridge_accept=true;h->poll(1369,{},201,1,false);
    for(unsigned now:{1370u,1371u,1380u,1390u})h->poll(now,{},201,1,false,0,false,false,2,false);
    check(!h->repeats,"duplicate client polls after deadline do not spend successor opportunity or admit early");
    h->poll(1403,{},201,1,false,0,false,false,2);
    check(h->repeats==1,"same native sample polling preserves one real successor opportunity");
}
void transient_phase_lifetime() {
    auto h=std::make_unique<Route>();h->first();
    h->poll(1204,{0x4000},201,2,false);
    check(h->repeats==1,"phase regression starts one physical held request");
    h->poll(1237,{},201,2,false);
    h->transient_phase_zero=true;
    const auto out=h->poll(1332,{},201,2,false);
    check(out.authored_playback && !out.held_input_released && !out.held_actor_acknowledged,
        "same-owner zero then AIR cannot withdraw held preparation at128ms");
    h->transient_phase_zero=false;
    h->poll(1366,{},201,2,true,0,true,true);
    const auto caught=h->poll(1400,{},201,2,true,1,true,true);
    check(caught.held_input_released && caught.held_actor_acknowledged && h->repeats==1,
        "rechecked phase grants no actor credit before independent held count and advancement");
}
void physical_repetition() {
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        const auto step=1000/hz;
        h->poll(1200,{},201,id,true);
        const auto out=h->poll(1200+step,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1 && !out.authored_playback &&
            !out.right_x && !out.right_y,
            "a new repeat cannot begin its multi-frame gesture inside the already-open native consumption window");
        h->poll(1300,{},201,id,false);
        h->poll(1334,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1,
            "a later preparation flag cannot turn a missed repeat into a disconnected flip");
    }
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        const auto step=1000/hz;
        h->poll(1200,{},201,id,true);
        h->poll(1200+step,{},201,id,false);
        const auto out=h->poll(1200+2*step,{0x4000},201,id,false);
        check(!h->repeats && h->bridge_calls==1 && !out.authored_playback &&
            !out.right_x && !out.right_y,
            "a second X after the observed native preparation window cannot start a disconnected gesture");
        h->poll(1300,{},201,id,true);
        h->poll(1317,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1,
            "a later native flag cannot reopen the first flip repetition window");
    }
    for(bool takeoff:{false,true}) {
        auto h=std::make_unique<Route>();
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
        h->poll(1085,{},100,1);h->poll(1102,{},200,1);
        h->poll(1119,{0x4000},takeoff?200:201,1,false);
        check(h->bridge_calls==1,"captured takeoff X reaches original flip bridge");
        h->poll(1136,{},201,1,false,0,false,false,2);
        h->poll(1187,{0x4000},201,1,false,0,false,false,2);
        h->poll(1204,{},201,1,true,0,false,false,2);
        check(h->repeats==1,"native animation Kickflip admits repeated X while physics retains the original Ollie selection");
        h->poll(1237,{},201,1,true,0,true,true,2);
        const auto caught=h->poll(1270,{},201,1,true,1,true,true,2);
        check(h->bridge_calls==1 && caught.held_actor_acknowledged && caught.held_input_released &&
            !caught.authored_playback && !caught.right_x && !caught.right_y,
            "animation-owned extra rotation catches without changing physics selection or issuing another flip");
    }
    {
        auto single=std::make_unique<Route>();single->first();
        for(unsigned now=1200;now<=1700;now+=25)single->poll(now);
        check(single->bridge_calls==1 && !single->repeats,"single X never starts held input without a second physical edge");
    }
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        h->poll(1204,{0x4000},201,id,false);
        check(h->repeats==1,"second physical press prepares native gesture before the short held admission window");
        h->poll(1220,{},201,id,false);
        auto out=h->poll(1237,{},201,id,true);
        check(h->repeats==1 && out.authored_playback,"same physical repetition keeps one prepared input path through native admission");
        bool endpoint=false;
        for(unsigned n=1;n<=hz/3;++n) {
            const auto now=1237+std::uint64_t{n}*1000/hz;
            out=h->poll(now,{},201,id,true,0,n>2,n>3);
            endpoint=endpoint || (out.authored_playback && out.right_y>0 && out.right_x!=0);
        }
        check(endpoint && h->repeats==1 && h->bridge_calls==1,"held input reaches authored endpoint without a second native flip or pop request");
        out=h->poll(1600,{},201,id,true,1,true,true);
        check(!out.authored_playback && !out.right_x && !out.right_y && out.held_input_released && out.held_actor_acknowledged,
            "one extra native rotation releases RS for original catch with independent actor evidence");
        h->poll(1634,{},100,0);h->poll(1668,{0x4000},100,0);
        check(h->repeats==1 && h->bridge_calls==1,"landing rejects repeat and cannot borrow ground X");
    }
    for(unsigned failure=0;failure<8;++failure) {
        auto h=std::make_unique<Route>();h->first();
        if(failure==0)++h->owner.rig;
        if(failure==1)h->admission.external_owner=true;
        if(failure==2)h->held_available=false;
        if(failure>=6) {
            NativeRequestSample intervening{};intervening.valid=true;intervening.state=201;
            intervening.crouch_mode=failure==6?2u:1u;
            h->producer.observe(1180,h->owner,intervening,{true,failure==6?3:2,16});
            intervening.crouch_mode=2;
            h->producer.observe(1190,h->owner,intervening,{true,2,16});
        }
        const auto time=failure==3?1500u:1204u;
        if(failure==3){h->poll(1250);h->poll(1350);h->poll(1450);}
        h->poll(time,{0x4000},failure==4?100u:201u,failure==5?3:2,true);
        check(!h->repeats,"owner/conflict/missing observation/window/landing/wrong base/intervening native action cannot extend a flip");
    }
}
}
int main(){physical_triples();triple_boundaries();triple_cancellation();transient_phase_lifetime();native_phase_recheck();preair_late_parent_readiness();late_parent_dependency_boundaries();native_boundaries();preair_successor_dependency();successor_dependency_boundaries();composed_successor_negatives();preair_queued_repetition();queued_first_repetition();physical_repetition();std::cout<<checks<<" held-flip input checks passed; native animation observations are fixtures\n";}
