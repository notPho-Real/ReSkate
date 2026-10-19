#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool c,const char* why){++checks;if(!c){std::cerr<<why<<'\n';std::exit(1);}}
struct Bridge{
 NativeInputState state;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample raw{};NativeInputFrame frame{};ArcadeInput input{true};
 Bridge(){input.enable(true);raw.valid=true;raw.state=100;raw.crouch_mode=1;raw.words[1]=0x4000;step(1000);}
 void step(std::uint64_t now){frame=state.observe(now,owner,raw,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);}
 void controls(InputBalanceControls c=InputBalanceControls::vertical()){input.publish_balance_controls(c,frame);}
};
}
int main(){
 for(bool nose:{false,true})for(unsigned buttons:{0u,1u,2u,3u}){
  Bridge b;if(nose){b.raw.words[1]=0x8000;b.step(1017);}const auto now=b.frame.sampled_at;b.controls(InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual));
  b.input.observe_physical(0,{true,now,static_cast<std::uint16_t>(buttons),12000,0});
  const auto pair=b.input.configured_balance(now);check(pair.available&&pair.sampled_at==now&&pair.first==bool(buttons&1u)&&pair.second==bool(buttons&2u),"typed configured components retain all A/B held combinations independently of canonical command direction");
  BalanceControl control;control.poll_manual(now,.1f,{pair.first,pair.second},pair.available);
  const auto effect=control.poll_manual(now+1000,.1f,{pair.first,pair.second},pair.available);
  check(effect==(buttons&1u?-1.f:buttons&2u?1.f:0.f),"configured components reach the unchanged digital A-first kernel");
 }
 {
  Bridge b;InputBalanceControls custom{InputBalanceKind::manual,{InputSource::buttons,8,InputEdge::press},{InputSource::triggers,1,InputEdge::press},true};b.controls(custom);
  b.input.observe_physical(0,{true,1000,0x100,0,0,0,0,0,255});auto pair=b.input.configured_balance(1000);
  check(pair.available&&pair.first&&pair.second,"configured tokens consume actual button/trigger levels without projection history");
  b.step(1100);check(b.input.configured_balance(1100).sampled_at==1000,"new native observations cannot retimestamp an old physical sample");
  b.step(1150);check(b.input.configured_balance(1150).available,"original physical and component leases include150ms");
  b.step(1151);b.controls(custom);check(!b.input.configured_balance(1151).available,"fresh native component publication cannot renew an expired physical input");
 }
 for(unsigned fault=0;fault<5;++fault){
  Bridge b;b.controls();b.input.observe_physical(0,{true,1000,1,0,0});check(b.input.configured_balance(1000).available,"bridge negative fixture has original phase and physical proof");
  if(fault==0)b.input.invalidate_authority();
  if(fault==1)b.input.enable(false);
  if(fault==2){++b.owner.entity;b.step(1017);b.controls();}
  if(fault==3)b.input.observe_physical(0,{});
  if(fault==4)b.input.publish_balance_controls({},b.frame);
  check(!b.input.configured_balance(1017).available,"focus/OFF/owner/missing physical/missing component cannot supply configured states");
 }
 for(bool nose:{false,true})for(bool admitted:{false,true})for(std::uint16_t buttons:{std::uint16_t{3},std::uint16_t{12}}){
  ManualActuation actuator;const int direction=nose?1:-1;const auto pose=nose?SkatePose::nose_manual:SkatePose::manual;
  check(actuator.request(1000,direction,false,true,false,1000),"native balance actuator request starts through its existing API");
  check(actuator.sample(1017,true,true,pose,false,0,admitted).owns_stick,"native balance fixture has matching independently observed contact");
  const auto output=actuator.sample(1034,true,true,pose,false,buttons,admitted);
  check(output.owns_stick==(admitted&&buttons==3),"only independently admitted vertical balance controls preserve RS; other opposed command cases still cancel");
 }
 for(bool admitted:{false,true})for(bool trigger_only:{false,true}){
  ManualActuation actuator;actuator.request(1000,-1,false,true,false,1000);actuator.sample(1017,true,true,SkatePose::manual,false,0,admitted);
  check(actuator.sample(1034,true,true,SkatePose::manual,true,0,admitted,trigger_only).owns_stick==(admitted&&trigger_only),"only an independently admitted balance can keep RS through an unsupported trigger-only request; preparation and real channel conflicts cancel");
 }
 std::cout<<checks<<" configured balance bridge checks passed\n";
}
