#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

namespace dingosdk::arcade_mode {
enum class GestureContext : std::uint8_t { ground, from_grind, underflip };
// Raw authored degrees. The upstream native adapter rotates arc angles by 180
// degrees before using clockwise-from-up XInput vectors; parsing preserves data.
struct AuthoredArc { float start{}, end{}, minimum{}, maximum{}, wait{}; };
struct AuthoredGesture {
    std::uint8_t native_id{};
    bool nollie{};
    GestureContext context{};
    float hold_start{}, hold_end{}, hold_minimum{};
    std::vector<AuthoredArc> arcs;
};
struct AuthoredLoadDiagnostics { unsigned stage{}, field{}, error{}, rejected{}; };
// Read only from the installed game's level-root bundle. Never writes game data.
std::vector<AuthoredGesture> read_authored_gestures(const std::filesystem::path& game_root,
                                                 AuthoredLoadDiagnostics* diagnostics = nullptr);
}
