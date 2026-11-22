#include "Extension/ArcadeMode/arcade_setup_selection.h"
#include <cstdio>
#include <cstdlib>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
struct Fixture {
    PhysicalSetupSelection selector{};
    InputEvents events{};
    TriggerJournal journal{};
    SetupSelectionContext frame{};
    std::uint64_t now{},step{},owner{1};
    Fixture(InputContext context=InputContext::ground) {
        frame.context=context;frame.enabled=true;poll(0,0);
    }
    void poll(std::uint64_t at,unsigned lt,TriggerOwner route=TriggerOwner::setup) {
        now=at;
        frame.native={true,true,{owner,1,1},++step,at,frame.context==InputContext::rail,
                      frame.context==InputContext::air,frame.context==InputContext::air?1u:0u,0};
        frame.native.flip.stance=0;
        events.sample(at,{0,0,0,static_cast<std::uint8_t>(lt),0},{},true,{});
        journal.observe(events.history(),frame.context,frame.native);
        if(journal.get(0).owner==TriggerOwner::none)
            journal.claim(0,{route,at+500},events.history(),at);
        selector.sample(at,journal.get(0),frame);
    }
};
void tap_and_hold() {
    for(auto context:{InputContext::ground,InputContext::balance,InputContext::rail})
    for(unsigned duration:{0u,1u,199u,200u,201u,1000u}) {
        Fixture f(context);f.poll(10,255);
        check(f.selector.desired()==SetupStance::normal,"LT press alone changes no launch or provisional pose");
        check(f.selector.launch()==SetupStance::unknown,"LT alone never freezes or creates a launch");
        check(f.selector.desired(true)==SetupStance::nollie,"overlapping A immediately selects provisional Nollie");
        f.poll(10+duration,0);
        const auto expected=duration<200?SetupStance::nollie:SetupStance::normal;
        check(f.selector.desired()==expected,"physical release duration distinguishes tap and hold, including same time");
        for(unsigned repeat=0;repeat<3;++repeat)f.poll(20+duration+repeat,0);
        check(f.selector.desired()==expected,"duplicate release cannot toggle setup twice");
        f.poll(40+duration,255);f.poll(50+duration,0);
        check(f.selector.desired()==(expected==SetupStance::normal?SetupStance::nollie:SetupStance::normal),
              "fresh tap toggles normal and Nollie without a three-state cycle");
    }
    for(unsigned cadence:{8u,16u,17u,33u,34u}) {
        Fixture f;f.poll(10,255);f.poll(20,0);f.poll(30,255);
        for(unsigned elapsed=0;elapsed<=500;elapsed+=cadence) {
            f.poll(30+elapsed,255);
            check(f.selector.desired(true)==(elapsed<200?SetupStance::normal:SetupStance::pressure),
                  "hold qualification uses physical time independently of polling cadence");
        }
        f.poll(600,0);
        check(f.selector.desired()==SetupStance::nollie,"Pressure release restores prior Nollie without synthetic tap");
    }
    for(unsigned duration:{199u,200u,201u}) {
        Fixture f;f.poll(10,255);f.poll(10+duration,255);
        check(f.selector.desired(true)==(duration<200?SetupStance::nollie:SetupStance::pressure),"held boundary is inclusive at 200ms");
    }
}
void frozen_flights() {
    {
        Fixture f;f.poll(10,0);check(f.selector.freeze_launch(10),"grounded pop has a frozen launch");
        f.poll(20,255);
        check(f.selector.desired(true)==SetupStance::normal&&f.selector.desired_setup(true)==SetupStance::nollie,
              "fresh prospective setup does not inherit an earlier grounded pop snapshot");
        f.selector.cancel_launch();
        check(f.selector.launch()==SetupStance::unknown&&f.selector.trigger_held()&&f.selector.remembered()==SetupStance::normal,
              "successful replacement can retire old launch without cancelling the fresh physical LT press");
        f.poll(30,0);
        check(f.selector.desired_setup()==SetupStance::nollie,"retiring old pop preserves short-release selection");
        check(f.selector.freeze_launch(30),"new setup can freeze its own admitted release");
        f.poll(40,255);f.poll(240,255);
        check(f.selector.desired()==SetupStance::nollie&&f.selector.desired_setup(true)==SetupStance::pressure,
              "prospective Pressure remains distinct from old frozen Nollie");
        f.selector.cancel_launch();f.poll(241,0);
        check(f.selector.remembered()==SetupStance::nollie,"cancelling a launch preserves base selection through held Pressure release");
    }
    {
        Fixture f;f.poll(10,255);f.poll(20,0);
        check(f.selector.remembered()==SetupStance::nollie,"first tap selects Nollie");
        f.poll(30,255);
        check(f.selector.desired(true)==SetupStance::normal,"second LT plus A provisionally selects the forthcoming normal toggle");
        check(f.selector.freeze_launch(30)&&f.selector.launch()==SetupStance::normal,"quick A release freezes prospective second-tap selection");
        f.frame.context=InputContext::air;f.poll(31,255);f.poll(40,0);f.poll(41,0);
        check(f.selector.launch()==SetupStance::normal&&f.selector.remembered()==SetupStance::normal,
              "short LT release in AIR toggles once without relabeling the normal launch");
    }
    for(unsigned release_at:{10u,11u,209u,210u,211u}) {
        Fixture f;f.poll(10,255);f.poll(release_at,255);
        const auto expected=release_at<210?SetupStance::nollie:SetupStance::pressure;
        check(f.selector.freeze_launch(release_at),"admitted physical A release freezes setup without waiting");
        check(!f.selector.freeze_launch(release_at),"one release cannot replace its launch snapshot");
        f.frame.context=InputContext::air;f.poll(release_at+1,255);f.poll(release_at+300,255);
        check(f.selector.desired(true)==expected&&f.selector.launch()==expected,"later hold qualification cannot relabel current flight");
        f.poll(release_at+301,0);
        check(f.selector.desired()==expected,"LT release in flight leaves the admitted launch immutable");
        f.poll(release_at+302,255);f.poll(release_at+303,0);
        check(f.selector.launch()==expected,"fresh airborne LT cannot select a different setup");
        f.frame.context=InputContext::ground;f.poll(release_at+304,0);
        check(f.selector.launch()==SetupStance::unknown&&f.selector.desired()==SetupStance::normal,
              "landing retires the flight and cannot inherit airborne trigger edges");
    }
    Fixture f;f.poll(10,255);f.poll(20,0);
    check(!f.selector.freeze_launch(19),"delayed callback cannot silently borrow later trigger selection");
    check(f.selector.freeze_launch(20)&&f.selector.launch()==SetupStance::nollie,"quick LT release before A release keeps selected Nollie");
    f.frame.context=InputContext::air;f.poll(21,0);
    f.frame.native.airborne_episode=2;f.selector.sample(21,f.journal.get(0),f.frame);
    check(f.selector.launch()==SetupStance::unknown,"different physical flight cannot inherit launch snapshot");
}
void ownership_and_context() {
    for(auto denied:{TriggerOwner::none,TriggerOwner::switch_stance,TriggerOwner::manual_pivot,
                     TriggerOwner::landing_revert,TriggerOwner::transfer,TriggerOwner::cancelled}) {
        Fixture f;f.poll(10,255,denied);f.poll(20,0,denied);
        check(f.selector.desired()==SetupStance::normal&&!f.selector.trigger_held(),"other trigger owner cannot select setup");
    }
    for(auto next:{InputContext::ground,InputContext::balance,InputContext::rail,InputContext::air,InputContext::foot}) {
        Fixture f;f.poll(10,255);f.frame.context=next;f.poll(210,255);
        if(next!=InputContext::ground)check(!f.selector.trigger_held(),"held pre-context LT cannot become a setup in another context");
        f.poll(211,0);
        check(f.selector.remembered()==SetupStance::normal,"context change or completed hold cannot manufacture a tap");
    }
    for(unsigned fault=0;fault<6;++fault) {
        Fixture f;f.poll(10,255);f.poll(20,0);check(f.selector.desired()==SetupStance::nollie,"negative has selected setup");
        f.poll(30,255);check(f.selector.freeze_launch(30),"negative has launch snapshot");
        if(fault==0)f.frame.enabled=false;
        if(fault==1)f.frame.native.valid=false;
        if(fault==2)f.frame.native.flip.stance=0x40;
        if(fault==3)++f.frame.native.generations.owner;
        const auto now=fault==4?181u:fault==5?29u:30u;
        f.selector.sample(now,f.journal.get(0),f.frame);
        check(f.selector.launch()==SetupStance::unknown&&f.selector.remembered()==SetupStance::normal,
              "OFF, owner loss, bail, stale native sample and regressing clock clear selection and launch");
    }
    Fixture f;f.poll(10,255);auto changed=f.journal.get(0);changed.owner=TriggerOwner::transfer;
    f.selector.sample(10,changed,f.frame);
    check(!f.selector.trigger_held(),"lost trigger ownership withdraws provisional setup immediately");
    changed.owner=TriggerOwner::setup;f.selector.sample(10,changed,f.frame);
    check(!f.selector.trigger_held(),"same cancelled sequence cannot reacquire setup");
    f.selector.reset();f.selector.sample(100,f.journal.get(0),f.frame);
    check(!f.selector.trigger_held(),"reset cannot inherit an already-held pre-context trigger");
}
void recorded_release_and_provenance() {
    for(unsigned duration:{0u,1u,199u,200u,201u}) {
        Fixture f;f.poll(10,255);auto released=f.journal.get(0);
        released.released=true;
        released.release={{InputSource::triggers,0,InputEdge::release},10+duration,released.press.sequence+1};
        f.frame.native.sampled_at=1000;
        f.selector.sample(1000,released,f.frame);
        check(f.selector.desired()==(duration<200?SetupStance::nollie:SetupStance::normal),
              "delayed consumption classifies original release time, not polling duration");
    }
    for(unsigned fault=0;fault<8;++fault) {
        Fixture f;auto frame=f.frame;frame.native.sampled_at=10;
        TriggerPress record{};record.owner=TriggerOwner::setup;record.context=InputContext::ground;
        record.native=frame.native;record.press={{InputSource::triggers,0,InputEdge::press},10,1};
        if(fault==0)record.press.token.code=1;
        if(fault==1)record.press.token.source=InputSource::buttons;
        if(fault==2)record.press.token.edge=InputEdge::release;
        if(fault==3)record.context=InputContext::air;
        if(fault==4)++record.native.generations.owner;
        if(fault==5)++record.native.generations.contact;
        if(fault==6)++record.native.generations.action;
        if(fault==7)record.press.at=11;
        f.selector.sample(10,record,frame);
        check(!f.selector.trigger_held()&&f.selector.desired(true)==SetupStance::normal,
              "unrelated token, context, owner generation or future time cannot select setup");
    }
    for(unsigned fault=0;fault<5;++fault) {
        Fixture f;f.poll(10,255);auto released=f.journal.get(0);released.released=true;
        released.release={{InputSource::triggers,0,InputEdge::release},20,released.press.sequence+1};
        if(fault==0)released.release.token.code=1;
        if(fault==1)released.release.token.edge=InputEdge::press;
        if(fault==2)released.release.sequence=released.press.sequence;
        if(fault==3)released.release.at=9;
        if(fault==4)released.release.at=21;
        f.frame.native.sampled_at=20;f.selector.sample(20,released,f.frame);
        check(!f.selector.trigger_held()&&f.selector.desired(true)==SetupStance::normal,
              "malformed release cannot toggle or sustain setup ownership");
    }
}
}
int main() {tap_and_hold();frozen_flights();ownership_and_context();recorded_release_and_provenance();std::printf("setup selection: %u checks passed\n",checks);}
