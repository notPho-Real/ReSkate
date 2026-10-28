#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool v,const char* why){++checks;if(!v){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
bool near(float a,float b){return std::abs(a-b)<.000001f;}
void digital_states(){
 for(bool a:{false,true})for(bool b:{false,true}){
  const auto expected=a?1.0f:b?-1.0f:0.0f;
  check(manual_balance_correction({a,b})==expected,"configured A precedes B including both held");
  const auto raw=static_cast<std::uint32_t>((a?1u:0u)|(b?2u:0u));
  check(balance_correction(BalanceKind::manual,0,0,true,raw)==expected,"raw Dpad compatibility path retains independent A and B pressed states");
 }
 for(short y:{short{16000},short{25000},short{32767},short{-16000},short{-32768}})
  check(balance_correction(BalanceKind::manual,0,y,true)==(y>0?1.0f:-1.0f),"active compatibility axes produce a full digital step rather than fractional force");
 for(short y:{short{-9000},short{-7849},short{-5000},short{0},short{5000},short{7849},short{9000}})
  check(balance_correction(BalanceKind::manual,0,y,true)==0,"compatibility bridge uses controller-normalized neutral rather than a second balance deadzone");
 check(manual_balance_native_y(1)==32767 && manual_balance_native_y(-1)==-32768 && manual_balance_native_y(0)==0,
  "effective correction converts to the existing native full-range LS convention for both manual types");
 for(float invalid:{.5f,-.5f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
  check(manual_balance_native_y(invalid)==0,"fractional or nonfinite values cannot fabricate a native correction");
 check(balance_correction(BalanceKind::grind,16000,0,true)==(16000.0f-7849)/(32767-7849),"grind analog correction is unchanged");
}
void timing_and_physics(){
 for(bool nose:{false,true})for(int lean_sign:{-1,1})for(bool a:{false,true})for(bool b:{false,true})
 for(std::uint64_t elapsed:{0u,1u,999u,1000u,1001u})for(float dt:{1.0f/60,1.0f/30}){
  const ManualBalanceButtons buttons{a,b};const float direction=a?1.0f:b?-1.0f:0.0f;
  BalanceControl controls;controls.poll_manual(1000,static_cast<float>(lean_sign)*.2f,buttons,true);
  const auto effective=controls.poll_manual(1000+elapsed,static_cast<float>(lean_sign)*.2f,buttons,true);
  const bool released=(!a&&!b) || elapsed>0;
  const bool protected_outward=elapsed<1000 && direction*static_cast<float>(lean_sign)>0;
  check(effective==(released&&!protected_outward?direction:0),"entry suppression ends at elapsed>0; first1000ms protects outward correction without neutral requirement");
  const auto routed=controls.manual_input();
  check(routed.amount==direction && routed.held==(a||b) && routed.released==released && routed.elapsed_ms==elapsed,
   "physical adapter receives original digital state and original section elapsed time");
  ManualPhysics model({},1);model.begin(nose);
  for(unsigned i=0;i<8;++i)model.sample(dt,static_cast<float>(lean_sign));
  model.stop();check(model.begin(nose),"prior physical section history can be reused without new synthetic admission");
  auto oracle=model;
  const auto actual=model.sample(dt,routed);
  const auto expected=oracle.sample(dt,PhysicalManualControl{direction,a||b,released,elapsed});
  check(actual.valid && near(actual.position,expected.position) && near(actual.velocity,expected.velocity) && actual.exit==expected.exit,
   "production digital correction reaches preserved physics identically to explicit source digital inputs, including history and post-integration protection");
 }
}
void release_and_boundaries(){
 for(std::uint64_t cadence:{17u,34u}){
  BalanceControl controls;controls.poll_manual(0,0,{true,false},true);
  check(controls.poll_manual(cadence,0,{true,false},true)==1,"held entry direction becomes effective on the next30/60Hz sample");
  const auto elapsed=controls.manual_input().elapsed_ms;
  check(controls.poll_manual(2*cadence,0,{},true)==0 && !controls.manual_input().held,"neutral releases the control without a manufactured press");
  check(controls.poll_manual(3*cadence,0,{false,true},true)==-1 && controls.manual_input().elapsed_ms>elapsed,
   "fresh opposite or repressed correction retains original section clock");
  check(controls.poll_manual(4*cadence,0,{true,true},false)==0 && !controls.manual_input().held,"unavailable input cannot correct balance");
  check(controls.poll_manual(5*cadence,0,{true,true},true)==1,"same-section input reacquisition retains its original clock and armed state");
  check(controls.poll_manual(6*cadence,0,{true,true},true)==1,"A priority survives held input without a renewed entry instant");
  controls.reset_manual();check(!controls.manual_input().held && !controls.manual_input().released,"StopBalance clears control state without altering physical history");
  check(controls.poll_manual(7*cadence,0,{true,false},true)==0,"new owner or menu reset cannot borrow the previous section's released flag");
  check(controls.poll_manual(6*cadence,0,{true,false},true)==0,"clock rollback cancels correction continuity");
  controls.poll(8*cadence,BalanceKind::none,0,0,0,true);
  check(!controls.manual_input().held && controls.poll_manual(9*cadence,0,{true,false},true)==0,"inactive context clears manual controls");
 }
 for(bool nose:{false,true})for(bool unsafe:{false,true})for(float dt:{.017f,.034f}){
  ManualPhysics model({},1);model.begin(nose);BalanceControl controls;PhysicalBalanceSample out{};
  const int sign=(nose!=unsafe)?1:-1;const ManualBalanceButtons buttons{sign>0,sign<0};
  std::uint64_t now{};
  for(unsigned i=0;i<2000 && model.active();++i){
   controls.poll_manual(now,out.position,buttons,true);out=model.sample(dt,controls.manual_input());
   now+=dt<.02f?17u:34u;
  }
  check(model.closed() && out.exit==(unsafe?BalanceExit::bail:BalanceExit::putdown),"digital controls retain direction-specific protected endpoints for both types at30/60Hz");
  check(!model.begin(nose),"digital repress cannot reopen a closed physical section");
 }
}
void input_lease_continuity(){
 for(std::uint64_t cadence:{17u,34u})for(int lean:{-1,1})for(ManualBalanceButtons buttons:{ManualBalanceButtons{true,false},{false,true},{true,true}}){
  BalanceControl controls;controls.begin_manual(1000);
  controls.poll_manual(1000,static_cast<float>(lean),buttons,true);
  controls.poll_manual(2000,static_cast<float>(lean),buttons,true);
  const float expected=manual_balance_correction(buttons);
  check(controls.manual_input().released&&controls.manual_input().elapsed_ms==1000,"native setup owns original section clock and historical arming");
  check(controls.poll_manual(2150,static_cast<float>(lean),buttons,true)==expected,"last fresh lease sample retains original section age");
  check(controls.poll_manual(2151,static_cast<float>(lean),buttons,false)==0&&!controls.manual_input().held&&controls.manual_input().amount==0,
        "expired input withdraws force without ending native balance");
  check(controls.manual_input().released&&controls.manual_input().elapsed_ms==1151,"missing input preserves original section time and armed state");
  controls.begin_manual(2151+cadence);
  check(controls.poll_manual(2151+cadence,static_cast<float>(lean),buttons,true)==expected&&controls.manual_input().elapsed_ms==1151+cadence,
        "same-section admission and input reacquisition cannot renew the first-second safe period");
  controls.reset_manual();controls.begin_manual(3000);
  check(controls.poll_manual(3000,static_cast<float>(lean),buttons,true)==0&&!controls.manual_input().released,
        "actual new section cannot inherit the old armed state");
  check(controls.poll_manual(3000+cadence,static_cast<float>(lean),buttons,true)==(expected*static_cast<float>(lean)>0?0:expected),
        "genuine new section reinstates only its own outward protection");
  controls.poll_manual(2999,static_cast<float>(lean),buttons,false);
  check(controls.poll_manual(3100,0,buttons,true)==0,"rollback during a missing input lease retires clock continuity");
 }
 for(std::uint64_t elapsed:{999u,1000u,1001u}){
  BalanceControl controls;controls.begin_manual(1000);controls.poll_manual(1000,1,{},false);
  const float value=controls.poll_manual(1000+elapsed,1,{true,false},true);
  check(value==(elapsed<1000?0:1)&&controls.manual_input().elapsed_ms==elapsed,
        "native section start precedes first available input rather than borrowing its timestamp");
 }
}
void thumb_travel(){
 struct Frame{short x,y;std::uint32_t pad;float expected;};
 const Frame trace[]{{0,0,0,0},{0,5000,0,0},{17000,16000,0,-1},{9000,22000,0,-1},
  {0,8000,0,0},{0,4000,0,0},{-15000,-14000,0,1},{0,-23000,1,-1},
  {0,-23000,3,-1},{0,23000,2,1},{0,0,0,0}};
 for(bool nose:{false,true})for(std::uint64_t cadence:{17u,34u}){
  BalanceControl controls;ManualPhysics model({},1),oracle({},1);model.begin(nose);oracle.begin(nose);
  std::uint64_t now{};
  for(const auto& frame:trace){
   const auto effective=controls.poll(now,BalanceKind::manual,0,frame.x,frame.y,true,frame.pad);
   check(effective==-frame.expected,"human-like partial/diagonal/neutral travel and Dpad priority remain digital at30/60Hz");
   const float dt=static_cast<float>(cadence)/1000;
   const auto actual=model.sample(dt,controls.manual_input());
   const auto expected=oracle.sample(dt,PhysicalManualControl{-frame.expected,frame.expected!=0,true,now});
   check(near(actual.position,expected.position)&&near(actual.velocity,expected.velocity)&&actual.exit==expected.exit,
    "mixed stick/Dpad held-release travel reaches the unchanged physical adapter with exact digital strength");
   now+=cadence;
  }
 }
}
void exclusions(){
 for(auto kind:{InputBalanceKind::manual,InputBalanceKind::nose})for(std::uint64_t cadence:{17u,34u})for(bool pad:{false,true}){
  InputEvents journal;const auto controls=InputBalanceControls::vertical(kind);
  journal.sample(0,{},controls,true,InputPolicy::production());
  for(unsigned i=1;i<=8;++i){
   const int sign=i<3?1:i<5?0:-1;
   const auto mask=static_cast<std::uint16_t>(pad?(sign>0?1u:sign<0?2u:0u):0u);
   journal.sample(i*cadence,{mask,0,static_cast<std::int16_t>(pad?0:sign*25000)},controls,true,InputPolicy::production());
   check(journal.history().events().empty(),"admitted correction controls remain excluded from trick history while physical held state updates");
   check(journal.held_direction()==(sign>0?8:sign<0?2:0),"held and release state survives balance-event exclusion");
  }
 }
}
void admitted_pad_boundary(){
 for(bool nose:{false,true}){
  ArcadeInput input(true);input.enable(true);NativeInputState observer;
  NativeRequestSample native{};native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=nose?0x8000:0x4000;
  const auto frame=observer.observe(1000,{1,2,3,4,5},native,{true,-1});
  input.publish_native_context(frame);input.publish_authority(1000,frame.owner_generation,1,true);
  const auto kind=nose?InputBalanceKind::nose:InputBalanceKind::manual;
  input.publish_balance_controls(InputBalanceControls::vertical(kind),frame);
  const auto packet=input.poll(0,1000,1,1234,0,0,-20000,true,false,false,0,false,false,false,0,false,{},false,true,
   nose?SkatePose::nose_manual:SkatePose::manual,false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true,true);
  check(!(packet.buttons&3u) && !packet.action.pad.owns_left_stick,"current production masks digital directions without an auxiliary LS actuator");
  const auto route=[&](std::uint64_t now,float effective,bool blocked,short physical){
   const auto admitted=input.balance_admission(now,kind);
   return admitted.valid&&!blocked?manual_balance_native_y(effective):physical;
  };
  check(route(1000,1,false,0)==32767 && route(1000,-1,false,0)==-32768 && route(1000,0,false,2345)==0,
   "qualified composition sends digital/neutral correction through the preserved native LS Y channel");
  check(route(1000,-1,true,2345)==2345,"another actor's channel is never overwritten");
  check(route(1151,-1,false,2345)==2345,"stale original admission proof cannot qualify native output");
  const auto other=observer.observe(1160,{6,7,8,9,10},native,{true,-1});input.publish_native_context(other);input.publish_authority(1160,other.owner_generation,1,true);
  check(route(1160,-1,false,2345)==2345,"new native owner cannot reuse old admitted controls");
  input.publish_balance_controls(InputBalanceControls::vertical(kind),other);input.publish_authority(1160,other.owner_generation,1,false);
  check(route(1160,-1,false,2345)==2345,"menu/focus authority loss cannot qualify native correction");
  input.enable(false);
  const auto off=input.poll(0,1170,1,1234,2345,3456,4567,true,false);
  check(off.buttons==1 && off.right_x==3456 && off.right_y==4567 && route(1170,-1,false,2345)==2345,
   "Arcade OFF restores physical directions and axes without any correction ownership");
 }
}

void signed_trajectories() {
 for(bool nose:{false,true}) for(float dt:{1.f/30,1.f/60,1.f/120}) {
  std::array<float,3> positions{};
  for(int direction:{-1,0,1}) {
   ManualPhysics physics({},1); physics.begin(nose);
   physics.adjust({1,0,0,0,0,0});
   BalanceControl controls; controls.begin_manual(0);
   InputEvents input;
   auto mapping=InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual);
   PhysicalBalanceSample out{};
   for(unsigned i=0;i<static_cast<unsigned>(std::lround(.1f/dt));++i) {
    const auto now=1000+static_cast<std::uint64_t>(i*dt*1000);
    input.sample(now,{0,0,static_cast<short>(direction*25000)},mapping,true,InputPolicy::production());
    const auto pair=input.digital_state().configured(mapping);
    controls.poll_manual(now,out.position,{pair.first,pair.second},true);
    out=physics.sample(dt,controls.manual_input());
   }
   positions[direction+1]=out.position;
   check(out.active && std::abs(out.position)<.05f,"100ms correction is manageable for both manual types and cadences");
   if(direction)check(out.position*direction>0,"physical UP raises actual balance; DOWN lowers it");
   State state{}; state.combo_active=true;state.balance_kind=BalanceKind::manual;state.balance=out.position;
   check(balance_meter(state).cursor_y==-out.position,"HUD follows corrected physical trajectory without inversion");
  }
  check(positions[0]<positions[2] && std::abs(positions[1])<.05f,"neutral drift stays bounded while UP and DOWN trajectories separate");
  ManualPhysics physics({},1);physics.begin(nose);PhysicalBalanceSample out{};
  for(unsigned i=0;i<static_cast<unsigned>(std::lround(6.f/dt));++i) {
   const float predicted=out.position+out.velocity*24;
   const float correction=predicted>.03f?-1.f:predicted<-.03f?1.f:0;
   out=physics.sample(dt,correction);
   check(out.active,"predictive brief corrections sustain six seconds without disabling endpoints");
  }
 }
}
}
int main(){signed_trajectories();digital_states();timing_and_physics();release_and_boundaries();input_lease_continuity();thumb_travel();exclusions();admitted_pad_boundary();std::cout<<checks<<" digital manual input checks passed\n";}
