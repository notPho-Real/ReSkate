#include "Extension/ArcadeMode/arcade_camera.h"
#include "Extension/ArcadeMode/arcade_camera_input.h"
#include "Extension/ArcadeMode/arcade_controller_route.h"
#include <cstdlib>
#include <iostream>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
float offset(const std::array<float,16>& matrix){return std::remainder(std::atan2(matrix[12],matrix[14])-3.14159265358979323846f,6.2831853071795864769f);}
void lookaround(){
    const std::array<float,16> follow{1,0,0,0,0,1,0,0,0,0,1,0,0,1,-4,1};
    const std::array<float,3> target{0,1,0};
    for(unsigned hz:{30u,60u,120u}){
        CameraOrbit orbit;std::array<float,16> result{};
        for(unsigned i=0;i<hz*4;++i){
            const auto next=orbit.update(follow,target,1,0,1.0f/static_cast<float>(hz));
            check(next.has_value(),"bounded hold produces a finite camera");result=*next;
            check(std::abs(offset(result))<=3.00001f,"held lookaround cannot orbit beyond its angle target");
        }
        check(std::abs(offset(result)+3)<.0001f,"full hold approaches the same bounded angle at30/60/120Hz");
        const auto held=offset(result);
        for(unsigned i=0;i<hz;++i)result=*orbit.update(follow,target,0,0,1.0f/static_cast<float>(hz));
        const float expected=held*std::pow(15.0f/16.0f,60.0f);
        check(std::abs(offset(result)-expected)<.0001f,"neutral returns with the control baseline response independently of callback cadence");
        std::cout<<"camera hz="<<hz<<" held_rad="<<held<<" neutral_after1s_rad="<<offset(result)<<'\n';
        orbit.reset();check(std::abs(offset(*orbit.update(follow,target,0,0,0)))<.00001f,"reset restores the follow angle without an inherited orbit");
        check(std::abs(offset(*orbit.update(follow,target,.2f,0,.017f)))<.00001f,"existing physical dead zone remains neutral");
    }
    CameraOrbit invalid;check(!invalid.update(follow,target,NAN,0,.017f),"nonfinite camera input stays rejected");
}
void routing(){
    ArcadeInput input{true};input.enable(true);NativeInputState producer;NativeControllerRoute route;
    NativeRequestSample sample{};sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[0]=0x20;sample.words[1]=0xc000;
    auto frame=producer.observe(1000,{1,2,3,4,5},sample,{true,-1,16});
    input.publish_native_context(frame);input.publish_authority(1000,frame.owner_generation,1,true);
    check(route.arm(1,1000,1000),"explicit source policy can be requested without claiming native selection");
    const ArcadeInput::Physical neutral{true,1000,0,0,0,0,0,0,0};
    route.observe_unfiltered(1,neutral);route.observe(1,neutral);route.select(1000,3,true,frame);
    route.observe(0,{true,1000,0,0,0,32767,-32767,0,0});
    route.observe(1,{true,1000,0,0,0,1234,-2345,0,0});
    const auto selected=camera_input_snapshot(route,input,1000,true);
    check(selected.valid&&selected.slot==1&&selected.right_x==1234&&selected.right_y==-2345,"camera uses only selected physical axes with another connected active controller");
    check(!camera_input_snapshot(route,input,1000,false).valid,"OFF cannot acquire camera input");
    check(!camera_input_snapshot(route,input,999,true).valid,"future input cannot be read as fresh camera authority");
    check(!camera_input_snapshot(route,input,1151,true).valid,"camera sampling cannot renew stale native or source evidence");
    input.invalidate_authority();check(!camera_input_snapshot(route,input,1000,true).valid,"focus/source authority loss withdraws camera input");
    input.publish_authority(1000,frame.owner_generation+1,1,true);
    check(!camera_input_snapshot(route,input,1000,true).valid,"native owner generation must match controller authority");
    input.publish_authority(1000,frame.owner_generation,1,true);
    route.disconnected(1);check(!camera_input_snapshot(route,input,1000,true).valid,"selected source disappearance withdraws camera input");
    route.reset();route.observe(0,neutral);route.select(1000,1,true);
    const auto reassigned=camera_input_snapshot(route,input,1000,true);
    check(reassigned.valid&&!selected.same_binding(reassigned),"route reassignment cannot inherit an old camera orbit");
    ++frame.owner.core;++frame.owner_generation;input.publish_native_context(frame);input.publish_authority(1000,frame.owner_generation,1,true);
    check(!reassigned.same_binding(camera_input_snapshot(route,input,1000,true)),"native owner replacement cannot retain an old camera epoch");
}
}
int main(){lookaround();routing();std::cout<<checks<<" camera input and lookaround checks passed\n";}
