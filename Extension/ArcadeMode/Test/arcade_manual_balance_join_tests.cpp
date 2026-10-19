#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_push.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {unsigned checks{};void check(bool x,const char*w){++checks;if(!x){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}}
int main(){
 for(bool nose:{false,true})for(bool activity:{false,true})for(auto raw:{SkatePose::riding,SkatePose::unknown}){
  ManualObservation identity;ManualObservationContext c{1000,1,true,true,activity,false,false,SkatePose::riding};
  ScoreLabelOccurrence baseline{true,0,0,0,0,ScoreLabelUncertainty::none};identity.poll(SkatePose::air,baseline,c);
  c.now=1017;c.contact=nose?SkatePose::nose_manual:SkatePose::manual;
  ScoreLabelOccurrence label{true,score_label_bit(nose?2:1),1,1017,1017,ScoreLabelUncertainty::none};
  const auto admitted=identity.poll(raw,label,c);
  check(admitted.confirmed&&admitted.admitted&&admitted.pose==c.contact,"fresh independent history and exact native contact admit balance without generic activity or name presentation");
  ManualPhysics physics;physics.begin(nose);const auto physical=physics.sample(1.0f/60,0);
  ComboSession combo;combo.step(SkatePose::air,0,0);combo.step(admitted.pose,1.0f/60,0,false,&physical);
  check(balance_meter(combo.balance_display_state()).visible,"physical balance activation publishes a meter on the same admitted client update");
 }
 for(bool nose:{false,true})for(unsigned delay:{0u,1u,17u,34u,149u,150u,151u})for(bool first:{false,true}) {
  ManualObservation identity;ManualObservationContext c{1000,1,true,true,false,false,false,SkatePose::riding};
  ScoreLabelOccurrence empty{true,0,0,0,0,ScoreLabelUncertainty::none};identity.poll(SkatePose::air,empty,c);
  const auto wanted=nose?SkatePose::nose_manual:SkatePose::manual;
  ScoreLabelOccurrence edge{true,score_label_bit(nose?2:1),1,1017,1017,ScoreLabelUncertainty::none};
  c.now=1017;c.contact=first?SkatePose::unknown:wanted;
  identity.poll(first?SkatePose::air:SkatePose::unknown,first?edge:empty,c);
  for(unsigned age=17;age<delay;age+=17) {c.now=1017+age;identity.poll(first?SkatePose::air:SkatePose::unknown,first?ScoreLabelOccurrence{true,0,1,1017,c.now,ScoreLabelUncertainty::none}:empty,c);}
  c.now=1017+delay;c.contact=wanted;
  if(!first)edge.observed_at=edge.sampled_at=c.now;
  auto result=identity.poll(SkatePose::unknown,first?ScoreLabelOccurrence{true,0,1,1017,c.now,ScoreLabelUncertainty::none}:edge,c);
  check(result.admitted==(!first||delay<=150),"both-order native identity/contact join uses fresh original evidence without a generic-activity dependency or deadline renewal");
 }
 for(bool nose:{false,true})for(unsigned age:{0u,199u,200u,201u}) {
  ManualObservation identity;NativePushContact push;
  ManualObservationContext c{1000,1,true,true,false,false,false,SkatePose::unknown,true};
  ScoreLabelOccurrence old{true,score_label_bit(nose?2:1),1,1000,1000,ScoreLabelUncertainty::none};
  push.poll(1000,true,100,1,0x4000,true,false);
  check(!identity.poll(nose?SkatePose::nose_manual:SkatePose::manual,old,c).confirmed,"actual native push and history cannot create balance");
  const auto contacts=nose?0x8000u:0x4000u;c.pushing=false;c.contact=nose?SkatePose::nose_manual:SkatePose::manual;
  for(unsigned elapsed=50;elapsed<age;elapsed+=50){c.now=1000+elapsed;identity.poll(SkatePose::unknown,{true,0,1,1000,c.now,ScoreLabelUncertainty::none},c);}
  c.now=1001+age;
  const bool push_permission=push.poll(c.now,true,100,1,contacts,false,false);
  check(push_permission==(age<200),"existing native recovery policy retains its exact200ms bound");
  ScoreLabelOccurrence fresh{true,score_label_bit(nose?2:1),2,c.now,c.now,ScoreLabelUncertainty::none};
  check(identity.poll(SkatePose::unknown,fresh,c).admitted,"fresh post-push identity/contact outranks mere push eligibility without admitting an active push");
 }
 for(unsigned fault=0;fault<7;++fault) {
  ManualObservation identity;ManualObservationContext c{1000,1,true,true,false,false,false,SkatePose::riding};
  identity.poll(SkatePose::air,{true,0,0,0,0,ScoreLabelUncertainty::none},c);
  c.now=1017;c.contact=SkatePose::manual;ScoreLabelOccurrence edge{true,score_label_bit(1),1,1017,1017,ScoreLabelUncertainty::none};
  if(fault==0)edge.sampled_at=1018;
  if(fault==1)edge.uncertainty=ScoreLabelUncertainty::coalesced;
  if(fault==2)edge.sequence=2;
  if(fault==3)c.owner_changed=true;
  if(fault==4)c.valid=false;
  if(fault==5)c.gap=true;
  if(fault==6)edge.labels=score_label_bit(2);
  check(!identity.poll(SkatePose::unknown,edge,c).confirmed,"future/coalesced/skipped/wrong-owner/invalid/gap/opposite identity cannot activate physical balance");
 }
 std::cout<<checks<<" manual balance evidence join checks passed\n";
}
