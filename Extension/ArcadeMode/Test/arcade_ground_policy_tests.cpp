#include "Extension/ArcadeMode/arcade_ground_rotation.h"
#include "Extension/ArcadeMode/arcade_ground_actions.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <type_traits>
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
 for(short x:{short(-32767),short(-16000),short(-1),short(1),short(16000),short(32767)}) {
  Frame f;GroundYaw controller;auto input=f.intent();
  input.ground_controls=GroundControl::physical(true,0,x,0);
  input.ground_controls.native_horizontal=x!=0;
  input.ground_controls=GroundControl::decode(input.ground_controls.encode());
  check(input.ground_controls.native_horizontal,"delivered native LX ownership survives the existing atomic carrier");
  for(unsigned contacts:{0xc000u,0x4000u,0xc000u,0x8000u,0xc000u}) {
   f.raw.words[1]=contacts;f.step(f.native.sampled_at+34);
   input.sampled_at=f.native.sampled_at;input.angular_proof=NativeAirSpinProof::from_frame(f.native);
   const auto output=controller.step(input,f.native,f.native.sampled_at,1.f/30,{0,0,4},true,false);
   check(!output.angular&&!output.velocity_owned,"ordinary native LX has no competing SDK angular/translation writer across push contacts");
  }
 }
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

 // Compose physical recognition with the once-only grounded writer policy.
 for(unsigned cadence:{17u,34u,75u}){
  Frame f;InputSession session;InputAdmission admission{};admission.gameplay=admission.authored=admission.may_start=true;admission.pose=SkatePose::riding;
  NativeGesture::Output result{};
  auto poll=[&](unsigned now,std::uint16_t buttons){
   f.step(now);admission.tick=now;admission.native_context={true,true,{f.native.owner_generation,f.native.contact_generation,f.native.action_generation},f.native.physics_step,f.native.sampled_at,false};
   return session.poll(now,{buttons},0,0,admission);
  };
  for(auto row:{std::pair{1000u,0u},std::pair{1017u,2u},std::pair{1034u,0u},std::pair{1051u,2u},std::pair{1068u,0u},std::pair{1085u,0x200u}})result=poll(row.first,static_cast<std::uint16_t>(row.second));
  check(result.ground_turn.serial&&!result.unsupported,"actual Session dispatch starts recognized ground command");
  const auto command=result.ground_turn;GroundYaw controller;float accumulated=0;unsigned elapsed=0;
  for(;;){
   if(elapsed)result=poll(1085+elapsed,0);
   check(result.ground_turn==command,"polling preserves original command identity and recognition clock");
   auto intent=f.intent();intent.ground_controls.left=true;intent.spin.left_pressed_at=1000;
   const float dt=cadence/1000.f;
   const auto output=controller.step(intent,f.native,1085+elapsed,dt,{1,0,2},true,false,&result.ground_turn);
   accumulated+=output.angle_delta;
   check(output.ready&&output.angular&&!output.velocity_owned,"physical command owns yaw while preserving native translation");
   check(near(accumulated,static_cast<float>(TapSpin::pi*std::min(elapsed,300u)/300.)),"physical command follows clipped300ms half-turn");
   check(GroundYaw::target_current(output,intent,f.native,1085+elapsed,false,&command),"current command reaches physical target policy");
   const auto again=controller.step(intent,f.native,1085+elapsed,dt,{1,0,2},true,false,&command);
   check(again.angle_delta==output.angle_delta&&again.rate==output.rate,"duplicate physics read does not advance elapsed or restart final step");
   auto future=f.native;++future.physics_step;
   check(!GroundYaw::target_current(output,intent,future,1085+elapsed,false,&command),"command target cannot leak into another physical step");
   if(elapsed>=300)break;
   elapsed+=cadence;
  }
  result=poll(1085+elapsed+cadence,0);auto intent=f.intent();
  const auto after=controller.step(intent,f.native,f.native.sampled_at,cadence/1000.f,{1,0,2},true,false,&command);
  check(after.angle_delta==0&&after.rate==0&&!after.command.serial,"first physical step after completion adds no command rotation");
  controller.reset();f.step(f.native.sampled_at+cadence);intent=f.intent();
  check(!controller.step(intent,f.native,f.native.sampled_at,cadence/1000.f,{1,0,2},true,false,&command).command.serial,"spent command cannot replay after reset");
 }
 for(unsigned fault=0;fault<12;++fault){
  Frame f;auto intent=f.intent();GroundTurnCommand command{1,1000,{f.native.owner_generation,f.native.contact_generation,f.native.action_generation},f.native.physics_step,1000};GroundYaw controller;
  check(controller.step(intent,f.native,1000,.017f,{1,0,2},true,false,&command).command.serial==1,"cancellation starts with independently qualified command");
  f.step(1017);intent=f.intent();bool ordinary=true,other=false;auto velocity=std::array<float,3>{1,0,2};auto selected=command;
  if(fault==0){++f.owner.core;f.step(1017);intent=f.intent();}
  if(fault==1){++selected.generations.contact;}
  if(fault==2){++selected.generations.action;}
  if(fault==3){f.raw.state=200;f.step(1017);intent=f.intent();}
  if(fault==4){f.raw.words[1]=0x4000;f.step(1017);intent=f.intent();}
  if(fault==5)ordinary=false;
  if(fault==6)other=true;
  if(fault==7)intent.enabled=false;
  if(fault==8)intent.angular_allowed=false;
  if(fault==9)intent.spin.physical_armed=false;
  if(fault==10)velocity[0]=std::numeric_limits<float>::quiet_NaN();
  const auto output=controller.step(intent,f.native,1017,.017f,velocity,ordinary,other,fault==11?nullptr:&selected);
  check(!output.command.serial&&!output.angular,"owner/contact/action/AIR/Manual/upright/OFF/conflict/nonfinite/withdrawal retires command");
  f.raw.state=100;f.raw.words[1]=0xc000;f.step(1034);intent=f.intent();
  check(!controller.step(intent,f.native,1034,.017f,{1,0,2},true,false,&command).command.serial,"returning eligibility cannot revive retired command");
 }

 for(unsigned fault=0;fault<10;++fault){
  Frame f;InputSession session;InputAdmission admission{};admission.gameplay=admission.authored=admission.may_start=true;admission.pose=SkatePose::riding;
  auto poll=[&](unsigned now,std::uint16_t buttons){f.step(now);admission.tick=now;admission.native_context={true,true,{f.native.owner_generation,f.native.contact_generation,f.native.action_generation},f.native.physics_step,f.native.sampled_at,false};return session.poll(now,{buttons},0,0,admission);};
  NativeGesture::Output result{};
  for(auto row:{std::pair{1000u,0u},std::pair{1017u,2u},std::pair{1034u,0u},std::pair{1051u,2u},std::pair{1068u,0u},std::pair{1085u,0x200u}})result=poll(row.first,static_cast<std::uint16_t>(row.second));
  const auto serial=result.ground_turn.serial;check(serial!=0,"Session cancellation fixture begins recognized command");
  if(fault==0)admission.gameplay=false;
  if(fault==1)admission.pose=SkatePose::air;
  if(fault==2)admission.pose=SkatePose::manual;
  if(fault==3)admission.pose=SkatePose::grind;
  if(fault==4)admission.observed_lip=true;
  if(fault==5)admission.external_owner=true;
  if(fault==6)admission.offboard=true;
  if(fault==7)++f.owner.core;
  if(fault==8)admission.pose=SkatePose::bail;
  result=poll(1102,fault==9?0x1000:0);
  if(result.ground_turn.serial)std::cerr<<"Session fault="<<fault<<"\n";
  check(!result.ground_turn.serial,"actual Session withdraws turn for gameplay/AIR/Manual/rail/Lip/external/offboard/owner/bail/jump loss");
  session.reset();admission={};admission.gameplay=admission.authored=admission.may_start=true;admission.pose=SkatePose::riding;
  for(auto row:{std::pair{1200u,0u},std::pair{1217u,2u},std::pair{1234u,0u},std::pair{1251u,2u},std::pair{1268u,0u},std::pair{1285u,0x200u}})result=poll(row.first,static_cast<std::uint16_t>(row.second));
  check(result.ground_turn.serial>serial,"new physical recognition after Session reset has a new command identity");
 }

 for(unsigned fault=0;fault<13;++fault){
  Frame f;GroundTurnAngularLease lease;GroundTurnAngularLease::Parts parts{};GroundTurnAngularLease::Spins spins{};
  for(unsigned i=0;i<32;++i){parts[i]=100+i;spins[i]={i*.02f,10.f+i*.03f,i*.04f};}
  std::array<float,3> up{0,1,0};
  lease.record(f.native,parts,spins,up);const auto original=spins;f.step(1017);bool permitted=true;
  if(fault==1)++f.native.owner.core;
  if(fault==2)++f.native.owner_generation;
  if(fault==3)++f.native.contact_generation;
  if(fault==4)++f.native.action_generation;
  if(fault==5)f.native.state=200;
  if(fault==6)f.native.contacts=0x4000;
  if(fault==7)++parts[9];
  if(fault==8)spins[9][0]+=1;
  if(fault==9)permitted=false;
  if(fault==10)f.native.sampled_at=1200;
  if(fault==12)up[0]=.005f;
  const unsigned now=fault==11?1151:1017;
  check(lease.release(f.native,parts,spins,up,now,permitted)==(fault==0),"angular release requires the exact verified write, owner/contact/action/body and qualified current context");
  check(!lease.release(f.native,parts,original,up,now,true),"consumed or revoked angular receipt cannot later release another owner's rate");
 }

 static_assert(!std::is_convertible_v<MotionCleanupProof,NativeInputFrame>);
 for(bool air:{false,true})for(unsigned fault=0;fault<8;++fault){
  NativeInputState commands,physical;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample sample{};
  sample.valid=true;sample.state=air?200:100;sample.crouch_mode=1;sample.words[1]=air?0:0xc000;
  const auto command=commands.observe(1000,owner,sample,{true,-1});
  const MotionCleanupProof original{physical.observe(1000,owner,sample,{true,-1})};
  GroundTurnAngularLease lease;GroundTurnAngularLease::Parts parts{};GroundTurnAngularLease::Spins spins{};
  for(unsigned i=0;i<32;++i){parts[i]=100+i;spins[i]={0,2.f+i*.1f,0};}
  const std::array<float,3> up{0,1,0};lease.record(command,parts,spins,up,original);
  commands.invalidate();
  check(!commands.correlate(1017,owner,sample).valid,"OFF invalidates normal proof even while a write receipt awaits cleanup");
  if(fault==1)++owner.core;
  if(fault==2)sample.words[1]=0x4000;
  if(fault==3)physical.invalidate();
  if(fault==4)spins[7][2]=1;
  if(fault==5)++parts[7];
  if(fault==6)sample.state=air?100:200;
  const MotionCleanupProof current{physical.observe(fault==7?1151:1017,owner,sample,{true,-1})};
  const unsigned now=fault==7?1151:1017;
  if(fault==0)check(!lease.release_cleanup({},parts,spins,up,now,true),"unavailable cleanup query defers without granting authority or consuming the original receipt");
  check(lease.release_cleanup(current,parts,spins,up,now,true)==(fault==0),"cleanup-only physical proof can retire an exact write across OFF but cannot cross owner/contact/gap/impulse/body/context loss");
  check(!lease.release_cleanup(current,parts,spins,up,now,true),"repeated OFF cannot reuse consumed or revoked cleanup authority");
 }
 std::cout<<checks<<" ground steering and action policy checks passed\n";
}
