#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_playback_packet.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool v,const char* m){++checks;if(!v){std::cerr<<m<<'\n';std::exit(1);}}
struct Pipeline {
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true,InputPolicy::strict()};NativeInputFrame frame{};
 NativeRequestSample sample{};NativeGesture::Output out{};RailAdapterPad pad{};PlaybackPacket packet;std::uint32_t serial{};
 Pipeline(){input.enable(true);sample.valid=true;sample.state=200;sample.crouch_mode=1;}
 void poll(std::uint64_t now,std::uint16_t buttons,short direction=0,bool compatible=true,bool external=false){
  frame=producer.observe(now,owner,sample,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
  out=input.poll(0,now,buttons,0,direction,111,-222,true,false,false,0,false,false,false,now/17,false,{},false,false,SkatePose::air,false,false,false,external);
  pad=input.sample_native_rail(out.rail,now,compatible);
  if(pad.owns_right_stick){out.right_x=pad.right_x;out.right_y=pad.right_y;}
  serial=packet.sample(7,7^synthetic_packet_mask(out.phase,out.buttons!=buttons||out.right_x!=111||out.right_y!=-222),pad.owns_right_stick);
 }
};
}
int main(){
 for(unsigned cadence:{17u,34u})for(short direction:{short{-25000},short{0},short{25000}}){
  Pipeline p;p.poll(1000,0);p.poll(1000+cadence,0x8000,direction);
  const auto request=p.out.rail.request;check(p.pad.owns_right_stick,"actual producer/facade semantic rail handoff owns RS");
  check(p.out.right_x==0&&p.out.right_y==(direction<0?-20000:direction>0?20000:0),"only mapped generic pressure delivered");
  check(!(p.out.buttons&0x8000)&&p.out.command_busy,"existing Y consumption and semantic busy retained");
  for(unsigned i=2;i<18;++i){p.sample.state=i<5?200u:401u;p.poll(1000+i*cadence,0x8000,direction);check(p.pad.owns_right_stick&&p.out.rail.request==request,"immutable ticket continues across AIR/rail after first issue deadline");}
  const auto owned=p.serial;p.poll(1000+18*cadence,0);check(!p.pad.owns_right_stick&&p.out.right_x==0&&p.out.right_y==0,"release restores the existing gameplay RS/camera baseline");
  check(p.serial!=owned,"restoration is visible with unchanged physical packet");
 }
 for(unsigned delay:{199u,200u,201u}){
  Pipeline p;p.poll(1000,0);p.poll(1010,0x8000,0,false);auto request=p.out.rail.request;
  p.poll(1110,0x8000,0,false);p.poll(1010+delay,0x8000);
  check(p.pad.owns_right_stick==(delay<200)&&p.out.rail.request==request,"final native admission cannot renew original exclusive readiness deadline");
 }
 for(unsigned fault=0;fault<7;++fault){
  Pipeline p;p.poll(1000,0);p.poll(1017,0x8000);check(p.pad.owns_right_stick,"cancellation starts from actually issued ownership");
  if(fault==0)p.input.enable(false);
  if(fault==1)p.owner.rig=9;
  if(fault==2)p.sample.state=100;
  if(fault==3)p.sample.state=700;
  if(fault==4)p.producer.invalidate();
  p.poll(1034,0x8000,0,fault!=5,fault==6);
  check(!p.pad.owns_right_stick,"OFF/owner/ground/bail/continuity/native-owner/input-owner conflict releases RS");
  p.poll(1051,0x8000);check(!p.pad.owns_right_stick,"cancelled held ticket cannot resume without new recognition");
 }
 std::cout<<checks<<" actual native rail runtime composition checks passed\n";
}
