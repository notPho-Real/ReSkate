#include "Extension/ArcadeMode/arcade_surface_motion.h"
#include "Extension/ArcadeMode/arcade_surface_motion_math.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#ifdef DINGOSDK_SURFACE_MOTION_REFERENCE
#include "contact_motion.h"
#endif
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) { ++checks; if(!value){std::cerr<<message<<'\n';std::exit(1);} }
bool close(SurfaceGeometryVector a,SurfaceGeometryVector b,float tolerance=.0001f) {
    for(std::size_t i=0;i<3;++i)if(std::abs(a[i]-b[i])>tolerance)return false; return true;
}
void transforms() {
    using namespace surface_motion_math;
    const float q=std::sqrt(.5f);
    const std::array<float,12> raw{2,3,4,std::numeric_limits<float>::quiet_NaN(),0,q,0,q,10,20,30,std::numeric_limits<float>::quiet_NaN()};
    Transform t;
    check(transform(raw,t),"numeric pose lanes only; padding is not geometry");
    const V point{1,2,3};
    check(close(local_point(world_point(point,t),t),point),"nonuniform-scale local/world point round trip");
    const auto normal=unit(V{1,2,3});
    const auto world=world_normal(normal,t);
    check(close(local_normal(world,t),normal),"inverse-transpose normal round trip");
    const auto tangent=sub(world_point(V{-2,1,0},t),t.position);
    check(std::abs(dot(world,tangent))<.0001f,"world normal perpendicular to scaled tangent");
    for(unsigned i=0;i<3;++i){auto bad=raw;bad[i]=0;check(!transform(bad,t),"zero scale unavailable");}
    auto bad=raw;bad[8]=std::numeric_limits<float>::infinity();check(!transform(bad,t),"nonfinite position unavailable");
}
void rotation() {
    using namespace surface_motion_math;
    for(const double step:{.017,.034}) {
        SurfacePointMotion motion;
        constexpr double speed=.2;
        for(unsigned i=0;i<60;++i) {
            const double time=1+step*i,angle=speed*(time-1);
            const auto point=add(V{10,0,10},rotate(V{2,0,0},Q{0,static_cast<float>(std::sin(angle/2)),0,static_cast<float>(std::cos(angle/2))}));
            const auto sample=motion.sample(time,point);
            if(!i){check(!sample.valid,"cold rotation needs a pair");continue;}
            check(sample.valid && std::abs(std::hypot(sample.velocity[0],sample.velocity[2])-.4)<.002,"stationary origin still has tangential point velocity");
            check(sample.sampled_at==time && close(sample.point,point),"rotation retains observed point/time");
        }
    }
    SurfacePointMotion motion;
    double time=2;
    for(unsigned i=0;i<100;++i) {
        time+=i%3==0?.017:i%3==1?.034:.006;
        const auto sample=motion.sample(time,{static_cast<float>(time),0,0});
        if(i) check(sample.valid && std::abs(sample.velocity[0]-1)<.0001f,"uneven observations do not create speed bursts");
    }
}
void clocks() {
    SurfacePointMotion motion;
    check(!motion.sample(1,{0,0,0}).valid,"cold");
    const auto a=motion.sample(1.017,{.017f,0,0});check(a.valid,"17ms motion");
    const auto b=motion.sample(1.018,{.018f,0,0});check(b.sampled_at==a.sampled_at && b.point==a.point && b.velocity==a.velocity,"sub240Hz sample retains immutable result");
    const auto c=motion.sample(1.034,{.017f,0,0});check(c.sampled_at==a.sampled_at && c.point==a.point,"unchanged short observation retains original time");
    check(!motion.sample(1.034,{.017f,0,0}).valid,"same-time observation resets as source specifies");
    check(!motion.sample(1.02,{.017f,0,0}).valid,"reversed time resets");
    check(!motion.sample(1.171,{.017f,0,0}).valid,"over150ms gap resets");
    check(motion.sample(1.188,{.034f,0,0}).valid,"fresh pair after gap");
    motion.reset();(void)motion.sample(2,{0,0,0});
    auto stop=motion.sample(2.051,{0,0,0});check(stop.valid && stop.velocity==SurfaceGeometryVector{},"50ms stopped evidence establishes zero");
    for(unsigned i=2;i<=8;++i)check(motion.sample(2+.05*i,{0,0,0}).valid,"observed stop retains identity");
    auto restart=motion.sample(2.42,{.02f,0,0});check(restart.valid && restart.velocity==SurfaceGeometryVector{},"long rest restart does not average parked interval");
    check(motion.sample(2.437,{.037f,0,0}).valid,"next fresh moving pair resumes");
    motion.reset();(void)motion.sample(3,{0,0,0});check(!motion.sample(3.017,{1,0,0}).valid,"over45m/s outlier rejected");
    check(!motion.sample(std::numeric_limits<double>::quiet_NaN(),{0,0,0}).valid,"nonfinite clock resets");
}
#ifdef DINGOSDK_SURFACE_MOTION_REFERENCE
void identical_source_traces() {
    car_grab::ContactMotion reference;
    SurfacePointMotion replacement;
    double time=1;float x=0;std::uint32_t seed=741;
    for(unsigned i=0;i<10000;++i) {
        seed=seed*1664525u+1013904223u;
        const double dt=i%101==0?-.01:i%83==0?.151:i%17==0?0:i%5==0?.001:i%3==0?.034:.017;
        time+=dt;if(seed%7)x+=static_cast<float>(seed%30)*.001f;
        const auto a=reference.sample(time,{x,0,0});const auto b=replacement.sample(time,{x,0,0});
        check(a.valid==b.valid && a.sampled_at==b.sampled_at && close({a.point.x,a.point.y,a.point.z},b.point,0) &&
            close({a.velocity.x,a.velocity.y,a.velocity.z},b.velocity,0),"identical trace must match actual published source exactly");
    }
}
#endif
}
int main(){transforms();rotation();clocks();
#ifdef DINGOSDK_SURFACE_MOTION_REFERENCE
identical_source_traces();
#endif
std::cout<<checks<<" surface point-motion checks passed\n";}
