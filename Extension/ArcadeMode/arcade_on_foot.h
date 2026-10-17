#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// The supported game's on-foot jump is native X. Keep native recovery and all
// other on-foot bindings; only a fresh, grounded A edge acquires this button.
class OnFootJump {
public:
    static constexpr std::uint16_t a = 0x1000, x = 0x4000;
    void cancel() noexcept { armed_ = previous_a_ = owns_ = false; }
    bool busy() const noexcept { return owns_; }
    std::uint16_t poll(std::uint16_t buttons, bool safe, bool grounded) noexcept {
        if (!safe) { cancel(); return buttons; }
        const bool held = (buttons & a) != 0;
        const bool edge = held && !previous_a_;
        previous_a_ = held;
        if (!armed_) { if (!held) armed_ = true; return buttons; }
        if (!held) owns_ = false;
        else if (edge && grounded) owns_ = true;
        return owns_ ? static_cast<std::uint16_t>((buttons & ~a) | x) : buttons;
    }
private:
    bool armed_{}, previous_a_{}, owns_{};
};
}
