#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
}
int main(){
    for(bool nose:{false,true})for(unsigned hold:{17u,100u,200u,250u}){
        ArcadeInput input(true,InputPolicy::production());input.enable(true);NativeInputState observer;
        NativeRequestSample native{};native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=nose?0x8000:0x4000;
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
        auto poll=[&](std::uint64_t now,std::uint16_t buttons,bool admitted=true){
            const auto frame=observer.observe(now,{1,2,3,4,5},native,{true,-1});
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            const bool balancing=native.state==100;
            input.publish_balance_controls(balancing?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},frame);
            return input.poll(0,now,buttons,0,25000,0,0,true,admitted,false,0,false,false,true,now/17,false,prepared,false,true,
                balancing?(nose?SkatePose::nose_manual:SkatePose::manual):SkatePose::riding,false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true,balancing);
        };
        poll(1000,0);const auto start=poll(1017,0x1000);
        check(start.authored_playback&&start.requested_id==1,"Manual/Nose context can use the existing admitted jump setup while its correction control is held");
        if(hold>150)poll(1150,0x1000);
        const auto released=poll(1017+hold,0);
        check(released.jump.released&&released.jump.setup_admitted&&released.jump.pressed_at==1017&&released.jump.released_at==1017+hold,
              "manual hop carries actual physical timestamps rather than balance/setup latency");
        NativeCharge charge;const auto token=charge.semantic_release(1017+hold,released.jump.released,released.jump.setup_admitted,
            released.jump.from_rail,released.jump.pressed_at,released.jump.released_at,true);
        check(token.sequence&&token.charge_ms==std::min(hold,200u),"manual hop uses the shared capped ordinary launch token");
        AirMotion air;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        air.step(intent,{1000,1,100,1.0f/60,5,0,0,true});
        if(hold>150){intent.sampled_at=1150;air.step(intent,{1150,1,103,1.0f/60,5,0,0,true});}
        intent={true,true,1017+hold,token.sequence,token.ms,token.charge_ms,0,1};
        air.step(intent,{1017+hold,1,103,1.0f/60,5,0,0,true});
        const auto launch=air.step(intent,{1034+hold,1,200,1.0f/60,5,30,0,true});
        check(launch.vertical&&std::abs(launch.velocity_y-GameplayProfile{}.jump_speed(hold))<.0001f,
              "actual takeoff compensates one selected jump impulse after Manual/Nose setup");
        native.state=200;check(!poll(1034+hold,0).jump.released,"airborne hop cannot replay a consumed physical release");
        input.enable(false);const auto off=poll(1051+hold,0x1000);
        check(off.buttons==0x1000&&!off.authored_playback&&!off.jump.released,"Arcade OFF yields manual-hop controls without semantic launch metadata");
    }
    std::cout<<checks<<" composed native manual hop checks passed\n";
}
