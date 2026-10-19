#pragma once
#include "arcade_model.h"
#include "arcade_balance_hud_lease.h"
namespace dingosdk::arcade_mode {
// Source is the immutable physical sample's GetTickCount64 timestamp. An invalid
// source clears the display. Publication does not establish new native evidence.
void publish_balance_hud(const State& state, unsigned danger, BalanceHudSource source) noexcept;
bool balance_hud_pending() noexcept;
void draw_balance_hud() noexcept;
}
