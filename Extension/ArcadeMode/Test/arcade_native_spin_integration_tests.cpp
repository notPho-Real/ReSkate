#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
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
}
int main(){
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
