#pragma once
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
// The supported native push code consumes request_words[0] bit0x40 (also read
// by trainer_push_speed). A native push lifts one wheel; that is permission to
// continue pushing while its independent request is present. The policy below
// separately bounds the measured wheel-recovery interval. Established
// balance, missing contact and every non-riding state remain exclusions.
constexpr bool native_push_allowed(bool valid, unsigned state, unsigned mode,
        unsigned contacts, bool native_pushing, bool balancing) noexcept {
    return valid && state == 100 && mode == 1 && !balancing &&
        (contacts == 0xc000u || (native_pushing && (contacts == 0x4000u || contacts == 0x8000u)));
}
// Native525–725ms push evidence is followed by about100ms of one-wheel
// recovery. Releasing X there turns a held push back into a tap. Only a fresh
// native push may bridge that contact; context/balance/owner resets revoke it.
class NativePushContact {
public:
    static constexpr std::uint64_t recovery_ms = 200;
    void reset() noexcept { *this = {}; }
    bool poll(std::uint64_t now, bool valid, unsigned state, unsigned mode,
              unsigned contacts, bool pushing, bool balancing) noexcept {
        if (!valid || state != 100 || mode != 1 || balancing ||
            (clocked_ && now < last_) ||
            (contacts != 0xc000u && contacts != 0x4000u && contacts != 0x8000u)) {
            reset(); return false;
        }
        clocked_ = true; last_ = now;
        if (pushing) { seen_ = true; pushed_at_ = now; }
        return native_push_allowed(valid, state, mode, contacts,
            pushing || (seen_ && now - pushed_at_ <= recovery_ms), balancing);
    }
private:
    std::uint64_t last_{}, pushed_at_{};
    bool clocked_{}, seen_{};
};
// Native held-button acceleration supplies its own push cadence. Short taps
// merely ask for the current speed clamped to a native minimum; repeating them
// does not reliably advance the held-push steps. Never write a speed target.
class AutomaticPush {
public:
    void reset() noexcept { *this = {}; }
    bool movement_intent() const noexcept { return armed_ && movement_; }
    bool poll(std::uint64_t now, std::int16_t lx, std::int16_t ly, float speed,
              bool gameplay, bool permitted) noexcept {
        if (!gameplay || (sampled_ && now < previous_time_)) { reset(); return false; }
        previous_time_ = now;
        sampled_ = true;
        const bool neutral = std::abs(static_cast<int>(lx)) < 9000 && std::abs(static_cast<int>(ly)) < 9000;
        // Release movement after a menu/toggle boundary before starting again.
        if (neutral) { armed_ = true; movement_ = false; return false; }
        movement_ = ly >= 13000 || (std::abs(static_cast<int>(lx)) >= 13000 && ly > -9000);
        if (!armed_ || !movement_) {
            return false;
        }
        // Release ownership during exclusions. Native held pushing owns its
        // acceleration steps and top speed; a lower software cruise cap cut
        // off the later steps and left forward input coasting for too long.
        if (!permitted || !std::isfinite(speed) || speed < 0) {
            return false;
        }
        return true;
    }
private:
    bool armed_{}, sampled_{}, movement_{};
    std::uint64_t previous_time_{};
};
}
