#include "Extension/ArcadeMode/arcade_authored_playback.h"
#include "Extension/ArcadeMode/arcade_playback_packet.h"
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}

#ifdef ARCADE_PLUMBING_COMBINED_TESTS
int run_arcade_action_dispatch_tests();
int run_arcade_revert_tests();
int run_arcade_action_observation_tests();
int run_arcade_balance_layout_tests();
int run_arcade_action_runtime_tests();
int run_arcade_manual_tests();
int run_arcade_score_event_tests();
#endif
int run_arcade_playback_tests() {
#ifdef ARCADE_PLUMBING_COMBINED_TESTS
    run_arcade_action_dispatch_tests();
    run_arcade_revert_tests();
    run_arcade_action_observation_tests();
    run_arcade_balance_layout_tests();
    run_arcade_action_runtime_tests();
    run_arcade_manual_tests();
    run_arcade_score_event_tests();
#endif
    using namespace dingosdk::arcade_mode;
    constexpr std::array<AuthoredStickStep, 6> steps{{
        {0, -3000, -32767}, {18, 12000, 20000}, {19, 26000, 9000},
        {20, -22000, 15000}, {24, 0, 0}, {56, 0, 0}}};
    AuthoredPlayback delivered, absolute;
    check(delivered.schedule() == AuthoredPlaybackSchedule::delivery_relative,
        "existing delivery schedule remains the default");
    absolute.set_schedule(AuthoredPlaybackSchedule::absolute_ticks);
    check(delivered.start(steps, 100, false) && absolute.start(steps, 100, false),
        "both policies accept the identical authored script");
    check(delivered.sample(118).x == 12000 && absolute.sample(118).x == 12000,
        "both schedules deliver the first arc on its authored tick");
    check(delivered.sample(120).x == 26000 && absolute.sample(120).x == -22000,
        "late poll preserves an arc in delivery mode and catches up in absolute mode");
    check(delivered.sample(120).x == 26000 && absolute.sample(120).x == -22000,
        "repeated same-tick reads do not advance either schedule");
    check(delivered.sample(121).x == -22000 && absolute.sample(121).x == -22000,
        "late arc reaches the delivery schedule one tick later");
    check(delivered.sample(124).x == -22000 && absolute.sample(124).x == 0,
        "absolute neutral release is not delayed by a missed poll");
    check(delivered.sample(125).x == 0 && absolute.sample(125).owns_stick,
        "both policies retain neutral ownership until completion");
    check(delivered.sample(156).owns_stick && !absolute.sample(156).owns_stick && !absolute.busy(),
        "absolute completion retains its original tick after late polling");
    check(!delivered.sample(157).owns_stick && !delivered.busy(),
        "delivery completion preserves the existing shifted timing");

    for (const auto schedule : {AuthoredPlaybackSchedule::delivery_relative, AuthoredPlaybackSchedule::absolute_ticks}) {
        AuthoredPlayback playback;
        playback.set_schedule(schedule);
        check(playback.start(steps, 0, true, true), "held reflected script starts near tick zero");
        check(playback.sample(0).x == 3000 && playback.sample(400).y == -32767,
            "held crouch stays owned beyond the released safety horizon");
        playback.release(401);
        check(playback.sample(401).x == -12000 && playback.sample(401).y == 20000,
            "release immediately delivers the first reflected flick arc");
        check(playback.sample(403).x == (schedule == AuthoredPlaybackSchedule::absolute_ticks ? 22000 : -26000),
            "held release retains each policy's skipped-poll behavior");
        check(steps[1].x == 12000, "reflection never changes the shared prepared script");
        check(!playback.sample(581).owns_stick && !playback.busy(),
            "both released schedules expire at the absolute safety bound");
        check(playback.start(steps, 10, false), "script restarts after expiry");
        playback.sample(20);
        check(!playback.sample(19).owns_stick && !playback.busy(),
            "backwards sampling cancels both schedules");
        playback.start(steps, 100, true);
        playback.sample(110);
        playback.release(109);
        check(!playback.busy(), "backwards release cancels held input");
    }

    absolute.start(steps, 10, true);
    absolute.release(11);
    check(absolute.sample(11).x == 12000 && absolute.sample(13).x == -22000,
        "absolute release offset works without subtracting eighteen from a small origin");
    check(absolute.sample(17).x == 0 && absolute.sample(17).owns_stick,
        "held absolute release reaches neutral at offset six");
    check(!absolute.sample(49).owns_stick && !absolute.busy(),
        "held absolute completion accounts for the removed eighteen-tick setup");
    absolute.start(steps, 100, false);
    absolute.set_schedule(AuthoredPlaybackSchedule::absolute_ticks);
    check(absolute.busy(), "reapplying the current schedule preserves a live script");
    absolute.set_schedule(AuthoredPlaybackSchedule::delivery_relative);
    check(!absolute.busy() && !absolute.sample(118).owns_stick,
        "changing schedule cancels live synthetic ownership");
    absolute.set_schedule(AuthoredPlaybackSchedule::absolute_ticks);
    absolute.start(steps, 200, false);
    check(!absolute.sample(256).owns_stick && !absolute.busy(),
        "first late poll beyond completion cannot replay stale arcs");
    absolute.start(steps, 300, false);
    absolute.cancel();
    check(absolute.schedule() == AuthoredPlaybackSchedule::absolute_ticks && !absolute.sample(318).owns_stick,
        "cancellation preserves the chosen policy and clears output");
    PlaybackPacket packet;
    check(packet.sample(12, 42, false) == 42, "ordinary packet behavior is unchanged");
    check(packet.sample(12, 42, true) == 13 && packet.sample(12, 42, true) == 14,
        "reference packets advance on repeated same-tick physical polls");
    check(packet.sample(15, 42, true) == 15,
        "a changing physical packet cannot cancel the synthetic increment through XOR");
    check(packet.sample(15, 15, false) == 16 && packet.sample(15, 15, false) == 15,
        "cancellation remains visible when the physical counter catches up");
    check(packet.sample(UINT32_MAX, 0, true) == 0 && packet.sample(0, 0, true) == 1,
        "reference packet wrap uses defined unsigned arithmetic");
    std::cout << checks << " authored playback checks passed\n";
    return 0;
}

#ifndef ARCADE_PLAYBACK_COMBINED_TESTS
int main() { return run_arcade_playback_tests(); }
#endif
