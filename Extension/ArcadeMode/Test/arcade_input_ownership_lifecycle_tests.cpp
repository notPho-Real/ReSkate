#include "Extension/ArcadeMode/arcade_gesture.h"
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{},errors{};
void check(bool value,const char* reason){++checks;if(!value){++errors;if(errors<20)std::cerr<<"FAIL: "<<reason<<'\n';}}
struct Harness {
 InputSession session;InputAdmission env{};std::uint64_t step{},episode{},departure{},owner{1};bool observed_air{};
 std::vector<PreparedGesture> catalog{
  PreparedGesture{1,false,GestureContext::ground,{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
  PreparedGesture{2,false,GestureContext::ground,{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
 Harness(bool exported,bool bound,SkatePose pose){env.gameplay=env.manual_safe=env.action_fresh=env.inspected_stance=true;env.action_mode=ActionExperiment::classic;env.export_rail_requests=exported;env.pose=pose;env.native_context.required=bound;env.catalog=catalog;env.authored=env.may_start=true;}
 NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample physical={}){
  env.tick=now/17;
  if(env.native_context.required){
   const bool air=env.pose==SkatePose::air;if(air&&!observed_air){++episode;departure=now;}observed_air=air;
   env.native_context={true,true,{owner,1,1},++step,now,false,air,episode,departure};
  }
  return session.poll(now,physical,0,0,env);
 }
};
void interrupted_rail(){
 for(std::uint64_t cadence:{17u,34u})for(bool exported:{false,true})for(bool bound:{false,true})for(int direction:{-1,0,1}){
  Harness h(exported,bound,SkatePose::air);h.poll(1000);const PhysicalInputSample held{0x8000,0,static_cast<std::int16_t>(direction*24000)};
  auto out=h.poll(1000+cadence,held);const auto original=out.rail.request;
  check(h.session.busy()&&(exported?out.rail.held:out.phase==1),"fresh Y reaches the existing rail request endpoint");
  h.env.external_owner=true;out=h.poll(1000+2*cadence,held);
  check(!out.rail.held&&!out.phase&&!h.session.busy(),"external ownership cancels rail intent and synthetic ownership immediately");
  h.env.external_owner=false;
  for(unsigned n=3;n<6;++n){out=h.poll(1000+n*cadence,held);check(!out.rail.held&&!out.phase&&!h.session.busy(),"held Y through an owner conflict cannot resurrect an old rail request");}
  h.poll(1000+6*cadence);out=h.poll(1000+7*cadence,held);
  check(h.session.busy()&&(exported?out.rail.held:out.phase==1),"actual Y release/press rearms the existing rail endpoint");
  if(exported)check(out.rail.request.physical_pressed_at==1000+7*cadence&&out.rail.request.input_sequence!=original.input_sequence,"rearmed rail request gets its actual new edge, not a retimed old ticket");
 }
}
void cancelled_rail_cannot_starve_auxiliary(){
 for(std::uint64_t cadence:{17u,34u})for(bool exported:{false,true})for(bool bound:{false,true})for(bool grab:{false,true}){
  Harness h(exported,bound,grab?SkatePose::air:SkatePose::riding);h.poll(1000);h.poll(1000+cadence,{0x8000});
  h.env.external_owner=true;h.poll(1000+2*cadence,{0x8000});h.env.external_owner=false;
  const PhysicalInputSample next{static_cast<std::uint16_t>(grab?0xa000u:0x8000u),0,0,0,static_cast<std::uint8_t>(grab?0:255)};
  const auto out=h.poll(1000+3*cadence,next);
  check(out.action.owned&&out.action.result==DispatchResult::started,"cancelled held rail intent cannot starve a fresh supported grab/revert request");
  check(out.action.request_kind==(grab?1u:3u),"only the newly pressed supported auxiliary endpoint owns output");
  check(!out.rail.held,"auxiliary execution cannot republish a cancelled rail ticket");
 }
}
void pending_handoff_preserves_release_owner(){
 for(std::uint64_t cadence:{17u,34u})for(bool exported:{false,true})for(bool bound:{false,true}){
  Harness h(exported,bound,SkatePose::air);h.session.set_recognition_capture(true);h.poll(1000);
  const auto grab=h.poll(1000+cadence,{0x2000});check(grab.action.owned,"supported held grab establishes its existing channel owner");
  const auto first_y=1000+2*cadence;auto out=h.poll(first_y,{0x8000});
  check(out.action.owned&&(!exported||(out.rail.held&&out.rail.blocked&&out.rail.occupied.owns_right_stick)),"unstarted handoff remains explicitly blocked while the existing grab neutral tail owns its channels");
  InputRequest original{};bool queued{};
  // Recognition capture retains the original request independently of first
  // native issue, including a blocked exported handoff.
  for(std::size_t i=0;i<h.session.diagnostic().recognition_count;++i){const auto& e=h.session.diagnostic().recognition[i];if(e.request.action==InputAction::rail_request){original=e.request;queued=e.queued;}}
  check(queued&&original.action==InputAction::rail_request&&original.physical_pressed_at==first_y&&original.deadline==first_y+InputTiming::readiness_ms,"blocked preissue handoff retains the original bounded rail request");
  for(unsigned n=3;n<=5;++n){out=h.poll(1000+n*cadence,{0x8000});if(exported&&out.rail.held)break;}
  check(exported?out.rail.held:out.phase==1,"rail dispatch follows the existing34ms grab neutral tail without a new Y edge");
  if(exported)check(out.rail.request.physical_pressed_at==first_y&&out.rail.request.deadline==original.deadline&&out.rail.request.input_sequence==original.input_sequence,"delayed handoff keeps original edge identity and deadline");
 }
}
void reset_and_rearm(){
 for(std::uint64_t cadence:{17u,34u})for(bool bound:{false,true})for(unsigned actor=0;actor<5;++actor)for(unsigned boundary=0;boundary<3;++boundary){
  if(boundary==2&&!bound)continue;
  Harness h(true,bound,actor==0||actor==1||actor==4?SkatePose::air:SkatePose::riding);
  const PhysicalInputSample commands[]{{0x8000},{0x2000},{0,0,0,0,255},{0x1000},{0x4000,-24000,0}};
  const auto physical=commands[actor];h.poll(1000);auto out=h.poll(1000+cadence,physical);
  check(out.command_busy,"supported rail/grab/revert/Ollie/flip command starts before reset boundary");
  if(boundary==0){
   h.env.gameplay=false;h.env.tick=(1000+2*cadence)/17;
   const auto restored=h.session.poll(1000+2*cadence,physical,1234,-2345,h.env);
   check(restored.buttons==physical.buttons&&restored.right_x==1234&&restored.right_y==-2345,"OFF returns the exact physical packet channels while retiring command state");
   h.env.gameplay=true;
  }else if(boundary==1){
   auto menu=physical;menu.buttons|=0x10;h.poll(1000+2*cadence,menu);
  }else{
   ++h.owner;out=h.poll(1000+2*cadence,physical);
   check(!out.command_busy&&!out.rail.held&&!out.action.owned&&!out.authored_playback,"new native owner retires every existing input actor without reusing held commands");
  }
  out=h.poll(1000+3*cadence,physical);
  check(!out.command_busy&&!out.rail.held&&!out.action.owned&&!out.authored_playback,"held controls through OFF/menu/owner reset stay baselined without new commands");
  h.poll(1000+4*cadence);out=h.poll(1000+5*cadence,physical);
  check(out.command_busy,"actual release and fresh press rearm the existing supported endpoint after reset");
 }
}
void air_context_cancels_supported_actor(){
 for(std::uint64_t cadence:{17u,34u})for(bool bound:{false,true})for(bool flip:{false,true}){
  Harness h(true,bound,SkatePose::air);h.poll(1000);const PhysicalInputSample held{static_cast<std::uint16_t>(flip?0x4000u:0x2000u),static_cast<std::int16_t>(flip?-24000:0),0};
  auto out=h.poll(1000+cadence,held);check(flip?out.authored_playback:out.action.owned,"supported air actor starts before context departure");
  h.env.pose=SkatePose::riding;out=h.poll(1000+2*cadence,held);
  check(!out.authored_playback&&!out.action.owned,"actual ground context withdraws airborne flip/grab ownership");
  h.env.pose=SkatePose::air;out=h.poll(1000+3*cadence,held);
  check(!out.authored_playback&&!out.action.owned,"held face through a new flight does not restart an old actor");
  h.poll(1000+4*cadence);out=h.poll(1000+5*cadence,held);
  check(flip?out.authored_playback:out.action.owned,"fresh release/press in the new flight rearms an existing supported air endpoint");
 }
}

struct BoundPipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true};
 NativeGesture::Output out{};RailAdapterPad pad{};
 BoundPipeline(){input.enable(true);}
 void poll(std::uint64_t now,std::uint16_t buttons,bool air=false,std::uint8_t rt=0,bool compatible=true){
  NativeRequestSample physical{};physical.valid=true;physical.state=air?200u:100u;physical.crouch_mode=1;physical.words[1]=air?0u:0xc000u;
  const auto frame=producer.observe(now,owner,physical,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
  out=input.poll(0,now,buttons,0,0,0,0,true,false,false,0,false,false,false,now/17,false,{},false,false,
   air?SkatePose::air:SkatePose::riding,false,false,false,false,true,false,0,rt,ActionExperiment::classic,true,true,false);
  pad=input.sample_native_rail(out.rail,now,compatible);
 }
};
void unissued_rail_expiry_releases_input_owner(){
 for(std::uint64_t cadence:{17u,34u}){
  BoundPipeline p;p.poll(1000,0);const auto first=1000+cadence;p.poll(first,0x8000);const auto original=p.out.rail.request;
  check(p.input.busy(0)&&p.out.rail.held&&!p.pad.owns_right_stick,"ground Y is pending input intent and not native rail actuation");
  for(auto now=first+cadence;now<first+199;now+=cadence)p.poll(now,0x8000);
  p.poll(first+199,0x8000);check(p.out.rail.held&&p.out.rail.request==original,"unissued ticket remains pending until its original exclusive readiness deadline");
  p.poll(first+200,0x8000,false,255);
  check(!p.out.rail.held&&p.out.rail.request==original,"expired unissued input owner emits one original terminal ticket without claiming physical release");
  check(p.out.action.owned&&p.out.action.request_kind==3,"expired unissued Y cannot starve a fresh supported RT revert at the same sample");
  check(!p.pad.owns_right_stick,"expiry never fabricates native rail readiness");
  p.poll(first+201,0x8000,false,255);check(p.out.rail.request.action==InputAction::none,"terminal expired handoff is not republished indefinitely");
  p.poll(first+250,0x8000,true);check(!p.out.rail.held&&!p.pad.owns_right_stick,"entering AIR cannot revive expired held Y");
  p.poll(first+251,0,true);p.poll(first+252,0x8000,true);
  check(p.pad.owns_right_stick&&p.out.rail.request.input_sequence!=original.input_sequence&&p.out.rail.request.physical_pressed_at==first+252,"fresh physical Y release/press rearms the existing native method after expiry");
 }
}
void admitted_rail_has_no_input_timeout(){
 for(std::uint64_t cadence:{17u,34u}){
  BoundPipeline p;p.poll(1000,0,true);const auto first=1000+cadence;p.poll(first,0x8000,true,0,false);const auto original=p.out.rail.request;
  check(!p.pad.owns_right_stick&&p.out.rail.held,"blocked preissue native handoff retains legitimate pending intent");
  for(auto now=first+cadence;now<first+199;now+=cadence)p.poll(now,0x8000,true,0,false);
  p.poll(first+199,0x8000,true);check(p.pad.owns_right_stick&&p.out.rail.request==original,"pending first issue can still occur within the original deadline without a new edge");
  for(auto now=first+200;now<first+500;now+=cadence){p.poll(now,0x8000,true);check(p.pad.owns_right_stick&&p.input.busy(0)&&p.out.rail.request==original,"accepted native method is not truncated by the input first-issue deadline");}
 }
}

void sampled_owner_handoff(){
 for(std::uint64_t cadence:{17u,34u})for(std::uint64_t phase=0;phase<cadence;++phase)for(bool grab:{false,true}){
  Harness h(true,true,grab?SkatePose::air:SkatePose::riding);unsigned starts{};
  for(std::uint64_t now=0;now<350;now+=cadence){
   h.env.external_owner=now>=100+phase&&now<150+phase;
   const bool rail=now>=20+phase,aux=now>=200+phase&&now<250+phase;
   const PhysicalInputSample physical{static_cast<std::uint16_t>((rail?0x8000u:0u)|(grab&&aux?0x2000u:0u)),0,0,0,static_cast<std::uint8_t>(!grab&&aux?255:0)};
   const auto out=h.poll(now,physical);
   if(now>=100+phase)check(!out.rail.held,"sampled held rail intent cannot cross an observed owner conflict at any30/60Hz phase");
   if(out.action.requested&&out.action.result==DispatchResult::started)++starts;
  }
  check(starts==1,"one fresh supported grab/revert edge executes after the sampled conflict without stale Y starvation");
 }
}

}
int main(){interrupted_rail();cancelled_rail_cannot_starve_auxiliary();pending_handoff_preserves_release_owner();reset_and_rearm();air_context_cancels_supported_actor();unissued_rail_expiry_releases_input_owner();admitted_rail_has_no_input_timeout();sampled_owner_handoff();std::cout<<checks<<" ownership lifecycle checks, "<<errors<<" failed\n";return errors?1:0;}
