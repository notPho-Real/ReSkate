#include "Extension/ArcadeMode/arcade_fingerflip.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Engine/Game/Build/20260929/arcade_fingerflip.h"
#include "Engine/Game/Build/20260929/arcade_grab_transition.h"
#include <limits>
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
void family_entry() {
    namespace contract=dingosdk::game::build::v20260929::arcade_grab_transition;
    std::set<int> families;
    for(const auto& actor:contract::family_entry_actors) {
        const auto* p=native_grab::pilot(native_grab::grab_family_trick(actor.family));
        check(p!=nullptr,"every original family entry has a transition source");
        families.insert(actor.family);
        const native_grab::GrabFamilyEntryObservation good{actor.family,true,true,1,0,1,1000};
        native_grab::Lease pose;
        check(pose.request(p->trick,1000,true) && !pose.acknowledged(),"Into must not claim a detailed pose loop");
        for(unsigned failure=0;failure<12;++failure) {
            auto entry=good;auto observed=parent(*p);std::uint64_t now=1010;
            if(failure==1)entry.family=actor.family%9+1;
            if(failure==2)entry.initialized=false;
            if(failure==3)entry.exact=false;
            if(failure==4)entry.delta=0;
            if(failure==5)entry.after=entry.before;
            if(failure==6)entry.before=std::numeric_limits<float>::quiet_NaN();
            if(failure==7)now=1000+native_grab::Lease::heartbeat_ms+1;
            if(failure==8)now=999;
            if(failure==9)observed.grabbing=false;
            if(failure==10)observed.can_grab=false;
            if(failure==11)observed.left=observed.right=false;
            native_grab::FingerflipInput input;
            check(input.start(*p,entry.eligible(*p,observed,now),observed,2,steps,now,60,false)==(failure==0),
                  "only a fresh exact advancing family actor admits transition before the pose loop");
            check(!pose.acknowledged(),"family transition admission never upgrades detailed-pose acceptance");
        }
    }
    check(families.size()==9,"all nine native fingerflip source families have Into evidence");
    const auto* footer=native_grab::pilot(ArcadeTrick::airwalk);
    check(!native_grab::GrabFamilyEntryObservation{footer->family,true,true,1,0,1,1000}.eligible(*footer,parent(*footer),1010),
          "a family Into actor does not make a one-footer eligible for fingerflips");
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
            native_grab::FingerflipObservation observed{true,false,true,true,0,row.family,true,true};
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
            observed.in_fingerflip=true;observed.identity=row.identity;
            check(!input.acknowledge(observed,true,true,0,0,1),"zero delta is not native animation acceptance");
            check(!input.acknowledge(observed,false,true,1,0,1),"uninitialized actor is not native acceptance");
            check(!input.acknowledge(observed,true,false,1,0,1),"a different original actor cannot acknowledge this row");
            ++observed.identity;
            check(!input.acknowledge(observed,true,true,1,0,1),"a stale or different identity cannot acknowledge this row");
            --observed.identity;
            check(input.acknowledge(observed,true,true,1,0,1),"matching initialized advancing actor acknowledges the input");
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
        native_grab::FingerflipObservation state{true,false,true,true,0,2,true,true};
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
void first_use() {
    const auto* p=native_grab::pilot(ArcadeTrick::basic_bs_grab);
    for(unsigned hz:{30u,60u,120u}) {
        bool state_exists=false,identity_exists=false,base_exists=true;
        bool native_active=false;int identity=0;
        auto sample=[&] {
            return native_grab::sample_fingerflip_state([&](unsigned index,auto& value) {
                if(index==9) {value=static_cast<decltype(value+0)>(native_active);return state_exists;}
                if(index==10) {value=static_cast<decltype(value+0)>(identity);return identity_exists;}
                value=static_cast<decltype(value+0)>(index==6?2:1);return base_exists;
            });
        };
        auto observed=sample();
        check(observed.valid && !observed.state_known && !observed.identity_known,
            "unallocated entry outputs do not invalidate an observed source grab");
        native_grab::FingerflipInput input;
        check(input.start(*p,true,parent(*p),2,steps,1000,60,false),"first use retains verified parent admission");
        bool delivered=false;std::uint64_t now=1000;
        for(unsigned n=0;n<8;++n) {
            now=1000+n*1000/hz;
            const auto control=input.poll(now,now*60/1000,true,observed);
            delivered=delivered || control.x || control.y;
            check(!input.acknowledge(observed,true,true,1,0,1),"unallocated native outputs never acknowledge an actor");
        }
        check(delivered && input.active() && !input.completed(),"cold gesture reaches native input without invented entry or catch");
        state_exists=true;native_active=true;observed=sample();
        now+=1000/hz;const auto control=input.poll(now,now*60/1000,true,observed);
        check(!control.x && !control.y,"observed native entry stops gesture replay before identity allocation");
        check(!input.acknowledge(observed,true,true,1,0,1),"state alone is insufficient for acceptance");
        identity_exists=true;identity=11;observed=sample();
        check(input.acknowledge(observed,true,true,1,0,1),"known identity and exact advancing actor acknowledge first use");
        state_exists=false;observed=sample();now+=1000/hz;
        input.poll(now,now*60/1000,true,observed);
        check(!input.active() && !input.completed(),"lost state after entry is not a successful catch");
        base_exists=false;
        check(!sample().valid,"missing native parent remains invalid even before first use");
    }
    native_grab::FingerflipInput input;
    check(input.start(*p,true,parent(*p),2,steps,1000,60,false),"timeout case starts from a qualified grab");
    const native_grab::FingerflipObservation unknown{true,false,true,true,0,2,false,false};
    for(std::uint64_t now=1000;now<=1900;now+=100)input.poll(now,now*60/1000,true,unknown);
    check(!input.active() && !input.completed(),"unallocated outputs cannot extend the native entry deadline");
}
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
int main(){family_entry();transitions();first_use();routing();std::cout<<"Native fingerflip routing and lifecycle tests passed\n";}
