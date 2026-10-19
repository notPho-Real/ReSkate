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
}
int main(){physical_footprints();expired_context_patterns();std::cout<<checks<<" input consolidation checks passed\n";}
