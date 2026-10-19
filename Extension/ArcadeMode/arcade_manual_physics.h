#pragma once
#include "arcade_gameplay_profile.h"
#include "arcade_model.h"
#include <cmath>
#include <initializer_list>

namespace dingosdk::arcade_mode {
struct ManualBalanceAdjustment {
    float time_multiplier{1}, time_add_seconds{};
    float lean_multiplier{1}, lean_add_signed{};
    float speed_multiplier{1}, speed_add_signed{};
};
struct PhysicalManualControl {
    float amount{};
    bool held{}, released{};
    std::uint64_t elapsed_ms{1000};
};
struct PhysicalBalanceSample {
    bool valid{}, active{};
    float position{}, velocity{}, elapsed{}, repeat_pressure{};
    BalanceExit exit{BalanceExit::none};
    float applied_correction{}; // Effective digital input after this step's protection.
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
    bool nose() const noexcept { return nose_; }
    // The caller must already own a qualified native transition. This changes
    // retained dynamics only: no type, setup, section clock or history reset.
    bool adjust(const ManualBalanceAdjustment& adjustment) noexcept {
        if (!active_ || closed_) return false;
        for (float value : {adjustment.time_multiplier, adjustment.time_add_seconds,
                adjustment.lean_multiplier, adjustment.lean_add_signed,
                adjustment.speed_multiplier, adjustment.speed_add_signed})
            if (!std::isfinite(value)) return false;
        const float time = time_ * adjustment.time_multiplier;
        const float lean = lean_ * adjustment.lean_multiplier;
        const float speed = speed_ * adjustment.speed_multiplier;
        const float adjusted_time = time + adjustment.time_add_seconds;
        const float adjusted_lean = lean + adjustment.lean_add_signed * sign(lean);
        const float adjusted_speed = speed + adjustment.speed_add_signed * sign(speed);
        if (!std::isfinite(time) || !std::isfinite(lean) || !std::isfinite(speed) ||
            !std::isfinite(adjusted_time) || !std::isfinite(adjusted_lean) || !std::isfinite(adjusted_speed))
            return false;
        time_ = std::max(0.0f, adjusted_time); lean_ = adjusted_lean; speed_ = adjusted_speed;
        return true;
    }
    bool begin(bool nose) noexcept {
        if (closed_) return false;
        if (active_ && nose_ == nose) return true; // Only the same type keeps its current setup.
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
        return sample(dt, PhysicalManualControl{correction, correction != 0, true, 1000});
    }
    PhysicalBalanceSample sample(float dt, const PhysicalManualControl& control) noexcept {
        if (!std::isfinite(dt) || dt <= 0 || dt > .25f || !std::isfinite(control.amount)) {
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
        float applied_correction=0;
        if (control.held && control.released) {
            // Safe-period direction is tested after integrating this step's
            // lean. A suppressed held branch does not consume neutral noise.
            const bool suppressed = control.elapsed_ms < 1000 && control.amount * lean_ > 0;
            if (!suppressed) {
                applied_correction=std::clamp(control.amount,-1.0f,1.0f);
                speed_ += p.correction_acceleration * frames * applied_correction;
            }
        }
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
            speed_ / GameplayProfile::balance_display_scale, time_, cheese_, outcome,applied_correction};
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
