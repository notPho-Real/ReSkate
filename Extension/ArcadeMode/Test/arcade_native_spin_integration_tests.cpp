#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool v,const char* m){++checks;if(!v){std::cerr<<m<<'\n';std::exit(1);}}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeInputFrame frame{};NativeRequestSample sample{};
 ArcadeInput facade{true};AirRotation rotation;AirMotion motion;AirMotionIntent intent{};AirMotionFrame body{};float dt{1.f/60};
 Pipeline(){facade.enable(true);sample.valid=true;sample.crouch_mode=1;intent.enabled=intent.angular_allowed=true;}
 void observe(std::uint64_t now,std::uint32_t state){
  sample.state=state;frame=producer.observe(now,owner,sample,{true,-1});facade.publish_native_context(frame);
  if(facade.publish_authority(now,frame.owner_generation,1,true))rotation.reset();
  body={now,owner.core,state,dt,5,0,0,true,0,0,true,NativeAirSpinProof::from_frame(frame)};
 }
 void publish(std::uint16_t held,bool enabled=true,bool ready=true,bool conflict=false){
  const auto pose=native_rotation_pose(facade.native_context(body.now),SkatePose::riding,false);
  auto rate=rotation.poll(body.now,held,enabled,pose,false,false,conflict,ready);
  if(pose==SkatePose::air&&!ready)rate={};
  intent.enabled=enabled;intent.sampled_at=body.now;intent.yaw_rate=rate.yaw_rate;intent.angular_proof=NativeAirSpinProof::from_frame(frame);
 }
 AirMotionOutput step(){return motion.step(intent,body);}
};
struct AuthoredPipeline {
 NativeInputState producer;NativeInputReadiness readiness;NativeInputOwner owner{1,2,3,4,5};
 ArcadeInput facade{true};AirRotation rotation;AirMotion motion;NativeInputFrame frame{};
 AirMotionIntent intent{};AirMotionOutput physical{};AirRotation::Output delivered{};std::vector<PreparedGesture> catalog;
 float dt{1.f/60};
 AuthoredPipeline(){
  facade.enable(true);
  for(unsigned id:{1u,2u,3u,4u,17u,18u,19u})
   catalog.emplace_back(static_cast<std::uint8_t>(id),id>=16,GestureContext::ground,
    std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}});
 }
 NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons=0,unsigned state=201,
                            int selection=-1,unsigned mode=2,SpinFacts facts={}){
  NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=mode;
  sample.words[1]=state==100?0xc000u:0;
  frame=producer.observe(now,owner,sample,{true,selection,16});
  readiness.publish(frame,{true,state==100&&mode==1,false,false,false,false,false,false,true,0});
  facade.publish_native_context(frame);
  if(facade.publish_authority(now,frame.owner_generation,1,true))rotation.reset();
  const auto available=readiness.sample(now,frame);
  const auto pose=native_rotation_pose(frame,SkatePose::riding,false);
  const auto output=facade.poll(0,now,buttons,0,0,0,0,true,available.start,false,0,false,state==100,
   true,now/17,false,catalog,available.mirror_x,false,pose,false,false,false,false,
   available.flip_ready,false,0,0,ActionExperiment::classic,available.action,available.mirror_x,false);
  auto rate=rotation.poll_qualified(now,buttons,false,false,true,pose,true,
   output.action.owned||output.phase==3,SpinMode::held,facts,&frame);
  intent.enabled=true;intent.angular_allowed=!output.action.owned&&output.phase!=3&&
   facade.allows_air_rotation(0,output.authored_playback);
  delivered=rate=AirRotation::native_shoulder_output(rate,SpinMode::held,buttons,intent.angular_allowed);
  intent.sampled_at=now;intent.yaw_rate=rate.yaw_rate;intent.angular_proof=NativeAirSpinProof::from_frame(frame);
  intent.spin=rate.spin;
  physical=motion.step(intent,{now,owner.core,state,dt,5,0,0,true,0,0,true,NativeAirSpinProof::from_frame(frame)});
  return output;
 }
};
}
int main(){
 for(auto side:{BoardChord::lb,BoardChord::rb})for(bool nollie:{false,true}) {
  AirRotation preparation; NativeInputFrame f{};f.valid=true;f.owner={1,2,3,4,5};
  f.owner_generation=f.contact_generation=f.action_generation=f.physics_step=1;f.state=103;
  auto sample=[&](std::uint64_t at,std::uint16_t buttons){f.sampled_at=at;return preparation.poll_preparation(at,buttons,true,f,f.state==103,nollie,false,SpinMode::held);};
  sample(1000,0);check(!sample(1017,side).owns_left,"prewind respects the existing chord window");
  check(sample(1138,side).owns_left,"fresh native crouch admits preparation");
  auto out=sample(1150,side);check(out.x==0&&out.y==0&&out.yaw_rate==0,"ground preparation reserves LS without delivering ground steering");
  out=sample(1198,side);check(out.x==0&&out.y==0,"held prewind cannot turn the grounded skater");
  check(!sample(1200,BoardChord::pair).owns_left,"board pair wins preparation ownership");
  sample(1217,0);f.state=100;check(!sample(1234,side).owns_left,"ordinary ground steering is untouched");
 }


 for(auto side:{BoardChord::lb,BoardChord::rb})for(unsigned cadence:{8u,17u,33u}) {
  AirRotation rotation;NativeInputFrame f{};f.valid=true;f.owner={1,2,3,4,5};
  f.owner_generation=f.action_generation=f.contact_generation=f.physics_step=1;
  rotation.poll_qualified(1000,0,false,false,true,SkatePose::riding,false,false,SpinMode::held,{},&f);
  f.airborne=true;f.state=200;f.airborne_episode=1;f.airborne_started_known=true;
  f.sampled_at=f.airborne_started_at=1000+cadence;
  const auto request=rotation.poll_qualified(f.sampled_at,side,false,false,true,SkatePose::air,false,false,SpinMode::held,{},&f);
  check(AirRotation::native_shoulder_output(request,SpinMode::held,side,true).x!=0,
        "fresh native AIR can consume held shoulders before custom body-rate readiness");
  f.valid=false;
  check(!rotation.poll_qualified(f.sampled_at+cadence,side,false,false,true,SkatePose::air,false,false,SpinMode::held,{},&f).owns_left,
        "decoupling the unused body writer never bypasses native AIR proof");
 }
 for(auto side:{BoardChord::lb,BoardChord::rb})for(bool allowed:{false,true})for(auto mode:{SpinMode::held,SpinMode::tap}) {
  AirRotation::Output request{true,0,0,side==BoardChord::lb?-7.75f:7.75f,{}};
  const auto delivered=AirRotation::native_shoulder_output(request,mode,side,allowed);
  const bool native=allowed&&mode==SpinMode::held;
  check(delivered.yaw_rate==(native?0:request.yaw_rate),"only admitted held shoulders switch actuator; tap rate remains intact");
  check(delivered.x==(native?(side==BoardChord::lb?-31783:31783):0),"native shoulder actuator preserves the established signed LS mapping");
 }
 check(!AirRotation::native_shoulder_output({},SpinMode::held,BoardChord::lb,true).owns_left,
       "actuator cannot create ownership or recover a rejected airborne request");

 for(unsigned cadence:{17u,34u})for(auto side:{BoardChord::lb,BoardChord::rb})
 for(const auto& choice:{std::pair<unsigned,unsigned>{0,2},{8,3},{2,4}}){
  AuthoredPipeline p;p.dt=static_cast<float>(cadence)/1000;
  p.poll(1000,0,100,-1,1);p.poll(1000+cadence);
  auto out=p.poll(1000+2*cadence,static_cast<std::uint16_t>(0x4000u|choice.first));
  if(!choice.first)out=p.poll(1000+3*cadence);
  check(out.requested_id==choice.second&&out.authored_playback,"real basic flip request starts its existing authored RS actor");
  check(p.facade.allows_air_rotation(0,out.authored_playback),"ordinary flip RS playback leaves independent AIR rotation eligible");
  const auto begin=1000+4*cadence;
  out=p.poll(begin,side);
  check(out.authored_playback&&!p.physical.angular&&p.delivered.x==(side==BoardChord::lb?-31783:31783)&&p.delivered.y==0,
   "held shoulder and actual flip playback compose through the original native physics proof");
  out=p.poll(begin+cadence,side,201,static_cast<int>(choice.second));
  check(out.authored_playback&&!p.physical.angular&&p.delivered.x!=0,"native LS spin coexists with the authored flip without a competing body rate");
  out=p.poll(begin+2*cadence,0,201,static_cast<int>(choice.second));
  check(out.authored_playback&&p.physical.spin_y==0&&p.delivered.x==0,"physical shoulder release stops native yaw while the flip actor retains RS");
  p.poll(begin+3*cadence,BoardChord::pair,201,static_cast<int>(choice.second));
  check(p.physical.spin_y==0&&p.delivered.x==0,"opposed shoulders during a flip select no angular direction");
  p.poll(begin+4*cadence,side,201,static_cast<int>(choice.second),2,{SpinFact::set,SpinFact::unknown,SpinFact::unknown});
  check(!p.physical.angular&&p.delivered.x==0,"known native spin prohibition still wins during authored playback");
  p.poll(begin+5*cadence,side,100,-1,1);
  check(!p.physical.angular&&p.delivered.x==0,"ground contact stops AIR rotation and does not become ordinary ground steering");
 }
 for(unsigned cadence:{17u,34u})for(auto side:{BoardChord::lb,BoardChord::rb})for(bool seen:{false,true}){
  Pipeline p;p.dt=static_cast<float>(cadence)/1000;p.observe(1000,100);p.publish(0);p.step();
  p.observe(1000+cadence,200);p.publish(side);check(p.step().angular,"actual producer/current held rotation reaches native motion");
  p.observe(1000+2*cadence,100);if(seen)p.publish(side);check(!p.step().angular,"native landing has no AIR rotation");
  p.observe(1000+3*cadence,200);check(!p.step().angular,"cached prior-flight intent rejected whether input saw landing or missed it");
  p.intent.sampled_at=p.body.now;check(!p.step().angular,"timestamp-only renewal cannot restore prior-flight authority");
  p.publish(side);const auto fresh=p.step();check(fresh.angular&&fresh.spin_y==(side==BoardChord::lb?-7.75f:7.75f),"fresh same-held publication resumes current flight without new press");
  p.observe(1000+4*cadence,200);p.publish(BoardChord::pair);check(p.step().spin_y==0,"nonadmitted opposed AIR shoulders select zero");
  p.observe(1000+5*cadence,200);p.publish(side);check(p.step().spin_y==(side==BoardChord::lb?-7.75f:7.75f),"remaining held shoulder immediately resumes when no board transition admitted");
  p.observe(1000+6*cadence,200);p.publish(side,true,false);check(p.step().spin_y==0,"missing native rate readiness emits zero, never legacy stick fallback");
  p.observe(1000+7*cadence,200);p.publish(side);check(p.step().angular,"current held tracking survives temporary rate-readiness loss");
  p.observe(1000+8*cadence,200);p.publish(0);check(p.step().spin_y==0,"physical zero release reaches native motion immediately");
 }
 for(unsigned fault=0;fault<6;++fault){
  Pipeline p;p.observe(1000,100);p.publish(0);p.step();p.observe(1017,200);p.publish(BoardChord::rb);check(p.step().angular,"lifecycle negative starts active");
  if(fault==0)++p.owner.rig;
  p.observe(1034,fault==3?700u:fault==4?dingosdk::game::build::v20260929::arcade_native_probe::offboard_state:200u);
  if(fault==1){p.facade.invalidate_authority();p.rotation.reset();}
  p.publish(static_cast<std::uint16_t>(BoardChord::rb|(fault==2?0x0010u:0)),fault!=4&&fault!=5);p.step();
  p.observe(1051,100);p.publish(BoardChord::rb);p.step();p.observe(1068,200);p.publish(BoardChord::rb);
  check(!p.step().angular,"owner/focus/menu/bail/actual offboard/OFF loss cannot rearm a held physical baseline");
 }
 {
  Pipeline p;p.observe(1000,100);p.publish(0);p.step();
  p.facade.poll(0,1000,0,0,0,0,0,true,false,false,0,false,true);
  p.observe(1017,100);
  const auto board=p.facade.poll(0,1017,BoardChord::pair,0,0,0,0,true,false,false,0,false,true);
  check((board.buttons&BoardChord::native_y)!=0,"actual native-ground facade admission reaches the existing board pulse adapter");
  p.publish(BoardChord::pair);p.step();
  p.observe(1034,dingosdk::game::build::v20260929::arcade_native_probe::offboard_state);p.publish(BoardChord::pair,false);p.step();
  p.observe(1051,100);p.publish(BoardChord::rb);p.step();p.observe(1068,200);p.publish(BoardChord::rb);
  check(!p.step().angular,"actual native offboard transition retires shoulder baseline after admitted board command");
 }
 // Actual board method requires qualified context; merely holding both in AIR
 // is not a transition. Native offboard observation above supplies lifecycle loss.
 BoardChord board;board.poll(0,0,true,true);check(board.poll(10,BoardChord::pair,true,true).pulse,"ground-qualified board command issues native pulse");
 BoardChord denied;denied.poll(0,0,true,false);check(!denied.poll(10,BoardChord::pair,true,false).pulse,"AIR pair cannot fabricate a board transition");
 std::cout<<checks<<" native spin/flight composition checks passed\n";
}
