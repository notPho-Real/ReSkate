"""Offline batch validation, transport and cleanup regressions. No real Deck."""
import ast
import copy
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent / "lab"))
import trick_batch as batch
import robot
import inspect_project
import plan_schema
import build_trusted_bundle


def plan():
    return {"schema": 1, "source_commit": "a" * 40, "dll_sha256": "b" * 64, "stock_sha256": "c" * 64,
            "candidates": [{"name": "kickflip", "family": "flip", "phases": [
                {"ms": 0}, {"ms": 1000, "ry": 32767}, {"ms": 1600, "rx": 28000, "ry": -28000},
                {"ms": 1850}], "expected": {"states": [100, 200, 100], "trick": "kickflip"},
                "timeout_ms": 6000, "reset_ms": 3000}]}


class BatchTests(unittest.TestCase):
    @staticmethod
    def partial_writer_error():
        error = RuntimeError('Skate exited; test stopped')
        error.partial_run = {'schema': 1, 'outcome': 'INTERRUPTED', 'game_response': 'UNVERIFIED',
            'input_sent': True, 'input_may_have_been_sent': True, 'neutralized': True,
            'neutralization': {'attempted': True, 'write_completed': True, 'elapsed_ms': 1.5},
            'delivery_timing': {'samples': [{'scheduled_ms': 0, 'write_ms': 0.1}],
                'dropped': 0, 'interrupted': True, 'interrupted_phase': 1,
                'scope': 'uinput_write_not_game_poll', 'elapsed_ms': 100},
            'kernel_pad': {'available': True, 'stage': 7, 'errno': 0, 'samples': [{'ns': 100, 'lt': 0, 'rt': 0}]},
            'sdl_pad': {'available': False, 'stage': 0, 'samples': []}}
        return error

    def test_interrupted_trial_preserves_readonly_evidence_without_reset_or_grade(self):
        value = plan()
        value['candidates'][0].update(repeat=2, fixture_reset=True)
        env = Mock()
        env.wait_idle.return_value = 1
        env.wait_grind_ready.return_value = {'log_end': 100, 'combo': {'samples': []}}
        observation = {'states': [{'seq': 2, 'valid': True, 'state': 200}],
                       'flip': {'samples': [{'id': 2, 'valid': True}], 'dropped': 0}}
        env.observe.side_effect = [{'log_end': 100}, observation]
        env.gesture.side_effect = self.partial_writer_error()
        result = batch.perform_batch(value, env)['trials']
        self.assertEqual(len(result), 1)
        self.assertTrue(result[0]['hard_stop'])
        partial = result[0]['partial_trial']
        self.assertEqual(partial['observations'], observation)
        self.assertEqual(partial['observation_status'], 'AVAILABLE')
        self.assertEqual(partial['reset_status'], 'NOT_ATTEMPTED')
        self.assertTrue(all(v == 'UNVERIFIED' for v in partial['criteria'].values()))
        self.assertTrue(partial['neutralized'])
        self.assertEqual(len(partial['delivery_timing']['samples']), 1)
        env.observe.assert_called_with(since=100)
        env.gesture.assert_called_once()
        env.wait_idle.assert_called_once()
        env.integrity.assert_called_once()
        env.fixture_reset.assert_not_called()

    def test_partial_read_failure_or_bound_never_replaces_original_stop(self):
        for observation, status in [(OSError('private diagnostic'), 'UNAVAILABLE'),
                                    ({'states': [0] * 400000}, 'BOUND_EXCEEDED')]:
            env = Mock()
            env.wait_idle.return_value = 1
            env.observe.side_effect = [{'log_end': 100}, observation]
            env.gesture.side_effect = self.partial_writer_error()
            with self.subTest(status=status):
                stopped = batch.perform_batch(plan(), env)['trials'][0]
                self.assertEqual(stopped['reason'], 'input_readiness_lost')
                self.assertEqual(stopped['partial_trial']['observations'], {})
                self.assertEqual(stopped['partial_trial']['observation_status'], status)
                self.assertNotIn('private diagnostic', json.dumps(stopped))
                env.gesture.assert_called_once()

    def test_partial_writer_envelope_rejects_raw_or_unbounded_data(self):
        for mutation in ('text', 'nan', 'rows', 'outcome', 'key'):
            error = self.partial_writer_error()
            value = error.partial_run
            if mutation == 'text': value['delivery_timing']['scope'] = '/private/path'
            elif mutation == 'nan': value['neutralization']['elapsed_ms'] = float('nan')
            elif mutation == 'rows': value['kernel_pad']['samples'] = [0] * 129
            elif mutation == 'outcome': value['outcome'] = 'OBSERVED'
            elif mutation == 'key': value['private'] = 1
            with self.subTest(mutation=mutation):
                self.assertIsNone(batch.bounded_partial_run(error))

    def test_normal_host_saves_interrupted_evidence_and_does_not_retry(self):
        value = plan()
        value['candidates'][0]['repeat'] = 2
        stopped = {'hard_stop': True, 'reason': 'input_readiness_lost', 'name': 'kickflip',
                   'repetition': 1, 'partial_trial': self.partial_writer_error().partial_run}
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp).resolve() / 'out'
            dispatch = Mock(return_value=stopped)
            summary = batch.run_batch(value, output, dispatch, lambda _: None)
            self.assertEqual(summary['stop'], 'input_readiness_lost')
            self.assertEqual(summary['trials'], [stopped])
            self.assertEqual(json.loads((output / 'summary.json').read_text())['trials'], [stopped])
            dispatch.assert_called_once()

    def test_phase_wait_is_allowlisted_in_both_plan_validators(self):
        good = plan()
        case = good['candidates'][0]
        case['capture_frames'] = 0
        case['phases'][1]['wait'] = {'condition': 'native-riding', 'timeout_ms': 1000, 'max_speed_mm_s': 2000}
        for validator in (batch.validate_plan, plan_schema.validate_plan):
            self.assertIs(validator(good), good)
            for mutation in ('predicate', 'code', 'negative', 'long', 'bool', 'speed', 'capture', 'many'):
                bad = copy.deepcopy(good)
                candidate = bad['candidates'][0]
                wait = candidate['phases'][1]['wait']
                if mutation == 'predicate': wait['condition'] = 'arbitrary-native-state'
                elif mutation == 'code': wait['eval'] = 'anything'
                elif mutation == 'negative': wait['timeout_ms'] = -1
                elif mutation == 'long': wait['timeout_ms'] = 3001
                elif mutation == 'bool': wait['timeout_ms'] = True
                elif mutation == 'speed': wait['max_speed_mm_s'] = 20001
                elif mutation == 'capture': candidate['capture_frames'] = 4
                elif mutation == 'many':
                    candidate['phases'][2]['wait'] = dict(wait)
                    candidate['phases'][3]['wait'] = dict(wait)
                with self.subTest(validator=validator.__module__, mutation=mutation), self.assertRaises(ValueError):
                    validator(bad)
        case['expected']['after_grind'] = True
        case['expected']['chain'] = {'poses': [2, 1], 'air_ids': [1]}
        for validator in (batch.validate_plan, plan_schema.validate_plan):
            self.assertIs(validator(good), good)

    @staticmethod
    def condition_observation(seq=1, ms=1000, state=100, mode=1, generation=4, speed=1):
        return {'states': [dict(seq=seq, valid=True, state=state, mode=mode)],
                'push': [dict(seq=seq, ms=ms, valid=True, speed=speed, contacts=0xc000)],
                'grind': {'samples': [dict(seq=seq, ms=ms, generation=generation, valid=True,
                           arcade=True, state=state, edge=0, dropped=0)],
                          'status': [dict(generation=generation, active=True, dropped=0, rejected=0)], 'dropped': 0},
                'combo': {'samples': [dict(ms=ms, state=state, mode=mode)], 'dropped': 0},
                'log_end': 1000 + seq, 'evidence_gap': None}

    def test_condition_requires_new_paired_heartbeat_and_correlated_owner(self):
        initial = self.condition_observation()
        owner = batch.phase_condition_owner(initial)
        now = [0.0]
        def observe(since=None):
            if since is None or now[0] < .25:
                return initial
            return self.condition_observation(2, 1250)
        result = batch.wait_native_riding({'condition': 'native-riding', 'timeout_ms': 500}, observe,
            owner, lambda: True, clock=lambda: now[0], sleep=lambda dt: now.__setitem__(0, now[0] + dt))
        self.assertEqual(result['status'], 'READY')
        self.assertGreaterEqual(now[0], .25)
        self.assertEqual(result['last_native']['heartbeat_seq'], 2)
        self.assertEqual(result['last_native']['generation'], 4)
        self.assertEqual(result['scope'], 'native_context_only_not_action_result')

    def test_condition_timeout_reports_native_cause_without_success(self):
        initial = self.condition_observation()
        for failure in ('air', 'mode', 'contacts', 'speed', 'contradiction', 'unpaired', 'old-owner-state'):
            now = [0.0]
            def observe(since=None):
                if since is None: return initial
                row = self.condition_observation(2, 1100, state=201 if failure == 'air' else 100,
                                                 mode=0 if failure == 'mode' else 1)
                if failure == 'contacts': row['push'][0]['contacts'] = 0x4000
                if failure == 'speed': row['push'][0]['speed'] = 3
                if failure == 'contradiction': row['combo']['samples'] = [dict(ms=1110, state=201, mode=2)]
                if failure == 'unpaired': row['push'][0]['seq'] = 1
                if failure == 'old-owner-state': row['grind']['samples'][0]['ms'] = 900
                return row
            with self.subTest(failure=failure):
                result = batch.wait_native_riding({'condition': 'native-riding', 'timeout_ms': 100, 'max_speed_mm_s': 2000},
                    observe, batch.phase_condition_owner(initial), lambda: True,
                    clock=lambda: now[0], sleep=lambda dt: now.__setitem__(0, now[0] + dt))
                self.assertEqual(result['status'], 'TIMEOUT')
                self.assertNotEqual(result['cause'], 'fresh_native_riding')
                self.assertAlmostEqual(now[0], .1)

    def test_condition_integrity_loss_and_native_stall_stop(self):
        initial = self.condition_observation()
        for failure in ('generation', 'gap', 'dropped', 'inactive', 'invalid-owner', 'native-clock', 'stall'):
            now = [0.0]
            def observe(since=None):
                if since is None: return initial
                row = self.condition_observation(2, 1100, state=201)
                if failure == 'generation': row['grind']['samples'][0]['generation'] += 1
                if failure == 'gap': row['evidence_gap'] = 'tail_limit'
                if failure == 'dropped': row['grind']['samples'][0]['dropped'] = 1
                if failure == 'inactive': row['grind']['status'][0]['active'] = False
                if failure == 'invalid-owner': row['grind']['samples'][0]['valid'] = False
                if failure == 'native-clock': row['push'][0]['ms'] = 999
                if failure == 'stall': return initial
                return row
            with self.subTest(failure=failure), self.assertRaises(batch.HardStop):
                batch.wait_native_riding({'condition': 'native-riding', 'timeout_ms': 1000}, observe,
                    batch.phase_condition_owner(initial), lambda: True,
                    clock=lambda: now[0], sleep=lambda dt: now.__setitem__(0, now[0] + dt))

    def test_conditioned_environment_forwards_waiter_and_keeps_fixed_path(self):
        case = plan()['candidates'][0]
        case['capture_frames'] = 0
        case['phases'][1]['wait'] = {'condition': 'native-riding', 'timeout_ms': 300}
        session = Mock()
        env = batch.DeckEnvironment(session)
        env.observe = Mock(side_effect=AssertionError('Full parsing must not run inside phase wait'))
        env.observe_condition = Mock(return_value=self.condition_observation())
        env.gesture(case)
        self.assertEqual(session.run.call_args.kwargs['phase_conditions'], {1: case['phases'][1]['wait']})
        self.assertTrue(callable(session.run.call_args.kwargs['condition_waiter']))
        with patch.object(batch, 'wait_native_riding', return_value={'status': 'READY'}) as wait:
            session.run.call_args.kwargs['condition_waiter'](case['phases'][1]['wait'], lambda: True)
            self.assertIs(wait.call_args.args[1], env.observe_condition)
        env.observe.assert_not_called()

    def test_condition_observation_matches_full_required_fields_and_window_guards(self):
        text = '\n'.join([
            'ARCADE HIL STATE: seq=7 valid=true state=100 mode=1.',
            'ARCADE PUSH OBSERVATION: seq=7 ms=1200 valid=true eligible=true speed=1.5 contacts=49152.',
            'ARCADE COMBO SAMPLE: ms=1200 state=100 mode=1 words=' + ','.join(['00000000'] * 8) + '.',
            'ARCADE GRIND STATE: seq=7 ms=1200 generation=4 valid=true state=100 edge=0 duration=0 grind_valid=false grind=0 skate_valid=false skate=0 speed_valid=true speed_mm_s=1500 arcade=true dropped=0.',
            'ARCADE GRIND STATUS: generation=4 active=true dropped=0 rejected=0.',
            'ARCADE MODE DEV: A press #1 seen on ReSkate client tick.', ''])
        required = {'states', 'push', 'combo', 'grind', 'log_end', 'evidence_gap'}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'ReSkate.log'
            path.write_text(text)
            env = batch.DeckEnvironment()
            env.log = path
            full = env.observe(since=0)
            with patch.object(batch, 'parse_flip_observations', side_effect=AssertionError('Unused parser')):
                limited = env.observe_condition(since=0)
            self.assertEqual(set(limited), required)
            self.assertEqual(limited, {key: full[key] for key in required})
            self.assertEqual(limited['states'][0]['seq'], 7)
            self.assertEqual(limited['grind']['samples'][0]['generation'], 4)
            self.assertEqual(limited['push'][0]['ms'], 1200)
            self.assertEqual(len(limited['combo']['samples']), 1)
            self.assertTrue(full['buttons'])
            path.write_text(text + 'x' * 270000)
            self.assertEqual(env.observe_condition(since=0)['evidence_gap'], 'tail_limit')
            path.write_text('')
            self.assertEqual(env.observe_condition(since=10)['evidence_gap'], 'log_truncated')

    def test_ordinary_condition_miss_keeps_batch_running_and_requires_reset(self):
        value = plan()
        value['candidates'][0].update(fixture_reset=True, repeat=2)
        missed = {'outcome': 'MISSED_CONDITION', 'input_sent': False, 'neutralized': True,
                  'delivery_timing': {'condition_failed': True}, 'fixture_reset': {'reset_ready': True},
                  'observations': {'grind_result': {'evidence': 'COMPLETE'}}}
        with patch.object(batch, 'perform_case', side_effect=[dict(missed), dict(missed)]):
            result = batch.perform_batch(value, Mock())
        self.assertEqual(len(result['trials']), 2)
        self.assertFalse(any(row.get('hard_stop') for row in result['trials']))
        bad = copy.deepcopy(missed)
        bad['fixture_reset']['reset_ready'] = False
        with patch.object(batch, 'perform_case', return_value=bad):
            result = batch.perform_batch(value, Mock())
        self.assertTrue(result['trials'][-1]['hard_stop'])

    def test_condition_checks_readiness_after_evidence_before_releasing(self):
        initial = self.condition_observation()
        count = [0]
        def observe(since=None):
            return initial if since is None else self.condition_observation(2, 1100)
        def alive():
            count[0] += 1
            return count[0] < 3
        with self.assertRaises(batch.HardStop):
            batch.wait_native_riding({'condition': 'native-riding', 'timeout_ms': 100}, observe,
                batch.phase_condition_owner(initial), alive, clock=lambda: 0, sleep=lambda dt: None)

    def test_revert_link_requires_independent_label_and_unbroken_continuation(self):
        def event(ms, pose=1, active=True, fatigue=.2):
            return dict(ms=ms, pose=pose, active=active, failed=False,
                        fatigue=fatigue, grind_fatigue=fatigue, manual_fatigue=0)
        obs = {'combo': {'baseline_available': True, 'dropped': 0,
                         'events': [event(1000), event(2000, 4)],
                         'revert_links': [dict(ms=1100, sequence=3, labels=16777216,
                                              accepted=True, remaining_ms=1100)]},
               'score_edges': {'samples': [dict(seq=3, labels=16777216, uncertainty=0,
                                                observed=1080, sampled=1090)], 'dropped': 0},
               'grind_result': {'evidence': 'COMPLETE'}}
        good = batch.native_revert_link_result(obs)
        self.assertEqual(good['native_revert'], 'PASS')
        self.assertEqual(good['revert_continuation'], 'PASS')
        self.assertEqual(good['revert_to_action_ms'], 900)
        for mutation in ('intent-only', 'missing-label', 'stale-label', 'future-label',
                         'uncertain-label', 'coalesced-label', 'duplicate-label', 'dropped',
                         'no-baseline', 'gap', 'incomplete'):
            with self.subTest(mutation=mutation):
                bad = copy.deepcopy(obs)
                combo, edges = bad['combo'], bad['score_edges']
                if mutation == 'intent-only': combo['revert_links'] = []
                elif mutation == 'missing-label': edges['samples'] = []
                elif mutation == 'stale-label': edges['samples'][0]['observed'] = 800
                elif mutation == 'future-label': edges['samples'][0]['sampled'] = 1110
                elif mutation == 'uncertain-label': edges['samples'][0]['uncertainty'] = 1
                elif mutation == 'coalesced-label': edges['samples'][0]['labels'] |= 4
                elif mutation == 'duplicate-label': edges['samples'] *= 2
                elif mutation == 'dropped': edges['dropped'] = 1
                elif mutation == 'no-baseline': combo['baseline_available'] = False
                elif mutation == 'gap': bad['evidence_gap'] = True
                else: bad['grind_result']['evidence'] = 'INCOMPLETE'
                self.assertNotEqual(batch.native_revert_link_result(bad)['native_revert'], 'PASS')
                self.assertNotEqual(batch.native_revert_link_result(bad)['revert_continuation'], 'PASS')
        for mutation in ('expired', 'banked', 'fatigue-reset', 'duplicate-accept',
                         'inactive-at-revert', 'overlong-grace', 'no-continuation'):
            with self.subTest(mutation=mutation):
                bad = copy.deepcopy(obs)
                combo = bad['combo']
                if mutation == 'expired': combo['events'][-1]['ms'] = 2201
                elif mutation == 'banked': combo['events'].insert(1, event(1500, active=False, fatigue=0))
                elif mutation == 'fatigue-reset': combo['events'][-1]['grind_fatigue'] = 0
                elif mutation == 'duplicate-accept': combo['revert_links'] *= 2
                elif mutation == 'inactive-at-revert': combo['events'][0]['active'] = False
                elif mutation == 'overlong-grace': combo['revert_links'][0]['remaining_ms'] = 1101
                else: combo['events'].pop()
                self.assertEqual(batch.native_revert_link_result(bad)['revert_continuation'], 'FAIL')

    def test_combo_chain_requires_one_lifecycle_and_native_ids_at_each_takeoff(self):
        expected = {'poses': [2, 3, 2, 4, 2, 1], 'air_ids': [1, 1, 1]}
        events = []
        for i, (ms, pose) in enumerate([(0, 2), (1000, 3), (1500, 2), (2300, 4),
                                        (2800, 1), (3200, 2), (4000, 1), (4450, 1)]):
            active = i < 7
            events.append(dict(ms=ms, pose=pose, active=active, failed=False,
                               fatigue=i * .1 if active else 0,
                               grind_fatigue=.05 if active and i >= 2 else 0,
                               manual_fatigue=.04 if active and i >= 4 else 0))
        observation = {'combo': {'events': events, 'dropped': 0, 'baseline_available': True},
                       'flip': {'samples': [dict(ms=ms, id=1, valid=True) for ms in (0, 1500, 3200)], 'dropped': 0},
                       'grind_result': {'evidence': 'COMPLETE'}}
        result = batch.combo_chain_result(expected, observation)
        self.assertTrue(all(result[k] == 'PASS' for k in ('continuous', 'native_air_ids', 'fatigue', 'lifecycle_grace')))
        self.assertEqual(result['score_banking'], 'UNVERIFIED')
        missing = copy.deepcopy(observation)
        missing['flip']['samples'].pop()
        self.assertEqual(batch.combo_chain_result(expected, missing)['native_air_ids'], 'UNVERIFIED')
        restarted = copy.deepcopy(observation)
        restarted['combo']['events'][4]['active'] = False
        self.assertEqual(batch.combo_chain_result(expected, restarted)['continuous'], 'FAIL')
        silent_restart = copy.deepcopy(observation)
        silent_restart['combo']['events'][4]['grind_fatigue'] = 0
        self.assertEqual(batch.combo_chain_result(expected, silent_restart)['continuous'], 'FAIL')
        bounce = copy.deepcopy(observation)
        bounce['combo']['events'][4]['ms'] = 2317
        self.assertEqual(batch.combo_chain_result(expected, bounce)['continuous'], 'FAIL')
        lost = copy.deepcopy(observation)
        lost['grind_result']['evidence'] = 'INCOMPLETE'
        self.assertEqual(batch.combo_chain_result(expected, lost)['continuous'], 'UNVERIFIED')
        early_bank = copy.deepcopy(observation)
        early_bank['combo']['events'][-1]['ms'] = 4100
        self.assertEqual(batch.combo_chain_result(expected, early_bank)['lifecycle_grace'], 'FAIL')
        value = plan()
        value['candidates'][0]['expected']['chain'] = expected
        batch.validate_plan(value)
        for bad in ({'poses': [True, 1], 'air_ids': []}, {'poses': [2, 1], 'air_ids': []},
                    {'poses': [2, 1], 'air_ids': [64]}, {'poses': [2, 1], 'air_ids': [1], 'grace_ms': [650, 350]}):
            value['candidates'][0]['expected']['chain'] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_fixture_baseline_reuses_only_fresh_confirmed_reset(self):
        env = batch.DeckEnvironment(Mock())
        ready = dict(reset_ready=True, present=True, failed=False, editor_open=False)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'fixture': ready}])
        with patch.object(batch, 'fixture_console') as console, patch.object(env, 'fixture_reset') as reset:
            self.assertEqual(env.fixture_baseline(1000), ready)
            console.assert_called_once_with('status')
            reset.assert_not_called()
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        env.observe = Mock(side_effect=[{'log_end': 100}, {'fixture': dict(ready, reset_ready=False)}])
        with patch.object(batch, 'fixture_console'), patch.object(env, 'fixture_reset', return_value=ready) as reset:
            self.assertEqual(env.fixture_baseline(1000), ready)
            reset.assert_called_once_with(1000)
        for observation in ({}, {'fixture': ready, 'evidence_gap': 'log_truncated'},
                            {'fixture': dict(ready, present=False)}, {'fixture': dict(ready, editor_open=True)}):
            env.observe = Mock(side_effect=[{'log_end': 100}, observation])
            with patch.object(batch, 'fixture_console'), patch.object(env, 'fixture_reset') as reset, \
                    self.assertRaises(batch.HardStop):
                env.fixture_baseline(1000)
            reset.assert_not_called()

    def test_fixture_trials_continue_misses_but_stop_lost_evidence_or_reset(self):
        value = plan()
        value['candidates'][0].update(fixture_reset=True, repeat=3)
        valid = dict(input_sent=True, fixture_reset={'reset_ready': True},
                     observations={'grind_result': {'evidence': 'COMPLETE', 'native_grind': 'FAIL'}})
        with patch.object(batch, 'perform_case', side_effect=lambda *_: copy.deepcopy(valid)) as run:
            self.assertEqual(len(batch.perform_batch(value, object())['trials']), 3)
            self.assertEqual(run.call_count, 3)
        for failed, reason in [(dict(valid, fixture_reset={'reset_ready': False}), 'fixture_reset_unverified'),
                               (dict(valid, observations={}), 'grind_evidence_incomplete')]:
            with patch.object(batch, 'perform_case', return_value=failed) as run:
                results = batch.perform_batch(value, object())['trials']
            self.assertEqual(run.call_count, 1)
            self.assertEqual(results[-1]['reason'], reason)
            self.assertIn('observations', results[0])

    def test_native_chain_evidence_loss_stops_after_preserving_failed_trial(self):
        value = plan()
        value['candidates'][0]['repeat'] = 3
        for loss in ({'flip': {'dropped': 16}}, {'combo': {'dropped': 1}},
                     {'evidence_gap': 'log_truncated'}):
            trial = dict(input_sent=True, neutralized=True, observations=loss)
            with patch.object(batch, 'perform_case', return_value=trial) as run:
                result = batch.perform_batch(value, object())['trials']
            self.assertEqual(run.call_count, 1)
            self.assertEqual(result[0]['observations'], loss)
            self.assertTrue(result[0]['neutralized'])
            self.assertEqual(result[-1]['reason'], 'chain_evidence_incomplete')
        with patch.object(batch, 'perform_case', return_value=dict(input_sent=True,
                observations={'flip': {'dropped': 0}, 'chain_result': {'continuous': 'FAIL'}})) as run:
            self.assertEqual(len(batch.perform_batch(value, object())['trials']), 3)

    def test_fixture_reset_neutralizes_and_requires_fresh_native_confirmation(self):
        session = Mock()
        env = batch.DeckEnvironment(session)
        status = dict(reset_ready=True, present=True, failed=False, editor_open=False)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'log_end': 200, 'fixture': status}])
        with patch.object(batch, 'fixture_console') as console:
            self.assertEqual(env.fixture_reset(1000), status)
        session.pad.neutral.assert_called_once()
        self.assertEqual([c.args for c in console.call_args_list], [('reset',), ('status',)])
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        env.observe = Mock(side_effect=[{'log_end': 200}, {'fixture': dict(status, reset_ready=False, present=False)}])
        with patch.object(batch, 'fixture_console'), self.assertRaises(batch.HardStop):
            env.fixture_reset(1000)

    def test_fixture_console_rejects_plan_commands_without_running_tools(self):
        with patch.object(robot.subprocess, 'run') as run:
            for command in ('place', 'save', 'reset; exit', 'status\nother'):
                with self.assertRaises(ValueError):
                    robot.fixture_console(command)
            run.assert_not_called()

    def test_fixture_native_remount_is_contextual_bounded_and_always_neutralized(self):
        status = dict(arcade=False, configured=True, present=True, busy=False, failed=False,
                      editor_open=False, skater=True, state=504, speed=0, heading=0,
                      position=[1, 2, 3], start=[1, 2, 3])
        session = Mock(pid=123)
        env = batch.DeckEnvironment(session)
        with patch.object(batch, 'capabilities', return_value=dict(ready=True, gamescope_pids=[123]), create=True), \
                patch.object(batch.time, 'sleep'):
            for arcade, keys in ((False, [0x134]), (True, [0x136, 0x137])):
                env.fixture_mount(dict(status, arcade=arcade))
                session.pad.frame.assert_called_with([(1, key, 1) for key in keys])
            for change in ({'state': 100}, {'position': [4, 2, 3]}, {'heading': 90},
                           {'arcade': None}, {'editor_open': True}, {'speed': 1}):
                session.pad.frame.reset_mock()
                with self.assertRaises(batch.HardStop):
                    env.fixture_mount(dict(status, **change))
                session.pad.frame.assert_not_called()
            session.pad.frame.side_effect = OSError('interrupted input')
            session.pad.neutral.reset_mock()
            with self.assertRaises(OSError):
                env.fixture_mount(status)
            session.pad.neutral.assert_called_once()

    def test_fixture_remount_waits_for_new_native_readiness_without_repeated_presses(self):
        session = Mock()
        env = batch.DeckEnvironment(session)
        foot = dict(reset_ready=False, present=True, failed=False, editor_open=False, state=504)
        riding = dict(foot, state=100, reset_ready=True)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'log_end': 200, 'fixture': foot},
                                       {'log_end': 300, 'fixture': foot}, {'log_end': 400, 'fixture': riding}])
        with patch.object(batch, 'fixture_console'), patch.object(batch.time, 'sleep'), \
                patch.object(env, 'fixture_mount') as mount:
            self.assertEqual(env.fixture_reset(1000), riding)
            mount.assert_called_once_with(foot)

    def test_fixture_readiness_accepts_coalesced_health_only_with_matching_fresh_rows(self):
        status = dict(generation=8, active=True, dropped=0, rejected=0)
        row = dict(generation=8, dropped=0, valid=True, state=100, edge=0, duration=0)
        fresh = dict(log_end=200, grind=dict(samples=[dict(row) for _ in range(3)],
                     pad=[dict(generation=8, dropped=0)], status=[], dropped=0))
        env = batch.DeckEnvironment(Mock())
        env.observe = Mock(side_effect=[dict(log_end=100, grind={'status': [status]}), fresh])
        self.assertEqual(env.wait_grind_ready(1000)['log_end'], 200)
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        for change in ({'generation': 9}, {'dropped': 1}, {'active': False}):
            old = dict(status, **change)
            env.observe = Mock(side_effect=[dict(log_end=100, grind={'status': [old]}),
                                           dict(fresh, grind=dict(fresh['grind'], status=[]))])
            with patch.object(batch.time, 'monotonic', side_effect=[0, 0, 2]), patch.object(batch.time, 'sleep'), \
                 self.assertRaises(batch.HardStop):
                env.wait_grind_ready(1000)

    def test_hard_stop_reports_only_allowlisted_cause_and_trial(self):
        value = plan()
        for message, reason in [("Skate exited; test stopped", "input_readiness_lost"),
                                ("No fresh Gamescope PNG/AVIF; capture interface unverified", "capture_timeout"),
                                ("private path and arbitrary output", "integrity_or_transport_failure")]:
            with patch.object(batch, "perform_case", side_effect=RuntimeError(message)):
                result = batch.perform_batch(value, object())["trials"]
            self.assertEqual(result, [dict(hard_stop=True, reason=reason, name="kickflip", repetition=1)])

    def test_readiness_failure_diagnostic_is_numeric_and_bounded(self):
        diagnostic = dict(age_ms=121, failed=False, context_ready=True,
                          owner_matches=True, capture_pending=True)
        for supplied, accepted in [(diagnostic, True), (dict(diagnostic, age_ms=100001), False),
                                   (dict(diagnostic, path='private'), False),
                                   (dict(diagnostic, failed='private'), False)]:
            error = RuntimeError('Skate exited; test stopped')
            error.readiness = supplied
            with patch.object(batch, 'perform_case', side_effect=error):
                stopped = batch.perform_batch(plan(), object())['trials'][0]
            self.assertEqual('readiness' in stopped, accepted)
            if accepted:
                self.assertEqual(stopped['readiness'], diagnostic)

    def test_stationary_baseline_requires_matching_fresh_speed(self):
        observation = {"states": [dict(seq=i, valid=True, state=100, mode=1) for i in range(1, 4)],
                       "push": [dict(seq=i, valid=True, speed=0.1, contacts=0xc000) for i in range(1, 4)]}
        self.assertEqual(batch.ready_baseline(observation, 0, 100), 3)
        self.assertIsNone(batch.ready_baseline(observation, 1, 100))
        for change in ({"speed": 2.38}, {"speed": float("nan")}, {"speed": -1},
                       {"seq": 0}, {"valid": False}, {"contacts": 0x4000}):
            bad = copy.deepcopy(observation)
            bad["push"][1].update(change)
            self.assertIsNone(batch.ready_baseline(bad, 0, 100))
            self.assertEqual(batch.ready_baseline(bad, 0), 3)
        observation.pop("push")
        self.assertIsNone(batch.ready_baseline(observation, 0, 100))

    def test_speed_bound_is_validated_and_applies_to_reset(self):
        value = plan()
        case = value["candidates"][0]
        for bound in (0, 100, 20000):
            case["baseline_max_speed_mm_s"] = bound
            batch.validate_plan(value)
        for bound in (-1, 20001, True, "100", 0.1):
            case["baseline_max_speed_mm_s"] = bound
            with self.assertRaises(ValueError):
                batch.validate_plan(value)
        case["baseline_max_speed_mm_s"] = 100
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.return_value = {"states": [], "buttons": []}
        result = batch.perform_case(value, case, env)
        self.assertEqual(result["criteria"]["returned_to_idle"], "FAIL")
        self.assertTrue(all(call.kwargs == {"max_speed_mm_s": 100} for call in env.wait_idle.call_args_list))
        env.reset_mock()
        env.wait_idle.side_effect = [None]
        result = batch.perform_case(value, case, env)
        self.assertEqual(result["outcome"], "SKIPPED_CONTEXT")
        env.gesture.assert_not_called()

    def test_fixture_shortens_only_pre_reset_idle_wait(self):
        value = plan()
        case = value['candidates'][0]
        case.update(fixture_reset=True, reset_ms=10000, baseline_max_speed_mm_s=100)
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {'input_sent': True, 'frames': []}
        env.observe.return_value = {'states': [], 'buttons': [], 'combo': {'samples': []}}
        env.wait_grind_ready.return_value = {'log_end': 100}
        env.fixture_reset.return_value = {'reset_ready': True}
        result = batch.perform_case(value, case, env)
        self.assertEqual([c.args for c in env.wait_idle.call_args_list], [(10000,), (2000,)])
        self.assertTrue(all(c.kwargs == {'max_speed_mm_s': 100} for c in env.wait_idle.call_args_list))
        env.fixture_baseline.assert_called_once_with(10000)
        env.wait_grind_ready.assert_called_once_with(10000)
        env.fixture_reset.assert_called_once_with(10000)
        self.assertEqual(result['criteria']['returned_to_idle'], 'FAIL')
        self.assertTrue(result['fixture_reset']['reset_ready'])

    def test_bounded_capture_policy_and_session_forwarding(self):
        value = plan()
        case = value["candidates"][0]
        for count in (0, 1, 4):
            case["capture_frames"] = count
            batch.validate_plan(value)
            session = Mock()
            batch.DeckEnvironment(session=session).gesture(case)
            self.assertEqual(session.run.call_args.kwargs["capture_frames"], count)
        for count in (2, 3, 5, True, "1"):
            case["capture_frames"] = count
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_failure_window_retains_more_than_old_tail_and_still_marks_overflow(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "ReSkate.log"
            marker = "ARCADE HIL STATE: seq=1 valid=true state=100 mode=1.\n"
            path.write_text(marker + "x" * 70000)
            env = batch.DeckEnvironment()
            env.log = path
            result = env.observe(since=0)
            self.assertEqual(len(result["states"]), 1)
            self.assertIsNone(result["evidence_gap"])
            path.write_text(marker + "x" * 270000)
            self.assertEqual(env.observe(since=0)["evidence_gap"], "tail_limit")

    def test_persistent_batch_reuses_environment_after_ordinary_failure(self):
        value = plan()
        value["candidates"][0]["repeat"] = 3
        environment = object()
        outcomes = [{"criteria": {"named_trick": "FAIL"}},
                    {"criteria": {"returned_to_idle": "FAIL"}},
                    {"criteria": {"named_trick": "PASS"}}]
        with patch.object(batch, "perform_case", side_effect=outcomes) as run:
            result = batch.perform_batch(value, environment)
        self.assertEqual(len(result["trials"]), 3)
        self.assertEqual([r["repetition"] for r in result["trials"]], [1, 2, 3])
        self.assertTrue(all(call.args[2] is environment for call in run.call_args_list))
        self.assertEqual(result["trials"][0]["criteria"]["named_trick"], "FAIL")

    def test_persistent_batch_stops_for_infrastructure_failure(self):
        value = plan()
        value["candidates"][0]["repeat"] = 3
        with patch.object(batch, "perform_case", side_effect=batch.HardStop("neutralization")) as run:
            result = batch.perform_batch(value, object())
        self.assertEqual(run.call_count, 1)
        self.assertTrue(result["trials"][0]["hard_stop"])

    def test_deck_environment_uses_existing_session(self):
        session = Mock()
        case = plan()["candidates"][0]
        environment = batch.DeckEnvironment(session=session)
        environment.gesture(case)
        session.run.assert_called_once_with(case["name"], batch.phase_events(case), case["timeout_ms"] / 1000, capture_frames=4)

    def test_trusted_batch_dispatch_uses_one_context_for_all_trials(self):
        spec = importlib.util.spec_from_file_location("batch_runner_test", Path(batch.__file__).with_name("trusted_runner.py"))
        runner = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(runner)
        value = plan()
        value["candidates"][0]["repeat"] = 3
        pins = {k: value[k] for k in ("source_commit", "dll_sha256", "stock_sha256")}
        request = {"operation": "batch", "plan": value, "pins": pins, "game_sha256": "d" * 64}
        session = Mock()
        session.__enter__ = Mock(return_value=session)
        session.__exit__ = Mock(return_value=False)
        factory = Mock(return_value=session)
        namespace = {"json": json, "sys": sys, "Path": Path, "HardStop": batch.HardStop,
                     "snapshot": lambda _: {"safe_paths": True, "game": "/fixture",
                         "installed_commit": pins["source_commit"], "dll_sha256": pins["dll_sha256"],
                         "backup_sha256": pins["stock_sha256"]},
                     "sha": lambda _: "d" * 64, "validate_plan": batch.validate_plan,
                     "ExperimentSession": factory, "DeckEnvironment": lambda session: session,
                     "perform_batch": batch.perform_batch}
        output = io.StringIO()
        with patch.object(sys, "stdin", io.StringIO(json.dumps(request))), contextlib.redirect_stdout(output), \
             patch.object(batch, "perform_case", side_effect=lambda *_: {"criteria": {"named_trick": "FAIL"}}) as run:
            exec(compile(runner.REMOTE, "trusted-batch-dispatch", "exec"), namespace)
        factory.assert_called_once_with(max_seconds=1100)
        session.__enter__.assert_called_once()
        session.__exit__.assert_called_once()
        self.assertEqual(run.call_count, 3)
        self.assertTrue(all(call.args[2] is session for call in run.call_args_list))
        self.assertEqual(len(json.loads(output.getvalue())["trials"]), 3)

    def test_inspections_have_no_forwarded_arguments(self):
        self.assertIn("notPho-Real/ReSkate", inspect_project.command("runs"))
        self.assertIn("arcade-mode", inspect_project.command("runs"))
        self.assertEqual(inspect_project.command("lab-status")[-1], "status")
        with self.assertRaises(KeyError):
            inspect_project.command("status; touch /tmp/owned")

    def test_screenshot_count_and_hash_checked(self):
        with tempfile.TemporaryDirectory() as temp:
            for frames in ([], [{}] * 5, [{"image_base64": "AA==", "sha256": "0" * 64,
                                        "format": "png"}] * 4):
                with self.assertRaises(ValueError):
                    robot.save_run({"schema": 1, "recipe": "test", "frames": frames},
                                   Path(temp).resolve() / "out")

    def test_cleanup_success_and_failure(self):
        for failure in (None, RuntimeError("failed replay")):
            pad = Mock()
            pad.__enter__ = Mock(return_value=pad)
            pad.__exit__ = Mock(return_value=False)
            with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [1]}), \
                 patch.object(robot, "Gamepad", return_value=pad), patch.object(robot.time, "sleep"), \
                 patch.object(robot, "screenshot"), patch.object(robot, "periodic", return_value=[]), \
                 patch.object(robot, "replay", side_effect=failure), \
                 patch.object(robot.signal, "signal"), patch.object(robot.signal, "alarm"):
                if failure:
                    with self.assertRaises(RuntimeError):
                        robot.experiment_events("test", [], 3)
                else:
                    self.assertTrue(robot.experiment_events("test", [], 3)["input_sent"])
            pad.neutral.assert_called()
            pad.__exit__.assert_called_once()

    def test_valid_and_full_neutral_phases(self):
        data = batch.validate_plan(plan())
        events = batch.phase_events(data["candidates"][0])
        self.assertTrue(all(event[2] == 0 for event in events[-1][1]))
        self.assertEqual(len(events[-1][1]), 14)

    def test_unknown_fields_reject_code(self):
        for field in ("command", "shell", "code", "host", "output", "restore"):
            value = plan()
            value["candidates"][0][field] = "touch /tmp/owned"
            with self.subTest(field=field), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_labels_cannot_inject(self):
        for label in ("$(id)", "';import os", "../escape", "a\nb", "x;sudo", "x`id`"):
            value = plan()
            value["candidates"][0]["name"] = label
            with self.subTest(label=label), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_numeric_trick_names_keep_the_same_data_only_boundary(self):
        value = plan()
        value["candidates"][0]["expected"]["trick"] = "360-flip"
        self.assertEqual(batch.validate_plan(value), value)
        for bad in ("360;id", "360$(id)", "360/escape", "360\nflip", "", "3" * 49):
            value["candidates"][0]["expected"]["trick"] = bad
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_phase_limits(self):
        for key, bad in (("ms", -1), ("ms", True), ("rx", 32768), ("ry", float("nan")),
                         ("lt", -1), ("dx", 2), ("buttons", ["START"]), ("buttons", "A")):
            value = plan()
            value["candidates"][0]["phases"][0][key] = bad
            with self.subTest(key=key, bad=bad), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_batch_and_time_bounds(self):
        for field, bad in (("repeat", 21), ("timeout_ms", 12001), ("reset_ms", 10001)):
            value = plan()
            value["candidates"][0][field] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)
        value = plan()
        value["candidates"] *= 25
        with self.assertRaises(ValueError):
            batch.validate_plan(value)

    def test_duplicate_and_nonmonotonic(self):
        value = plan()
        value["candidates"][0]["phases"][1]["ms"] = 0
        with self.assertRaises(ValueError):
            batch.validate_plan(value)

    def test_remote_payload_uses_stdin_not_code(self):
        value = plan()
        source = batch.remote_payload(value, value["candidates"][0])
        compile(source, "remote-batch", "exec")
        self.assertIn("request=json.load(sys.stdin)", source)
        self.assertNotIn("change_installation(", source)
        self.assertNotIn("publish_report(", source)
        self.assertNotIn(value["source_commit"], source)

    def test_remote_code_is_frozen_for_batch_process(self):
        value = plan()
        with patch.object(Path, "read_text", side_effect=AssertionError("mid-batch read")):
            source = batch.remote_payload(value, value["candidates"][0])
        compile(source, "frozen-remote-batch", "exec")

    def test_observations_sanitize_and_bound(self):
        text = "secret password and personal path\n" + "\n".join(
            "ARCADE HIL STATE: seq=%d valid=true state=100 mode=1." % i for i in range(200))
        value = batch.observations(text)
        self.assertEqual(len(value["states"]), 128)
        self.assertNotIn("secret", json.dumps(value))

    def test_wrong_trick_not_promoted_and_reset_failure_continues(self):
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.return_value = {"states": [{"seq": 2, "valid": True, "state": 100, "mode": 1}], "buttons": []}
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["criteria"]["native_state_sequence"], "UNVERIFIED")
        self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")
        self.assertEqual(result["criteria"]["returned_to_idle"], "FAIL")
        self.assertEqual(env.integrity.call_count, 2)

    def test_exit_identity_cannot_be_satisfied_by_the_approach_ollie(self):
        observed = {'flip': {'samples': [{'ms': 12, 'valid': True, 'id': 1}], 'dropped': 0},
                    'grind_result': {'evidence': 'COMPLETE', 'native_grind': 'PASS'},
                    'grind': {'samples': [{'ms': 20, 'valid': True, 'edge': 1},
                                          {'ms': 30, 'valid': True, 'edge': 2}]}}
        self.assertEqual(batch.named_flip_result('ollie', observed, 100, True), 'UNVERIFIED')
        observed['flip']['samples'].append({'ms': 28, 'valid': True, 'id': 2})
        self.assertEqual(batch.named_flip_result('ollie', observed, 100, True), 'FAIL')
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'PASS')
        observed['grind']['samples'].append({'ms': 40, 'valid': True, 'edge': 1})
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'UNVERIFIED')
        observed['grind_result']['evidence'] = 'INCOMPLETE'
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'UNVERIFIED')
        value = plan()
        value['candidates'][0]['expected']['after_grind'] = True
        batch.validate_plan(value)
        for bad in (1, 'true', None):
            value['candidates'][0]['expected']['after_grind'] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_named_flip_requires_exact_fresh_independent_identifier(self):
        observed = {"flip": {"samples": [{"ms": 12, "valid": True, "id": 2}], "dropped": 0}}
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("heelflip", observed, 100), "FAIL")
        self.assertEqual(batch.named_flip_result("double-kickflip", observed, 100), "UNVERIFIED")
        self.assertEqual(batch.named_flip_result("kickflip", observed, None), "UNVERIFIED")
        observed["evidence_gap"] = "tail_limit"
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")
        observed.pop("evidence_gap")
        observed["flip"]["dropped"] = 1
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")
        observed["flip"]["dropped"] = 0
        observed["flip"]["samples"][0]["valid"] = False
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")

    def test_impossible_requires_its_current_game_identifier_and_context(self):
        observed = {"flip": {"samples": [{"ms": 12, "valid": True, "id": 36}], "dropped": 0}}
        self.assertEqual(batch.named_flip_result("impossible", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("nollie-impossible", observed, 100), "FAIL")
        observed["flip"]["samples"][0]["id"] = 38
        self.assertEqual(batch.named_flip_result("nollie-impossible", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("double-impossible", observed, 100), "UNVERIFIED")

    def test_combo_capture_is_bounded_and_not_named_from_state(self):
        text = "private unrelated log\n" + "\n".join(
            "ARCADE COMBO SAMPLE: ms=%d state=100 mode=1 words=%s." %
            (i, ",".join(["00000000"] * 8)) for i in range(200))
        value = batch.observations(text)
        self.assertEqual(len(value["combo"]["samples"]), 128)
        self.assertEqual(value["combo"]["native_event_identity"], "UNVERIFIED")
        self.assertNotIn("private", json.dumps(value))

    def test_combo_capture_excludes_previous_candidate(self):
        env = Mock()
        env.wait_idle.side_effect = [1, 3]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.side_effect = [
            {"combo": {"samples": [{"ms": 100}]}},
            {"states": [], "combo": {"samples": [{"ms": 99, "state": 100}, {"ms": 100, "state": 100}, {"ms": 101, "state": 100}]}}]
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["observations"]["combo"]["samples"], [{"ms": 101, "state": 100}])
        self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")

    def test_physics_sequence_can_prove_pop_missed_by_heartbeat(self):
        for dropped in (0, 1):
            env = Mock()
            env.wait_idle.side_effect = [1, 3]
            env.gesture.return_value = {"input_sent": True, "frames": []}
            env.observe.side_effect = [
                {"log_end": 100},
                {"states": [], "combo": {"dropped": dropped, "samples": [
                    {"ms": i, "state": state} for i, state in enumerate((100, 200, 100))]}}]
            value = plan()
            result = batch.perform_case(value, value["candidates"][0], env)
            self.assertEqual(result["criteria"]["native_state_sequence"], "PASS" if dropped == 0 else "UNVERIFIED")
            self.assertEqual(result["criteria"]["heartbeat_state_sequence"], "UNVERIFIED")
            self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")

    def test_quiet_baseline_captures_new_physics_rows_by_byte_boundary(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "ReSkate.log"
            path.write_text("quiet heartbeat\n")
            env = batch.DeckEnvironment()
            env.log = path
            boundary = env.observe()["log_end"]
            with path.open("a") as stream:
                stream.write("ARCADE COMBO SAMPLE: ms=10 state=100 mode=1 words=" +
                             ",".join(["00000000"] * 8) + ".\n")
            result = env.observe(since=boundary)
            self.assertEqual(len(result["combo"]["samples"]), 1)
            self.assertIsNone(result["evidence_gap"])
            path.write_text("")
            self.assertEqual(env.observe(since=boundary)["evidence_gap"], "log_truncated")

    def test_invalid_context_sends_no_input(self):
        env = Mock()
        env.wait_idle.return_value = None
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["outcome"], "SKIPPED_CONTEXT")
        env.gesture.assert_not_called()

    def run_fake(self, dispatch, check=lambda p: None):
        value = plan()
        value["candidates"][0]["repeat"] = 2
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(robot, "save_run"):
                return batch.run_batch(value, Path(temp).resolve() / "out", dispatch, check)

    def test_failed_candidate_continues(self):
        dispatch = Mock(return_value={"input_sent": True, "neutralized": True, "outcome": "OBSERVED",
            "criteria": {"native_state_sequence": "FAIL"}, "frames": []})
        result = self.run_fake(dispatch)
        self.assertEqual(len(result["trials"]), 2)
        self.assertEqual(result["stop"], "candidates_exhausted")

    def test_host_preserves_phase_zero_timeout_and_continues(self):
        value = plan()
        case = value['candidates'][0]
        case.update(capture_frames=0, repeat=2)
        case['phases'][0]['wait'] = {'condition': 'native-riding', 'timeout_ms': 100}
        timing = {'samples': [], 'condition_failed': True, 'conditions': [
            {'phase': 0, 'status': 'TIMEOUT', 'cause': 'native_not_riding',
             'started_ms': 0, 'completed_ms': 100, 'wait_ms': 100, 'shift_ms': 100,
             'last_native': {'state': 201, 'heartbeat_seq': 7, 'generation': 4}}]}
        response = {'schema': 1, 'recipe': case['name'], 'input_sent': False, 'neutralized': True,
                    'outcome': 'MISSED_CONDITION', 'delivery_timing': timing, 'frames': [],
                    'criteria': {'named_trick': 'UNVERIFIED'}, 'observations': {'after': {'state': 201}}}
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp).resolve() / 'out'
            dispatch = Mock(side_effect=lambda *_: copy.deepcopy(response))
            summary = batch.run_batch(value, output, dispatch, lambda _: None)
            self.assertEqual(summary['stop'], 'candidates_exhausted')
            self.assertEqual(dispatch.call_count, 2)
            for index, trial in enumerate(summary['trials']):
                self.assertEqual(trial['outcome'], 'MISSED_CONDITION')
                self.assertFalse(trial['input_sent'])
                self.assertTrue(trial['neutralized'])
                self.assertEqual(trial['delivery_timing'], timing)
                self.assertEqual(trial['criteria']['named_trick'], 'UNVERIFIED')
                self.assertEqual(json.loads((output / ('trial-%03d' % index) / 'run.json').read_text()), response)

    def test_host_refuses_unconfirmed_or_malformed_missed_condition(self):
        for failure, expected in [('neutral', 'neutralization_unconfirmed'),
                                  ('marker', 'invalid_response'), ('condition', 'invalid_response')]:
            value = plan()
            case = value['candidates'][0]
            case.update(capture_frames=0, repeat=2)
            case['phases'][0]['wait'] = {'condition': 'native-riding', 'timeout_ms': 100}
            response = {'input_sent': False, 'neutralized': True, 'outcome': 'MISSED_CONDITION',
                        'delivery_timing': {'samples': [], 'condition_failed': True,
                                            'conditions': [{'phase': 0, 'status': 'TIMEOUT'}]}}
            if failure == 'neutral': response['neutralized'] = False
            elif failure == 'marker': response['delivery_timing']['condition_failed'] = False
            elif failure == 'condition': response['delivery_timing']['conditions'] = []
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as temp, patch.object(robot, 'save_run') as save:
                dispatch = Mock(return_value=response)
                result = batch.run_batch(value, Path(temp).resolve() / 'out', dispatch, lambda _: None)
                self.assertEqual(result['stop'], expected)
                self.assertEqual(dispatch.call_count, 1)
                self.assertEqual(result['trials'], [])
                save.assert_not_called()

    def test_remote_and_reviewable_bundle_include_condition_dependencies(self):
        required = {'validate_phase_wait', 'validate_phase_conditions', 'phase_condition_owner',
                    'check_phase_condition_integrity', 'wait_native_riding', 'condition_observations',
                    'bounded_partial_run', 'stopped_trial'}
        value = plan()
        payload = batch.remote_payload(value, value['candidates'][0])
        names = lambda source: {node.name for node in ast.parse(source).body
                                if isinstance(node, (ast.FunctionDef, ast.ClassDef))}
        self.assertTrue(required <= names(payload))
        (batch.ROOT / 'artifacts').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=batch.ROOT / 'artifacts') as temp:
            output = Path(temp) / 'bundle'
            with contextlib.redirect_stdout(io.StringIO()):
                build_trusted_bundle.build(output)
            sources = [(output / name).read_text() for name in
                       ('worker_robot.py', 'worker_probe.py', 'plan_schema.py', 'worker_batch.py')]
            self.assertTrue(required <= names('\n'.join(sources)))

    def test_integrity_failure_stops_before_input(self):
        dispatch = Mock()
        result = self.run_fake(dispatch, Mock(side_effect=batch.HardStop("wrong_hash")))
        dispatch.assert_not_called()
        self.assertEqual(result["stop"], "wrong_hash")

    def test_unconfirmed_neutralization_stops(self):
        dispatch = Mock(return_value={"input_sent": True, "outcome": "OBSERVED"})
        self.assertEqual(self.run_fake(dispatch)["stop"], "neutralization_unconfirmed")
        self.assertEqual(dispatch.call_count, 1)

    def test_lost_ack_never_replayed(self):
        dispatch = Mock(side_effect=subprocess.TimeoutExpired("ssh", 55))
        self.assertEqual(self.run_fake(dispatch)["stop"], "transport_or_evidence_integrity_unconfirmed")
        self.assertEqual(dispatch.call_count, 1)

    def test_interrupt_preserves_summary(self):
        result = self.run_fake(Mock(side_effect=KeyboardInterrupt))
        self.assertEqual(result["stop"], "transport_or_evidence_integrity_unconfirmed")

    def test_preflight_retries_reads_only(self):
        lab = Mock()
        lab.remote.side_effect = OSError("offline")
        with self.assertRaises(batch.HardStop):
            batch.preflight(plan(), lab, sleep=lambda _: None)
        self.assertEqual(lab.remote.call_count, 3)
        self.assertTrue(all(c.args == ("status",) for c in lab.remote.call_args_list))

    def test_wrong_hash_and_backup_refuse(self):
        value = plan()
        status = dict(safe_paths=True, active=True, updates=False, recovery_pending=False,
                      installed_commit=value["source_commit"], dll_sha256=value["dll_sha256"], backup_sha256=value["stock_sha256"])
        for key in ("dll_sha256", "backup_sha256", "installed_commit"):
            lab = Mock()
            lab.remote.return_value = dict(status, **{key: "0" * 64})
            with self.assertRaises(batch.HardStop):
                batch.preflight(value, lab)

    def test_no_restore_path(self):
        source = Path(batch.__file__).read_text()
        self.assertNotIn('remote("restore"', source)
        self.assertNotIn("change_installation", source)

    def test_scoped_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp).resolve()
            with self.assertRaises(ValueError):
                batch.scoped(root / "../escape", root, False)
            (root / "link").symlink_to(root.parent)
            with self.assertRaises((ValueError, RuntimeError)):
                batch.scoped(root / "link/escape", root, False)

    def test_batch_cleanup_uses_robot_finally_on_interrupt(self):
        pad = Mock()
        pad.__enter__ = Mock(return_value=pad)
        pad.__exit__ = Mock(return_value=False)
        with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [1]}), \
             patch.object(robot, "Gamepad", return_value=pad), patch.object(robot.time, "sleep"), \
             patch.object(robot, "screenshot"), patch.object(robot, "periodic", return_value=[]), \
             patch.object(robot, "replay", side_effect=KeyboardInterrupt), \
             patch.object(robot.signal, "signal"), patch.object(robot.signal, "alarm"):
            with self.assertRaises(KeyboardInterrupt):
                robot.experiment_events("test", [], 3)
        pad.neutral.assert_called()
        pad.__exit__.assert_called_once()


