#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <cstdlib>
#include <iostream>
#include <type_traits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool x,const char*w){++checks;if(!x){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}
template<class Intent>void proof(Intent& i,const NativeInputFrame& f){if constexpr(requires{i.angular_proof;}){using T=std::remove_cvref_t<decltype(i.angular_proof)>;i.angular_proof=T::from_frame(f);}}
template<class Frame>void current(Frame& f,const NativeInputFrame& n){if constexpr(requires{f.native_required;f.native;}){using T=std::remove_cvref_t<decltype(f.native)>;f.native_required=true;f.native=T::from_frame(n);}}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeInputFrame native{};AirMotion motion;
 AirMotionIntent intent{true,true,10,0,0,0,0,1};AirMotionFrame body{10,3,100,1.0f/60,5,0,0,true};
 void observe(std::uint64_t at,unsigned state){NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=1;native=producer.observe(at,owner,s,{true,-1});body.now=at;body.state=state;current(body,native);}
 void publish(std::uint64_t at,float rate){intent.sampled_at=at;intent.yaw_rate=rate;proof(intent,native);}
 AirMotionOutput sample(){auto o=motion.step(intent,body);if(o.vertical)body.velocity_y=o.velocity_y;return o;}
};
}
int main(){
 for(float side:{-1.0f,1.0f})for(unsigned rate:{30u,60u}) {
  Pipeline p;p.body.dt=1.0f/static_cast<float>(rate);p.observe(10,100);p.publish(10,0);p.sample();
  p.observe(17,200);p.publish(17,side*7.75f);check(p.sample().angular,"qualified first-flight held rate reaches the physical controller");
  const auto cached=p.intent;p.observe(20,100);p.sample();p.observe(30,200);p.intent=cached;
  check(!p.sample().angular,"cached previous-flight nonzero intent cannot cross actual landing/relaunch");
  p.intent.sampled_at=30;check(!p.sample().angular,"retimestamping cached nonzero intent cannot create current-flight proof");
  p.publish(30,side*7.75f);const auto fresh=p.sample();
  check(fresh.angular&&fresh.spin_y==side*7.75f,"genuinely fresh current-flight held publication resumes without an artificial zero or a new physical press");
 }
 for(float side:{-1.0f,1.0f})for(unsigned rate:{30u,60u})for(unsigned fault=0;fault<13;++fault) {
  Pipeline p;p.body.dt=1.0f/static_cast<float>(rate);p.observe(10,100);p.publish(10,0);p.sample();
  p.observe(17,200);p.publish(17,side*7.75f);p.sample();p.observe(34,200);
  if(fault==0)p.intent.enabled=false;
  if(fault==1)p.intent.sampled_at=0;
  if(fault==2)p.body.state=700;
  if(fault==3){++p.owner.entity;p.observe(34,200);}
  if(fault==4){p.producer.invalidate();p.observe(34,200);}
  if(fault==5){auto f=p.native;++f.airborne_episode;proof(p.intent,f);}
  if(fault==6){auto f=p.native;++f.air_departure;proof(p.intent,f);}
  if(fault==7){auto f=p.native;++f.physics_step;proof(p.intent,f);}
  if(fault==8){auto f=p.native;++f.sampled_at;proof(p.intent,f);}
  if(fault==9){auto f=p.native;f.valid=false;proof(p.intent,f);}
  if(fault==10){auto f=p.native;f.airborne=false;proof(p.intent,f);}
  if(fault==11){auto f=p.native;++f.owner.rig;proof(p.intent,f);}
  if(fault==12){auto f=p.native;f.valid=false;current(p.body,f);}
  if(fault!=1)p.intent.sampled_at=34;
  check(!p.sample().angular,"menu/stale publication/bail/owner/epoch/flight/departure/future step/future proof/missing proof/nonAIR/wrong link/current loss cannot acquire selected angular ownership");
 }
 for(float side:{-1.0f,1.0f})for(unsigned rate:{30u,60u}) {
  Pipeline p;p.body.dt=1.0f/static_cast<float>(rate);p.observe(10,100);p.publish(10,0);p.sample();
  p.observe(17,200);p.publish(17,side*7.75f);p.sample();
  for(std::uint64_t at=34;at<=187;at+=17){p.observe(at,201);p.intent.sampled_at=at;const auto out=p.sample();
   check(out.angular==(at-17<=150),"retimestamped intent cannot extend its original native proof lease even during continuous same-owner AIR");}
  p.publish(187,side*7.75f);check(p.sample().angular,"fresh same-held publication can recover from an expired proof using actual current-frame evidence");
  p.observe(204,201);p.publish(204,0);auto released=p.sample();
  check(released.angular&&released.spin_y==0,"fresh physical release retains the one zero-target handoff within the same qualified flight");
  p.observe(221,201);p.publish(221,0);check(!p.sample().angular,"zero handoff cannot become indefinite angular ownership");
 }
 {
  Pipeline p;p.observe(10,100);p.publish(10,0);p.sample();p.observe(17,200);p.publish(17,7.75f);p.sample();
  NativeRequestSample s{};s.valid=true;s.state=201;s.crouch_mode=1;s.words[1]=0x4000;
  p.native=p.producer.observe(34,p.owner,s,{true,2,99});p.body.now=34;p.body.state=201;current(p.body,p.native);p.intent.sampled_at=34;
  check(p.sample().angular,"continuous AIR contact and own selection acknowledgment do not fabricate a new flight or invalidate held spin");
 }
 {
  Pipeline p;p.observe(10,100);p.publish(10,0);p.sample();p.observe(17,200);p.publish(17,7.75f);
  const auto old=p.sample();const auto old_proof=NativeAirSpinProof::from_frame(p.native);
  p.observe(20,100);p.sample();p.observe(30,200);p.publish(30,7.75f);
  const auto now=NativeAirSpinProof::from_frame(p.native);
  check(!AirMotion::angular_target_current(old,p.intent,old_proof,now,30),"cached previous-flight motion target cannot apply even after a fresh new-flight intent arrives");
  const auto fresh=p.sample();check(AirMotion::angular_target_current(fresh,p.intent,now,now,30),"fresh held intent and current-flight physics output share native target authority");
  p.publish(30,0);check(!AirMotion::angular_target_current(fresh,p.intent,now,now,30),"fresh zero-rate release prevents an older nonzero orientation output from adding a step");
  p.publish(30,7.75f);p.intent.angular_allowed=false;
  check(!AirMotion::angular_target_current(fresh,p.intent,now,now,30),"current competing owner revokes cached orientation target authority");
 }
 std::cout<<checks<<" native angular publication proof checks passed\n";
}
