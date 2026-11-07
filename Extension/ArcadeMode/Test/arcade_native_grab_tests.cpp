#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include "Extension/ArcadeMode/arcade_native_grab_lease.h"
#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Engine/Game/Input/gameplay_options.h"
#include "Engine/Game/Skater/physics_state_read.h"
#include "Engine/Game/Build/20260929/arcade_grab_transition.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void native_family_change_waits_for_exit() {
    const native_grab::RetargetObservation nose{true,true,true,false,true,6,true};
    const native_grab::RetargetObservation bs{true,true,true,false,true,2,true};
    const native_grab::RetargetObservation clear{true,false,false,false,false,0,false};
    for(unsigned hz:{30u,60u,120u}) {
        const auto dt=(1000u+hz-1)/hz;
        native_grab::Lease lease;
        lease.request(ArcadeTrick::airwalk,1000,true);
        check(lease.controls(nose,true).feet==0x2000,"Airwalk uses original both-feet input after Nose entry");
        check(lease.acknowledge(0,3,0,3,true,true,true,12),"original Airwalk actor owns the first grab");
        lease.observe_footer(1000,true,true,false);
        lease.request(ArcadeTrick::christ_air,1100,true);
        auto now=1100u;
        for(unsigned frame=0;frame<hz;++frame) {
            now+=dt;lease.refresh(now,true,true);lease.observe_footer(now,true,true,true);
            const auto out=lease.controls(nose,true);
            check(lease.changing_family() && out.hands==2 && out.y==-28000 && !out.feet,
                "Christ Air request withdraws Airwalk feet but retains the original Nose hand through native catch");
            check(!lease.acknowledge(0,2,0,2,true,true,true,6),
                "a target tag or actor cannot acknowledge before the previous family exits");
        }
        now+=dt;lease.refresh(now,true,true);lease.observe_footer(now,true,false,false);
        auto out=lease.controls(nose,true);
        check(!out.hands && !out.selector() && !out.feet && lease.changing_family(),
            "finished catch withdraws the original hand and selector");
        for(unsigned fault=0;fault<4;++fault) {
            auto incomplete=clear;
            if(fault==0)incomplete.valid=false;
            if(fault==1)incomplete.grabbing=true;
            if(fault==2)incomplete.can_grab=true;
            if(fault==3)incomplete.family=6;
            check(!lease.controls(incomplete,true).hands && lease.changing_family(),
                "unreadable state, an active owner, eligibility or stale family cannot admit the new grab");
        }
        out=lease.controls(clear,true);
        check(!lease.changing_family() && out.hands==2 && !out.selector() && !out.feet,
            "independently cleared native state admits neutral BS hand without borrowing the old foot input");
        check(lease.controls(bs,true).feet==0x2000 &&
            lease.acknowledge(0,3,0,3,true,true,true,6),"Christ Air requires its own BS admission, original feet and actor");
        now+=dt;lease.refresh(now,false,true);lease.observe_footer(now,true,true,true);
        check(lease.controls(bs,true).hands==2 && !lease.controls(bs,true).feet,
            "new Christ Air keeps the existing native release ordering");
        lease.observe_footer(now,true,false,false);
        check(!lease.intent(now),"completed new catch withdraws all input");
        lease.request(ArcadeTrick::basic_bs_grab,now,true);
        lease.request(ArcadeTrick::method,now+dt,true);
        check(!lease.changing_family() && lease.controls(bs,true).hands==2,
            "BS to Method remains an immediate native same-family tweak");
        lease.request(ArcadeTrick::indy,now+2*dt,true);
        check(lease.changing_family(),"a different hand/family also requires original cleanup");
        lease.refresh(now+3*dt,true,false);
        check(!lease.intent(now+3*dt) && !lease.controls(clear,true).hands,
            "landing or owner loss cancels a pending family change");
        lease.request(ArcadeTrick::airwalk,3000,true);
        lease.request(ArcadeTrick::christ_air,3100,true);
        for(now=3100+dt;now<4500;now+=dt) {
            lease.refresh(now,true,true);lease.observe_footer(now,true,true,true);
            lease.controls(nose,true);
        }
        lease.refresh(now,true,true);lease.observe_footer(now,true,false,false);
        lease.controls(clear,true);
        const auto admitted=now;
        for(unsigned frame=0;frame<hz/2;++frame) {
            now+=dt;lease.refresh(now,true,true);lease.controls(bs,true);
        }
        check(now>3100+native_grab::Lease::initialization_limit_ms && lease.intent(now) &&
            lease.acknowledge(0,3,0,3,true,true,true,6),
            "target initialization starts after observed old-family cleanup, without expiring a valid new entry");
        check(admitted<3100+native_grab::Lease::initialization_limit_ms,
            "the old cleanup itself remains within its existing failure bound");
        lease.cancel();lease.request(ArcadeTrick::airwalk,6000,true);
        lease.request(ArcadeTrick::christ_air,6100,true);
        for(now=6100+dt;now<=6100+native_grab::Lease::initialization_limit_ms+dt;now+=dt) {
            lease.refresh(now,true,true);lease.controls(nose,true);
        }
        check(!lease.intent(now),"a family that never clears cannot hold or emit target inputs indefinitely");
        lease.cancel();lease.request(ArcadeTrick::basic_bs_grab,8000,true);
        lease.begin_transition();lease.request(ArcadeTrick::weddle,8100,true);
        check(!lease.changing_family(),"verified BS-to-Weddle fingerflip return preserves its established destination");
    }
}
void native_footer_exit_contract() {
    for(unsigned known=0;known<4;++known) for(unsigned active=0;active<4;++active) {
        unsigned reads{};
        const auto observed=native_grab::sample_footer_state([&](unsigned index,bool& value) {
            check(index==15 || index==16,"footer observations read only original state tags");
            const auto bit=1u<<(index-15);++reads;
            value=(active&bit)!=0;
            return (known&bit)!=0;
        });
        check(reads==2,"an absent first field does not suppress the independent second read");
        check(observed.active()==((known&active)!=0),"only readable active tags retain native ownership");
        check(observed.known()==((known&active)!=0 || known==3),"an active air tag survives an unallocated first-exit slot");
    }
    for(unsigned hz:{30u,60u,120u}) for(const auto& profile:native_grab::pilots) {
        if(!profile.one_footer)continue;
        const auto dt=(1000u+hz-1)/hz;
        const native_grab::RetargetObservation native{true,true,true,
            (profile.hands&1u)!=0,(profile.hands&2u)!=0,profile.family,true};
        for(unsigned fault=0;fault<5;++fault) {
            native_grab::Lease lease;
            lease.request(profile.trick,1000,true);
            check(lease.controls(native,true).feet==native_grab::foot_button(profile.foot),"native family admits original foot input");
            check(lease.acknowledge(0,1,0,1,true,true,true,profile.one_footer),"exact original footer actor establishes ownership");
            auto now=1000+dt;
            lease.observe_footer(now,true,true,false);
            lease.refresh(now,false,true);
            const auto release=lease.controls(native,true);
            check(lease.intent(now) && !lease.physical_hold(now) && release.hands==profile.hands &&
                release.feet==0 && release.x==profile.selector_x && release.y==profile.selector_y,
                "B release withdraws feet while preserving the established hand and selector for native exit");
            // Exit clears the trick identity before its state tags. Neither
            // controller cadence nor an elapsed animation timer ends the hold.
            for(unsigned frame=0;frame<hz;++frame) {
                now+=dt;lease.refresh(now,false,true);lease.observe_footer(now,true,true,true);
                check(lease.intent(now) && lease.controls(native,true).feet==0,
                    "native exit owns hand lifetime without retriggering a foot");
            }
            if(fault==0)lease.observe_footer(now,true,false,false);
            if(fault==1)lease.observe_footer(now,false,true,true);
            if(fault==2)lease.refresh(now+dt,false,false);
            if(fault==3)now+=native_grab::Lease::heartbeat_ms+1;
            if(fault==4)lease.cancel();
            check(!lease.intent(now),"completion, missing tags, owner loss, stale heartbeat and OFF withdraw hands");
            lease.refresh(now+dt,true,false);
            lease.observe_footer(now+dt,true,true,true);
            check(!lease.intent(now+dt),"later input cannot resurrect a retired occurrence");
        }
    }
    namespace contract=dingosdk::game::build::v20260929::arcade_grab_transition;
    check(contract::footer_exit_actors.size()==13,"all original footer exits have separate actor contracts");
    for(std::size_t i=0;i<contract::footer_exit_actors.size();++i) {
        const auto& exit=contract::footer_exit_actors[i];
        check(exit.one_footer==static_cast<std::int32_t>(i+1),"exit actor lookup preserves native footer identity order");
        for(const auto& into:contract::footer_entry_actors)
            check(exit.actor.guid!=into.actor.guid,"an Into actor cannot acknowledge Exit");
        for(const auto& loop:contract::actor_assets)
            check(exit.actor.guid!=loop.guid,"a held grab actor cannot acknowledge Exit");
        for(std::size_t j=0;j<i;++j)
            check(exit.actor.guid!=contract::footer_exit_actors[j].actor.guid,"footer exits remain distinct original actors");
    }
}
void current_air_read_preserves_only_verified_hold() {
    for (unsigned hz:{30u,60u,120u}) for (unsigned fault=0;fault<8;++fault) {
        native_grab::Lease lease;
        check(lease.request(ArcadeTrick::judo,1000,true),"start Judo physical hold");
        const native_grab::RetargetObservation entered{true,true,true,false,true,2,true};
        check(lease.controls(entered,true).feet!=0,"original foot input follows established BS owner");
        check(lease.acknowledge(0,3,0,3,true,true,true,4),"original Judo Into advances before hold check");
        unsigned reads{},owners{};
        const auto observation=dingosdk::game::skater::read_current_physics_state(
            [&](std::uint32_t& state) {
                ++reads;
                if(fault==5 || (fault==6 && reads==2))return false;
                state=reads==1?0u:fault==1?0u:fault==2?100u:fault==3?504u:201u;
                return true;
            },[&] {++owners;return !(fault==4 || (fault==7 && owners==2));});
        const bool air=observation.known() && (observation.state==200 || observation.state==201);
        lease.refresh(1000+1000/hz,true,air);
        check(lease.intent(1000+1000/hz)==(fault==0),
              "fresh same-owner air recheck retains held Judo; unknown, ground, bail, read failure and replaced owner cancel");
        check(reads<=2 && owners<=2,"state recheck is bounded without polling or waiting");
        if(fault==0) {
            check(observation.first==0 && observation.state==201 && observation.rechecked,
                  "recovery records both actual reads instead of claiming zero is air");
            lease.refresh(1000+2*(1000/hz),false,true);
            check(!lease.intent(1000+2*(1000/hz)),"actual physical release still withdraws acknowledged hold");
        }
    }
    for (std::uint32_t state:{100u,200u,201u,403u,504u,601u,999u}) {
        unsigned reads{},owners{};
        const auto observed=dingosdk::game::skater::read_current_physics_state(
            [&](std::uint32_t& value){++reads;value=state;return true;},[&]{++owners;return true;});
        check(observed.state==state && observed.readable && !observed.rechecked && reads==1 && owners==0,
              "nonzero native transitions are never retried or replaced");
    }
}
struct Route {
    ArcadeInput input{true};NativeInputState producer;NativeInputReadiness readiness;
    NativeInputOwner owner{1,2,3,4,5};NativeInputFrame frame{};
    native_grab::Lease lease;std::vector<InputRequest> requests;std::vector<ArcadeTrick> tricks;
    bool accept{true};std::uint64_t now{};std::uintptr_t selection_object{16};
    bool selection_valid{true},client_action_ready{true};native_grab::RetargetObservation grab_observation{};
    bool selector_available{true},foot_settings_ready{true},switched{};ArcadeTrick selected{ArcadeTrick::double_grab};
    InputGenerations grab_lineage{};InputAirEpisode grab_air{};NativeInputOwner grab_owner{};
    std::vector<PreparedGesture> recipes{{1,false,GestureContext::ground,{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
    Route(){input.enable(true);input.set_recognition_capture(0,true);}
    static bool submit(void* p,const InputRequest& request,ArcadeTrick trick,bool held) noexcept {
        auto& self=*static_cast<Route*>(p);if(!self.accept)return false;
        self.requests.push_back(request);self.tricks.push_back(trick);
        self.grab_lineage=request.generations;self.grab_air=request.air_episode;self.grab_owner=self.owner;
        return self.lease.request(trick,self.now,held);
    }
    static bool right_hand_input(void* p) noexcept {
        auto& self=*static_cast<Route*>(p);
        return self.frame.valid && self.frame.airborne && self.owner==self.grab_owner &&
            self.frame.airborne_episode==self.grab_air.number && self.lease.intent(self.now);
    }
    static unsigned hand_inputs(void* p) noexcept {
        auto& self=*static_cast<Route*>(p);
        return right_hand_input(p)?self.lease.selected()->hands:0u;
    }
    static native_grab::Control controls(void* p) noexcept {
        auto& self=*static_cast<Route*>(p);
        return right_hand_input(p)?self.lease.controls(self.grab_observation,self.foot_settings_ready):native_grab::Control{};
    }
    static bool continues(void* p,InputGenerations origin,InputAirEpisode air) noexcept {
        auto& self=*static_cast<Route*>(p);
        return self.frame.valid && self.frame.airborne && self.owner==self.grab_owner &&
            origin==self.grab_lineage && air==self.grab_air &&
            InputNativeContext::from_frame(self.frame).air_episode()==air &&
            self.lease.intent(self.now) && self.grab_observation.eligible(self.lease.selected());
    }
    NativeGesture::Output poll(std::uint64_t at,std::uint16_t buttons=0,std::uint32_t state=201,
                              std::int32_t selection=-1,bool enabled=true) {
        now=at;NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:state==200?0:2;
        sample.words[1]=state==100?0xc000u:0;
        sample.words[0]=switched?0x20u:0;
        frame=producer.observe(at,owner,sample,{selection_valid,selection,selection_object});
        readiness.publish(frame,{true,state==100,false,false,false,false,false,false,true,0});
        input.publish_native_context(frame);input.publish_authority(at,frame.owner_generation,1,true);
        const auto admission=readiness.sample(at,frame);
        lease.refresh(at,(buttons&0x2000)!=0,enabled && frame.airborne);
        auto grab=client_action_ready?NativeGrabRequest{this,submit,right_hand_input,continues,hand_inputs,
            selector_available?controls:nullptr}:NativeGrabRequest{};
        grab.selected=selected;
        return input.poll(0,at,buttons,0,0,0,0,enabled,admission.start,false,0,false,false,true,at*60/1000,
            false,recipes,admission.mirror_x,false,frame.airborne&&client_action_ready?SkatePose::air:SkatePose::riding,
            false,false,false,false,admission.flip_ready,false,0,0,ActionExperiment::classic,admission.action&&client_action_ready,
            admission.stance_known,false,{},grab);
    }
};
AirRotation::Output rotate(Route& route,AirRotation& rotation,const NativeGesture::Output& output,
        std::uint16_t buttons,SpinMode mode=SpinMode::held,SpinFacts facts={},bool reference=false) {
    const auto held=buttons&BoardChord::pair;
    const bool conflict=output.action.blocks_air_rotation(mode==SpinMode::held &&
        (held==BoardChord::lb || held==BoardChord::rb));
    auto result=rotation.poll_qualified(route.now,buttons,false,false,true,
        native_rotation_pose(route.frame,SkatePose::air,false),false,
        reference||conflict||output.phase==3,mode,facts,&route.frame);
    return AirRotation::native_shoulder_output(result,mode,buttons,!reference&&!conflict&&output.phase!=3&&
        route.input.allows_air_rotation(0,output.authored_playback));
}
void grab_and_native_shoulders() {
    for(unsigned hz:{30u,60u,120u}) for(auto shoulder:{BoardChord::lb,BoardChord::rb}) {
        auto r=std::make_unique<Route>();AirRotation rotation;AirMotion motion;
        auto at=[&](unsigned n){return 1000+std::uint64_t{n}*1000/hz;};
        r->poll(900,0,100); // Establish the independently observed departure.
        auto out=r->poll(at(0));rotate(*r,rotation,out,0);
        const auto buttons=static_cast<std::uint16_t>(0x2004|shoulder);
        out=r->poll(at(1),buttons);
        auto spin=rotate(*r,rotation,out,buttons);
        check(out.action.owned && out.action.native_grab_triggers && out.action.pad.right_trigger==255,
            "production B delivery retains trigger ownership while allowing an independent native shoulder");
        check(r->tricks.size()==1 && r->tricks[0]==ArcadeTrick::melon && !out.authored_playback,
            "shoulder composition preserves the selected named grab without pop playback");
        check(spin.owns_left && spin.x==(shoulder==BoardChord::lb?-31783:31783) && !spin.y && spin.yaw_rate==0,
            "actual grab output reaches the existing native LS spin actuator with no SDK body rate");
        check(r->lease.acknowledge(4,1,0,1,true,true,true),
            "fixture supplies independent native grab acknowledgment before testing held release");
        const auto episode=r->frame.airborne_episode;
        for(unsigned n=2;n<8;++n) {
            out=r->poll(at(n),buttons);spin=rotate(*r,rotation,out,buttons);
            check(spin.x!=0 && out.action.pad.right_trigger==255 && r->requests.size()==1 &&
                r->frame.airborne_episode==episode,"held grab and held shoulder retain independent channels in the same flight");
            AirMotionIntent intent{};intent.enabled=true;intent.angular_allowed=true;intent.sampled_at=r->now;
            intent.yaw_rate=spin.yaw_rate;intent.spin=spin.spin;intent.angular_proof=NativeAirSpinProof::from_frame(r->frame);
            const auto physical=motion.step(intent,{r->now,r->owner.core,201,1.f/hz,5,0,0,true,0,0,true,
                NativeAirSpinProof::from_frame(r->frame)});
            check(!physical.angular && physical.spin_y==0,"native grab/shoulder composition never adds custom angular motion");
        }
        out=r->poll(at(8),0x2004);spin=rotate(*r,rotation,out,0x2004);
        check(!spin.owns_left && !spin.x && out.action.pad.right_trigger==255,
            "shoulder release stops native LS without releasing the held grab");
        out=r->poll(at(9),static_cast<std::uint16_t>(0x2004|BoardChord::pair));
        spin=rotate(*r,rotation,out,static_cast<std::uint16_t>(0x2004|BoardChord::pair));
        check(!spin.x && spin.yaw_rate==0 && out.action.pad.right_trigger==255,
            "opposed shoulders cannot create spin or steal the grab trigger");
        out=r->poll(at(10),buttons);spin=rotate(*r,rotation,out,buttons,
            SpinMode::held,{SpinFact::set,SpinFact::unknown,SpinFact::unknown});
        check(!spin.owns_left && !spin.x,"native spin prohibition still wins during a trigger-only grab");
        check(!rotate(*r,rotation,out,buttons,SpinMode::held,{},true).owns_left,
            "reference action ownership still blocks native shoulder sharing");
        check(!rotate(*r,rotation,out,buttons,SpinMode::tap).owns_left,
            "grab sharing never enables a tap body-rate writer");
        auto stale_rotation=rotation;const auto sampled_now=r->now;r->now+=151;
        check(!rotate(*r,stale_rotation,out,buttons).owns_left,"stale native AIR proof still rejects rotation");
        r->now=sampled_now;
        out=r->poll(at(11),shoulder);
        check(!out.action.native_grab_triggers && !out.action.pad.owns_triggers,
            "B release withdraws the acknowledged grab channel even while a shoulder remains held");
        out=r->poll(at(20),shoulder,100);spin=rotate(*r,rotation,out,shoulder);
        check(!out.action.native_grab_triggers && !out.action.pad.owns_triggers && !spin.owns_left,
            "landing removes the sharing receipt and cannot turn into ground steering");
        out=r->poll(at(21),buttons,201,-1,false);
        check(!out.action.native_grab_triggers,"Arcade OFF cannot publish a sharing receipt");
    }
    for(unsigned fault=0;fault<7;++fault) {
        auto r=std::make_unique<Route>();r->poll(1000);auto out=r->poll(1017,0x2000);
        check(!out.action.blocks_air_rotation(true),"only a current native trigger-only grab qualifies");
        if(fault==0)out.action.native_grab_triggers=false;
        if(fault==1)out.action.pad.owns_triggers=false;
        if(fault==2)out.action.pad.owns_left_stick=true;
        if(fault==3)out.action.pad.owns_right_stick=true;
        if(fault==4)out.action.pad.owns_face_buttons=true;
        if(fault==5)out.action.pad.left_trigger=128;
        if(fault==6)out.action.pad.right_trigger=0;
        check(out.action.blocks_air_rotation(true),"another channel owner or invalid grab receipt remains exclusive");
    }
    auto owner=std::make_unique<Route>();owner->poll(1000);owner->poll(1017,0x2000);
    ++owner->owner.entity;
    check(!owner->poll(1034,0x2000).action.native_grab_triggers,
        "a replaced local skater cannot inherit native grab channel sharing");
}
void requests() {
    for(unsigned hz:{30u,60u,120u}) for(unsigned variant:{0u,1u,2u}) {
        auto r=std::make_unique<Route>();auto at=[&](unsigned n){return 1000+std::uint64_t{n}*1000/hz;};
        r->poll(at(0));const std::uint16_t direction=variant?4:0;
        auto out=r->poll(at(1),static_cast<std::uint16_t>(0x2000|direction));
        check(r->requests.size()==1,"physical B reaches the production native callback in its press poll");
        check(out.action.pad.owns_triggers && out.action.pad.right_trigger==255 && !out.action.pad.left_trigger,
              "B reaches the native trigger input that establishes grab eligibility in the same poll");
        check(!out.action.pad.owns_right_stick && !out.authored_playback,"grab delivery never emits a flip flick or replays the pop");
        check(r->requests[0].physical_pressed_at==at(1),"original physical press timestamp survives native handoff");
        check(r->lease.intent(at(1)) && !r->lease.acknowledged(),"queueing is not native animation acknowledgment");
        if(variant==2) {r->poll(at(2),direction);r->poll(at(3),static_cast<std::uint16_t>(0x2000|direction));}
        const auto expected=variant==0?ArcadeTrick::basic_bs_grab:variant==1?ArcadeTrick::melon:ArcadeTrick::method;
        check(r->tricks.back()==expected,"neutral, left B and left repeated B retain exact semantic identity");
        const auto count=r->requests.size();
        for(unsigned n=4;n<10;++n)r->poll(at(n),static_cast<std::uint16_t>(0x2000|direction));
        check(r->requests.size()==count,"one held physical input never repeats native requests");
        out=r->poll(at(10));check(r->lease.intent(at(10)) && out.action.pad.right_trigger==255,
              "brief release retains native input until evidence without a fixed setup interval");
        const auto detail=native_grab::pilot(expected)->detail;
        check(!r->lease.acknowledge(detail+20,1,0,1,true,true,true),"wrong actual native identity cannot acknowledge the semantic request");
        check(!r->lease.acknowledge(detail,0,0,1,true,true,true),"prediction cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,0,true,true,true),"stalled animation cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,1,false,true,true),"missing native initialization cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,1,true,false,true),"another actor cannot acknowledge a grab");
        check(r->lease.acknowledge(detail,1,0,1,true,true,true),"independent exact actor advancement and native tag complete delivery");
        check(!r->lease.intent(at(10)),"released tap withdraws intent immediately upon native acknowledgment");
        out=r->poll(at(11));check(!out.action.pad.owns_triggers,"acknowledged released tap stops native trigger delivery");
        out=r->poll(at(12),0,100);check(!r->lease.selected() && !out.action.pad.owns_triggers,"landing clears the native request owner and trigger");
    }
}
void complete_direct_families() {
    struct Binding {std::uint16_t buttons;ArcadeTrick trick;int detail;unsigned hands;bool repeat;};
    const Binding rows[]{
        {0x2000,ArcadeTrick::basic_bs_grab,1,2,false},{0x2004,ArcadeTrick::melon,4,2,false},
        {0x2004,ArcadeTrick::method,2,2,true},{0x2005,ArcadeTrick::japan,3,2,false},
        {0x2008,ArcadeTrick::indy,6,1,false},{0x2008,ArcadeTrick::stiffy,10,1,true},
        {0x2085,ArcadeTrick::crossbone,5,2,false},{0x2089,ArcadeTrick::fs_nosebone,7,1,false},
        {0x2086,ArcadeTrick::fs_tailbone,8,1,false},{0x208a,ArcadeTrick::tuck_knee,9,1,false},
        {0x2080,ArcadeTrick::double_grab,33,3,false},{0x2081,ArcadeTrick::double_grab_front,36,3,false},
        {0x2082,ArcadeTrick::double_grab_back,37,3,false},{0x2084,ArcadeTrick::double_grab_left,34,3,false},
        {0x2088,ArcadeTrick::double_grab_right,35,3,false}};
    for(unsigned hz:{30u,60u,120u})for(const auto& row:rows) {
        auto r=std::make_unique<Route>();const unsigned dt=(1000u+hz-1)/hz;
        r->poll(1000);auto out=r->poll(1000+dt,row.buttons);
        if(row.repeat) {r->poll(1000+2*dt,static_cast<std::uint16_t>(row.buttons&~0x2000u));out=r->poll(1000+3*dt,row.buttons);}
        check(!r->tricks.empty() && r->tricks.back()==row.trick,"each direct-family binding reaches its intended native profile");
        check(out.action.pad.left_trigger==((row.hands&1u)?255:0) && out.action.pad.right_trigger==((row.hands&2u)?255:0),
            "original hand-input producer receives exactly the selected native hand pair");
        check(!out.action.pad.owns_right_stick && !out.action.pad.owns_left_stick && !out.authored_playback,
            "native grab never injects a pop or controller flick");
        check(!out.action.blocks_air_rotation(true),"all direct hand pairs retain native shoulder spin");
        check(!(row.buttons&0x80u) || (out.action.consume_buttons&0x80u),"extended chord cannot leak its R3 press alongside B");
        check(!r->lease.acknowledge(row.detail,1,0,1,true,false,true),"matching detail without exact actor cannot acknowledge");
        check(!r->lease.acknowledge(row.detail,0,1,1,true,true,true),"nonadvancing native actor cannot acknowledge");
        // Contract only: the independent native observer must provide these facts in gameplay.
        check(r->lease.acknowledge(row.detail,1,0,1,true,true,true),"exact advancing actor can acknowledge its own profile");
        out=r->poll(1000+4*dt,static_cast<std::uint16_t>(row.buttons&~0x2000u));
        check(!out.action.pad.owns_triggers,"B release after acknowledgment withdraws both hands");
        out=r->poll(1000+5*dt,0,100);
        check(!out.action.pad.owns_triggers,"landing cannot retain a native grab input owner");
    }
}
void selected_native_families() {
    const auto* seatbelt=native_grab::pilot(ArcadeTrick::seatbelt_bs_shifty);
    check(seatbelt && seatbelt->family==4 && seatbelt->detail==24 && seatbelt->x==1 && seatbelt->y==0 &&
        seatbelt->hands==2 && seatbelt->selector_y==28000,
        "Seatbelt BS uses the original family/blend endpoint and actual emitted tag, never declaration17 or the Nose family");
    for(unsigned hz:{30u,60u,120u}) for(const auto& profile:native_grab::pilots) {
        auto r=std::make_unique<Route>();r->selected=profile.trick;
        const auto dt=(1000u+hz-1)/hz;r->poll(1000);
        auto out=r->poll(1000+dt,0x2080);
        check(r->tricks.size()==1 && r->tricks.back()==profile.trick,"each selectable native pose has a reachable R3+B binding");
        const auto control=profile.control();
        if (control.selector()) {
            check(!out.action.pad.left_trigger && !out.action.pad.right_trigger &&
                out.action.pad.right_x==control.x && out.action.pad.right_y==control.y,
                "every directional family publishes its selector before a hand");
            r->grab_observation.selector_ready=true;
            out=r->poll(1000+dt+1,0x2080);
        }
        check(out.action.pad.left_trigger==((control.hands&1u)?255:0) && out.action.pad.right_trigger==((control.hands&2u)?255:0) &&
            out.action.pad.right_x==control.x && out.action.pad.right_y==control.y,
            "family admission receives the original paired hand and RS input from one lease");
        check(!out.authored_playback && out.action.pad.owns_right_stick==control.selector() &&
            out.action.native_grab_selector==control.selector() && !out.action.pad.owns_left_stick && !out.action.pad.owns_face_buttons,
            "native family selection owns only its established channels and never creates a second Ollie");
        check(!out.action.blocks_air_rotation(true) && out.action.blocks_air_rotation(false),
            "only native held shoulders share the family selector; custom spin remains excluded");
        check(!trick_name(profile.trick).empty(),"every selectable profile has a discoverable name");
        check(!r->lease.acknowledge(profile.detail,1,0,1,true,false,true),"native tag alone never proves exact selected pose");
        if (profile.one_footer) {
            check(!r->lease.acknowledge(profile.detail,1,0,1,true,true,true,profile.one_footer),
                "a stale one-footer tag cannot acknowledge before the native foot request");
            r->grab_observation={true,true,true,(profile.hands&1)!=0,(profile.hands&2)!=0,profile.family};
            r->foot_settings_ready=false;
            out=r->poll(1000+2*dt,0x2080);
            check(!out.action.pad.owns_face_buttons,"native grab alone cannot bypass unconsumed foot-control preferences");
            r->foot_settings_ready=true;
            out=r->poll(1000+3*dt,0x2080);
            check(out.action.native_grab_feet && out.action.pad.face_buttons==native_grab::foot_button(profile.foot) &&
                !out.authored_playback && !out.jump.released,"established grab receives exactly its original foot input, with no pop");
            check(!out.action.blocks_air_rotation(true),"one-footer native face input preserves native shoulder spin");
            check(!r->lease.acknowledge(profile.detail,1,0,1,true,true,true),"starting grab cannot acknowledge a one-footer");
            check(!r->lease.acknowledge(profile.detail,1,0,1,true,true,true,profile.one_footer%13+1),
                "another native one-footer identity cannot acknowledge");
        }
        check(r->lease.acknowledge(profile.detail,1,0,1,true,true,true,profile.one_footer),"fixture supplies exact native actor advancement");
        r->selected=ArcadeTrick::double_grab;
        out=r->poll(1000+4*dt,0x2000);
        check(r->tricks.size()==1 && r->lease.selected()->trick==profile.trick && out.action.pad.right_x==control.x && out.action.pad.right_y==control.y,
            "changing the menu choice cannot mutate an already held grab");
        out=r->poll(1000+5*dt);
        check(!out.action.pad.owns_triggers && !out.action.pad.owns_right_stick && !out.action.pad.owns_face_buttons,
            "acknowledged B release withdraws selector, feet and both hands together");
        out=r->poll(1000+6*dt,0,100);
        check(!r->lease.selected() && !out.action.owned,"landing clears the complete native input lease");
    }
    auto buffered=std::make_unique<Route>();buffered->selected=ArcadeTrick::nosegrab;
    buffered->poll(900,0,100);buffered->poll(1000,0,200,1);buffered->client_action_ready=false;
    buffered->poll(1010,0x2080,200,1);buffered->selected=ArcadeTrick::tailgrab;
    buffered->poll(1020,0,200,1);buffered->client_action_ready=true;
    buffered->poll(1030,0,201,1);
    check(buffered->tricks.size()==1 && buffered->tricks[0]==ArcadeTrick::nosegrab && buffered->requests[0].physical_pressed_at==1010,
        "200-to-201 buffering retains the selected profile from the physical B event");
    auto missing=std::make_unique<Route>();missing->selector_available=false;missing->selected=ArcadeTrick::nosegrab;
    missing->poll(1000);auto out=missing->poll(1017,0x2080);
    check(missing->requests.empty() && !out.action.owned,"a hand-only adapter cannot substitute BS for a family requiring native RS admission");
    for(const auto binding:{std::uint16_t{0x2001},std::uint16_t{0x2002}}) {
        auto r=std::make_unique<Route>();r->poll(1000);r->poll(1017,binding);
        check(r->tricks.size()==1 && r->tricks[0]==(binding==0x2001?ArcadeTrick::nosegrab:ArcadeTrick::tailgrab),
            "original Up+B and Down+B now reach genuine Nose and Tail profiles");
    }
    auto rocket=std::make_unique<Route>();rocket->poll(1000);rocket->poll(1017,0x2001);
    rocket->grab_observation.selector_ready=true;
    rocket->poll(1034,0x0001);out=rocket->poll(1051,0x2001);
    check(rocket->tricks.size()==2 && rocket->tricks.back()==ArcadeTrick::rocket_air &&
        out.action.pad.left_trigger==255 && out.action.pad.right_trigger==255 && out.action.pad.right_y==-28000,
        "Up+B,B enters original Rocket from Nose using both hands and original Nose admission");
    check(!rocket->lease.acknowledge(21,1,0,1,true,true,true),"the preceding Nose actor cannot acknowledge Rocket");
}
void native_grab_after_stance_change() {
    // Captured original controls in leading-foot stance 0x20: RT -> FS,
    // LT -> BS, RS negative X -> Stale/Weddle; nose selector keeps Y.
    for (bool switched:{false,true}) for (const auto& profile:native_grab::pilots) {
        auto r=std::make_unique<Route>();r->switched=switched;r->selected=profile.trick;
        r->poll(1000);auto out=r->poll(1034,0x2080);
        check(r->requests.size()==1 && r->tricks.back()==profile.trick,
            "known native stance admits the same catalog entry regardless of mirror direction");
        r->grab_observation.selector_ready=true;out=r->poll(1035,0x2080);
        const auto c=profile.control();
        check(out.action.pad.left_trigger==((c.hands&(switched?2u:1u))?255:0) &&
              out.action.pad.right_trigger==((c.hands&(switched?1u:2u))?255:0),
              "native hand semantics survive a leading-foot change without relabeling the trick");
        check(out.action.pad.right_x==(switched?-c.x:c.x) && out.action.pad.right_y==c.y,
              "horizontal family selectors mirror once while nose/tail keep their native direction");
        out=r->poll(1068,0,100);
        check(!out.action.pad.owns_triggers && !out.action.pad.owns_right_stick,
              "landing retires the changed-stance grab controls");
    }
    const native_grab::Control finger{1,12000,-23000,0,true};
    const auto result=native_grab::physical_control(finger,false);
    check(result.hands==2 && result.x==finger.x && result.y==finger.y,
          "an authored fingerflip already mirrored by playback is not mirrored twice");
    NativeInputReadiness readiness;
    check(!readiness.sample(1000,{}).stance_known,"missing native owner evidence never grants stance admission");
}
void foot_setting_admission() {
    dingosdk::input::GameplayOptions options;
    check(!options.sampled("egf_invertxaonboard",1000,1000),"unknown settings do not grant foot input");
    check(options.observe("egf_invertxaonboard",true,1000,false),"record the original preference");
    check(options.hold("foot","egf_invertxaonboard",false,1001).empty(),"lease can temporarily hold an observed preference");
    check(!options.sampled("egf_invertxaonboard",1002,1002),"requesting a preference does not prove native consumption");
    check(options.observe("egf_invertxaonboard",true,1003,true),"UI retains the original preference");
    check(!options.sampled("egf_invertxaonboard",1002,1003),"UI reads cannot qualify foot input");
    check(!options.observe("egf_invertxaonboard",true,1004,false) &&
        options.sampled("egf_invertxaonboard",1002,1005)==false,"native producer must read the applied preference");
    check(options.get("egf_invertxaonboard")==true,"temporary foot routing never saves a preference");
    check(!options.sampled("egf_invertxaonboard",1002,1155),"stale setting observations fail closed");
    check(!options.sampled("egf_invertxaonboard",1002,1003),"clock reversal fails closed");
    options.release("foot");
    check(options.observe("egf_invertxaonboard",true,1006,false),"release restores the original control configuration");
    native_grab::Lease lease;
    lease.request(ArcadeTrick::benihana,2000,true);
    auto wrong=native_grab::RetargetObservation{true,true,true,true,false,2};
    check(!lease.controls(wrong,true).feet,"wrong established family cannot emit foot input");
    wrong.family=3;wrong.right=true;
    check(!lease.controls(wrong,true).feet,"wrong hand pair cannot emit foot input");
    wrong.right=false;wrong.valid=false;
    check(!lease.controls(wrong,true).feet,"unknown native owner cannot emit foot input");
    wrong.valid=true;
    check(lease.controls(wrong,true).feet==0x4000,"original left-foot producer follows actual Tail entry");
    lease.refresh(2017,true,false);
    check(!lease.controls(wrong,true).feet,"owner loss withdraws the entire foot request");
}
void delayed_tap_and_controls() {
    {
        auto route=std::make_unique<Route>();route->selected=ArcadeTrick::nosegrab;
        route->poll(900,0,100);route->poll(1000,0,201,1);
        const auto out=route->poll(1010,0x2080,201,1);
        check(out.action.pad.owns_right_stick && out.action.pad.right_y==-28000 &&
            out.action.pad.owns_triggers && !out.action.pad.left_trigger && !out.action.pad.right_trigger,
            "Nosegrab must deliver selector before either hand when native selector is not yet observed");
        route->grab_observation.selector_ready=true;
        const auto admitted=route->poll(1020,0x2080,201,1);
        check(admitted.action.pad.right_trigger==255 && admitted.action.pad.right_y==-28000,
            "original selector acknowledgment admits the same retained physical grab");
    }
    for(unsigned hz:{30u,60u,120u}) {
        // Modifier/direction belong to the original press even if native AIR
        // readiness arrives after both controls have physically been released.
        auto extended=std::make_unique<Route>();
        extended->poll(900,0,100);
        extended->poll(1000,0,200,1);
        extended->client_action_ready=false;
        extended->poll(1010,0x2089,200,1);
        extended->poll(1020,0,200,1);
        extended->client_action_ready=true;
        extended->poll(1030,0,201,1);
        check(extended->tricks.size()==1 && extended->tricks[0]==ArcadeTrick::fs_nosebone,
            "R3 plus directional B survives 200 to 201 with its original binding");
        check(extended->requests[0].physical_pressed_at==1010,
            "buffered extended grab retains the original B event timestamp");
        auto r=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        r->poll(1000);r->accept=false;r->poll(1000+dt,0x2004);r->poll(1000+2*dt);
        r->accept=true;r->poll(1000+3*dt);
        check(r->requests.size()==1 && r->tricks[0]==ArcadeTrick::melon,"released directional tap survives native delivery wait");
        check(r->requests[0].physical_pressed_at==1000+dt,"waiting never renews the physical event");
        check(r->lease.intent(1000+3*dt),"released tap is admitted to native initialization");
        auto ground=std::make_unique<Route>();ground->poll(1000,0,100);
        const auto out=ground->poll(1000+dt,0x6000,100);
        check(!(out.buttons&0x6000) && ground->requests.empty(),"ground X/B remain suppressed and never start a grab");
        auto off=std::make_unique<Route>();off->poll(1000);off->poll(1000+dt,0x2004,201,-1,false);
        check(off->requests.empty(),"no native request while gameplay authority is off");
    }
    native_grab::Lease lease;lease.request(ArcadeTrick::melon,1000,true);
    check(lease.acknowledge(4,1,0,1,true,true,true),"held exact grab acknowledges without releasing");
    for(std::uint64_t now=1100;now<5000;now+=100){lease.refresh(now,true,true);check(lease.intent(now),"held pose uses physical lifetime");}
    lease.refresh(5000,false,true);check(!lease.intent(5000),"held release is immediate after acknowledgment");
    lease.request(ArcadeTrick::method,6000,false);
    for(std::uint64_t now=6100;now<=7600;now+=100)lease.refresh(now,false,true);
    check(!lease.selected(),"absent native evidence expires instead of fabricating success");
    lease.request(ArcadeTrick::melon,8000,true);lease.refresh(8200,true,true);
    check(!lease.selected(),"lost physical heartbeat cancels");
    check(!lease.request(ArcadeTrick::madonna,9000,true),"unqualified named grab never aliases a supported pose");
}
void repeated_grab_after_unsupported_first_binding() {
    for (unsigned hz:{30u,60u,120u}) {
        const auto dt=1000u/hz;
        auto r=std::make_unique<Route>();r->poll(900,0,100);r->poll(1000);
        r->poll(1000+dt,0x2009);
        check(r->requests.empty(),"unsupported Madonna never substitutes another native grab");
        r->poll(1000+2*dt,0x0009);
        r->poll(1000+3*dt,0x2009);
        check(r->requests.size()==1 && r->tricks.back()==ArcadeTrick::judo,
            "physical second B reaches supported Judo despite unsupported first binding");
        check(r->requests.back().presses==2 && r->requests.back().physical_pressed_at==1000+3*dt,
            "Judo retains second physical edge rather than executing a substitute first grab");
        auto retired=std::make_unique<Route>();
        retired->poll(900,0,100);retired->poll(1000,0,201,1);
        retired->poll(1000+dt,0x2009,201,1);
        retired->poll(1000+2*dt,0x0009,201,-1);
        retired->poll(1000+3*dt,0x2009,201,-1);
        check(retired->requests.size()==1 && retired->tricks.back()==ArcadeTrick::judo,
            "unsupported first grab keeps its physical repeat through observed Ollie selection retirement");
        for(unsigned fault=0;fault<5;++fault) {
            auto bad=std::make_unique<Route>();
            bad->poll(900,0,100);bad->poll(1000,0,201,1);
            bad->poll(1000+dt,0x2009,201,1);
            if(fault==0)++bad->owner.entity;
            if(fault==1)++bad->selection_object;
            bad->poll(1000+2*dt,0x0009,fault==2?100:201,fault==3?2:-1);
            bad->poll(fault==4?1300:1000+3*dt,0x2009,fault==2?100:201,fault==3?2:-1);
            check(bad->requests.empty(),
                "retired unsupported prefix rejects owner/selector replacement, landing, another flip and timeout");
        }
        for(unsigned fault=0;fault<3;++fault) {
            auto bad=std::make_unique<Route>();bad->poll(900,0,100);bad->poll(1000);
            bad->poll(1000+dt,0x2009);bad->poll(1000+2*dt,0x0009);
            if(fault==0)++bad->owner.entity;
            bad->poll(fault==2?1300:1000+3*dt,0x2009,fault==1?100:201);
            check(bad->requests.empty(),"unsupported-prefix repeat cannot survive owner replacement, landing or timeout");
        }
    }
}
void repeated_grab_uses_physical_interval() {
    for (unsigned hz:{30u,60u,120u}) for (unsigned interval:{200u,300u}) {
        auto r=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        const auto delivery_frame=1+(100u+dt-1)/dt;
        const auto second_frame=1+(interval+dt-1)/dt;
        r->poll(1000);r->accept=false;
        for(unsigned n=1;n<=second_frame;++n) {
            if(n==delivery_frame)r->accept=true;
            r->poll(1000+n*dt,static_cast<std::uint16_t>(4u|((n==1 || n==second_frame)?0x2000u:0u)));
        }
        check(r->requests.size()==2,"each physical B press is delivered once despite initial native wait");
        check(r->requests.front().physical_pressed_at==1000+dt &&
              r->requests.back().physical_pressed_at==1000+second_frame*dt,
              "grab delivery retains both original physical press timestamps");
        check(r->tricks.front()==ArcadeTrick::melon &&
              r->tricks.back()==(interval==200?ArcadeTrick::method:ArcadeTrick::melon),
              "native admission delay must not extend the physical repeated-B window");
        check(r->requests.front().air_episode==r->requests.back().air_episode,
              "physical repeated-B selection preserves the original airborne episode");
    }
}
void input_order_and_launch() {
    for (unsigned hz:{30u,60u,120u}) {
        const auto dt=(1000u+hz-1)/hz;
        auto direction_first=std::make_unique<Route>();
        direction_first->poll(1000);direction_first->poll(1000+dt,4);
        direction_first->poll(1000+2*dt,0x2304);
        check(direction_first->tricks.size()==1 && direction_first->tricks[0]==ArcadeTrick::melon,
              "direction before B with airborne shoulder rotation retains Melon");
        auto face_first=std::make_unique<Route>();face_first->poll(1000);
        face_first->poll(1000+dt,0x2000);face_first->poll(1000+2*dt,0x2004);
        check(face_first->tricks.size()==1 && face_first->tricks[0]==ArcadeTrick::basic_bs_grab,
              "later direction cannot relabel or repeat an already consumed B press");
        auto jump=std::make_unique<Route>();jump->poll(1000,0,100);
        jump->poll(1000+dt,0x1000,100);jump->poll(1000+2*dt,0,100);
        // Native flight is supplied as an observation, never simulated by the grab adapter.
        unsigned n=3;for (;n<30;++n) jump->poll(1000+n*dt,0,201);
        const auto episode=jump->frame.airborne_episode;
        jump->poll(1000+n*dt,0x2000,201);
        check(jump->requests.size()==1 && jump->tricks[0]==ArcadeTrick::basic_bs_grab,
              "production A launch input then observed flight admits B");
        check(episode && jump->frame.airborne_episode==episode,"grab admission preserves the observed airborne episode");
    }
}
void native_air_before_client_readiness() {
    for (unsigned hz:{30u,60u,120u}) for(bool held:{false,true}) {
        const auto dt=(1000u+hz-1)/hz;
        auto r=std::make_unique<Route>();
        r->poll(1000,0,100);
        r->poll(1000+dt,0x1000,100);
        r->poll(1000+2*dt,0,103,1);
        for(auto at=1000+3*dt;at<1600;at+=dt)r->poll(at,0,103,1);
        r->client_action_ready=false;
        r->poll(1600,0x2004,200,1);
        const auto episode=r->frame.airborne_episode;
        r->poll(1600+dt,held?0x2004:4,201,1);
        check(r->requests.empty(),"native AIR history alone cannot bypass client/action admission");
        r->client_action_ready=true;
        const auto out=r->poll(1600+2*dt,held?0x2004:4,201,1);
        check(r->requests.size()==1 && r->tricks.front()==ArcadeTrick::melon,
              "B observed in native 200 survives forward 201 and delayed client action admission");
        check(r->requests.front().physical_pressed_at==1600 &&
              r->requests.front().air_episode.number==episode && r->frame.airborne_episode==episode,
              "delayed action admission preserves the original physical edge and flight");
        check(!out.jump.released && !out.authored_playback && out.action.pad.right_trigger==255,
              "early grab handoff uses the native trigger without another pop");
        r->poll(1600+3*dt,held?0x2004:4,201,1);
        check(r->requests.size()==1,"preserving a held early B does not synthesize another press");
        r->poll(1600+4*dt,4,201,1);
        r->poll(1600+5*dt,0x2004,201,1);
        check(r->requests.size()==2 && r->tricks.back()==ArcadeTrick::method &&
              r->requests.back().physical_pressed_at==1600+5*dt,
              "the next physical B selects Method using the original early press interval");
    }
    for(unsigned hz:{30u,60u,120u}) for(unsigned fault=0;fault<8;++fault) {
        const auto dt=(1000u+hz-1)/hz;
        auto r=std::make_unique<Route>();r->poll(1000,0,100);
        r->client_action_ready=false;
        r->poll(1000+dt,0x2004,fault==0?100:200,1);
        if(fault==1)++r->owner.entity;
        if(fault==2)r->selection_object=32;
        if(fault==3)r->poll(1000+2*dt,4,100,1);
        if(fault==4)r->poll(1000+2*dt,4,201,5);
        if(fault==5)r->poll(1000+2*dt,4,201,1,false);
        if(fault==6)r->selection_valid=false;
        const auto admitted_at=fault==7?1300u:1000+3*dt;
        r->client_action_ready=true;
        r->poll(admitted_at,4,201,1);
        check(r->requests.empty(),
              "ground B, owner/selector change, landing, other action, OFF or stale history never borrows grab admission");
    }
    for(unsigned hz:{30u,60u,120u}) {
        const auto dt=(1000u+hz-1)/hz;
        auto r=std::make_unique<Route>();r->poll(1000,0,100);
        r->client_action_ready=false;r->poll(1000+dt,0x2004,200,1);
        for(auto at=1000+2*dt;at<1700;at+=dt)r->poll(at,4,201,1);
        r->client_action_ready=true;r->poll(1700,4,201,1);
        check(r->requests.empty(),"delayed client readiness cannot renew an expired physical B edge");
    }
}
void repeated_grab_after_native_jump_retirement() {
    for (unsigned hz:{30u,60u,120u}) {
        const auto dt=(1000u+hz-1)/hz;
        auto r=std::make_unique<Route>();
        r->poll(1000,0,201,1);
        r->poll(1000+dt,0x2004,201,1);
        const auto first=r->requests.back();
        r->grab_observation={true,true,true,false,true,2};
        r->poll(1000+2*dt,4,201,-1);
        r->poll(1000+3*dt,0x2004,201,-1);
        check(r->requests.size()==2 && r->tricks.back()==ArcadeTrick::method,
              "second physical B survives native Ollie selection retirement in the same air episode");
        check(r->requests.back().physical_pressed_at==1000+3*dt &&
              r->requests.back().air_episode==first.air_episode,
              "repeat retains the second physical edge and original airborne episode");
        check(r->requests.back().generations.action==r->frame.action_generation &&
              first.generations.action!=r->requests.back().generations.action,
              "new physical repeat uses the independently qualified current action generation");
        const auto& proof=r->input.input_diagnostic(0)->grab_repeat;
        check(proof.observed && proof.extra && proof.native_adapter && proof.retired && proof.native_owner &&
              proof.before.flip.object==proof.current.flip.object && proof.origin.action==first.generations.action,
              "capture retains the actual repeat qualification without treating it as animation evidence");
        for (unsigned invalid=0;invalid<3;++invalid) {
            auto bad=std::make_unique<Route>();bad->poll(1000,0,201,1);
            bad->poll(1000+dt,0x2004,201,1);
            if (invalid==0) bad->poll(1000+2*dt,4,201,5);
            if (invalid==1) bad->owner.entity=9;
            bad->poll(1000+3*dt,4,invalid==2?100:201,-1);
            bad->poll(1000+4*dt,0x2004,201,-1);
            check(bad->tricks.back()!=ArcadeTrick::method,
                  "different action, owner replacement or landing never inherit the prior grab repeat");
        }
        auto replacement=std::make_unique<Route>();replacement->poll(1000,0,201,1);
        replacement->poll(1000+dt,0x2004,201,1);replacement->selection_object=32;
        replacement->poll(1000+2*dt,4,201,-1);
        const auto rejected=replacement->poll(1000+3*dt,0x2004,201,-1);
        const auto* diagnostic=replacement->input.input_diagnostic(0);
        check(diagnostic->grab_repeat.observed && !diagnostic->grab_repeat.retired &&
              diagnostic->grab_repeat.before.flip.object!=diagnostic->grab_repeat.current.flip.object &&
              diagnostic->dispatched==1 && diagnostic->dispatch[0].disposition==InputDisposition::cancelled &&
              rejected.action.grab_gate==0 && replacement->requests.size()==1,
              "replacement selection identity is observable and still rejected before the adapter");
    }
}
void independent_native_identity_fields() {
    for(const auto& profile:native_grab::pilots) for(bool available:{false,true}) {
        native_grab::Lease lease;
        check(lease.request(profile.trick,1000,true),"native identity regression starts a known profile");
        if(profile.one_footer)lease.controls({true,true,true,(profile.hands&1)!=0,(profile.hands&2)!=0,profile.family},true);
        unsigned reads{};
        const auto identity=native_grab::observe_identity(profile,[&](bool footer,std::int32_t& value) noexcept {
            ++reads;
            check(footer==(profile.one_footer!=0),"one-footer observation never requires the unallocated ordinary detail field");
            if(!available)return false;
            value=footer?profile.one_footer:profile.detail;
            return true;
        });
        check(reads==1 && identity.valid==available,"only the selected native identity is required; missing remains unknown");
        lease.refresh(1033,false,true);
        check(lease.intent(1033),"released input waits for independent actor evidence");
        const bool acknowledged=identity.valid && lease.acknowledge(identity.detail,1,0,1,true,true,true,identity.one_footer);
        check(acknowledged==available && lease.intent(1033)==!available,
            "exact advancing footer actor withdraws released input without an unrelated detail allocation");
    }
}
void repeated_grab_without_flip_selection() {
    for (unsigned hz:{30u,60u,120u}) for (unsigned failed=0;failed<9;++failed) {
        auto r=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        r->poll(1000,0,201,1);r->poll(1000+dt,0x2004,201,1);
        const auto first=r->requests.back();
        r->selection_valid=false;
        r->grab_observation={true,true,true,false,true,2};
        if(failed==1)r->grab_observation.valid=false;
        if(failed==2)r->grab_observation.grabbing=false;
        if(failed==3)r->grab_observation.can_grab=false;
        if(failed==4)r->grab_observation.left=true;
        if(failed==5)r->grab_observation.right=false;
        if(failed==6)r->grab_observation.family=3;
        if(failed==7)++r->grab_lineage.action;
        if(failed==8)++r->grab_air.number;
        r->poll(1000+2*dt,4,201,-1);
        const auto out=r->poll(1000+3*dt,0x2004,201,-1);
        const auto& proof=r->input.input_diagnostic(0)->grab_repeat;
        check(!proof.retired && !proof.current.flip.valid,
              "actual grab-entry loss of flip selection is retained, never relabeled as idle");
        check(proof.native_owner==!failed && r->requests.size()==(failed?1u:2u),
              "only the exact native BS-grab owner admits Method through unavailable flip selection");
        if(!failed)check(r->tricks.back()==ArcadeTrick::method && out.action.grab_gate==1 &&
            r->requests.back().physical_pressed_at==1000+3*dt && r->requests.back().air_episode==first.air_episode,
            "native grab continuation preserves the new physical edge and original flight");
    }
}
}
int main(){native_family_change_waits_for_exit();native_footer_exit_contract();current_air_read_preserves_only_verified_hold();requests();complete_direct_families();selected_native_families();native_grab_after_stance_change();foot_setting_admission();delayed_tap_and_controls();repeated_grab_after_unsupported_first_binding();repeated_grab_uses_physical_interval();input_order_and_launch();native_air_before_client_readiness();repeated_grab_after_native_jump_retirement();repeated_grab_without_flip_selection();independent_native_identity_fields();grab_and_native_shoulders();std::cout<<checks<<" native grab source/contract checks passed (gameplay unverified)\n";}
