#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Read-only graph/schema mapping adapted from SmokeyDawg58/reskate-custom-mods
// (GPL-3.0), de837826417b55e2a5756c303f4805ae6a39ae89:
// src/Engine/Game/Build/20260929/one_up_scoring.h and
// src/Extension/Throwdowns/one_up_native.cpp. Contracts target build 25414733.
// Native scoring settings and the source's game-mode scoring policy are omitted.
namespace dingosdk::arcade_mode::native_score_detail {
enum class Route : std::uint8_t { none, line_start, line_end, stats_line_end, sequence_end };
struct Graph { std::uint32_t hash, frame, constants, words, input; Route route; };
inline constexpr std::array graphs{
    Graph{0xa4d1121c, 0x40, 0x48, 0x7b, 0, Route::line_start},
    Graph{0x69843dad, 0x70, 0x2d0, 0x408, 40, Route::line_end},
    Graph{0xe2d57480, 0x50, 0, 0x21, 16, Route::stats_line_end},
    Graph{0xafef12e9, 0x470, 0x908, 0xffe, 1080, Route::sequence_end}};
inline constexpr std::array<std::uint32_t, 4> type_hashes{
    0xd7f51dda, 0xf95bfe33, 2977686039U, 1606121107U};
inline constexpr const Graph* graph_for(std::uint32_t hash) noexcept {
    for (const auto& g : graphs) if (g.hash == hash) return &g;
    return nullptr;
}
inline constexpr bool contract(const Graph& g, std::uint32_t frame,
    std::uint32_t constants, std::uint32_t words) noexcept {
    return g.frame == frame && g.constants == constants && g.words == words;
}
struct Type { std::uintptr_t object{}, record{}; std::uint16_t size{}; };
struct Field { std::uintptr_t type{}; std::uint16_t offset{}, size{}; unsigned kind{}; };
struct Binding {
    std::array<Type, 4> types{};
    Field line_data{}, reason{}, line_score{}, sequence_data{}, success{}, sequence_score{};
};
template<class Read> bool type_matches(Read& read, const Type& type, std::uint32_t hash) noexcept {
    std::uintptr_t record{}; std::uint32_t actual{}; std::uint16_t flags{}, size{};
    return read(type.object, record) && record == type.record && read(record, actual) && actual == hash &&
        read(record + 4, flags) && ((flags >> 5) & 31) == 2 &&
        read(record + 6, size) && size == type.size && size != 0;
}
// Only the four embedded struct schemas are needed: no arrays, inheritance,
// history traversal, arbitrary strings, or unbounded reflection recursion.
template<class Read> bool field(Read& read, const Type& parent, std::uint32_t hash, Field& result) noexcept {
    std::uint16_t count{}; std::uintptr_t fields{};
    if (!read(parent.record + 0x2a, count) || count > 96 || !read(parent.record + 0x60, fields)) return false;
    for (unsigned i = 0; i < count; ++i) {
        const auto at = fields + i * 24ULL;
        std::uint32_t actual{};
        if (!read(at, actual)) return false;
        if (actual != hash) continue;
        Field f{}; std::uintptr_t record{}; std::uint16_t flags{};
        if (!read(at + 8, f.offset) || !read(at + 16, f.type) || !read(f.type, record) ||
            !read(record + 4, flags) || !read(record + 6, f.size)) return false;
        f.kind = (flags >> 5) & 31;
        if (!f.size || static_cast<unsigned>(f.offset) + f.size > parent.size) return false;
        result = f; return true;
    }
    return false;
}
template<class Read> bool bind_schema(Read& read, const std::array<std::uintptr_t, 4>& objects, Binding& output) noexcept {
    Binding b{};
    for (unsigned i = 0; i < objects.size(); ++i) {
        auto& t = b.types[i]; t.object = objects[i];
        if (!read(t.object, t.record) || !read(t.record + 6, t.size) || !type_matches(read, t, type_hashes[i])) return false;
    }
    if (!field(read, b.types[0], 0xcdfa75f5, b.line_data) || !field(read, b.types[0], 0x9f0bcbf5, b.reason) ||
        !field(read, b.types[1], 0x40bff667, b.line_score) ||
        !field(read, b.types[2], 3982548569U, b.sequence_data) || !field(read, b.types[2], 3611278405U, b.success) ||
        !field(read, b.types[3], 2099127164U, b.sequence_score)) return false;
    if (b.line_data.type != objects[1] || b.line_data.kind != 2 || b.line_data.size != b.types[1].size ||
        b.sequence_data.type != objects[3] || b.sequence_data.kind != 2 || b.sequence_data.size != b.types[3].size ||
        b.reason.size != 4 || (b.reason.kind != 8 && b.reason.kind != 15 && b.reason.kind != 16) ||
        b.line_score.size != 4 || (b.line_score.kind != 19 && b.line_score.kind != 15) ||
        b.success.size != 1 || b.success.kind != 10 || b.sequence_score.size != 4 ||
        (b.sequence_score.kind != 15 && b.sequence_score.kind != 16)) return false;
    output = b; return true;
}
struct Payload { double score{}; std::uint32_t reason{}; bool success{}; };
inline bool score_valid(double score) noexcept {
    return std::isfinite(score) && score >= 0 && score <= 2147483647.0;
}
template<class Read> bool decode(Read& read, const Binding& b, Route route,
    std::uintptr_t payload, Payload& output) noexcept {
    Payload result{};
    if (route == Route::sequence_end) {
        std::uint8_t success{};
        if (!read(payload + b.success.offset, success) || success > 1) return false;
        result.success = success != 0;
        const auto at = payload + b.sequence_data.offset + b.sequence_score.offset;
        if (b.sequence_score.kind == 16) { std::uint32_t v{}; if (!read(at, v)) return false; result.score = v; }
        else { std::int32_t v{}; if (!read(at, v)) return false; result.score = v; }
    } else if (route == Route::line_end || route == Route::stats_line_end) {
        if (!read(payload + b.reason.offset, result.reason) || result.reason > 8) return false;
        const auto at = payload + b.line_data.offset + b.line_score.offset;
        if (b.line_score.kind == 19) { float v{}; if (!read(at, v)) return false; result.score = v; }
        else { std::int32_t v{}; if (!read(at, v)) return false; result.score = v; }
    } else return false;
    if (!score_valid(result.score)) return false;
    output = result; return true;
}
}
