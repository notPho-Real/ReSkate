#include "Extension/ArcadeMode/arcade_manual_release.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}
}
}
int main() {
    for (auto pose : {SkatePose::manual,SkatePose::nose_manual}) {
        ManualRelease release;
        check(!release.step(1000,1,true,pose,false).pending,"ordinary manual contact does not request release");
        const auto started=release.step(1017,1,true,pose,true);
        check(started.pending&&started.started,"safe endpoint requests a neutral native exit with one cleanup edge");
        const auto continuing=release.step(1034,1,true,pose,true);
        check(continuing.pending&&!continuing.started,"continued native contact neither completes nor restarts cleanup");
        check(release.step(1051,1,true,SkatePose::unknown,true).pending,"unknown physical observation cannot complete putdown");
        check(release.step(1068,1,true,pose,false).pending,"a lost model flag cannot silently restore actuation before physical exit");
        check(!release.step(1069,1,true,pose,true).started,"flickering model endpoint cannot clear fresh input twice");
        const auto exit=release.step(1085,1,true,SkatePose::riding,false);
        check(exit.completed&&!exit.pending,"independent two-wheel riding observation completes neutral release");
        check(!release.step(1102,1,true,SkatePose::riding,false).completed,"physical completion is an edge rather than a persistent presentation label");
        for(auto next : {SkatePose::air,SkatePose::bail,SkatePose::grind}) {
            release.reset();release.step(1200,1,true,pose,true);
            const auto ended=release.step(1217,1,true,next,true);
            check(ended.completed&&!ended.pending,"observed departure yields to jump, bail or rail ownership");
            check(!release.step(1234,1,true,next,true).completed,"lingering model endpoint cannot repeatedly finish one release");
        }
        for(unsigned fault=0;fault<4;++fault) {
            release.reset();release.step(2000,1,true,pose,true);
            const auto stopped=release.step(fault==0?1900:fault==1?2200:2017,fault==2?2:1,fault!=3,pose,true);
            check(!stopped.pending&&!stopped.completed,"clock, stale cadence, owner and validity loss relinquish without fabricating native completion");
        }
        InputSession input;
        InputAdmission admission{}; admission.gameplay=admission.authored=admission.manual_safe=true;
        admission.pose=SkatePose::riding;
        input.poll(3000,{0,0,0},0,0,admission);
        input.poll(3017,{0,0,25000},0,0,admission);
        check(input.poll(3034,{0,0,-25000},0,0,admission).manual.owns,"production input initially owns an eligible manual");
        check(input.history().events().size()>0,"manual input episode contains real journal events before putdown");
        release.reset(); const auto putdown=release.step(3051,1,true,pose,true);
        check(putdown.pending&&putdown.started,"physical endpoint blocks the input facade and requests full cleanup");
        if (putdown.started) input.reset();
        check(input.history().events().size()==0&&!input.busy()&&input.pending_manual().action==InputAction::none,
              "putdown clears shared history, pending manual and native playback owners together");
        admission.manual_safe=false;
        check(!input.poll(3051,{0,0,25000},0,0,admission).manual.owns &&
              !input.poll(3068,{0,0,-25000},0,0,admission).manual.owns,
              "closed production eligibility prevents immediate direction-pair reacquisition between native updates");
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
        admission.catalog=prepared; admission.may_start=true;
        const auto pending=release.step(3085,1,true,pose,true);
        if (pending.started) input.reset();
        check(pending.pending&&!pending.started&&input.poll(3085,{0x1000},0,0,admission).authored_playback,
              "continued physical putdown permits a fresh authored jump escape after one-time cleanup");
        const auto next=release.step(3102,1,true,pose,true);
        if (next.started) input.reset();
        check(input.poll(3102,{0x1000},0,0,admission).authored_playback,
              "repeated endpoint polls cannot erase the newly admitted escape setup");
    }
    for (int direction : {-1,1}) for (std::uint64_t age : {699u,700u,701u}) {
        ManualActuation actuation;
        const bool admitted=actuation.request(1000+age,direction,true,true,true,1000);
        check(admitted==(age<=700), "airborne native admission preserves the inclusive recognized-slot lifetime");
        ManualActuation waiting; waiting.request(1000,direction,true,true,true,1000);
        check(waiting.sample(1000+age,true,true,SkatePose::air,false,0).owns_stick==(age<=700),
              "held airborne preparation and initial admission use the same exact deadline");
    }
    for (int direction : {-1,1}) {
        for (const auto landing : {SkatePose::riding, SkatePose::manual, SkatePose::nose_manual}) {
            ManualActuation waiting;
            check(waiting.request(1000,direction,true,true,true,1000), "air request starts with its original recognition timestamp");
            check(waiting.sample(1100,true,true,SkatePose::unknown,false,0).owns_stick,
                  "brief unknown observation retains only the original airborne lease");
            check(waiting.requested_at()==1000, "unknown contact cannot start a landing settling clock");
            check(waiting.sample(1700,true,true,SkatePose::unknown,false,0).owns_stick,
                  "unknown evidence does not erase inclusive700ms preparation");
            check(!waiting.sample(1701,true,true,landing,false,0).owns_stick,
                  "late real contact cannot revive preparation after an unknown observation");
        }
        for (const auto landing : {SkatePose::riding, direction < 0 ? SkatePose::manual : SkatePose::nose_manual}) {
            ManualActuation admitted;
            admitted.request(1000,direction,true,true,true,1000);
            admitted.sample(1699,true,true,SkatePose::unknown,false,0);
            check(admitted.sample(1700,true,true,landing,false,0).owns_stick && admitted.requested_at()==1700,
                  "independent contact at the inclusive boundary begins settling exactly once");
            check(admitted.sample(1800,true,true,SkatePose::air,false,0).owns_stick,
                  "a bounded landing rebound retains the original settling lease");
            check(admitted.requested_at()==1700, "air rebound cannot renew settling");
            check(!admitted.sample(2900,true,true,SkatePose::riding,false,0).owns_stick,
                  "unadmitted settling expires at1200ms without renewal");
        }
    }
    std::cout<<checks<<" manual release checks passed\n";
}
