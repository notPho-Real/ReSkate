"""Offline opt-in trajectory handoff checks; no native acceptance claim."""
import copy
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch
sys.path.insert(0, str(Path(__file__).parent / 'lab'))
import trick_batch as batch
import plan_schema
from probe import parse_fixture_trajectory


def trajectory_row(seq):
    ms = 1000 + seq * 50
    return (f'ARCADE FIXTURE TRAJECTORY: session=1 seq={seq} ms={ms} sampled={ms} generation=2 revision=3 '
            'status=2 dropped=0 x_mm=0 y_mm=0 z_mm=0 speed_mm_s=0 heading_mdeg=0 state=100.')


def plan():
    return dict(schema=1, source_commit='a'*40, dll_sha256='b'*64, stock_sha256='c'*64,
        candidates=[dict(name='trajectory', family='grind', fixture_reset=True, trajectory_capture=True,
            timeout_ms=6000, reset_ms=3000, capture_frames=0, phases=[dict(ms=0)], expected=dict(states=[100]))])


def baseline(generation=10):
    return dict(log_end=200, combo=dict(samples=[dict(ms=1100)]), grind=dict(dropped=0,
        status=[dict(active=True, generation=generation, dropped=0, rejected=0)],
        samples=[dict(ms=1100, generation=generation, dropped=0, valid=True, state=100, edge=0, duration=0)],
        pad=[dict(generation=generation, dropped=0)]))


def captured():
    return dict(log_end=300, fixture_trajectory=dict(evidence_gap=False,
        status=dict(status='armed', dropped=0, session=1), samples=[dict(valid=True, session=1,
            generation=2, revision=3, state=100, speed_mm_s=0, sampled=1100)]))


