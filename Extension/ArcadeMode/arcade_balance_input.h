#pragma once
#include "arcade_model.h"

namespace dingosdk::arcade_mode {
// Left stick: right is positive for grind, up is positive for either manual.
// Nose manual initially shares this screen-space convention (no inversion).
// Caller supplies the raw physical sample, never the synthetic game-facing pad.
inline float balance_correction(BalanceKind kind, short left_x, short left_y, bool available,
                                std::uint32_t buttons = 0) noexcept {
    if (!available || kind == BalanceKind::none) return 0;
    if (kind == BalanceKind::manual && (buttons & 3u)) {
        return (buttons & 3u) == 1u ? 1.0f : (buttons & 3u) == 2u ? -1.0f : 0.0f;
    }
    constexpr float deadzone = 7849.0f;
    const float raw = static_cast<float>(kind == BalanceKind::grind ? left_x : left_y);
    const float magnitude = std::abs(raw);
    if (magnitude <= deadzone) return 0;
    const float scale = raw < 0 ? 32768.0f : 32767.0f;
    return std::copysign(std::min(1.0f, (magnitude - deadzone) / (scale - deadzone)), raw);
}
class BalanceControl {
public:
    float poll(BalanceKind kind, short x, short y, bool available, std::uint32_t buttons = 0) noexcept {
        const auto correction = balance_correction(kind, x, y, available, buttons);
        if (kind != BalanceKind::manual || !available) {
            manual_ = armed_ = false;
            return correction; // Preserve existing grind correction.
        }
        if (!manual_) {
            manual_ = true; entry_ = correction; armed_ = std::abs(correction) < .05f;
        } else if (std::abs(correction) < .05f || correction * entry_ < 0) {
            armed_ = true;
        }
        // Completing the directional command is not also a balance correction.
        // A release or a fresh opposite tap hands control to the player.
        return armed_ ? correction : 0;
    }
private:
    bool manual_{}, armed_{};
    float entry_{};
};
struct BalanceMeter {
    bool visible{}, vertical{};
    float cursor_x{}, cursor_y{}; // normalized offsets from center; screen Y points down
};
inline BalanceMeter balance_meter(const State& state) noexcept {
    if (!state.combo_active || state.balance_kind == BalanceKind::none) return {};
    const bool vertical = state.balance_kind == BalanceKind::manual;
    return {true, vertical, vertical ? 0.0f : state.balance, vertical ? -state.balance : 0.0f};
}
}
