#include "Extension/ArcadeMode/arcade_action_runtime.h"
#include "Extension/ArcadeMode/arcade_playback_packet.h"
#include "Extension/ArcadeMode/arcade_native_events.h"
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
    std::cout << checks << " action runtime checks passed\n";
    return 0;
}
#ifndef ARCADE_PLUMBING_COMBINED_TESTS
int main() { return run_arcade_action_runtime_tests(); }
#endif
