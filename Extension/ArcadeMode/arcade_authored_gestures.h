#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace dingosdk::arcade_mode {
enum class GestureContext : std::uint8_t { ground, from_grind, underflip };
// Fixed vocabulary only: unknown authored labels never become arbitrary log text.
constexpr unsigned authored_name_tokens(std::string_view name) noexcept {
    constexpr std::string_view words[]{"impossible", "nocomply", "double", "triple", "540",
        "frontfoot", "backfoot", "360", "kickflip", "heelflip"};
    unsigned bits{};
    for (unsigned i = 0; i < 10; ++i) if (name.find(words[i]) != std::string_view::npos) bits |= 1u << i;
    if (name.find("no_comply") != std::string_view::npos) bits |= 2;
    if (name.find("front_foot") != std::string_view::npos) bits |= 32;
    if (name.find("back_foot") != std::string_view::npos) bits |= 64;
    return bits;
}
// Raw authored degrees. The upstream native adapter rotates arc angles by 180
// degrees before using clockwise-from-up XInput vectors; parsing preserves data.
struct AuthoredArc { float start{}, end{}, minimum{}, maximum{}, wait{}; };
struct AuthoredGesture {
    std::uint8_t native_id{};
    bool nollie{};
    GestureContext context{};
    float hold_start{}, hold_end{}, hold_minimum{};
    std::vector<AuthoredArc> arcs;
    unsigned source_tokens{};
};
struct AuthoredLoadDiagnostics { unsigned stage{}, field{}, error{}, rejected{}; };
// Read only from the installed game's level-root bundle. Never writes game data.
std::vector<AuthoredGesture> read_authored_gestures(const std::filesystem::path& game_root,
                                                 AuthoredLoadDiagnostics* diagnostics = nullptr);
}
