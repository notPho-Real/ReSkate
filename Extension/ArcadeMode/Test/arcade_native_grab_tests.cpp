#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include "Extension/ArcadeMode/arcade_native_grab_lease.h"
#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Route {
    ArcadeInput input{true};NativeInputState producer;NativeInputReadiness readiness;
    NativeInputOwner owner{1,2,3,4,5};NativeInputFrame frame{};
    native_grab::Lease lease;std::vector<InputRequest> requests;std::vector<ArcadeTrick> tricks;
    bool accept{true};std::uint64_t now{};std::uintptr_t selection_object{16};
    bool selection_valid{true};native_grab::RetargetObservation grab_observation{};
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
    static bool continues(void* p,InputGenerations origin,InputAirEpisode air) noexcept {
        auto& self=*static_cast<Route*>(p);
        return self.frame.valid && self.frame.airborne && self.owner==self.grab_owner &&
            origin==self.grab_lineage && air==self.grab_air &&
            InputNativeContext::from_frame(self.frame).air_episode()==air &&
            self.lease.intent(self.now) && self.grab_observation.eligible();
    }
    NativeGesture::Output poll(std::uint64_t at,std::uint16_t buttons=0,std::uint32_t state=201,
                              std::int32_t selection=-1,bool enabled=true) {
        now=at;NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:2;
        sample.words[1]=state==100?0xc000u:0;
        frame=producer.observe(at,owner,sample,{selection_valid,selection,selection_object});
        readiness.publish(frame,{true,state==100,false,false,false,false,false,false,true,0});
        input.publish_native_context(frame);input.publish_authority(at,frame.owner_generation,1,true);
        const auto admission=readiness.sample(at,frame);
        lease.refresh(at,(buttons&0x2000)!=0,enabled && frame.airborne);
        return input.poll(0,at,buttons,0,0,0,0,enabled,admission.start,false,0,false,false,true,at*60/1000,
            false,recipes,admission.mirror_x,false,frame.airborne?SkatePose::air:SkatePose::riding,
            false,false,false,false,admission.flip_ready,false,0,0,ActionExperiment::classic,admission.action,
            admission.mirror_x,false,{}, {this,submit,right_hand_input,continues});
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
        if(fault==5)out.action.pad.left_trigger=255;
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
void delayed_tap_and_controls() {
    for(unsigned hz:{30u,60u,120u}) {
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
    check(!lease.request(ArcadeTrick::indy,9000,true),"unqualified named grab never aliases a supported pose");
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
int main(){requests();delayed_tap_and_controls();repeated_grab_uses_physical_interval();input_order_and_launch();repeated_grab_after_native_jump_retirement();repeated_grab_without_flip_selection();grab_and_native_shoulders();std::cout<<checks<<" native grab source/contract checks passed (gameplay unverified)\n";}
