#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_balance_layout.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
bool near(float a, float b) { return std::abs(a - b) < 0.01f; }
}
int run_arcade_balance_layout_tests() {
    using namespace dingosdk::arcade_mode;
    for (const auto screen : {BalanceHudPoint{1280, 800}, BalanceHudPoint{1280, 720}, BalanceHudPoint{1920, 1080}}) {
        for (const auto kind : {BalanceKind::grind, BalanceKind::manual}) {
            const bool vertical = kind == BalanceKind::manual;
            const auto layout = balance_hud_layout(screen.x, screen.y, vertical);
            const float axis = vertical ? screen.y : screen.x;
            check(layout.half_length * 2 >= axis * 0.25f && layout.half_length * 2 <= axis * 0.35f,
                  "meter occupies 25 to 35 percent of its relevant screen dimension");
            check(layout.panel_min.x >= 0 && layout.panel_min.y >= 0 &&
                  layout.panel_max.x <= screen.x && layout.panel_max.y <= screen.y,
                  "meter card stays inside standard Deck and external displays");
            check(layout.track_half_width * 2 >= 16 && layout.cursor_radius * 2 >= 21,
                  "track and cursor remain prominent at normal display sizes");
            for (const float balance : {-0.8f, -0.4f, 0.0f, 0.4f, 0.8f}) {
                State state{};
                state.combo_active = true;
                state.balance_kind = kind;
                state.balance = balance;
                const auto meter = balance_meter(state);
                const auto cursor = layout.point(vertical ? meter.cursor_y : meter.cursor_x);
                const float radius = layout.cursor_radius + 4 * layout.scale;
                check(cursor.x - radius > layout.panel_min.x && cursor.x + radius < layout.panel_max.x &&
                      cursor.y - radius > layout.panel_min.y && cursor.y + radius < layout.panel_max.y,
                      "entire cursor stays within the card at requested capture positions");
                check(vertical ? near(cursor.x, layout.center.x) : near(cursor.y, layout.center.y),
                      "cursor stays on its published balance axis");
                check(vertical ? near((layout.center.y - cursor.y) / layout.half_length, balance) :
                                 near((cursor.x - layout.center.x) / layout.half_length, balance),
                      "layout preserves actual model position and manual up-positive convention");
            }
            const auto low = layout.point(-2), high = layout.point(2);
            check(vertical ? near(low.y, layout.center.y - layout.half_length) && near(high.y, layout.center.y + layout.half_length) :
                             near(low.x, layout.center.x - layout.half_length) && near(high.x, layout.center.x + layout.half_length),
                  "out-of-range failure positions remain at the visible limits");
            const auto invalid = layout.point(std::numeric_limits<float>::quiet_NaN());
            check(near(invalid.x, layout.center.x) && near(invalid.y, layout.center.y),
                  "invalid visual input cannot submit nonfinite vertices");
        }
    }
    check(balance_hud_layout(0, 800, false).scale == 0 && balance_hud_layout(1280, -1, true).scale == 0 &&
          balance_hud_layout(std::numeric_limits<float>::infinity(), 800, false).scale == 0,
          "invalid display dimensions suppress drawing");
    check(!balance_meter({}).visible, "idle gameplay has no meter");
    for(auto kind:{BalanceKind::manual,BalanceKind::grind})for(auto invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        State state{};state.combo_active=true;state.balance_kind=kind;state.balance=invalid;
        check(!balance_meter(state).visible,"unavailable balance position cannot appear as a centered valid meter");
        state.balance=0;state.balance_velocity=invalid;
        check(!balance_meter(state).visible,"unavailable physical balance velocity cannot masquerade as valid presentation");
    }
    std::cout << checks << " balance layout checks passed\n";
    return 0;
}
#ifndef ARCADE_BALANCE_LAYOUT_COMBINED_TESTS
int main() { return run_arcade_balance_layout_tests(); }
#endif
