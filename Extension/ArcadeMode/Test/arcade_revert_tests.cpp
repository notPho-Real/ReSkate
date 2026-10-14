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
    const auto fs = make_revert_recipe(RevertSide::frontside);
    const auto bs = make_revert_recipe(RevertSide::backside);
    unsigned duration{}, rotations{};
    for (const auto& step : fs) {
        duration += step.duration_ms;
        rotations += step.phase == RevertPhase::rotation;
        check(step.right_x == 0 && step.right_y == -32767,
            "the full regular pop remains held throughout the candidate");
    }
    check(duration == 235 && rotations == 33,
        "inclusive circle endpoint produces the reference's actual 235 ms duration");
    check(fs[1].left_x == 32767 && fs[1].left_y == 0 &&
        fs[9].left_x == 0 && fs[9].left_y == -32767 &&
        fs[17].left_x == -32767 && fs[25].left_y == 32767,
        "frontside candidate starts right and rotates clockwise in game coordinates");
    check(bs[1].left_x == -32767 && bs[9].left_y == -32767 &&
        bs[17].left_x == 32767 && bs[25].left_y == 32767,
        "backside candidate starts left and rotates counterclockwise");
    check(fs[33].left_x == fs[1].left_x && fs[33].left_y == fs[1].left_y,
        "the final rotational sample repeats the starting position");
    for (const bool nollie : {false, true}) {
        const auto mirrored = make_revert_recipe(RevertSide::frontside, {nollie, true, false});
        const auto regular = make_revert_recipe(RevertSide::frontside, {nollie, false, false});
        for (unsigned i = 0; i < fs.size(); ++i) {
            check(mirrored[i].left_x == bs[i].left_x && mirrored[i].left_y == bs[i].left_y,
                "stance mirroring reverses the requested circle including its starting side");
            check(regular[i].left_x == fs[i].left_x && regular[i].left_y == fs[i].left_y &&
                regular[i].right_y == (nollie ? 32767 : -32767),
                "default nollie context changes pop polarity without reversing the circle");
        }
    }
    const auto twice_mirrored = make_revert_recipe(RevertSide::frontside, {true, true, true});
    check(twice_mirrored[1].left_x == fs[1].left_x && twice_mirrored[9].left_y == fs[9].left_y,
        "explicit nollie and stance mirroring compose without ambiguity");

    RevertPlayback playback;
    check(!playback.start(100, RevertSide::frontside, {}, false) && !playback.busy(),
        "ineligible requests cannot acquire synthetic input");
    check(playback.start(100, RevertSide::frontside, {}, true), "eligible candidate starts");
    check(!playback.start(110, RevertSide::backside, {}, true),
        "repeated requests cannot restart or extend an active candidate");
    check(playback.sample(129, true).phase == RevertPhase::setup &&
        playback.sample(130, true).left_x == 32767,
        "lead phase transitions to circle at the absolute 30 ms boundary");
    check(playback.sample(170, true).left_y == -32767 &&
        playback.sample(170, true).left_y == -32767,
        "late polls catch up and repeated polls do not advance the circle");
    const auto tail = playback.sample(295, true);
    check(tail.phase == RevertPhase::settle && tail.left_x == 0 && tail.left_y == 0 &&
        tail.right_y == -32767 && tail.left_trigger == 0 && tail.right_trigger == 0,
        "tail centers left stick while pop remains held and triggers remain released");
    check(playback.sample(334, true).owns_sticks && !playback.sample(335, true).owns_sticks &&
        !playback.busy(), "ownership expires at 235 ms despite missed polls and repeated requests");
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
