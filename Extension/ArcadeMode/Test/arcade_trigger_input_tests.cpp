#include "Extension/ArcadeMode/arcade_trigger_input.h"
#include <cstdio>
#include <cstdlib>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"trigger input: %s\n",message);std::exit(1);}
}
struct Fixture {
    InputEvents events;
    TriggerJournal triggers;
    InputNativeContext native{};
    PhysicalInputSample pad{};
    void sample(std::uint64_t now,InputContext context=InputContext::ground) {
        native.sampled_at=now;
        events.sample(now,pad,false,false,InputPolicy::production());
        triggers.observe(events.history(),context,native);
    }
};
}
int main() {
    for(const unsigned step:{8u,17u,34u}) {
        Fixture f;f.native.generations.owner=11;f.sample(0);
        f.pad.lt=255;f.sample(step);
        const auto press=f.triggers.get(0);
        check(press && press.press.at==step && press.context==InputContext::ground,"LT records original ground press");
        check(f.triggers.claim(0,{TriggerOwner::setup,0},f.events.history(),step),"setup exclusively claims LT");
        check(!f.triggers.claim(0,{TriggerOwner::transfer,step+150},f.events.history(),step),"one edge cannot select setup and transfer");
        f.native.airborne=true;f.native.airborne_episode=2;f.sample(step*2,InputContext::air);
        check(f.triggers.get(0).press.sequence==press.press.sequence && !f.triggers.get(0).native.airborne,"takeoff cannot rewrite origin");
        check(f.triggers.owned_held()==1,"held setup ownership persists across takeoff");
        f.pad.lt=0;f.sample(step*3,InputContext::air);
        check(f.triggers.get(0).released && f.triggers.get(0).release.at==step*3 && !f.triggers.owned_held(),"physical release withdraws held ownership");
        check(f.events.history().events().back().owners==InputConsumption::trigger,"release shares exclusive cycle consumption");
        f.pad.rt=255;f.sample(step*4,InputContext::air);
        const auto air=f.triggers.get(1);
        check(!f.triggers.claim(1,{TriggerOwner::switch_stance,step*4+150},f.events.history(),step*4),"unclaimed air edge cannot select ground Switch");
        check(f.triggers.claim(1,{TriggerOwner::transfer,step*4+150},f.events.history(),step*4),"air transfer claims physical RT");
        f.native.airborne=false;f.sample(step*5);
        check(f.triggers.get(1).context==InputContext::air && f.triggers.get(1).press.sequence==air.press.sequence,"touchdown preserves airborne origin");
        check(!f.triggers.claim(1,{TriggerOwner::landing_revert,step*5+150},f.events.history(),step*5),"transfer cannot become landing revert");
        check(!f.triggers.claim(1,{TriggerOwner::switch_stance,step*5+150},f.events.history(),step*5),"transfer cannot become ground Switch");
        f.pad.rt=0;f.sample(step*6);
        f.pad.rt=255;f.sample(step*7);
        check(f.triggers.get(1).press.sequence>air.press.sequence && f.triggers.get(1).context==InputContext::ground,"release/repress creates new ground command");
        check(!f.triggers.claim(1,{TriggerOwner::switch_stance,step*7+501},f.events.history(),step*7),"reject unbounded reservation");
        check(f.triggers.claim(1,{TriggerOwner::switch_stance,step*7+150},f.events.history(),step*7),"fresh ground Switch claim");
        f.events.reset();f.triggers.reset();f.native.generations.owner=12;f.sample(step*8);
        check(!f.triggers.get(1),"owner/OFF reset does not replay held RT");
        f.pad.rt=0;f.sample(step*9);f.pad.rt=255;f.sample(step*10);
        check(f.triggers.get(1).native.generations.owner==12,"new owner requires fresh physical press");
    }
    for(const auto duration:{0u,1u,199u,200u,201u,2000u}) {
        Fixture f;f.sample(0);f.pad.lt=255;f.sample(10);
        check(f.triggers.get(0).held_for(10+duration,200)==(duration>=200),"hold qualification uses physical duration");
        f.pad.lt=0;f.sample(10+duration);
        check(f.triggers.get(0).held_for(9999,200)==(duration>=200),"release freezes hold duration");
    }
    Fixture f;f.sample(0);f.pad.rt=255;f.sample(10);
    check(f.events.history().consume(InputMatch::single(f.triggers.get(1).press),InputConsumption::regular),"another existing action owns event");
    check(!f.triggers.claim(1,{TriggerOwner::switch_stance,160},f.events.history(),10),"journal rejects competing consumption");
    check(!f.triggers.claim(1,{TriggerOwner::switch_stance,600},f.events.history(),511),"expired opportunity cannot revive");
    std::printf("trigger input: %u checks passed\n",checks);
}
