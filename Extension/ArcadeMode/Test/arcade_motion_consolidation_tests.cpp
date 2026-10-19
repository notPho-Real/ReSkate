#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_movement.h"
#include <cstdlib>
#include <iostream>
#include <span>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
template<class Motion> float target(float root,const AirMotionOutput& out,float dt){
    if constexpr(requires{Motion::next_height(root,out,dt);}) return Motion::next_height(root,out,dt);
    return root+out.velocity_y*dt;
}
template<class Drive> bool deliver(std::span<std::array<float,3>> velocities,float dx,float dz){
    if constexpr(requires{Drive::apply_bodies(velocities,dx,dz);}) return Drive::apply_bodies(velocities,dx,dz);
    for(auto& v:velocities){v[0]+=dx;v[2]+=dz;}return true;
}
}
int main(int argc,char**argv){
    if(argc==1 || argv[1][0]=='t')for(float dt:{1.0f/30,1.0f/60})for(unsigned charge:{0u,17u,34u,50u,100u,150u,200u,500u}){
        const float launch=GameplayProfile{}.jump_speed(charge);
        AirMotionOutput out{true,true,false,launch,0,0};
        const float expected=5+launch*dt-.5f*34.29f*dt*dt;
        check(near(target<AirMotion>(5,out,dt),expected),"ordinary native motion target includes the source half-acceleration displacement");
    }
    if(argc==1 || argv[1][0]=='m'){
        std::array<std::array<float,3>,3> v{{{10,1,0},{11,2,1},{999.8f,3,2}}};const auto original=v;
        check(!deliver<GroundDrive>(v,.5f,.25f)&&v==original,"invalid final ground body rejects the entire acceleration batch before any momentum changes");
    }
    if(argc==1) {
        for(unsigned rate:{30u,60u})for(unsigned charge:{0u,17u,34u,50u,100u,150u,200u,500u}) {
            const float dt=1.0f/static_cast<float>(rate),launch=GameplayProfile{}.jump_speed(charge);
            AirMotion motion;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
            AirMotionFrame f{1000,1,100,dt,0,0,0,true};motion.step(intent,f);
            intent.release_sequence=1;intent.release_at=f.now=intent.sampled_at=1017;intent.charge_ms=std::min(charge,200u);motion.step(intent,f);
            float y=0,peak=0;
            for(unsigned i=0;i<rate;++i) {
                f.now=intent.sampled_at=1034+i*(rate==30?34u:17u);f.state=200;f.root_y=y;
                const auto out=motion.step(intent,f);y=target<AirMotion>(y,out,dt);f.velocity_y=out.velocity_y;
                const float t=static_cast<float>(i+1)*dt;
                check(near(y,launch*t-.5f*34.29f*t*t),"composed charge/controller/native target path matches analytic ordinary displacement at both simulation cadences");
                peak=std::max(peak,y);
            }
            const float ideal=launch*launch/(2*34.29f);
            check(std::abs(peak-ideal)<=34.29f*dt*dt/8+.0001f,"sampled apex differs from analytic10/10 charge peak only by discrete sample phase");
        }
        for(unsigned count:{1u,3u,32u})for(float x:{-2.0f,0.0f,2.0f})for(float z:{-3.0f,0.0f,3.0f}) {
            std::array<std::array<float,3>,32> v{};
            for(unsigned i=0;i<count;++i)v[i]={10+static_cast<float>(i),static_cast<float>(i)-4,20-static_cast<float>(i)};
            const auto original=v;
            check(deliver<GroundDrive>({v.data(),count},x,z),"bounded ground acceleration reaches every validated articulated body");
            for(unsigned i=0;i<count;++i)check(near(v[i][0]-original[i][0],x)&&v[i][1]==original[i][1]&&near(v[i][2]-original[i][2],z),
                "uniform signed ground correction preserves each vertical component and relative horizontal momentum");
        }
        for(unsigned invalid=0;invalid<5;++invalid) {
            std::array<std::array<float,3>,3> v{{{10,1,2},{11,2,3},{12,3,4}}};
            float x=1,z=2;
            if(invalid==0)x=std::numeric_limits<float>::quiet_NaN();
            if(invalid==1)z=std::numeric_limits<float>::infinity();
            if(invalid==2)v[2][2]=999;
            if(invalid==3)v[2][1]=1000;
            if(invalid==4)v[2][0]=-1000;
            const auto original=v;
            check(!deliver<GroundDrive>(v,x,z)&&v==original,"invalid delta or late body leaves the complete ground batch untouched");
        }
        check(!deliver<GroundDrive>({},1,1),"an empty native body set cannot receive ground acceleration");
    }
    std::cout<<checks<<" movement consolidation checks passed\n";
}
