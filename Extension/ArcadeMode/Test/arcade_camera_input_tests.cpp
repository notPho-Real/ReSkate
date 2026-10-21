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
using Vec=std::array<float,3>;
using Pose=std::array<float,16>;
float dot(Vec a,Vec b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
Vec cross(Vec a,Vec b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
Vec row(const Pose& p,unsigned i){return {p[i*4],p[i*4+1],p[i*4+2]};}
Vec subtract(Vec a,Vec b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
Vec yaw(Vec v,float angle){return {std::cos(angle)*v[0]+std::sin(angle)*v[2],v[1],-std::sin(angle)*v[0]+std::cos(angle)*v[2]};}
Pose scene_yaw(Pose p,float angle){for(unsigned i=0;i<4;++i){const auto v=yaw(row(p,i),angle);for(unsigned j=0;j<3;++j)p[i*4+j]=v[j];}return p;}
void near_pose(const Pose& actual,const Pose& expected,float tolerance,const char* why){for(unsigned i=0;i<16;++i)check(std::abs(actual[i]-expected[i])<=tolerance,why);}
Pose framed(){
    // Optical direction deliberately differs from the eye-to-pivot direction.
    const float p=.45f,r=.31f;
    Pose frame{std::cos(r),std::cos(p)*std::sin(r),std::sin(p)*std::sin(r),0,
        -std::sin(r),std::cos(p)*std::cos(r),std::sin(p)*std::cos(r),0,
        0,-std::sin(p),std::cos(p),0,0,1.5f,-3.4f,1};
    frame=scene_yaw(frame,.27f);
    return frame;
}
void rigid_frame(){
    const auto follow=framed();const Vec pivot{0,1,0};
    CameraOrbit neutral;
    for(float dt:{0.f,1.f/30,1.f/60,1.f/120}){
        const auto view=neutral.update(follow,pivot,0,0,dt);
        check(view.has_value(),"valid neutral frame accepted");
        near_pose(*view,follow,0,"neutral must preserve every supplied frame component including aim and roll");
    }
    for(float x:{-1.f,0.f,1.f})for(float y:{-1.f,0.f,1.f}){
        CameraOrbit orbit;const auto view=orbit.update(follow,pivot,x,y,1.f/60);
        check(view.has_value(),"look input preserves a valid frame");
        const auto original_eye=subtract(row(follow,3),pivot),eye=subtract(row(*view,3),pivot);
        check(std::abs(dot(original_eye,original_eye)-dot(eye,eye))<.00002f,"rigid look preserves eye-to-pivot radius");
        for(unsigned i=0;i<3;++i){
            check(std::abs(dot(row(*view,i),row(*view,i))-1)<.00001f,"camera basis stays unit length");
            check(std::abs(dot(row(*view,i),eye)-dot(row(follow,i),original_eye))<.00002f,"basis and eye receive the same rotation without changing optical aim offset");
            for(unsigned j=i+1;j<3;++j)check(std::abs(dot(row(*view,i),row(*view,j)))<.00001f,"camera axes stay orthogonal");
        }
        check(std::abs(dot(cross(row(*view,0),row(*view,1)),row(*view,2))-1)<.00001f,"camera basis keeps its orientation");
        CameraOrbit translated;auto shifted=follow;const Vec shift{1000,-200,300};Vec shifted_pivot=pivot;
        for(unsigned i=0;i<3;++i){shifted[12+i]+=shift[i];shifted_pivot[i]+=shift[i];}
        auto expected=*view;for(unsigned i=0;i<3;++i)expected[12+i]+=shift[i];
        near_pose(*translated.update(shifted,shifted_pivot,x,y,1.f/60),expected,.0002f,"scene translation moves only the camera eye");
        CameraOrbit rotated;const float angle=.8f;
        near_pose(*rotated.update(scene_yaw(follow,angle),yaw(pivot,angle),x,y,1.f/60),scene_yaw(*view,angle),.00001f,"scene rotation about world up commutes with look rotation");
    }
    CameraOrbit left,right;
    const auto l=left.update(follow,pivot,-1,0,1.f/60),r=right.update(follow,pivot,1,0,1.f/60);
    near_pose(scene_yaw(*l,-.375f),*r,.00001f,"opposite horizontal samples rotate the same frame equally in opposite directions");
    CameraOrbit zero;
    near_pose(*zero.update(follow,pivot,1,-1,0),follow,0,"zero timestep cannot alter geometry before any look offset exists");
    for(float height:{13.f,-4.f}){
        auto steep=follow;steep[13]=height;
        CameraOrbit fresh;
        near_pose(*fresh.update(steep,pivot,0,0,0),steep,0,"neutral does not clamp the incoming camera geometry");
        const auto turned=fresh.update(steep,pivot,1,0,1.f/60);
        check(turned&&std::abs((*turned)[13]-height)<.00001f,"horizontal look cannot snap an incoming elevation outside the orbit limits");
    }
    CameraOrbit invalid;
    check(!invalid.update(follow,pivot,0,0,-1),"negative camera interval remains rejected");
    check(!invalid.update(follow,{NAN,1,0},0,0,.017f),"nonfinite pivot remains rejected");
    auto bad=follow;bad[0]=NAN;
    check(!invalid.update(bad,pivot,0,0,.017f),"nonfinite frame remains rejected");
    auto too_far=follow;too_far[12]=20;
    check(!invalid.update(too_far,pivot,0,0,.017f),"unqualified orbit radius remains rejected");
}
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
int main(){rigid_frame();lookaround();routing();std::cout<<checks<<" camera input and lookaround checks passed\n";}
