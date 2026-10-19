#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool v,const char* message){++checks;if(!v){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
struct Fixture {
 NativeInputState producer; NativeInputOwner owner{1,2,3,4,5}; NativeRequestSample sample{};
 ArcadeInput input{true}; NativeInputFrame frame{};
 Fixture(){input.enable(true);sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=0x4000;observe(1000);}
 void observe(std::uint64_t now){frame=producer.observe(now,owner,sample,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);}
 void publish(InputBalanceKind kind,NativeInputFrame proof){input.publish_balance_controls(InputBalanceControls::vertical(kind),proof);}
 bool admitted(InputBalanceKind kind,std::uint64_t now=1000){return input.balance_admission(now,kind).valid;}
 void poll(std::uint64_t now,short y){observe(now);input.poll(0,now,0,0,y,0,0,true,false,false,0,false,false,false,now/17,false,{},false,false,SkatePose::riding);}
 bool up_recorded()const{for(const auto& e:input.history(0)->events())if(e.token==InputToken{InputSource::direction,8,InputEdge::press})return true;return false;}
};
}
int main(){
 // Typed fixtures exercise the existing publisher contract; they do not map
 // native Grind/Slide/Lip actors or infer a phase from generic rail contact.
 for(auto kind:{InputBalanceKind::manual,InputBalanceKind::nose,InputBalanceKind::grind,InputBalanceKind::slide,InputBalanceKind::lip}){
  Fixture good;good.publish(kind,good.frame);check(good.admitted(kind),"matching original owner/contact/action proof admitted");
  good.observe(1017);check(good.admitted(kind,1017),"new unchanged physical observation preserves original admission");
  for(unsigned fault=0;fault<12;++fault){
   Fixture f;auto proof=f.frame;
   switch(fault){
    case 0:++proof.owner.base;break;case 1:++proof.owner.entity;break;case 2:++proof.owner.core;break;
    case 3:++proof.owner.context;break;case 4:++proof.owner.rig;break;
    case 5:proof.owner={};break;case 6:++proof.owner_generation;break;
    case 7:++proof.contact_generation;break;case 8:++proof.action_generation;break;
    case 9:proof.physics_step=0;break;case 10:proof.physics_step+=100;break;case 11:proof.sampled_at=2000;break;
   }
   f.publish(kind,proof);check(!f.admitted(kind),"foreign owner/generation, missing or future physical proof cannot authorize phase");
   f.poll(1017,0);f.poll(1034,25000);
   check(f.up_recorded()==(fault!=8),"invalid owner/contact/order proof cannot suppress input; action-only invalidation preserves physical correction exclusion");
  }
  Fixture lease;lease.publish(kind,lease.frame);
  lease.observe(1150);check(lease.admitted(kind,1150),"original admission includes150ms endpoint");
  lease.observe(1151);check(!lease.admitted(kind,1151),"new physics cannot renew expired component proof");
 }
 std::cout<<checks<<" native balance proof checks passed\n";
}
