#include "Extension/ArcadeMode/arcade_model.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void check(bool passed, const char* description) {
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}
int main() {
    using namespace dingosdk::arcade_mode;
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
    check(combo.state().fatigue == linked_fatigue, "manual entry preserves fatigue");
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
        check(combo.tick(0.1f, 0.0f, sign) && !combo.state().combo_active, "both balance limits bail");
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
    combo.tick(100.0f);
    check(combo.state().combo_active && combo.state().link_grace_s == 0.0f &&
        combo.state().combo_age_s < 2.0f, "time bounded; expired grace does not silently settle score");
    Tuning invalid;
    invalid.fatigue_per_second = std::numeric_limits<float>::quiet_NaN();
    invalid.balance_drift_rate = -2.0f;
    Model safe(invalid);
    safe.trick(1);
    safe.begin_balance(BalanceKind::manual);
    safe.tick(1.0f);
    check(safe.state().fatigue == 0.0f && safe.state().balance == 0.0f, "unsafe tuning sanitized");
    for (int i = 0; i < 1000; ++i) safe.link();
    check(safe.state().fatigue == 10.0f, "fatigue bounded");
    std::cout << "Arcade Mode model tests passed\n";
}
