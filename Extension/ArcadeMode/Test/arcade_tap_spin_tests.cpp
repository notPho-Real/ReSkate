#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool x,const char* why){++checks;if(!x){std::cerr<<why<<'\n';std::exit(1);}}
bool near(double a,double b){return std::abs(a-b)<.00001;}
AirRotation::Output poll(AirRotation& input,unsigned now,std::uint16_t buttons=0,bool left=false,bool right=false,SpinMode mode=SpinMode::tap,SkatePose pose=SkatePose::air,bool ready=true,bool conflict=false,SpinFacts facts={}){
 return input.poll_qualified(now,buttons,left,right,true,pose,ready,conflict,mode,facts);
}
}
int main(){
 for(float dt:{.017f,.034f})for(int taps=1;taps<=4;++taps)for(int side:{-1,1}){
  SpinCommand command{SpinMode::tap,4,0,0,{}};TapSpin pending;pending.baseline(command);command.serial=static_cast<unsigned>(taps);command.balance=side*taps;
  unsigned steps=0;double angle=0;float rate=0;
  do{rate=pending.step(command,0,dt,7.75f);angle+=rate*dt;++steps;check(steps<200,"bounded tap accumulator terminates");}while(pending.remaining()!=0);
  const auto expected_steps=static_cast<unsigned>(std::ceil(taps*TapSpin::pi/(7.75*dt)));
  check(steps==expected_steps&&near(std::abs(angle),steps*7.75*dt),"one through four taps retain the full final physics step instead of clamping to ideal pi");
  check(std::abs(angle)>=taps*TapSpin::pi&&std::abs(angle)<taps*TapSpin::pi+7.75*dt,"tap overshoot stays below one actual physics increment");
  check((rate<0)==(side>0)&&pending.tap_step(),"source left-positive pending angle uses existing native handedness through the final step");
  check(pending.step(command,0,dt,7.75f)==0&&!pending.tap_step(),"unchanged counters cannot replay a completed tap");
 }
 check(near(TapSpin::pi/7.75,.4053667940115862),"ideal single-pi duration is independent of full-final-step delivery");
 {
  TapSpin pending;SpinCommand command{SpinMode::tap,1};pending.baseline(command);
  command.serial=2;command.balance=0;check(pending.step(command,0,.017f,7.75f)==0,"opposite taps cancel before a physics sample");
  command.serial=3;command.balance=1;check(pending.step(command,0,.017f,7.75f)<0,"left tap begins positive semantic pending");
  command.serial=4;command.balance=0;check(pending.step(command,0,.017f,7.75f)>0,"opposite tap subtracts pi from the remaining accumulator after a partial step");
  ++command.epoch;check(pending.step(command,0,.017f,7.75f)==0&&pending.remaining()==0,"owner or mode epoch change withdraws pending work");
 }
 for(auto side:{BoardChord::lb,BoardChord::rb}){
  AirRotation input;poll(input,0);auto out=poll(input,1,side);
  check(out.spin.serial==1&&out.spin.balance==(side==BoardChord::lb?1:-1),"single physical trigger appends exactly one signed pi request");
  for(unsigned now=2;now<100;++now)check(poll(input,now,side).spin.serial==1,"held shoulder polls never repeat tap recognition");
  poll(input,100);check(poll(input,101,side).spin.serial==2,"physical release permits the next independent tap");
 }
 {
  AirRotation input;poll(input,0);auto out=poll(input,1,BoardChord::pair);check(out.spin.serial==0,"simultaneous opposing triggers are both consumed without pending angle");
  check(poll(input,2,BoardChord::lb).spin.serial==0,"releasing one opposed shoulder cannot manufacture a new trigger");
  poll(input,3);check(poll(input,4,BoardChord::lb).spin.serial==1,"new trigger after opposition is available");
 }
 for(auto mode:{SpinMode::held,SpinMode::tap}){
  AirRotation input;poll(input,0,0,false,false,mode,SkatePose::riding);poll(input,1,BoardChord::lb,false,false,mode,SkatePose::riding);
  auto out=poll(input,200,BoardChord::lb,false,false,mode);
  check(mode==SpinMode::held ? out.yaw_rate==-7.75f : out.spin.serial==0,"preheld shoulder launch retains held default and does not fabricate a tap at AIR entry");
  check(!poll(input,217,BoardChord::lb,false,false,mode,SkatePose::riding).owns_left,"landing emits no prewind or ordinary ground spin recipe");
 }
 for(unsigned age:{99u,100u,101u}){
  AirRotation input;poll(input,0,0,false,false,SpinMode::held);poll(input,1,0,true,false,SpinMode::held);
  const auto out=poll(input,1+age,0,true,false,SpinMode::held);
  check(out.yaw_rate==(age>100?-7.75f:0),"directional selection uses strict100ms and full selected rate without a50ms ramp");
 }
 {
  AirRotation input;poll(input,0,0,false,false,SpinMode::held);poll(input,1,0,true,true,SpinMode::held);
  check(poll(input,102,0,true,true,SpinMode::held).yaw_rate==7.75f,"source Right precedence applies to explicitly supplied directional controls");
  SpinFacts facts{};facts.orientation_disabled=SpinFact::set;
  check(poll(input,103,0,true,false,SpinMode::held,SkatePose::air,true,false,facts).yaw_rate==0,"known orientation exclusion suppresses directional spin");
  check(poll(input,104,BoardChord::lb,true,false,SpinMode::held,SkatePose::air,true,false,facts).yaw_rate==-7.75f,"orientation exclusion leaves shoulder selection independent");
  facts.no_spin=SpinFact::set;check(poll(input,105,BoardChord::lb,true,false,SpinMode::held,SkatePose::air,true,false,facts).yaw_rate==0,"known NoSpin cannot select a rate");
  check(SpinFacts::decode(SpinFacts{}.encode()).no_spin==SpinFact::unknown,"missing native facts stay unknown through transport");
 }
 // Actual input selector -> physics controller -> articulated body / target
 // interfaces. This is an offline composition, not native action evidence.
 for(unsigned cadence:{17u,34u}){
  NativeInputState physics;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample native{};native.valid=true;native.state=100;native.crouch_mode=1;
  AirMotion motion;AirRotation input;AirMotionIntent intent{};intent.enabled=intent.angular_allowed=true;unsigned now=1000;
  auto step=[&](std::uint16_t buttons){
   auto frame=physics.observe(now,owner,native,{true,-1});
   const auto selected=poll(input,now,buttons,false,false,SpinMode::tap,native.state==100?SkatePose::riding:SkatePose::air);
   intent.sampled_at=now;intent.yaw_rate=selected.yaw_rate;intent.spin=selected.spin;intent.angular_proof=NativeAirSpinProof::from_frame(frame);
   const auto output=motion.step(intent,{now,owner.core,native.state,cadence*.001f,10,0,0,true,5,0,true,intent.angular_proof});
   now+=cadence;return output;
  };
  step(0);native.state=200;step(0);auto out=step(BoardChord::lb);unsigned frames=0;double angle=0;
  while(out.spin_y!=0){
   ++frames;angle+=out.spin_y*cadence*.001;
   std::array<std::array<float,3>,2> velocities{{{5,0,0},{6,1,2}}},spins{{{0,0,0},{1,2,3}}};
   check(AirMotion::apply_bodies(out,velocities,spins)&&velocities[1][0]-velocities[0][0]==1&&near(spins[1][1]-spins[0][1],2),"actual body adapter preserves relative articulated velocities and spin");
   auto current=intent.angular_proof;
   check(AirMotion::angular_target_current(out,intent,current,current,now-cadence),"current tap target survives physical release while its exact physics output remains current");
   auto changed=intent;++changed.spin.serial;
   check(!AirMotion::angular_target_current(out,changed,current,current,now-cadence),"new tap serial withdraws the cached target until physics consumes it");
   changed=intent;changed.spin.mode=SpinMode::held;
   check(!AirMotion::angular_target_current(out,changed,current,current,now-cadence),"mode change revokes cached tap target");
   out=step(0);check(frames<40,"composed single tap terminates");
  }
  const auto expected=static_cast<unsigned>(std::ceil(TapSpin::pi/(7.75*cadence*.001)));
  check(frames==expected&&std::abs(angle)>TapSpin::pi,"actual rotation-to-motion composition retains full-final-step oracle at17/34ms");
  out=step(BoardChord::rb);check(out.spin_y>0,"opposite new tap starts after neutral continuation");
  native.state=100;check(!step(0).angular,"observed landing withdraws a partially completed tap");
  native.state=200;check(step(0).spin_y==0,"later flight cannot replay landed pending angle");
 }
 for(unsigned fault=0;fault<4;++fault){
  AirRotation input;poll(input,0);auto out=poll(input,1,BoardChord::lb);
  check(out.spin.serial==1,"negative-control fixture begins one physical tap");
  if(fault==0)input.reset();
  if(fault==1)input.poll_qualified(2,BoardChord::lb,false,false,false,SkatePose::air,true,false,SpinMode::tap);
  if(fault==2)poll(input,2,static_cast<std::uint16_t>(BoardChord::lb|0x10u));
  if(fault==3){poll(input,2,0);poll(input,3,BoardChord::rb,false,false,SpinMode::tap,SkatePose::air,false);}
  out=poll(input,10,BoardChord::rb);
  check(out.spin.serial==0 || (fault==3&&out.spin.serial==1),"reset/OFF/menu/unready edges cannot replay an old or deferred tap");
 }
 std::cout<<checks<<" tap and direction spin checks passed\n";
}
