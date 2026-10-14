#pragma once
#include <algorithm>
#include <cmath>

namespace dingosdk::arcade_mode {
struct BalanceHudPoint { float x{}, y{}; };
struct BalanceHudLayout {
    bool vertical{};
    float scale{}, half_length{}, track_half_width{}, cursor_radius{};
    BalanceHudPoint center{}, panel_min{}, panel_max{};
    BalanceHudPoint point(float normalized, float across = 0) const noexcept {
        const float offset = std::clamp(std::isfinite(normalized) ? normalized : 0.0f, -1.0f, 1.0f) * half_length;
        return vertical ? BalanceHudPoint{center.x + across, center.y + offset}
                        : BalanceHudPoint{center.x + offset, center.y + across};
    }
};
inline BalanceHudLayout balance_hud_layout(float width, float height, bool vertical) noexcept {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return {};
    const float scale = std::min(width / 1280.0f, height / 800.0f);
    const float half = (vertical ? height : width) * 0.15f;
    const BalanceHudPoint center{width * (vertical ? 0.80f : 0.5f), height * (vertical ? 0.50f : 0.78f)};
    const float across = 50 * scale;
    return {vertical, scale, half, 9 * scale, 12 * scale, center,
        {center.x - (vertical ? across : half + 24 * scale),
         center.y - (vertical ? half + 42 * scale : 62 * scale)},
        {center.x + (vertical ? across : half + 24 * scale),
         center.y + (vertical ? half + 24 * scale : 34 * scale)}};
}
}
