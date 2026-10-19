#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* reason){++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}}
struct Capture {
 unsigned calls{};InputRequest last{};InputDisposition result{InputDisposition::started};
 static InputDisposition request(void* state,const InputRequest& request) noexcept {auto& self=*static_cast<Capture*>(state);++self.calls;self.last=request;return self.result;}
 InputAdapter adapter(){return {this,request};}
};
PhysicalInputSample direction(std::int16_t code,std::uint16_t buttons=0){return {buttons,static_cast<std::int16_t>(code==4?-25000:code==6?25000:0),static_cast<std::int16_t>(code==8?25000:code==2?-25000:0)};}
void active_session() {
 for(bool grab:{false,true}) {
  InputSession session;InputAdmission admission{};admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=true;
  admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;
  const std::array catalog{PreparedGesture{2,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};admission.catalog=catalog;
  session.poll(0,{},0,0,admission);session.poll(10,direction(4),0,0,admission);session.poll(20,{},0,0,admission);session.poll(30,direction(4),0,0,admission);
  const auto out=session.poll(40,direction(4,static_cast<std::uint16_t>(grab?0x2000:0x4000)),0,0,admission);
  check(out.unsupported && !out.authored_playback && !out.action.owned,"a recognized directional triple with no native mapping must not substitute a simpler ordinary Flip or Grab");
 }
}
void manual_prefix_reuse() {
 struct Receiver {
  unsigned air_calls{};InputRequest last{};
  static InputDisposition request(void* state,const InputRequest& request) noexcept {
   auto& self=*static_cast<Receiver*>(state);
   if(request.action==InputAction::flip || request.action==InputAction::grab){++self.air_calls;self.last=request;return InputDisposition::started;}
   return InputDisposition::waiting;
  }
 };
 for(bool grab:{false,true})for(bool repeat:{false,true})for(bool unrelated:{false,true}) {
  UnifiedInput input;Receiver receiver;const InputAdapter adapter{&receiver,Receiver::request};InputEnvironment environment{InputContext::air,true};
  environment.policy=InputPolicy::strict();environment.manual_safe=false;environment.manual_prepare=true;
  input.poll(0,{},environment,adapter);input.poll(10,direction(8),environment,adapter);input.poll(100,direction(2),environment,adapter);
  bool manual_only{};for(const auto& event:input.history().events())if(event.token==InputToken{InputSource::direction,2,InputEdge::press})manual_only=event.owners==InputConsumption::manual;
  check(manual_only,"the actual preparation-only Manual path marks the canonical prefix without owning native playback");
  input.poll(110,{},environment,adapter);
  if(unrelated)input.poll(112,{0x200},environment,adapter);
  if(repeat)input.poll(115,direction(2),environment,adapter);
  input.poll(120,direction(repeat?2:0,static_cast<std::uint16_t>(grab?0x2000:0x4000)),environment,adapter);
  const bool expected=repeat || !unrelated;
  bool face_unconsumed{};for(const auto& event:input.history().events())if(event.token.source==InputSource::buttons && event.token.code==(grab?13:14) && event.at==120)face_unconsumed=event.owners==0;
  if(!expected)check(face_unconsumed,"a failed category/adjacency query leaves the fresh face footprint unconsumed");
  check(receiver.air_calls==(expected?1u:0u) && (!expected || receiver.last.form==(repeat?InputForm::repeated_direction:InputForm::ordinary)),
   "regular air definitions reuse a Manual-only prefix; real intervening presses reject the adjacency predicate but not an ordered triple");
 }
}
void session_manual_prefix() {
 for(bool grab:{false,true})for(bool unrelated:{false,true}) {
  InputSession session;InputAdmission admission{};admission.gameplay=admission.manual_prepare=admission.action_fresh=true;
  admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;admission.input_policy=InputPolicy::strict();
  session.poll(0,{},0,0,admission);session.poll(10,direction(8),0,0,admission);const auto queued=session.poll(100,direction(2),0,0,admission);
  check(!queued.manual.owns && session.pending_manual().recognized_at==100,"the real session queues a Manual without native ownership before the category-sharing trace");
  session.poll(110,{},0,0,admission);if(unrelated)session.poll(115,{0x200},0,0,admission);
  const auto out=session.poll(120,{static_cast<std::uint16_t>(grab?0x2000:0x4000)},0,0,admission);
  bool dispatched{};const auto& diagnostic=session.diagnostic();for(std::size_t i=0;i<diagnostic.dispatched;++i)
   dispatched=dispatched || diagnostic.dispatch[i].request.action==(grab?InputAction::grab:InputAction::flip);
  check(dispatched==!unrelated && !out.action.owned && !out.authored_playback,
   "the actual Session caller reuses Manual-only history but never weakens unrelated-press or native-capability negatives");
 }
}
void other_category_prefixes() {
 for(unsigned owner:{InputConsumption::regular,InputConsumption::extra,InputConsumption::extra_grind})for(bool held:{false,true}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{InputContext::air,true};environment.policy=InputPolicy::strict();environment.named_grabs=true;
  const std::int16_t prefix = owner==InputConsumption::extra ? 6 : 8;
  if(owner==InputConsumption::extra) {
   environment.context=InputContext::balance;
   input.poll(0,{},environment,capture.adapter());input.poll(10,direction(4),environment,capture.adapter());
   input.poll(20,direction(6),environment,capture.adapter());input.poll(30,{0x4000},environment,capture.adapter());
   environment.context=InputContext::air;input.poll(200,direction(held?6:0),environment,capture.adapter());
  } else {
  input.poll(0,{},environment,capture.adapter());input.poll(10,direction(8),environment,capture.adapter());
  if(owner==InputConsumption::extra_grind){input.poll(20,{},environment,capture.adapter());input.poll(30,direction(8),environment,capture.adapter());input.poll(40,direction(8,0x8000),environment,capture.adapter());}
  else {
   input.poll(20,direction(8,0x4000),environment,capture.adapter());
  }
  }
  bool owned{};for(const auto& event:input.history().events())if(event.token==InputToken{InputSource::direction,prefix,InputEdge::press})owned=owned || event.owners==owner;
  check(owned,"the actual dispatcher establishes the non-Manual prefix category used by this negative control");
  if(owner!=InputConsumption::extra){input.poll(55,direction(8),environment,capture.adapter());input.poll(60,direction(held?8:0),environment,capture.adapter());}
  const auto calls=capture.calls;
  input.poll(owner==InputConsumption::extra?210:70,direction(held?prefix:0,0x2000),environment,capture.adapter());
  if(capture.calls!=calls+(held?1u:0u)) std::cerr<<"category="<<owner<<" held="<<held<<" calls="<<calls<<" actual="<<capture.calls<<"\n";
  check(capture.calls==calls+(held?1u:0u) && (!held || capture.last.action==InputAction::grab),
   "regular/extra/extra-grind consumed released prefixes stay blocked; separately defined held-direction reuse remains allowed");
 }
}
void prefix_boundaries() {
 for(bool grab:{false,true})for(std::int16_t code:{std::int16_t{4},std::int16_t{6},std::int16_t{8},std::int16_t{2}})
 for(unsigned age:{299u,300u,301u})for(bool release_first:{false,true}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{InputContext::air,true};
  input.poll(0,{},environment,capture.adapter());input.poll(10,direction(code),environment,capture.adapter());
  if(release_first){input.poll(20,{},environment,capture.adapter());input.poll(30,direction(code),environment,capture.adapter());}
  input.poll(10+age,direction(code,static_cast<std::uint16_t>(grab?0x2000:0x4000)),environment,capture.adapter());
  check(capture.calls==1 && (capture.last.form==InputForm::repeated_direction)==(release_first && age<300),
   "a distinct directional triple has a strict300ms first-edge age; held directions and expired triples remain ordinary actions");
  if(capture.last.form==InputForm::repeated_direction)check(capture.last.direction==code && capture.last.presses==1 && !capture.last.direction_inherited &&
   capture.last.first_input_at==10 && capture.last.last_input_at==10+age,
   "directional repetition retains the exact prefix footprint and is distinct from face-button repetition");
 }
}
void sampled_prefixes() {
 for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)for(bool grab:{false,true})
 for(std::int16_t code:{std::int16_t{4},std::int16_t{6},std::int16_t{8},std::int16_t{2}})
 for(bool pad:{false,true})for(bool repeated:{false,true})for(bool intervening:{false,true}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{InputContext::air,true};
  std::uint64_t first{};
  for(unsigned now=phase;now<600;now+=cadence) {
   std::int16_t selected{};
   if(now>=100 && (!repeated || now<150))selected=code;
   if(repeated && now>=200 && now<240)selected=code;
   if(intervening && repeated && now>=240 && now<280)selected=code==6?4:6;
   if(!first && selected==code)first=now;
   std::uint16_t buttons=now>=300 && now<370?static_cast<std::uint16_t>(grab?0x2000:0x4000):std::uint16_t{0};
   if(intervening && now>=170 && now<210)buttons=static_cast<std::uint16_t>(buttons|0x100u);
   auto physical=direction(selected,buttons);
   if(pad){physical.x=physical.y=0;physical.buttons=static_cast<std::uint16_t>(physical.buttons|(selected==4?4u:selected==6?8u:selected==8?1u:selected==2?2u:0u));}
   environment.native_context={true,true,{1,1,1},now+1,now,false};
   input.poll(now,physical,environment,capture.adapter());
  }
  check(capture.calls==1 && (capture.last.form==InputForm::repeated_direction)==repeated && capture.last.direction==code,
   "every30/60Hz sample phase selects the triple before its base action, allows unrelated travel, and rejects held-only accidental repeats");
  unsigned regular{};bool unrelated_available=true;
  for(const auto& event:input.history().events()) {
   if(event.owners&InputConsumption::regular)++regular;
   if(event.token.source==InputSource::buttons && event.token.code==8)unrelated_available=unrelated_available && event.owners==0;
  }
  check(regular==(repeated?4u:2u) && unrelated_available,
   "the winning definition consumes exactly its four or two physical events and leaves unrelated inputs available");
  if(repeated)check(capture.last.first_input_at==first && capture.last.recognized_at>=300 && capture.last.recognized_at<300+cadence,
   "sampled prefix age begins at the actual first physical press, while queue recognition remains a distinct timestamp");
 }
}
void ground_prefixes() {
 for(unsigned age:{332u,333u,334u})for(bool reverse:{false,true})for(bool first_release:{false,true})for(bool ground:{false,true}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{ground?InputContext::ground:InputContext::air,true};
  input.poll(0,{},environment,capture.adapter());input.poll(10,direction(2),environment,capture.adapter());
  if(first_release)input.poll(20,{},environment,capture.adapter());
  if(reverse){input.poll(30,direction(first_release?0:2,0x200),environment,capture.adapter());input.poll(10+age,direction(2,0x200),environment,capture.adapter());}
  else{input.poll(30,direction(2),environment,capture.adapter());input.poll(10+age,direction(2,0x200),environment,capture.adapter());}
  const bool expected=ground && first_release && age<333;
  check(capture.calls==(expected?1u:0u),"ground directional turn requires the released first press, either order of the final pair, and a strict333ms age");
  if(expected)check(capture.last.action==InputAction::ground_turn && capture.last.direction==2 && capture.last.variant==1 && capture.last.control==0x200 &&
   capture.last.first_input_at==10 && capture.last.last_input_at==10+age,"ground triple emits its exact semantic destination and footprint without a native trick identity");
 }
 for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)for(bool reverse:{false,true})for(bool pad:{false,true})for(bool release_first:{false,true}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{InputContext::ground,true};
  for(unsigned now=phase;now<600;now+=cadence) {
   const bool down=now>=100 && (!release_first || now<150 || now>=(reverse?250u:200u));
   std::uint16_t buttons=now>=(reverse?200u:250u) && now<400?0x200:0;
   if(now>=170 && now<300)buttons=static_cast<std::uint16_t>(buttons|0x80u);
   auto physical=direction(down?2:0,buttons);if(pad){physical.y=0;physical.buttons=static_cast<std::uint16_t>(physical.buttons|(down?2u:0u));}
   environment.native_context={true,true,{1,1,1},now+1,now,false};input.poll(now,physical,environment,capture.adapter());
  }
  check(capture.calls==(release_first?1u:0u),"ground triple uses distinct physical edges at every30/60Hz phase, permits an unrelated held control, and never repeats from holding alone");
  unsigned footprint{};bool unrelated_free=true;
  for(const auto& event:input.history().events()){
   if(event.owners&InputConsumption::regular)++footprint;
   if(event.token.source==InputSource::buttons && event.token.code==7)unrelated_free=unrelated_free && event.owners==0;
  }
  check(footprint==(release_first?4u:0u) && unrelated_free,"ground query commits only its successful four-event footprint and leaves accidental-trigger negatives unconsumed");
 }
 InputSession session;InputAdmission admission{};admission.gameplay=true;admission.pose=SkatePose::riding;
 session.poll(0,{},0,0,admission);session.poll(10,direction(2),0,0,admission);session.poll(20,{},0,0,admission);session.poll(30,direction(2),0,0,admission);
 const auto missing=session.poll(40,direction(2,0x200),0,0,admission);
 check(missing.unsupported && !missing.action.owned && !missing.authored_playback,"missing native ground-turn capability reports unsupported without executing movement or a substitute jump");
}
void simultaneous_final_pair() {
 for(InputAction action:{InputAction::flip,InputAction::grab,InputAction::ground_turn}) {
  UnifiedInput input;Capture capture;InputEnvironment environment{action==InputAction::ground_turn?InputContext::ground:InputContext::air,true};
  const std::int16_t code=action==InputAction::ground_turn?2:4;
  const std::uint16_t terminal=action==InputAction::flip?0x4000:action==InputAction::grab?0x2000:0x200;
  input.poll(0,{},environment,capture.adapter());input.poll(10,direction(code),environment,capture.adapter());input.poll(20,{},environment,capture.adapter());
  input.poll(30,direction(code,terminal),environment,capture.adapter());
  check(capture.calls==1 && capture.last.action==action && capture.last.first_input_at==10 && capture.last.last_input_at==30,
   "a simultaneous second direction and terminal uses the unordered final pair and dispatches exactly once");
  input.poll(40,direction(code,terminal),environment,capture.adapter());check(capture.calls==1,"holding a simultaneous completion cannot replay its consumed footprint");
 }
}
void extra_priority_and_waiting() {
 UnifiedInput input;Capture capture;InputEnvironment environment{InputContext::air,true};
 input.poll(0,{},environment,capture.adapter());input.poll(10,direction(4),environment,capture.adapter());input.poll(20,direction(4,0x4000),environment,capture.adapter());
 input.poll(30,{},environment,capture.adapter());input.poll(40,direction(4),environment,capture.adapter());input.poll(50,{},environment,capture.adapter());
 input.poll(60,direction(4),environment,capture.adapter());input.poll(70,direction(4,0x4000),environment,capture.adapter());
 check(capture.calls==2 && capture.last.form==InputForm::ordinary && capture.last.presses==2,
  "an installed parent extra table has priority over an otherwise complete ordinary directional triple");
 for(bool landed:{false,true}) {
  UnifiedInput queued;Capture sink;sink.result=InputDisposition::waiting;InputEnvironment e{InputContext::air,true};
  queued.poll(0,{},e,sink.adapter());queued.poll(10,direction(4),e,sink.adapter());queued.poll(20,{},e,sink.adapter());queued.poll(30,direction(4),e,sink.adapter());
  queued.poll(40,direction(4,0x4000),e,sink.adapter());queued.poll(50,{},e,sink.adapter());const auto calls=sink.calls;
  sink.result=InputDisposition::started;if(landed)e.context=InputContext::ground;queued.poll(1000,{},e,sink.adapter());
  check(sink.calls==calls+(landed?0u:1u) && (landed || (sink.last.form==InputForm::repeated_direction && sink.last.recognized_at==40)),
   "a directional triple retains its identity and timestamp across release/readiness delay; landing cancels it before native dispatch");
 }
}
void recognition_observation_lineage() {
 for(bool grab:{false,true})for(const std::uint64_t cadence:{17u,34u}) {
  UnifiedInput input; input.set_recognition_capture(true); Capture capture; InputEnvironment environment{InputContext::air,true};
  environment.named_grabs=true;
  const auto poll=[&](std::uint64_t now,PhysicalInputSample physical,std::uint64_t action) {
   environment.native_context={true,true,{1,1,action},now+1,now,false};
   environment.native_context.airborne=true;environment.native_context.airborne_episode=1;environment.native_context.air_departure=1;
   input.poll(now,physical,environment,capture.adapter());
  };
  const auto face=static_cast<std::uint16_t>(grab?0x2000:0x4000);
  poll(0,{},1);poll(cadence,direction(4),1);poll(2*cadence,direction(4,face),1);
  check(input.diagnostic().recognition_count==1 && capture.calls==1,
   "semantic parent starts with its own actual observation proof");
  const auto parent=input.diagnostic().recognition[0].request;
  poll(3*cadence,direction(4),2);poll(4*cadence,direction(4,face),2);
  const auto& diagnostic=input.diagnostic();
  check(diagnostic.recognition_count==1,"extra face produces its own attempt at30/60Hz");
  const auto& child=diagnostic.recognition[0].request;
  check(child.presses==2 && child.generations==parent.generations && child.generations.action==1 &&
   child.recognition->observation==environment.native_context && child.recognition->observation.generations.action==2 &&
   child.recognition->observation.sampled_at==4*cadence && child.recognition->observation.physics_step==4*cadence+1 &&
   child.recognition->observation_known && child.recognition->sequence!=parent.recognition->sequence,
   "extra Flip and Grab preserve inherited semantic lineage separately from real newer native observation proof");
  check(diagnostic.dispatched==1 && diagnostic.dispatch[0].disposition==InputDisposition::cancelled && capture.calls==1 &&
   diagnostic.dispatch[0].request.recognition==child.recognition,
   "unchanged generation eligibility cancels the child while preserving exact diagnostic proof");
 }
}

}
int main(){recognition_observation_lineage();active_session();manual_prefix_reuse();session_manual_prefix();other_category_prefixes();prefix_boundaries();sampled_prefixes();ground_prefixes();simultaneous_final_pair();extra_priority_and_waiting();std::cout<<checks<<" pattern dispatcher checks passed\n";}
