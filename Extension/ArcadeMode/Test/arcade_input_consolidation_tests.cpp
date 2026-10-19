#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* reason){++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}}
struct Receiver {
 InputRequest last{};unsigned calls{};
 static InputDisposition receive(void* state,const InputRequest& request) noexcept{auto& self=*static_cast<Receiver*>(state);self.last=request;++self.calls;return InputDisposition::started;}
 InputAdapter adapter(){return {this,receive};}
};
void physical_footprints() {
 for(unsigned family=0;family<5;++family) {
  UnifiedInput input;Receiver receiver;InputEnvironment environment{family==0?InputContext::air:family==3?InputContext::lip:family==4?InputContext::ground:InputContext::rail,true};
  if(family==0)environment.surface={1,1,true,true,false,false,0};
  if(family==1)environment.rail={InputRailPhase::early,1,0,0,0,0,true,true,true};
  if(family==2)environment.rail={InputRailPhase::established,1,0,0,0,0,true,true,true};
  if(family==3)environment.rail={InputRailPhase::lip_established,1,0,0,0,0,true,true,true};
  input.poll(0,{},environment,receiver.adapter());
  if(family==0){environment.surface.physics_step=2;input.poll(10,{0,0,-25000},environment,receiver.adapter());environment.surface.physics_step=3;input.poll(20,{0x1000,0,-25000},environment,receiver.adapter());}
  if(family==1){input.poll(10,{0,25000,25000},environment,receiver.adapter());input.poll(20,{0x8000,25000,25000},environment,receiver.adapter());input.poll(30,{0,25000,25000},environment,receiver.adapter());input.poll(40,{0x8000,25000,25000},environment,receiver.adapter());}
  if(family==2){input.poll(10,{0x8000},environment,receiver.adapter());input.poll(20,{},environment,receiver.adapter());input.poll(30,{0x4000},environment,receiver.adapter());}
  if(family==3){input.poll(10,{0x2000},environment,receiver.adapter());input.poll(20,{},environment,receiver.adapter());input.poll(30,{0x2000},environment,receiver.adapter());}
  if(family==4){input.poll(10,{0,0,25000},environment,receiver.adapter());input.poll(20,{},environment,receiver.adapter());input.poll(30,{0,0,25000},environment,receiver.adapter());input.poll(40,{0x8000,0,25000},environment,receiver.adapter());input.poll(50,{},environment,receiver.adapter());}
  const auto expected=family==0?InputAction::wall_command:family==3?InputAction::lip_change:family==4?InputAction::rail_select:InputAction::rail_change;
  const std::uint64_t last=family==0?20:family==1 || family==4?40:30;
  check(receiver.last.action==expected && receiver.last.first_input_at==10 && receiver.last.last_input_at==last && receiver.last.recognized_at==last,
   "live context consumers retain physical match endpoints separately from recognition and adapter readiness");
  for(const auto& event:input.history().events())check(event.token.source!=InputSource::stick && event.token.source!=InputSource::pad,
   "general production direction history is canonical; device metadata is not a second command history");
 }
}
void expired_context_patterns() {
 struct Target {
  InputAction action;unsigned calls{};
  static InputDisposition receive(void* state,const InputRequest& request) noexcept{auto& self=*static_cast<Target*>(state);if(request.action==self.action)++self.calls;return InputDisposition::unsupported;}
 };
 for(unsigned family=0;family<5;++family) {
  Target target{family==0?InputAction::wall_command:family==3?InputAction::lip_change:family==4?InputAction::rail_select:InputAction::rail_change};
  const InputAdapter adapter{&target,Target::receive};UnifiedInput input;
  InputEnvironment environment{family==0?InputContext::air:family==3?InputContext::lip:family==4?InputContext::ground:InputContext::rail,true};
  if(family==0)environment.surface={1,1,true,true,false,false,0};
  if(family==1)environment.rail={InputRailPhase::early,1,0,0,0,0,true,true,true};
  if(family==2)environment.rail={InputRailPhase::established,1,0,0,0,0,true,true,true};
  if(family==3)environment.rail={InputRailPhase::lip_established,1,0,0,0,0,true,true,true};
  input.poll(0,{},environment,adapter);
  if(family==0){environment.surface.physics_step=2;input.poll(10,{0,0,-25000},environment,adapter);environment.surface.physics_step=3;input.poll(461,{0x1000,0,-25000},environment,adapter);}
  if(family==1){input.poll(10,{0,25000,25000},environment,adapter);input.poll(20,{0x8000,25000,25000},environment,adapter);input.poll(30,{0,25000,25000},environment,adapter);input.poll(1011,{0x8000,25000,25000},environment,adapter);}
  if(family==2){input.poll(10,{0x8000},environment,adapter);input.poll(20,{},environment,adapter);input.poll(311,{0x4000},environment,adapter);}
  if(family==3){input.poll(10,{0x2000},environment,adapter);input.poll(20,{},environment,adapter);input.poll(311,{0x2000},environment,adapter);}
  if(family==4){input.poll(10,{0,0,25000},environment,adapter);input.poll(20,{},environment,adapter);input.poll(30,{0,0,25000},environment,adapter);input.poll(411,{0x8000,0,25000},environment,adapter);}
  check(target.calls==0,"canonical-only context scans cannot revive a prefix after its source query window");
 }
}
void release_grammar() {
 const InputToken up{InputSource::direction,8,InputEdge::press}, jump{InputSource::buttons,12,InputEdge::release};
 for(unsigned count:{1u,2u})for(const std::uint64_t age:{299u,300u,301u,499u,500u,501u})for(bool intervening:{false,true}) {
  InputHistory history;history.record(up,10);
  if(count==2){history.record({up.source,up.code,InputEdge::release},20);history.record(up,30);}
  if(intervening)history.record({InputSource::buttons,9,InputEdge::press},40);
  history.record(jump,10+age);
  const auto window=count==1?300u:500u;
  const auto match=history.release_after_presses(up,jump,count,window,10+age,~InputConsumption::manual);
  check(static_cast<bool>(match)==(age<window),"release grammar window starts at first direction press with strict300/500ms boundary");
  if(match) {
   check(match.count==count+1 && match.first_at==10 && match.last_at==10+age,"one/two tap release retains only exact original required edges");
   check(history.consume(match,InputConsumption::regular,~InputConsumption::manual),"successful release grammar commits its category atomically");
   check(!history.release_after_presses(up,jump,count,window,10+age,~InputConsumption::manual),"consumed release cannot retrigger");
   for(const auto& event:history.events())if(event.token.source==InputSource::buttons && event.token.code==9)
    check(!event.owners,"permissible intervening control is not consumed");
  } else for(const auto& event:history.events())check(!event.owners,"failed release query leaves all history reusable");
 }
 for(unsigned owner:{InputConsumption::manual,InputConsumption::regular,InputConsumption::extra}) {
  InputHistory history;history.record(up,10);const auto prefix=history.match({std::span(&up,1),100},10);
  history.consume(prefix,owner);history.record(jump,20);
  check(static_cast<bool>(history.release_after_presses(up,jump,1,300,20,~InputConsumption::manual))==(owner==InputConsumption::manual),
   "release grammar honors regular rejection and Manual-only shared-prefix reuse");
 }
 InputHistory absent;absent.record(jump,20);
 check(!absent.release_after_presses(up,jump,1,300,20) && !absent.release_after_presses(up,jump,0,300,20) &&
  !absent.release_after_presses(up,jump,3,500,20),"held baseline and undefined repetition counts cannot fabricate a release pattern");
}
void jump_release_bindings() {
 for(auto binding:{InputJumpBinding::disabled,InputJumpBinding::single_release,InputJumpBinding::repeated_release})
 for(bool repeated:{false,true})for(auto context:{InputContext::ground,InputContext::air,InputContext::balance,InputContext::rail}) {
  UnifiedInput input;Receiver receiver;InputEnvironment environment{context,true};environment.jump_binding=binding;environment.jump_generation=1;environment.nollie=true;
  input.poll(0,{},environment,receiver.adapter());input.poll(10,{0,0,25000},environment,receiver.adapter());
  if(repeated){input.poll(20,{},environment,receiver.adapter());input.poll(30,{0,0,25000},environment,receiver.adapter());}
  input.poll(40,{0x1000,0,25000},environment,receiver.adapter());input.poll(50,{0,0,25000},environment,receiver.adapter());
  const bool expected=context==InputContext::ground && binding!=InputJumpBinding::disabled &&
   (binding==InputJumpBinding::single_release || repeated);
  check((receiver.last.action==InputAction::jump_variant)==expected,"only explicitly selected grounded release binding can replace ordinary Ollie release");
  if(expected)check(receiver.last.variant==static_cast<std::uint8_t>(binding) && !receiver.last.nollie &&
   receiver.last.jump_generation==1 && receiver.last.control==0x1000 && receiver.last.physical_pressed_at==40 && receiver.last.physical_released_at==50 &&
   receiver.last.recognized_at==50 && receiver.last.first_input_at==(binding==InputJumpBinding::single_release && repeated?30u:10u) && receiver.last.last_input_at==50,
   "semantic jump destination preserves charge/release and exact pattern while clearing nollie context");
  else check(receiver.last.action==InputAction::ollie_release,"disabled or unmatched release retains supported ordinary jump path");
 }
 for(bool missing_epoch:{false,true}) {
  UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::ground,true};
  environment.jump_binding=InputJumpBinding::single_release;environment.jump_generation=missing_epoch?0u:1u;
  input.poll(0,{0,0,25000},environment,receiver.adapter());
  input.poll(10,{0x1000,0,25000},environment,receiver.adapter());input.poll(20,{0,0,25000},environment,receiver.adapter());
  check(receiver.last.action==InputAction::ollie_release,"held startup Up cannot replace ordinary release even when a table is selected");
  input.poll(30,{},environment,receiver.adapter());input.poll(40,{0,0,25000},environment,receiver.adapter());
  input.poll(50,{0x1000,0,25000},environment,receiver.adapter());input.poll(60,{0,0,25000},environment,receiver.adapter());
  check((receiver.last.action==InputAction::jump_variant)==!missing_epoch,"missing selected-table epoch keeps the proven ordinary path enabled");
 }
 struct Unsupported {
  unsigned variants{},ordinary{};
  static InputDisposition receive(void* state,const InputRequest& request) noexcept {
   auto& self=*static_cast<Unsupported*>(state);
   if(request.action==InputAction::jump_variant){++self.variants;return InputDisposition::unsupported;}
   if(request.action==InputAction::ollie_release)++self.ordinary;
   return InputDisposition::started;
  }
 };
 {
  UnifiedInput input;Unsupported receiver;InputEnvironment environment{InputContext::ground,true};
  environment.jump_binding=InputJumpBinding::single_release;environment.jump_generation=1;
  const InputAdapter adapter{&receiver,Unsupported::receive};
  input.poll(0,{},environment,adapter);input.poll(10,{0,0,25000},environment,adapter);
  input.poll(20,{0x1000,0,25000},environment,adapter);input.poll(30,{0,0,25000},environment,adapter);input.poll(40,{},environment,adapter);
  check(receiver.variants==1 && !receiver.ordinary,"unsupported exact release variant never substitutes ordinary actor or retriggers from released state");
 }
 struct Waiting {
  InputRequest original{};unsigned jump_calls{},plain_release{};InputDisposition result{InputDisposition::waiting};
  static InputDisposition receive(void* state,const InputRequest& request) noexcept {
   auto& self=*static_cast<Waiting*>(state);
   if(request.action==InputAction::ollie_release)++self.plain_release;
   if(request.action!=InputAction::jump_variant)return InputDisposition::started;
   self.original=request;++self.jump_calls;return self.result;
  }
 };
 for(unsigned cancel=0;cancel<5;++cancel) {
  UnifiedInput input;Waiting receiver;InputEnvironment environment{InputContext::ground,true};environment.jump_binding=InputJumpBinding::single_release;environment.jump_generation=1;
  const InputAdapter adapter{&receiver,Waiting::receive};
  input.poll(0,{},environment,adapter);input.poll(10,{0,0,25000},environment,adapter);
  input.poll(20,{0x1000,0,25000},environment,adapter);input.poll(30,{0,0,25000},environment,adapter);
  const auto original=receiver.original;check(receiver.jump_calls==1 && !receiver.plain_release,"matched binding queues one semantic request, no ordinary-action substitution");
  if(cancel==1)environment.jump_binding=InputJumpBinding::repeated_release;
  if(cancel==2)environment.context=InputContext::air;
  if(cancel==3)environment.gameplay=false;
  if(cancel==4)environment.jump_generation=2;
  receiver.result=InputDisposition::started;input.poll(1000,{0,0,25000},environment,adapter);
  check(receiver.jump_calls==(cancel?1u:2u) && (cancel || receiver.original==original),
   "waiting variant retains original identity/time; table/context/menu changes cancel without timestamp renewal");
 }
}
void sampled_release_bindings() {
 for(const std::uint64_t cadence:{17u,34u})for(std::uint64_t phase=0;phase<cadence;++phase)
 for(auto binding:{InputJumpBinding::single_release,InputJumpBinding::repeated_release})for(const std::uint64_t gap:{250u,300u,450u,500u})for(bool pad:{false,true}) {
  UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::ground,true};environment.jump_binding=binding;environment.jump_generation=1;
  std::uint64_t first{},release{};
  for(std::uint64_t now=0;now<800;now+=cadence) {
   const auto start=100+phase;const auto end=start+gap;
   const bool up=now>=start && (binding==InputJumpBinding::single_release || now<start+50 || now>=start+100);
   const bool held=now>=start+150 && now<end;
   PhysicalInputSample physical{};physical.buttons=static_cast<std::uint16_t>((held?0x1000u:0u)|(pad && up?1u:0u));
   if(!pad && up)physical.y=25000;
   if(up && !first)first=now;if(now>=end && !release)release=now;
   input.poll(now,physical,environment,receiver.adapter());
   if(now>=end)break;
  }
  const bool expected=release-first<(binding==InputJumpBinding::single_release?300u:500u);
  check((receiver.last.action==InputAction::jump_variant)==expected,"sampled30/60Hz release grammar uses observed first-edge clock without adding Manual sampling slack");
 }
}

}
int main(){release_grammar();jump_release_bindings();sampled_release_bindings();physical_footprints();expired_context_patterns();std::cout<<checks<<" input consolidation checks passed\n";}
