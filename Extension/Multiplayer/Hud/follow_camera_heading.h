#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace dingosdk::multiplayer {
// Position sampling and heading easing have separate clocks: camera phases
// may read the same skater pose more than once before physics advances.
class FollowCameraHeading {
public:
    void reset(const std::array<float, 3>& position, float x, float z) noexcept {
        last_ = position;
        const float length = std::hypot(x, z);
        yaw_ = target_ = std::isfinite(length) && length > .01f ? std::atan2(x, z) : 0;
        sample_seconds_ = 0;
    }
    std::array<float, 2> update(const std::array<float, 3>& position, float seconds) noexcept {
        if (!std::isfinite(seconds) || seconds < 0) return direction();
        for (const auto value : position) if (!std::isfinite(value)) return direction();
        const float dt = std::min(seconds, .1f);
        sample_seconds_ += dt;
        const float dx = position[0] - last_[0], dz = position[2] - last_[2];
        const float distance = std::hypot(dx, dz);
        if (distance > .0001f && sample_seconds_ > 0) {
            const float speed = distance / sample_seconds_;
            if (speed > 1 && speed < 60) target_ = std::atan2(dx, dz);
            // A teleport is a new baseline, never a travel-direction sample.
            last_ = position;
            sample_seconds_ = 0;
        } else if (sample_seconds_ >= .1f) {
            last_ = position;
            sample_seconds_ = 0;
        }
        constexpr float pi = 3.14159265358979323846f;
        float angle = std::remainder(target_ - yaw_, 2 * pi);
        // Exactly opposite unit vectors cannot be eased by normalized linear
        // blending: they remain opposite forever. Ease the shortest yaw arc.
        if (std::abs(std::abs(angle) - pi) < .00001f) angle = pi;
        yaw_ = std::remainder(yaw_ + angle * -std::expm1(-dt / .2f), 2 * pi);
        return direction();
    }
private:
    std::array<float, 2> direction() const noexcept { return {std::sin(yaw_), std::cos(yaw_)}; }
    std::array<float, 3> last_{};
    float yaw_{}, target_{}, sample_seconds_{};
};
}
