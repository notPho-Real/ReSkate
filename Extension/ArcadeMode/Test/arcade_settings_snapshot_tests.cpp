#include "Extension/ArcadeMode/arcade_settings_snapshot.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <type_traits>

int main() {
    using namespace dingosdk;
    using profile::PreferenceState;
    unsigned checks{};
    const auto check = [&](bool value) {
        ++checks;
        if (!value) std::exit(1);
    };
    static_assert(std::is_trivially_copyable_v<arcade_mode::RuntimeSettingsSnapshot>);
    const std::array states{PreferenceState::unavailable, PreferenceState::absent,
        PreferenceState::false_value, PreferenceState::true_value, PreferenceState::invalid};
    const std::array<std::string_view, 5> names{"unavailable", "absent", "false", "true", "invalid"};
    for (std::size_t index = 0; index < states.size(); ++index) {
        profile::PreferenceSnapshot preference;
        preference.state = states[index];
        preference.revision_known = states[index] != PreferenceState::unavailable;
        preference.revision = preference.revision_known ? 12 : 0;
        check(profile::preference_state_name(states[index]) == names[index]);
        check(profile::preference_snapshot_valid(preference));
        // Runtime mode and saved preference are independent observations.
        for (const bool enabled : {false, true}) {
            arcade_mode::RuntimeSettingsSnapshot snapshot{1000, 0xC0, true, enabled, preference};
            check(arcade_mode::runtime_settings_snapshot_valid(snapshot));
            check(snapshot.enabled == enabled && snapshot.preference.state == states[index]);
            snapshot.console_key = 0x70;
            check(snapshot.console_key == 0x70 && arcade_mode::runtime_settings_snapshot_valid(snapshot));
        }
    }
    check(!profile::preference_snapshot_valid({PreferenceState::unavailable, true, 12}));
    check(!profile::preference_snapshot_valid({PreferenceState::absent, false, 0}));
    check(!profile::preference_snapshot_valid({static_cast<PreferenceState>(255), true, 12}));
    check(!arcade_mode::runtime_settings_snapshot_valid({}));
    std::cout << checks << " settings snapshot value controls passed\n";
}
