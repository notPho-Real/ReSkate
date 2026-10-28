#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
template<class Frame> void horizontal(Frame& frame,float x,float z){
    if constexpr(requires{frame.velocity_x;frame.velocity_z;}){frame.velocity_x=x;frame.velocity_z=z;}
}
float limited(float speed,float dt){
    speed=std::min(speed,1100*.0254f);
    if(speed>900*.0254f) speed=std::max(0.0f,speed-.0001f/.0254f*60*dt*speed*speed);
    return speed;
}
}
int main(){
    for(float dt:{1.0f/30,1.0f/60})for(float speed:{0.0f,10.0f,900*.0254f,24.0f,40.0f})for(float sign:{-1.0f,1.0f}){
        AirMotion motion;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        AirMotionFrame frame{1000,1,100,dt,5,0,0,true};horizontal(frame,0,10);
        motion.step(intent,frame);
        intent.sampled_at=frame.now=1017;frame.state=200;horizontal(frame,.6f*speed*sign,.8f*speed);
        const auto out=motion.step(intent,frame);
        std::array<std::array<float,3>,3> velocities{{{.6f*speed*sign,0,.8f*speed},{.6f*speed*sign+1,2,.8f*speed+3},{.6f*speed*sign-2,-1,.8f*speed-1}}},spins{};
        const auto original=velocities;
        check(AirMotion::apply_bodies(out,velocities,spins),"ordinary takeoff reaches existing physical body adapter");
        check(near(std::hypot(velocities[0][0],velocities[0][2]),limited(speed,dt)),"ordinary AIR applies selected10/10 hard clamp then heavy quadratic loss");
        check(near(velocities[1][0]-velocities[0][0],1)&&near(velocities[1][2]-velocities[0][2],3)&&
            near(velocities[2][0]-velocities[0][0],-2)&&near(velocities[2][2]-velocities[0][2],-1),"speed limiting preserves articulated relative horizontal momentum");
        check(velocities[0][1]==original[0][1]&&velocities[1][1]==original[1][1]&&velocities[2][1]==original[2][1],"horizontal limits preserve vertical and relative vertical motion");
    }
    for(float dt:{1.0f/30,1.0f/60}) {
        for(float initial:{10.0f,std::nextafter(900*.0254f,0.0f),900*.0254f,
                std::nextafter(900*.0254f,100.0f),1100*.0254f,40.0f}) {
            GroundResistance limits;limits.observe_ordinary(10);
            const float target=limited(initial,dt);
            check(near(initial+limits.step_air(dt,initial),target),"strict soft boundary and hard-before-heavy order match independent selected10/10 oracle");
            for(float fraction:{0.0f,.5f,1.0f,2.0f}) {
                GroundResistance credited;credited.observe_ordinary(initial);
                // Explicit prior ordinary ownership is needed for fast arrivals.
                credited.observe_ordinary(10);credited.observe_ordinary(initial);
                const float native=std::max(0.0f,initial-(initial-target)*fraction);
                check(near(native+credited.step_air(dt,native),std::min(native,target)),"native horizontal resistance is credited once and stronger losses are never restored");
            }
        }
        AirMotion continuous;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        AirMotionFrame frame{1000,1,100,dt,5,1,0,true};horizontal(frame,6,8);continuous.step(intent,frame);
        frame.state=200;
        for(unsigned i=0;i<200;++i) {
            intent.sampled_at=frame.now=1017+i*17;
            const auto out=continuous.step(intent,frame);
            std::array<std::array<float,3>,1> velocity{{{6,frame.velocity_y,8}}},spin{};
            check(AirMotion::apply_bodies(out,velocity,spin)&&velocity[0][0]==6&&velocity[0][2]==8,
                "ordinary air below soft limit adds no wind/rolling drag across a sustained flight");
            frame.velocity_y=out.velocity_y;
        }
        for(unsigned charge:{0u,17u,34u,50u,100u,150u,200u,500u}) {
            AirMotion jump;AirMotionIntent held{true,true,1000,0,0,0,0,1};
            AirMotionFrame launch{1000,1,100,dt,5,0,0,true};horizontal(launch,0,10);jump.step(held,launch);
            NativeCharge physical;const auto token=physical.semantic_release(1017+charge,true,true,false,1017,1017+charge,true);
            // Keep the admitted native observation cadence during long holds.
            for(std::uint64_t at=1100;at<1017+charge;at+=100) {
                held.sampled_at=launch.now=at;jump.step(held,launch);
            }
            held={true,true,1017+charge,token.sequence,token.ms,token.charge_ms,0,1};launch.now=1017+charge;
            jump.step(held,launch);launch.state=200;launch.now=held.sampled_at=1034+charge;horizontal(launch,0,40);
            const auto out=jump.step(held,launch);
            check(near(out.velocity_y,GameplayProfile{}.launch_speed(charge,LaunchDescriptor::ordinary())),"horizontal limits preserve the source-selected physical charge launch target");
        }
    }
    for(unsigned fault=0;fault<9;++fault) {
        AirMotion motion;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        AirMotionFrame frame{1000,1,100,1.0f/60,5,0,0,true};horizontal(frame,0,10);motion.step(intent,frame);
        frame.now=intent.sampled_at=1017;frame.state=200;horizontal(frame,0,40);
        const auto launched=motion.step(intent,frame);frame.velocity_y=launched.velocity_y;
        check(launched.horizontal,"qualified ordinary flight initially owns heavy limit correction");
        frame.now=intent.sampled_at=1034;
        if(fault==0) intent.enabled=false;
        if(fault==1) frame.owner=2;
        if(fault==2) intent.sampled_at=800;
        if(fault==3) frame.state=403;
        if(fault==4) frame.dt=0;
        if(fault==5) frame.velocity_y+=4;
        if(fault==6) frame.now=intent.sampled_at=1000;
        if(fault==8) frame.ordinary_ground=false;
        if(fault==7) frame.velocity_x=std::numeric_limits<float>::quiet_NaN();
        check(!motion.step(intent,frame).horizontal,"OFF/owner/stale/rail/cadence/native trajectory takeover/rollback/invalid horizontal data/upright loss relinquish limits");
        if(fault==8) {
            frame.ordinary_ground=true;frame.now=intent.sampled_at=1051;
            check(!motion.step(intent,frame).horizontal,"upright reacquisition in the same flight cannot recreate missing transfer authority");
        }
    }
    {
        AirMotion unknown;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        AirMotionFrame frame{1000,1,100,1.0f/60,5,0,0,true};horizontal(frame,0,40);unknown.step(intent,frame);
        frame.now=intent.sampled_at=1017;frame.state=200;
        check(!unknown.step(intent,frame).horizontal,"unknown fast arrival retains native transfer-override ownership");
        frame.now=intent.sampled_at=1034;frame.state=100;frame.ordinary_ground=false;horizontal(frame,0,10);unknown.step(intent,frame);
        frame.now=intent.sampled_at=1051;frame.state=200;horizontal(frame,0,40);
        check(!unknown.step(intent,frame).horizontal,"unqualified ground cannot arm ordinary air speed limits");
    }
    {
        AirMotionOutput out{true,false,false,0,0,0};out.horizontal=true;out.delta_x=-2;out.delta_z=-3;
        std::array<std::array<float,3>,2> velocity{{{10,4,10},{11,5,12}}},spin{};const auto original=velocity;
        check(AirMotion::apply_bodies(out,velocity,spin)&&velocity[0][1]==4&&velocity[1][1]==5,
            "horizontal-only body delivery does not acquire vertical or angular channels");
        out.delta_z=std::numeric_limits<float>::quiet_NaN();velocity=original;
        check(!AirMotion::apply_bodies(out,velocity,spin)&&velocity==original,"nonfinite horizontal target rejects the entire body batch atomically");
    }
    std::cout<<checks<<" ordinary air speed limit checks passed\n";
}
