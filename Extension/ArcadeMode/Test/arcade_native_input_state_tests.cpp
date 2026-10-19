#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
NativeRequestSample sample(unsigned state=100,unsigned contacts=0xc000){NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=1;s.words[1]=contacts;return s;}
}
int main(){
    NativeInputOwner owner{1,2,3,4,5};NativeInputState state;
    const auto first=state.observe(1000,owner,sample(),{true,-1});
    check(first.valid&&first.owner_generation&&first.contact_generation&&first.action_generation&&first.physics_step==1,"actual owner/contact/selection establish one native generation");
    const auto next=state.observe(1017,owner,sample(),{true,-1});
    check(next.owner_generation==first.owner_generation&&next.contact_generation==first.contact_generation&&next.action_generation==first.action_generation&&next.physics_step==2,"repeated native reads are not fresh action occurrences");
    for(unsigned i=0;i<20;++i)check(state.correlate(1020,owner,sample()).physics_step==2,"input/client consumers cannot manufacture physics steps");
    auto contact=state.observe(1034,owner,sample(100,0x4000),{true,-1});
    check(contact.contact_generation>next.contact_generation&&contact.action_generation==next.action_generation,"wheel change invalidates contact correlation without inventing a trick");
    auto action=state.observe(1051,owner,sample(100,0x4000),{true,2});
    check(action.action_generation>contact.action_generation&&action.contact_generation==contact.contact_generation,"real selection change advances action invalidation separately");
    check(!state.correlate(1060,owner,sample()).valid,"client contact from a different episode cannot use the current physics frame");
    check(!state.correlate(1060,owner,sample(200,0x4000)).valid,"client state mismatch cannot cross native contexts");
    auto mode=sample(100,0x4000);mode.crouch_mode=0;
    check(!state.correlate(1060,owner,mode).valid,"client mode mismatch rejects correlation");
    check(state.correlate(1201,owner,sample(100,0x4000)).valid&&!state.correlate(1202,owner,sample(100,0x4000)).valid,"native snapshot lease is inclusive150ms");
    auto changed_stance=sample(100,0x4000);changed_stance.words[0]=0x20;
    check(!state.correlate(1060,owner,changed_stance).valid,"stance change cannot borrow a previous native action context");
    changed_stance.words[0]=0x40;
    check(!state.correlate(1060,owner,changed_stance).valid,"native push transition cannot borrow a prior wheel-contact snapshot");
    check(!state.correlate(1050,owner,sample(100,0x4000)).valid,"future physics frame cannot authorize an earlier consumer");
    for(unsigned field=0;field<5;++field){auto changed=owner;switch(field){case 0:++changed.base;break;case 1:++changed.entity;break;case 2:++changed.core;break;case 3:++changed.context;break;default:++changed.rig;break;}check(!state.correlate(1060,changed,sample(100,0x4000)).valid,"every owner link participates in correlation");}
    for(unsigned rail:{400u,401u,402u,403u,404u,499u}){const auto r=state.observe(1300+rail,owner,sample(rail),{});check(r.valid&&r.rail_family&&!r.flip_known,"verified native400 family publishes rail exclusion without invented enum semantics");}
    for(unsigned nonrail:{100u,200u,399u,500u,701u}){const auto r=state.observe(2000+nonrail,owner,sample(nonrail),{});check(r.valid&&!r.rail_family,"nonrail states never inherit rail contact");}
    const auto before=state.observe(3000,owner,sample(),{true,-1});state.invalidate();
    check(!state.correlate(3001,owner,sample()).valid,"explicit loss invalidates previously fresh context");
    const auto after=state.observe(3017,owner,sample(),{true,-1});check(after.owner_generation>before.owner_generation&&after.contact_generation>before.contact_generation&&after.action_generation>before.action_generation,"reacquisition changes every episode generation");
    const auto bail=state.observe(3034,owner,sample(),{true,-1},true);check(bail.owner_generation>after.owner_generation,"wipeout invalidates same-address ownership episode");
    const auto gap=state.observe(3200,owner,sample(),{true,-1});check(gap.owner_generation>bail.owner_generation,"observation gap cannot bridge action/contact correlation");
    check(!state.observe(3217,{},sample(),{}).valid&&!state.correlate(3218,owner,sample()).valid,"invalid owner clears all published context");
    std::cout<<checks<<" shared native input state checks passed\n";
}