class TrajectoryBatchTests(unittest.TestCase):
    def test_measured_trajectory_window_has_explicit_bounded_headroom(self):
        prefix = parse_fixture_trajectory(trajectory_row(1))['samples']
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'ReSkate.log'
            native = 'ARCADE HIL STATE: seq=1 valid=true state=100 mode=1.\n'
            start = native + trajectory_row(1) + '\n'
            end = trajectory_row(2) + '\n'
            path.write_text(start + 'x' * (301681 - len(start) - len(end) - 1) + '\n' + end)
            env = batch.DeckEnvironment(); env.log = path
            self.assertEqual(env.observe(since=0)['evidence_gap'], 'tail_limit')
            expanded = env.observe(since=0, trajectory=True)
            self.assertIsNone(expanded['evidence_gap'])
            self.assertEqual(expanded['log_end'], 301681)
            self.assertEqual(expanded['log_limit'], 524288)
            self.assertEqual(len(expanded['states']), 1)
            trajectory = batch.collect_fixture_trajectory(env, dict(log_start=0, session=1, prefix=prefix))
            self.assertFalse(trajectory['evidence_gap'])
            self.assertEqual(trajectory['prefix_status'], 'MATCHED')
            self.assertEqual(trajectory['sequence_missing'], 0)
            self.assertEqual(trajectory['collection']['log_start'], 0)
            self.assertIsNone(trajectory['transport_gap'])
            # Both loss boundaries remain explicit; no general limit increase.
            path.write_text(start + 'x' * 524288 + '\n' + end)
            self.assertEqual(env.observe(since=0, trajectory=True)['evidence_gap'], 'tail_limit')
            lost = batch.collect_fixture_trajectory(env, dict(log_start=0, session=1, prefix=prefix))
            self.assertTrue(lost['evidence_gap'])
            self.assertEqual(lost['transport_gap'], 'tail_limit')
            self.assertEqual(lost['prefix_status'], 'UNVERIFIED')
            self.assertEqual(env.observe(since=path.stat().st_size + 1, trajectory=True)['evidence_gap'], 'log_truncated')
            with self.assertRaises(ValueError): env.observe(since=0, trajectory=1)

    def test_trajectory_budget_cannot_hide_native_invalid_terminal(self):
        prefix = parse_fixture_trajectory(trajectory_row(1))['samples']
        terminal = ('ARCADE FIXTURE TRAJECTORY: session=1 seq=2 ms=1100 sampled=0 generation=2 revision=3 '
                    'status=4 dropped=0 x_mm=0 y_mm=0 z_mm=0 speed_mm_s=0 heading_mdeg=0 state=0.')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'ReSkate.log'
            path.write_text(trajectory_row(1) + '\n' + 'x' * 300000 + '\n' + terminal)
            env = batch.DeckEnvironment(); env.log = path
            result = batch.collect_fixture_trajectory(env, dict(log_start=0, session=1, prefix=prefix))
            self.assertEqual(result['prefix_status'], 'MATCHED')
            self.assertIsNone(result['transport_gap'])
            self.assertEqual(result['samples'][-1]['status'], 'invalid')
            self.assertTrue(result['evidence_gap'])

    def test_explicit_boolean_and_compatible_baseline_required(self):
        for validator in (batch.validate_plan, plan_schema.validate_plan):
            self.assertEqual(validator(plan()), plan())
            for change in (dict(trajectory_capture=1), dict(fixture_reset=False), dict(score_capture_ready=True)):
                value = plan(); value['candidates'][0].update(change)
                with self.assertRaises(ValueError): validator(value)
            value = plan(); value['candidates'][0].update(trajectory_capture=False, fixture_reset=False)
            validator(value)

    def test_arm_is_final_console_handoff_before_action(self):
        for enabled in (False, True):
            value = plan(); case = value['candidates'][0]; case['trajectory_capture'] = enabled
            env = Mock(); env.wait_idle.return_value = 1
            env.observe.return_value = baseline(); env.wait_grind_ready.return_value = baseline()
            env.arm_fixture_trajectory.return_value = (baseline(11), dict(session=1))
            env.gesture.side_effect = batch.HardStop('installation_integrity')
            with self.assertRaises(batch.HardStop): batch.perform_case(value, case, env)
            calls = [c[0] for c in env.mock_calls]
            if enabled:
                self.assertLess(calls.index('wait_grind_ready'), calls.index('arm_fixture_trajectory'))
                self.assertEqual(calls[calls.index('arm_fixture_trajectory')+1], 'gesture')
            else: env.arm_fixture_trajectory.assert_not_called()
            env.fixture_reset.assert_not_called()

    def setup_env(self, current=None, ready=None):
        env = batch.DeckEnvironment(Mock())
        env.observe = Mock(side_effect=[dict(log_end=100), current or captured()])
        env.wait_idle = Mock(return_value=1)
        env.wait_grind_ready = Mock(return_value=ready or baseline(11))
        return env

    def test_fresh_post_console_baseline_retained_with_original_timestamps(self):
        env = self.setup_env()
        fixture = dict(reset_ready=True, generation=2, revision=3)
        with patch.object(batch, 'fixture_console') as console, patch.object(batch.time, 'monotonic', return_value=1):
            ready, receipt = env.arm_fixture_trajectory(baseline(), fixture)
        console.assert_called_once_with('trajectory')
        env.wait_idle.assert_called_once_with(3000, max_speed_mm_s=100)
        env.wait_grind_ready.assert_called_once_with(3000)
        self.assertEqual(ready['grind']['samples'], baseline(11)['grind']['samples'])
        self.assertEqual(ready['log_end'], 300)
        self.assertEqual(receipt['sampled'], 1100)
        self.assertEqual(receipt['log_start'], 100)
        self.assertEqual(receipt['prefix'], captured()['fixture_trajectory']['samples'])
        self.assertEqual(receipt['scope'], 'relative_motion_diagnostic_only')
        env.session.pad.neutral.assert_called_once()

    def test_missing_failed_stale_wrong_fixture_and_loss_stop_before_input(self):
        fixture = dict(reset_ready=True, generation=2, revision=3)
        for fault in range(9):
            current, ready = captured(), baseline(11)
            d = current['fixture_trajectory']
            if fault == 0: d['status'] = None
            if fault == 1: d['evidence_gap'] = True
            if fault == 2: d['samples'] = []
            if fault == 3: d['samples'][0]['revision'] = 4
            if fault == 4: d['samples'][0]['sampled'] = 900
            if fault == 5: d['samples'][0]['state'] = 200
            if fault == 6: d['samples'][0]['speed_mm_s'] = 101
            if fault == 7: ready['grind']['status'][0]['dropped'] = 1
            if fault == 8: ready['grind']['status'][0]['generation'] = 9
            env = self.setup_env(current, ready)
            with self.subTest(fault=fault), patch.object(batch, 'fixture_console') as console, \
                    patch.object(batch.time, 'monotonic', return_value=1), self.assertRaises(batch.HardStop):
                env.arm_fixture_trajectory(baseline(), fixture)
            console.assert_called_once_with('trajectory')
            env.session.run.assert_not_called()

    def test_idle_timeout_and_expired_handoff_cannot_send_action(self):
        fixture = dict(reset_ready=True, generation=2, revision=3)
        for idle, clock in ((None, [1]), (1, [1, 4.1])):
            env = self.setup_env(); env.wait_idle.return_value = idle
            with patch.object(batch, 'fixture_console'), patch.object(batch.time, 'monotonic', side_effect=clock), \
                    self.assertRaisesRegex(batch.HardStop, 'trajectory_capture_unverified'):
                env.arm_fixture_trajectory(baseline(), fixture)
            env.wait_grind_ready.assert_not_called()
            env.session.run.assert_not_called()

    def test_preinput_failure_is_explicit_and_does_not_retry(self):
        value = plan(); env = Mock(); env.wait_idle.return_value = 1
        env.arm_fixture_trajectory.side_effect = batch.HardStop('trajectory_capture_unverified')
        result = batch.perform_batch(value, env)['trials'][0]
        self.assertFalse(result['input_sent'])
        self.assertEqual(result['stage'], 'trajectory_baseline')
        self.assertEqual(result['game_response'], 'UNVERIFIED')
        env.gesture.assert_not_called()
        env.fixture_reset.assert_not_called()

    def test_validated_prefix_removes_no_real_sequence_gaps(self):
        prefix = parse_fixture_trajectory('\n'.join(trajectory_row(i) for i in (1, 2)))['samples']
        receipt = dict(log_start=100, session=1, prefix=prefix)
        # Reading only the trial boundary would falsely claim seq1/2 were lost.
        self.assertTrue(parse_fixture_trajectory(trajectory_row(3))['evidence_gap'])
        for sequence, missing in (((1, 2, 3), 0), ((1, 2, 4), 1)):
            env = Mock()
            env.observe_trajectory.return_value = dict(fixture_trajectory=parse_fixture_trajectory(
                '\n'.join(trajectory_row(i) for i in sequence)))
            result = batch.collect_fixture_trajectory(env, receipt)
            self.assertEqual(result['prefix_status'], 'MATCHED')
            self.assertEqual(result['sequence_missing'], missing)
            self.assertEqual(result['evidence_gap'], bool(missing))
            env.observe_trajectory.assert_called_once_with(since=100)

    def test_changed_missing_prefix_or_bounded_read_gap_stays_unverified(self):
        prefix = parse_fixture_trajectory(trajectory_row(1))['samples']
        receipt = dict(log_start=100, session=1, prefix=prefix)
        full = parse_fixture_trajectory(trajectory_row(1) + '\n' + trajectory_row(2))
        for fault in ('changed', 'missing', 'tail', 'unavailable'):
            observed = dict(fixture_trajectory=copy.deepcopy(full))
            if fault == 'changed': observed['fixture_trajectory']['samples'][0]['x_mm'] = 1
            if fault == 'missing': observed['fixture_trajectory']['samples'] = []
            if fault == 'tail': observed['evidence_gap'] = 'tail_limit'
            if fault == 'unavailable': observed = {}
            env = Mock(); env.observe_trajectory.return_value = observed
            result = batch.collect_fixture_trajectory(env, receipt)
            self.assertTrue(result['evidence_gap'])
            self.assertEqual(result['owner_proof'], 'UNVERIFIED')

    def test_trial_keeps_other_streams_at_post_handoff_boundary(self):
        value = plan(); env = Mock(); env.wait_idle.side_effect = [1, 3]
        prefix = parse_fixture_trajectory(trajectory_row(1) + '\n' + trajectory_row(2))['samples']
        receipt = dict(log_start=100, session=1, prefix=prefix)
        env.observe.side_effect = [baseline(), dict(states=[dict(seq=2, valid=True, state=100)],
            combo=dict(samples=[dict(ms=1200, state=100)]), fixture_trajectory=parse_fixture_trajectory(trajectory_row(3)))]
        env.wait_grind_ready.return_value = baseline()
        env.arm_fixture_trajectory.return_value = (dict(baseline(), log_end=300), receipt)
        env.gesture.return_value = dict(input_sent=True)
        env.observe_trajectory.return_value = dict(fixture_trajectory=parse_fixture_trajectory(
            '\n'.join(trajectory_row(i) for i in (1, 2, 3))))
        result = batch.perform_case(value, value['candidates'][0], env)
        env.observe.assert_called_with(since=300, trajectory=True)
        env.observe_trajectory.assert_called_once_with(since=100)
        self.assertEqual(result['observations']['combo']['samples'], [dict(ms=1200, state=100)])
        self.assertFalse(result['observations']['fixture_trajectory']['evidence_gap'])


if __name__ == '__main__': unittest.main()
