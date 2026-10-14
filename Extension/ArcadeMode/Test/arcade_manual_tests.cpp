#include "Extension/ArcadeMode/arcade_manual_input.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
using namespace dingosdk::arcade_mode;
ManualGesture::Output poll(ManualGesture& manual, std::uint64_t ms, short y = 0,
        SkatePose pose = SkatePose::riding, std::uint16_t buttons = 0, short x = 0,
        bool safe = true, bool conflict = false, bool gameplay = true) {
    return manual.poll(ms, buttons, x, y, gameplay, safe, pose, conflict);
}
void start(ManualGesture& manual, std::uint64_t ms = 0, bool nose = false) {
    poll(manual, ms);
    poll(manual, ms + 10, nose ? -25000 : 25000);
    const auto out = poll(manual, ms + 20, nose ? 25000 : -25000);
    check(out.owns_stick && !out.confirmed && out.y == (nose ? 20000 : -20000),
        "a fresh directional pair requests the exact partial native deflection");
}
}

int run_arcade_manual_tests() {
    for (const bool nose : {false, true}) {
        ManualGesture manual;
        start(manual, 0, nose);
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        check(poll(manual, 30, 0, pose).confirmed, "only matching native pose confirms the request");
        for (unsigned i = 0; i < 10; ++i) {
            const auto out = poll(manual, 40 + i * 10, i % 2 ? 25000 : -25000, pose);
            check(out.confirmed && out.y == (nose ? 20000 : -20000),
                "balance corrections cannot reverse an active native manual request");
        }
        check(poll(manual, 279, 0, SkatePose::unknown).owns_stick,
            "brief observer gap retains the existing partial hold");
        check(!poll(manual, 280, 0, SkatePose::unknown).owns_stick,
            "confirmed hold expires exactly150ms after its last matching observation");
        check(!poll(manual, 281, 25000).owns_stick && !poll(manual, 282, -25000).owns_stick,
            "expired hold requires a neutral rearm before another directional pair");
    }
    {
        ManualGesture manual;
        start(manual);
        check(poll(manual, 1219).owns_stick && !poll(manual, 1220).owns_stick,
            "unconfirmed request releases exactly1200ms after initiation");
        start(manual, 1300);
        check(poll(manual, 1330, 0, SkatePose::manual).confirmed, "fresh retry confirms");
        check(!poll(manual, 1331, 0, SkatePose::air).owns_stick,
            "observed takeoff immediately releases a confirmed manual");
    }
    for (auto pose : {SkatePose::manual, SkatePose::nose_manual, SkatePose::unknown,
                      SkatePose::bail, SkatePose::grind}) {
        ManualGesture manual;
        poll(manual, 0, 0, pose);
        poll(manual, 10, 25000, pose);
        check(!poll(manual, 20, -25000, pose).owns_stick,
            "correction or unknown incompatible pose cannot initiate a manual");
    }
    for (auto pose : {SkatePose::bail, SkatePose::grind, SkatePose::nose_manual}) {
        ManualGesture manual;
        start(manual);
        check(!poll(manual, 30, 0, pose).owns_stick,
            "independent incompatible native pose releases even if external eligibility lags");
    }
    for (std::uint16_t button : {0x1000u, 0x2000u, 0x4000u, 0x8000u, 0x0010u, 0x0020u, 0x0300u}) {
        ManualGesture manual;
        start(manual);
        check(!poll(manual, 30, 0, SkatePose::manual, button).owns_stick,
            "face menu and board commands cannot retain manual synthetic ownership");
    }
    {
        ManualGesture manual;
        poll(manual, 0);
        poll(manual, 10, 25000);
        poll(manual, 20, 0, SkatePose::riding, 0, 25000);
        check(!poll(manual, 30, -25000).owns_stick, "horizontal motion breaks a pending vertical sequence");
        manual.cancel(); poll(manual, 100);
        poll(manual, 110, 25000);
        poll(manual, 120, 0, SkatePose::riding, 3);
        check(!poll(manual, 130, -25000).owns_stick, "opposing D-pad directions break the sequence");
        manual.cancel(); poll(manual, 200);
        poll(manual, 210, 25000);
        check(!poll(manual, 561, -25000).owns_stick, "direction sequence expires after350ms");
        manual.cancel(); poll(manual, 600);
        poll(manual, 610, 25000);
        check(poll(manual, 960, -25000).owns_stick, "direction sequence accepts the350ms boundary");
        check(!poll(manual, 959).owns_stick, "backwards time cancels manual state");
    }
    {
        ManualGesture manual;
        poll(manual, 0, 0, SkatePose::air);
        poll(manual, 10, 0, SkatePose::air, 1);
        auto out = poll(manual, 20, 0, SkatePose::air, 2);
        check(out.owns_stick && !out.confirmed && out.y == -20000,
            "airborne D-pad command can prepare a manual without claiming it landed");
        check(poll(manual, 21, 0, SkatePose::air, 2).y == -20000,
            "repeated held direction does not create another command");
        check(poll(manual, 22, 0, SkatePose::manual).confirmed, "actual manual landing confirms airborne preparation");
        check(!poll(manual, 23, 0, SkatePose::manual, 0, 0, true, true).owns_stick,
            "external owner conflict immediately releases partial input");
        start(manual, 100);
        check(!poll(manual, 121, 0, SkatePose::manual, 0, 0, true, false, false).owns_stick,
            "gameplay loss cancels synthetic ownership");
        start(manual, 200);
        check(!poll(manual, 221, 0, SkatePose::manual, 0, 0, false).owns_stick,
            "native eligibility loss cancels synthetic ownership");
    }
    {
        ManualGesture manual;
        const auto base = std::numeric_limits<std::uint64_t>::max() - 1300;
        start(manual, base);
        check(!poll(manual, base + 1220).owns_stick, "near-limit timestamps use bounded differences without overflow");
    }
    std::cout << checks << " manual input checks passed\n";
    return 0;
}
#ifndef ARCADE_PLUMBING_COMBINED_TESTS
int main() { return run_arcade_manual_tests(); }
#endif
