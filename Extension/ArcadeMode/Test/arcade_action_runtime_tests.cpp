#include "Extension/ArcadeMode/arcade_action_runtime.h"
#include "Extension/ArcadeMode/arcade_playback_packet.h"
#include "Extension/ArcadeMode/arcade_native_events.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
namespace {
unsigned checks{};
void check(bool pass, const char* reason) {
    ++checks;
    if (!pass) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
}
int run_arcade_action_runtime_tests() {
    using namespace dingosdk::arcade_mode;
    ActionRuntime input;
    auto poll = [&](ActionExperiment mode, std::uint64_t ms, ActionPhysicalPad pad = {},
                    SkatePose pose = SkatePose::riding, bool active = true, bool fresh = true,
                    bool inspected = true, bool other = false) {
        return input.poll(mode, ms, pad, active, fresh, pose, inspected, other);
    };
    check(static_cast<unsigned>(ActionExperiment::off) == 0 &&
        static_cast<unsigned>(ActionExperiment::observe) == 1 &&
        static_cast<unsigned>(ActionExperiment::grabs) == 2 &&
        static_cast<unsigned>(ActionExperiment::revert_frontside) == 3 &&
        static_cast<unsigned>(ActionExperiment::revert_backside) == 4 &&
        static_cast<unsigned>(default_action_mode) == 5, "default mode appends without changing diagnostic identifiers");
    poll(default_action_mode, 0);
    auto classic_out = poll(default_action_mode, 10, {0, 0, 0, 0, 255});
    check(classic_out.requested && classic_out.result == DispatchResult::started && classic_out.owned &&
        classic_out.consume_rt && !classic_out.consume_buttons && classic_out.request_kind == 3,
        "default RT requests the existing native recipe without enabling grab candidates");
    check(classic_out.pad.right_y == -32767 && classic_out.pad.left_trigger == 0 && classic_out.pad.right_trigger == 0,
        "default generic revert uses verified regular setup and owns triggers");
    check(poll(default_action_mode, 40, {0, 0, 0, 0, 255}).pad.left_x > 32000,
        "default route reuses the existing rotation direction without claiming native side");
    check(!poll(default_action_mode, 245, {0, 0, 0, 0, 255}).owned && !input.busy(),
        "default ownership ends at original235ms deadline");
    check(!poll(default_action_mode, 500, {0, 0, 0, 0, 255}).requested,
        "held default RT never restarts after expiry");
    poll(default_action_mode, 510);
    classic_out = poll(default_action_mode, 520, {0, 0, 0, 255, 255});
    check(classic_out.requested && classic_out.result == DispatchResult::unsafe_context &&
        !classic_out.owned && !classic_out.pad.owns_triggers, "default LT plus RT cannot start unverified nollie revert");
    check(!poll(default_action_mode, 530, {0, 0, 0, 0, 255}).requested,
        "releasing LT alone cannot turn a rejected held RT into an action");
    poll(default_action_mode, 540);
    check(poll(default_action_mode, 550, {0, 0, 0, 0, 255}).owned,
        "fresh RT edge works after rejected context releases");
    check(!poll(default_action_mode, 560, {0, 0, 0, 255, 255}).owned && !input.busy(),
        "LT pressed during default playback cancels ownership without changing recipe");
    for (unsigned conflict : {0x1000u, 0x4000u, 0x0100u, 0x0200u, 0x0300u, 0x0010u}) {
        input.reset(); poll(default_action_mode, 600);
        const auto denied = poll(default_action_mode, 610,
            {static_cast<std::uint16_t>(conflict), 0, 0, 0, 255});
        check(!denied.owned && denied.result == DispatchResult::busy,
            "default revert yields to authored commands, spin, board pair and menu");
    }
    for (unsigned cancellation = 0; cancellation < 6; ++cancellation) {
        input.reset(); poll(default_action_mode, 700);
        check(poll(default_action_mode, 710, {0, 0, 0, 0, 255}).owned, "default cancel test starts a real owner");
        const auto mode = cancellation == 0 ? ActionExperiment::off : default_action_mode;
        const auto canceled = poll(mode, 720, {0, 0, 0, 0, 255},
            cancellation == 5 ? SkatePose::air : SkatePose::riding,
            cancellation != 1, cancellation != 2, cancellation != 3, cancellation == 4);
        check(!canceled.owned && !input.busy(),
            "off, Arcade OFF, stale state, uninspected stance, new owner and air cancel default playback");
        if (cancellation <= 2)
            check(!canceled.consume_rt && !canceled.consume_buttons, "inactive path restores native channels without masking");
        check(!poll(default_action_mode, 730, {0, 0, 0, 0, 255}).requested,
            "held RT cannot replay when an interrupted default context becomes eligible");
    }
    input.reset(); poll(default_action_mode, 800);
    check(!poll(default_action_mode, 810, {0, 0, 0, 0, 255}, SkatePose::manual).owned,
        "default RT cannot fabricate a riding revert from manual pose");
    {
        // Classic command routing reads LT before auxiliary ownership, unchanged.
        ArcadeInput classic;
        classic.enable(true);
        const std::array catalog{PreparedGesture{16, true, GestureContext::ground,
            {{0, 0, 32767}, {18, 0, -32767}, {22, 0, 0}}}};
        const auto routed = [&](unsigned tick, std::uint16_t buttons) {
            return classic.poll(0, tick * 17, buttons, 0, 0, 0, 0, true, true,
                false, 0, true, true, true, tick, true, catalog, true, true, SkatePose::riding);
        };
        routed(0, 0);
        const auto nollie = routed(1, ClassicGesture::a);
        check(nollie.requested_id == 16 && nollie.authored_playback && nollie.right_y == 32767,
            "LT retains authored nollie A routing with production RT available");
        classic.enable(false);
        const auto native = classic.poll(0, 40, 0x0300, 100, 200, 300, 400, true, true);
        check(native.buttons == 0x0300 && native.right_x == 300 && native.right_y == 400 && !native.phase,
            "Arcade OFF retains physical shoulders and right stick");
    }
    input.reset();
    check(!poll(ActionExperiment::off, 0, {0x2004}).owned, "off never owns input");
    poll(ActionExperiment::observe, 1);
    poll(ActionExperiment::observe, 10, {0x2004});
    auto out = poll(ActionExperiment::observe, 190, {4});
    check(out.requested && out.trick == ArcadeTrick::melon && !out.owned && !out.consume_buttons,
        "observe records exact B intent without actuation or masking");
    input.reset();
    poll(ActionExperiment::grabs, 200, {0x2004}, SkatePose::air);
    check(!poll(ActionExperiment::grabs, 400, {0x2004}, SkatePose::air).requested,
        "held B across enabling requires release");
    poll(ActionExperiment::grabs, 410, {}, SkatePose::air);
    poll(ActionExperiment::grabs, 420, {0x2004}, SkatePose::air);
    out = poll(ActionExperiment::grabs, 600, {4}, SkatePose::air);
    check(out.requested && out.result == DispatchResult::started && out.owned && out.consume_buttons == 0x2000,
        "explicit grabs mode routes B candidate only in observed air");
    check(out.pad.right_trigger == 255 && !out.pad.owns_left_stick, "grab reserves trigger but preserves steering");
    check(poll(ActionExperiment::grabs, 650, {4}, SkatePose::air).pad.right_y == 32767,
        "game-facing grab trajectory advances with absolute time");
    check(!poll(ActionExperiment::grabs, 660, {}, SkatePose::riding).owned && !input.busy(),
        "native landing cancels grab immediately");
    for (const auto mode : {ActionExperiment::revert_frontside, ActionExperiment::revert_backside}) {
        input.reset();
        poll(mode, 1000);
        out = poll(mode, 1010, {0, 0, 0, 0, 255});
        check(out.requested && out.result == DispatchResult::started && out.consume_rt && out.owned,
            "RT edge routes explicitly selected side candidate");
        check(out.pad.right_y == -32767 && out.pad.left_trigger == 0 && out.pad.right_trigger == 0,
            "native setup owns triggers without passing physical RT");
        out = poll(mode, 1040, {0, 0, 0, 0, 255});
        check(out.pad.owns_left_stick && (mode == ActionExperiment::revert_frontside ? out.pad.left_x > 32000 : out.pad.left_x < -32000),
            "side candidate starts on the distinct sourced side");
        poll(mode, 1050);
        out = poll(mode, 1060, {0, 0, 0, 0, 255});
        check(out.requested && out.result == DispatchResult::busy, "repeated RT cannot extend current playback");
        check(!poll(mode, 1245, {0, 0, 0, 0, 255}).owned, "retrigger did not extend 235 ms expiry");
        check(!poll(mode, 2000, {0, 0, 0, 0, 255}).requested, "held RT cannot restart after expiry");
        poll(mode, 2010);
        out = poll(mode, 2020, {0, 0, 0, 255, 255});
        check(out.pad.right_y == 32767, "nollie context reaches candidate independent of physical LT masking");
        check(!poll(mode, 2021, {}, SkatePose::riding, true, false).owned, "stale observer cancels both sticks");
        check(!poll(mode, 2100, {0, 0, 0, 0, 255}).requested, "held RT after stale gap must rearm");
    }
    for (unsigned conflict : {0x1000u, 0x4000u, 0x0100u, 0x0200u, 0x0010u}) {
        input.reset(); poll(ActionExperiment::revert_frontside, 3000);
        out = poll(ActionExperiment::revert_frontside, 3010,
            {static_cast<std::uint16_t>(conflict), 0, 0, 0, 255});
        check(!out.owned && out.result == DispatchResult::busy, "existing face/shoulder/menu owner prevents candidate");
    }
    input.reset(); poll(ActionExperiment::revert_frontside, 4000);
    out = poll(ActionExperiment::revert_frontside, 4010, {0, 0, 0, 0, 255}, SkatePose::air);
    check(out.result == DispatchResult::unsafe_context && !out.owned, "airborne RT cannot become ground revert");
    input.reset(); poll(ActionExperiment::grabs, 5000, {}, SkatePose::air);
    poll(ActionExperiment::grabs, 5010, {0x2004}, SkatePose::air);
    out = poll(ActionExperiment::grabs, 5190, {4}, SkatePose::air, true, true, false);
    check(out.result == DispatchResult::unsafe_context && !out.owned, "uninspected grab stance remains gated");
    input.reset(); poll(ActionExperiment::grabs, 6000, {}, SkatePose::air);
    poll(ActionExperiment::grabs, 6010, {0x2004}, SkatePose::air);
    poll(ActionExperiment::grabs, 6190, {4}, SkatePose::air);
    check(!poll(ActionExperiment::grabs, 6191, {}, SkatePose::air, true, true, true, true).owned,
        "pending authored/manual owner cancels active auxiliary playback");
    input.reset(); poll(ActionExperiment::revert_frontside, 7000);
    poll(ActionExperiment::revert_frontside, 7010, {0, 0, 0, 0, 255});
    check(!poll(ActionExperiment::revert_frontside, 7009).owned, "clock reversal clears ownership");
    PlaybackPacket packet;
    const auto first = packet.sample(10, 10, true);
    const auto second = packet.sample(10, 10, true);
    check(second == first + 1 && packet.sample(second, second, false) != second,
        "synthetic action packets and neutral restoration remain visible");
    input.reset();
    NativeComboObservation native{true, true, false, false, 100, 1, 0xc000};
    poll(ActionExperiment::revert_frontside, 8000, {}, observed_pose(native), true, action_input_eligible(native));
    out = poll(ActionExperiment::revert_frontside, 8010, {0, 0, 0, 0, 255}, observed_pose(native), true, action_input_eligible(native));
    check(out.owned, "native riding state admits revert setup");
    native.state = 103;
    out = poll(ActionExperiment::revert_frontside, 8040, {0, 0, 0, 0, 255}, observed_pose(native), true, action_input_eligible(native));
    check(out.owned && out.pad.left_x > 32000, "native setup crouch preserves revert rotation eligibility");
    native.bailed = true;
    check(!poll(ActionExperiment::revert_frontside, 8050, {}, observed_pose(native), true, action_input_eligible(native)).owned,
        "independent native bail cancels crouched revert");
    for (unsigned state : {0u, 310u, 400u}) {
        native.bailed = false; native.state = state;
        check(!action_input_eligible(native), "unknown/offboard/grind state cannot own auxiliary input");
    }
    for (const unsigned delay : {100u, 400u, 650u}) {
        input.reset();
        poll(default_action_mode, 0);
        poll(default_action_mode, 20, {}, SkatePose::air, true, true, true, true);
        const auto grab = poll(default_action_mode, delay, {0x2000}, SkatePose::air);
        check(grab.requested && grab.result == DispatchResult::started && grab.owned,
            "first fresh airborne B after neutral Ollie ownership dispatches without a second press or fixed wait");
        check(!poll(default_action_mode, delay + 17, {4}, SkatePose::riding).owned,
            "landing cancels airborne grab ownership immediately");
        check(!poll(default_action_mode, delay + 34, {0x2004}, SkatePose::riding).owned,
            "grounded B keeps native braking and cannot start a grab");
        check(!poll(default_action_mode, delay + 51, {0x2004}, SkatePose::air).requested,
            "grounded B cannot queue a stale grab for the next jump");
    }
    input.reset(); poll(default_action_mode, 0);
    check(!poll(default_action_mode, 10, {0x2000}).consume_buttons,
        "ordinary grounded B remains native brake");
    input.reset(); poll(default_action_mode, 0);
    const auto directional_basic = poll(default_action_mode, 20, {0x2004}, SkatePose::air);
    check(directional_basic.requested && directional_basic.owned &&
          directional_basic.result == DispatchResult::started && directional_basic.trick == ArcadeTrick::basic_bs_grab,
          "single directional B explicitly requests the verified basic grab rather than claiming an unproved Melon");
    for (const auto direction : {0u, 1u, 2u, 4u, 8u, 5u, 9u, 6u, 10u}) {
        input.reset(); poll(default_action_mode, 0);
        const auto grab = poll(default_action_mode, 20, {static_cast<std::uint16_t>(0x2000u | direction)}, SkatePose::air);
        check(grab.owned && grab.trick == ArcadeTrick::basic_bs_grab && grab.pad.right_trigger == 255,
              "every valid D-pad modifier keeps normal B available with its actual basic identity");
    }
    for (const auto axis : {-25000, 25000}) {
        input.reset(); poll(default_action_mode, 0);
        const auto grab = poll(default_action_mode, 20, {0x2000, 0, static_cast<short>(axis)}, SkatePose::air);
        check(grab.owned && grab.trick == ArcadeTrick::basic_bs_grab && !grab.pad.owns_left_stick,
              "forward or backward movement cannot suppress the basic grab or take away steering");
    }
    check(select_named_trick(TrickFamily::grab, TrickDirection::left, 1) == ArcadeTrick::melon,
          "named candidate catalog remains distinct from the production basic-grab control");
    input.reset(); poll(default_action_mode, 0);
    const auto basic = poll(default_action_mode, 20, {0x2000}, SkatePose::air);
    check(basic.trick == ArcadeTrick::basic_bs_grab && basic.pad.right_trigger == 255 &&
          basic.pad.right_x == 0 && basic.pad.right_y == 0,
          "neutral B requests the measured basic RT grab without an unproved modifier");
    check(poll(default_action_mode, 519, {0x2000}, SkatePose::air).pad.right_trigger == 255 &&
          poll(default_action_mode, 520, {0x2000}, SkatePose::air).pad.right_trigger == 0,
          "basic grab retains measured hold and explicit neutral release");
    check(!poll(default_action_mode, 554, {0x2000}, SkatePose::air).owned &&
          !poll(default_action_mode, 555, {0x2000}, SkatePose::air).requested,
          "completed grab releases ownership without replaying held B");
    poll(default_action_mode, 560, {}, SkatePose::air);
    check(poll(default_action_mode, 600, {0x2000}, SkatePose::air).owned,
          "fresh B rearms after completed grab while native air remains eligible");
    std::cout << checks << " action runtime checks passed\n";
    return 0;
}
#ifndef ARCADE_PLUMBING_COMBINED_TESTS
int main() { return run_arcade_action_runtime_tests(); }
#endif
