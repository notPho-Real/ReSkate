#pragma once
#include <cstdint>
#include <limits>
#include <string_view>

namespace dingosdk::arcade_mode {
struct ScoreDisplayNumber {
    bool valid{};
    std::uint64_t value{};
};
// Numeric display text only. Unsupported formatting stays unavailable rather
// than becoming zero. No locale separators are assumed without native evidence.
inline ScoreDisplayNumber parse_score_display_number(std::string_view text, bool multiplier = false) noexcept {
    if (multiplier && !text.empty() && (text.front() == 'x' || text.front() == 'X')) text.remove_prefix(1);
    if (text.empty() || text.size() > 20) return {};
    std::uint64_t value{};
    for (const char digit : text) {
        if (digit < '0' || digit > '9') return {};
        const auto part = static_cast<unsigned>(digit - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - part) / 10) return {};
        value = value * 10 + part;
    }
    if (multiplier && value == 0) return {};
    return {true, value};
}
}
