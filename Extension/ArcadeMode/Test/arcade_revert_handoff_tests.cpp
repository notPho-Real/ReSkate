#include "Extension/ArcadeMode/arcade_input_session.h"
#include <array>
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
InputAdmission base(){InputAdmission a{};a.gameplay=a.may_start=a.authored=a.action_fresh=a.inspected_stance=a.manual_safe=a.manual_prepare=true;a.pose=SkatePose::riding;a.action_mode=ActionExperiment::classic;return a;}
NativeGesture::Output poll(InputSession& s,InputAdmission& a,std::uint64_t now,PhysicalInputSample p={}){a.tick=now;a.native_context={true,true,{1,1,1},now+1,now,false,a.pose==SkatePose::air,1,1};return s.poll(now,p,0,0,a);}
int main(){
 const std::array catalog{PreparedGesture{1,false,GestureContext::ground,{{0,0,-32767},{18,0,32767},{21,0,0},{24,0,0}}},PreparedGesture{2,false,GestureContext::ground,{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
 unsigned failures{};
 std::cout<<"family,age_ms,drained_first,admitted,expected\n";
 for(std::uint64_t age:{99u,100u,101u,234u,235u,236u,300u})for(bool drained:{false,true}){
  InputSession s;auto a=base();a.catalog=catalog;poll(s,a,0);poll(s,a,10,{0,0,0,0,255});
  if(drained)poll(s,a,10+age-1);
  const auto press=poll(s,a,10+age,{0x1000});const auto release=poll(s,a,60+age);
  const bool admitted=press.authored_playback && press.requested_id==1 && release.jump.setup_admitted;
  const bool expected=age>=235;
  failures+=admitted!=expected;std::cout<<"ollie,"<<age<<","<<drained<<","<<admitted<<","<<expected<<"\n";
 }
 for(int sign:{-1,1}){
  InputSession s;auto a=base();poll(s,a,0);poll(s,a,10,{0,0,0,0,255});poll(s,a,100,{0,0,static_cast<std::int16_t>(-sign*25000)});
  const auto same=poll(s,a,245,{0,0,static_cast<std::int16_t>(sign*25000)});const auto next=poll(s,a,246);
  const bool admitted=same.manual.owns && next.manual.owns && same.manual.request==sign && next.manual.request==sign;
  failures+=!admitted;std::cout<<(sign<0?"manual,":"nose-manual,")<<235<<",0,"<<admitted<<",1"<<"\n";
 }
 {
  InputSession s;auto a=base();a.catalog=catalog;poll(s,a,0);poll(s,a,10,{0,0,0,0,255});poll(s,a,100,{0,-25000});a.pose=SkatePose::air;
  const auto air=poll(s,a,110,{0x4000,-25000});a.pose=SkatePose::riding;const auto landed=poll(s,a,120);
  failures+=!air.authored_playback || landed.authored_playback;std::cout<<"flip-one-air-sample,100,0,"<<air.authored_playback<<",1"<<"\n";
 }
 return failures?1:0;
}
