#include "Extension/ArcadeMode/arcade_model.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_observation.h"
#include "Extension/ArcadeMode/arcade_native_probe.h"
#include "Extension/ArcadeMode/arcade_ollie.h"
#include "Extension/ArcadeMode/arcade_flip.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
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
int main() {
    using namespace dingosdk::arcade_mode;
    check(balance_correction(BalanceKind::grind, -32768, 32767, true) == -1, "left corrects grind left");
    check(balance_correction(BalanceKind::grind, 32767, -32768, true) == 1, "right corrects grind right");
    check(balance_correction(BalanceKind::manual, -32768, 32767, true) == 1, "up corrects manual up");
    check(balance_correction(BalanceKind::manual, 32767, -32768, true) == -1, "down corrects manual down");
    check(balance_correction(BalanceKind::manual, 32767, 32767, false) == 0, "unavailable input is neutral");
    check(balance_correction(BalanceKind::grind, 7849, 32767, true) == 0, "balance deadzone suppresses idle noise");
    {
        Model m;
        check(!balance_meter(m.state()).visible, "idle meter hidden");
        m.begin_balance(BalanceKind::grind, 0.5f);
        auto view = balance_meter(m.state());
        check(view.visible && !view.vertical && view.cursor_x == 0.5f && view.cursor_y == 0, "grind meter horizontal and model-driven");
        m.begin_balance(BalanceKind::manual, 0.5f);
        view = balance_meter(m.state());
        check(view.visible && view.vertical && view.cursor_x == 0 && view.cursor_y == -0.5f, "manual meter vertical and up-positive");
        m.bail();
        check(!balance_meter(m.state()).visible, "bail hides meter");
    }
    for (const auto direction : {std::uint16_t{4}, std::uint16_t{8}}) {
        NativeGesture gesture;
        const auto expected = direction == 4 ? NativeTrick::kickflip : NativeTrick::heelflip;
        for (unsigned trial = 0; trial < 20; ++trial) {
            const auto t = 3000ull * trial;
            auto out = gesture.poll(t, NativeGesture::x | direction, 0, 0, 0, 0, true, true);
            check(out.started == expected && !(out.buttons & NativeGesture::x), "X direction starts once and consumes X");
            out = gesture.poll(t + 1000, NativeGesture::x | direction, 0, 0, 0, 0, true, false);
            check(out.phase == 1 && out.right_y == -32768 && out.started == NativeTrick::none, "gesture continues across native crouch state");
            out = gesture.poll(t + 1600, NativeGesture::x | direction, 0, 0, 0, 0, true, false);
            check(out.phase == 2 && out.right_y == 28000 && out.right_x == (direction == 4 ? 28000 : -28000), "flip polarity matches discovered native gesture");
            out = gesture.poll(t + 1850, NativeGesture::x, 0, 0, 0, 0, true, true);
            check(out.phase == 0 && out.right_x == 0 && out.right_y == 0, "flip ends neutral at boundary");
            check(gesture.poll(t + 2200, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "long hold never repeats");
            gesture.poll(t + 2500, 0, 0, 0, 0, 0, true, true);
        }
    }
    {
        NativeGesture gesture;
        check(gesture.poll(0, NativeGesture::x, 0, 0, 0, 0, true, false).started == NativeTrick::none, "air/invalid state refuses new flip");
        check(gesture.poll(1, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "refused edge is not queued");
        gesture.poll(2, 0, 0, 0, 0, 0, true, true);
        check(gesture.poll(3, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::kickflip, "neutral X selects kickflip");
        auto out = gesture.poll(1003, NativeGesture::x, 0, 0, 9000, -10000, true, true);
        check(out.phase == 0 && out.right_x == 9000 && out.right_y == -10000, "physical right stick cancels without modification");
        check(gesture.poll(1603, NativeGesture::x, 0, 0, 0, 0, true, true).phase == 0, "cancelled hold cannot restart");
        gesture.poll(2000, 0, 0, 0, 0, 0, false, false);
        check(gesture.poll(2001, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "menu return requires release");
        gesture.poll(2002, 0, 0, 0, 0, 0, true, true);
        check(gesture.poll(2003, NativeGesture::x | 1, 0, 0, 0, 0, true, true).started == NativeTrick::none, "unmeasured impossible is not faked");
    }
    {
        NativeGesture shared;
        OllieGesture old;
        for (const auto now : {0ull, 100ull, 999ull, 1000ull, 1599ull, 1600ull, 1749ull, 1750ull, 3000ull}) {
            const auto buttons = static_cast<std::uint16_t>(now < 100 ? 0x1000 : 0);
            const auto before = old.poll(now, buttons, 0, true, true);
            const auto after = shared.poll(now, buttons, 0, 0, 0, 0, true, true);
            check(before.right_y == after.right_y && before.buttons == after.buttons && before.phase == after.phase,
                  "shared A profile preserves measured ollie boundaries");
        }
    }
    check(flip_direction(0x0004, 0, 0) == FlipTrick::kickflip, "D-pad left selects kickflip");
    check(flip_direction(0x0008, 0, 0) == FlipTrick::heelflip, "D-pad right selects heelflip");
    check(flip_direction(0x0001, 0, 0) == FlipTrick::impossible, "D-pad up selects impossible");
    check(flip_direction(0x0002, 0, 0) == FlipTrick::pop_shove_it, "D-pad down selects pop shove-it");
    check(flip_direction(0x0005, 0, 0) == FlipTrick::hardflip, "D-pad top-left selects hardflip");
    check(flip_direction(0x0009, 0, 0) == FlipTrick::inward_heelflip, "D-pad top-right selects inward heelflip");
    check(flip_direction(0x0006, 0, 0) == FlipTrick::varial_kickflip, "D-pad bottom-left selects varial kickflip");
    check(flip_direction(0x000a, 0, 0) == FlipTrick::varial_heelflip, "D-pad bottom-right selects varial heelflip");
    check(flip_direction(0, -32768, 0) == FlipTrick::kickflip, "left stick left selects kickflip");
    check(flip_direction(0, 32767, 0) == FlipTrick::heelflip, "left stick right selects heelflip");
    check(flip_direction(0, 0, 32767) == FlipTrick::impossible, "left stick up selects impossible");
    check(flip_direction(0, 0, -32768) == FlipTrick::pop_shove_it, "left stick down selects pop shove-it");
    check(flip_direction(0, -25000, 25000) == FlipTrick::hardflip, "left stick top-left selects hardflip");
    check(flip_direction(0, 25000, 25000) == FlipTrick::inward_heelflip, "left stick top-right selects inward heelflip");
    check(flip_direction(0, -25000, -25000) == FlipTrick::varial_kickflip, "left stick bottom-left selects varial kickflip");
    check(flip_direction(0, 25000, -25000) == FlipTrick::varial_heelflip, "left stick bottom-right selects varial heelflip");
    check(flip_direction(0, 12000, 12000) == FlipTrick::none, "stick deadzone remains unselected");
    check(flip_direction(0x000c, 0, 0) == FlipTrick::none, "opposing D-pad directions are rejected");
    check(flip_direction(0x0004, 32767, 0) == FlipTrick::kickflip, "D-pad overrides steering stick");
    OllieGesture ollie;
    auto frame = ollie.poll(0, 0, 0, true, true);
    check(frame.buttons == 0 && frame.phase == 0, "idle leaves native controller alone");
    frame = ollie.poll(10, OllieGesture::a_button, 0, true, false);
    check(frame.buttons == OllieGesture::a_button && !frame.started,
        "A stays native when off board");
    frame = ollie.poll(20, 0, 0, true, true);
    frame = ollie.poll(30, OllieGesture::a_button, 0, true, true);
    check(frame.started && frame.buttons == 0 && frame.phase == 0,
        "on-board A edge starts one gesture and suppresses native A");
    frame = ollie.poll(1029, OllieGesture::a_button, 0, true, false);
    check(!frame.started && frame.right_y == 0 && frame.buttons == 0,
        "holding A neither repeats nor aborts when board state changes");
    frame = ollie.poll(1030, OllieGesture::a_button, 0, true, false);
    check(frame.phase == 1 && frame.right_y == -32768, "native crouch phase begins");
    frame = ollie.poll(1630, 0, 0, true, false);
    check(frame.phase == 2 && frame.right_y == 32767, "native release phase begins");
    frame = ollie.poll(1780, 0, 0, true, false);
    check(frame.phase == 0 && frame.right_y == 0, "gesture always returns stick to neutral");
    frame = ollie.poll(1900, OllieGesture::a_button, 15000, true, true, true);
    check(!frame.started && frame.buttons == OllieGesture::a_button && frame.right_y == 15000,
        "physical right-stick control takes precedence");
    frame = ollie.poll(1910, 0, 0, true, true);
    frame = ollie.poll(1920, OllieGesture::a_button, 0, true, true);
    frame = ollie.poll(2920, 0, 15000, true, false, true);
    check(frame.phase == 0 && frame.right_y == 15000, "player stick cancels synthetic gesture");
    frame = ollie.poll(2930, OllieGesture::a_button, 0, false, false);
    check(frame.buttons == OllieGesture::a_button && frame.phase == 0,
        "menu input capture cancels synthetic gesture");
    frame = ollie.poll(2940, 0, 0, true, true);
    frame = ollie.poll(2950, OllieGesture::a_button, 0, true, true);
    frame = ollie.poll(2949, OllieGesture::a_button, 0, true, false);
    check(frame.phase == 0, "backwards time cancels synthetic gesture");
    NativeRequestCapture capture;
    NativeRequestSample native{true, 100, 0, {1,2,3,4,5,6,7,8}};
    check(!capture.sample(100, native), "native capture requires explicit arm");
    capture.arm(100);
    check(capture.sample(100, native) && capture.count() == 1, "capture includes first sample");
    check(!capture.sample(110, native), "unchanged native request suppressed");
    native.words[7] ^= 1;
    check(capture.sample(120, native), "request change recorded without state change");
    native.valid = false;
    check(capture.sample(130, native) && capture.last() == NativeRequestSample{},
        "ownership loss clears all native fields");
    check(!capture.sample(140, {}), "repeated unavailable capture suppressed");
    native.valid = true;
    check(capture.sample(150, native), "recovered owner creates fresh sample");
    check(!capture.sample(5100, native) && !capture.active(), "five-second capture expires");
    capture.arm(100);
    check(!capture.sample(99, native) && !capture.active(), "backwards clock cancels capture");
    capture.arm(100);
    for (unsigned i = 0; i < NativeRequestCapture::limit; ++i) {
        native.state = i;
        check(capture.sample(100+i, native), "changing samples accepted up to limit");
    }
    ++native.state;
    check(!capture.sample(300, native) && capture.count() == NativeRequestCapture::limit &&
        !capture.active(), "native trace cannot exceed sample limit");
    capture.arm(1000);
    check(capture.sample(1000, native) && capture.count() == 1, "explicit rearm resets capture");
    ObservationTracker observer;
    check(!observer.update({false, 200, 1, 0}), "invalid watch cannot publish stale state");
    check(!observer.update({true, 0, 1, 0}), "uninitialized selector is unavailable");
    check(observer.update({true, 100, 1, 0}), "first native sample recorded");
    check(!observer.update({true, 100, 1, 0}), "unchanged sample does not spam logs");
    check(observer.update({true, 100, 3, 0}), "counter exposes intervening missed transitions");
    check(observer.update({true, 100, 3, 1}), "wipeout counter change retained");
    check(observer.update({false, 100, 3, 1}) && !observer.state().valid,
        "expired ownership discards stale sample");
    check(!observer.update({false, 300, 4, 2}), "unavailable samples stay quiet");
    check(observer.update({true, 200, 4, 2}), "watch recovery starts a new sample");
    Model combo;
    combo.trick(100);
    check(combo.state().combo_active && combo.state().multiplier == 1, "first trick starts combo");
    combo.trick(200);
    check(combo.state().raw_points == 300 && combo.state().multiplier == 2, "tricks accumulate");
    check(combo.bank() == 600 && !combo.state().combo_active && combo.state().fatigue == 0.0f, "bank and reset");
    check(combo.bank() == 0, "empty bank");
    combo.link();
    check(combo.state().fatigue == 0.0f, "idle link does nothing");

    combo.trick(100);
    combo.begin_balance(BalanceKind::grind);
    combo.tick(0.25f);
    const auto fatigue = combo.state().fatigue;
    combo.end_balance();
    check(combo.state().fatigue == fatigue, "balance exit preserves fatigue");
    combo.link();
    check(std::abs(combo.state().fatigue - fatigue - 0.12f) < 0.00001f, "link adds fatigue");
    const auto linked_fatigue = combo.state().fatigue;
    combo.begin_balance(BalanceKind::manual);
    check(combo.state().fatigue >= linked_fatigue, "manual entry preserves accumulated fatigue");
    Model fresh;
    fresh.trick(100);
    fresh.begin_balance(BalanceKind::manual);
    combo.tick(0.25f);
    fresh.tick(0.25f);
    check(combo.state().balance > fresh.state().balance, "tired combo drifts farther");
    combo.bail();
    check(!combo.state().combo_active && combo.state().raw_points == 0 &&
        combo.state().multiplier == 0 && combo.state().balance_kind == BalanceKind::none &&
        combo.state().fatigue == 0.0f, "bail clears combo");

    for (float sign : {-1.0f, 1.0f}) {
        combo.trick(1);
        combo.begin_balance(BalanceKind::grind, sign * 0.99f);
        check(combo.tick(1.0f, 0.0f, sign) && !combo.state().combo_active, "both balance limits bail");
    }
    combo.trick(1);
    combo.begin_balance(BalanceKind::manual, 1.0f);
    check(!combo.state().combo_active, "initial balance at limit fails");
    combo.trick(1);
    combo.begin_balance(BalanceKind::manual, 0.5f);
    combo.tick(0.1f, -1.0f, 0.0f);
    check(combo.state().balance < 0.5f, "signed correction restores balance");
    combo.tick(-1.0f);
    combo.tick(std::numeric_limits<float>::infinity());
    check(std::abs(combo.state().combo_age_s - 0.1f) < 0.00001f, "invalid time ignored");
    combo.tick(0.1f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity());
    check(std::isfinite(combo.state().balance), "nonfinite controls sanitized");
    combo.end_balance();
    combo.landing();
    combo.tick(100.0f);
    check(!combo.state().combo_active && combo.take_banked_points() == 1,
        "expired landing grace banks score exactly once");
    check(combo.take_banked_points() == 0, "bank notification consumed once");
    Tuning invalid;
    invalid.fatigue_per_second = std::numeric_limits<float>::quiet_NaN();
    invalid.balance_drift_rate = -2.0f;
    Model safe(invalid);
    safe.trick(1);
    safe.begin_balance(BalanceKind::manual);
    safe.tick(1.0f);
    check(safe.state().fatigue == 0.12f && safe.state().balance == 0.0f, "unsafe tuning sanitized; valid link fatigue retained");
    for (int i = 0; i < 1000; ++i) safe.link();
    check(safe.state().fatigue == 1.0f, "fatigue bounded");
    for (bool continuation : {false, true}) {
        Model m;
        m.trick(100);
        m.tick(1);
        check(m.state().combo_active, "airborne combo has no landing timeout");
        check(!m.revert(), "early revert rejected");
        m.landing();
        m.tick(0.3f);
        check(m.revert(), "revert in landing grace accepted");
        const auto points = m.state().raw_points;
        const auto multiplier = m.state().multiplier;
        for (int i = 0; i < 20; ++i) {
            check(!m.revert(), "repeat revert cannot refresh grace");
            m.tick(0.01f);
        }
        check(m.state().raw_points == points && m.state().multiplier == multiplier, "revert does not farm score");
        if (continuation) m.begin_balance(BalanceKind::manual);
        m.tick(0.3f);
        check(m.state().combo_active == continuation, "only actual continuation consumes pending revert");
        if (!continuation) check(m.take_banked_points() == 100 && !m.revert(), "expired revert banks and late revert rejected");
    }
    for (auto kind : {BalanceKind::grind, BalanceKind::manual}) {
        Model m;
        m.trick(10);
        m.begin_balance(kind);
        const auto first = m.difficulty();
        m.tick(1, 0, 0);
        m.tick(1, 0, 0);
        const auto mode_fatigue = kind == BalanceKind::grind ? m.state().grind_fatigue : m.state().manual_fatigue;
        m.trick(10);
        m.begin_balance(kind == BalanceKind::grind ? BalanceKind::manual : BalanceKind::grind);
        m.tick(1, 0, 0);
        m.trick(10);
        m.begin_balance(kind);
        check(m.difficulty() > first, "second balance section harder across other mode");
        check((kind == BalanceKind::grind ? m.state().grind_fatigue : m.state().manual_fatigue) == mode_fatigue,
              "mode fatigue persists through other mode");
        const auto count = m.state().multiplier;
        for (int i = 0; i < 100; ++i) m.begin_balance(kind);
        check(m.state().multiplier == count, "continuous balance entry cannot farm multiplier");
        m.bank();
        check(m.state().fatigue == 0 && m.state().grind_fatigue == 0 && m.state().manual_fatigue == 0,
              "bank resets all fatigue");
    }
    float positions[3]{};
    unsigned index = 0;
    for (int hz : {30, 60, 120}) {
        Model m({}, 42), identical({}, 42);
        m.trick(1); identical.trick(1);
        m.begin_balance(BalanceKind::manual); identical.begin_balance(BalanceKind::manual);
        for (int i = 0; i < hz * 3; ++i) {
            m.tick(1.0f / static_cast<float>(hz), -0.05f);
            identical.tick(1.0f / static_cast<float>(hz), -0.05f);
        }
        check(m.state().combo_active && m.state().balance == identical.state().balance, "same seed reproduces exact run");
        positions[index++] = m.state().balance;
    }
    check(std::abs(positions[0] - positions[1]) < 0.00001f && std::abs(positions[1] - positions[2]) < 0.00001f,
          "30/60/120 Hz materially equivalent");
    {
        Model m;
        for (int i = 0; i < 100000; ++i) m.trick(std::numeric_limits<std::uint32_t>::max());
        check(m.bank() == std::numeric_limits<std::uint64_t>::max(), "score product saturates without overflow");
        m.begin_balance(BalanceKind::grind);
        check(m.state().combo_active, "observed fresh grind starts a combo");
        m.tick(0.2f, 1);
        const auto x = m.state().balance;
        m.tick(0.02f, 0, 0);
        check(m.state().balance > x, "velocity persists after correction releases");
        m.bail();
        check(m.state().fatigue == 0 && m.state().grind_fatigue == 0 && m.state().manual_fatigue == 0 &&
              !m.state().revert_pending && m.take_banked_points() == 0, "bail resets every fatigue and loses score");
    }
    {
        Model fresh_model, tired_model;
        fresh_model.trick(1); tired_model.trick(1);
        for (int i = 0; i < 20; ++i) tired_model.link();
        fresh_model.begin_balance(BalanceKind::grind, 0.5f);
        tired_model.begin_balance(BalanceKind::grind, 0.5f);
        fresh_model.tick(0.25f, -1, 0); tired_model.tick(0.25f, -1, 0);
        check(fresh_model.state().balance < tired_model.state().balance, "fatigue reduces recovery authority");
        Model neutral;
        neutral.begin_balance(BalanceKind::manual);
        bool failed = false;
        for (int i = 0; i < 3600 && !failed; ++i) failed = neutral.tick(1.0f / 60.0f);
        check(failed && !neutral.state().combo_active, "uncorrected balance eventually fails deterministically");
    }
    {
        Model m;
        std::uint32_t sequence = 12345;
        for (unsigned i = 0; i < 10000; ++i) {
            sequence = sequence * 1664525u + 1013904223u;
            switch ((sequence >> 16u) % 8u) {
            case 0: m.trick(sequence); break;
            case 1: m.landing(); break;
            case 2: m.revert(); break;
            case 3: m.begin_balance(BalanceKind::grind); break;
            case 4: m.begin_balance(BalanceKind::manual); break;
            case 5: m.end_balance(); break;
            case 6: m.bail(); break;
            default: m.bank(); break;
            }
            const float dt = i % 17 == 0 ? std::numeric_limits<float>::quiet_NaN() : 1.0f / 30.0f;
            m.tick(dt, sequence & 256u ? 1.0f : -1.0f);
            const auto& s = m.state();
            check(std::isfinite(s.balance) && std::isfinite(s.balance_velocity) && std::abs(s.balance) < 1,
                  "adversarial transitions preserve finite bounded motion");
            check(s.fatigue >= 0 && s.fatigue <= 1 && s.grind_fatigue >= 0 && s.grind_fatigue <= 1 &&
                  s.manual_fatigue >= 0 && s.manual_fatigue <= 1, "adversarial transitions preserve fatigue bounds");
            check(s.combo_active || (s.raw_points == 0 && s.multiplier == 0 && !s.revert_pending &&
                  s.balance_kind == BalanceKind::none), "inactive state never retains a combo or HUD");
        }
    }
    std::cout << "Arcade Mode tests: " << checks << " checks passed, 0 failed\n";
}
