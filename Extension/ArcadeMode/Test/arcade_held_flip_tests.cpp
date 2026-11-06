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
    std::int32_t requested{};std::vector<PreparedGesture> recipes;bool held_available{true};
    Route() {
        AuthoredGesture kick{2,false,GestureContext::ground,90,180,.85f,{{280,30,.5f,1,0},{90,150,.5f,1,.1f}}};
        AuthoredGesture heel{3,false,GestureContext::ground,180,270,.85f,{{330,80,.5f,1,0},{210,270,.5f,1,.1f}}};
        const std::array catalog{kick,heel};recipes=prepare_gestures(catalog);
        recipes.emplace_back(std::uint8_t{1},false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}});
        admission.gameplay=admission.authored=admission.action_fresh=true;
        admission.may_start=admission.inspected_stance=true;admission.action_mode=ActionExperiment::classic;
        admission.catalog=recipes;admission.held_flip.available=true;
        admission.normal_flip={this,[](void* p,std::int32_t id) noexcept {
            auto& self=*static_cast<Route*>(p);++self.bridge_calls;self.requested=id;return true;
        }};
    }
    NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample physical={},std::uint32_t state=201,std::int32_t id=2,
                               bool prepare=true,float extra=0,bool held=false,bool actor=false,int animation_id=-999) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:state==200?0:2;
        sample.words[1]=state==100?0xc000u:0;
        frame=producer.observe(now,owner,sample,{true,id,16});
        admission.native_context=InputNativeContext::from_frame(frame);
        admission.pose=frame.airborne?SkatePose::air:SkatePose::riding;
        admission.flip_ready=id==-1;admission.tick=now*60/1000;
        admission.held_flip={held_available,frame.airborne,prepare,held,actor,animation_id==-999?id:animation_id,extra,now};
        const auto out=session.poll(now,physical,0,0,admission);
        const auto& d=session.diagnostic();
        for(std::size_t i=0;i<d.dispatched;++i)
            repeats+=d.dispatch[i].request.form==InputForm::held_repeat && d.dispatch[i].disposition==InputDisposition::started;
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
void physical_repetition() {
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
int main(){native_boundaries();physical_repetition();std::cout<<checks<<" held-flip input checks passed; native animation observations are fixtures\n";}
