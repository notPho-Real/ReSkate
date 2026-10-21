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
        // Lookaround is a bounded angle target, not a rate that accumulates
        // indefinitely. Neutral returns toward the follow frame. Normalize
        // the per-sample response to the existing 60 Hz control baseline.
        const float blend = -std::expm1(std::log(15.0f / 16.0f) * dt * 60.0f);
        const float horizontal = axis(right_x), vertical = axis(right_y);
        const float yaw_target = -horizontal * 3.0f;
        const float pitch_target = -vertical * (vertical > 0 ? 1.4f : .56f);
        yaw_ += (yaw_target - yaw_) * blend;
        pitch_ += (pitch_target - pitch_) * blend;
        const float dx = follow[12] - target[0], dy = follow[13] - target[1], dz = follow[14] - target[2];
        const float radius = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!std::isfinite(radius) || radius < .5f || radius > 15) return {};
        // Neutral preserves the supplied eye, optical aim and roll. The pivot
        // controls only the orbit; it is not a replacement optical target.
        if (yaw_ == 0 && pitch_ == 0) return follow;
        const float base_pitch = std::asin(std::clamp(dy / radius, -1.0f, 1.0f));
        // An incoming pose outside the orbit range must not snap on entry.
        const float pitch = std::clamp(base_pitch + pitch_, std::min(-.35f, base_pitch), std::max(1.25f, base_pitch));
        const float elevation = pitch - base_pitch;
        const float yaw = std::atan2(dx, dz) + yaw_;
        const float sy = std::sin(yaw), cy = std::cos(yaw);
        const float turn_sin = std::sin(yaw_), turn_cos = std::cos(yaw_);
        const float tilt_sin = std::sin(elevation), tilt_cos = std::cos(elevation);
        // Apply the same rigid world-up yaw and orbit-plane elevation to all
        // basis axes and the eye offset. Preserve their relative geometry.
        const auto rotate = [&](float x, float y, float z) {
            const float turned_x = x * turn_cos + z * turn_sin;
            const float turned_z = z * turn_cos - x * turn_sin;
            const float side = turned_x * cy - turned_z * sy;
            const float radial = turned_x * sy + turned_z * cy;
            const float tilted = radial * tilt_cos - y * tilt_sin;
            return std::array<float, 3>{side * cy + tilted * sy,
                radial * tilt_sin + y * tilt_cos, tilted * cy - side * sy};
        };
        auto result = follow;
        for (unsigned basis_axis = 0; basis_axis < 3; ++basis_axis) {
            const auto rotated = rotate(follow[basis_axis * 4], follow[basis_axis * 4 + 1], follow[basis_axis * 4 + 2]);
            for (unsigned component = 0; component < 3; ++component) result[basis_axis * 4 + component] = rotated[component];
        }
        const auto eye = rotate(dx, dy, dz);
        for (unsigned component = 0; component < 3; ++component) result[12 + component] = target[component] + eye[component];
        return result;
    }
private:
    float yaw_{}, pitch_{};
};
}
