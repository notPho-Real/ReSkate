#include "Extension/ArcadeMode/arcade_action_dispatch.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
}
int main() {
    ActionContext air{true, true, true, false, true, false};
    for (auto held_ms : {1u, 500u, 534u, 1000u, 10000u}) {
        ActionDispatch dispatch;
        check(dispatch.request_grab(ArcadeTrick::basic_bs_grab, 100, air, true) == DispatchResult::started,
              "known trigger-only grab starts a held native actuator");
        const auto held = dispatch.sample(100 + held_ms, air);
        check(held.owns_triggers && held.right_trigger == 255 && dispatch.busy(),
              "physical hold determines duration, including beyond the former fixed recipe");
        dispatch.release_grab(100 + held_ms);
        dispatch.release_grab(110 + held_ms); // Repeated released samples cannot renew the neutral tail.
        const auto released = dispatch.sample(100 + held_ms, air);
        check(released.owns_triggers && !released.right_trigger,
              "release immediately publishes native neutral without inventing another action");
        check(dispatch.sample(133 + held_ms, air).owns_triggers,
              "existing bounded neutral tail remains active before its endpoint");
        check(!dispatch.sample(134 + held_ms, air).owns_triggers && !dispatch.busy(),
              "neutral completion releases ownership exactly once");
    }
    for (unsigned fault = 0; fault < 5; ++fault) {
        ActionDispatch dispatch; auto context = air;
        dispatch.request_grab(ArcadeTrick::basic_bs_grab, 100, context, true);
        dispatch.sample(200, context);
        if (fault == 0) context.airborne = false;
        if (fault == 1) context.gameplay_fresh = false;
        if (fault == 2) context.experimental_enabled = false;
        if (fault == 3) context.other_input_owner = true;
        const auto result = dispatch.sample(fault == 4 ? 199 : 217, context);
        check(!result.owns_triggers && !dispatch.busy(),
              "landing, stale input, OFF, other ownership and backwards time cancel held native output");
    }
    ActionDispatch dispatch;
    check(dispatch.request_grab(ArcadeTrick::melon, 100, air, true) == DispatchResult::unavailable,
          "unknown named hold behavior is not substituted by a generic trigger hold");
    std::cout << checks << " native grab hold checks passed\n";
}
