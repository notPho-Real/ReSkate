#include "Extension/ArcadeMode/arcade_combo_session.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
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
    for (bool nose : {false, true}) for (int sign : {-1, 1}) {
        ComboSession session;
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        session.step(pose, 0, 0);
        bool bailed{};
        for (unsigned step = 0; step < 2000 && session.model().state().balance_kind != BalanceKind::none; ++step)
            bailed |= session.step(pose, .017f, static_cast<float>(sign));
        const bool putdown = nose ? sign > 0 : sign < 0;
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
