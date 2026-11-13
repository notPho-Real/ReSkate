#include "Extension/ArcadeMode/arcade_wallplant_hold.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
WallplantCommand command() {
    WallplantCommand c;c.motion.ready=true;c.input_sequence=7;c.contact=3;c.issued_at=1000;
    auto& f=c.native;f.valid=f.airborne=true;f.owner={1,2,3,4,5};f.owner_generation=2;
    f.action_generation=4;f.state=201;f.sampled_at=990;f.physics_step=100;
    f.physics_seconds=1.0f/30;f.airborne_episode=6;f.air_departure=8;return c;
}
NativeInputFrame next(const WallplantCommand& c,std::uint64_t at,std::uint32_t state) {
    auto f=c.native;f.physics_step+=1+(at-1000)/33;f.sampled_at=at;f.state=state;
    f.airborne=state==200||state==201;
    if(state==100)++f.air_departure;
    if(state!=c.native.state)++f.action_generation;
    return f;
}
}
int main() {
    const auto c=command();WallplantHoldContinuity hold;
    check(hold.begin(c,1000),"valid consumed AIR command begins one phase");
    check(hold.deadline()==1160,"hold uses fixed160ms original start");
    check(hold.observe(c.native,1000),"same immutable original observation is reusable");
    auto ground=next(c,1033,100);
    check(hold.observe(ground,1033),"native contact100 carries original episode");
    check(hold.observe(ground,1040),"same-step target callbacks do not mint frames");
    check(hold.target_active(1159)&&!hold.release_due(1159),"release cannot precede160ms");
    auto later=next(c,1159,100);
    check(hold.observe(later,1159),"fresh native ground observation maintains contact hold");
    check(hold.release_due(1160)&&hold.target_active(1160),"due release retains target until the body callback consumes it");
    check(hold.target_expiry()==1194 && hold.target_active(1185),"185ms release keeps the final original motion callback anchored");
    check(hold.target_active(1194)&&!hold.target_active(1195),"target uses only the existing native dispatch grace");
    check(hold.release_due(1194)&&!hold.release_due(1195),"only one original30Hz step of dispatch grace");
    check(!hold.observe(next(c,1195,100),1195)&&!hold.active(),"late callback cancels instead of late release");
    check(hold.begin(c,1000),"new phase independent of previous cancellation");
    check(hold.observe(ground,1033),"ground handoff re-established");
    auto resumed=next(c,1066,200);++resumed.airborne_episode;++resumed.air_departure;++resumed.action_generation;
    check(hold.observe(resumed,1066),"single observed resumedAIR permitted");
    auto final_ground=next(c,1099,100);++final_ground.airborne_episode;++final_ground.air_departure;
    check(!hold.observe(final_ground,1099),"new landing cannot retain wall hold");
    for(unsigned fault=0;fault<14;++fault) {
        check(hold.begin(c,1000),"negative starts from valid phase");
        auto bad=next(c,1033,100);std::uint64_t now=1033;
        switch(fault) {
        case 0:++bad.owner.core;break;
        case 1:++bad.owner_generation;break;
        case 2:++bad.action_generation;break;
        case 3:bad.rail_family=true;break;
        case 4:bad.valid=false;break;
        case 5:bad.state=504;break;
        case 6:bad.state=103;break;
        case 7:++bad.airborne_episode;break;
        case 8:++bad.air_departure;break;
        case 9:bad.physics_step=99;break;
        case 10:bad.sampled_at=989;break;
        case 11:now=999;break;
        case 12:bad.sampled_at=1034;break;
        case 13:bad.physics_step=100;break;
        }
        check(!hold.observe(bad,now)&&!hold.active(),"changed authority/order/state cancels without release");
    }
    for(unsigned fault=0;fault<7;++fault) {
        auto bad=c;
        switch(fault) {
        case 0:bad.native.physics_seconds=0;break;
        case 1:bad.native.physics_seconds=std::numeric_limits<float>::quiet_NaN();break;
        case 2:bad.native.physics_seconds=.11f;break;
        case 3:bad.input_sequence=0;break;
        case 4:bad.contact=0;break;
        case 5:bad.native.airborne=false;break;
        case 6:bad.native.sampled_at=849;break;
        }
        check(!hold.begin(bad,1000),"malformed command cannot own hold");
    }
    check(hold.begin(c,1000),"cancellation setup");hold.cancel("off");
    check(!hold.target_active(1001)&&!hold.release_due(1160),"OFF cancellation grants neither target nor release");
    check(hold.begin(c,1000)&&hold.observe(next(c,1160,100),1160),"release bridge starts with observed contact100");
    auto bridge=hold;
    check(bridge.release_bridge(1190),"actual late scheduled release grants one original step of combo provenance");
    check(!bridge.target_active(1190)&&!bridge.release_due(1190),"combo copy never grants physical target or repeated release");
    check(bridge.observe(next(c,1224,100),1224),"combo grace measured from actual release, not original deadline");
    check(!bridge.observe(next(c,1225,100),1225),"combo grace expires after one original step");
    check(hold.release_due(1190)&&!hold.release_due(1195),"copy bridge cannot alter physical expiry");
    check(hold.begin(c,1000)&&hold.observe(ground,1033)&&hold.observe(resumed,1066),"resumed phase setup");
    check(!hold.release_bridge(1160),"already resumed AIR cannot leave a later ground-preservation receipt");
    NativeInputState tracker;
    NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
    auto tracked=c;tracked.native=tracker.observe(1000,c.native.owner,sample,{true,-1,0x1234},false,1.f/30);
    check(hold.begin(tracked,1000),"real native tracker establishes AIR hold origin");
    for(const auto state:{100u,200u,201u}) {
        sample.state=state;sample.crouch_mode=state==100?1u:2u;
        const auto at=1000u+(state==100?33u:state==200?66u:99u);
        const auto actual=tracker.observe(at,c.native.owner,sample,{true,-1,0x1234},false,1.f/30);
        check(hold.observe(actual,at),"real tracker state increments preserve exact contact and resumed AIR");
    }
    const auto changed=tracker.observe(1132,c.native.owner,sample,{true,2,0x1234},false,1.f/30);
    check(!hold.observe(changed,1132),"real tracker new same-state trick cancels hold");
    for(unsigned fault=0;fault<4;++fault) {
        NativeInputState other;sample.state=201;sample.crouch_mode=2;
        tracked.native=other.observe(1000,c.native.owner,sample,{true,-1,0x1234},false,1.f/30);
        check(hold.begin(tracked,1000),"real tracker competing selection setup");
        sample.state=100;sample.crouch_mode=1;
        NativeFlipSelection selected{true,-1,0x1234};
        if(fault==0)sample.words[0]=0x20;
        if(fault==1)selected.valid=false;
        if(fault==2)selected.id=2;
        if(fault==3)selected.object=0x5678;
        const auto actual=other.observe(1033,c.native.owner,sample,selected,false,1.f/30);
        check(!hold.observe(actual,1033),"real tracker simultaneous state and competing selection cannot hide in single generation increment");
        sample.words[0]=0;
    }
    NativeInputState reverse_tracker;sample.state=201;sample.crouch_mode=2;
    tracked.native=reverse_tracker.observe(1000,c.native.owner,sample,{true,-1,0x1234},false,1.f/30);
    check(hold.begin(tracked,1000),"real tracker reverse-state setup");
    sample.state=200;
    check(!hold.observe(reverse_tracker.observe(1033,c.native.owner,sample,{true,-1,0x1234},false,1.f/30),1033),
        "same-flight201 to200 is not the original contact reconstruction");
    auto anchored=c;anchored.anchor_verified=true;anchored.anchor={0,0,0};
    check(wallplant_hold_anchor_current(anchored,{.70104f,0,0}),"entry root accepts original maximum adjustment");
    check(!wallplant_hold_anchor_current(anchored,{.70105f,0,0}),"entry root rejects stale farther anchor before brake");
    check(!wallplant_hold_anchor_current(anchored,{.5f,.5f,0}),"entry bound is full three-dimensional displacement");
    check(!wallplant_hold_anchor_current(anchored,{std::numeric_limits<float>::quiet_NaN(),0,0}),"nonfinite root rejects");
    anchored.anchor_verified=false;
    check(!wallplant_hold_anchor_current(anchored,{0,0,0}),"unverified anchor rejects");
    std::cout<<checks<<" wallplant hold checks passed\n";
}
