#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
void check(bool value,const char* message) {if(!value){std::cerr<<message<<'\n';std::exit(1);}}
struct Receiver {
    InputRequest last{};unsigned calls{};
    InputAdapter adapter() {return {this,[](void* p,const InputRequest& request) noexcept {
        auto& self=*static_cast<Receiver*>(p);self.last=request;++self.calls;return InputDisposition::started;
    }};}
};
void recognition() {
    for (unsigned hz:{30u,60u,120u}) for (unsigned count:{1u,2u}) for (bool pad:{false,true}) {
        UnifiedInput input;Receiver receiver;InputEnvironment env{InputContext::ground,true};env.native_footplants=true;
        for (unsigned tick=0;tick<=hz;++tick) {
            const auto now=100+tick*1000/hz;
            const bool up=(now>=200 && now<270) || (count==2 && now>=310 && now<370);
            const bool a=now>=400 && now<500;
            PhysicalInputSample physical{static_cast<std::uint16_t>((a?0x1000:0)|(up&&pad?1:0)),0,
                static_cast<std::int16_t>(up&&!pad?25000:0)};
            input.poll(now,physical,env,receiver.adapter());
        }
        check(receiver.calls==1 && receiver.last.action==InputAction::crouch && receiver.last.variant==count,
            "Up/A and Up/Up/A dispatch exactly once without an ordinary release or synthetic second press");
    }
    for (unsigned test=0;test<6;++test) {
        UnifiedInput input;Receiver receiver;InputEnvironment env{InputContext::ground,true};env.native_footplants=true;
        if(test==0)env.native_footplants=false;
        if(test==1)env.context=InputContext::air;
        if(test==2)env.nollie=true;
        input.poll(100,test==3?PhysicalInputSample{0,0,25000}:PhysicalInputSample{},env,receiver.adapter());
        if(test!=3)input.poll(200,{0,0,25000},env,receiver.adapter());
        if(test==4) {input.poll(250,{0x4000},env,receiver.adapter());input.poll(270,{},env,receiver.adapter());}
        input.poll(test==5?600:300,{0x1000},env,receiver.adapter());
        check(!receiver.last.variant,"disabled, air, nollie, startup-held, intervening flip and expired prefixes keep footplants unavailable");
    }
}
FootplantObservation observation(std::uint64_t now,std::uint64_t step) {
    FootplantObservation out{};out.valid=true;out.state=100;out.sampled_at=now;out.step=step;return out;
}
void native_entry() {
    for (unsigned hz:{30u,60u,120u}) for (Footplant kind:{Footplant::no_comply,Footplant::boneless}) {
        FootplantInput input;auto native=observation(100,1);
        check(input.start(kind,100,native),"ground request starts");
        check(input.native_options()==(kind==Footplant::no_comply?
            std::array{true,true,true}:std::array{false,false,false}),
            "A preparation uses the native mongo push preset; Boneless retains absolute-foot plant controls");
        auto out=input.poll(100,native,false);
        check(out.owned && !out.buttons && !out.left_trigger && !out.right_y,"wait for actual settings consumption before native output");
        native=observation(120,2);out=input.poll(120,native,true);
        check(out.owned && (kind==Footplant::no_comply?out.buttons==0x1000:out.left_trigger==255),"original push or hand establishes prerequisite");
        native.can_no_comply=true;native.left_hand=true;native.grab=1;native.sampled_at=121;
        out=input.poll(121,native,true);
        check(!out.right_y && (kind==Footplant::no_comply || !out.buttons),"repeated input poll in same physics step cannot authorize entry");
        native.sampled_at=120+1000/hz;++native.step;out=input.poll(native.sampled_at,native,true);
        check(kind==Footplant::no_comply?out.right_y==-28000:out.buttons==0x1000,"native prerequisite permits original entry input");
        native.sampled_at+=1000/hz;++native.step;native.no_comply=kind==Footplant::no_comply;
        native.boneless=kind==Footplant::boneless;if(native.boneless)native.state=602;
        out=input.poll(native.sampled_at,native,true);
        check(!out.owned && input.entered(),"original entry withdraws input and leaves native launch/cleanup ownership");
        check(!input.poll(native.sampled_at+20,native,true).owned,"no replay from one command");
    }
    for(unsigned hz:{30u,60u,120u}) {
        FootplantInput input;auto native=observation(100,1);
        check(input.start(Footplant::no_comply,100,native),"No Comply starts while the original push finishes");
        for(unsigned tick=0;tick<=hz*3/2;++tick) {
            const auto now=100+tick*1000/hz;native=observation(now,tick+1);
            const auto out=input.poll(now,native,true);
            check(out.owned && out.buttons==0x1000 && !out.right_y,
                "regular push completion may exceed one second without fabricating the mongo prerequisite");
        }
        native=observation(1650,hz*2);native.can_no_comply=true;
        check(input.poll(1650,native,true).right_y==-28000,
            "late native mongo tag admits the original No Comply entry input");
        native=observation(1951,hz*3);
        check(!input.poll(1951,native,true).owned,"issued No Comply entry remains bounded to300ms");
        native=observation(3000,hz*4);check(input.start(Footplant::no_comply,3000,native),"new bounded request starts");
        input.poll(3000,native,true);native=observation(5001,hz*7);
        check(!input.poll(5001,native,true).owned,"missing native prerequisite still expires at two seconds");
    }
    for(unsigned test=0;test<7;++test) {
        FootplantInput input;auto native=observation(100,1);input.start(Footplant::boneless,100,native);
        input.poll(100,native,true);native.sampled_at=120;native.step=2;
        native.left_hand=true;native.grab=1;
        if(test==0)native.right_hand=true;
        if(test==1)native.grab=3;
        if(test==2)native.left_hand=false;
        if(test==3)native.valid=false;
        if(test==4)native.state=201;
        if(test==5)native.step=0;
        const auto out=input.poll(120,native,test!=6);
        check(!out.buttons,"wrong grab/hand, invalid owner, air, step rollback or lost settings cannot issue plant");
        native=observation(1200,100);
        check(!input.poll(1200,native,true).owned,"missing native entry expires without fallback Ollie");
    }
}
void session() {
    struct Native {
        Footplant kind{};unsigned count{};FootplantControl control{true};
        NativeFootplantRequest adapter() {return {this,[](void* p,Footplant requested_kind) noexcept {
            auto& s=*static_cast<Native*>(p);s.kind=requested_kind;++s.count;return true;
        },[](void* p) noexcept {return static_cast<Native*>(p)->control;}};}
    } native;
    InputSession input;InputAdmission env{};env.gameplay=env.may_start=true;env.pose=SkatePose::riding;
    env.action_mode=ActionExperiment::classic;env.footplant=native.adapter();
    input.poll(100,{},0,0,env);input.poll(200,{0,0,25000},0,0,env);
    auto out=input.poll(300,{0x1000},0,0,env);
    check(native.count==1 && native.kind==Footplant::no_comply && out.footplant_owned && !out.jump.setup_started,
        "production session routes footplant without starting authored Ollie");
    native.control={true,0x1000,0,-28000};env.tick=1;
    out=input.poll(320,{0x1000},100,200,env);
    check(out.action.pad.right_y==-28000 && out.action.pad.face_buttons==0x1000 && !out.automatic_push,
        "native output owns face, stick and triggers without automatic push or camera leakage");
    native.control={};out=input.poll(340,{0x1000},100,200,env);
    check(!out.footplant_owned && !out.right_x && !out.right_y,"withdrawn native owner returns neutral");
    input.poll(360,{0x1000},0,0,env);input.poll(380,{},0,0,env);
    check(native.count==1,"held A and its later release cannot start another footplant");
}
}
int main(){recognition();native_entry();session();std::cout<<"Native footplant input tests passed\n";}
