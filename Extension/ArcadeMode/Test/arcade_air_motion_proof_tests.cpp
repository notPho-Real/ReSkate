#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <cstdlib>
#include <iostream>
#include <type_traits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool x,const char*w){++checks;if(!x){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}
template<class Intent>void proof(Intent& i,const NativeInputFrame& f){if constexpr(requires{i.angular_proof;}){using T=std::remove_cvref_t<decltype(i.angular_proof)>;i.angular_proof=T::from_frame(f);}}
template<class Frame>void current(Frame& f,const NativeInputFrame& n){if constexpr(requires{f.native_required;f.native;}){using T=std::remove_cvref_t<decltype(f.native)>;f.native_required=true;f.native=T::from_frame(n);}if constexpr(requires{f.native_action;})f.native_action={n.flip_known,n.flip_id,n.selection_object};}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeInputFrame native{};AirMotion motion;
 AirMotionIntent intent{true,true,10,0,0,0,0,1};AirMotionFrame body{10,3,100,1.0f/60,5,0,0,true};
 void observe(std::uint64_t at,unsigned state,std::int32_t action=-1){NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=1;native=producer.observe(at,owner,s,{true,action,16});body.now=at;body.state=state;current(body,native);}
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
 // Vertical targets use the same original flight identity as angular output,
 // plus the delivered root velocity; an independent native impulse takes over.
 for(unsigned cadence:{17u,34u})for(unsigned charge:{0u,199u,200u})for(unsigned fault=0;fault<25;++fault){
  Pipeline p;p.body.dt=static_cast<float>(cadence)*.001f;
  p.observe(1000,100);p.publish(1000,0);p.sample();
  p.observe(1017,103);p.intent.release_sequence=1;p.intent.native_launch_id=1;p.intent.release_at=1017;p.intent.charge_ms=charge;p.publish(1017,0);p.sample();
  p.observe(1051,200);p.publish(1051,0);auto output=p.sample();
  check(output.vertical && std::abs(output.velocity_y-GameplayProfile{}.launch_speed(charge,LaunchDescriptor::ordinary()))<.0001f,
        "qualified ordinary launch keeps the selected absolute charge mapping");
  auto original=NativeAirSpinProof::from_frame(p.native);auto current=original;
  float velocity=output.velocity_y,dt=p.body.dt;std::uint64_t now=1051;
  if(fault==1)++original.physics_step;
  if(fault==2)p.intent.sampled_at=now+1;
  if(fault==3)++current.owner.context;
  if(fault==4){p.observe(1068,100);p.observe(1085,200);current=NativeAirSpinProof::from_frame(p.native);now=1085;p.intent.sampled_at=now;}
  if(fault==5)current.valid=false;
  if(fault==6)original.sampled_at=now-151;
  if(fault==7)dt=0;
  if(fault==8)dt=.101f;
  if(fault==9)velocity=std::numeric_limits<float>::quiet_NaN();
  if(fault==10)velocity=0;
  if(fault==11)velocity+=4;
  if(fault==12)velocity-=AirMotion::ordinary_gravity*dt;
  if(fault==13){velocity-=AirMotion::ordinary_gravity*dt;velocity=std::nextafter(velocity,0.f);}
  if(fault==14)output.vertical=false;
  if(fault==15)current.airborne=false;
  if(fault==16)++current.owner_generation;
  if(fault==17)++current.episode;
  if(fault==18)p.intent.enabled=false;
  if(fault==19)p.intent.sampled_at=now-151;
  if(fault>=20){const std::array links{&NativeInputOwner::base,&NativeInputOwner::entity,&NativeInputOwner::core,&NativeInputOwner::context,&NativeInputOwner::rig};++(current.owner.*links[fault-20]);}
  const bool expected=fault==0 || fault==12 || fault==13;
  check(AirMotion::vertical_target_current(output,p.intent,original,current,now,velocity,dt)==expected,
        "cached vertical target requires original current flight, valid cadence, fresh input and unsuperseded native velocity");
 }
 for(unsigned cadence:{17u,34u})for(int action:{2,3,4,17,18,19}){
  Pipeline p;p.body.dt=cadence*.001f;p.observe(1000,100);p.publish(1000,0);p.sample();
  p.observe(1017,103,1);p.intent.release_sequence=1;p.intent.native_launch_id=1;p.intent.release_at=1017;p.intent.charge_ms=100;p.publish(1017,0);p.sample();
  p.observe(1051,200,1);p.publish(1051,0);const auto ordinary=p.sample();
  check(ordinary.vertical,"ordinary jump still owns its original launch before a different native actor");
  const auto ordinary_proof=NativeAirSpinProof::from_frame(p.native);
  const float unchanged_velocity=p.body.velocity_y;
  p.observe(1051+cadence,201,action);p.publish(1051+cadence,7.75f);
  check(!AirMotion::vertical_target_current(ordinary,p.intent,ordinary_proof,NativeAirSpinProof::from_frame(p.native),p.body.now,
      unchanged_velocity,p.body.dt,{true,action,16}),
      "new native actor rejects a cached ordinary height before the next body update despite identical velocity");
  const auto native=p.sample();
  check(!native.vertical&&!native.horizontal&&native.angular&&native.spin_y==7.75f&&p.body.velocity_y==unchanged_velocity,
      "native actor takes trajectory ownership without losing independent qualified shoulder yaw");
  p.observe(1051+2*cadence,201);p.publish(p.body.now,7.75f);const auto idle=p.sample();
  check(!idle.vertical&&!idle.horizontal&&idle.angular,"native actor retirement cannot resume old ordinary gravity or speed limits");
  p.observe(1051+3*cadence,100);p.publish(p.body.now,0);p.sample();
  p.intent.release_sequence=2;p.intent.release_at=p.body.now;p.intent.native_launch_id=1;p.observe(1051+4*cadence,200,1);p.publish(p.body.now,0);
  check(p.sample().vertical,"a new qualified ordinary flight may establish fresh trajectory ownership");
 }
 for(int launch:{1,16})for(int actor:{-1,1,16,2,3,4,17,18,19}){
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();
  p.intent.release_sequence=1;p.intent.release_at=1017;p.intent.native_launch_id=launch;
  p.observe(1017,103,launch);p.publish(1017,0);p.sample();
  p.observe(1034,200,launch);p.publish(1034,0);const auto launched=p.sample();
  check(launched.vertical&&launched.native_launch_id==launch,"admitted Ollie or Nollie owns only its captured native launch");
  const auto proof=NativeAirSpinProof::from_frame(p.native);
  p.intent.native_launch_id=actor; // A later input sample cannot relabel this flight.
  p.observe(1051,201,actor);p.publish(1051,7.75f);
  const bool ordinary=actor==-1||actor==launch;
  check(AirMotion::vertical_target_current(launched,p.intent,proof,NativeAirSpinProof::from_frame(p.native),1051,
      p.body.velocity_y,p.body.dt,{true,actor,16})==ordinary,"cached target uses captured launch identity, not a mutable later publication");
  const auto out=p.sample();check(out.vertical==ordinary&&out.angular,"unrelated positive actor yields trajectory while retaining qualified yaw");
  if(!ordinary){p.observe(1068,201,launch);p.publish(1068,7.75f);check(!p.sample().vertical,"even the original launch ID cannot reacquire after native takeover");
   p.observe(1085,201,-1);p.publish(1085,7.75f);check(!p.sample().vertical,"idle cannot revive retired flight ownership");}
 }
 for(int actor:{-1,1,16}){
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();p.observe(1017,200,actor);p.publish(1017,0);
  check(p.sample().vertical==(actor==-1),"a ledge without admitted release cannot globally exempt a positive launch actor");
 }
 for(int launch:{1,16}){
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();
  p.intent.release_sequence=1;p.intent.release_at=1017;p.intent.native_launch_id=launch;
  p.observe(1017,200,launch);p.publish(1017,0);check(p.sample().vertical,"fresh matching launch starts trajectory");
  p.observe(1034,100);p.publish(1034,0);p.sample();
  p.observe(1051,200,launch);p.publish(1051,0);
  check(!p.sample().vertical,"a consumed release cannot exempt its old native actor in another flight");
  p.observe(1068,100);p.publish(1068,0);p.sample();
  p.intent.release_sequence=2;p.intent.release_at=1085;
  p.observe(1085,200,launch);p.publish(1085,0);
  check(p.sample().vertical,"fresh same-kind release can own a subsequent qualified flight");
 }
 {
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();p.intent.release_sequence=1;p.intent.release_at=1017;p.intent.native_launch_id=1;p.observe(1017,200,1);p.publish(1017,0);
  check(p.sample().vertical,"original body establishes an ordinary vertical write");
  // The guarded target callback can observe actor takeover before the next
  // body update. Its retirement must survive a later idle observation.
  p.motion.yield_ordinary_motion();p.observe(1034,201,-1);p.publish(1034,7.75f);
  auto out=p.sample();check(!out.vertical&&!out.horizontal&&out.angular,"target-first retirement is irreversible in the same flight and independent of yaw");
  p.observe(1051,100);p.publish(1051,0);p.sample();p.intent.release_sequence=2;p.intent.release_at=1068;p.intent.native_launch_id=1;p.observe(1068,200,1);p.publish(1068,0);
  check(p.sample().vertical,"target-first retirement ends only at a new ordinary flight");
 }
 for(int flip:{2,3,4}) {
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();
  p.intent.release_sequence=1;p.intent.release_at=1017;p.intent.native_launch_id=1;
  p.observe(1017,200,1);p.publish(1017,0);const auto launch=p.sample();
  const auto launch_proof=NativeAirSpinProof::from_frame(p.native);
  p.observe(1034,201,flip);p.publish(1034,0);p.intent.native_followup_id=flip;
  const auto current=NativeAirSpinProof::from_frame(p.native);
  check(AirMotion::vertical_target_current(launch,p.intent,launch_proof,current,1034,p.body.velocity_y,p.body.dt,{true,flip,16}),
    "target-first native flip acknowledgment retains the qualified ordinary trajectory before the body callback");
  check(!AirMotion::other_native_action({true,flip,16},launch.native_launch_id,AirMotion::qualified_followup(p.intent,current,1034)),
    "target-first retirement guard uses current authored flight authority");
 }
 for(int flip:{2,3,4})for(unsigned fault=0;fault<5;++fault){
  Pipeline p;p.observe(1000,100);p.publish(1000,0);p.sample();
  p.intent.release_sequence=1;p.intent.release_at=1017;p.intent.native_launch_id=1;
  p.observe(1017,200,1);p.publish(1017,0);check(p.sample().vertical,"qualified ordinary launch begins");
  p.intent.native_followup_id=fault==1?0:flip;
  p.observe(1034,201,fault==2?16:flip);p.publish(1034,0);
  if(fault==3)++p.intent.angular_proof.departure;
  if(fault==4)p.body.velocity_y+=4; // independent native impulse, even during a legitimate flip
  const auto out=p.sample();
  check(out.vertical==(fault==0),"only qualified same-flight basic flip retains launch; unknown actor, lost proof and native impulse still win");
  if(!fault){const auto proof=NativeAirSpinProof::from_frame(p.native);
   check(AirMotion::vertical_target_current(out,p.intent,proof,proof,1034,p.body.velocity_y,p.body.dt,{true,flip,16}),
     "native motion-target callback agrees with body writer on legitimate basic flip continuation");
   p.intent.native_followup_id=0;
   check(!AirMotion::vertical_target_current(out,p.intent,proof,proof,1034,p.body.velocity_y,p.body.dt,{true,flip,16}),
     "withdrawn authored ownership cannot retain the flip trajectory exemption");
  }
 }
 for(unsigned rate:{30u,60u,120u})for(int flip:{2,3,4}) {
  Pipeline plain,flipped;float plain_y{},flip_y{};
  for(auto* p:{&plain,&flipped}) {
   p->body.dt=1.0f/static_cast<float>(rate);p->observe(1000,100);p->publish(1000,0);p->sample();
   p->intent.release_sequence=1;p->intent.release_at=1017;p->intent.native_launch_id=1;
  }
  for(unsigned step=0;step<rate;++step) {
   const auto now=1017+step*1000/rate;
   plain.observe(now,step?201:200,1);plain.publish(now,0);
   flipped.observe(now,step?201:200,step>=2?flip:1);flipped.publish(now,0);
   if(step>=1)flipped.intent.native_followup_id=flip;
   const auto a=plain.sample(),b=flipped.sample();
   check(a.vertical && b.vertical && a.velocity_y==b.velocity_y,
     "same launch retains identical velocity before and after qualified native flip acknowledgment without a second impulse");
   plain_y=AirMotion::next_height(plain_y,a,plain.body.dt);flip_y=AirMotion::next_height(flip_y,b,flipped.body.dt);
   check(plain_y==flip_y,"production native motion targets retain identical trajectory through flip selection and playback");
  }
 }
 std::cout<<checks<<" native motion publication proof checks passed\n";
}
