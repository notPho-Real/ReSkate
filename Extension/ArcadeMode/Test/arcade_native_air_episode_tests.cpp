#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* reason){++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true};NativeInputFrame frame{};
 const std::array<PreparedGesture,1> catalog{PreparedGesture{2,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
 Pipeline(){input.enable(true);}
 void observe(std::uint64_t now,unsigned state=200,NativeFlipSelection selection={true,-1,55}){
  NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;sample.words[1]=state==100?0xc000u:0u;
  frame=producer.observe(now,owner,sample,selection);input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
 }
 NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons=0,bool ready=true,std::int16_t x=-25000,std::int16_t y=0,bool safe=false,bool prepare=false){
  return input.poll(0,now,buttons,x,y,0,0,true,false,false,0,false,false,true,now/17,false,catalog,false,safe,SkatePose::air,
   false,prepare,false,false,ready,false,0,0,ActionExperiment::classic,true,true);
 }
};
void active(){
 for(bool grab:{false,true})for(unsigned change=0;change<3;++change){
  Pipeline p;p.observe(0);p.poll(0);p.observe(10);const std::uint16_t button=grab?0x2000:0x4000;
  const auto first=p.poll(10,button);check(grab?first.action.owned:first.authored_playback,"actual bound producer/facade delivers the old action before the boundary");
  const auto epoch=InputNativeContext::from_frame(p.frame).airborne_episode;
  if(change==0){p.observe(20,100);p.observe(30,200);p.observe(40,200);}
  if(change==1){p.observe(20,201);p.observe(30,200);p.observe(40,200);}
  if(change==2){const NativeFlipSelection ack{true,grab?-1:2,77};p.observe(20,200,ack);p.observe(30,200,ack);p.observe(40,200,ack);}
  const auto after=p.poll(40,button);const bool owned=grab?after.action.owned:after.authored_playback;
  check(owned==(change!=0),"actual producer departure releases old channels while continuous AIR and own selection acknowledgement preserve them");
  const auto next=InputNativeContext::from_frame(p.frame).airborne_episode;
  check(change==0?next>epoch:next==epoch,"only the actual physics episode transition advances the marker; input does not reconstruct or stamp it");
  if(change==0){p.observe(50);const auto held=p.poll(50,button);check(!held.action.owned && !held.authored_playback,"held-through input cannot restart in the new flight");p.observe(60);p.poll(60);p.observe(70);const auto rearmed=p.poll(70,button);check(grab?rearmed.action.owned:rearmed.authored_playback,"a new physical edge can rearm after the actual departure");}
 }
}
void release_tail(){
 for(bool departed:{false,true}){
  Pipeline p;p.observe(0);p.poll(0);p.observe(10);p.poll(10,0x2000);
  const auto released=p.poll(15);check(released.action.owned && released.action.pad.right_trigger==0,"actual already-active grab release owns neutral channels before the tail ends");
  if(departed){p.observe(20,100);p.observe(30,200);}else{p.observe(20,200);p.observe(30,200);}p.observe(40,200);
  const auto next=p.poll(40);check(next.action.owned==!departed,"actual active neutral tail cancels across departure but preserves its continuous-flight duration");
 }
}
void pending_and_manual(){
 Pipeline p;p.observe(0);p.poll(0);p.observe(10);const auto waiting=p.poll(10,0x4000,false);check(!waiting.authored_playback && waiting.command_busy,"actual pending control waits for readiness in its original flight");
 p.observe(20,100);p.observe(30,200);p.observe(40,200);const auto after=p.poll(40,0x4000);check(!after.authored_playback && !after.command_busy,"actual missed departure clears pending intent before readiness returns");
 Pipeline m;m.observe(0);m.poll(0,0,true,0,0,false,true);m.observe(10);m.poll(10,0,true,0,25000,false,true);m.observe(100);const auto prepared=m.poll(100,0,true,0,-25000,false,true);check(!prepared.manual.owns,"actual preparation-only Manual does not yet own channels");
 m.observe(120,100);const auto landed=m.poll(120,0,true,0,-25000,true,false);check(landed.manual.owns && landed.manual.request_at==120,"actual native departure preserves prepared Manual and starts the separate native settling clock");
}
}
int main(){Pipeline capability;capability.observe(0);if(!InputNativeContext::from_frame(capability.frame).airborne_episode){std::cerr<<"BLOCKED: immutable native producer does not publish airborne episode/departure\n";return 3;}active();release_tail();pending_and_manual();std::cout<<checks<<" actual producer/bound facade/air episode checks passed\n";}
