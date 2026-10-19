#pragma once
#include "arcade_gameplay_profile.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace dingosdk::arcade_mode {
// Selected ground kick response, in SI units. This supplies a bounded
// acceleration budget through the existing Trainer body writer. Native
// steering, vertical velocity and collisions remain authoritative.
class GroundDrive {
public:
    static constexpr float standing_speed = GameplayProfile{}.kick_speed(false);
    static constexpr float standing_acceleration = GameplayProfile{}.kick_acceleration(false);
    static constexpr float crouched_speed = GameplayProfile{}.kick_speed(true);
    static constexpr float crouched_acceleration = GameplayProfile{}.kick_acceleration(true);
    explicit GroundDrive(GameplayProfile profile = {}) noexcept : profile_(profile) {}
    void reset() noexcept { const auto selected = profile_; *this = GroundDrive(selected); }
    float step(float dt, const std::array<float, 3>& position,
               const std::array<float, 3>& velocity, bool crouched, bool may_start = false) noexcept {
        if (!std::isfinite(dt) || dt < .005f || dt > .04f) { reset(); return 0; }
        for (unsigned i = 0; i < 3; ++i)
            if (!std::isfinite(position[i]) || std::abs(position[i]) > 1000000 ||
                !std::isfinite(velocity[i]) || std::abs(velocity[i]) >= 1000) { reset(); return 0; }
        const float speed = std::hypot(velocity[0], velocity[2]);
        if (blocked_) {
            if (speed < .5f || std::hypot(position[0] - position_[0], position[2] - position_[2]) < .25f)
                return 0;
            reset(); // Native motion must first clear the obstruction.
        }
        if (speed < .5f && !may_start) { reset(); return 0; }
        float change{};
        if (primed_) {
            const float dx = position[0] - position_[0], dz = position[2] - position_[2];
            travel_time_ += dt; expected_travel_ += speed * dt;
            const float travel = std::hypot(dx, dz);
            const bool teleported = travel > std::max(1.0f, expected_travel_ * 4);
            const bool stalled = travel_time_ >= .1f && travel < expected_travel_ * .2f;
            const bool impact = speed < previous_speed_ - 18 * dt;
            // Resting bodies retain tiny signed velocity noise. It is not a
            // travel direction against which a fresh forward start can reverse.
            const bool reversed = speed >= .5f && std::hypot(velocity_[0], velocity_[2]) >= .5f &&
                velocity[0] * velocity_[0] + velocity[2] * velocity_[2] < 0;
            if (teleported || stalled || impact || reversed) {
                reset(); blocked_ = true; position_ = position; return 0;
            }
            if (travel_time_ >= .1f) {
                position_ = position; travel_time_ = expected_travel_ = 0;
            }
            const float acceleration = profile_.kick_acceleration(crouched);
            const float limit = profile_.kick_speed(crouched);
            // Count acceleration already supplied by native pushing. Compensate
            // only bounded ground steering drag, never a collision-sized loss.
            const bool flat = std::abs(velocity[1]) < .3f && std::abs(position[1] - position_[1]) < .025f;
            const float budget = acceleration * dt - std::max(0.0f, speed - previous_speed_);
            const float goal = std::min(limit, flat ? previous_speed_ + acceleration * dt : speed + std::max(0.0f, budget));
            change = std::clamp(goal - speed, 0.0f, (acceleration + 18) * dt);
        } else {
            primed_ = true; position_ = position;
            // The guarded caller permits this only on native two-wheel ground.
            // Ground acceleration need not wait for the native kick animation.
            if (may_start && speed < .5f)
                change = profile_.kick_acceleration(crouched) * dt;
        }
        previous_speed_ = speed + change; velocity_ = velocity;
        return change;
    }
private:
    GameplayProfile profile_{};
    bool primed_{}, blocked_{};
    float previous_speed_{}, travel_time_{}, expected_travel_{};
    std::array<float, 3> position_{}, velocity_{};
};
}
