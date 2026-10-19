#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace{unsigned checks{};void check(bool c,const char* why){++checks;if(!c){std::cerr<<why<<'\n';std::exit(1);}}}
int main(){
 for(unsigned cadence:{17u,34u})for(unsigned ordering=0;ordering<3;++ordering){
  NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample raw{};raw.valid=true;raw.state=100;raw.crouch_mode=1;
  auto frame=producer.observe(1000,owner,raw,{true,-1});AirRotation input;
  input.poll_qualified(1000,0,false,false,true,SkatePose::riding,false,false,SpinMode::tap,{},&frame);
  if(ordering==1)input.poll_qualified(1001,BoardChord::lb,false,false,true,SkatePose::riding,false,false,SpinMode::tap,{},&frame);
  raw.state=200;frame=producer.observe(1000+cadence,owner,raw,{true,-1});const auto entered=frame.airborne_started_at;
  check(frame.airborne_started_known&&entered==1000+cadence,"native AIR entry timestamp comes from the actual physics observer");
  if(ordering!=2)frame=producer.observe(1000+2*cadence,owner,raw,{true,7});
  const auto now=ordering==2?1000+cadence:1000+2*cadence;
  check(frame.airborne_started_at==entered,"new AIR steps and selection changes cannot renew native entry time");
  const auto out=input.poll_qualified(now,BoardChord::lb,false,false,true,SkatePose::air,true,false,SpinMode::tap,{},&frame);
  check(out.spin.serial==(ordering==0?1u:0u),"first observed AIR input admits a genuinely later edge while preheld/coincident negatives remain denied");
 }
 for(float dt:{.017f,.034f}){
  NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample raw{};raw.valid=true;raw.state=100;raw.crouch_mode=1;
  AirMotion motion;AirMotionIntent intent{};intent.enabled=intent.angular_allowed=true;intent.spin.mode=SpinMode::tap;unsigned now=1000;
  const auto step=[&](){auto frame=producer.observe(now,owner,raw,{true,-1});intent.sampled_at=now;intent.angular_proof=NativeAirSpinProof::from_frame(frame);auto out=motion.step(intent,{now,3,raw.state,dt,1,0,0,true,0,0,true,intent.angular_proof});now+=static_cast<unsigned>(std::lround(dt*1000));return out;};
  step();raw.state=200;step();++intent.spin.serial;++intent.spin.balance;auto out=step();unsigned active=1;const float yaw=out.yaw;
  intent.spin.facts.no_spin=SpinFact::set;for(unsigned i=0;i<3;++i){out=step();check(!out.angular&&out.yaw==yaw,"composed AirMotion preserves pending angle while known NoSpin pauses the same flight");}
  intent.spin.facts.no_spin=SpinFact::clear;
  do{out=step();if(out.spin_y!=0)++active;check(active<40,"resumed tap terminates");}while(out.spin_y!=0);
  check(active==static_cast<unsigned>(std::ceil(TapSpin::pi/(7.75*dt))),"same-flight resume retains the original full-final-step count");
  raw.state=100;step();raw.state=200;out=step();check(out.spin_y==0,"landing withdraws paused or completed tap work");
 }
 std::cout<<checks<<" spin review regression checks passed\n";
}
