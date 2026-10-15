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
    // Observed on Deck after a native revert changes riding context. This
    // preserves semantic identity; persistent text still cannot prove repeats.
    if (text == "Switch FS Revert") return 3;
    if (text == "Switch BS Revert") return 4;
    // Exact current-game guide spellings, sharing the same semantic IDs.
    if (text == "Nose Grab") return 6;
    if (text == "Tail Grab") return 8;
    if (text == "Japan Air") return 9;
    if (text == "One-Foot Tail Grab") return 16;
    if (text == "Judo Air") return 18;
    for (std::size_t i = 0; i < score_labels.size(); ++i)
        if (text == score_labels[i]) return static_cast<std::uint32_t>(i + 1);
    return 0;
}
}
