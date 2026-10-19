#pragma once
#include "arcade_gameplay_profile.h"
#include "arcade_model.h"
#include <cmath>

namespace dingosdk::arcade_mode {
struct PhysicalBalanceSample {
    bool valid{}, active{};
    float position{}, velocity{}, elapsed{}, repeat_pressure{};
    BalanceExit exit{BalanceExit::none};
};
// Physical balance state is independent of score, multipliers and input
// recognition. Callers admit native identity/contact before starting a section.
class ManualPhysics {
public:
    explicit ManualPhysics(GameplayProfile profile = {}, std::uint32_t seed = 1) noexcept
        : profile_(profile), random_(seed ? seed : 1) {}
    void reset() noexcept { lean_ = speed_ = time_ = cheese_ = section_ = 0; active_ = closed_ = false; }
    void stop() noexcept { active_ = closed_ = false; section_ = 0; }
    bool active() const noexcept { return active_; }
    bool closed() const noexcept { return closed_; }
    bool begin(bool nose) noexcept {
        if (closed_) return false;
        if (active_) return true; // Same physical balance does not reset history.
        const auto p = profile_.balance(PhysicalBalanceKind::manual);
        if (!(p.endpoint > 0 && p.cheese_frames > 0)) return false;
        if (speed_ != 0) speed_ = std::copysign(std::max(std::abs(speed_ * p.repeat_multiplier), p.repeat_minimum), speed_);
        else speed_ = nose ? -p.repeat_minimum : p.repeat_minimum;
        lean_ = lean_ * p.lean_repeat_multiplier + cheese_ * sign(lean_);
        if (std::abs(lean_) > p.endpoint) {
            lean_ = std::copysign(p.endpoint * .9f, speed_);
            speed_ = std::copysign(p.endpoint, speed_);
        }
        cheese_ = p.cheese; section_ = 0; nose_ = nose; active_ = true;
        return true;
    }
    PhysicalBalanceSample sample(float dt, float correction) noexcept {
        if (!std::isfinite(dt) || dt <= 0 || dt > .25f || !std::isfinite(correction)) {
            stop(); return {};
        }
        const auto p = profile_.balance(PhysicalBalanceKind::manual);
        if (!active_) return {true, false};
        const float frames = dt * 60;
        time_ += dt; section_ += dt;
        if (cheese_ > .01f) cheese_ = std::max(0.0f, cheese_ - p.cheese / p.cheese_frames * frames);
        const auto instability = p.instability_base + time_ * p.instability_rate;
        // Position integrates the previous velocity before this frame's input.
        lean_ += (lean_ * p.gravity + speed_) * instability * frames;
        if (correction != 0) speed_ += p.correction_acceleration * frames * std::clamp(correction, -1.0f, 1.0f);
        else if (std::abs(speed_) < p.minimum_speed) {
            const auto bound = static_cast<unsigned>(p.random_speed) | 1u;
            speed_ = static_cast<float>(next() % bound) * sign(speed_);
        } else speed_ += static_cast<float>(next() % 50u) / 100 * sign(speed_);
        BalanceExit outcome{};
        if (std::abs(lean_) > p.endpoint) {
            const bool safe = nose_ ? lean_ > 0 : lean_ < 0;
            outcome = safe ? BalanceExit::putdown : BalanceExit::bail;
            active_ = false; closed_ = true;
        }
        return {true, active_, std::clamp(lean_ / GameplayProfile::balance_display_scale, -1.0f, 1.0f),
            speed_ / GameplayProfile::balance_display_scale, time_, cheese_, outcome};
    }
private:
    static float sign(float value) noexcept { return value < 0 ? -1.0f : 1.0f; }
    std::uint32_t next() noexcept {
        random_ ^= random_ << 13; random_ ^= random_ >> 17; random_ ^= random_ << 5;
        return random_;
    }
    GameplayProfile profile_;
    std::uint32_t random_;
    float lean_{}, speed_{}, time_{}, cheese_{}, section_{};
    bool active_{}, closed_{}, nose_{};
};
}
