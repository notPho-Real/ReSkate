#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* reason){++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}}
struct Session {
 InputSession input;InputAdmission admission{};
 const std::array<PreparedGesture,1> catalog{PreparedGesture{2,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
 Session(){admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=true;admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;admission.catalog=catalog;}
 NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons,std::uint64_t episode=1,std::uint64_t action=1,std::uint64_t contact=1){
  admission.native_context={true,true,{1,contact,action},now+1,now,false,admission.pose==SkatePose::air,episode,episode?episode-1:0};
  admission.tick=now/17;return input.poll(now,{buttons,-25000},0,0,admission);
 }
};
void active_ownership() {
 for(bool grab:{false,true})for(unsigned change=0;change<3;++change) {
  Session session;session.poll(0,0);const auto button=static_cast<std::uint16_t>(grab?0x2000:0x4000);
  const auto active=session.poll(10,button);check(grab?active.action.owned:active.authored_playback,"the existing air adapter is active before the observed episode boundary");
  // Contact/selection can change within continuous AIR. Only a producer episode
  // marker establishes a missed departure; it is not inferred from increments.
  const auto next=session.poll(40,button,change==0?2:1,change==2?2:1,change==1?2:1);
  const bool owns=grab?next.action.owned:next.authored_playback;
  check(owns==(change!=0),"active air channels release on a missed departure, but survive wheel-only changes and own action acknowledgement");
  if(change==0){
   const auto held=session.poll(50,button,2);check(!held.action.owned && !held.authored_playback,"held input cannot restart old playback after an airborne episode changes");
   session.poll(60,0,2);const auto fresh=session.poll(70,button,2);
   check(grab?fresh.action.owned:fresh.authored_playback,"a real release and new press can start an action in the new airborne episode");
  }
 }
}
void unavailable_producer() {
 for(bool grab:{false,true}) {
  Session session;session.poll(0,0,0);const auto out=session.poll(10,static_cast<std::uint16_t>(grab?0x2000:0x4000),0);
  check(out.unsupported && !out.authored_playback && !out.action.owned,"a native-bound producer without airborne episode correlation cannot admit unowned cross-flight playback");
 }
 struct OldFrame {bool valid{true},rail_family{};std::uint64_t owner_generation{1},contact_generation{1},action_generation{1},physics_step{1},sampled_at{};};
 check(InputNativeContext::from_frame(OldFrame{}).airborne_episode==0,"older native frame layouts explicitly lack episode capability rather than inventing a counter");
 struct NewFrame : OldFrame {bool airborne{true};std::uint64_t airborne_episode{4},air_departure{1};};
 check(InputNativeContext::from_frame(NewFrame{}).airborne_episode==4,"the typed bridge forwards the actual producer episode without reconstructing history at input cadence");
}
void active_release_tail() {
 for(bool departed:{false,true}) {
  Session session;session.poll(0,0);session.poll(10,0x2000);const auto released=session.poll(20,0);
  check(released.action.owned,"release of an already active grab enters its existing owned neutral tail");
  const auto tail=session.poll(40,0,departed?2:1);
  check(tail.action.owned==!departed,"the active neutral tail is bound to its flight, without changing its continuous-air duration");
 }
}
void pending_and_manual() {
 Session pending;pending.admission.flip_ready=false;pending.poll(0,0);pending.poll(10,0x4000);
 pending.admission.flip_ready=true;const auto cancelled=pending.poll(40,0x4000,2);
 check(!cancelled.authored_playback && !cancelled.command_busy,"an old pending flip cannot start in a new episode even when readiness has returned");
 Session grab;grab.poll(0,0);grab.poll(10,0x4000);grab.poll(20,0x2000);
 check(grab.input.pending_grab().recognized_at==20,"a grab can wait behind a delivered flip without owning its own channels");
 const auto departed=grab.poll(40,0x2000,2);
 check(!departed.action.owned && !departed.authored_playback && grab.input.pending_grab().action==InputAction::none,
  "a missed departure cancels the waiting grab as well as its old predecessor without inventing a new press");
 InputSession manual;InputAdmission admission{};admission.gameplay=admission.manual_prepare=true;admission.pose=SkatePose::air;
 auto poll=[&](std::uint64_t now,std::int16_t y,std::uint64_t episode){admission.native_context={true,true,{1,episode,episode},now+1,now,false,admission.pose==SkatePose::air,episode,episode?episode-1:0};return manual.poll(now,{0,0,y},0,0,admission);};
 poll(0,0,1);poll(10,25000,1);poll(100,-25000,1);check(manual.pending_manual().recognized_at==100,"prepared Manual exists independently of the ordinary air episode queue");
 admission.pose=SkatePose::riding;admission.manual_safe=true;const auto landed=poll(120,-25000,2);
 check(landed.manual.owns && landed.manual.request_at==120,"an actual airborne departure preserves prepared Manual landing and its separate native settling clock");
}
}
int main(){active_ownership();active_release_tail();unavailable_producer();pending_and_manual();std::cout<<checks<<" air episode ownership checks passed\n";}
