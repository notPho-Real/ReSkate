#pragma once
#include "Extension/Profile/local_profile_preference_snapshot.h"

namespace dingosdk::arcade_mode {
// Per-field readback only. This is not a process, store-instance or native
// ownership receipt, nor an independent observation of the database on disk.
struct RuntimeSettingsSnapshot {
    std::uint64_t sampled_at{};
    unsigned console_key{};
    bool ready{}, enabled{};
    profile::PreferenceSnapshot preference{};
};
constexpr bool runtime_settings_snapshot_valid(const RuntimeSettingsSnapshot& snapshot) noexcept {
    return snapshot.sampled_at && snapshot.console_key && snapshot.console_key <= 255 &&
        profile::preference_snapshot_valid(snapshot.preference);
}
}
