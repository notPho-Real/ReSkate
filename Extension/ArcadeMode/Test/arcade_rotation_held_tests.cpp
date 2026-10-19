#include "Extension/ArcadeMode/arcade_rotation.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
AirRotation::Output poll(AirRotation& spin,std::uint64_t now,std::uint16_t held,SkatePose pose=SkatePose::air,bool conflict=false,bool enabled=true){return spin.poll(now,held,enabled,pose,false,false,conflict,true);}
}
int main(){
 for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)for(auto side:{BoardChord::lb,BoardChord::rb}){
  const float expected=side==BoardChord::lb?-7.75f:7.75f;AirRotation spin;poll(spin,0,0);
  auto out=poll(spin,phase+cadence,side);check(out.owns_left&&out.yaw_rate==expected&&out.x==0&&out.y==0,"first eligible held sample selects rate immediately");
  check(!poll(spin,phase+2*cadence,side,SkatePose::riding).owns_left,"ground sample has no air rotation");
  out=poll(spin,phase+3*cadence,side);check(out.owns_left&&out.yaw_rate==expected,"fresh eligible air resumes currently held shoulder after ground");
  check(!poll(spin,phase+4*cadence,BoardChord::pair).owns_left,"opposing shoulders select no direction");
  out=poll(spin,phase+5*cadence,side);check(out.owns_left&&out.yaw_rate==expected,"remaining shoulder selects rate without new press");
  check(!poll(spin,phase+6*cadence,side,SkatePose::air,true).owns_left,"real reservation suppresses rate");
  out=poll(spin,phase+7*cadence,side);check(out.owns_left&&out.yaw_rate==expected,"cleared temporary reservation permits current held direction");
  check(!poll(spin,phase+8*cadence,0).owns_left,"release immediately outputs zero");
 }
 for(unsigned boundary=0;boundary<7;++boundary){
  AirRotation spin;poll(spin,0,0);check(poll(spin,10,BoardChord::rb).owns_left,"baseline before lifecycle boundary");
  if(boundary==0)poll(spin,20,BoardChord::rb,SkatePose::air,false,false);
  if(boundary==1)poll(spin,20,BoardChord::rb,SkatePose::bail);
  if(boundary==2)poll(spin,20,BoardChord::rb,SkatePose::unknown);
  if(boundary==3)poll(spin,20,static_cast<std::uint16_t>(BoardChord::rb|0x0010u));
  if(boundary==4)poll(spin,20,static_cast<std::uint16_t>(BoardChord::rb|0x0020u));
  if(boundary==5)spin.reset();
  if(boundary==6)poll(spin,9,BoardChord::rb);
  check(!poll(spin,30,BoardChord::rb).owns_left,"owner/focus/menu/bail/OFF/rollback requires renewed physical baseline");
  poll(spin,40,0);check(poll(spin,50,BoardChord::rb).owns_left,"neutral baseline then new held control rearms safely");
 }
 AirRotation startup;check(!poll(startup,0,BoardChord::rb).owns_left,"unknown startup held input cannot establish baseline");
 AirRotation transition;poll(transition,0,0);poll(transition,10,BoardChord::pair,SkatePose::unknown);check(!poll(transition,20,BoardChord::rb).owns_left,"actually admitted unavailable context transition invalidates held baseline");
 AirRotation fallback;fallback.poll(0,0,true,SkatePose::air,false,false);check(!fallback.poll(10,BoardChord::rb,true,SkatePose::air,false,false).owns_left,"unsupported native rate fallback retains established chord wait");
 check(fallback.poll(131,BoardChord::rb,true,SkatePose::air,false,false).owns_left,"existing LS fallback available after its approved wait");
 std::cout<<checks<<" continuous held rotation checks passed\n";
}
