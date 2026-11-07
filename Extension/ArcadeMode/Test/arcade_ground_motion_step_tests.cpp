#include "Extension/ArcadeMode/arcade_ground_motion_step.h"
#include "Extension/ArcadeMode/arcade_movement.h"
#include "Extension/ArcadeMode/arcade_push.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) { ++checks; if(!value){std::cerr<<why<<'\n';std::exit(1);} }
bool near(float a,float b) { return std::abs(a-b)<.0001f; }
struct Observations {
    NativeInputState observer;
    NativeInputOwner owner{1,2,3,4,5};
    std::uint64_t now{1000};
    float seconds;
    explicit Observations(float dt=1.f/60):seconds(dt) {}
    NativeInputFrame frame{};
    NativeInputFrame next(std::uint64_t at,unsigned state=100) {
        now=at;NativeRequestSample sample{};sample.valid=true;sample.state=state;
        sample.crouch_mode=1;sample.words[1]=0xc000;
        frame=observer.observe(now,owner,sample,{true,-1},false,seconds);return frame;
    }
};
// Controlled observed-velocity sequences, not a model of native Skate physics.
// Only original physics observations advance; extra consumers see cached dt.
float trace(unsigned hz,unsigned callbacks,bool crouched,bool native_loss) {
    const float dt=1.f/static_cast<float>(hz);
    GroundMotionStep drive_steps,resistance_steps;GroundDrive drive;GroundResistance resistance;
    AutomaticPush push;Observations observations(dt);float position{},speed{1};
    push.poll(1000,0,0,speed,true,true);
    for(unsigned step=0;step<hz;++step) {
        const auto now=1000+static_cast<std::uint64_t>(std::llround(1000.0*step/hz));
        const auto frame=observations.next(now,crouched?103u:100u);
        const auto controls=GroundControl::physical(true,0,0,32767);
        push.poll(now,0,32767,speed,true,true);
        check(push.movement_intent()&&controls.can_kick(1,crouched),"physical forward admits the existing kick policy");
        position+=speed*dt;
        if(native_loss&&step)speed=std::max(0.0f,speed-.1f*dt);
        for(unsigned callback=0;callback<callbacks;++callback) {
            const auto drive_step=drive_steps.consume(frame,now,true);
            if(drive_step) {
                if(drive_step.baseline)drive.reset();
                if(controls.can_kick(speed,crouched))speed+=drive.step(drive_step.seconds,{0,0,position},{0,0,speed},crouched,true);
                else drive.reset();
            }
            const auto resistance_step=resistance_steps.consume(frame,now,true);
            if(resistance_step) {
                if(resistance_step.baseline)resistance.reset();
                const float change=resistance.step(resistance_step.seconds,speed,crouched);
                speed+=change;drive.account_resistance(change);
            }
            check(std::isfinite(speed)&&speed>=0,"coupled budgets preserve finite forward momentum");
        }
    }
    return speed;
}
}
int main() {
    for(unsigned hz:{30u,60u}) {
        const float dt=1.f/static_cast<float>(hz);
        Observations o(dt);GroundMotionStep steps;auto frame=o.next(1000);
        auto first=steps.consume(frame,o.now,true);
        check(bool(first)&&first.baseline,"first original observation establishes an episode");
        check(first.seconds==dt,"admission returns the original callback interval at30/60Hz");
        for(unsigned poll=0;poll<8;++poll)
            check(steps.consume(frame,o.now+poll,true).status==GroundStepStatus::duplicate,"cached dt cannot spend another scalar budget");
        check(!steps.consume(frame,o.now+8,false),"withdrawn permission cannot write");
        check(steps.consume(frame,o.now+9,true).status==GroundStepStatus::duplicate,"permission recovery cannot erase the spent step");
        auto altered=frame;++altered.owner_generation;
        check(!steps.consume(altered,o.now+10,true),"a new command generation cannot reuse an original physics interval");
        frame=o.next(1000+1000/hz);
        auto next=steps.consume(frame,o.now,true);
        check(bool(next)&&!next.baseline,"one new contiguous original step advances one budget");
        const auto older=o.frame;frame=o.next(o.now+1000/hz);frame=o.next(o.now+1000/hz);
        const auto gap=steps.consume(frame,o.now,true);
        check(bool(gap)&&gap.baseline,"missing physical steps rebaseline instead of catching up");
        check(!steps.consume(older,o.now,true),"older observations cannot reset the watermark");
        check(steps.consume(frame,o.now,true).status==GroundStepStatus::duplicate,"rejected replay cannot make the latest step available again");
        ++o.owner.context;frame=o.next(o.now+1000/hz);
        const auto replacement=steps.consume(frame,o.now,true);
        check(bool(replacement)&&replacement.baseline,"fresh full-owner replacement receives a new baseline");
        for(bool crouched:{false,true})for(bool native_loss:{false,true}) {
            const float once=trace(hz,1,crouched,native_loss);
            check(near(once,trace(hz,2,crouched,native_loss))&&near(once,trace(hz,4,crouched,native_loss)),
                  "30/60Hz physical traces are invariant to repeated native consumers");
        }
        for(bool crouched:{false,true}) {
            GroundDrive drive;const float previous=5,loss=.2f;
            drive.step(dt,{0,0,0},{0,0,previous},crouched,true);
            const float change=drive.step(dt,{0,0,previous*dt},{0,0,previous-loss},crouched,true);
            check(near(change,GameplayProfile{}.kick_acceleration(crouched)*dt),
                  "unclassified native slowing is not refunded as steering drag");
            check(previous-loss+change<previous+GameplayProfile{}.kick_acceleration(crouched)*dt,
                  "native speed loss remains present after the selected assistance");
            drive.reset();drive.step(dt,{0,0,0},{0,0,previous},crouched,true);
            const float native_gain=GameplayProfile{}.kick_acceleration(crouched)*dt*2;
            check(drive.step(dt,{0,0,previous*dt},{0,0,previous+native_gain},crouched,true)==0,
                  "sufficient native acceleration receives no second acceleration budget");
            drive.reset();drive.step(dt,{0,0,0},{0,0,previous},crouched,true);
            check(drive.step(dt,{0,0,previous*dt},{0,0,0},crouched,true)==0,"native collision stop wins over held forward");
            drive.reset();drive.step(dt,{0,0,0},{0,0,0},crouched,true);
            check(drive.step(dt,{0,0,0},{0,0,0},crouched,true)==0&&drive.step(dt,{0,0,0},{0,0,0},crouched,true)==0,
                  "startup cannot keep injecting speed into an unchanged native stop");
        }
    }
    for(unsigned fault=0;fault<12;++fault) {
        Observations o;auto frame=o.next(1000);GroundMotionStep steps;auto now=o.now;
        switch(fault) {
        case 0:frame.valid=false;break;case 1:frame.owner.core=0;break;case 2:frame.owner_generation=0;break;
        case 3:frame.contact_generation=0;break;case 4:frame.action_generation=0;break;case 5:frame.physics_step=0;break;
        case 6:frame.sampled_at=now+1;break;case 7:now+=151;break;case 8:frame.physics_seconds=0;break;
        case 9:frame.physics_seconds=std::numeric_limits<float>::quiet_NaN();break;case 10:frame.physics_seconds=.041f;break;case 11:frame.state=401;frame.rail_family=true;break;
        }
        check(!steps.consume(frame,now,true),"missing owner/step, stale/future evidence, invalid dt and rail cannot own ground budgets");
        check(bool(steps.consume(o.frame,o.now,true)),"failed admission never consumes an otherwise unspent valid observation");
    }
    for(float dt:{.005f,.04f}) {
        Observations o(dt);GroundMotionStep steps;const auto frame=o.next(1000);
        check(bool(steps.consume(frame,1150,true)),"existing dt and original-observation leases retain inclusive boundaries");
        check(!steps.consume(frame,1151,true),"one millisecond past the original lease cannot spend a budget");
    }
    {
        Observations o;GroundMotionStep steps;auto frame=o.next(1000);
        check(bool(steps.consume(frame,o.now,true)),"baseline for current-step mutation controls");
        ++frame.contact_generation;
        check(steps.consume(frame,o.now,true).status==GroundStepStatus::unavailable,
              "same-step contact changes cannot masquerade as a new interval");
        frame=o.frame;++frame.action_generation;
        check(steps.consume(frame,o.now,true).status==GroundStepStatus::unavailable,
              "same-step action changes cannot masquerade as a new interval");
        frame=o.next(1151);
        const auto recovered=steps.consume(frame,o.now,true);
        check(bool(recovered)&&recovered.baseline,"original-observation gap rebaselines the response instead of accumulating compensation");
    }
    {
        Observations o;GroundMotionStep steps;const auto first=o.next(1000);
        check(bool(steps.consume(first,1000,true)),"original clock receipt baseline");
        auto mutated=first;mutated.physics_seconds=1.f/30;
        check(steps.consume(mutated,1000,true).status==GroundStepStatus::unavailable,
              "changing dt on a spent original frame cannot reopen its budget");
        check(steps.consume(first,1000,true).status==GroundStepStatus::duplicate,
              "rejected clock mutation preserves the original receipt");
        o.seconds=1.f/30;const auto next=o.next(1001);
        const auto interval=steps.consume(next,1001,true);
        check(bool(interval)&&interval.seconds==1.f/30,
              "simulation interval comes from the producer, not wall-clock timestamp differences");
        for(unsigned poll=0;poll<4;++poll) {
            const auto correlated=o.observer.correlate(1001+poll,o.owner,[] {
                NativeRequestSample sample{};sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=0xc000;return sample;
            }());
            check(correlated.physics_seconds==next.physics_seconds&&correlated.sampled_at==next.sampled_at,
                  "client correlation copies original seconds without renewing its interval or timestamp");
        }
    }
    for(float unavailable:{0.f,.0005f,.1f,.101f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        Observations o(unavailable);const auto frame=o.next(1000);GroundMotionStep steps;
        check(frame.valid&&!steps.consume(frame,1000,true),
              "valid input observation without a supported original motion clock cannot actuate ground physics");
    }
    {
        Observations o;const auto timed=o.next(1000);
        NativeRequestSample sample{};sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=0xc000;
        const auto untimed=o.observer.observe(1017,o.owner,sample,{true,-1});GroundMotionStep steps;
        check(timed.physics_seconds>0&&untimed.valid&&untimed.physics_seconds==0&&!steps.consume(untimed,1017,true),
              "an untimed observation cannot inherit the preceding original interval");
    }
    for(short x:{short(-32767),short(0),short(32767)})for(short y:{short(-32767),short(0),short(32767)}) {
        const auto c=GroundControl::physical(true,0,x,y);
        const bool down=y<0,left=x<0,right=x>0;
        check(c.down==down&&c.left==left&&c.right==right,"forward/turn/brake arbitration uses the actual physical normalized axes");
        check(c.can_kick(5,false)==!down,"physical Down suppresses kick while horizontal steering can coexist with forward");
        check(c.trying_to_brake(5,1)==(down&&!left&&!right),"fast forward-facing Down plus turn selects sharp turning instead of a brake");
        check(c.trying_to_brake(5,-1)==down,"reverse-facing Down plus turn still selects braking");
    }
    const auto no_auto=GroundControl::physical(false,0,0,32767),square=GroundControl::physical(false,0x4000,0,32767);
    check(!no_auto.can_kick(5,false)&&!square.can_kick(5,false),"physical X cannot authorize a kick when auto-push is OFF");
    for(unsigned hz:{30u,60u}) {
        GroundResistance coast;const float dt=1.f/static_cast<float>(hz);GroundFriction friction{false,false};
        coast.step(dt,5,false,false,false,friction);
        check(coast.step(dt,5,false,false,false,friction)==0,"auto-kick OFF default friction cannot invent coasting resistance");
    }
    std::array<std::array<float,3>,2> bodies{{{1,2,3},{2,-1,4}}};
    check(GroundDrive::apply_bodies(bodies,.2f,-.1f)&&near(bodies[0][1],2)&&near(bodies[1][1],-1)&&
          near(bodies[1][0]-bodies[0][0],1)&&near(bodies[1][2]-bodies[0][2],1),
          "bounded corrections preserve vertical and articulated relative velocities");
    std::cout<<checks<<" ground motion ownership and physical trace checks passed\n";
}
