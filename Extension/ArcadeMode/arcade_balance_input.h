#pragma once
#include "arcade_model.h"
#include "arcade_manual_physics.h"
#include "arcade_input_events.h"
#include <cstdint>

namespace dingosdk::arcade_mode {
// Pressed states of the mechanic's configured controls, already normalized by
// the input owner. A wins when both controls are held; neutral is not required.
struct ManualBalanceButtons { bool a{}, b{}; };
constexpr float manual_balance_correction(ManualBalanceButtons buttons) noexcept {
    return buttons.a ? -1.0f : buttons.b ? 1.0f : 0.0f;
}
// Coordinate conversion only, after the existing correction/ownership gates.
// Unknown or fractional values cannot become a new native correction request.
constexpr std::int16_t manual_balance_native_y(float effective_correction) noexcept {
    return effective_correction == -1.0f ? std::int16_t{32767} :
        effective_correction == 1.0f ? std::int16_t{-32768} : std::int16_t{0};
}
// Compatibility boundary delegates normalization to the existing controller
// rather than defining another axis threshold. Configured layouts should pass
// their already-normalized states to poll_manual directly.
inline ManualBalanceButtons manual_balance_buttons(short y, std::uint32_t buttons = 0, short x = 0) noexcept {
    if (buttons & 3u) return {(buttons & 1u) != 0, (buttons & 2u) != 0};
    InputEvents normalized;
    normalized.sample(0,{static_cast<std::uint16_t>(buttons & 0xffffu),x,y},false,false,InputPolicy::production());
    const auto direction = normalized.held_direction();
    return {direction >= 7, direction >= 1 && direction <= 3};
}
// Left stick: right increases rail lean; up reduces manual lean and down
// increases it. Manual and Nose share these controls, with distinct endpoints.
// Caller supplies the raw physical sample, never the synthetic game-facing pad.
inline float balance_correction(BalanceKind kind, short left_x, short left_y, bool available,
                                std::uint32_t buttons = 0) noexcept {
    if (!available || kind == BalanceKind::none) return 0;
    if (kind == BalanceKind::manual) return manual_balance_correction(manual_balance_buttons(left_y,buttons,left_x));
    constexpr float deadzone = 7849.0f;
    const float raw = static_cast<float>(kind == BalanceKind::grind ? left_x : left_y);
    const float magnitude = std::abs(raw);
    if (magnitude <= deadzone) return 0;
    const float scale = raw < 0 ? 32768.0f : 32767.0f;
    const auto value = std::copysign(std::min(1.0f, (magnitude - deadzone) / (scale - deadzone)), raw);
    return value;
}
class BalanceControl {
public:
    void reset_manual() noexcept { manual_ = released_ = false; control_ = {}; }
    // Native setup owns this clock independently of the physical-input lease.
    // Repeated admitted observations cannot renew an existing section.
    void begin_manual(std::uint64_t now) noexcept {
        if (!manual_) { manual_ = true; began_ = last_ = now; released_ = false; control_ = {}; }
    }
    float poll(std::uint64_t now, BalanceKind kind, float lean, short x, short y,
               bool available, std::uint32_t buttons = 0) noexcept {
        if (kind == BalanceKind::manual) return poll_manual(now,lean,manual_balance_buttons(y,buttons,x),available);
        control_ = {}; manual_ = released_ = false;
        return balance_correction(kind,x,y,available,buttons); // Preserve grind correction.
    }
    float poll_manual(std::uint64_t now, float lean, ManualBalanceButtons buttons, bool available) noexcept {
        control_ = {};
        if (!std::isfinite(lean) || (manual_ && now < last_)) {
            manual_ = released_ = false; return 0;
        }
        if (!available) {
            if (manual_) { last_ = now; control_ = {0,false,released_,now-began_}; }
            return 0;
        }
        const auto correction = manual_balance_correction(buttons);
        begin_manual(now);
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
