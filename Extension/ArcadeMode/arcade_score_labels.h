#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace dingosdk::arcade_mode {
// Diagnostic IDs, separate from native animation IDs. Exact text matches only;
// unknown labels and player-authored text never enter telemetry.
constexpr std::array<std::string_view, 20> score_labels{
    "Manual", "Nose Manual", "FS Revert", "BS Revert",
    "Melon", "Nosegrab", "Indy", "Tailgrab", "Japan", "Madonna", "Benihana", "Airwalk",
    "Method", "Rocket Air", "Stiffy", "One Foot Tailgrab", "One Foot Japan", "Judo", "Sacktap", "Christ Air"
};
constexpr std::uint32_t score_label_id(std::string_view text) noexcept {
    for (std::size_t i = 0; i < score_labels.size(); ++i)
        if (text == score_labels[i]) return static_cast<std::uint32_t>(i + 1);
    return 0;
}
}
