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
    for (bool pad : {false, true}) for (int sign : {-1, 1}) {
        for (auto elapsed : {0u, 1u, 17u, 999u, 1000u, 1001u}) {
            BalanceControl control;
            const auto button = static_cast<std::uint32_t>(pad ? (sign < 0 ? 1 : 2) : 0);
            const auto y = static_cast<short>(pad ? 0 : sign < 0 ? 32767 : -32768);
            check(control.poll(1000, BalanceKind::manual, static_cast<float>(sign) * .2f, 0, y, true, button) == 0,
                  "held entry is ignored at elapsed zero");
            const auto outward = control.poll(1000 + elapsed, BalanceKind::manual,
                static_cast<float>(sign) * .2f, 0, y, true, button);
            check(outward == (elapsed >= 1000 ? static_cast<float>(sign) : 0),
                  "outward correction is suppressed before1000ms and effective at its exact boundary");
            BalanceControl inward;
            inward.poll(1000, BalanceKind::manual, -static_cast<float>(sign) * .2f, 0, y, true, button);
            check(inward.poll(1000 + elapsed, BalanceKind::manual, -static_cast<float>(sign) * .2f,
                0, y, true, button) == (elapsed ? static_cast<float>(sign) : 0),
                  "held inward correction takes effect after elapsed time becomes positive, without requiring neutral");
        }
    }
    BalanceControl control;
    control.poll(1000, BalanceKind::manual, .2f, 0, 32767, true);
    check(control.poll(1001, BalanceKind::manual, .2f, 0, 32767, true) == -1,
          "Up corrects positive manual lean inward");
    check(control.poll(999, BalanceKind::manual, .2f, 0, 32767, true) == 0,
          "backwards time clears manual correction continuity");
    check(control.poll(1010, BalanceKind::manual, .2f, 0, 32767, true) == 0,
          "the next valid entry must establish a fresh baseline");
    check(control.poll(1011, BalanceKind::manual, .2f, 0, 32767, false) == 0,
          "unavailable controller produces no correction");
    check(control.poll(1012, BalanceKind::grind, 0, -32768, 0, true) == -1,
          "rail correction retains its original axis and immediate response");
    std::cout << checks << " timed balance correction checks passed\n";
}
