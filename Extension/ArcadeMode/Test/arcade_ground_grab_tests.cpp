#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
GroundGrabObservation observation(std::uint64_t now,std::uint64_t step) {
    GroundGrabObservation out{};out.valid=true;out.state=100;out.sampled_at=now;out.step=step;return out;
}
void native_entry() {
    for(unsigned hz:{30u,60u,120u})for(auto kind:{GroundGrab::frontside,GroundGrab::backside,GroundGrab::double_grab,GroundGrab::coffin}) {
        GroundGrabInput input;auto native=observation(100,1);
        check(input.start(kind,100,native),"ordinary ground starts a native grab input owner");
        auto control=input.poll(100,native,true);
        check(control.owned && !control.buttons,"preparation never issues foot inputs");
        native.can_grab=true;native.left=kind!=GroundGrab::backside;native.right=kind!=GroundGrab::frontside;
        native.sampled_at=101;
        check(!input.poll(101,native,true).buttons,"a second poll in the same physics step cannot enter Coffin");
        for(unsigned step=2;step<hz*10;++step) {
            const auto now=100+step*1000/hz;native.sampled_at=now;native.step=step;
            native.coffin=kind==GroundGrab::coffin && step>2;
            control=input.poll(now,native,true);
            check(control.owned,"native acknowledgment sustains a ten-second held grab");
            check(control.left_trigger==(native.left?255:0) && control.right_trigger==(native.right?255:0),"hold preserves native hand selection");
            check(control.buttons==(kind==GroundGrab::coffin?0x5000:0),"only Coffin supplies native foot-release chord");
        }
        check(!input.poll(native.sampled_at,native,false).owned && !input.active(),"physical B release withdraws all native inputs");
    }
    for(unsigned failure=0;failure<7;++failure) {
        GroundGrabInput input;auto native=observation(100,1);input.start(GroundGrab::coffin,100,native);
        input.poll(100,native,true);native=observation(150,2);native.can_grab=true;native.left=native.right=true;
        if(failure==0)native.left=false;
        if(failure==1)native.can_grab=false;
        if(failure==2)native.valid=false;
        if(failure==3)native.state=200;
        if(failure==4)native.step=0;
        if(failure==5)native.sampled_at=0;
        check(!input.poll(150,native,failure!=6).buttons,"unqualified or released Coffin cannot produce a hippy-jump chord");
        native=observation(1400,80);
        check(!input.poll(1400,native,true).owned,"missing native admission times out without fallback or fake scoring");
    }
}
struct Native {
    unsigned requests{};GroundGrab kind{};bool active{};
    NativeGroundGrabRequest adapter(){return {this,[](void* p,GroundGrab k) noexcept {
        auto& n=*static_cast<Native*>(p);++n.requests;n.kind=k;n.active=true;return true;
    },[](void* p,bool held) noexcept {
        auto& n=*static_cast<Native*>(p);n.active=n.active&&held;
        return GroundGrabControl{n.active,0,255,255};
    }};}
};
InputAdmission admission(Native& native) {
    InputAdmission env{};env.gameplay=env.may_start=env.may_push=true;env.pose=SkatePose::riding;
    env.action_mode=ActionExperiment::classic;env.ground_grab=native.adapter();return env;
}
void session() {
    for(unsigned direction:{0u,4u,8u,2u}) {
        Native native;InputSession input;auto env=admission(native);
        input.poll(100,{},0,0,env);
        const auto button=static_cast<std::uint16_t>(0x2080u|direction);
        auto out=input.poll(200,{button},123,456,env);
        const auto expected=direction==4?GroundGrab::backside:direction==8?GroundGrab::frontside:direction==2?GroundGrab::coffin:GroundGrab::double_grab;
        check(native.requests==1 && native.kind==expected && out.ground_grab_owned,"R3 plus B dispatches native context and direction exactly once");
        check(!out.jump.setup_started && !out.automatic_push && !out.right_x && !out.right_y && !(out.buttons&0x2080u),"grab owner suppresses brake, camera click, auto push and Ollie");
        out=input.poll(220,{0x2000},0,0,env);
        check(out.ground_grab_owned,"R3 may be released while B sustains the grab");
        out=input.poll(240,{},0,0,env);
        check(!out.ground_grab_owned,"releasing B ends control ownership");
        input.poll(260,{},0,0,env);
        check(native.requests==1,"one gesture never dispatches twice");
    }
    for(unsigned failure=0;failure<8;++failure) {
        Native native;InputSession input;auto env=admission(native);
        if(failure==0)env.ground_grab={};
        if(failure==1)env.pose=SkatePose::air;
        if(failure==2)env.pose=SkatePose::manual;
        if(failure==3)env.external_owner=true;
        if(failure==4)env.action_mode=ActionExperiment::off;
        input.poll(100,failure==5?PhysicalInputSample{0x2080}:PhysicalInputSample{},0,0,env);
        PhysicalInputSample physical{static_cast<std::uint16_t>(failure==6?0x2000:0x2080)};
        if(failure==7)physical.rt=255;
        const auto out=input.poll(200,physical,0,0,env);
        check(!native.requests && !out.ground_grab_owned,"disabled, air, manual, conflicting owner, startup-held, plain B and revert inputs cannot start ground grabs");
    }
}
}
int main(){native_entry();session();std::cout<<"Native ground grab input tests passed\n";}
