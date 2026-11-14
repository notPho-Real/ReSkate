#pragma once
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
#include <memory>

namespace takeoff_followup_tests {
using namespace dingosdk::arcade_mode;
inline void require(bool value,const char* why) {
    if (!value) {std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}
}
struct Route {
    InputSession session;
    NativeInputState producer;
    NativeInputOwner owner{1,2,3,4,5};
    InputAdmission admission{};
    unsigned flips{},grabs{};
    bool switched{},nose{};NativeGrabRequest grab_bridge{};
    NativeCharge charge;NativeCharge::Release release{};
    std::vector<PreparedGesture> catalog{
        {1,false,GestureContext::ground,{{0,0,-31000},{18,0,31000},{21,0,0},{24,0,0}}},
        {16,true,GestureContext::ground,{{0,0,31000},{18,0,-31000},{21,0,0},{24,0,0}}},
        {1,false,GestureContext::from_grind,{{0,0,-28000},{18,0,28000},{21,0,0},{24,0,0}}}};
    Route() {
        admission.gameplay=admission.authored=admission.may_start=admission.inspected_stance=true;
        admission.action_mode=ActionExperiment::classic;admission.catalog=catalog;
        admission.normal_flip={this,[](void* context,std::int32_t id) noexcept {
            auto& self=*static_cast<Route*>(context);
            require(self.admission.native_context.airborne && id==2,"flip submission requires actual AIR");
            ++self.flips;return true;
        }};
        admission.normal_grab.context=this;
        admission.normal_grab.submit=[](void* context,const InputRequest&,ArcadeTrick,bool) noexcept {
            auto& self=*static_cast<Route*>(context);
            require(self.admission.native_context.airborne,"grab submission requires actual AIR");
            ++self.grabs;return true;
        };
        grab_bridge=admission.normal_grab;
    }
    NativeGesture::Output poll(std::uint64_t now,unsigned state,unsigned mode,int selected,
            PhysicalInputSample pad={},bool balance=false,bool fakie=false) {
        NativeRequestSample raw{};raw.valid=true;raw.state=state;raw.crouch_mode=mode;
        raw.words[1]=balance?(nose?0x8000u:0x4000u):0xc000u;raw.words[0]=switched?0x20u:0u;
        const auto frame=producer.observe(now,owner,raw,{true,selected,16},false,0,{true,fakie});
        admission.native_context=InputNativeContext::from_frame(frame);
        session.observe_manual_transition(admission.native_context);
        session.observe_rail_followup(admission.native_context);
        admission.normal_grab=frame.airborne?grab_bridge:NativeGrabRequest{};
        admission.pose=frame.airborne?SkatePose::air:state>=400&&state<=406?SkatePose::grind:
            state==701?SkatePose::grind_exit_transition:balance?(nose?SkatePose::nose_manual:SkatePose::manual):SkatePose::riding;
        admission.nollie=nose && (pad.buttons&classic_a);
        admission.action_fresh=frame.airborne || state==100;
        admission.flip_ready=frame.airborne && selected==-1;
        admission.manual_safe=state==100;
        admission.manual_balance_admitted=balance;
        admission.balance_generations=admission.native_context.generations;
        admission.tick=now*60/1000;
        const auto out=session.poll(now,pad,0,0,admission);
        const bool valid=NativeCharge::input_permitted(admission.gameplay,out.action.owned,pad.buttons,
            session.released_pop_active(now,admission.native_context));
        if(out.jump.setup_started)charge.admit_setup(now,out.jump.pressed_at,out.jump.launch,valid,out.jump.nollie);
        release=charge.semantic_release(now,out.jump.released,out.jump.setup_admitted,out.jump.from_rail,
            out.jump.pressed_at,out.jump.released_at,valid,out.jump.launch,out.jump.nollie);
        return out;
    }
};
inline void run() {
    // Device trace: native rail exit precedes the physical release by 8ms;
    // its Quick Ollie selection follows one physics step later.
    for (bool grab : {false,true}) for (unsigned fault=0;fault<10;++fault) {
        auto h=std::make_unique<Route>();
        h->poll(1000,401,1,-1);
        require(h->poll(1034,401,1,-1,{static_cast<std::uint16_t>(classic_a)}).jump.setup_started,"measured rail setup admitted");
        h->poll(1068,403,1,-1,{static_cast<std::uint16_t>(classic_a)});
        if(fault==1)++h->owner.entity;
        if(fault==2)h->admission.gameplay=false;
        if(fault==3)h->admission.external_owner=true;
        if(fault==8)h->switched=true;
        const auto departed=h->poll(1102,fault==7?300u:701u,1,-1,{static_cast<std::uint16_t>(classic_a)});
        require(!departed.authored_playback,"native departure still releases input ownership");
        const auto shift=fault==4?501u:0u;
        const auto held=static_cast<std::uint16_t>(fault==5?classic_a:0);
        h->poll(1110+shift,701,1,-1,{static_cast<std::uint16_t>(held),0,0,0,static_cast<std::uint8_t>(fault==9?255:0)});
        const int selected=fault==6?4:31;
        h->poll(1136+shift,701,0,selected,{static_cast<std::uint16_t>(held)});
        const auto face=static_cast<std::uint16_t>(grab?0x2000:classic_x);
        h->poll(1144+shift,701,0,selected,{static_cast<std::uint16_t>(face|held)});
        h->poll(1170+shift,701,0,selected,{static_cast<std::uint16_t>(face|held)});
        require(!h->flips && !h->grabs,"Quick Ollie selection alone cannot execute before AIR");
        h->poll(1204+shift,200,0,selected,{static_cast<std::uint16_t>(held)});
        for(unsigned i=0;i<10;++i)h->poll(1238+shift+i*34,201,2,selected,{static_cast<std::uint16_t>(held)});
        require(grab?h->grabs==(fault?0u:1u):h->flips==(fault?0u:1u),
            "device rail follow-up requires original owner, gameplay, timely release, native Quick Ollie and no conflict");
        require(grab?!h->flips:!h->grabs,"rail follow-up remains in requested family");
    }
    for(bool grab:{false,true}) {
        auto h=std::make_unique<Route>();
        h->poll(1000,401,1,-1);
        h->poll(1034,401,1,-1,{classic_a});
        h->poll(1068,403,1,-1,{classic_a});
        h->poll(1102,701,1,-1,{classic_a});
        h->poll(1136,701,0,31,{classic_a});
        h->poll(1170,200,0,31,{classic_a});
        for(unsigned i=0;i<3;++i)h->poll(1204+i*34,201,2,31,{classic_a});
        require(!h->flips && !h->grabs,"held rail A never starts a follow-up");
        h->poll(1306,201,2,31);
        h->poll(1340,201,2,31,{static_cast<std::uint16_t>(grab?0x2000:classic_x)});
        for(unsigned i=0;i<8;++i)h->poll(1374+i*34,201,2,31);
        require(grab?h->grabs==1:h->flips==1,"quarter-second rail hold retains its continuously observed native pop after release");
    }
    for(bool grab:{false,true}) {
        auto h=std::make_unique<Route>();
        h->poll(1000,701,0,31);
        h->poll(1034,701,0,31,{static_cast<std::uint16_t>(grab?0x2000:classic_x)});
        for(unsigned i=0;i<12;++i)h->poll(1068+i*34,i<2?200:201,2,31);
        require(!h->flips && !h->grabs,"unowned Quick Ollie cannot authorize a follow-up");
    }
    require(!NativeCharge::input_permitted(true,false,0x2000),"B without released-pop proof still cancels launch input");
    require(!NativeCharge::input_permitted(true,true,0x2000,true),"a competing actor still cancels a released launch");
    for(bool rail:{false,true})for(bool grab:{false,true}) {
        auto h=std::make_unique<Route>();
        h->poll(1000,rail?401:100,1,1);
        h->poll(1017,rail?401:100,1,1,{static_cast<std::uint16_t>(grab?0x2000:classic_x)});
        for(unsigned i=0;i<12;++i)h->poll(1034+i*17,i<2?200:201,2,1);
        require(!h->flips && !h->grabs,"a face before an unowned takeoff cannot borrow later AIR");
    }
    unsigned cases{};
    for(bool rail:{false,true})for(bool grab:{false,true})for(unsigned cadence:{8u,17u,34u})
    for(bool simultaneous:{false,true})for(bool switched:{false,true})for(bool nose:{false,true})for(unsigned fault=0;fault<9;++fault) {
        if(rail && nose)continue;
        auto h=std::make_unique<Route>();h->switched=switched;h->nose=nose;
        const int jump=nose?16:1;
        std::uint64_t now=1000;
        if(!rail) {
            // Native RT transition and fakie acknowledgment precede this hop.
            h->poll(now,100,1,-1,{},true);
            now+=cadence;
            require(h->poll(now,100,1,-1,{0,0,0,0,255},true).action.owned,"manual revert starts");
            for(unsigned i=0;i<700/cadence;++i){now+=cadence;h->poll(now,101,1,-1);}
            now+=cadence;h->poll(now,100,1,-1,{},true,true);
            for(unsigned i=0;i<600/cadence;++i){now+=cadence;h->poll(now,100,1,-1,{},true,true);}
        } else h->poll(now,401,1,-1);
        now+=cadence;
        require(h->poll(now,rail?401:100,1,jump,{classic_a},!rail,!rail).jump.setup_started,"chain starts its own pop");
        for(unsigned i=0;i<20;++i){now+=cadence;h->poll(now,rail?401:103,rail?1:0,jump,{classic_a},false,!rail);}
        now+=cadence;
        const auto face=static_cast<std::uint16_t>(grab?0x2000:classic_x);
        require(h->poll(now,rail?401:103,rail?1:0,jump,{static_cast<std::uint16_t>(simultaneous?face:0)},false,!rail).jump.released,
            "chain releases its own pop");
        const auto pressed=now+(simultaneous?0:cadence);
        if(!simultaneous) {now=pressed;h->poll(now,rail?401:103,rail?1:0,jump,{face},false,!rail);}
        require(!h->flips && !h->grabs,"preload cannot execute on a supporting surface");
        require(rail?!h->release.sequence:h->release.sequence!=0,
            "preload preserves the original ordinary launch ticket and never creates a rail ticket");
        if(fault==1)++h->owner.entity;
        if(fault==2)h->admission.gameplay=false;
        if(fault==3)h->admission.external_owner=true;
        for(unsigned i=0;i<16;++i) {
            now+=cadence;
            const auto state=i==0?(fault==4?300u:fault==5?100u:rail?701u:103u):i<3?200u:201u;
            if(fault==6 && i==0)now+=501;
            const auto out=h->poll(now,state,i<1?(rail?1:0):2,fault==7?4:fault==8?(nose?1:16):jump,{},false,!rail);
            if(fault)require(!out.native_flip_request && !out.action.requested,"withdrawn takeoff cannot submit stale intent");
            h->poll(now,state,i<1?(rail?1:0):2,fault==7?4:fault==8?(nose?1:16):jump,{},false,!rail);
        }
        require(grab?h->grabs==(fault?0u:1u):h->flips==(fault?0u:1u),
            rail?(grab?"released grind pop retains B through takeoff":"released grind pop retains X through takeoff"):
                 (grab?"revert Manual pop retains B through takeoff":"revert Manual pop retains X through takeoff"));
        require(grab?!h->flips:!h->grabs,"one face cannot request the other trick family");
        ++cases;
    }
    std::cout<<cases<<" takeoff follow-up transition cases passed\n";
}
}
