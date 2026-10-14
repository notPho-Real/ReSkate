#pragma once
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Native button cadence only. Speed is an observation, never a velocity target.
class AutomaticPush {
public:
    static constexpr float resume_speed = 4.0f, cruise_speed = 6.0f;
    // Keep room between ordinary pushes. cat pouch
    static constexpr std::uint64_t pulse_ms = 180, cadence_ms = 850;
    void reset() noexcept { *this = {}; }
    bool poll(std::uint64_t now, std::int16_t lx, std::int16_t ly, float speed,
              bool gameplay, bool permitted) noexcept {
        if (!gameplay || (sampled_ && now < previous_time_)) { reset(); return false; }
        previous_time_ = now;
        sampled_ = true;
        const bool neutral = std::abs(static_cast<int>(lx)) < 9000 && std::abs(static_cast<int>(ly)) < 9000;
        // Release movement after a menu/toggle boundary before starting again.
        if (neutral) { armed_ = true; stop(); return false; }
        const bool movement = ly >= 13000 || (std::abs(static_cast<int>(lx)) >= 13000 && ly > -9000);
        if (!armed_ || !movement || !permitted || !std::isfinite(speed) || speed < 0 || speed >= cruise_speed) {
            stop(); return false;
        }
        if (!accelerating_ && speed >= resume_speed) return false;
        accelerating_ = true;
        if (pulsing_ && now - began_ >= pulse_ms) pulsing_ = false;
        if (!pulsing_ && (!has_pushed_ || now - began_ >= cadence_ms)) {
            began_ = now;
            pulsing_ = has_pushed_ = true;
        }
        return pulsing_;
    }
private:
    void stop() noexcept { accelerating_ = pulsing_ = false; }
    bool armed_{}, accelerating_{}, pulsing_{}, has_pushed_{}, sampled_{};
    std::uint64_t began_{}, previous_time_{};
};
}
