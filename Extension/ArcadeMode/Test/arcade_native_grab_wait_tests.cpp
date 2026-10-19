#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
}
int main(){
    // This is the actual queue -> authored owner -> auxiliary adapter path.
    // Fixture pad stages are not native asset/init/ForceInit timing evidence.
    for(unsigned frame:{17u,34u})for(bool released:{false,true})for(unsigned boundary=0;boundary<3;++boundary){
        InputSession session;InputAdmission admission{};
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
        admission.gameplay=admission.authored=admission.manual_safe=admission.may_start=admission.action_fresh=admission.inspected_stance=true;
        admission.action_mode=ActionExperiment::classic;admission.pose=SkatePose::riding;admission.catalog=prepared;
        auto poll=[&](unsigned step,std::uint16_t buttons=0){admission.tick=step*frame/17;return session.poll(step*frame,{buttons},0,0,admission);};
        poll(0);check(poll(1,0x1000).authored_playback,"real authored setup supplies the preceding input owner");poll(2);
        admission.pose=SkatePose::air;const auto waiting=poll(3,0x2000);
        check(!waiting.action.owned&&session.pending_grab().action==InputAction::grab,"held grab queues while real authored playback owns the channels");
        const auto recognized=session.pending_grab().recognized_at;
        NativeGesture::Output ready{};
        for(unsigned step=4;step<=12;++step){ready=poll(step,released?0:0x2000);if(ready.action.owned)break;}
        if(released){
            check(!ready.action.owned&&ready.action.result==DispatchResult::unavailable,"release-before-ready stays unavailable without a fabricated500ms init/out recipe");
            check(session.pending_grab().recognized_at==recognized,"waiting for unavailable native initialization never renews recognition");
        }else{
            check(ready.action.owned&&ready.action.pad.right_trigger==255,"held request hands off only after delivered authored neutral");
            ready=poll(13);check(ready.action.owned&&ready.action.pad.right_trigger==0,"delivered held primitive releases through its existing neutral pad");
        }
        if(boundary==0)admission.gameplay=false;
        if(boundary==1)admission.pose=SkatePose::riding;
        if(boundary==2)session.reset();
        const auto cancelled=poll(14);
        check(!cancelled.action.owned&&session.pending_grab().action==InputAction::none,"OFF/landing/reset restores channels and cancels a waiting or released grab");
        admission.gameplay=true;admission.pose=SkatePose::air;
        check(!poll(15).action.owned,"old request cannot restart in a later airborne context");
    }
    std::cout<<checks<<" composed native grab waiting checks passed\n";
}
