#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_rail_adapter.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool v,const char* why){++checks;if(!v){std::cerr<<why<<'\n';std::exit(1);}}
InputNativeContext native(std::uint64_t now,std::uint64_t step=1,bool rail=false) {
 return {true,true,{1,1,1},step,now,rail,!rail,1,rail?1u:0u};
}
struct Fixture {
 InputSession session;RailAdapter actor;InputAdmission admission{};std::uint64_t step{},now{};
 NativeGesture::Output out{};RailAdapterPad pad{};
 const std::array<PreparedGesture,1> catalog{PreparedGesture{1,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
 Fixture(){admission.gameplay=admission.manual_safe=admission.may_start=admission.authored=true;admission.action_fresh=admission.inspected_stance=true;admission.action_mode=ActionExperiment::classic;admission.export_rail_requests=true;admission.catalog=catalog;}
 void observe(std::uint64_t time,SkatePose pose=SkatePose::air) {
  now=time;admission.pose=pose;admission.tick=now/17;admission.native_context=native(now,++step,pose==SkatePose::grind);
  if(pose==SkatePose::riding)admission.native_context.airborne=false;
  actor.observe(admission.native_context,pose==SkatePose::riding?100u:pose==SkatePose::grind?401u:200u);
 }
 void poll(std::uint16_t buttons=0,std::int16_t y=0,bool ready=true) {
  out=session.poll(now,{buttons,0,y},0,0,admission);pad=actor.sample(out.rail,now,admission.gameplay,ready);
 }
};
void basic(){
 Fixture f;f.observe(0);f.poll();f.observe(10);f.poll(0x8000,25000);
 const auto original=f.out.rail.request;
 check(f.pad.owns_right_stick&&f.pad.right_y==20000,"composed semantic nose pressure executes");
 check(original.action==InputAction::rail_request&&original.control==0x8000&&original.input_sequence&&original.session,"exact action/control/physical sequence exported");
 check(original.recognized_at==10&&original.physical_pressed_at==10&&original.first_input_at==10&&original.last_input_at==10&&original.deadline==210,"physical origin and original strict handoff deadline retained");
 check(f.out.right_y==0&&!f.out.rail.occupied.owns_right_stick,"external export does not also deliver legacy RS");
 for(auto t:{100u,200u,300u}) {f.observe(t,SkatePose::grind);f.poll(0x8000,25000);check(f.out.rail.request==original&&f.pad.owns_right_stick,"accepted held owner continues without renewing original request deadline");}
 f.observe(310,SkatePose::grind);f.poll();
 check(!f.pad.owns_right_stick&&!f.out.rail.held&&f.out.rail.request.physical_released_at==310,"actual initiating release exported and releases channels");
 f.observe(320,SkatePose::grind);f.poll();check(f.out.rail.request.action==InputAction::none,"release is not repeated");
}
void readiness(){
 for(auto start_at:{209u,210u,211u}) {
  Fixture f;f.observe(0);f.poll();f.observe(10);f.poll(0x8000,0,false);
  auto original=f.out.rail.request;check(!f.pad.owns_right_stick,"unready handoff owns no channel");
  f.observe(100);f.poll(0x8000,0,false);f.observe(start_at);f.poll(0x8000);
  check(f.pad.owns_right_stick==(start_at==209)&&f.out.rail.request==original,"original exclusive deadline, not latest poll, controls first issue");
 }
 Fixture ground;ground.observe(0,SkatePose::riding);ground.poll();ground.observe(10,SkatePose::riding);ground.poll(0x8000);
 check(!ground.pad.owns_right_stick,"ground selection is not native RS actuation");
 ground.observe(50);ground.poll(0x8000);check(ground.pad.owns_right_stick&&ground.out.rail.request.recognized_at==10,"same consumed ground request hands off in fresh air within original deadline");
}
void cancellations(){
 for(unsigned reason=0;reason<5;++reason) {
  Fixture f;f.observe(0);f.poll();f.observe(10);f.poll(0x8000);check(f.pad.owns_right_stick,"active cancellation setup");
  const auto old=f.out.rail;
  f.observe(20);
  if(reason==0)f.admission.gameplay=false;
  if(reason==1)f.admission.external_owner=true;
  if(reason==2){f.admission.native_context.generations.owner=2;f.actor.observe(f.admission.native_context,200);}
  if(reason==3){f.now=171;}
  if(reason==4)f.admission.pose=SkatePose::bail;
  f.poll(0x8000);check(!f.pad.owns_right_stick,"OFF/conflict/owner/stale/bail releases old ownership");
  check(!f.actor.sample(old,20,true,true).owns_right_stick,"old held handoff cannot restart after cancellation");
 }
}
void explicit_conflicts(){
 for(unsigned owner=0;owner<4;++owner) {
  Fixture f;f.observe(0);f.poll();f.observe(10);f.poll(0x8000,0,false);auto waiting=f.out.rail;
  if(owner==0)waiting.blocked=true;
  if(owner==1)waiting.occupied.owns_right_stick=true;
  if(owner==2){waiting.occupied.owns_triggers=true;waiting.occupied.owns_right_stick=true;}
  if(owner==3){waiting.occupied.owns_face_buttons=true;waiting.blocked=true;}
  check(!f.actor.sample(waiting,10,true,true).owns_right_stick,"manual/flip/grab/external reservation excludes issue");
  f.observe(20);f.poll(0x8000);check(f.pad.owns_right_stick,"preissue reservation may clear within unchanged deadline");
  waiting=f.out.rail;waiting.blocked=true;
  check(!f.actor.sample(waiting,20,true,true).owns_right_stick,"active conflict cancels rail channel");
  waiting.blocked=false;check(!f.actor.sample(waiting,20,true,true).owns_right_stick,"cancelled actor is not restarted by held output");
 }
}
void composed_owners(){
 Fixture manual;manual.observe(0,SkatePose::riding);manual.poll();manual.observe(10,SkatePose::riding);manual.poll(0,25000);
 manual.observe(100,SkatePose::riding);manual.poll(0,-25000);
 check(manual.out.manual.owns,"actual Manual owns RS before initiating rail command");
 manual.observe(110,SkatePose::riding);manual.poll(0x8000,-25000);
 check(!manual.out.manual.owns&&!manual.pad.owns_right_stick,"existing Y cancellation releases Manual; grounded rail preparation claims no simultaneous RS");
 Fixture flip;flip.observe(0,SkatePose::riding);flip.poll();flip.observe(10,SkatePose::riding);flip.poll(0x1000);flip.observe(20);flip.poll(0x8000);flip.observe(30);flip.poll(0x8000);
 check(flip.out.authored_playback&&flip.out.rail.blocked&&flip.out.rail.occupied.owns_right_stick&&!flip.pad.owns_right_stick,"actual authored Flip/Ollie playback reserves RS before rail handoff");
 Fixture grab;grab.observe(0);grab.poll();grab.observe(10);grab.poll(0x2000);grab.observe(20);grab.poll(0xa000);
 check(grab.out.action.owned&&grab.out.rail.blocked&&grab.out.rail.occupied.owns_right_stick&&!grab.pad.owns_right_stick,"actual Grab actor reserves its channels before rail handoff");
 auto original=grab.out.rail.request;grab.observe(30);grab.poll(0x8000);grab.observe(70);grab.poll(0x8000);
 check(grab.pad.owns_right_stick&&grab.out.rail.request==original,"original handoff waits through actual grab neutral tail without recognition renewal");
}
void sampling_phases(){
 for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase){
  Fixture f;f.observe(0);f.poll();const auto press=phase+cadence;f.observe(press);f.poll(0x8000,0,false);
  const auto original=f.out.rail.request;f.observe(press+cadence);f.poll(0x8000);
  check(f.pad.owns_right_stick&&original.physical_pressed_at==press&&original.deadline==press+200&&f.out.rail.request==original,"all sampling phases preserve event origin and readiness deadline");
  f.observe(press+2*cadence);f.poll();check(!f.pad.owns_right_stick&&f.out.rail.request.physical_released_at==press+2*cadence,"all sampling phases preserve initiating release");
 }
}
void identity_and_categories(){
 Fixture f;f.observe(0);f.poll();f.observe(10);f.poll(0x8000,0,false);auto old=f.out.rail;
 auto changed=old;changed.request.deadline++;
 check(!f.actor.sample(changed,10,true,true).owns_right_stick,"same command cannot acquire a renewed deadline");
 check(!f.actor.sample(old,10,true,true).owns_right_stick,"mutation retires original ticket too");
 f.observe(20);f.poll();f.observe(20);f.poll(0x8000);
 check(f.pad.owns_right_stick&&f.out.rail.request.input_sequence!=old.request.input_sequence,"new physical event identity rearms even at equal millisecond timestamp");
 check(!f.actor.sample(old,20,true,true).owns_right_stick,"superseded physical event cannot replace a newer semantic ticket");
 for(auto action:{InputAction::rail_select,InputAction::rail_change,InputAction::lip_request,InputAction::lip_select}) {
  RailAdapter a;a.observe(native(10),200);auto named=old;named.request.action=action;
  check(!a.sample(named,10,true,true).owns_right_stick,"named and Lip actions never substitute basic pressure");
 }
 InputSession legacy,exported;InputAdmission admission{};admission.gameplay=admission.manual_safe=admission.action_fresh=admission.inspected_stance=true;admission.pose=SkatePose::air;
 legacy.poll(0,{},0,0,admission);admission.export_rail_requests=true;exported.poll(0,{},0,0,admission);
 admission.export_rail_requests=false;auto a=legacy.poll(10,{0x8000},0,0,admission);admission.export_rail_requests=true;auto b=exported.poll(10,{0x8000},0,0,admission);
 check(a.right_y==b.right_y&&a.phase==1&&b.phase==0,"legacy actuation retained by default and explicitly omitted on export");
 auto l=legacy.history().events(),r=exported.history().events();check(l.size()==r.size(),"handoff adds no events");
 for(std::size_t i=0;i<l.size();++i)check(l[i].sequence==r[i].sequence&&l[i].owners==r[i].owners,"category consumption remains unchanged");
}
}
int main(){basic();readiness();cancellations();explicit_conflicts();composed_owners();sampling_phases();identity_and_categories();std::cout<<checks<<" rail semantic handoff checks passed\n";}
