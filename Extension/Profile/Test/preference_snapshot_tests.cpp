// Scratch stores only. The readback must not commit, migrate or change history.
#include "Extension/Profile/local_profile.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

int main() {
    using namespace dingosdk;
    unsigned checks{};
    const auto check = [&](bool value, const char* reason) {
        ++checks;
        if (!value) throw std::runtime_error(reason);
    };
    const auto folder = std::filesystem::temp_directory_path() /
        ("preference-snapshot-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(folder)) return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{folder};
    try {
        const auto observe = [&](profile::Store& store, profile::PreferenceState expected) {
            const auto before = store.export_json();
            const auto revision = store.revision(), changes = profile::Store::changes();
            const auto result = store.arcade_preference_snapshot();
            check(result.state == expected, "Preference state was conflated");
            check(result.revision_known && result.revision == revision, "Revision did not belong to the read");
            check(store.export_json() == before && store.revision() == revision &&
                profile::Store::changes() == changes, "Readback changed the store");
        };
        {
            profile::Store store(folder / "values.sqlite3");
            observe(store, profile::PreferenceState::absent);
            store.set_user_value("ReSkate.ArcadeMode", false);
            observe(store, profile::PreferenceState::false_value);
            store.set_user_value("ReSkate.ArcadeMode", true);
            observe(store, profile::PreferenceState::true_value);
        }
        {
            profile::Store store(folder / "invalid.sqlite3");
            store.set_user_value("ReSkate.ArcadeMode", 7);
            observe(store, profile::PreferenceState::invalid);
            // A real Boolean option takes precedence over a legacy setting.
            store.set_bool_option("ReSkate.ArcadeMode", false);
            observe(store, profile::PreferenceState::false_value);
        }
        const profile::PreferenceSnapshot missing{};
        check(missing.state == profile::PreferenceState::unavailable && !missing.revision_known &&
            missing.revision == 0, "Unavailable store acquired a revision");
        std::cout << checks << " store readback controls passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
