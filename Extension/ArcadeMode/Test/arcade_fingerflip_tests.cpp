#include "Extension/ArcadeMode/arcade_fingerflip.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Engine/Game/Build/20260929/arcade_fingerflip.h"
#include <cstdlib>
#include <iostream>
#include <set>
using namespace dingosdk::arcade_mode;
namespace {
void check(bool value,const char* why){if(!value){std::cerr<<why<<'\n';std::exit(1);}}
const std::array<AuthoredStickStep,5> steps{{{0,0,0},{2,0,-28000},{4,28000,28000},{7,0,0},{9,0,0}}};
native_grab::RetargetObservation parent(const native_grab::Pilot& p) {
    return {true,true,true,(p.hands&1u)!=0,(p.hands&2u)!=0,p.family};
}
void transitions() {
    std::set<int> covered;
    for(unsigned hz:{30u,60u,120u})for(const auto& row:native_grab::fingerflips) {
        const auto* p=native_grab::pilot(native_grab::grab_family_trick(row.family));
        for(int gesture:{2,3,4,7}) {
            if(!(row.gestures&native_grab::finger_gesture(gesture)))continue;
            native_grab::FingerflipInput input;
            check(input.start(*p,true,parent(*p),gesture,steps,1000,60,false),"each source-qualified row has a gesture entry");
            check(input.row()==&row,"gesture maps to the source CDB row without confusing namespaces");
            native_grab::FingerflipObservation observed{true,false,true,true,0,row.family};
            auto control=input.poll(1000,60,false,observed);
            check(control.hands==p->hands && control.transition && !control.feet && !control.x && !control.y,"unobserved setting cannot emit a gesture or foot/jump input");
            bool delivered=false;
            for(unsigned n=1;n<8;++n) {
                const auto now=1000+n*1000/hz;
                control=input.poll(now,now*60/1000,true,observed);
                delivered=delivered || control.x || control.y;
                check(control.hands==p->hands && !control.feet,"source hand stays held throughout original gesture delivery");
            }
            check(delivered,"original authored gesture advances at every tested sampling cadence");
            check(!input.acknowledge(row.identity,true,true,0,0,1),"zero delta is not native animation acceptance");
            check(!input.acknowledge(row.identity,false,true,1,0,1),"uninitialized actor is not native acceptance");
            check(!input.acknowledge(row.identity,true,false,1,0,1),"a different original actor cannot acknowledge this row");
            check(!input.acknowledge(row.identity+1,true,true,1,0,1),"a stale or different identity cannot acknowledge this row");
            check(input.acknowledge(row.identity,true,true,1,0,1),"matching initialized advancing actor acknowledges the input");
            observed.in_fingerflip=true;observed.identity=row.identity;observed.family=row.destination;
            auto now=1000+8*1000/hz;
            control=input.poll(now,now*60/1000,true,observed);
            check(control.hands==native_grab::pilot(native_grab::grab_family_trick(row.destination))->hands &&
                control.transition && !control.x && !control.y && !input.completed(),"native destination changes the hand without replaying a gesture");
            observed.in_fingerflip=false;observed.grabbing=false;now+=1000/hz;
            input.poll(now,now*60/1000,true,observed);
            check(!input.completed(),"leaving FingerFlip alone cannot prove the destination grab");
            observed.grabbing=true;now+=1000/hz;input.poll(now,now*60/1000,true,observed);
            check(input.completed(),"matching native destination and grab admission complete the handoff");
            covered.insert(row.identity);
        }
    }
    check(covered.size()==26 && !covered.contains(15) && !covered.contains(18),"all non-Coffin identities are reachable; Coffin variants require their distinct context");
    const auto* p=native_grab::pilot(ArcadeTrick::basic_bs_grab);
    for(unsigned failure=0;failure<7;++failure) {
        native_grab::FingerflipInput input;auto observed=parent(*p);
        if(failure==0)observed.grabbing=false;
        if(failure==1)observed.family=1;
        if(failure==2)observed.right=false;
        const bool started=input.start(*p,failure!=3,observed,failure==4?36:2,steps,1000,60,false);
        if(failure<5){check(!started,"unqualified parent or unsupported gesture cannot enter");continue;}
        native_grab::FingerflipObservation state{true,false,true,true,0,2};
        input.poll(1000,60,true,state);
        if(failure==5)state.valid=false;
        input.poll(failure==6?1200:1017,61,true,state);
        check(!input.active(),"stale polls or missing native owner withdraw all transition output");
    }
}
struct Route {
    InputSession session;NativeInputState producer;InputAdmission env{};bool active{},accept{true};
    unsigned requests{},ordinary{};std::uint8_t selected{};
    std::vector<PreparedGesture> gestures;
    Route() {
        for(const auto id:std::array<std::uint8_t,4>{2,3,4,7})gestures.emplace_back(id,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-28000},{18,28000,28000},{21,0,0},{24,0,0}});
        env.gameplay=env.authored=env.action_fresh=env.inspected_stance=true;env.action_mode=ActionExperiment::classic;
        env.catalog=gestures;
        env.normal_flip={this,[](void* p,std::int32_t)noexcept{++static_cast<Route*>(p)->ordinary;return true;}};
        env.normal_grab.context=this;
        env.normal_grab.submit=[](void*,const InputRequest&,ArcadeTrick,bool)noexcept{return true;};
        env.normal_grab.fingerflip=[](void* p,std::uint8_t id,std::span<const AuthoredStickStep> path,std::uint64_t,bool)noexcept{
            auto& r=*static_cast<Route*>(p);++r.requests;r.selected=id;r.active=r.accept && !path.empty();return r.active;
        };
        env.normal_grab.controls=[](void* p)noexcept{
            const auto& r=*static_cast<Route*>(p);return native_grab::Control{2,0,0,0,r.active};
        };
    }
    NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons,std::uint32_t state=201) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:2;
        sample.words[1]=state==100?0xc000u:0;
        env.native_context=InputNativeContext::from_frame(producer.observe(now,{1,2,3,4,5},sample,{true,-1,16}));
        env.pose=state==100?SkatePose::riding:SkatePose::air;env.tick=now*60/1000;
        return session.poll(now,{buttons},0,0,env);
    }
};
void routing() {
    for(unsigned direction:{0u,4u,8u,2u,1u}) {
        Route r;r.poll(1000,0,100);r.poll(1017,0x2000);r.poll(1034,0x2000);
        const auto out=r.poll(1051,static_cast<std::uint16_t>(0x6000u|direction));
        const auto expected=direction==8?3u:direction==2?4u:direction==1?7u:2u;
        check(r.requests==1 && r.selected==expected && !r.ordinary,"B plus physical X routes one native grab transition with the held direction");
        check(!out.jump.setup_started && !out.native_flip_request && !out.authored_playback &&
            out.action.pad.owns_triggers && out.action.pad.owns_right_stick,"grab transition keeps native hands and neutral RS without ordinary flip/Ollie playback");
        r.poll(1068,0x6000);r.poll(1085,0x2000);
        check(r.requests==1 && !r.ordinary,"held X and its release do not duplicate the physical request");
    }
    {Route r;r.accept=false;r.poll(1000,0,100);r.poll(1017,0x2000);r.poll(1034,0x6000);r.poll(1051,0);
        check(r.requests==1 && !r.ordinary,"rejected native entry never falls back to an ordinary flip");}
    {Route r;r.poll(1000,0x6000);r.poll(1017,0x6000);check(!r.requests,"startup-held buttons are not a fresh contextual request");}
    {Route r;r.poll(1000,0,100);r.poll(1017,0x6000,100);check(!r.requests,"ground B plus X cannot borrow airborne admission");}
}
}
int main(){transitions();routing();std::cout<<"Native fingerflip routing and lifecycle tests passed\n";}
