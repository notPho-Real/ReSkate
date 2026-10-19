#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Fixture {
    ArcadeInput input{true,InputPolicy::production()};NativeInputState observer;NativeRequestSample native{};
    NativeFlipSelection flip{true,-1,0x200000};SkatePose caller{SkatePose::air};bool prepare{};
    const std::array<PreparedGesture,2> prepared{PreparedGesture{1,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
        PreparedGesture{2,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,-25000,28000},{21,0,0},{24,0,0}}}};
    Fixture(){input.enable(true);native.valid=true;native.state=200;native.crouch_mode=1;}
    NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons=0,short y=0){
        const auto frame=observer.observe(now,{1,2,3,4,5},native,flip);
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        return input.poll(0,now,buttons,-25000,y,321,-456,true,true,true,10,false,false,true,now/17,false,prepared,
            false,true,caller,false,prepare,false,false,true,false,0,0,ActionExperiment::classic,true,true,false);
    }
};
}
int main(){
    for(unsigned step:{17u,34u}){
        Fixture f;f.flip.id=2;f.poll(1000);check(!f.poll(1000+step,0x4000).authored_playback,"air flip waits while the real native selection is occupied");
        f.native.state=100;f.native.words[1]=0xc000;f.flip.id=-1;
        check(!f.poll(1000+2*step).authored_playback,"native landing retires the old queued airborne request despite a stale air caller pose");
        check(!f.poll(1000+3*step,0x4000).authored_playback,"fresh X on native ground cannot use a stale caller air pose to execute an air flip");
        f.native.state=200;f.native.words[1]=0;
        check(!f.poll(1000+4*step,0x4000).authored_playback,"landing then relaunch cannot replay the held ground press or old waiting request");
        f.poll(1000+5*step);
        check(f.poll(1000+6*step,0x4000).requested_id==2,"a genuinely fresh press in independently observed air still uses the established native mapping");
    }
    Fixture air;air.caller=SkatePose::riding;air.poll(2000);
    check(!air.poll(2017,0x1000).authored_playback,"actual air rejects stale grounded setup permission");
    Fixture recovery;recovery.caller=SkatePose::riding;recovery.native.state=101;recovery.prepare=true;
    recovery.poll(3000);recovery.poll(3017,0,25000);
    check(!recovery.poll(3034,0,-25000).manual.owns,"verified native revert preparation recognizes without executing");
    recovery.native.state=100;recovery.native.words[1]=0xc000;recovery.prepare=false;
    check(recovery.poll(3051,0,-25000).manual.owns,"source-supported preparation still dispatches the recognized pair after actual ground readiness");
    Fixture unknown;unknown.native.state=999;const auto unavailable=unknown.poll(4000,0x4000);
    check(unavailable.buttons==0x4000&&unavailable.right_x==321&&!unavailable.authored_playback,"unknown native state restores physical channels rather than guessing air/ground eligibility");
    std::cout<<checks<<" native pose reconciliation checks passed\n";
}
