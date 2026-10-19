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
        check(release.step(1017,1,true,pose,true).pending,"safe endpoint requests a neutral native exit");
        check(release.step(1034,1,true,pose,true).pending,"continued native contact cannot be counted as a completed putdown");
        check(release.step(1051,1,true,SkatePose::unknown,true).pending,"unknown physical observation cannot complete putdown");
        check(release.step(1068,1,true,pose,false).pending,"a lost model flag cannot silently restore actuation before physical exit");
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
        release.reset(); check(release.step(3051,1,true,pose,true).pending,"physical endpoint blocks the input facade");
        input.cancel_manual(); admission.manual_safe=false;
        check(!input.poll(3051,{0,0,25000},0,0,admission).manual.owns &&
              !input.poll(3068,{0,0,-25000},0,0,admission).manual.owns,
              "closed production eligibility prevents immediate direction-pair reacquisition between native updates");
    }
    for (int direction : {-1,1}) for (std::uint64_t age : {699u,700u,701u}) {
        ManualActuation actuation;
        const bool admitted=actuation.request(1000+age,direction,true,true,true,1000);
        check(admitted==(age<=700), "airborne native admission preserves the inclusive recognized-slot lifetime");
        ManualActuation waiting; waiting.request(1000,direction,true,true,true,1000);
        check(waiting.sample(1000+age,true,true,SkatePose::air,false,0).owns_stick==(age<=700),
              "held airborne preparation and initial admission use the same exact deadline");
    }
    std::cout<<checks<<" manual release checks passed\n";
}
