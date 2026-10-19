#include "Extension/ArcadeMode/arcade_ground_rotation.h"
#include "Extension/ArcadeMode/arcade_ground_actions.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool c,const char* why){++checks;if(!c){std::cerr<<why<<'\n';std::exit(1);}}
bool near(float a,float b){return std::abs(a-b)<.00001f;}
struct Frame {
 NativeInputState state;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample raw{};NativeInputFrame native{};
 Frame(){raw.valid=true;raw.state=100;raw.crouch_mode=1;raw.words[1]=0xc000;step(1000);}
 void step(std::uint64_t now){native=state.observe(now,owner,raw,{true,-1});}
 AirMotionIntent intent()const{AirMotionIntent value{};value.enabled=value.angular_allowed=true;value.sampled_at=native.sampled_at;value.angular_proof=NativeAirSpinProof::from_frame(native);value.spin.physical_armed=true;value.ground_controls.known=true;return value;}
};
}
int main(){
 for(bool left:{false,true})for(bool right:{false,true})for(bool down:{false,true})
 for(float inches:{9.9f,10.f,10.1f})for(unsigned age:{599u,600u,601u})
 for(auto lock:{GroundVelocityLock::unknown,GroundVelocityLock::unlocked,GroundVelocityLock::locked}){
  Frame f;f.step(1000+age);auto input=f.intent();input.ground_controls.left=left;input.ground_controls.right=right;input.ground_controls.down=down;input.ground_controls.velocity_lock=lock;
  input.spin.left_pressed_at=input.spin.right_pressed_at=1000;const float speed=inches*.0254f;
  const float ramp=down&&inches<10 ? std::min(age,600u)/600.f : 1.f;
  const float expected=(left?-1.f:right?1.f:0.f)*(down?3.6f:1.8f)*ramp;
  for(float dt:{.017f,.034f}){
   GroundYaw controller;const auto out=controller.step(input,f.native,f.native.sampled_at,dt,{0,0,speed},true,false);
   check(out.ready&&near(out.rate,expected)&&out.angular==(left||right),"actual ground law uses Left priority, selected normal/sharp rate, strict10-inch threshold and600ms ramp");
   check(out.velocity_owned==((left||right)&&lock==GroundVelocityLock::unlocked),"unknown lock is distinct from unlocked and locked");
   std::array<std::array<float,3>,2> velocity{{{0,0,speed},{1,2,speed+3}}},spin{{{0,0,0},{1,2,3}}};const auto before=velocity;
   if(out.angular){
    check(AirMotion::apply_bodies(GroundYaw::body_output(out),velocity,spin),"ground uses the common articulated body adapter");
    check(velocity[0][1]==0&&velocity[1][1]==2&&near(velocity[1][0]-velocity[0][0],1)&&near(velocity[1][2]-velocity[0][2],3),"ground turn preserves Y and articulated relative velocities");
    if(lock!=GroundVelocityLock::unlocked)check(velocity==before,"locked and unavailable velocity branches retain native translation");
    else check(near(std::hypot(velocity[0][0],velocity[0][2]),speed),"unlocked yaw rotates velocity without restoring or removing speed");
    check(GroundYaw::target_current(out,input,f.native,f.native.sampled_at),"original actual ground observation reaches cached physical target");
    auto stale=input;++stale.spin.epoch;check(!GroundYaw::target_current(out,stale,f.native,f.native.sampled_at),"new input epoch revokes cached ground output");
    check(!GroundYaw::target_current(out,input,f.native,f.native.sampled_at,true),"an independently admitted Manual withdraws the ground target");
   }
  }
 }
 for(unsigned fault=0;fault<9;++fault){
  Frame f;auto input=f.intent();input.ground_controls.left=true;input.spin.left_pressed_at=1000;
  GroundYaw controller;check(controller.step(input,f.native,1000,.017f,{0,0,1},true,false).angular,"negative fixture begins actual ground yaw");
  bool ordinary=true,other=false;unsigned now=1017;
  if(fault==0){++f.owner.core;f.step(now);}
  if(fault==1){f.raw.state=200;f.step(now);}
  if(fault==2){f.raw.words[1]=0x4000;f.step(now);}
  if(fault==3)ordinary=false;
  if(fault==4)other=true;
  if(fault==5)input.angular_allowed=false;
  if(fault==6)input.spin.physical_armed=false;
  if(fault==7)now=1151;
  if(fault==8)input.spin.left_pressed_at=1100;
  check(!controller.step(input,f.native,now,.017f,{0,0,1},ordinary,other).angular,"owner/body/contact/AIR/unsupported/Manual/ownership/unarmed/stale/future bounds stop the ground writer");
 }
 {
  Frame f;SetupStanceSelection selection;
  check(!selection.request_cycle(f.native,1000,true,false),"unknown stance cannot become a fabricated Normal state");
  for(auto observed:{SetupStance::normal,SetupStance::pressure,SetupStance::nollie}){
   check(selection.observe(observed,f.native,f.native,1000),"positive native stance observation is retained");
   check(selection.request_cycle(f.native,1000,true,false)==next_setup_stance(observed),"normal Pressure Nollie cycle requests preserve ordering");
   check(selection.remembered()==observed,"request does not commit an unacknowledged native stance");
   check(!selection.request_cycle(f.native,1000,false,false)&&!selection.request_cycle(f.native,1000,true,true),"unsupported or Manual-owned request cannot change stance or channel ownership");
  }
  check(reset_setup_stance(SetupStance::pressure,GroundActionKind::switch_animation)==SetupStance::pressure,"ground switch NollieOff preserves Pressure");
  check(reset_setup_stance(SetupStance::nollie,GroundActionKind::powerslide_180)==SetupStance::normal,"explicit NollieOff target clears Nollie");
  check(reset_setup_stance(SetupStance::pressure,GroundActionKind::landing_revert)==SetupStance::normal,"landing Revert requests both setup modes off");
  check(reset_setup_stance(SetupStance::nollie,GroundActionKind::manual_link)==SetupStance::nollie,"ManualLink has no invented ground-switch reset");
 }
 check(physical_revert_side(SpinFact::set,SpinFact::set,RevertSide::backside)==RevertSide::frontside,"explicit FS has priority over BS and last physical spin");
 check(physical_revert_side(SpinFact::clear,SpinFact::set,RevertSide::frontside)==RevertSide::backside,"explicit BS precedes physical spin fallback");
 check(physical_revert_side(SpinFact::clear,SpinFact::clear,RevertSide::frontside)==RevertSide::frontside,"last physical spin supplies only independently known fallback");
 check(!physical_revert_side(SpinFact::unknown,SpinFact::set,RevertSide::frontside)&&!physical_revert_side(SpinFact::clear,SpinFact::clear,{}),"unknown side facts remain unavailable");
 {
  RevertFriction friction;for(float expected:{0.f,0.f,5.f,10.f,15.f,25.f,25.f,25.f})check(friction.select(1000)==expected,"repeat friction clamps selects then increments");
  const unsigned index=friction.index();friction.tick(20999);check(friction.index()==index,"19999ms does not decrement");
  friction.tick(21000);check(friction.index()==index,"exact20000ms does not decrement");
  friction.tick(21001);check(friction.index()==index-1,"strict20001ms decrements once");
  friction.tick(21001);check(friction.index()==index-1,"nonzero decrement restamps so duplicate polls do not decrement again");
  friction.tick(1000000);check(friction.index()==index-2,"long missed interval decrements once, not once for every elapsed interval");
  friction.physical_end();check(friction.index()==0,"existing physical clean-end or bail boundary resets the sequence");
 }
 for(unsigned age:{99u,100u,101u,666u,667u,668u}){
  Frame f;PhysicalRevert revert;NativeGroundActionObservation observation{f.native,GroundActionKind::landing_revert,9,1,0,NativeGroundPhase::initialized,true,true};
  check(revert.begin(observation,f.native,1000,SpinFact::set,SpinFact::clear,{},false),"positively observed native occurrence admits the physical policy fixture");
  for(unsigned elapsed=17;elapsed<age;elapsed+=17){f.step(1000+elapsed);observation.frame=f.native;observation.physical_elapsed_ms=elapsed;check(revert.sample(observation,f.native,1000+elapsed).available,"actual intervening physics observations retain the same native occurrence");}
  f.step(1000+age);observation.frame=f.native;observation.physical_elapsed_ms=age;
  const auto out=revert.sample(observation,f.native,1000+age);
  check(out.available&&out.active&&!out.can_kick&&out.ollied_exception==(age>=100)&&out.friction_override==(age<667)&&!out.restore_tables,"100ms Ollied,667ms friction and actual animation finish remain distinct boundaries");
  check(out.request_nollie_off&&out.request_pressure_off,"Revert policy requests both mode resets independently of unknown native setters");
  f.step(1001+age);observation.frame=f.native;observation.physical_elapsed_ms=age+1;observation.phase=NativeGroundPhase::animation_finished;
  const auto ended=revert.sample(observation,f.native,1001+age);check(ended.can_kick&&ended.restore_tables&&!ended.active&&ended.friction_override==(age+1<667),"only actual finish restores tables; special friction can outlive animation");
  check(bool(revert.rolling_friction(f.native,1001+age,true,age+1))==(age+1<667),"physical friction is independent of completed animation ownership");
  check(!revert.begin(observation,f.native,1001+age,SpinFact::set,SpinFact::clear,{},false),"spent action occurrence cannot restart");
 }
 for(auto kind:{GroundActionKind::unknown,GroundActionKind::switch_animation,GroundActionKind::landing_revert,GroundActionKind::manual_link})check(!powerslide_half_turn(kind,300),"other action kinds cannot reuse the300ms powerslide rotation");
 check(near(*powerslide_half_turn(GroundActionKind::powerslide_180,300),static_cast<float>(TapSpin::pi))&&near(*powerslide_half_turn(GroundActionKind::powerslide_180,301),static_cast<float>(TapSpin::pi)),"explicit powerslide timed rotation stops at one180-degree target");
 check(ground_action_name(GroundActionKind::landing_revert)=="Revert"&&ground_action_name(GroundActionKind::manual_link).empty(),"semantic Revert catalog entry is distinct from unsupported ManualLink identity");
 for(float speed_squared:{99.f,100.f,101.f})for(float dot:{-1.f,0.f,1.f})
  check(backwards_facing_change(speed_squared,dot,10.f,SpinFact::clear,SpinFact::clear,SpinFact::clear)==(dot<0&&speed_squared>100),"backwards facing uses strict physical dot and speed-squared tests");
 check(!backwards_facing_change(101,-1,{},SpinFact::clear,SpinFact::clear,SpinFact::clear)&&
       !backwards_facing_change(101,-1,10.f,SpinFact::clear,SpinFact::unknown,SpinFact::clear),"missing threshold or Skitching facts cannot authorize a facing flip");
 std::cout<<checks<<" ground steering and action policy checks passed\n";
}
