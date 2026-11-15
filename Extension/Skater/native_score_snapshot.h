#pragma once
#include <cstdint>
#include <vector>
namespace dingosdk::tricks {
struct NativeScoreSequence {
    // Native index metadata repeats within a line; it is not an occurrence ID.
    std::uint32_t points{}, index{}, kind{};
    std::uint64_t current_action{};
    // Internal references scoped to the live scoring owner. Never dereference
    // after the registry lock or expose to plugins. An asset address alone is
    // not a globally unique performed-trick occurrence.
    std::vector<std::uint64_t> action_objects;
};
struct NativeScoreSnapshot {
    std::uint64_t hud{}, scoring{};
    std::uintptr_t manager{};
    NativeScoreSequence current, previous;
    std::vector<NativeScoreSequence> completed, previous_line;
};
}
