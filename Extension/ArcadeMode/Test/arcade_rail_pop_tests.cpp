#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "arcade_native_flip_entry_tests.h"
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
    NativeFlipRequest bridge{};
    AuthoredTickClock clock;
    std::uint64_t tick{};
    // Distinct ground/rail vectors expose any accidental ground fallback.
    std::vector<PreparedGesture> catalog{
        {1,false,GestureContext::ground,{{0,0,-31000},{18,0,31000},{21,0,0},{24,0,0}}},
        {1,false,GestureContext::from_grind,{{0,0,-28000},{18,0,28000},{21,0,0},{24,0,0}}}};
    Pipeline() { input.enable(true); }
    void step(std::uint64_t now, unsigned state, unsigned hz, int selected=-1, std::uintptr_t object=16, unsigned mode=1) {
        NativeRequestSample sample{}; sample.valid=true; sample.state=state; sample.crouch_mode=mode;
        frame=physics.observe(now,owner,sample,{true,selected,object});
        input.publish_native_context(frame);
        input.publish_authority(now,frame.owner_generation,1,true);
        tick=clock.advance(hz);
    }
    NativeGesture::Output poll(std::uint64_t now, std::uint16_t buttons, bool gameplay=true, bool may_start=true) {
        auto out=input.poll(0,now,buttons,0,0,0,0,gameplay,may_start,false,0,false,false,true,tick,false,catalog,
            false,false,SkatePose::grind,false,false,false,false,false,false,0,0,ActionExperiment::classic,true,true,false,bridge);
        rail_pad=input.sample_native_rail(out.rail,now,gameplay && !out.action.owned);
        return out;
    }
};
}
int main() {
    // The original Kickflip high clip's rail reselection interval is [7,12].
    // Its native Pop and Jump tags both span [12,14]; the unchanged bridge
    // must initialize after that interval while preserving the selected clip.
    for(unsigned selected:{4u,5u,6u}) {
        native_entry_tests::Memory memory;
        memory.put(0x10000,0x8400000du|(selected<<12));
        const auto entry=memory.entry();
        check(entry && entry->tick==16 && entry->key==0x8400800d,
            "native rail reselection samples use the existing post-pop actor initialization");
    }
    for (unsigned hz : {30u,60u,120u}) for (unsigned state : {400u,401u,402u,403u,404u,405u,406u}) {
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
        // A retained approach selection is not a new flip and must not discard
        // the released rail recipe before native Ollie selection catches up.
        for(unsigned fault=0;fault<4;++fault) {
            Pipeline p; unsigned submissions{};
            p.bridge={&submissions,[](void* context,std::int32_t id) noexcept {
                if(id!=2)return false;
                ++*static_cast<unsigned*>(context);return true;
            }};
            p.step(1000,401,hz,4);p.poll(1000,0);
            p.step(1000+1000/hz,401,hz,4);p.poll(1000+1000/hz,classic_a);
            for(unsigned i=2;i<26;++i) {
                const auto now=1000+i*1000/hz;p.step(now,401,hz,4);p.poll(now,classic_a);
            }
            const auto release=1000+26*1000/hz;
            p.step(release,401,hz,4);p.poll(release,0);
            check(p.input.rail_followup_active(release,p.frame),"unchanged approach selection retains released rail lease");
            for(unsigned i=27;i<34;++i) {
                const auto now=1000+i*1000/hz;
                const int selected=fault==1?4:fault==2?5:fault==3?-1:i==27?4:1;
                p.step(now,i==27?701:i<30?200:201,hz,selected,16,i<28?1:2);
                p.poll(now,0);
            }
            const auto press=1000+34*1000/hz;
            p.step(press,201,hz,fault==1?4:fault==2?5:fault==3?-1:1,16,2);
            const auto out=p.poll(press,classic_x);
            check((out.native_flip_request==2)==(fault==0),
                "only observed new native Ollie permits early flip after retained approach selection");
            p.poll(press,classic_x);
            check(submissions==(fault==0?1u:0u),"retained approach selection never fabricates or repeats a native flip");
        }
        for(unsigned fault=0;fault<10;++fault) {
            Pipeline p; unsigned submissions{};
            p.bridge={&submissions,[](void* context,std::int32_t id) noexcept {
                if(id!=2)return false;
                ++*static_cast<unsigned*>(context);return true;
            }};
            p.step(1000,401,hz);p.poll(1000,0);
            p.step(1000+1000/hz,401,hz);p.poll(1000+1000/hz,classic_a);
            for(unsigned i=2;i<26;++i) {
                const auto now=1000+i*1000/hz;p.step(now,401,hz,1);p.poll(now,classic_a);
            }
            const auto release=1000+26*1000/hz;
            p.step(release,403,hz,1);p.poll(release,0);
            check(p.input.rail_followup_active(release,p.frame),"admitted rail release exposes its own native follow-up lease");
            for(unsigned i=27;i<34;++i) {
                const auto now=1000+i*1000/hz;
                if(i==27 && fault==1)++p.owner.entity;
                p.step(now,i==27?(fault==2?100:fault==3?300:701):i<30?200:201,hz,1,
                    fault==4?17:16,i<28?1:2);
                // Physics-only samples expose an intermediate forbidden state,
                // even when no game-facing input poll observes that state.
                if(i>=28)p.poll(now,0,fault!=5);
            }
            const auto press=1000+34*1000/hz;
            if(fault==6)p.input.enable(false);
            p.step(press,201,hz,fault==7?2:1,16,2);
            if(fault==8)p.input.publish_native_context({});
            if(fault==9) {
                p.bridge={};
                // A real catalog contains rail flip recipes. Missing native
                // bridge permission must not silently select one as fallback.
                p.catalog.emplace_back(2,false,GestureContext::from_grind,
                    std::vector<AuthoredStickStep>{{0,0,-27000},{18,27000,27000},{24,0,0}});
            }
            auto out=p.poll(press,classic_x,fault!=5);
            check((out.native_flip_request==2)==(fault==0),"only continuous rail release to AIR submits the early native flip");
            check(!out.requested_id && !out.authored_playback,"early native follow-up does not emit another rail flick");
            p.poll(press,classic_x,fault!=5);
            check(submissions==(fault==0?1u:0u),"one physical X cannot repeat native submission; invalid owners never submit");
            p.step(press+1000/hz,201,hz,2,16,2);p.poll(press+1000/hz,0,fault!=5);
            if(!fault)check(p.input.rail_followup_active(press+1000/hz,p.frame),
                "submitted native selection retains ownership during motion initialization");
            p.step(press+2*1000/hz,100,hz);
            check(!p.input.rail_followup_active(press+2*1000/hz,p.frame),"landing retires rail native follow-up ownership");
        }
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
        for (unsigned phase : {400u,402u,403u,404u,405u,406u}) {
            Pipeline p; p.step(1000,401,hz); p.poll(1000,0);
            p.step(1034,401,hz); p.poll(1034,classic_a);
            p.step(1068,phase,hz);
            check(p.poll(1068,classic_a).right_y==-28000,"native grind class change retains an admitted hold");
            p.step(1102,401,hz);
            check(p.poll(1102,0).jump.setup_admitted,"native grind class change does not discard the release");
        }
        for (unsigned state : {399u,407u,499u,701u,300u,504u}) {
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
