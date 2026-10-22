"""Read-only provider integration checks; these do not replace C++ execution."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[3]


def body(path, marker):
    source = (ROOT / path).read_text()
    assert source.count(marker) == 1
    hidden = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                    lambda match: ' ' * len(match.group()), source)
    start = source.index(marker)
    opening = hidden.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (hidden[end] == '{') - (hidden[end] == '}')
        end += 1
    return source[start:end]


class ReadOnlySettingsSource(unittest.TestCase):
    def test_live_sources_not_configuration_or_last_log(self):
        provider = body('Extension/ArcadeMode/arcade_mode.cpp',
                        'RuntimeSettingsSnapshot read_runtime_settings_snapshot() noexcept')
        for expected in ('launcher::overlay_keys().console', 'ready.load(', 'mode_enabled.load(',
                         'profile_runtime::local_arcade_preference_snapshot()', 'GetTickCount64()'):
            self.assertIn(expected, provider)
        for forbidden in ('set_enabled(', 'set_local_preference(', 'getenv(', 'ifstream', 'ofstream',
                          'Store(', 'SaveDatabase', 'path(', 'preference_loaded'):
            self.assertNotIn(forbidden, provider)

    def test_fixed_store_lookup_nonblocking_and_read_only(self):
        store = body('Extension/Profile/local_profile.cpp',
                     'PreferenceSnapshot Store::arcade_preference_snapshot() const noexcept')
        self.assertIn('std::try_to_lock', store)
        self.assertIn('if (!lock.owns_lock()) return {};', store)
        self.assertIn('const std::string key = "ReSkate.ArcadeMode";', store)
        self.assertLess(store.index('value_.bool_options.find'), store.index('value_.settings'))
        self.assertIn('value_.revision', store)
        for forbidden in ('Update ', 'commit(', 'set_user_value(', 'set_bool_option(', 'Store(',
                          'SaveDatabase', 'ifstream', 'ofstream', '.path(', '++', '--'):
            self.assertNotIn(forbidden, store)

    def test_profile_owner_and_unavailability(self):
        provider = body('Extension/Profile/local_profile_preferences.cpp',
                        'profile::PreferenceSnapshot local_arcade_preference_snapshot() noexcept')
        for expected in ('std::try_to_lock', 'lock.owns_lock()', 's.active.load(', '!s.store',
                         's.store->arcade_preference_snapshot()'):
            self.assertIn(expected, provider)
        for forbidden in ('set_local_preference(', 'make_unique', 'make_shared', 'new ', 'initialize',
                          'save(', 'commit(', 'logging::', '.path('):
            self.assertNotIn(forbidden, provider)

    def test_all_typed_states_and_no_invented_identity(self):
        source = (ROOT / 'Extension/Profile/local_profile_preference_snapshot.h').read_text()
        for label in ('unavailable', 'absent', 'false', 'true', 'invalid'):
            self.assertIn('return "' + label + '";', source)
        mode = (ROOT / 'Extension/ArcadeMode/arcade_mode.cpp').read_text()
        self.assertIn('store_identity_known=false store_identity=0', mode)
        self.assertIn('scope=per_field disk_readback=false', mode)

    def test_existing_bounded_heartbeat_and_worker_only_output(self):
        mode = (ROOT / 'Extension/ArcadeMode/arcade_mode.cpp').read_text()
        self.assertEqual(mode.count('read_runtime_settings_snapshot()});'), 1)
        tick = body('Extension/ArcadeMode/arcade_mode.cpp', 'void on_client_tick() noexcept')
        self.assertIn('now - hil_heartbeat_at >= 250', tick)
        self.assertNotIn('ARCADE SETTINGS SNAPSHOT:', tick)
        self.assertEqual(mode.count('ARCADE SETTINGS SNAPSHOT:'), 1)
        telemetry = (ROOT / 'Extension/ArcadeMode/arcade_input_telemetry.h').read_text()
        self.assertIn('RuntimeSettingsSnapshot settings{};', telemetry)
        self.assertIn('now - heartbeat->ms <= 100', telemetry)

    def test_test_registration(self):
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        for name in ('arcade_settings_snapshot', 'profile_preference_snapshot', 'arcade_settings_snapshot_source'):
            self.assertIn('add_test(NAME ' + name + ' ', cmake)


    def test_required_windows_gate_selects_snapshot_tests(self):
        workflow = (ROOT / '.github/workflows/arcade-dev.yml').read_text()
        required = workflow.split('  windows-test-diagnostic:', 1)[0]
        configure = next(line for line in required.splitlines() if 'run: cmake --preset' in line)
        for option in ('DINGOSDK_BUILD_PARK_EDITOR_TESTS', 'DINGOSDK_BUILD_ARCADE_MODE_TESTS'):
            self.assertIn('-D' + option + '=ON', configure)
        compile_line = next(line for line in required.splitlines() if 'run: cmake --build' in line)
        targets = compile_line.split('--target ', 1)[1].split()
        for target in ('dingosdk_profile_preference_snapshot_tests', 'dingosdk_arcade_settings_snapshot_tests'):
            self.assertIn(target, targets)
        ctest = next(line for line in required.splitlines() if 'run: ctest ' in line)
        pattern = re.search(r'-R "([^"]+)"', ctest).group(1)
        for name in ('profile_preference_snapshot', 'arcade_settings_snapshot', 'arcade_settings_snapshot_source'):
            self.assertIsNotNone(re.fullmatch(pattern, name), name)
        for name in ('profile_preference_snapshot_extra', 'arcade_settings_snapshot_extra', 'arcade_settings_snapshot_source_extra'):
            self.assertIsNone(re.fullmatch(pattern, name), name)

if __name__ == '__main__':
    unittest.main()
