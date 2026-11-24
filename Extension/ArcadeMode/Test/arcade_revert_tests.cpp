#include "Extension/ArcadeMode/arcade_revert_input.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
void landing_path() {
    using namespace dingosdk::arcade_mode;
    // Independent checkpoints from the retained stock shape5 timing. In the
    // failed landing trace the old path was already at bottom by160ms, then
    // paused there until200ms. Native101 alone did not complete a180.
    struct Point {unsigned at;short x,y;};
    constexpr Point reference[]{{0,0,0},{20,16384,0},{40,32767,0},
        {120,23170,-23170},{160,12539,-30273},{200,0,-32767},
        {255,-25996,-19947},{310,-31650,8481},{450,-31650,8481},
        {579,-31650,8481},{580,0,0},{679,0,0}};
    for(bool mirror:{false,true})for(auto side:{RevertSide::frontside,RevertSide::backside}) {
        const int sign=((side==RevertSide::frontside)!=mirror)?1:-1;
        RevertPlayback playback;
        check(playback.start(100,side,{false,mirror,false,0,true},true),"qualified landing starts existing actuator");
        for(const auto& point:reference) {
            const auto out=playback.sample(100+point.at,true);
            check(out.owns_sticks && std::abs(out.left_x-sign*point.x)<=1 && std::abs(out.left_y-point.y)<=1,
                "landing follows stock neutral ramp, continuous quarter/sweep, hold and neutral boundaries");
            check(!out.right_x && !out.right_y && !out.left_trigger && !out.right_trigger,
                "landing does not manufacture Manual support or leak trigger commands");
        }
        check(!playback.sample(780,true).owns_sticks,"landing releases at original680ms deadline");
        for(unsigned cadence:{8u,17u,34u}) {
            check(playback.start(1000,side,{false,mirror,false,0,true},true),"fresh landing cadence run");
            double previous=90;bool crossed{},held{},neutral{};
            for(unsigned at=0;at<680;at+=cadence) {
                const auto out=playback.sample(1000+at,true),repeat=playback.sample(1000+at,true);
                check(out.owns_sticks && out.left_x==repeat.left_x && out.left_y==repeat.left_y,
                    "30/60/120Hz and repeated polls preserve original landing clock");
                if(at>=40 && at<580) {
                    auto angle=std::atan2(double(sign*out.left_x),double(out.left_y));
                    if(angle<0)angle+=2*3.14159265358979323846;
                    const auto degrees=angle*180/3.14159265358979323846;
                    check(degrees+.01>=previous && degrees<285.01,"native landing sweep is monotonic without reverse corners");
                    previous=degrees;crossed|=degrees>=180;held|=at>=310;
                }
                neutral|=at>=580 && !out.left_x && !out.left_y;
            }
            check(crossed && held && neutral && !playback.sample(1680,true).owns_sticks,
                "all cadences cross bottom, hold the endpoint and release on time");
        }
    }
    for(short hold:{short(-20000),short(20000)}) {
        RevertPlayback manual,landing_flag;
        manual.start(100,RevertSide::frontside,{false,false,false,hold},true);
        landing_flag.start(100,RevertSide::frontside,{false,false,false,hold,true},true);
        for(unsigned at=100;at<800;at+=17) {
            const auto a=manual.sample(at,true),b=landing_flag.sample(at,true);
            check(a.owns_sticks==b.owns_sticks && a.left_x==b.left_x && a.left_y==b.left_y && a.right_y==b.right_y,
                "established Manual/Pivot keeps byte-identical proven playback even with a landing flag");
        }
    }
    for(bool allowed:{false,true}) {
        RevertPlayback playback;playback.start(100,RevertSide::frontside,{false,false,false,0,true},true);
        playback.sample(110,true);
        check(!playback.sample(allowed?261:120,allowed).owns_sticks && !playback.busy(),
            "landing cancels on ownership loss or stale poll instead of replaying its path");
    }
}
}

