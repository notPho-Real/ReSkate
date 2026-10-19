#include "Extension/ArcadeMode/arcade_rail_adapter.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* message) { ++checks; if (!value) { std::cerr << message << '\n'; std::exit(1); } }
InputNativeContext frame(std::uint64_t step = 1, std::uint64_t now = 10, bool rail = false) {
    return {true,true,{1,step,step},step,now,rail,!rail,1,rail ? 1u : 0u};
}
RailAdapterPad pad(RailAdapter& actor, std::uint64_t now = 10) { return actor.sample(now,true,true,true,false); }
RailAdapterResult start(RailAdapter& actor, RailAdapterMethod method = RailAdapterMethod::align,
        std::uint64_t now = 10, std::uint64_t deadline = 1000) {
    return actor.request(method,now,deadline,true,true,false);
}
}
int main() {
    for (auto method : {RailAdapterMethod::align,RailAdapterMethod::nose_pressure,RailAdapterMethod::tail_pressure}) {
        RailAdapter actor; actor.observe(frame(),200);
        check(start(actor,method) == RailAdapterResult::started,"supported board method starts in verified air");
        const auto first = pad(actor);
        const std::int16_t expected = method == RailAdapterMethod::nose_pressure ? 20000 : method == RailAdapterMethod::tail_pressure ? -20000 : 0;
        check(first.owns_right_stick && first.right_x == 0 && first.right_y == expected && first.button_mask == 0,"only right stick pressure owned");
        actor.observe(frame(2,20,true),401);
        const auto engaged = pad(actor,20);
        check(engaged.owns_right_stick && engaged.right_y == first.right_y,"unchanged air to rail delivery");
        actor.observe(frame(3,30,true),403);
        check(pad(actor,30).owns_right_stick && actor.phase() == RailAdapterPhase::rail_contact,"rail end is contact not invented exit or table epoch");
        auto air = frame(4,40);air.airborne_episode = 2;air.air_departure = 1;actor.observe(air,200);
        check(pad(actor,40).owns_right_stick,"continuous native pop retains caller-held board method");
        check(!actor.sample(41,false,true,true,false).owns_right_stick && !actor.active(),"release cancels once");
        check(!pad(actor,42).owns_right_stick,"held level cannot restart after cancellation");
    }
    RailAdapter actor; actor.observe(frame(),201);
    check(start(actor,RailAdapterMethod::dark_catch)==RailAdapterResult::started,"candidate native catch method starts in air");
    const auto dark = pad(actor);
    check(!dark.owns_right_stick && dark.button_mask == 0x0200 && dark.button_values == 0x0200,"catch owns only RB bit and preserves camera and all other input");
    actor.observe(frame(2,20,true),401);
    check(pad(actor,20).button_values==0x0200,"catch remains held through physical rail contact");
    actor.cancel();check(pad(actor,21).button_mask==0,"explicit native actor cancellation releases its mask");
    check(start(actor,RailAdapterMethod::dark_catch,20)==RailAdapterResult::unsafe,"rail contact cannot initiate an air catch");
    for (auto method : {RailAdapterMethod::named_grind,RailAdapterMethod::lip,static_cast<RailAdapterMethod>(255)}) {
        check(start(actor,method,20)==RailAdapterResult::unsupported && !actor.active(),"unknown named and lip methods never substitute generic pressure");
    }
    for (unsigned reason = 0; reason < 10; ++reason) {
        RailAdapter a;a.observe(frame(),200);check(start(a)==RailAdapterResult::started,"cancellation setup");
        auto next=frame(2,20);
        if(reason==0) {next.airborne=false;a.observe(next,100);}
        if(reason==1) {next.generations.owner=2;a.observe(next,200);}
        if(reason==2) {next.physics_step=3;a.observe(next,200);}
        if(reason==3) {next.valid=false;a.observe(next,200);}
        if(reason==4) {next.sampled_at=9;a.observe(next,200);}
        if(reason==5) {next.rail_family=true;a.observe(next,200);}
        if(reason==6) a.sample(20,true,false,true,false);
        if(reason==7) a.sample(20,true,true,false,false);
        if(reason==8) a.sample(20,true,true,true,true);
        if(reason==9) a.sample(161,true,true,true,false);
        check(!pad(a,20).owns_right_stick && !a.active(),"unsafe continuity or context loss releases board method");
    }
    RailAdapter lease;lease.observe(frame(),200);check(start(lease,RailAdapterMethod::align,10,160)==RailAdapterResult::started,"bounded deadline supplied by caller");
    check(pad(lease,160).owns_right_stick,"inclusive native sample lease and deadline");
    check(!pad(lease,161).owns_right_stick,"poll cannot renew timestamp or deadline");
    RailAdapter duplicate;duplicate.observe(frame(),200);start(duplicate);duplicate.observe(frame(),200);
    check(pad(duplicate).owns_right_stick,"identical cached frame permitted");
    auto changed=frame();changed.generations.action++;duplicate.observe(changed,200);
    check(!pad(duplicate).owns_right_stick,"same physics step cannot change correlation");
    RailAdapter changed_state;changed_state.observe(frame(),200);start(changed_state);changed_state.observe(frame(),201);
    check(!pad(changed_state).owns_right_stick && start(changed_state)==RailAdapterResult::unsafe,"same-step state mutation cannot authorize replacement");
    RailAdapter ack;ack.observe(frame(),200);start(ack);ack.observe(frame(2,20),201);
    check(pad(ack,20).owns_right_stick,"own selection and continuous AIR do not cancel");
    check(ack.request(RailAdapterMethod::align,20,1000,true,true,false)==RailAdapterResult::busy,"no concurrent actor ownership");
    ack.reset();ack.observe(frame(),200);
    check(ack.request(RailAdapterMethod::align,10,1000,true,true,true)==RailAdapterResult::busy,"external ownership excludes start");
    check(start(ack,RailAdapterMethod::align,10,9)==RailAdapterResult::unsafe,"expired request never starts");
    std::cout << checks << " rail adapter checks passed\n";
}
