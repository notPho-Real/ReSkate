#pragma once
#include "arcade_model.h"
namespace dingosdk::arcade_mode {
void publish_balance_hud(const State& state, unsigned danger) noexcept;
void draw_balance_hud() noexcept;
}
