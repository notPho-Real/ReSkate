#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include "Extension/ArcadeMode/arcade_native_grab_lease.h"
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
    bool accept{true};std::uint64_t now{};
    std::vector<PreparedGesture> recipes{{1,false,GestureContext::ground,{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
    Route(){input.enable(true);input.set_recognition_capture(0,true);}
    static bool submit(void* p,const InputRequest& request,ArcadeTrick trick,bool held) noexcept {
        auto& self=*static_cast<Route*>(p);if(!self.accept)return false;
        self.requests.push_back(request);self.tricks.push_back(trick);
        return self.lease.request(trick,self.now,held);
    }
    NativeGesture::Output poll(std::uint64_t at,std::uint16_t buttons=0,std::uint32_t state=201,
                              std::int32_t selection=-1,bool enabled=true) {
        now=at;NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:2;
        sample.words[1]=state==100?0xc000u:0;
        frame=producer.observe(at,owner,sample,{true,selection,16});
        readiness.publish(frame,{true,state==100,false,false,false,false,false,false,true,0});
        input.publish_native_context(frame);input.publish_authority(at,frame.owner_generation,1,true);
        const auto admission=readiness.sample(at,frame);
        lease.refresh(at,(buttons&0x2000)!=0,enabled && frame.airborne);
        return input.poll(0,at,buttons,0,0,0,0,enabled,admission.start,false,0,false,false,true,at*60/1000,
            false,recipes,admission.mirror_x,false,frame.airborne?SkatePose::air:SkatePose::riding,
            false,false,false,false,admission.flip_ready,false,0,0,ActionExperiment::classic,admission.action,
            admission.mirror_x,false,{}, {this,submit});
    }
};
void requests() {
    for(unsigned hz:{30u,60u,120u}) for(unsigned variant:{0u,1u,2u}) {
        auto r=std::make_unique<Route>();auto at=[&](unsigned n){return 1000+std::uint64_t{n}*1000/hz;};
        r->poll(at(0));const std::uint16_t direction=variant?4:0;
        auto out=r->poll(at(1),static_cast<std::uint16_t>(0x2000|direction));
        check(r->requests.size()==1,"physical B reaches the production native callback in its press poll");
        check(!out.action.pad.owns_triggers && !out.authored_playback,"native grab request emits no trigger recipe or flip playback");
        check(r->requests[0].physical_pressed_at==at(1),"original physical press timestamp survives native handoff");
        check(r->lease.intent(at(1)) && !r->lease.acknowledged(),"queueing is not native animation acknowledgment");
        if(variant==2) {r->poll(at(2),direction);r->poll(at(3),static_cast<std::uint16_t>(0x2000|direction));}
        const auto expected=variant==0?ArcadeTrick::basic_bs_grab:variant==1?ArcadeTrick::melon:ArcadeTrick::method;
        check(r->tricks.back()==expected,"neutral, left B and left repeated B retain exact semantic identity");
        const auto count=r->requests.size();
        for(unsigned n=4;n<10;++n)r->poll(at(n),static_cast<std::uint16_t>(0x2000|direction));
        check(r->requests.size()==count,"one held physical input never repeats native requests");
        r->poll(at(10));check(r->lease.intent(at(10)),"brief release awaits native evidence without a fixed hold interval");
        const auto detail=native_grab::pilot(expected)->detail;
        check(!r->lease.acknowledge(detail+20,1,0,1,true,true,true),"wrong actual native identity cannot acknowledge the semantic request");
        check(!r->lease.acknowledge(detail,0,0,1,true,true,true),"prediction cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,0,true,true,true),"stalled animation cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,1,false,true,true),"missing native initialization cannot acknowledge a grab");
        check(!r->lease.acknowledge(detail,1,0,1,true,false,true),"another actor cannot acknowledge a grab");
        check(r->lease.acknowledge(detail,1,0,1,true,true,true),"independent exact actor advancement and native tag complete delivery");
        check(!r->lease.intent(at(10)),"released tap withdraws intent immediately upon native acknowledgment");
        r->poll(at(11),0,100);check(!r->lease.selected(),"landing clears the native request owner");
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
}
int main(){requests();delayed_tap_and_controls();input_order_and_launch();std::cout<<checks<<" native grab source/contract checks passed (gameplay unverified)\n";}
