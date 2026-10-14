#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace dingosdk::arcade_mode {
// Orbit the existing follow-camera frame; never changes the skater or velocity.
class CameraOrbit {
public:
    void reset() noexcept { yaw_ = pitch_ = 0; }
    std::optional<std::array<float, 16>> update(const std::array<float, 16>& follow,
            const std::array<float, 3>& target, float right_x, float right_y, float seconds) noexcept {
        for (const auto value : follow) if (!std::isfinite(value)) return {};
        for (const auto value : target) if (!std::isfinite(value)) return {};
        if (!std::isfinite(right_x) || !std::isfinite(right_y) || !std::isfinite(seconds) || seconds < 0) return {};
        const auto axis = [](float v) { return std::abs(v) <= .2f ? 0.0f : std::copysign((std::min(std::abs(v), 1.0f) - .2f) / .8f, v); };
        const float dt = std::min(seconds, .05f);
        yaw_ = std::remainder(yaw_ - axis(right_x) * dt * 2.8f, 6.283185307f);
        pitch_ = std::clamp(pitch_ - axis(right_y) * dt * 1.8f, -.6f, 1.0f);
        const float dx = follow[12] - target[0], dy = follow[13] - target[1], dz = follow[14] - target[2];
        const float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!std::isfinite(radius) || radius < .5f || radius > 15) return {};
        const float yaw = std::atan2(dx, dz) + yaw_;
        const float pitch = std::clamp(std::asin(std::clamp(dy / radius, -1.0f, 1.0f)) + pitch_, -.35f, 1.25f);
        const float cp = std::cos(pitch), sp = std::sin(pitch), sy = std::sin(yaw), cy = std::cos(yaw);
        const std::array<float, 3> back{sy * cp, sp, cy * cp};
        const std::array<float, 3> right{cy, 0, -sy};
        const std::array<float, 3> up{-sp * sy, cp, -sp * cy};
        return std::array<float, 16>{right[0], right[1], right[2], 0, up[0], up[1], up[2], 0,
            back[0], back[1], back[2], 0, target[0] + back[0] * radius,
            target[1] + back[1] * radius, target[2] + back[2] * radius, 1};
    }
private:
    float yaw_{}, pitch_{};
};
}
