"""Current native menu observations diagnose fixture failures, never close UI."""
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, patch
sys.path.insert(0, str(Path(__file__).parent / 'lab'))
import probe
import trick_batch as batch


def fixture(**extra):
    row = dict(configured=True, available=True, busy=False, failed=False, editor_open=False,
        asset_available=True, present=True, skater=True, reset_ready=False, arcade=True,
        heading=0, speed=0, state=504, generation=3, revision=1, position=[1, 2, 3],
        start=[1, 2, 3], rail_position=[1, 2, 11], rail_rotation=[0, 0, 0, 1], rail_scale=1)
    row.update(extra)
    return row


def parse(**extra):
    return probe.parse_fixture_status('ARCADE FIXTURE: ' + json.dumps(fixture(**extra)))


class FixtureMenuTests(unittest.TestCase):
    def test_current_pair_explicit_and_historical_transition_ignored(self):
        for opened in (False, True):
            row = parse(native_ui_available=True, native_in_menu=opened, private='omit')
            self.assertEqual(row['native_menu'], 'open' if opened else 'closed')
            self.assertIs(row['native_ui_available'], True)
            self.assertIs(row['native_in_menu'], opened)
            self.assertNotIn('private', row)
            self.assertFalse(row['reset_ready'])
        historical = 'Trace: a menu of the game\x27s is up.\nARCADE FIXTURE: ' + json.dumps(fixture())
        self.assertNotIn('native_menu', probe.parse_fixture_status(historical))

    def test_unavailable_and_malformed_are_not_closed(self):
        row = parse(native_ui_available=False, native_in_menu=None)
        self.assertEqual(row['native_menu'], 'unavailable')
        self.assertIs(row['native_ui_available'], False)
        self.assertIsNone(row['native_in_menu'])
        for fields in (dict(native_ui_available=True), dict(native_ui_available=False), dict(native_in_menu=False),
                       dict(native_ui_available=1, native_in_menu=False),
                       dict(native_ui_available=True, native_in_menu=0),
                       dict(native_ui_available=False, native_in_menu=False)):
            row = parse(**fields)
            self.assertEqual(row['native_menu'], 'invalid')
            self.assertIsNone(row['native_ui_available'])
            self.assertIsNone(row['native_in_menu'])

    def test_latest_status_never_inherits_earlier_menu_or_readiness(self):
        closed = 'ARCADE FIXTURE: ' + json.dumps(fixture(native_ui_available=True, native_in_menu=False))
        legacy = 'ARCADE FIXTURE: ' + json.dumps(fixture())
        self.assertNotIn('native_menu', probe.parse_fixture_status(closed + '\n' + legacy))
        invalid = 'ARCADE FIXTURE: ' + json.dumps(fixture(native_ui_available=True, native_in_menu=None))
        self.assertEqual(probe.parse_fixture_status(closed + '\n' + invalid)['native_menu'], 'invalid')
        for bad in ('ARCADE FIXTURE: {broken', 'ARCADE FIXTURE: ' + json.dumps(fixture(state='invalid'))):
            self.assertIsNone(probe.parse_fixture_status(closed + '\n' + bad))

    def test_menu_at_baseline_stops_before_reset_or_mount(self):
        for available, menu, reason in ((True, True, 'fixture_native_menu_open'),
                                      (False, None, 'fixture_native_menu_unverified')):
            env = batch.DeckEnvironment(Mock())
            env.observe = Mock(side_effect=[dict(log_end=100), dict(fixture=parse(
                native_ui_available=available, native_in_menu=menu))])
            with patch.object(batch, 'fixture_console') as console, patch.object(env, 'fixture_reset') as reset, \
                    self.assertRaisesRegex(batch.HardStop, reason):
                env.fixture_baseline(1000)
            console.assert_called_once_with('status')
            reset.assert_not_called()
            env.session.pad.frame.assert_not_called()

    def test_menu_after_reset_stops_before_mount_chord(self):
        env = batch.DeckEnvironment(Mock())
        env.observe = Mock(side_effect=[dict(log_end=100), dict(log_end=200,
            fixture=parse(native_ui_available=True, native_in_menu=True))])
        with patch.object(batch, 'fixture_console') as console, patch.object(env, 'fixture_mount') as mount, \
                self.assertRaisesRegex(batch.HardStop, 'fixture_native_menu_open'):
            env.fixture_reset(1000)
        self.assertEqual([c.args for c in console.call_args_list], [('reset',), ('status',)])
        mount.assert_not_called()

    def test_direct_mount_guard_does_not_relax_physical_baseline(self):
        env = batch.DeckEnvironment(Mock())
        with self.assertRaisesRegex(batch.HardStop, 'fixture_native_menu_open'):
            env.fixture_mount(parse(native_ui_available=True, native_in_menu=True))
        with self.assertRaisesRegex(batch.HardStop, 'fixture_reset_unverified'):
            env.fixture_mount(parse(native_ui_available=True, native_in_menu=False, state=100))
        env.session.pad.frame.assert_not_called()
        batch.require_fixture_menu_closed(parse())  # legacy absence remains legacy, not UI proof

    def test_failure_receipt_identifies_missing_preinput_condition(self):
        case = dict(name='trial')
        row = batch.stopped_trial(batch.HardStop('fixture_native_menu_open'), case, 0)
        self.assertEqual(row['reason'], 'fixture_native_menu_open')
        self.assertEqual(row['stage'], 'fixture_baseline')
        self.assertNotIn('input_sent', row)
        self.assertEqual(row['game_response'], 'UNVERIFIED')

    def test_later_menu_failure_does_not_deny_prior_mount_input(self):
        env = batch.DeckEnvironment(Mock(pid=123))
        closed = parse(native_ui_available=True, native_in_menu=False)
        opened = parse(native_ui_available=True, native_in_menu=True)
        env.observe = Mock(side_effect=[dict(log_end=100), dict(log_end=200, fixture=closed),
                                       dict(log_end=300, fixture=opened)])
        with patch.object(batch, 'fixture_console'), patch.object(batch.time, 'sleep'), \
                patch.object(batch, 'capabilities', return_value=dict(ready=True, gamescope_pids=[123]), create=True):
            with self.assertRaisesRegex(batch.HardStop, 'fixture_native_menu_open') as caught:
                env.fixture_reset(1000)
        env.session.pad.frame.assert_called_once_with([(1, 0x136, 1), (1, 0x137, 1)])
        receipt = batch.stopped_trial(caught.exception, dict(name='trial'), 0)
        self.assertNotIn('input_sent', receipt)


if __name__ == '__main__': unittest.main()
