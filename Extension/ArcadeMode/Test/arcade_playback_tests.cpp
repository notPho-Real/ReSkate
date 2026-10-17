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
int run_arcade_score_numbers_tests();
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
    run_arcade_score_numbers_tests();
#endif
    using namespace dingosdk::arcade_mode;
    constexpr std::array<AuthoredStickStep, 6> steps{{
        {0, -3000, -32767}, {18, 12000, 20000}, {19, 26000, 9000},
        {20, -22000, 15000}, {24, 0, 0}, {56, 0, 0}}};
    AuthoredPlayback delivered, absolute;
    const auto air = compile_airborne_flick(steps);
    check(air.front().tick == 0 && !air.front().x && !air.front().y && air[1].tick == 2,
        "source-derived late stroke starts with30ms native neutral reset");
    check(air[2].tick == 3 && air[2].x == steps[0].x && air[2].y == steps[0].y && air[3].tick == 5,
        "late stroke loads native setup then holds it for35ms instead of a single frame");
    check(air[3].x == steps[1].x && air[4].tick == 6 && air[4].x == steps[2].x,
        "late stroke retains the fast authored path after a stable native setup");
    check(!air[air.size()-2].x && air.back().tick - air[air.size()-3].tick == 6,
        "late stroke holds its end for60ms then releases a bounded neutral tail");
    const auto ready_air = compile_airborne_flick(steps, false);
    {
        AuthoredPlayback phase;
        check(phase.start(ready_air, 100, false, false, true), "air timing phase control starts");
        phase.sample(100, 1000);
        check(phase.sample(102, 1020).x == ready_air[0].x,
            "an early tick publication cannot shorten the native35ms setup");
        check(phase.sample(104, 1035).x == ready_air[1].x,
            "native setup advances once both game time and delivered35ms elapsed");
    }
    check(ready_air[0].tick == 0 && ready_air[0].y == steps[0].y && ready_air[1].tick == 2 && ready_air[1].x == steps[1].x,
        "proven native neutral permits immediate setup with the same35ms load and fast authored flick");
    NeutralStickDelivery neutral;
    neutral.observe(100, 10, true, 0, 0);
    check(!neutral.ready(140), "one delivered neutral poll cannot prove a reset by waiting");
    neutral.observe(140, 10, true, 0, 0);
    check(!neutral.ready(140), "a paused game tick cannot prove native neutral delivery");
    neutral.observe(141, 11, true, 0, 0);
    check(neutral.ready(141) && neutral.ready(291) && !neutral.ready(292), "neutral proof needs distinct ticks and expires with the gameplay freshness contract");
    neutral.observe(150, 12, true, 0, 20000);
    check(!neutral.ready(150), "manual or another native stick owner invalidates neutral immediately");
    neutral.observe(160, 13, true, 0, 0); neutral.observe(189, 14, true, 0, 0);
    check(!neutral.ready(189), "a new neutral tail must finish its own30ms reset");
    neutral.observe(190, 15, true, 0, 0);
    check(neutral.ready(190), "a completed neutral tail rearms the fast air path");
    neutral.observe(191, 16, false, 0, 0);
    check(!neutral.ready(191), "menu, OFF and offboard transitions invalidate input ownership provenance");
    neutral.observe(200, 17, true, 0, 0); neutral.observe(240, 18, true, 0, 0); neutral.observe(239, 19, true, 0, 0);
    check(!neutral.ready(240), "clock reversal cannot preserve neutral proof");
    const AuthoredGesture native{2, false, GestureContext::ground, 0, 0, 0,
        {{280, 30, .5f, 1, 0}, {90, 150, .5f, 1, .1f}}, 256};
    const auto normal_range = compile_authored_flick(native);
    const auto full_range = compile_authored_flick(native, 24, true);
    check(std::hypot(full_range[1].x, full_range[1].y) > 32765 &&
          std::hypot(normal_range[1].x, normal_range[1].y) < 24576,
          "fast native flick uses the authored allowed maximum rather than a weaker midpoint");
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
        AuthoredPlayback handoff;
        handoff.set_schedule(schedule);
        handoff.start(steps, 0, true);
        check(!handoff.finish_neutral_tail(), "held setup cannot yield to an airborne action");
        handoff.release(100);
        handoff.sample(100);
        check(!handoff.finish_neutral_tail(), "active authored arcs cannot yield to an airborne action");
        for (unsigned tick = 101; tick <= 106; ++tick) handoff.sample(tick);
        check(handoff.busy() && handoff.finish_neutral_tail() && !handoff.busy(),
            "delivered neutral tail permits immediate airborne action ownership");
        check(!handoff.sample(107).owns_stick && !handoff.finish_neutral_tail(),
            "handoff releases once and never resumes stale authored input");
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
