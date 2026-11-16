#include "Extension/ArcadeMode/arcade_combo_session.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_manual_acquisition.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
void physical_endpoint_directions() {
    for (bool nose : {false, true}) for (bool switched : {false, true}) for (bool fakie : {false, true})
        for (float direction : {-1.f, 1.f}) for (float dt : {1.f / 30, 1.f / 60, 1.f / 120}) {
        ManualPhysics physics({GameplayStats::standard, GameplayDifficulty::medium, switched});
        ComboSession session;
        NativeInputFrame frame{};
        frame.valid = true; frame.state = 100; frame.mode = 1;
        frame.contacts = nose ? 0x8000u : 0x4000u;
        frame.orientation = {true, fakie};
        const auto pose = manual_acquisition_pose(frame);
        check(physics.begin(pose == SkatePose::nose_manual), "verified wheel type begins its own physical balance");
        PhysicalBalanceSample sample;
        bool failure{};
        for (unsigned i = 0; i < 10000 && physics.active(); ++i) {
            sample = physics.sample(dt, PhysicalManualControl{direction, true, true, 1000});
            failure |= session.step(pose, dt, direction, false, &sample, false, true);
        }
        const bool unsafe = nose ? direction > 0 : direction < 0;
        check(physics.closed() && sample.position * direction > 0,
              "held correction reaches the selected physical endpoint");
        check(sample.exit == (unsafe ? BalanceExit::bail : BalanceExit::putdown),
              "regular backward and nose forward endpoints bail; lowering the raised end puts down");
        check(failure == unsafe && session.failed_section() == unsafe && session.manual_putdown() != unsafe,
              "physical result routes failure separately from putdown without fabricating native execution");
        check(physics.nose() == nose && frame.orientation.fakie == fakie &&
              frame.contacts == (nose ? 0x8000u : 0x4000u),
              "switch difficulty and fakie travel never change verified wheel or stance identity");
        for (unsigned duplicate = 0; duplicate < 3; ++duplicate)
            check(!session.step(pose, dt, direction, false, &sample, false, true),
                  "continuing contact cannot issue another endpoint failure");
    }
    for (bool nose : {false, true}) for (float direction : {-1.f, 1.f}) {
        ManualPhysics physics;
        physics.begin(!nose);
        physics.sample(1.f / 60, direction);
        const auto before = physics.snapshot();
        physics.transfer(nose);
        const auto after = physics.snapshot();
        check(after.position == before.position && after.velocity == before.velocity &&
              after.elapsed == before.elapsed && after.repeat_pressure == before.repeat_pressure,
              "verified successor changes endpoint type without resetting the retained balance");
        // Only this fixture places lean beyond an endpoint. Production
        // transfer retains the actual dynamics checked above.
        physics.adjust({1, 0, 0, direction * 5000, 0, 0});
        const auto terminal = physics.sample(1.f / 60, 0);
        const bool unsafe = nose ? terminal.position > 0 : terminal.position < 0;
        check(terminal.exit == (unsafe ? BalanceExit::bail : BalanceExit::putdown),
              "retained dynamics use the verified successor wheel type at termination");
    }
}
}
int main() {
    physical_endpoint_directions();
    for (bool nose : {false, true}) for (int sign : {-1, 1}) {
        ComboSession session;
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        session.step(pose, 0, 0);
        bool bailed{};
        for (unsigned step = 0; step < 2000 && session.model().state().balance_kind != BalanceKind::none; ++step)
            bailed |= session.step(pose, .017f, static_cast<float>(sign));
        const bool putdown = nose ? sign < 0 : sign > 0;
        check(session.model().state().balance_kind == BalanceKind::none,
              "both manual endpoint paths terminate active balance");
        check(session.manual_putdown() == putdown && session.failed_section() != putdown && bailed != putdown,
              "Manual and Nose swap safe putdown and physical bail endpoints");
        check(session.model().last_settlement().kind != SettlementKind::bail || !putdown,
              "controlled putdown does not create a synthetic bail settlement");
        const auto fatigue = session.model().state().manual_fatigue;
        for (unsigned sample = 0; sample < 10; ++sample) {
            session.step(pose, .017f, 0);
            check(session.model().state().balance_kind == BalanceKind::none,
                  "lingering physical contact cannot reopen a terminated balance section");
        }
        if (putdown) {
            check(!balance_meter(session.balance_display_state()).visible,
                  "putdown hides the completed meter instead of displaying a failed edge");
            check(fatigue > 0 && session.model().state().manual_fatigue == fatigue,
                  "stop alone preserves balance history until the established landing/reset lifecycle");
        } else {
            check(balance_meter(session.balance_display_state()).visible,
                  "protected physical bail retains its failed manual edge until contact ends");
        }
        session.step(SkatePose::riding, .017f, 0);
        check(!session.manual_putdown() && !session.failed_section(),
              "actual physical exit clears either endpoint latch");
        session.step(pose, .017f, 0);
        check(session.model().state().balance_kind == BalanceKind::manual,
              "a fresh independently admitted section may enter after physical exit");
        session.invalidate();
        check(!session.manual_putdown() && !session.failed_section(), "invalid context clears physical endpoint lifecycle");
    }
    Model rail;
    rail.begin_balance(BalanceKind::grind, .99f);
    for (unsigned i = 0; i < 2000 && rail.state().balance_kind != BalanceKind::none; ++i)
        rail.tick(.017f, 1, 1, BalanceEndpoints::negative_bail);
    check(rail.balance_exit() == BalanceExit::bail,
          "manual endpoint selection never turns rail failure into a putdown");
    rail.reset();
    check(rail.balance_exit() == BalanceExit::none, "reset cannot retain a stale endpoint notification");
    std::cout << checks << " manual physical endpoint checks passed\n";
}
