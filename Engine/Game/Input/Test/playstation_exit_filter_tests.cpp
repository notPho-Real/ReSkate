#include "Engine/Game/Input/playstation_report.h"
#include <array>
#include <cstdlib>
#include <iostream>
using namespace dingosdk;
namespace {unsigned checks{};void check(bool v,const char* text){++checks;if(!v){std::cerr<<text<<'\n';std::exit(1);}}}
int main(){
 struct Layout{PlayStationPad kind;std::uint8_t id;std::size_t size,stick,left,right,shoulder;std::uint32_t crc;};
 for(const auto l:{Layout{PlayStationPad::dualshock4,1,64,1,8,9,6,0},Layout{PlayStationPad::dualshock4,0x11,78,3,10,11,8,0x62b89fdd},
   Layout{PlayStationPad::dualsense,1,64,1,5,6,9,0},Layout{PlayStationPad::dualsense,0x31,78,2,6,7,10,0x6ae46ece},Layout{PlayStationPad::dualsense,1,78,1,8,9,6,0}}){
  std::array<std::uint8_t,80> bytes;bytes.fill(0xa5);auto* report=bytes.data()+1;report[0]=l.id;report[l.stick]=10;report[l.left]=100;report[l.right]=200;report[l.shoulder]=0x2c;
  const auto before=bytes;
  check(!filter_playstation_exit_report(l.kind,report,l.size,[](auto&){}),"inactive filter preserves every report byte including CRC");check(bytes==before,"no-op callback has no writeback");
  unsigned calls{};
  check(filter_playstation_exit_report(l.kind,report,l.size,[&](auto& p){++calls;check(p.left_trigger==100&&p.right_trigger==200,"callback receives correctly parsed physical trigger values");p.left_trigger=0;p.right_trigger=55;p.left_x=0;p.buttons=0;p.left_y=32767;p.right_x=32767;p.right_y=-32768;}),"existing callback can alter owned HID exit controls");
  check(calls==1&&report[l.left]==0&&report[l.right]==55&&report[l.stick]==128,"one callback updates only documented trigger and left-X locations");
  check(report[l.shoulder]==0x28,"zero analog trigger clears its digital bit without changing other buttons");
  for(std::size_t i=0;i<l.size;++i){
   const bool changed=i==l.left||i==l.right||i==l.stick||i==l.shoulder||(l.crc&&i>=74);
   if(!changed)check(report[i]==before[i+1],"unowned report fields, sequence and sensor data preserved");
  }
  check(bytes.front()==0xa5&&bytes[l.size+1]==0xa5,"USB/BT writeback stays inside the completed report");
  if(l.crc){
   // Independent fixture CRCs produced with zlib over prefix0xA1+74 bytes.
   for(unsigned i=0;i<4;++i)check(report[74+i]==std::uint8_t(l.crc>>(8*i)),"full Bluetooth report has exact independent CRC fixture");
  }
  check(filter_playstation_exit_report(l.kind,report,l.size,[](auto& p){p.right_trigger=0;p.left_x=-32768;}),"second changed owned sample accepted");
  check((report[l.shoulder]&0x0c)==0&&report[l.stick]==1,"both released trigger bits clear and source left-X quantization preserved");
  const auto after=bytes;
  check(!filter_playstation_exit_report(l.kind,report,l.size,[](auto& p){p.buttons=0;p.left_y=12345;}),"unsupported channel changes alone never mutate the report");check(bytes==after,"unowned changes preserve exact bytes");
  for(std::size_t size=0;size<(l.id==1?10u:12u);++size){unsigned invoked{};const auto original=bytes;
   check(!filter_playstation_exit_report(l.kind,report,size,[&](auto&){++invoked;}),"short malformed reports cannot reach callback");
   check(!invoked&&bytes==original,"invalid reports perform no mutation");
  }
 }
 std::array<std::uint8_t,78> unknown{};unknown[0]=0x7f;unsigned invoked{};
 check(!filter_playstation_exit_report(PlayStationPad::dualsense,unknown.data(),78,[&](auto&){++invoked;}),"unknown report ID rejected");
 check(!filter_playstation_exit_report(PlayStationPad::none,unknown.data(),78,[&](auto&){++invoked;}),"unknown pad rejected");
 check(!filter_playstation_exit_report(PlayStationPad::dualsense,nullptr,78,[&](auto&){++invoked;})&&!invoked,"absent report rejected before callback");
 std::cout<<checks<<" native PlayStation exit filter checks passed\n";
}
