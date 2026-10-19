#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool x,const char*w){++checks;if(!x){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}
struct Pipeline {
 NativeInputState physics;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true};NativeInputFrame frame{};
 Pipeline(){input.enable(true);}
 void step(std::uint64_t now,unsigned state,bool deliver=true){
  NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=1;
  frame=physics.observe(now,owner,s,{true,-1});if(deliver)input.publish_native_context(frame);
  input.publish_authority(now,frame.owner_generation,1,true);
 }
};
}
int main(){
 for(auto method:{RailAdapterMethod::align,RailAdapterMethod::nose_pressure,RailAdapterMethod::tail_pressure}) {
  Pipeline p;p.step(1000,200);
  check(p.input.native_rail_phase(1000)==RailAdapterPhase::air,"actual physics frame reaches the bound facade rail observer");
  check(p.input.request_native_rail(method,1000,1200,true)==RailAdapterResult::started,"explicit admitted semantic method can enter the cached native adapter");
  auto out=p.input.sample_native_rail(1000,true,true);
  check(out.owns_right_stick&&out.right_x==0&&out.right_y==(method==RailAdapterMethod::nose_pressure?20000:method==RailAdapterMethod::tail_pressure?-20000:0)&&!out.button_mask,
      "generic alignment/pressure owns only the established native right-stick shape");
  for(auto state:{401u,403u,200u}){p.step(p.frame.sampled_at+17,state);check(p.input.sample_native_rail(p.frame.sampled_at,true,true).owns_right_stick,"every actual AIR/rail observation retains compatible generic pressure through native contact and pop");}
  p.step(1068,100);check(!p.input.sample_native_rail(1068,true,true).owns_right_stick,"actual ground ends adapter ownership before any input poll requests output");
  p.step(1085,200);check(!p.input.sample_native_rail(1085,true,true).owns_right_stick,"held input cannot recreate a cancelled semantic actor");
 }
 for(unsigned fault=0;fault<9;++fault) {
  Pipeline p;p.step(1000,200);p.input.request_native_rail(RailAdapterMethod::nose_pressure,1000,1300,true);
  if(fault==0)p.input.cancel_commands();
  if(fault==1)p.input.enable(false);
  if(fault==2)p.input.invalidate_authority();
  if(fault==3){++p.owner.entity;p.step(1017,200);}
  if(fault==4){p.step(1017,100,false);p.step(1034,200);}
  if(fault==5)p.input.publish_native_context({});
  if(fault==6)p.step(1017,403);
  const auto now=fault==7?1151u:static_cast<unsigned>(p.frame.sampled_at);
  const bool compatible=fault!=6,held=fault!=8;
  check(!p.input.sample_native_rail(now,held,compatible).owns_right_stick,"command/OFF/authority/owner/skipped-step/invalid/conflict/stale/release loss cancels the adapter");
 }
 {
  Pipeline p;p.step(1000,200);
  check(p.input.request_native_rail(RailAdapterMethod::nose_pressure,1000,1000,false)==RailAdapterResult::busy,"missing explicit compatible-channel admission cannot activate a native method");
  check(p.input.request_native_rail(RailAdapterMethod::named_grind,1000,1200,true)==RailAdapterResult::unsupported&&
        p.input.request_native_rail(RailAdapterMethod::lip,1000,1200,true)==RailAdapterResult::unsupported,"unmapped named and Lip requests cannot fall back to generic pressure");
  p.input.request_native_rail(RailAdapterMethod::tail_pressure,1000,1100,true);
  p.step(1100,200);check(p.input.sample_native_rail(1100,true,true).owns_right_stick,"original semantic absolute deadline is inclusive");
  p.step(1101,200);check(!p.input.sample_native_rail(1101,true,true).owns_right_stick,"native observations cannot renew an expired semantic deadline");
  p.step(1118,200);check(!p.input.sample_native_rail(1118,true,true).owns_right_stick,"actor expiry requires a genuinely new consumed request");
 }
 {
  Pipeline p;p.step(1000,200);p.input.request_native_rail(RailAdapterMethod::dark_catch,1000,1200,true);
  const auto candidate=p.input.sample_native_rail(1000,true,true);
  check(!candidate.owns_right_stick&&candidate.button_mask==BoardChord::rb&&candidate.button_values==BoardChord::rb,"explicit compatible dark-catch candidate owns only its native RB mask");
  p.step(1017,401);check(!p.input.sample_native_rail(1017,true,true).button_mask,"dark-catch candidate cannot persist past its admitted AIR context");
 }
 std::cout<<checks<<" native rail facade wiring checks passed\n";
}
