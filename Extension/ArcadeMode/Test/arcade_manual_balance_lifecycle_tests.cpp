#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_manual_acquisition.h"
#include "Extension/ArcadeMode/arcade_manual_release.h"
#include "Extension/ArcadeMode/arcade_manual_hop.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_balance_hud_lease.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool x,const char*w){++checks;if(!x){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}
struct Pipeline {
 bool nose{},admitted{},activity{};NativeInputOwner owner{1,2,3,4,5};NativeInputState physics;NativeInputFrame frame{};
 ArcadeInput input{true};ManualObservation identity;ManualAcquisition acquisition;ManualRelease putdown;
 ManualPhysics balance;BalanceControl controls;ComboSession session;NativeGesture::Output output{};
 std::uint64_t sequence{};SkatePose pose{SkatePose::air};bool exiting{};
 explicit Pipeline(bool n):nose(n){input.enable(true);}
 void step(std::uint64_t now,unsigned state,unsigned contacts,short y=0,bool label=false,bool pushing=false){
  NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;sample.words[1]=contacts;sample.words[0]=pushing?0x40:0;
  frame=physics.observe(now,owner,sample,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
  auto physical=manual_contact_pose({true,false,false,false,state,1,contacts},pushing);
  if(state==200||state==201)physical=SkatePose::air;
  ScoreLabelOccurrence history{true,label?score_label_bit(nose?2:1):0,sequence,0,now,ScoreLabelUncertainty::none};
  if(label){history.sequence=++sequence;history.observed_at=now;}
  if(putdown.pending())identity.reset(history);
  const auto observed=identity.poll(observed_pose({true,activity,activity,false,state,1,contacts}),history,
      {now,frame.owner_generation,true,true,activity,false,false,physical,pushing});
  const bool request_exit=acquisition.step(now,frame,physical,observed.confirmed,true,pushing);
  pose=request_exit&&balance_kind(observed.pose)==BalanceKind::manual?SkatePose::riding:observed.pose;
  const auto correction=controls.poll(now,balance_kind(pose),session.model().state().balance,0,y,true);
  auto modeled=manual_physics_pose(balance,pose,1.0f/60,controls.manual_input(),acquisition.preserves_history(now,frame));
  session.step(pose,1.0f/60,correction,false,&modeled);
  const auto release=putdown.step(now,frame.owner_generation,true,physical,session.manual_putdown()||request_exit);
  exiting=release.pending;admitted=observed.confirmed&&session.model().state().balance_kind==BalanceKind::manual&&!exiting&&!session.failed_section();
  if(exiting||session.failed_section()||session.manual_putdown())input.cancel_manuals();
  if(session.failed_section()||session.manual_putdown())acquisition.reset();
  input.publish_balance_controls(admitted?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},frame);
  const auto control_pose=manual_input_pose(pose,pushing,admitted,physical);
  output=input.poll(0,now,0,0,y,0,0,true,false,false,0,false,false,false,now/17,false,{},false,true,control_pose,
      false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true,admitted);
  if(exiting&&!output.authored_playback&&!output.action.owned)output.right_x=output.right_y=0;
  acquisition.observe(now,frame,output.manual,true);
 }
 bool meter()const{return balance_meter(session.balance_display_state()).visible;}
 void pair(){step(1000,200,0);step(1017,200,0,nose?-25000:25000);step(1034,200,0,nose?25000:-25000);}
};
}
int main(){
 for(bool nose:{false,true}){
  Pipeline p(nose);p.pair();check(p.output.manual.owns,"actual airborne input pair owns bounded native preparation");
  p.step(1051,100,nose?0x8000:0x4000,0,true);
  check(p.admitted&&p.meter()&&p.output.manual.confirmed,"landing into either type joins actual contact/history to balance and meter without generic activity");
  for(std::uint64_t now=1068;now<2300;now+=17)p.step(now,100,nose?0x8000:0x4000,p.session.model().state().balance_velocity>0?25000:-25000);
  check(p.admitted&&p.meter()&&!p.exiting,"admitted contact continues balance after1200ms without waiting for presentation or refreshing identity");
  p.step(2310,100,0);check(!p.admitted&&!p.meter(),"unknown contact grants no new physical balance or stale meter");
  p.step(2327,100,nose?0x8000:0x4000);
  check(p.exiting&&!p.meter()&&p.output.right_y==0,"lost identity with persistent acquired contact enters neutral putdown instead of endless no-bar manual");
  p.step(2344,100,nose?0x8000:0x4000,0,true);
  check(p.exiting&&!p.admitted&&!p.meter(),"late identity cannot reopen a native section already delivering controlled exit");
  p.step(2361,100,0xc000);check(!p.exiting&&!p.meter(),"only actual two-wheel completion retires the putdown owner");
 }
 for(bool nose:{false,true}){
  Pipeline p(nose);p.pair();p.step(1051,100,nose?0x8000:0x4000);
  for(std::uint64_t now=1068;now<2251;now+=17)p.step(now,100,nose?0x8000:0x4000);
  p.step(2251,100,nose?0x8000:0x4000);
  check(p.exiting&&!p.admitted&&!p.meter()&&p.output.right_y==0,"no identity means bounded acquisition exit at the original1200ms grounded lease");
 }
 for(bool nose:{false,true}) {
  Pipeline p(nose);p.pair();p.step(1051,100,nose?0x8000:0x4000,0,true);
  for(std::uint64_t at=1068;at<6000&&!p.session.failed_section();at+=17)p.step(at,100,nose?0x8000:0x4000);
  check(p.session.failed_section()&&p.meter()&&!p.output.manual.owns&&!p.exiting,"admitted balance failure retains its failed meter and withdraws actuation instead of substituting unadmitted putdown");
  // Keep the actual producer cadence and physical failed section continuous.
  auto at=p.frame.sampled_at;
  for(unsigned i=0;i<5;++i){at+=17;p.step(at,100,nose?0x8000:0x4000);}
  check(p.session.failed_section()&&p.meter()&&!p.exiting,"continued failed native contact cannot restart balance or create a safe-exit substitute");
 }
 for(bool nose:{false,true})for(unsigned fault=0;fault<9;++fault) {
  ManualAcquisition acquisition;NativeInputState observer;NativeInputOwner owner{1,2,3,4,5};
  NativeRequestSample s{};s.valid=true;s.state=100;s.crouch_mode=1;s.words[1]=nose?0x8000:0x4000;
  auto f=observer.observe(1000,owner,s,{true,-1});ManualInputDiagnostic request{};
  request.owns=true;request.request=nose?1:-1;request.request_at=1000;
  acquisition.observe(1000,f,request,true);
  const auto wanted=nose?SkatePose::nose_manual:SkatePose::manual;
  check(!acquisition.step(1000,f,wanted,false,true),"matching acquired native contact does not invent immediate balance or expiry");
  for(std::uint64_t at=1100;at<2200;at+=100){f=observer.observe(at,owner,s,{true,-1});acquisition.observe(at,f,request,true);check(!acquisition.step(at,f,SkatePose::unknown,false,true),"unknown contact and repeated polls cannot renew or prematurely finish the original acquisition deadline");}
  f=observer.observe(2199,owner,s,{true,-1});check(!acquisition.step(2199,f,wanted,false,true),"unadmitted acquisition is still valid at1199ms");
  f=observer.observe(2200,owner,s,{true,-1});
  if(fault==0)check(acquisition.step(2200,f,wanted,false,true),"original1200ms boundary requests putdown");
  if(fault==1)check(!acquisition.step(2200,f,wanted,true,true),"independently admitted balance waives the unadmitted acquisition cap");
  if(fault==2)check(!acquisition.step(2200,f,wanted,false,false),"invalid gameplay cannot retain a cleanup actuator");
  if(fault==3)check(!acquisition.step(2200,f,wanted,false,true,true),"actual native pushing revokes an old acquisition token");
  if(fault==4){++f.owner.rig;check(!acquisition.step(2200,f,wanted,false,true),"full owner replacement cannot inherit pending cleanup");}
  if(fault==5){++f.owner_generation;check(!acquisition.step(2200,f,wanted,false,true),"same-address owner generation replacement cannot inherit cleanup");}
  if(fault==6)check(!acquisition.step(2199,f,wanted,false,true),"future native proof fails closed");
  if(fault==7)check(!acquisition.step(2200,f,SkatePose::air,false,true),"actual hop escape retires acquired contact without putdown");
  if(fault==8)check(!acquisition.step(2200,f,SkatePose::riding,false,true),"actual two-wheel ground retires acquisition without a fake balance");
 }
 for(bool nose:{false,true}) {
  ManualPhysics balance;balance.begin(false);balance.sample(1.0f/60,0);balance.stop();const auto expected=balance;
  ManualAcquisition acquisition;NativeInputState observer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample s{};
  s.valid=true;s.state=200;s.crouch_mode=1;auto frame=observer.observe(1000,owner,s,{true,-1});
  ManualInputDiagnostic request{};request.owns=true;request.request=nose?1:-1;request.request_at=1000;
  acquisition.observe(1000,frame,request,true);s.state=100;s.words[1]=0xc000;frame=observer.observe(1017,owner,s,{true,-1});
  manual_physics_pose(balance,SkatePose::riding,1.0f/60,{},acquisition.preserves_history(1017,frame));
  auto oracle=expected;oracle.begin(nose);const auto correct=oracle.sample(1.0f/60,0);balance.begin(nose);const auto actual=balance.sample(1.0f/60,0);
  check(actual.elapsed==correct.elapsed&&actual.velocity==correct.velocity&&actual.position==correct.position,
      "delivered pending landing preserves physical repeat history before delayed identity without creating balance");
  request.owns=false;acquisition.observe(1034,frame,request,true);
  check(!acquisition.preserves_history(1034,frame),"withdrawn command cannot preserve history through ordinary landing indefinitely");
 }
 for(auto age:{149u,150u,151u,249u,250u,251u}){
  check(BalanceHudLease::fresh(std::chrono::milliseconds(age))==(age<=150),"HUD cannot extend the native observation lease to the former250ms stale-display interval");
 }
 check(!BalanceHudLease::fresh(std::chrono::milliseconds(-1)),"future-dated HUD provenance is unavailable");
 std::cout<<checks<<" composed physical balance lifecycle checks passed\n";
}
