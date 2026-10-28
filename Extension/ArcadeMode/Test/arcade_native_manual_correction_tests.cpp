#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_playback_packet.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool condition,const char* why){++checks;if(!condition){std::cerr<<why<<'\n';std::exit(1);}}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true};NativeRequestSample raw{};NativeInputFrame frame{};
 Pipeline(){input.enable(true);raw.valid=true;raw.state=100;raw.crouch_mode=1;raw.words[1]=0x4000;step(100);}
 void step(std::uint64_t now,int flip=-1){frame=producer.observe(now,owner,raw,{true,flip});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);}
 void admit(float correction,bool nose=false){input.publish_balance_controls(InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual),frame);input.publish_manual_correction(correction,frame,true);}
};
}
int main(){
 for(bool nose:{false,true})for(float dt:{.017f,.034f})for(float amount:{-1.f,0.f,1.f})for(unsigned age:{0u,1u,999u,1000u,1001u}){
  ManualPhysics physical;check(physical.begin(nose),"fresh actual physical Manual/Nose section begins");
  const auto result=physical.sample(dt,{amount,amount!=0,age>0,age});
  const float expected=age==0 || (age<1000&&amount*result.position>0) ? 0 : amount;
  check(result.applied_correction==expected,"native publication exposes the kernel's actual post-integration safe-period decision");
  Pipeline p;if(nose){p.raw.words[1]=0x8000;p.step(117);}p.admit(result.applied_correction,nose);
  const ManualBalanceButtons buttons{amount>0,amount<0};auto out=p.input.manual_correction(p.frame.sampled_at,buttons,true);
  check(out.owns_y&&out.y==manual_balance_native_y(expected),"qualified physical correction reaches existing native LS convention with protected zero ownership");
  out=p.input.manual_correction(p.frame.sampled_at,{},true);check(out.owns_y&&out.y==0,"physical release withdraws an old force before a later client step");
  out=p.input.manual_correction(p.frame.sampled_at,{amount<0,amount>0},true);check(out.owns_y&&out.y==0,"opposite new input cannot borrow an old post-integration force decision");
 }
 for(unsigned fault=0;fault<13;++fault){
  Pipeline p;p.admit(-1);check(p.input.manual_correction(100,{true,false},true).owns_y,"negative-control fixture starts with admitted correction");
  unsigned now=117;bool compatible=true;
  if(fault==0)p.input.enable(false);
  if(fault==1)p.input.invalidate_authority();
  if(fault==2){++p.owner.rig;p.step(now);}
  if(fault==3){p.raw.words[1]=0x8000;p.step(now);}
  if(fault==4)p.step(now,7);
  if(fault==5){p.raw.state=200;p.step(now);}
  if(fault==6){p.raw.state=103;p.step(now);}
  if(fault==7){p.raw.words[0]=0x40;p.step(now);}
  if(fault==8){now=251;p.step(now);}
  if(fault==9)p.input.cancel_manuals();
  if(fault==10)compatible=false;
  if(fault==11)p.input.publish_manual_correction(.5f,p.frame,true);
  if(fault==12)p.input.publish_manual_correction(-1,p.frame,false);
  check(!p.input.manual_correction(now,{true,false},compatible).owns_y,"OFF/focus/owner/contact/action/hop/air/push/stale/cancel/competing/invalid/closed contexts relinquish native correction");
 }
 {
  Pipeline p;p.admit(1);p.step(250);check(p.input.manual_correction(250,{true,true},true).y==32767,"both configured controls choose A and retain original inclusive150ms lease");
  p.step(251);check(!p.input.manual_correction(251,{true,true},true).owns_y,"a new physical frame cannot renew old effective force");
 }
 {
  PlaybackPacket packet;const auto first=packet.sample(7,7,true);const auto second=packet.sample(7,7,true);const auto restored=packet.sample(7,second,false);
  check(first!=second&&restored!=second,"native correction ownership and restoration remain visible with an unchanged physical packet");
 }
 std::cout<<checks<<" native manual correction checks passed\n";
}