int run_arcade_revert_tests() {
    using namespace dingosdk::arcade_mode;
    landing_path();
    for (bool mirror : {false,true}) for (auto side : {RevertSide::frontside,RevertSide::backside})
    for (short hold : {short(0),short(-20000),short(20000)}) {
        const auto recipe=make_revert_recipe(side,{false,mirror,false,hold});
        unsigned duration{};
        for(const auto& point:recipe) {
            duration+=point.duration_ms;
            check(point.right_x==0 && point.right_y==hold,"revert preserves only the qualified partial Manual hold; ordinary ground has no crouch");
        }
        check(duration==revert_recipe_duration_ms,"source-supported path duration includes endpoint and neutral settle");
        const int sign=((side==RevertSide::frontside)!=mirror)?1:-1;
        check(recipe[0].left_x==sign*32767 && recipe[4].left_y==-32767 && recipe[8].left_x*sign<0 && recipe[8].left_y>0,
            "both stances sweep from side through bottom to opposite upper side");
        for(unsigned cadence:{8u,17u,34u}) {
            RevertPlayback playback;
            check(playback.start(100,side,{false,mirror,false,hold},true),"fresh discrete request starts");
            unsigned completed{};unsigned visits{};bool bottom{},crossed{},neutral{};short previous_x=123,previous_y=123;
            for(unsigned now=100;now<1150;now+=cadence) {
                auto out=playback.sample(now,true);
                if(!out.owns_sticks){completed=now-100;break;}
                const auto repeat=playback.sample(now,true);
                check(out.left_x==repeat.left_x && out.left_y==repeat.left_y,"same-time input polls cannot consume multiple waypoints");
                check(out.right_y==hold && !out.left_trigger && !out.right_trigger,"quick RT release does not interrupt the owned path or leak grab input");
                if(out.left_x!=previous_x || out.left_y!=previous_y){++visits;previous_x=out.left_x;previous_y=out.left_y;}
                bottom|=out.left_y==-32767;
                crossed|=bottom && out.left_x*sign<0 && out.left_y>0;
                neutral|=crossed && !out.left_x && !out.left_y;
                check(!playback.start(now,side,{},true),"held/repeated request cannot restart active path");
            }
            check(completed>=680 && completed<680+cadence,"cumulative deadlines bound total duration to one poll of nominal time without skipped geometry");
            check(visits>=10 && bottom && crossed && neutral && !playback.busy(),"30/60/120Hz reaches every essential native path boundary then releases");
        }
    }
    RevertPlayback playback;
    check(!playback.start(100,RevertSide::frontside,{},false),"ineligible revert is rejected");
    playback.start(1000, RevertSide::backside, {}, true);
    check(!playback.sample(2000, true).owns_sticks && !playback.busy(),
        "an entirely missed gesture cannot replay stale input");
    playback.start(3000, RevertSide::frontside, {}, true);
    playback.sample(3010, true);
    check(!playback.sample(3009, true).owns_sticks && !playback.busy(),
        "backwards clocks release ownership immediately");
    playback.start(4000, RevertSide::frontside, {}, true);
    check(!playback.sample(4010, false).owns_sticks && !playback.busy(),
        "loss of gameplay eligibility immediately cancels playback");
    playback.start(5000, RevertSide::frontside, {}, true);
    playback.cancel();
    const auto canceled = playback.sample(5010, true);
    check(!canceled.owns_sticks && canceled.phase == RevertPhase::inactive && canceled.right_y == 0,
        "explicit cancellation clears synthetic output");
    const auto near_limit = std::numeric_limits<std::uint64_t>::max() - 100;
    playback.start(near_limit, RevertSide::frontside, {}, true);
    check(playback.sample(near_limit + 40, true).owns_sticks && !playback.sample(0, true).owns_sticks,
        "clock rollover cancels without overflowing a computed expiry deadline");
    std::cout << checks << " revert candidate checks passed\n";
    return 0;
}

#ifndef ARCADE_REVERT_COMBINED_TESTS
int main() { return run_arcade_revert_tests(); }
#endif
