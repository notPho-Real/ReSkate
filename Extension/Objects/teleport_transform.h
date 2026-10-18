#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <optional>

namespace dingosdk {
// Same upright transform accepted by the existing native teleport manager.
// Heading follows Trainer's atan2(forward.x, forward.z), in degrees.
inline std::optional<std::array<float, 16>> upright_teleport_transform(
        const std::array<float, 3>& position, float heading) noexcept {
    for (const auto v : position)
        if (!std::isfinite(v) || std::abs(v) > 1e6f) return {};
    if (!std::isfinite(heading) || std::abs(heading) > 360) return {};
    const float angle = heading * std::numbers::pi_v<float> / 180;
    const float c = std::cos(angle), s = std::sin(angle);
    return std::array<float, 16>{c,0,-s,0, 0,1,0,0, s,0,c,0,
        position[0],position[1],position[2],1};
}
}
