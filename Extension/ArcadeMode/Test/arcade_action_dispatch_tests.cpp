#include "Extension/ArcadeMode/arcade_action_dispatch.h"
#include "ReferenceInput/arcade_aux_controls.h"
#include <cstdlib>
#include <iostream>
namespace {
unsigned checks{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
}
int run_arcade_action_dispatch_tests() {
    using namespace dingosdk::arcade_mode;
    using namespace dingosdk::arcade_mode::baseline;
    ActionDispatch dispatch;
    ActionContext context{};
    check(dispatch.request_grab(ArcadeTrick::melon, 0, context) == DispatchResult::disabled, "default gate closed");
    context = {true, true, true, false, true, false};
    check(dispatch.request_grab(ArcadeTrick::indy, 0, context) == DispatchResult::unavailable, "missing exact method not substituted");
    context.inspected_grab_stance = false;
    check(dispatch.request_grab(ArcadeTrick::melon, 0, context) == DispatchResult::unsafe_context, "opposite stance unverified");
    context.inspected_grab_stance = true;
    check(dispatch.request_grab(ArcadeTrick::melon, 0, context) == DispatchResult::started, "candidate starts explicitly");
    auto out = dispatch.sample(0, context);
    check(out.owns_triggers && out.right_trigger == 255 && !out.owns_left_stick && out.right_y == 0, "trigger precedes melon stick; steering retained");
    check(dispatch.request_revert(RevertSide::frontside, {}, 1, context) == DispatchResult::busy, "no overlapping owners");
    check(dispatch.sample(50, context).right_y > 32000, "melon second phase");
    out = dispatch.sample(350, context);
    check(out.owns_triggers && out.right_trigger == 0 && out.right_y == 0, "explicit neutral release");
    check(!dispatch.sample(384, context).owns_right_stick && !dispatch.busy(), "bounded completion");
    check(dispatch.request_revert(RevertSide::frontside, {}, 400, context) == DispatchResult::unsafe_context, "air revert rejected");
    context.airborne = false; context.riding = true;
    check(dispatch.request_revert(RevertSide::frontside, {}, 400, context) == DispatchResult::started, "ground revert candidate");
    check(dispatch.sample(430, context).owns_left_stick, "revert owns native rotation");
    context.other_input_owner = true;
    check(!dispatch.sample(440, context).owns_left_stick && !dispatch.busy(), "conflict cancels ownership");
    context.other_input_owner = false;
    check(dispatch.request_grind(ArcadeTrick::nosegrind, RailRelationship::unknown, context) == DispatchResult::needs_geometry, "grind requires geometry");
    check(dispatch.request_grind(ArcadeTrick::nosegrind, RailRelationship::parallel, context) == DispatchResult::unavailable, "geometry alone is not native actuation");
    for (auto trick : {ArcadeTrick::melon, ArcadeTrick::nosegrab, ArcadeTrick::tailgrab, ArcadeTrick::japan,
            ArcadeTrick::method, ArcadeTrick::rocket_air, ArcadeTrick::stiffy, ArcadeTrick::benihana,
            ArcadeTrick::airwalk, ArcadeTrick::one_foot_tailgrab, ArcadeTrick::judo, ArcadeTrick::christ_air}) {
        NativeActionPlayback playback;
        const auto plan = grab_plan(trick, true);
        check(playback.start(plan, 100, true, true), "guide candidate starts");
        check(!playback.start(plan, 110, true, true), "retrigger does not extend candidate");
        check(playback.sample(101, true, true).owns_pad, "recipe active");
        check(playback.sample(100, true, true).cancelled && !playback.busy(), "clock rollback neutralizes");
        check(playback.start(plan, 200, true, true), "fresh restart");
        check(playback.sample(201, true, false).cancelled, "landing stops grab");
        check(playback.start(plan, 300, true, true), "restart for late poll");
        check(playback.sample(2000, true, true).finished && !playback.busy(), "late poll never extends lifetime");
    }
    AuxiliaryControls controls;
    controls.poll(0, 0x2004, 0, 0, true, false);
    controls.poll(20, 4, 0, 0, true, false);
    auto request = controls.poll(180, 4, 0, 0, true, false);
    check(request.ready && request.trick == ArcadeTrick::melon, "single directional B");
    controls.poll(300, 0x2004, 0, 0, true, false);
    controls.poll(320, 4, 0, 0, true, false);
    controls.poll(350, 0x2004, 0, 0, true, false);
    request = controls.poll(530, 4, 0, 0, true, false);
    check(request.ready && request.trick == ArcadeTrick::method, "double B uses exact table");
    check(!controls.poll(531, 4, 0, 0, true, false).ready, "request emits once");
    controls.poll(600, 0, 0, 0, true, false);
    controls.poll(620, 1, 0, 0, true, false);
    controls.poll(640, 0, 0, 0, true, false);
    controls.poll(660, 0x8001, 0, 0, true, false);
    request = controls.poll(840, 1, 0, 0, true, false, RailRelationship::parallel);
    check(request.ready && request.trick == ArcadeTrick::nosebluntslide, "double direction Y");
    controls.poll(900, 0, 0, 0, true, false);
    controls.poll(920, 0x8000, 0, 0, true, false);
    request = controls.poll(1100, 0, 0, 0, true, false);
    check(request.ready && request.rejected, "neutral grind cannot invent rail approach");
    check(controls.poll(1200, 0xa000, 0, 0, true, false).rejected, "simultaneous B/Y rejected");
    check(!controls.poll(1400, 0x2004, 0, 0, true, false).ready, "conflict requires release");
    controls.poll(1500, 0, 0, 0, true, false);
    controls.poll(1520, 0x2004, 0, 0, true, false);
    controls.poll(1540, 4, 0, 0, false, false);
    check(!controls.poll(1800, 0, 0, 0, true, false).ready, "menu cancels pending command");
    AuxiliaryControls late;
    late.poll(0, 0x2004, 0, 0, true, false);
    late.poll(20, 4, 0, 0, true, false);
    check(late.poll(250, 0x2004, 0, 0, true, false).trick == ArcadeTrick::melon,
        "late new edge flushes previous command");
    check(late.poll(430, 4, 0, 0, true, false).trick == ArcadeTrick::melon,
        "late new edge also starts a fresh command");
    AuxiliaryControls direction;
    direction.poll(0, 4, 0, 0, true, false);
    direction.poll(20, 0, 0, 0, true, false);
    direction.poll(40, 0x8004, 0, 0, true, false);
    check(direction.poll(220, 4, 0, 0, true, false).trick == ArcadeTrick::noseslide_or_tailslide,
        "repeated sideways steering does not create unsupported double direction");
    AuxiliaryControls consumed;
    consumed.poll(0, 1, 0, 0, true, false);
    consumed.poll(20, 0, 0, 0, true, false);
    consumed.poll(40, 0x8001, 0, 0, true, false);
    consumed.poll(60, 1, 0, 0, true, false);
    check(consumed.poll(220, 1, 0, 0, true, false).trick == ArcadeTrick::nosebluntslide, "double up selected");
    consumed.poll(240, 0x8001, 0, 0, true, false);
    check(consumed.poll(420, 1, 0, 0, true, false).trick == ArcadeTrick::nosegrind,
        "second Y cannot reuse consumed double direction");
    std::cout << checks << " action dispatch checks passed\n";
    return 0;
}
#ifndef ARCADE_PLUMBING_COMBINED_TESTS
int main() { return run_arcade_action_dispatch_tests(); }
#endif
