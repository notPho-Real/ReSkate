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
        check(near(10.3f+resistance.step(dt,10.3f,crouched),10.3f-((crouched ? .000002f : .00001f)/.0254f*60*dt*10.3f*10.3f+.00001f*.0254f*60*dt)),
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
    for(auto stats : {GameplayStats::standard,GameplayStats::maxed}) for (bool crouched : {false,true}) for(float dt : {1.0f/30,1.0f/60}) {
        const GameplayProfile selected{stats};
        GroundDrive drive(selected); GroundResistance resistance(selected);
        std::array<float,3> position{}, velocity{0,0,1};
        float oracle=1;
        for (unsigned i=0;i<300;++i) {
            position[2]+=velocity[2]*dt;
            velocity[2]+=drive.step(dt,position,velocity,crouched,true);
            const float change=resistance.step(dt,velocity[2],crouched);
            velocity[2]+=change; drive.account_resistance(change);
            if(i) {
                if(oracle<=selected.kick_speed(crouched)) oracle+=selected.kick_acceleration(crouched)*dt;
                oracle=std::max(0.0f,oracle-(crouched?.000002f:.00001f)/.0254f*60*dt*oracle*oracle-.00001f*.0254f*60*dt);
            }
            check(near(velocity[2],oracle),"composed production flat-ground path matches the independent discrete kick-then-friction oracle at both profiles and cadences");
        }
        check(velocity[2]>selected.kick_speed(crouched)-.5f&&velocity[2]<selected.kick_speed(crouched)+selected.kick_acceleration(crouched)*dt,
              "steady response crosses the kick eligibility threshold without turning it into an absolute cap");
        for(int offset : {-1,0,1}) {
            const float limit=selected.kick_speed(crouched);
            const float initial=offset<0?std::nextafter(limit,0.0f):offset>0?std::nextafter(limit,100.0f):limit;
            GroundDrive boundary(selected);boundary.step(dt,{}, {0,0,initial},crouched);
            const float increment=boundary.step(dt,{0,0,initial*dt},{0,0,initial},crouched);
            check(offset>0?increment==0:near(increment,selected.kick_acceleration(crouched)*dt),
                  "native kick gate permits a complete step at the exact threshold and rejects just above it");
        }
    }
    for(float dt : {1.0f/30,1.0f/60}) for(bool crouched : {false,true}) {
        GroundResistance brake;
        const float impulse=22.86f*dt;
        check(brake.selected_speed(std::nextafter(2*impulse,0.0f),dt,crouched,true)==0,
              "braking stops strictly below twice its step impulse without reversing velocity");
        check(brake.selected_speed(2*impulse,dt,crouched,true)>impulse-.001f,
              "the exact braking threshold applies one impulse instead of an early stop");
        brake.step(dt,10,crouched);
        const float expected=brake.selected_speed(10,dt,crouched,true);
        check(near(10+brake.step(dt,10,crouched,true),expected),"selected native brake request reaches the resistance budget");
        brake.reset();brake.step(dt,10,crouched);
        check(brake.step(dt,expected-.1f,crouched,true)==0,"stronger native braking or collision loss is never restored");
        brake.reset();brake.step(dt,40,crouched,true);
        check(near(40+brake.step(dt,40,crouched,true),40-impulse),
              "braking an unknown fast arrival retains transfer caps and applies only independently requested brake deceleration");
    }
    for(unsigned state : {100u,103u,200u,403u}) for(bool setup : {false,true}) {
        const auto context=ground_motion_context(state,0,0xc000,false,false,setup);
        check(context.eligible==(state==100||state==103)&&context.crouched==(state==103&&setup),
              "actual ground class and delivered held setup jointly select crouched acceleration");
        check(context.may_start==(state==103&&setup),"an independently grounded crouch may start from rest");
        check(!ground_motion_context(state,0,0xc000,false,true,setup).eligible,
              "native braking always vetoes acceleration regardless of setup state");
    }
    for(unsigned contacts:{0x4000u,0x8000u,0xc000u}) {
        NativeManualContact proof{{1,2,3,4,5},1000,contacts,true,1,2,3};
        check(proof.generation_matches(1,2,3),"manual movement binds to the independently admitted native episode");
        check(!proof.generation_matches(2,2,3)&&!proof.generation_matches(1,3,3)&&!proof.generation_matches(1,2,4),
              "owner/contact/action changes independently retire the old movement proof");
        check(proof.matches(1000,{1,2,3,4,5},100,1,contacts),"independent Manual/Nose proof admits its current physical wheel contact");
        check(proof.matches(1150,{1,2,3,4,5},100,1,contacts)&&!proof.matches(1151,{1,2,3,4,5},100,1,contacts),
              "manual movement proof preserves its original inclusive150ms observation lease");
        check(!proof.matches(999,{1,2,3,4,5},100,1,contacts),"future observation cannot authorize movement");
        for(unsigned field=0;field<5;++field){auto changed=proof.owner;++changed[field];check(!proof.matches(1017,changed,100,1,contacts),"all five native owner links must match before manual resistance");}
        for(unsigned state:{103u,200u,401u,701u})check(!proof.matches(1017,proof.owner,state,1,contacts),"crouch/air/rail/transition cannot inherit a basic manual resistance proof");
        for(unsigned mode:{0u,2u,3u})check(!proof.matches(1017,proof.owner,100,mode,contacts),"native mode must still admit physical ground balance");
        check(!proof.matches(1017,proof.owner,100,1,0),"lost native wheel contact yields without changing momentum");
        proof.admitted=false;check(!proof.matches(1017,proof.owner,100,1,contacts),"contact without independent balance identity grants no movement correction");
        for(float dt:{1.0f/30,1.0f/60})for(bool native_brake:{false,true}){
            GroundResistance resistance;resistance.step(dt,10,true,native_brake,true);
            const float target=10-.00001f/.0254f*60*dt*100-.00001f*.0254f*60*dt;
            check(near(10+resistance.step(dt,10,true,native_brake,true),target),"admitted manual uses ordinary coast resistance rather than crouch or brake budget");
            resistance.reset();resistance.step(dt,10,false,false,true);
            check(resistance.step(dt,9,false,true,true)==0,"stronger native collision/brake loss remains authoritative during manual movement");
        }
    }
    std::cout<<checks<<" ground resistance checks passed\n";
}
