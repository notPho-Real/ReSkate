#pragma once
#include "arcade_model.h"
#include <cstdint>
namespace dingosdk::arcade_mode {
void publish_balance_hud(const State& state, unsigned danger, std::uint64_t source_age_ms = 0) noexcept;
bool balance_hud_pending() noexcept;
void draw_balance_hud() noexcept;
}
