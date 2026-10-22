#pragma once
#include <cstdint>

namespace dingosdk::profile {
enum class PreferenceState : std::uint8_t { unavailable, absent, false_value, true_value, invalid };
struct PreferenceSnapshot {
    PreferenceState state{PreferenceState::unavailable};
    bool revision_known{};
    std::uint64_t revision{};
};
constexpr const char* preference_state_name(PreferenceState state) noexcept {
    switch (state) {
    case PreferenceState::unavailable: return "unavailable";
    case PreferenceState::absent: return "absent";
    case PreferenceState::false_value: return "false";
    case PreferenceState::true_value: return "true";
    case PreferenceState::invalid: return "invalid";
    }
    return "unavailable";
}
constexpr bool preference_snapshot_valid(const PreferenceSnapshot& snapshot) noexcept {
    switch (snapshot.state) {
    case PreferenceState::unavailable: return !snapshot.revision_known && snapshot.revision == 0;
    case PreferenceState::absent:
    case PreferenceState::false_value:
    case PreferenceState::true_value:
    case PreferenceState::invalid: return snapshot.revision_known;
    }
    return false;
}
}
