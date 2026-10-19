#include "Extension/ArcadeMode/arcade_movement.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
bool near(float a, float b) { return std::abs(a-b) < .00002f; }
}
int main() {
    const GameplayProfile profile;
    for (bool crouched : {false,true}) for (float dt : {1.0f/30,1.0f/60}) {
        GroundResistance resistance;
        const float loss = (crouched ? .000002f : .00001f) / .0254f * 60 * dt * 100 + .00001f * .0254f * 60 * dt;
        check(near(resistance.selected_speed(10,dt,crouched),10-loss), "selected coast uses squared SI speed with converted coefficient and constant rolling loss");
        for (float fraction : {0.0f,.5f,1.0f,2.0f}) {
            resistance.reset(); resistance.step(dt,10,crouched);
            const float native = 10-loss*fraction;
            const float correction = resistance.step(dt,native,crouched);
            check(correction <= 0 && near(native+correction,std::min(native,10-loss)),
                  "native resistance is credited once; stronger native drag is never restored");
        }
        resistance.reset(); resistance.step(dt,10,crouched);
        check(near(10.3f+resistance.step(dt,10.3f,crouched),10.3f-loss),
              "native acceleration does not erase the selected resistance budget");
        resistance.reset(); resistance.step(dt,20,crouched);
        check(20+resistance.step(dt,20,crouched)>profile.kick_speed(crouched),
              "earned speed above kick threshold remains available instead of being clamped to cruise");
        resistance.reset(); resistance.step(dt,20,crouched);
        check(30+resistance.step(dt,30,crouched)<=profile.hard_speed(), "ordinary ground hard cap is enforced after established ownership");
        const float limited = profile.hard_speed();
        const float heavy = limited-.0001f/.0254f*60*dt*limited*limited;
        const float wind = heavy-(crouched?.000002f:.00001f)/.0254f*60*dt*heavy*heavy;
        check(near(resistance.selected_speed(40,dt,crouched),std::max(0.0f,wind-.00001f*.0254f*60*dt)),
              "hard clamp precedes heavy wind and ordinary ground resistance");
        resistance.reset();
        check(resistance.step(dt,40,crouched)==0 && resistance.step(dt,35,crouched)==0,
              "unclassified fast arrival retains native transfer-limit ownership");
        resistance.step(dt,20,crouched);
        check(resistance.step(dt,30,crouched)<0, "return below soft threshold establishes ordinary ground limits");
        resistance.reset(); resistance.step(dt,10,crouched);
        check(resistance.step(dt,0,crouched)==0, "collision or native stop never receives a reverse or compensating impulse");
    }
    for (float invalid : {0.0f,.2f,-.1f}) {
        GroundResistance resistance; resistance.step(1.0f/60,10,false);
        check(resistance.step(invalid,10,false)==0 && resistance.step(1.0f/60,40,false)==0,
              "invalid cadence clears the prior resistance episode");
    }
    for (bool crouched : {false,true}) {
        GroundDrive drive; GroundResistance resistance;
        std::array<float,3> position{}, velocity{0,0,1};
        const float dt=1.0f/60;
        for (unsigned i=0;i<300;++i) {
            position[2]+=velocity[2]*dt;
            velocity[2]+=drive.step(dt,position,velocity,crouched,true);
            const float change=resistance.step(dt,velocity[2],crouched);
            velocity[2]+=change; drive.account_resistance(change);
            check(velocity[2]>=0 && velocity[2]<=profile.kick_speed(crouched)+.0001f,
                  "composed production drive and resistance remain bounded without undoing the preceding loss");
        }
        check(velocity[2]<profile.kick_speed(crouched)-.001f && velocity[2]>profile.kick_speed(crouched)-.5f,
              "steady kick response retains coast resistance rather than canceling it next frame");
    }
    std::cout<<checks<<" ground resistance checks passed\n";
}
