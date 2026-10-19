#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Extension/ArcadeMode/arcade_rail_adapter.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
int main() {
 unsigned checks{};auto check=[&](bool v){++checks;if(!v)std::exit(1);};
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};RailAdapter actor;
 auto step=[&](std::uint64_t now,std::uint32_t state,bool deliver=true) {
  NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;
  auto frame=producer.observe(now,owner,sample,{true,-1,55});if(deliver)actor.observe(frame);return frame;
 };
 step(10,200);check(actor.request(RailAdapterMethod::nose_pressure,10,1000,true,true,false)==RailAdapterResult::started);
 step(20,401);auto output=actor.sample(20,true,true,true,false);check(output.owns_right_stick && output.right_y==20000);
 step(30,403);check(actor.phase()==RailAdapterPhase::rail_contact);
 step(40,200);check(actor.sample(40,true,true,true,false).owns_right_stick);
 step(50,100);check(!actor.sample(50,true,true,true,false).owns_right_stick);
 step(60,200);check(!actor.sample(60,true,true,true,false).owns_right_stick);
 check(actor.request(RailAdapterMethod::dark_catch,60,1000,true,true,false)==RailAdapterResult::started);
 step(70,100,false);step(80,200);check(actor.sample(80,true,true,true,false).button_mask==0);
 step(90,200);check(actor.request(RailAdapterMethod::dark_catch,90,1000,true,true,false)==RailAdapterResult::started);
 owner.entity=8;step(100,200);check(actor.sample(100,true,true,true,false).button_mask==0);
 std::cout<<checks<<" native producer rail adapter composition checks passed\n";
}