class ScoreCaptureReadyTests(unittest.TestCase):
    @staticmethod
    def observation(seq=1, ms=1000, health=False):
        value = BatchTests.condition_observation(seq, ms, speed=0)
        value['grind']['samples'][0]['duration'] = 0
        value['grind']['pad'] = [dict(generation=4, dropped=0)]
        value['native_score'] = dict(health_dropped=0, health_malformed=0, health=[])
        if health:
            value['native_score']['health'] = [dict(ms=ms, epoch=9, active=True, bound=True,
                route=route, unbound=0, expired=0, changed=0, overflow=False) for route in range(1, 5)]
        return value

    def run_wait(self, mutate=None, *, delay=0, timeout=2000, read_delay=0, baseline=None):
        now, reads = [0.0], []
        initial = baseline or self.observation()
        def observe(since=None):
            seq = len(reads) + 2
            value = self.observation(seq, 1001 + round(now[0] * 1000), now[0] >= delay)
            # log offset is independent of native timestamp and advances per read.
            value['log_end'] = initial['log_end'] + seq
            reads.append(since)
            if mutate:
                mutate(value, now[0])
            now[0] += read_delay
            return value
        result = batch.wait_score_capture_ready(timeout, initial, observe, lambda: True, 100,
            clock=lambda: now[0], sleep=lambda dt: now.__setitem__(0, now[0] + dt))
        return result, now[0], reads

    def test_opt_in_boolean_schema_is_identical_and_defaults_unchanged(self):
        for validator in (batch.validate_plan, plan_schema.validate_plan):
            value = plan(); self.assertIs(validator(value), value)
            value['candidates'][0]['score_capture_ready'] = False
            self.assertIs(validator(value), value)
            value['candidates'][0].update(fixture_reset=True, score_capture_ready=True)
            self.assertIs(validator(value), value)
            for bad in (1, 'true', None, {}):
                value['candidates'][0]['score_capture_ready'] = bad
                with self.assertRaises(ValueError): validator(value)
            value['candidates'][0].update(score_capture_ready=True, fixture_reset=False)
            with self.assertRaises(ValueError): validator(value)

    def test_fresh_four_routes_record_readiness_without_scoring_authority(self):
        (observed, result), _, reads = self.run_wait()
        self.assertEqual(reads[0], 1001)
        self.assertEqual(result['status'], 'READY')
        self.assertEqual(result['scope'], 'observer_binding_only')
        self.assertEqual(len(result['health']), 4)
        self.assertFalse(result['owner_verified'])
        self.assertEqual(result['capture_completeness'], 'UNVERIFIED')
        self.assertGreater(observed['log_end'], reads[0])

    def test_five_second_health_cadence_accepts_fresh_riding_updates(self):
        (_, result), elapsed, reads = self.run_wait(delay=5, timeout=10000)
        self.assertGreaterEqual(elapsed, 5)
        self.assertLess(elapsed, 5.2)
        self.assertEqual(result['status'], 'READY')
        self.assertEqual(reads, sorted(set(reads)))

    def test_reset_skips_become_baseline_after_binding_without_input(self):
        def mutate(value, now):
            for row in value['native_score']['health']:
                row.update(bound=now >= .1, unbound=7 + round(now * 10))
        (_, result), elapsed, _ = self.run_wait(mutate)
        self.assertGreaterEqual(elapsed, .1)
        self.assertTrue(all(row['unbound'] >= 8 for row in result['health']))

    def test_missing_stale_incomplete_or_wrong_context_snapshots_timeout(self):
        for failure in ('missing', 'stale', 'three_routes', 'unbound', 'not_riding', 'speed', 'old_log', 'old_physics'):
            def mutate(value, now):
                health = value['native_score']['health']
                if failure == 'missing': health.clear()
                elif failure == 'stale':
                    for row in health: row['ms'] = 1000
                elif failure == 'three_routes': health.pop()
                elif failure == 'unbound':
                    for row in health: row['bound'] = False
                elif failure == 'not_riding': value['states'][-1]['state'] = 200
                elif failure == 'speed': value['push'][-1]['speed'] = 1
                elif failure == 'old_log': value['log_end'] = 1001
                elif failure == 'old_physics':
                    for row in health: row['ms'] += 1000
            with self.subTest(failure=failure), self.assertRaisesRegex(batch.HardStop, 'score_capture_baseline_timeout'):
                self.run_wait(mutate)

    def test_owner_loss_malformed_health_and_overflow_stop_before_input(self):
        for failure in ('owner', 'grind_loss', 'health_loss', 'malformed', 'overflow', 'epoch', 'duplicate', 'health_bool'):
            def mutate(value, now):
                health = value['native_score']['health']
                if failure == 'owner': value['grind']['samples'][0]['generation'] += 1
                elif failure == 'grind_loss': value['grind']['dropped'] = 1
                elif failure == 'health_loss': value['native_score']['health_dropped'] = 1
                elif failure == 'malformed': value['native_score']['health_malformed'] = 1
                elif failure == 'overflow': health[0]['overflow'] = True
                elif failure == 'epoch': health[-1]['epoch'] += 1
                elif failure == 'duplicate': health[-1]['route'] = 1
                elif failure == 'health_bool': health[0]['unbound'] = False
            with self.subTest(failure=failure), self.assertRaisesRegex(batch.HardStop, 'score_capture_evidence_lost'):
                self.run_wait(mutate)

    def test_slow_observation_cannot_supply_current_snapshot(self):
        with self.assertRaisesRegex(batch.HardStop, 'score_capture_baseline_timeout'):
            self.run_wait(read_delay=.2)

    def test_routes_from_different_snapshot_times_cannot_be_combined(self):
        def mutate(value, now):
            for row in value['native_score']['health']:
                row['ms'] += row['route']
        with self.assertRaisesRegex(batch.HardStop, 'score_capture_baseline_timeout'):
            self.run_wait(mutate)

    def test_backward_host_clock_fails_closed(self):
        initial = self.observation()
        clocks = iter([1.0, 0.5])
        with self.assertRaisesRegex(batch.HardStop, 'score_capture_evidence_lost'):
            batch.wait_score_capture_ready(2000, initial, Mock(), lambda: True,
                                           clock=lambda: next(clocks), sleep=lambda _: None)

    def test_coalesced_grind_status_keeps_validated_owner(self):
        def mutate(value, now): value['grind']['status'] = []
        (observed, _), _, _ = self.run_wait(mutate)
        self.assertEqual(observed['grind']['status'][0]['generation'], 4)

    def test_health_only_slice_retains_exact_validated_native_baseline_rows(self):
        retained = self.observation(2, 1001)
        def mutate(value, now):
            if now == 0:
                value['native_score']['health'] = []
            else:
                value['grind'] = {'samples': [], 'pad': [], 'status': [], 'dropped': 0}
                value['states'] = []; value['push'] = []
                value['combo'] = {'samples': [], 'dropped': 0}
        (observed, receipt), elapsed, _ = self.run_wait(mutate)
        self.assertGreater(elapsed, 0)
        self.assertEqual(observed['grind']['samples'], retained['grind']['samples'])
        self.assertEqual(observed['grind']['pad'], retained['grind']['pad'])
        self.assertEqual(observed['combo']['samples'], retained['combo']['samples'])
        self.assertEqual(observed['states'], retained['states'])
        self.assertEqual(observed['push'], retained['push'])
        self.assertGreater(receipt['ms'], observed['grind']['samples'][0]['ms'])
        self.assertEqual(batch.grind_result(observed, observed, True)['evidence'], 'COMPLETE')

    def test_deck_wait_neutralizes_once_and_uses_existing_readonly_checks(self):
        session = Mock(pid=123); env = batch.DeckEnvironment(session)
        expected = (self.observation(), {'status': 'READY'})
        def waiter(milliseconds, baseline, observe, alive, speed):
            self.assertTrue(alive()); self.assertEqual(speed, 100)
            return expected
        with patch.object(batch, 'wait_score_capture_ready', side_effect=waiter), \
                patch.object(batch, 'capabilities', return_value={'ready': True, 'gamescope_pids': [123]}, create=True):
            self.assertEqual(env.wait_score_capture_ready(2000, self.observation(), 100), expected)
        session.pad.neutral.assert_called_once()
        session.pad.frame.assert_not_called()
        session.run.assert_not_called()

    def test_timeout_is_explicit_pre_input_stop_and_no_repeat(self):
        value = plan(); case = value['candidates'][0]
        case.update(fixture_reset=True, score_capture_ready=True, repeat=2)
        env = Mock(); env.wait_idle.return_value = 1
        env.wait_grind_ready.return_value = self.observation()
        env.wait_score_capture_ready.side_effect = batch.HardStop('score_capture_baseline_timeout')
        result = batch.perform_batch(value, env)['trials']
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['reason'], 'score_capture_baseline_timeout')
        self.assertFalse(result[0]['input_sent'])
        env.gesture.assert_not_called()
        env.wait_score_capture_ready.assert_called_once()

    def test_accepted_snapshot_advances_trial_boundary_before_one_gesture(self):
        value = plan(); case = value['candidates'][0]
        case.update(fixture_reset=True, score_capture_ready=True)
        env = Mock(); env.wait_idle.side_effect = [1, 3]
        env.fixture_baseline.return_value = {'reset_ready': True}
        env.fixture_reset.return_value = {'reset_ready': True}
        baseline = self.observation(); accepted = self.observation(2, 6000, True)
        accepted['log_end'] = 5000
        receipt = {'status': 'READY', 'scope': 'observer_binding_only', 'owner_verified': False,
                   'capture_completeness': 'UNVERIFIED'}
        env.wait_grind_ready.return_value = baseline
        env.wait_score_capture_ready.return_value = (accepted, receipt)
        env.observe.side_effect = [{'log_end': 100}, {'states': [], 'combo': {'samples': []}}]
        env.gesture.return_value = {'input_sent': True, 'frames': []}
        result = batch.perform_case(value, case, env)
        env.observe.assert_called_with(since=5000)
        env.gesture.assert_called_once_with(case)
        env.wait_score_capture_ready.assert_called_once_with(case['reset_ms'], baseline)
        self.assertEqual(result['score_capture_baseline'], receipt)

    def test_partial_gesture_failure_keeps_pre_input_score_baseline(self):
        value = plan(); case = value['candidates'][0]
        case.update(fixture_reset=True, score_capture_ready=True)
        env = Mock(); env.wait_idle.return_value = 1
        baseline = self.observation(); receipt = {'status': 'READY', 'owner_verified': False}
        env.wait_grind_ready.return_value = baseline
        env.wait_score_capture_ready.return_value = (baseline, receipt)
        env.observe.side_effect = [{'log_end': 100}, {'states': []}]
        env.gesture.side_effect = BatchTests.partial_writer_error()
        result = batch.perform_batch(value, env)['trials'][0]
        self.assertEqual(result['partial_trial']['score_capture_baseline'], receipt)
        env.gesture.assert_called_once()

    def test_default_candidate_does_not_call_score_readiness(self):
        env = Mock(); env.wait_idle.side_effect = [1, 3]
        env.observe.side_effect = [{'log_end': 100}, {'states': [], 'combo': {'samples': []}}]
        env.gesture.return_value = {'input_sent': True, 'frames': []}
        value = plan(); batch.perform_case(value, value['candidates'][0], env)
        env.wait_score_capture_ready.assert_not_called()

    def test_bundle_exports_readiness_helper_without_new_remote_capability(self):
        value = plan()
        source = batch.remote_payload(value, value['candidates'][0])
        self.assertIn('def wait_score_capture_ready(', source)

    def test_initial_mixed_generation_failure_reports_bounded_baseline_summary(self):
        initial = self.observation()
        earlier = dict(initial['grind']['samples'][0], generation=3, seq=0, ms=900)
        initial['grind']['samples'].insert(0, earlier)
        with self.assertRaises(batch.HardStop) as stopped:
            self.run_wait(baseline=initial)
        receipt = stopped.exception.score_capture_failure
        self.assertEqual(receipt['cause'], 'initial_grind_evidence')
        self.assertEqual(receipt['initial'], dict(sample_count=2, status_count=1, generation_count=2,
                                               first_generation=3, last_generation=4))
        self.assertEqual(receipt['owner']['generation'], 4)
        self.assertEqual(receipt['native']['generation'], 4)

    def test_older_covered_worker_health_matches_existing_grind_rule(self):
        def mutate(value, now):
            value['grind']['status'].insert(0, dict(generation=3, active=True, dropped=0, rejected=0))
            self.assertEqual(batch.grind_result(value, self.observation(), True)['stale_health_rows'], 1)
        (_, result), _, _ = self.run_wait(mutate)
        self.assertEqual(result['status'], 'READY')

    def test_hil49_older_inactive_health_does_not_replace_current_validated_owner(self):
        initial = self.observation()
        for row in initial['grind']['samples'] + initial['grind']['pad'] + initial['grind']['status']:
            row['generation'] = 212
        initial['grind']['samples'] *= 3
        def mutate(value, now):
            for row in value['grind']['samples'] + value['grind']['pad']:
                row['generation'] = 212
            value['grind']['status'] = [dict(generation=211, active=False, dropped=0, rejected=0)]
        (observed, result), _, _ = self.run_wait(mutate, baseline=initial)
        self.assertEqual(result['status'], 'READY')
        self.assertEqual(result['generation'], 212)
        self.assertEqual(observed['grind']['status'], initial['grind']['status'])
        self.assertEqual(observed['grind']['samples'][-1]['generation'], 212)
        self.assertFalse(result['owner_verified'])

    def test_uncovered_older_and_current_or_future_health_still_fail_closed(self):
        cases = [dict(generation=3, active=active, dropped=dropped, rejected=rejected)
                 for active in (False, True) for dropped, rejected in ((1, 0), (0, 1))]
        cases += [dict(generation=4, active=False, dropped=0, rejected=0)]
        cases += [dict(generation=5, active=active, dropped=0, rejected=0) for active in (False, True)]
        for health in cases:
            def mutate(value, now): value['grind']['status'] = [health]
            with self.subTest(health=health), self.assertRaises(batch.HardStop) as stopped:
                self.run_wait(mutate)
            receipt = stopped.exception.score_capture_failure
            self.assertEqual(receipt['cause'], 'grind_status_generation' if health['active'] else 'grind_status_inactive')
            self.assertEqual(receipt['owner']['generation'], 4)
            self.assertEqual(receipt['grind_health'], health)
            result = batch.stopped_trial(stopped.exception, plan()['candidates'][0], 2)
            self.assertEqual(result['score_capture_failure'], receipt)
            self.assertFalse(result['input_sent'])

    def test_older_covered_health_cannot_refresh_missing_native_samples(self):
        def mutate(value, now):
            value['grind']['status'] = [dict(generation=3, active=False, dropped=0, rejected=0)]
            value['grind']['samples'] = []
        with self.assertRaises(batch.HardStop) as stopped:
            self.run_wait(mutate, delay=1)
        self.assertEqual(stopped.exception.score_capture_failure['cause'], 'grind_stale')

    def test_missing_initial_owner_cannot_be_replaced_by_older_health(self):
        initial = self.observation()
        initial['grind']['status'] = []
        with self.assertRaises(batch.HardStop) as stopped:
            self.run_wait(baseline=initial)
        self.assertEqual(stopped.exception.score_capture_failure['cause'], 'malformed_fields')

    def test_health_failure_receipt_retains_numeric_cause_without_raw_extra_fields(self):
        def mutate(value, now):
            value['native_score']['health'][0].update(overflow=True, private='unrelated sensitive text')
        with self.assertRaises(batch.HardStop) as stopped:
            self.run_wait(mutate)
        receipt = stopped.exception.score_capture_failure
        self.assertEqual(receipt['cause'], 'health_overflow')
        self.assertTrue(receipt['health']['overflow'])
        self.assertEqual(receipt['health']['route'], 1)
        self.assertEqual(receipt['parser']['health_malformed'], 0)
        self.assertLess(len(json.dumps(receipt)), 4096)
        self.assertNotIn('private', json.dumps(receipt))
        self.assertNotIn('unrelated sensitive text', json.dumps(receipt))

    def test_failure_receipt_validator_refuses_unknown_text_extra_fields_and_unbounded_numbers(self):
        base = batch.score_capture_failure_receipt('timeout')
        self.assertIsNone(batch.score_capture_failure_receipt('unreviewed exception text'))
        for mutation in ('cause', 'extra', 'number', 'bool', 'schema', 'schema_bool'):
            receipt = copy.deepcopy(base)
            if mutation == 'cause': receipt['cause'] = 'unreviewed exception text'
            if mutation == 'extra': receipt['health']['raw'] = 'private'
            if mutation == 'number': receipt['health']['ms'] = 1 << 100
            if mutation == 'bool': receipt['health']['overflow'] = 1
            if mutation == 'schema': receipt['schema'] = 2
            if mutation == 'schema_bool': receipt['schema'] = True
            error = batch.HardStop('score_capture_evidence_lost'); error.score_capture_failure = receipt
            with self.subTest(mutation=mutation):
                result = batch.stopped_trial(error, plan()['candidates'][0], 1)
                self.assertNotIn('score_capture_failure', result)

    def test_timeout_receipt_carries_last_native_snapshot_and_wait_age(self):
        def mutate(value, now): value['native_score']['health'] = []
        with self.assertRaises(batch.HardStop) as stopped:
            self.run_wait(mutate)
        receipt = stopped.exception.score_capture_failure
        self.assertEqual(receipt['cause'], 'timeout')
        self.assertGreaterEqual(receipt['timing']['elapsed_ms'], 2000)
        self.assertEqual(receipt['timing']['routes_mask'], 0)
        self.assertEqual(receipt['native']['state'], 100)
        self.assertIsNone(receipt['health']['ms'])


if __name__ == "__main__":
    unittest.main()
