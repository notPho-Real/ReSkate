#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
constexpr std::uint16_t classic_y=0x8000;
void check(bool value, const char* why) {
    ++checks; if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
struct Pipeline {
    NativeInputState physics;
    NativeInputOwner owner{1,2,3,4,5};
    ArcadeInput input{true};
    NativeInputFrame frame{};
    RailAdapterPad rail_pad{};
    AuthoredTickClock clock;
    std::uint64_t tick{};
    // Distinct ground/rail vectors expose any accidental ground fallback.
    std::vector<PreparedGesture> catalog{
        {1,false,GestureContext::ground,{{0,0,-31000},{18,0,31000},{21,0,0},{24,0,0}}},
        {1,false,GestureContext::from_grind,{{0,0,-28000},{18,0,28000},{21,0,0},{24,0,0}}}};
    Pipeline() { input.enable(true); }
    void step(std::uint64_t now, unsigned state, unsigned hz) {
        NativeRequestSample sample{}; sample.valid=true; sample.state=state; sample.crouch_mode=1;
        frame=physics.observe(now,owner,sample,{true,-1});
        input.publish_native_context(frame);
        input.publish_authority(now,frame.owner_generation,1,true);
        tick=clock.advance(hz);
    }
    NativeGesture::Output poll(std::uint64_t now, std::uint16_t buttons, bool gameplay=true, bool may_start=true) {
        auto out=input.poll(0,now,buttons,0,0,0,0,gameplay,may_start,false,0,false,false,true,tick,false,catalog,
            false,false,SkatePose::grind);
        rail_pad=input.sample_native_rail(out.rail,now,gameplay && !out.action.owned);
        return out;
    }
};
}
int main() {
    for (unsigned hz : {30u,60u,120u}) for (unsigned state : {401u,403u}) {
        Pipeline p; p.step(1000,state,hz); p.poll(1000,0);
        const auto press=1000+1000/hz;
        p.step(press,state,hz); auto out=p.poll(press,classic_a);
        check(out.jump.setup_started && out.authored_playback && out.right_y==-28000,
            "production facade admits A using the exact from-grind recipe");
        check(out.jump.from_rail && out.jump.launch.context==LaunchContext::rail && !out.jump.launch.ordinary_writer(),
            "rail setup never grants the ordinary launch writer");
        check(!p.poll(press,classic_a).jump.setup_started,"repeated game-facing poll cannot restart setup");
        for (unsigned i=2;i<=25;++i) {
            auto now=1000+i*1000/hz; p.step(now,state,hz); out=p.poll(now,classic_a);
            check(out.right_y==-28000 && !out.jump.released,"held A retains rail setup without popping");
        }
        const auto release=1000+26*1000/hz;
        p.step(release,state,hz); out=p.poll(release,0);
        check(out.jump.released && out.jump.setup_admitted && out.jump.from_rail &&
            out.jump.pressed_at==press && out.jump.released_at==release,
            "release preserves original physical times and rail ownership");
        NativeCharge charge;
        check(!charge.semantic_release(release,true,true,true,press,release,true,out.jump.launch).sequence,
            "rail release cannot issue a flat-ground velocity ticket");
        bool flick=out.right_y>0;
        for(unsigned i=27;i<40;++i) {
            auto now=1000+i*1000/hz; p.step(now,state,hz); out=p.poll(now,0);
            flick=flick || out.right_y>0;
            check(!out.jump.released,"one release cannot execute again");
        }
        check(flick,"release delivers the authored native rail flick");
    }
    for (unsigned hz : {30u,60u,120u}) {
        for (bool recipe_available : {false,true}) {
            Pipeline p; p.step(1000,200,hz); p.poll(1000,0);
            const auto press=1000+1000/hz;
            p.step(press,200,hz); p.poll(press,classic_y);
            check(p.rail_pad.owns_right_stick,"held Y acquires the production rail adapter before contact");
            const auto contact=1000+2*1000/hz;
            p.step(contact,401,hz); p.poll(contact,classic_y);
            check(p.rail_pad.owns_right_stick,"ordinary first-grind handoff retains its channel");
            if(!recipe_available)p.catalog.pop_back();
            const auto setup=1000+3*1000/hz;
            p.step(setup,401,hz); auto out=p.poll(setup,classic_y|classic_a);
            check(out.jump.setup_started==recipe_available,"only an available native rail recipe can replace the held rail command");
            check(recipe_available ? out.rail.request.action==InputAction::none : out.rail.held,
                "admitted rail pop retires the old semantic Y owner; unsupported setup preserves it");
            check(p.rail_pad.owns_right_stick!=recipe_available,"native rail pressure cannot overwrite the admitted A setup");
            if(!recipe_available)continue;
            const auto release=1000+4*1000/hz;
            p.step(release,403,hz); out=p.poll(release,classic_y);
            check(out.jump.released && out.jump.setup_admitted,"A release pops while the initiating Y remains held");
            for(unsigned i=5;i<40;++i) {
                const auto now=1000+i*1000/hz;
                p.step(now,i==5?701:200,hz); out=p.poll(now,classic_y);
                check(!p.rail_pad.owns_right_stick && out.rail.request.action==InputAction::none,
                    "the old held Y cannot resume a rail channel through exit and AIR");
            }
            check(!out.command_busy,"completed rail pop leaves no stale semantic owner blocking later actions");
            const auto neutral=1000+40*1000/hz;
            p.step(neutral,200,hz); p.poll(neutral,0);
            const auto reenter=1000+41*1000/hz;
            p.step(reenter,200,hz); p.poll(reenter,classic_y);
            check(p.rail_pad.owns_right_stick,"a fresh physical Y edge can prepare the next grind");
        }
        for (unsigned phase : {402u,404u}) {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz); p.poll(1034,classic_a);
            p.step(1068,phase,hz);
            check(p.poll(1068,classic_a).right_y==-28000,"brief measured rail phase retains an admitted hold");
            p.step(1102,401,hz);
            check(p.poll(1102,0).jump.setup_admitted,"brief rail identity phase does not discard the release");
        }
        for (unsigned state : {400u,402u,404u,499u,701u,300u,504u}) {
            Pipeline p; p.step(1000,state,hz); p.poll(1000,0);
            p.step(1034,state,hz); const auto out=p.poll(1034,classic_a);
            check(!out.jump.setup_started && !out.authored_playback,
                "unqualified rail, transition, bail and offboard cannot start a rail pop");
        }
        for (unsigned fault=0;fault<8;++fault) {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz);
            if(fault==0)p.catalog.pop_back();
            if(fault==1)p.input.enable(false);
            if(fault==2)p.input.invalidate_authority();
            if(fault==3)p.input.publish_native_context({});
            const auto out=p.poll(fault==4?1200:1034,classic_a | (fault==5?0x10:0),fault!=6,fault!=7);
            check(!out.jump.setup_started && !out.authored_playback,
                "missing rail recipe, OFF, authority, evidence, stale, menu or admission cannot borrow ground setup");
        }
        for (unsigned boundary : {100u,101u,200u,701u,300u,504u}) {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz); p.poll(1034,classic_a);
            p.step(1068,boundary,hz); auto out=p.poll(1068,classic_a);
            check(!out.authored_playback,"leaving contact before release withdraws held rail setup");
            p.step(1102,boundary,hz); out=p.poll(1102,0);
            check(!out.jump.setup_admitted && !out.authored_playback,"withdrawn setup cannot pop on later release");
        }
        {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz); p.poll(1034,classic_a);
            ++p.owner.entity; p.step(1068,401,hz); p.poll(1068,classic_a);
            p.step(1102,401,hz); auto out=p.poll(1102,0);
            check(!out.jump.setup_admitted && !out.authored_playback,"new owner cannot inherit a held rail pop");
        }
        {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz); p.poll(1034,classic_a);
            p.step(1068,401,hz); p.poll(1068,classic_a);
            p.step(1102,403,hz); const auto out=p.poll(1102,0);
            check(out.jump.setup_admitted,"measured 401 to 403 contact preserves held setup");
            p.step(1136,701,hz); p.poll(1136,0);
            p.step(1170,200,hz); p.poll(1170,0);
            p.step(1204,100,hz);
            check(!p.poll(1204,0).authored_playback,"landing retires released rail playback");
        }
    }
    std::cout << checks << " rail-pop routing checks passed (no native gameplay acceptance)\n";
}
