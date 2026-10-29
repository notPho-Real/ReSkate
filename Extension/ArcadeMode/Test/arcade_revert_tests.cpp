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
}

int run_arcade_revert_tests() {
    using namespace dingosdk::arcade_mode;
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
