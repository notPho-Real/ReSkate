#include "Extension/ArcadeMode/arcade_stance_input.h"
#include <cstdio>
#include <cstdlib>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
InputNativeContext frame(std::uint64_t at=10,std::uint64_t step=1,unsigned stance=0) {
    InputNativeContext value{true,true,{1,1,1},step,at};
    value.flip={true,-1,0x10000,100,1,stance,step,1,1};return value;
}
TriggerPress press(InputNativeContext native=frame()) {
    return {{{InputSource::triggers,1,InputEdge::press},10,1},{},InputContext::ground,
        native,TriggerOwner::switch_stance,160};
}
void original_edge_and_delivery() {
    for(unsigned stance:{0u,0x20u}) {
        StanceInputLease lease;auto native=frame(10,1,stance);auto input=press(native);
        input.released=true;input.release={{InputSource::triggers,1,InputEdge::release},10,2};
        check(lease.start(input,native,10),"original RT can initiate a native gesture even after its tap release");
        check(!lease.observed_change()&&!lease.was_issued(),"queued input proves neither delivery nor native stance");
        check(lease.issued(native,10),"adapter acknowledges the original gesture once");
        check(!lease.issued(native,10),"duplicate controller polls cannot repeat native issuance");
        check(lease.observe(frame(20,2,stance),20,true),"RT release does not cancel the already initiated native actor");
        auto changed=frame(30,3,stance^0x20u);++changed.generations.action;
        check(!lease.observe(changed,30,true)&&lease.observed_change(),"fresh native leading-foot change completes either stance direction");
        check(!lease.start(input,changed,30),"completed RT sequence cannot replay");
    }
    for(unsigned fault=0;fault<3;++fault) {
        StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"negative starts owned input");
        if(fault)check(lease.issued(frame(),10),"negative supplies gesture before impossible observation");
        auto changed=frame(fault==2?10u:20u,fault==1?1u:2u,0x20);
        check(!lease.observe(changed,20,true)&&!lease.observed_change(),
            "pre-existing, duplicate-step or pre-delivery-time bit changes are not action proof");
    }
}
void interruptions() {
    for(unsigned fault=0;fault<11;++fault) {
        StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"interruption starts lease");
        check(lease.issued(frame(),10),"interruption begins after delivery");
        auto next=frame(20,2);auto now=20u;
        if(fault==0)next.required=false;
        if(fault==1)++next.generations.owner;
        if(fault==2)++next.generations.contact;
        if(fault==3)++next.flip.object;
        if(fault==4)next.airborne=true;
        if(fault==5)next.rail_family=true;
        if(fault==6)next.flip.stance=0x40;
        if(fault==7)next.flip.state=602;
        if(fault==8)now=9;
        if(fault==9)now=171;
        check(!lease.observe(next,now,fault!=10)&&!lease.observed_change(),"owner, contact, stale clock, OFF and incompatible native contexts cancel");
        check(!lease.start(input,frame(),10),"cancelled physical sequence cannot revive");
    }
    StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"waiting request starts");
    check(!lease.observe(frame(161,2),161,true),"waiting request cannot renew its physical edge deadline");
    input.press.sequence=3;input.press.at=170;input.deadline=320;input.native=frame(170,3);
    check(lease.start(input,input.native,170),"fresh physical edge remains available after retirement");
    check(lease.issued(input.native,170),"completion ceiling has an issued native gesture");
    for(unsigned at=180;at<=1170;at+=10)
        check(lease.observe(frame(at,at),at,true),"fresh samples may observe completion without another controller press");
    check(!lease.observe(frame(1171,1171),1171,true)&&!lease.observed_change(),
        "unacknowledged native request expires without a false stance success or another press");
}
void denied_sources() {
    for(unsigned fault=0;fault<10;++fault) {
        StanceInputLease lease;auto input=press();auto native=frame();
        if(fault==0)input.owner=TriggerOwner::landing_revert;
        if(fault==1)input.context=InputContext::air;
        if(fault==2)input.press.token.code=0;
        if(fault==3)input.press.token.source=InputSource::buttons;
        if(fault==4)input.press.token.edge=InputEdge::release;
        if(fault==5)input.deadline=9;
        if(fault==6)input.deadline=511;
        if(fault==7)++native.generations.action;
        if(fault==8)native.flip.stance=0x20;
        if(fault==9)input.native.valid=false;
        check(!lease.start(input,native,10),"unrelated, expired, relabeled or unowned input cannot request Switch");
    }
}
}
int main(){original_edge_and_delivery();interruptions();denied_sources();std::printf("stance input: %u checks passed\n",checks);}
