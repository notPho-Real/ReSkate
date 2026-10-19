#pragma once
#include "arcade_model.h"
#include "arcade_manual_physics.h"

namespace dingosdk::arcade_mode {
// Left stick: right increases rail lean; up reduces manual lean and down
// increases it. Manual and Nose share these controls, with distinct endpoints.
// Caller supplies the raw physical sample, never the synthetic game-facing pad.
inline float balance_correction(BalanceKind kind, short left_x, short left_y, bool available,
                                std::uint32_t buttons = 0) noexcept {
    if (!available || kind == BalanceKind::none) return 0;
    if (kind == BalanceKind::manual && (buttons & 3u)) {
        return (buttons & 3u) == 1u ? -1.0f : (buttons & 3u) == 2u ? 1.0f : 0.0f;
    }
    constexpr float deadzone = 7849.0f;
    const float raw = static_cast<float>(kind == BalanceKind::grind ? left_x : left_y);
    const float magnitude = std::abs(raw);
    if (magnitude <= deadzone) return 0;
    const float scale = raw < 0 ? 32768.0f : 32767.0f;
    const auto value = std::copysign(std::min(1.0f, (magnitude - deadzone) / (scale - deadzone)), raw);
    return kind == BalanceKind::manual ? -value : value;
}
class BalanceControl {
public:
    void reset_manual() noexcept { manual_ = released_ = false; control_ = {}; }
    float poll(std::uint64_t now, BalanceKind kind, float lean, short x, short y,
               bool available, std::uint32_t buttons = 0) noexcept {
        control_ = {};
        const auto correction = balance_correction(kind, x, y, available, buttons);
        if (kind != BalanceKind::manual || !available) {
            manual_ = released_ = false;
            return correction; // Preserve existing grind correction.
        }
        if (!std::isfinite(lean) || (manual_ && now < last_)) {
            manual_ = released_ = false; return 0;
        }
        if (!manual_) { manual_ = true; began_ = now; released_ = false; }
        last_ = now;
        const auto elapsed = now - began_;
        if (correction == 0 || elapsed > 0) released_ = true;
        control_ = {correction, correction != 0, released_, elapsed};
        if (!released_) return 0;
        // The first-second outward-control suppression is a step, not a smooth
        // ramp. Inward correction remains effective after the entry instant.
        if (elapsed < 1000 && correction * lean > 0) return 0;
        return correction;
    }
    PhysicalManualControl manual_input() const noexcept { return control_; }
private:
    PhysicalManualControl control_{};
    bool manual_{}, released_{};
    std::uint64_t began_{}, last_{};
};
struct BalanceMeter {
    bool visible{}, vertical{};
    float cursor_x{}, cursor_y{}; // normalized offsets from center; screen Y points down
};
inline BalanceMeter balance_meter(const State& state) noexcept {
    if (!state.combo_active || state.balance_kind == BalanceKind::none ||
        !std::isfinite(state.balance) || !std::isfinite(state.balance_velocity)) return {};
    const bool vertical = state.balance_kind == BalanceKind::manual;
    return {true, vertical, vertical ? 0.0f : state.balance, vertical ? -state.balance : 0.0f};
}
}
