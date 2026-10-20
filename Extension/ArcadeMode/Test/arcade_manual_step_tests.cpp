#include "Extension/ArcadeMode/arcade_manual_step.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
bool same(const PhysicalBalanceSample& a,const PhysicalBalanceSample& b){
 return a.valid==b.valid && a.active==b.active && a.exit==b.exit &&
  std::abs(a.position-b.position)<=1e-6f && std::abs(a.velocity-b.velocity)<=1e-6f &&
  std::abs(a.elapsed-b.elapsed)<=1e-6f && std::abs(a.repeat_pressure-b.repeat_pressure)<=1e-6f && a.applied_correction==b.applied_correction;
}
struct Pipeline {
 NativeInputOwner owner{1,2,3,4,5};NativeInputState observer;NativeInputFrame frame{};ManualPhysics physics;ManualPhysicsSteps steps;
 bool nose{};std::uint64_t now=1000;unsigned cadence;
 Pipeline(bool n,unsigned dt):nose(n),cadence(dt){}
 SkatePose pose()const{return nose?SkatePose::nose_manual:SkatePose::manual;}
 InputDigitalState input(unsigned i)const{InputDigitalState v{};v.available=true;v.sampled_at=now;v.stick_buttons=(i%3==0)?1u:(i%3==1)?2u:0u;v.buttons=v.stick_buttons;return v;}
 void observe(unsigned i){NativeRequestSample s{};s.valid=true;s.state=100;s.crouch_mode=1;s.words[1]=nose?0x8000:0x4000;
  frame=observer.observe(now,owner,s,{true,-1});steps.observe(frame,cadence/1000.f,input(i));now+=cadence;}
 PhysicalBalanceSample sample(){return steps.sample(physics,frame,pose(),frame.sampled_at,true);}
};
}
int main(){
 for(bool nose:{false,true})for(unsigned dt:{17u,34u})for(unsigned lag:{0u,1u,2u,3u}){
  Pipeline p(nose,dt);ManualPhysics oracle;BalanceControl controls;PhysicalBalanceSample expected{};
  for(unsigned i=0;i<36;++i){
   p.observe(i);const auto digital=p.input(i);auto mapping=InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual);mapping.admitted=true;
   const auto pair=digital.configured(mapping);controls.begin_manual(p.frame.sampled_at);controls.poll_manual(p.frame.sampled_at,expected.position,{pair.first,pair.second},true);
   expected=manual_physics_pose(oracle,p.pose(),dt/1000.f,controls.manual_input());
   if(i && i%(lag+1)!=0 && i!=35)continue;
   const auto actual=p.sample();check(same(actual,expected),"each original interval/control contributes once regardless of delayed client drains");
   for(unsigned duplicate=0;duplicate<9;++duplicate)check(same(p.sample(),expected),"cached native step cannot repeat dt/correction/fatigue/RNG advancement");
  }
  // A subsequent neutral step detects hidden RNG consumption by duplicate reads.
  p.observe(36);auto neutral=p.input(36);neutral.stick_buttons=0; // This row's input remains the observed original; no client substitution.
  controls.poll_manual(p.frame.sampled_at,expected.position,{true,false},true);expected=manual_physics_pose(oracle,p.pose(),dt/1000.f,controls.manual_input());
  check(same(p.sample(),expected),"duplicates cannot consume hidden neutral random stream before the next distinct interval");
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u})for(unsigned fault=0;fault<12;++fault){
  Pipeline p(nose,dt);p.observe(0);check(p.sample().valid,"qualified first interval integrates");p.observe(1);
  if(fault==0)++p.frame.owner.rig;if(fault==1)++p.frame.owner_generation;
  if(fault==2)--p.frame.physics_step;if(fault==3)p.frame.valid=false;
  if(fault==4)p.frame.sampled_at+=1;
  if(fault==5)p.steps.invalidate();if(fault==6)p.steps.reset();
  if(fault==7){p.frame.physics_step+=1;}
  const auto now=fault==8?p.frame.sampled_at+151:fault==9?p.frame.sampled_at-1:p.frame.sampled_at;
  const auto pose=fault==10?(nose?SkatePose::manual:SkatePose::nose_manual):p.pose();
  check(!p.steps.sample(p.physics,p.frame,pose,now,fault!=11).valid,"mismatch/rollback/gap/clock/stale/OFF/reset/type/paused context cannot advance old physical ownership");
  check(!p.physics.active(),"invalid original interval clears physical balance rather than extrapolating client time");
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u}){
  Pipeline p(nose,dt);p.observe(0);p.sample();p.observe(1);p.steps.observe(p.frame,dt/1000.f,p.input(1));
  check(!p.sample().valid,"duplicate producer sequence is an invalid interval, not two physics steps");
  Pipeline gap(nose,dt);gap.observe(0);gap.sample();NativeRequestSample missing{};missing.valid=true;missing.state=100;missing.crouch_mode=1;missing.words[1]=nose?0x8000:0x4000;gap.observer.observe(gap.now,gap.owner,missing,{true,-1});gap.now+=dt;gap.observe(2);
  check(!gap.sample().valid,"lost original interval is not replaced by last dt times a guessed count");
  Pipeline invalid(nose,dt);invalid.observe(0);invalid.sample();invalid.observe(1);invalid.steps.observe(invalid.frame,0,{});
  check(!invalid.sample().valid,"invalid original dt revokes the original batch");
  Pipeline many(nose,dt);many.observe(0);many.sample();for(unsigned i=1;i<=ManualPhysicsSteps::capacity+1;++i)many.observe(i);
  check(!many.sample().valid,"overflow rejects missing provenance without a catch-up timer");
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u}){
  for(unsigned boundary=0;boundary<4;++boundary){
   Pipeline p(nose,dt);p.observe(0);p.sample();
   NativeRequestSample raw{};raw.valid=true;raw.state=100;raw.crouch_mode=1;raw.words[1]=nose?0x8000:0x4000;
   if(boundary==0)++p.owner.entity;if(boundary==1)p.observer.invalidate();
   if(boundary==2)raw.words[0]=0x20;
   if(boundary==3){p.nose=!nose;raw.words[1]=p.nose?0x8000:0x4000;}
   p.frame=p.observer.observe(p.now,p.owner,raw,{true,-1});p.steps.observe(p.frame,dt/1000.f,p.input(1));
   check(!p.sample().valid&&!p.physics.active(),"genuine new owner/action/type observation cannot inherit prior physical dynamics");
  }
  for(unsigned boundary=0;boundary<5;++boundary){
   Pipeline p(nose,dt);p.observe(0);p.sample();p.observe(1);
   const auto pose=boundary==0?SkatePose::bail:boundary==1?SkatePose::unknown:boundary==2?SkatePose::riding:boundary==3?SkatePose::air:SkatePose::grind;
   check(p.steps.sample(p.physics,p.frame,pose,p.frame.sampled_at,true).valid&&!p.physics.active(),"known cancellation/exit immediately stops dynamics without consuming cached dt again");
  }
  for(unsigned age:{150u,151u}){
   Pipeline p(nose,dt);p.observe(0);ManualPhysicsSteps steps;ManualPhysics actual,expected;BalanceControl control;
   InputDigitalState neutral{};neutral.available=true;neutral.sampled_at=p.frame.sampled_at;
   steps.observe(p.frame,dt/1000.f,neutral);control.begin_manual(p.frame.sampled_at);control.poll_manual(p.frame.sampled_at,0,{},true);
   const auto first=manual_physics_pose(expected,p.pose(),dt/1000.f,control.manual_input());
   check(same(steps.sample(actual,p.frame,p.pose(),p.frame.sampled_at,true),first),"neutral original interval primes held-entry release before lease boundary test");
   p.observe(1);
   const bool up=first.position>=0;
   InputDigitalState digital{};digital.available=true;digital.buttons=digital.stick_buttons=up?1u:2u;digital.sampled_at=p.frame.sampled_at-age;
   steps.observe(p.frame,dt/1000.f,digital);
   control.poll_manual(p.frame.sampled_at,first.position,{up,!up},age<=150);
   const auto oracle=manual_physics_pose(expected,p.pose(),dt/1000.f,control.manual_input());
   check(same(steps.sample(actual,p.frame,p.pose(),p.frame.sampled_at,true),oracle),"physical-control lease uses observer-time original age, never delayed client time");
  }
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u})for(unsigned delay:{0u,1u,99u,150u,151u}){
  Pipeline p(nose,dt);p.observe(0);const auto original=p.frame;
  const auto waiting=p.steps.sample(p.physics,original,SkatePose::riding,original.sampled_at,true);
  check(waiting.valid&&!waiting.active&&!p.physics.active(),"unadmitted one-wheel candidate cannot start dynamics");
  const auto admitted=p.steps.sample(p.physics,original,p.pose(),original.sampled_at+delay,true);
  if(delay<=150){
   check(admitted.valid&&admitted.active&&std::abs(admitted.elapsed-dt/1000.f)<1e-6f,"late qualified identity can consume only the retained original current interval");
   check(same(p.steps.sample(p.physics,original,p.pose(),original.sampled_at+delay,true),admitted),"same-step late admission still consumes interval and RNG only once");
  }else check(!admitted.valid&&!p.physics.active(),"candidate retention cannot renew original native freshness lease");
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u})for(unsigned fault=0;fault<8;++fault){
  Pipeline p(nose,dt);p.observe(0);auto current=p.frame;
  if(fault==0)current.contacts=0xc000;
  if(fault==1)current.stance_and_push=0x40;
  if(fault==2)current.state=200;
  if(fault==3)current.mode=2;
  if(fault<4){p.steps.reset();p.steps.observe(current,dt/1000.f,p.input(0));}
  p.steps.sample(p.physics,current,fault==4?SkatePose::bail:fault==5?SkatePose::unknown:SkatePose::riding,current.sampled_at,true);
  if(fault==6)++current.owner.rig;if(fault==7)++current.action_generation;
  check(!p.steps.sample(p.physics,current,p.pose(),current.sampled_at+1,true).valid&&!p.physics.active(),"ordinary ground/push/air/mode/bail/unknown/owner/action loss cannot retain a late admission interval");
 }
 for(bool nose:{false,true})for(unsigned dt:{17u,34u}){
  Pipeline p(nose,dt);p.observe(0);p.sample();
  p.steps.sample(p.physics,p.frame,SkatePose::riding,p.frame.sampled_at,true);
  check(!p.sample().valid,"retention cannot rearm an interval already consumed by a terminated section");
 }
 std::cout<<checks<<" original native Manual step checks passed\n";
}
