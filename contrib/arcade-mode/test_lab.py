"""Offline Mac/Linux integration tests. Never connect to a real host or HOME."""
import ast
import base64
import contextlib
import fcntl
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from lab import controller, probe
import deck_installer as installer
import deck_updater as updater
import package_deck_installer as package
from test_deck_installer import fake_dll, game_at

STOPPED = {"skate": False, "launcher": False, "check_complete": True}


class ComboTraceTests(unittest.TestCase):
    def test_fixture_status_is_numeric_allowlisted_and_rejects_invalid_context(self):
        row = {k: False for k in ('configured', 'available', 'busy', 'failed', 'editor_open',
                                  'asset_available', 'present', 'skater', 'reset_ready')}
        row.update(configured=True, heading=0., speed=0., state=100, generation=3, revision=1,
                   position=[1, 2, 3], start=[1, 2, 3], rail_position=[1, 2, 11])
        def parse(value):
            return probe.parse_fixture_status('ARCADE FIXTURE: ' + json.dumps(value))
        self.assertEqual(parse(dict(row, private='never exported')), row)
        for key, value in [('speed', -1), ('heading', float('nan')), ('state', 1.5),
                           ('generation', True), ('revision', -1), ('start', [0, 0]),
                           ('position', [0, 0, 100001]), ('present', 1)]:
            self.assertIsNone(parse(dict(row, **{key: value})))
        del row['reset_ready']
        self.assertIsNone(parse(row))
        row['configured'] = False
        del row['start'], row['rail_position']
        self.assertFalse(parse(row)['reset_ready'])

    def test_grind_native_evidence_and_unknown_enum_semantics(self):
        from lab import trick_batch
        row = ('ARCADE GRIND STATE: seq=1 ms=200 generation=3 valid=true state=401 edge=1 duration=0 '
               'grind_valid=true grind=42 skate_valid=false skate=0 speed_valid=true speed_mm_s=5000 arcade=true dropped=0.')
        status = 'ARCADE GRIND STATUS: generation=3 active=true dropped=0 rejected=0.'
        pad = 'ARCADE GRIND PAD: seq=2 ms=210 generation=3 user=0 y=true x=-32768 ry=32767 arcade=true dropped=0.'
        parsed = probe.parse_grind_observations('\n'.join([row, status, pad]))
        self.assertEqual(parsed['samples'][0]['grind'], 42)
        self.assertTrue(parsed['pad'][0]['requested_y'])
        result = trick_batch.grind_result({'grind': parsed}, {'grind': parsed}, True)
        self.assertEqual((result['native_grind'], result['named_identity'], result['entries']), ('PASS', 'UNKNOWN', 1))
        self.assertEqual(result['raw_enum_pairs'], [(42, None)])
        for bad in (row + ' raw text', row.replace('state=401', 'state=4294967296'),
                    row.replace('generation=3', 'generation=0'), row.replace('duration=0', 'duration=18446744073709551616')):
            self.assertEqual(probe.parse_grind_observations(bad)['samples'], [])
        self.assertEqual(probe.parse_grind_observations(pad.replace('ry=32767', 'ry=32768'))['pad'], [])
        overflowing = probe.parse_grind_observations('\n'.join([row] * 257 + [status]))
        self.assertEqual((len(overflowing['samples']), overflowing['dropped']), (256, 1))
        self.assertEqual(trick_batch.grind_result({'grind': overflowing}, {'grind': parsed}, True)['native_grind'], 'UNVERIFIED')
        lost = probe.parse_grind_observations('\n'.join([row.replace('dropped=0', 'dropped=1'), status]))
        self.assertEqual(trick_batch.grind_result({'grind': lost}, {'grind': parsed}, True)['native_grind'], 'UNVERIFIED')
        only_input = probe.parse_grind_observations('\n'.join([pad, status]))
        self.assertEqual(trick_batch.grind_result({'grind': only_input}, {'grind': parsed}, True)['native_grind'], 'UNVERIFIED')

    def test_native_score_events_are_bounded_numeric_and_keep_unknown_identity(self):
        from lab import trick_batch
        row = 'ARCADE NATIVE SCORE: seq=1 ms=200 epoch=3 route=2 score=840.5 reason=7 success=false owner_verified=false.'
        health = 'ARCADE NATIVE SCORE STATUS: epoch=3 active=true bound=true dropped=2 rejected=1 contended=3 stale=4 owner_verified=false.'
        result = trick_batch.observations('\n'.join([row] * 130 + [health] * 34))
        native = result['native_score']
        self.assertEqual((len(native['samples']), native['dropped'], len(native['status']), native['status_dropped']), (128, 2, 32, 2))
        self.assertEqual(native['samples'][0]['score'], 840.5)
        self.assertFalse(native['samples'][0]['owner_verified'])
        self.assertEqual(native['native_event_identity'], 'UNVERIFIED')
        self.assertEqual(native['status'][0]['dropped'], 2)
        self.assertEqual(trick_batch.named_flip_result('kickflip', result, 0), 'UNVERIFIED')
        for bad in (row + ' raw text', row.replace('score=840.5', 'score=1e309'),
                    row.replace('score=840.5', 'score=-1'), row.replace('score=840.5', 'score=2147483648'),
                    row.replace('epoch=3', 'epoch=18446744073709551616'), row.replace('seq=1', 'seq=0'),
                    row.replace('owner_verified=false', 'owner_verified=true'), row.replace('reason=7', 'reason=9'),
                    row.replace('route=2', 'route=1'), row.replace('route=2', 'route=4'),
                    row.replace('success=false', 'success=true')):
            self.assertEqual(probe.parse_native_score_events(bad)['samples'], [])
        self.assertEqual(probe.parse_native_score_events(health.replace('active=true', 'active=false'))['status'], [])
        sequence = row.replace('route=2', 'route=4').replace('reason=7', 'reason=0').replace('success=false', 'success=true')
        self.assertTrue(probe.parse_native_score_events(sequence)['samples'][0]['success'])

    def test_score_numbers_preserve_invalid_unknown_and_stale_evidence(self):
        row = ('ARCADE SCORE NUMBERS: ms=200 sampled=100 score_valid=true score=500 '
               'multiplier_valid=true multiplier=2 owner_verified=false.')
        result = probe.parse_score_numbers('\n'.join(row.replace('ms=200', f'ms={i + 200}') for i in range(130)))
        self.assertEqual((len(result['samples']), result['dropped']), (128, 2))
        self.assertEqual(result['samples'][0]['ms'], 202)
        self.assertTrue(result['samples'][0]['fresh'])
        self.assertFalse(result['samples'][-1]['fresh'])
        self.assertFalse(result['samples'][-1]['owner_verified'])
        invalid = row.replace('score_valid=true score=500', 'score_valid=false score=0')
        self.assertFalse(probe.parse_score_numbers(invalid)['samples'][0]['score_valid'])
        for field in ('ms', 'score', 'multiplier'):
            maximum = re.sub(rf'{field}=\d+', f'{field}=18446744073709551615', row)
            self.assertEqual(probe.parse_score_numbers(maximum)['samples'][0][field], 18446744073709551615)
            self.assertEqual(probe.parse_score_numbers(maximum.replace('18446744073709551615', '18446744073709551616'))['samples'], [])
        for bad in (row + ' raw text', row.replace('sampled=100', 'sampled=201'),
                    row.replace('score_valid=true', 'score_valid=false'), row.replace('score=500', 'score=-1'),
                    row.replace('multiplier=2', 'multiplier=0'), row.replace('multiplier_valid=true', 'multiplier_valid=false'),
                    row.replace('owner_verified=false', 'owner_verified=true')):
            self.assertEqual(probe.parse_score_numbers(bad)['samples'], [])

    def test_score_numbers_never_establish_native_trick_identity(self):
        from lab import trick_batch
        result = trick_batch.observations('ARCADE SCORE NUMBERS: ms=200 sampled=100 score_valid=true score=500 multiplier_valid=true multiplier=2 owner_verified=false.')
        self.assertEqual(result['score_numbers']['samples'][0]['score'], 500)
        self.assertEqual(trick_batch.named_flip_result('kickflip', result, 0), 'UNVERIFIED')

    def test_score_edges_are_bounded_numeric_label_appearances(self):
        row = 'ARCADE SCORE EDGE: seq=3 observed=125 sampled=120 labels=1048575 uncertainty=5.'
        result = probe.parse_score_edges('\n'.join(row.replace('seq=3', f'seq={i}') for i in range(130)))
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['samples'][0]['seq'], 2)
        self.assertEqual(result['samples'][-1], dict(seq=129, observed=125, sampled=120, labels=1048575, uncertainty=5))
        for field in ('seq', 'observed', 'sampled'):
            maximum = re.sub(rf'{field}=\d+', f'{field}=18446744073709551615', row)
            self.assertEqual(probe.parse_score_edges(maximum)['samples'][0][field], 18446744073709551615)
            self.assertEqual(probe.parse_score_edges(maximum.replace('18446744073709551615',
                '18446744073709551616'))['samples'], [])
        for bad in (row + ' private', row.replace('labels=1048575', 'labels=1048576'),
                    row.replace('labels=1048575', 'labels=-1'), row.replace('uncertainty=5', 'uncertainty=6'),
                    row.replace('sampled=120', 'sampled=-1'), row.replace('observed=125', 'observed=true')):
            self.assertEqual(probe.parse_score_edges(bad), dict(samples=[], dropped=0))
        for value in range(6):
            self.assertEqual(probe.parse_score_edges(row.replace('uncertainty=5', f'uncertainty={value}'))
                             ['samples'][0]['uncertainty'], value)

    def test_score_edge_batch_exposure_does_not_establish_native_identity(self):
        from lab import trick_batch
        result = trick_batch.observations('ARCADE SCORE EDGE: seq=3 observed=125 sampled=120 labels=8 uncertainty=0.')
        self.assertEqual(result['score_edges']['samples'][0]['labels'], 8)
        self.assertEqual(result['score_edges']['dropped'], 0)
        self.assertEqual(trick_batch.named_flip_result('kickflip', result, 0), 'UNVERIFIED')

    def test_action_trace_numeric_limits_tail_loss_and_booleans(self):
        row = ('ARCADE ACTION SAMPLE: seq=3 ms=125 tick=7 user=3 mode=4 requested=true '
               'kind=4 trick=53 result=5 owned=false lx=-32768 ly=32767 lt=0 rt=255 packet=4294967295.')
        result = probe.parse_action_samples('\n'.join(row.replace('seq=3', f'seq={i}') for i in range(130)))
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['samples'][0]['seq'], 2)
        self.assertEqual(result['samples'][-1], dict(seq=129, ms=125, tick=7, user=3, mode=4,
            requested=True, kind=4, trick=53, result=5, owned=False, lx=-32768, ly=32767, lt=0, rt=255,
            packet=4294967295))
        for field in ('seq', 'ms', 'tick'):
            maximum = re.sub(rf'{field}=\d+', f'{field}=18446744073709551615', row)
            self.assertEqual(probe.parse_action_samples(maximum)['samples'][0][field], 18446744073709551615)
            self.assertEqual(probe.parse_action_samples(maximum.replace('18446744073709551615',
                '18446744073709551616'))['samples'], [])
        for field in ('requested', 'owned'):
            for value in ('true', 'false'):
                changed = re.sub(rf'{field}=(true|false)', f'{field}={value}', row)
                self.assertIs(probe.parse_action_samples(changed)['samples'][0][field], value == 'true')
        for field, old, bad in (
                ('user', '3', '4'), ('mode', '4', '5'), ('kind', '4', '6'), ('trick', '53', '54'),
                ('result', '5', '6'), ('requested', 'true', '1'), ('owned', 'false', 'True'),
                ('lx', '-32768', '-32769'), ('ly', '32767', '32768'), ('lt', '0', '-1'),
                ('rt', '255', '256'), ('packet', '4294967295', '4294967296'), ('seq', '3', '-1')):
            with self.subTest(field=field):
                self.assertEqual(probe.parse_action_samples(row.replace(f'{field}={old}', f'{field}={bad}')),
                                 dict(samples=[], dropped=0))
        self.assertEqual(probe.parse_action_samples(row + ' private')['samples'], [])
        self.assertEqual(probe.parse_action_samples(row[:-1])['samples'], [])

    def test_action_trace_is_joinable_dispatch_evidence_not_native_success(self):
        from lab import trick_batch
        text = ('ARCADE STICK SAMPLE: seq=3 ms=125 tick=7 user=0 x=12000 y=20000 phase=1.\n'
                'ARCADE ACTION SAMPLE: seq=3 ms=125 tick=7 user=0 mode=2 requested=true '
                'kind=1 trick=21 result=0 owned=true lx=0 ly=0 lt=0 rt=255 packet=11.')
        result = trick_batch.observations(text)
        action, stick = result['actions']['samples'][0], result['stick']['samples'][0]
        self.assertEqual(tuple(action[key] for key in ('seq', 'ms', 'tick', 'user')),
                         tuple(stick[key] for key in ('seq', 'ms', 'tick', 'user')))
        self.assertEqual(trick_batch.named_flip_result('kickflip', result, 0), 'UNVERIFIED')
        self.assertEqual(result['actions']['dropped'], 0)

    def test_rotation_delivery_is_not_a_native_trick_claim(self):
        row = ('ARCADE ACTION SAMPLE: seq=9 ms=125 tick=7 user=0 mode=0 requested=false '
               'kind=5 trick=0 result=0 owned=true lx=-31783 ly=0 lt=0 rt=0 packet=12.')
        sample = probe.parse_action_samples(row)['samples'][0]
        self.assertEqual((sample['kind'], sample['trick']), (5, 0))
        self.assertTrue(sample['owned'])
        self.assertFalse(sample['requested'])

    def test_playback_trace_preserves_cadence_and_packet_bounds_with_explicit_loss(self):
        row = 'ARCADE PLAYBACK SAMPLE: seq=3 poll=4 tick=7 mode=2 input_packet=0 output_packet=4294967295.'
        result = probe.parse_playback_samples('\n'.join(row.replace('seq=3', f'seq={i}') for i in range(130)))
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['samples'][0]['seq'], 2)
        self.assertEqual(result['samples'][-1], dict(seq=129, poll=4, tick=7, mode=2,
                                                   input_packet=0, output_packet=4294967295))
        for mode in range(3):
            self.assertEqual(probe.parse_playback_samples(row.replace('mode=2', f'mode={mode}'))['samples'][0]['mode'], mode)
        for field in ('seq', 'poll', 'tick'):
            maximum = re.sub(rf'{field}=\d+', f'{field}=18446744073709551615', row)
            self.assertEqual(probe.parse_playback_samples(maximum)['samples'][0][field], 18446744073709551615)
            for bad in (maximum.replace('18446744073709551615', '18446744073709551616'),
                        maximum.replace('18446744073709551615', '-1')):
                self.assertEqual(probe.parse_playback_samples(bad), dict(samples=[], dropped=0))
        for bad in (row + ' private', row.replace('mode=2', 'mode=3'), row.replace('mode=2', 'mode=-1'),
                    row.replace('input_packet=0', 'input_packet=4294967296'),
                    row.replace('input_packet=0', 'input_packet=-1'), row.replace('4294967295', '4294967296')):
            self.assertEqual(probe.parse_playback_samples(bad), dict(samples=[], dropped=0))

    def test_trial_observations_expose_playback_and_stick_rows_with_shared_sequence(self):
        from lab import trick_batch
        text = ('ARCADE STICK SAMPLE: seq=3 ms=125 tick=7 user=0 x=12000 y=20000 phase=1.\n'
                'ARCADE PLAYBACK SAMPLE: seq=3 poll=4 tick=7 mode=1 input_packet=10 output_packet=11.')
        result = trick_batch.observations(text)
        self.assertEqual(result['playback']['samples'][0]['seq'], result['stick']['samples'][0]['seq'])
        self.assertEqual(result['playback']['samples'][0]['mode'], 1)
        self.assertEqual(result['playback']['dropped'], 0)

    def test_delivered_stick_trace_has_bounded_numeric_fields_and_explicit_loss(self):
        row = 'ARCADE STICK SAMPLE: seq=3 ms=125 tick=7 user=0 x=-32768 y=32767 phase=1.'
        result = probe.parse_stick_samples((row + '\n') * 130)
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['samples'][-1], dict(seq=3, ms=125, tick=7, user=0, x=-32768, y=32767, phase=1))
        for bad in (row + ' private', row.replace('user=0', 'user=4'), row.replace('32767', '32768'),
                    row.replace('-32768', '-32769'), row.replace('tick=7', 'tick=18446744073709551616')):
            self.assertEqual(probe.parse_stick_samples(bad)['samples'], [])

    def test_authored_arc_details_are_bounded_complete_and_consistent(self):
        entry = 'ARCADE AUTHORED GESTURE: index=0 total=1 id=36 nollie=false context=0 arcs=2.\n'
        arc = 'ARCADE AUTHORED ARC: index=0 arc=0 start=-45000 end=45000 min=800 max=1000 wait=-1000.\n'
        ready = 'ARCADE AUTHORED STATUS: state=2.'
        partial = probe.parse_authored_catalog(entry + arc + ready)
        self.assertFalse(partial['entries'][0]['arcs_complete'])
        complete = entry + arc + arc.replace('arc=0', 'arc=1') + ready
        self.assertTrue(probe.parse_authored_catalog(complete)['entries'][0]['arcs_complete'])
        self.assertEqual(partial['entries'][0]['arc_data'][0]['start_mdeg'], -45000)
        conflict = probe.parse_authored_catalog(complete + '\n' + arc.replace('max=1000', 'max=1100'))
        self.assertFalse(conflict['complete'])
        self.assertFalse(conflict['entries'][0]['arcs_complete'])
        for bad in (arc.replace('index=0', 'index=256'), arc.replace('arc=0', 'arc=32'),
                    arc.replace('-45000', '-3600001'), arc.replace('max=1000', 'max=799'),
                    arc.replace('wait=-1000', 'wait=10001'), arc.rstrip() + ' private\n'):
            self.assertNotIn('arc_data', probe.parse_authored_catalog(entry + bad + ready)['entries'][0])

    def test_flip_identifiers_are_numeric_bounded_and_do_not_infer_tricks(self):
        row = 'ARCADE FLIP OBSERVATION: ms=123 valid=true id=2.'
        result = probe.parse_flip_observations((row + '\n') * 130)
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['samples'][-1], {'ms': 123, 'valid': True, 'id': 2})
        for bad in (row + ' private', row.replace('id=2', 'id=64'), row.replace('id=2', 'id=-2'),
                    row.replace('123', '18446744073709551616')):
            self.assertEqual(probe.parse_flip_observations(bad)['samples'], [])

    def test_authored_catalog_requires_all_consistent_bounded_entries(self):
        a = 'ARCADE AUTHORED GESTURE: index=0 total=2 id=2 nollie=false context=0 arcs=3.\n'
        b = 'ARCADE AUTHORED GESTURE: index=1 total=2 id=17 nollie=true context=1 arcs=4.\n'
        ready = 'ARCADE AUTHORED STATUS: state=2.'
        self.assertTrue(probe.parse_authored_catalog(a + b + ready)['complete'])
        for text in (a + ready, a + b, a + b + a.replace('id=2', 'id=3') + ready,
                     a + b.replace('total=2', 'total=3') + ready):
            self.assertFalse(probe.parse_authored_catalog(text)['complete'])
        for bad in (a.replace('arcs=3', 'arcs=33'), a.replace('id=2', 'id=64'),
                    a.replace('total=2', 'total=257'), a.replace('context=0', 'context=3')):
            self.assertEqual(probe.parse_authored_catalog(bad)['entries'], [])

    def test_authored_load_diagnostics_never_return_paths_or_arbitrary_text(self):
        row = 'ARCADE AUTHORED DIAGNOSTIC: stage=6 field=13 error=2 rejected=1.'
        result = probe.parse_authored_catalog(row)
        self.assertTrue(result['partial'])
        self.assertEqual(result['diagnostic'], {'stage': 6, 'field': 13, 'error': 2, 'rejected': 1})
        for bad in (row + ' private', row.replace('field=13', 'field=15'),
                    row.replace('rejected=1', 'rejected=65536'), row.replace('stage=6', 'stage=8')):
            self.assertIsNone(probe.parse_authored_catalog(bad)['diagnostic'])

    def test_authored_label_tokens_are_bounded_and_conflicts_invalidate_catalog(self):
        entry = 'ARCADE AUTHORED GESTURE: index=0 total=1 id=36 nollie=false context=0 arcs=8.\n'
        label = 'ARCADE AUTHORED LABEL: index=0 tokens=33.\n'
        ready = 'ARCADE AUTHORED STATUS: state=2.'
        result = probe.parse_authored_catalog(entry + label + ready)
        self.assertTrue(result['complete'])
        self.assertEqual(result['entries'][0]['source_tokens'], 33)
        self.assertFalse(probe.parse_authored_catalog(entry + label + label.replace('33', '1') + ready)['complete'])
        for bad in (label.replace('33', '1024'), label.replace('index=0', 'index=256'), label.rstrip() + ' private\n'):
            self.assertNotIn('source_tokens', probe.parse_authored_catalog(entry + bad + ready)['entries'][0])

    def test_source_pad_observation_is_separate_and_bounded(self):
        row = 'ARCADE PAD SOURCE: seq=1 ms=123 user=0 buttons=768 lt=255 rt=0.'
        result = probe.parse_pad_samples((row + '\n') * 130)
        self.assertEqual(result['samples'], [])
        self.assertEqual(len(result['source_samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['source_samples'][-1]['lt'], 255)
        for bad in (row + ' private', row.replace('255', '256'), row.replace('user=0', 'user=4'),
                    row.replace('768', '65536'), row.replace('123', '18446744073709551616')):
            self.assertEqual(probe.parse_pad_samples(bad)['source_samples'], [])

    def test_pad_observation_bounds_and_explicit_evidence_loss(self):
        row = 'ARCADE PAD SAMPLE: seq=1 ms=123 user=0 buttons=768 lt=255 rt=0 output=768 phase=0.'
        result = probe.parse_pad_samples((row + '\n') * 130 + 'ARCADE PAD GAP: dropped=3.')
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 5)
        self.assertEqual(result['samples'][-1], dict(seq=1, ms=123, user=0, buttons=768, lt=255, rt=0, output=768, phase=0))
        for bad in (row + ' private', row.replace('255', '256'), row.replace('user=0', 'user=4'),
                    row.replace('768', '65536'), row.replace('123', '18446744073709551616')):
            self.assertEqual(probe.parse_pad_samples(bad)['samples'], [])

    def test_push_observation_is_numeric_bounded_and_read_only(self):
        row = 'ARCADE PUSH OBSERVATION: seq=1 ms=123 valid=true eligible=true speed=5.25 contacts=49152.'
        result = probe.parse_push_samples((row + '\n') * 200 + ' unrelated private text')
        self.assertEqual(len(result), 128)
        self.assertEqual(result[-1], dict(seq=1, ms=123, valid=True, eligible=True, speed=5.25, contacts=49152))
        for bad in (row + ' unrelated', row.replace('5.25', 'nan'), row.replace('5.25', '1e999'),
                    row.replace('5.25', '-1'), row.replace('49152', '49153'),
                    row.replace('123', '18446744073709551616')):
            self.assertEqual(probe.parse_push_samples(bad), [])

    def test_balance_samples_preserve_measured_correction_without_private_text(self):
        row = ('ARCADE BALANCE SAMPLE: ms=12 kind=2 x=0.4 v=0.2 input=-0.3 '
               'difficulty=0.5 raw_x=0 raw_y=-15000.')
        samples = probe.parse_combo_samples((row + '\n') * 200 + 'secret')['balance']
        self.assertEqual(len(samples), 128)
        self.assertEqual(samples[-1]['correction'], -0.3)
        self.assertEqual(samples[-1]['raw_y'], -15000)
        for bad in (row + ' private', row.replace('x=0.4', 'x=1e999'), row.replace('-15000', '-40000')):
            self.assertEqual(probe.parse_combo_samples(bad)['balance'], [])

    def test_runtime_combo_events_are_bounded_and_sanitized(self):
        row = ('ARCADE COMBO EVENT: ms=12 pose=4 active=true failed=false '
               'fatigue=0.5 grind=0.2 manual=0.3 native_failure_verified=false.')
        events = probe.parse_combo_samples((row + '\n') * 200 + 'secret')['events']
        self.assertEqual(len(events), 128)
        self.assertEqual(events[-1]['pose'], 4)
        self.assertFalse(events[-1]['native_failure_verified'])
        for bad in (row + ' private', row.replace('0.5', '1e999'), row.replace('pose=4', 'pose=9')):
            self.assertEqual(probe.parse_combo_samples(bad)['events'], [])

    def test_nested_score_probe_is_numeric_bounded_and_private(self):
        rows = ('ARCADE SCORE NODE: path=9b74fc7c.12345678 kind=10 size=1.\n'
                'ARCADE SCORE VALUE: ms=99 path=9b74fc7c.12345678 value=1.\n') * 500
        value = probe.parse_score_samples(rows + 'private unrelated text')
        self.assertEqual(len(value['nodes']), 256)
        self.assertEqual(len(value['values']), 128)
        self.assertNotIn('private', json.dumps(value))
        self.assertTrue(probe.parse_score_samples('ARCADE SCORE LIMIT: reached=true.')['limit_reached'])
        self.assertEqual(probe.parse_score_samples(
            'ARCADE SCORE VALUE: ms=99 path=../../home value=1.')['values'], [])
        self.assertEqual(probe.parse_score_samples(
            'ARCADE SCORE VALUE: ms=99 path=9b74fc7c value=4294967296.')['values'], [])

    def test_score_probe_allowlist_and_bounds(self):
        text = 'unrelated secret /home/personal\n' + (
            'ARCADE SCORE FIELD: hash=12345678 kind=7 size=8.\n'
            'ARCADE SCORE SAMPLE: ms=12 hash=12345678 value=2.\n') * 200
        result = probe.parse_score_samples(text)
        self.assertEqual(len(result['fields']), 128)
        self.assertEqual(len(result['samples']), 128)
        self.assertNotIn('secret', json.dumps(result))
        self.assertEqual(result['event_identity'], 'UNVERIFIED')

    def test_score_label_events_are_exact_bounded_and_distinct_from_numeric_fields(self):
        row = 'ARCADE SCORE LABEL: ms=12 path=9b74fc7c.12345678 id=20.'
        result = probe.parse_score_samples((row + '\n') * 200)
        self.assertEqual(len(result['labels']), 128)
        self.assertEqual(result['labels_dropped'], 72)
        self.assertEqual(result['labels'][-1]['id'], 20)
        for bad in (row.replace('id=20', 'id=21'), row.replace('id=20', 'id=0'),
                    row + ' private', row.replace('ms=12', 'ms=99999999999999999999'),
                    row.replace('LABEL', 'VALUE')):
            self.assertEqual(probe.parse_score_samples(bad)['labels'], [])

    def test_score_probe_rejects_arbitrary_values_and_suffix(self):
        for row in ('ARCADE SCORE SAMPLE: ms=12 hash=12345678 value=5.',
                    'ARCADE SCORE SAMPLE: ms=12 hash=12345678 value=2. private',
                    'ARCADE SCORE SAMPLE: ms=99999999999999999999 hash=12345678 value=1.'):
            self.assertEqual(probe.parse_score_samples(row)['samples'], [])

    def test_numeric_allowlist(self):
        row = 'ARCADE COMBO SAMPLE: ms=12 state=401 mode=1 words=' + ','.join(['00000001'] * 8) + '.'
        result = probe.parse_combo_samples('password=secret /home/personal\n' + row)
        self.assertEqual(result['samples'], [{'ms': 12, 'state': 401, 'mode': 1, 'words': [1] * 8}])
        self.assertNotIn('secret', json.dumps(result))
        self.assertEqual(result['native_event_identity'], 'UNVERIFIED')

    def test_bounds_and_gaps(self):
        row = 'ARCADE COMBO SAMPLE: ms=12 state=401 mode=1 words=' + ','.join(['00000001'] * 8) + '.'
        result = probe.parse_combo_samples((row + '\n') * 500 + 'ARCADE COMBO GAP: dropped=17.')
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 17)

    def test_malformed_rejected(self):
        row = 'ARCADE COMBO SAMPLE: ms=12 state=9999999999 mode=1 words=' + ','.join(['00000001'] * 8) + '.'
        self.assertEqual(probe.parse_combo_samples(row)['samples'], [])
        self.assertEqual(probe.parse_combo_samples(row.replace('9999999999', '401') + ' private')['samples'], [])


class Fixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.home = self.root / "deck-home"
        self.game = game_at(self.home / ".local/share/Steam/steamapps/common/skate")
        (self.game / installer.SETTINGS).write_text('{"updates":false,"width":1280,"other":"untouched"}')
        self.dll = fake_dll()
        self.commit = "a" * 40
        self.manifest = {"commit": self.commit, "sha256": installer.digest(self.dll)}
        self.dll_path = self.root / "build.dll"
        self.dll_path.write_bytes(self.dll)
        self.desktop = self.root / "Install.desktop"
        package.build(self.dll_path, self.commit, self.desktop)
        self.channel = json.loads(self.desktop.with_name("channel.json").read_text())
        self.fetch = Mock(side_effect=lambda url, *a: json.dumps(self.channel).encode()
                          if url == updater.CHANNEL else self.desktop.read_bytes())
        self.process_info = dict(STOPPED)
        self.commands = []
        self.lab = controller.Lab(directory=self.root / "mac-lab", game=str(self.game),
                                  run=self.fake_ssh, fetch=self.fetch)

    def fake_ssh(self, argv, **kwargs):
        self.commands.append(argv)
        self.assertEqual(argv[0], "ssh")
        code = shlex.split(argv[-1])
        self.assertEqual(code[:3], ["python3", "-B", "-c"])
        output = io.StringIO()
        # Run the actual serialized remote code with only external process state,
        # HOME and stdin substituted. The real installer writes the fake game.
        source = code[3].replace("def processes(", "def original_processes(")
        namespace = {"__name__": "fake_remote", "processes": lambda: self.process_info}
        with patch("pathlib.Path.home", return_value=self.home), patch("sys.stdin", io.StringIO(kwargs["input"])), \
             patch("subprocess.run", return_value=subprocess.CompletedProcess([], 1)), contextlib.redirect_stdout(output):
            exec(compile(source, "remote.py", "exec"), namespace)
        return subprocess.CompletedProcess(argv, 0, output.getvalue(), "")

    def snap(self):
        return probe.snapshot({"game": str(self.game), "dll_size": len(self.dll)},
                              home=self.home, media=self.root / "media", process_info=self.process_info)

    def contents(self):
        return {str(p.relative_to(self.home)): p.read_bytes() for p in self.home.rglob("*") if p.is_file()}

    def installed(self):
        with patch.object(installer, "ensure_stopped"):
            installer.change_installation(self.game, self.dll, self.commit)

    def log(self, text=None):
        path = self.game / "logs/ReSkate.log"
        path.parent.mkdir(exist_ok=True)
        content = text if text is not None else ('Arcade Mode ready (development observer).\n'
            '[2026-10-11 22:00:00.123] [42:7] SDK: Arcade mode initialized: active=true\n' +
            ''.join('ARCADE MODE DEV: ' + b + ' press #1 seen on ReSkate client tick.\n' for b in "ABXY"))
        path.write_text(content)
        receipt = json.loads((self.game / installer.RECEIPT).read_text()) if (self.game / installer.RECEIPT).exists() else {}
        timestamp = receipt.get("installed_at", 0) + 2
        os.utime(path, (timestamp, timestamp))
        return path


class LabTests(Fixture):
    def test_readonly_generated_code_ast(self):
        for operation in ("preflight", "status", "collect", "trace"):
            tree = ast.parse(controller.remote_code(operation))
            banned = {"write_text", "write_bytes", "mkdir", "unlink", "rename", "replace", "chmod", "touch",
                      "atomic_write", "change_installation", "exec", "eval", "system", "run", "Popen", "kill"}
            for node in ast.walk(tree):
                if isinstance(node, ast.Call):
                    name = getattr(node.func, "attr", getattr(node.func, "id", ""))
                    # str.replace only normalizes paths; it is not os.replace.
                    if name == "replace":
                        self.assertIsInstance(node.func.value, (ast.Name, ast.Subscript))
                        continue
                    self.assertNotIn(name, banned)
                    if name == "open":
                        self.assertEqual(ast.literal_eval(node.args[0]), "rb")
        self.assertNotIn("deck_installer", controller.remote_code("preflight"))

    def test_preflight_actual_remote_no_mutation(self):
        before = self.contents()
        result = self.lab.preflight()
        self.assertTrue(result["pass"])
        self.assertEqual(result["latest_tested_commit"], self.commit)
        self.assertEqual(result["expected_dll_sha256"], self.manifest["sha256"])
        self.assertEqual(before, self.contents())
        self.assertFalse(self.lab.directory.exists())

    def test_fake_deck_deploy_repeat_exact_restore(self):
        original = (self.game / "ReSkate.dll").read_bytes()
        settings = installer.load_settings(self.game)
        for _ in range(2):
            self.lab.deploy()
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        receipt = json.loads((self.game / installer.RECEIPT).read_text())
        self.assertEqual(receipt["commit"], self.commit)
        self.assertEqual(receipt["dll_sha256"], self.manifest["sha256"])
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), original)
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), original)
        self.assertEqual(installer.load_settings(self.game), settings)
        self.lab.remote("restore")
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), original)

    def test_deploy_running_refused(self):
        for name in ("skate", "launcher"):
            self.process_info[name] = True
            with self.assertRaises(RuntimeError):
                self.lab.deploy()
            self.process_info[name] = False
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_deploy_unknown_processes_refused(self):
        self.process_info["check_complete"] = False
        with self.assertRaises(RuntimeError):
            self.lab.deploy()

    def test_deploy_preflight_missing_file_refused(self):
        (self.game / "Skate.exe").unlink()
        with self.assertRaises(RuntimeError):
            self.lab.deploy()
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_remote_rechecks_running_before_deploy(self):
        self.process_info["launcher"] = True
        with self.assertRaises(RuntimeError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit=self.commit,
                            dll=base64.b64encode(self.dll).decode(), dll_size=len(self.dll))

    def test_remote_wrong_source_commit(self):
        with self.assertRaises(ValueError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit="b" * 40,
                            dll=base64.b64encode(self.dll).decode(), dll_size=len(self.dll))
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_remote_wrong_source_hash(self):
        with self.assertRaises(ValueError):
            self.lab.remote("deploy", manifest=self.manifest, expected_commit=self.commit,
                            dll=base64.b64encode(self.dll + b"corrupt").decode(), dll_size=len(self.dll))

    def test_destination_mismatch_rejected(self):
        with patch.object(self.lab, "remote", side_effect=[dict(self.snap(), **{"pass": True}),
                                                         {"dll_sha256": "0" * 64, "installed_commit": self.commit}]):
            with self.assertRaises(RuntimeError):
                self.lab.deploy()

    def test_ssh_host_key_policy(self):
        for pair in (False, True):
            args = controller.ssh_arguments("deck@steamdeck.local", self.root, pair)
            self.assertIn("StrictHostKeyChecking=" + ("ask" if pair else "yes"), args)
            self.assertIn("IdentitiesOnly=yes", args)
            self.assertNotIn("StrictHostKeyChecking=no", args)
            self.assertNotIn("accept-new", " ".join(args))
            self.assertNotIn("sshpass", args)

    def test_host_command_injection_rejected(self):
        for host in ("-oProxyCommand=bad", "deck@foo;touch x", "deck@$(id)"):
            with self.assertRaises(ValueError):
                controller.ssh_arguments(host, self.root)

    def test_pair_only_modifies_ssh_authorization(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake private key")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        run = Mock()
        self.lab.run = run
        before = self.contents()
        self.lab.pair()
        self.assertEqual(run.call_count, 1)
        args, options = run.call_args
        command = args[0][-1]
        self.assertIn("authorized_keys", command)
        for text in ("Skate", "installer", "config.json", "private key", "password"):
            self.assertNotIn(text, command)
        self.assertNotIn("input", options)
        self.assertNotIn("capture_output", options)
        self.assertEqual(before, self.contents())
        self.fetch.assert_not_called()

    def test_key_generation_ed25519_no_password_argument(self):
        self.lab.run = Mock()
        self.lab.key()
        args = self.lab.run.call_args.args[0]
        self.assertEqual(args[:3], ["ssh-keygen", "-t", "ed25519"])
        self.assertEqual(args[args.index("-N") + 1], "")

    def test_pair_remote_is_idempotent(self):
        # Execute only generated shell against a temporary HOME; fake key, no SSH.
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        self.lab.run = Mock()
        self.lab.pair()
        command = self.lab.run.call_args.args[0][-1]
        for _ in range(2):
            subprocess.run(["/bin/sh", "-c", command], env=dict(os.environ, HOME=str(self.home)), check=True)
        self.assertEqual((self.home / ".ssh/authorized_keys").read_text(), "ssh-ed25519 AAAA lab\n")

    def test_pair_changed_host_key_failure_never_deploys(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").write_text("fake")
        (self.lab.directory / "id_ed25519.pub").write_text("ssh-ed25519 AAAA lab")
        self.lab.run = Mock(side_effect=subprocess.CalledProcessError(255, ["ssh"]))
        with self.assertRaises(subprocess.CalledProcessError):
            self.lab.pair()
        self.fetch.assert_not_called()
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_key_symlink_refused_before_keygen(self):
        self.lab.directory.mkdir()
        (self.lab.directory / "id_ed25519").symlink_to(self.root / "victim")
        self.lab.run = Mock()
        with self.assertRaises(RuntimeError):
            self.lab.key()
        self.lab.run.assert_not_called()

    def test_bootstrap_failure_writes_nothing(self):
        with patch.object(controller, "HOME", self.root / "bootstrap-home"), \
             patch.object(controller, "self_test", return_value=False), patch.object(controller.Lab, "key") as key:
            with self.assertRaises(RuntimeError):
                controller.bootstrap()
            self.assertFalse(controller.HOME.exists())
            key.assert_not_called()

    def test_bootstrap_success_never_deploys(self):
        destination = self.root / "bootstrap-home"
        with patch.object(controller, "HOME", destination), patch.object(controller, "self_test", return_value=True), \
             patch.object(controller, "Lab") as lab, patch("builtins.input", return_value="n"):
            lab.return_value.preflight.return_value = {"pass": True}
            controller.bootstrap()
            lab.return_value.deploy.assert_not_called()
            lab.return_value.pair.assert_not_called()
            lab.return_value.key.assert_called_once()
            self.assertTrue((destination / "controller/lab/controller.py").exists())

    def test_collect_sanitized_and_unknown_hardware_criteria(self):
        self.installed()
        self.log()
        self.process_info["skate"] = True
        path, published, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])
        self.assertTrue(all(report["buttons"].values()))
        self.assertIsNone(report["unexpected_crash"])
        self.assertEqual(report["observer_acceptance"], "UNVERIFIED")
        self.assertNotIn(str(self.home), path.read_text())
        self.assertNotIn("steamdeck.local", path.read_text())
        self.assertFalse(published)

    def test_collect_crash_detected(self):
        self.installed()
        path = self.log()
        path.write_text(path.read_text() + "ReSkate unhandled exception\nsecret IP 192.168.1.9\n")
        _, _, report = self.lab.collect(no_crash=True, publish=False)
        self.assertTrue(report["unexpected_crash"])
        self.assertEqual(report["observer_acceptance"], "FAIL")
        self.assertNotIn("192.168", json.dumps(report))

    def test_formatted_initialization_false_not_accepted(self):
        self.installed()
        self.log('Arcade Mode ready\nArcade mode initialized: active=false\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["arcade_mode_initialized"])

    def test_trace_json_initialization_supported(self):
        self.installed()
        self.log('Arcade Mode ready\nNative event: {"event":"arcade_mode_initialized","active":true}\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])

    def test_explicit_log_fallback_with_deploy_baseline(self):
        path = self.root / "proton/LocalAppData/ReSkate/logs/ReSkate.log"
        path.parent.mkdir(parents=True)
        self.lab.log = str(path)
        self.lab.deploy()
        path.write_text('Arcade Mode ready\n{"event":"arcade_mode_initialized","active":true}\n')
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["arcade_mode_initialized"])

    def test_mismatched_dll_cannot_claim_runtime_acceptance(self):
        self.installed()
        self.log()
        (self.game / "ReSkate.dll").write_bytes(b"different")
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])

    def test_stale_log_not_accepted(self):
        self.log()
        self.installed()
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])
        self.assertFalse(report["arcade_mode_initialized"])

    def test_appended_old_log_not_accepted(self):
        path = self.log()
        self.installed()
        path.write_text(path.read_text() + "unrelated later output\n")
        _, _, report = self.lab.collect(publish=False)
        self.assertFalse(report["fresh_log"])

    def test_latest_session_not_union_of_buttons(self):
        self.installed()
        path = self.log()
        path.write_text(path.read_text() + "Arcade Mode ready\nARCADE MODE DEV: A press #1 seen on ReSkate client tick.\n")
        _, _, report = self.lab.collect(publish=False)
        self.assertTrue(report["buttons"]["A"])
        self.assertFalse(report["buttons"]["B"])

    def test_restore_collect_retains_same_install_evidence(self):
        self.installed()
        self.log()
        self.process_info["skate"] = True
        self.lab.collect(holds=True, native=True, no_crash=True, publish=False)
        self.process_info["skate"] = False
        self.lab.remote("restore")
        _, _, report = self.lab.collect(publish=False)
        self.assertEqual(report["observer_acceptance"], "PASS")  # SIMULATED ONLY

    def test_new_install_cannot_reuse_old_acceptance(self):
        self.installed()
        self.log()
        self.lab.collect(holds=True, native=True, no_crash=True, publish=False)
        self.installed()
        self.lab.remote("restore")
        _, _, report = self.lab.collect(publish=False)
        self.assertEqual(report["observer_acceptance"], "UNVERIFIED")

    def test_publication_failure_keeps_local_report_and_dll(self):
        self.installed()
        self.log()
        with patch.object(controller, "publish_report", side_effect=FileNotFoundError("gh")):
            path, published, _ = self.lab.collect()
        self.assertTrue(path.is_file())
        self.assertFalse(published)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)

    def test_publication_pinned_orphan_branch_no_force(self):
        self.installed()
        self.log()
        _, _, report = self.lab.collect(publish=False)
        report["evil_secret"] = "do not publish"
        runner = Mock(side_effect=[subprocess.CompletedProcess([], 0, json.dumps(v)) for v in
                                  ([], {"sha": "b" * 40}, {"sha": "c" * 40}, {})])
        controller.publish_report(report, runner)
        calls = runner.call_args_list
        for call in calls:
            self.assertTrue(call.args[0][2].startswith("repos/notPho-Real/ReSkate/"))
            self.assertNotIn("evil_secret", str(call))
            self.assertNotIn("refs/heads/arcade-mode", str(call))
        self.assertEqual(json.loads(calls[2].kwargs["input"])["parents"], [])
        self.assertEqual(json.loads(calls[3].kwargs["input"])["ref"], "refs/heads/arcade-lab-results")

    def test_publication_existing_branch_fast_forward(self):
        self.installed()
        self.log()
        _, _, report = self.lab.collect(publish=False)
        values = ([{"ref": "refs/heads/arcade-lab-results", "object": {"sha": "d" * 40}}],
                  {"tree": {"sha": "e" * 40}}, {"sha": "f" * 40}, {"sha": "b" * 40}, {})
        runner = Mock(side_effect=[subprocess.CompletedProcess([], 0, json.dumps(v)) for v in values])
        controller.publish_report(report, runner)
        self.assertFalse(json.loads(runner.call_args.kwargs["input"])["force"])
        self.assertIn("PATCH", runner.call_args.args[0])


class TransactionTests(Fixture):
    def test_legacy_shell_helpers_use_shared_install_restore(self):
        source = self.dll_path.with_name("ReSkate.dll")
        source.write_bytes(self.dll)
        source.with_name("commit.txt").write_text(self.commit)
        source.with_name("ReSkate.dll.sha256").write_text(self.manifest["sha256"] + "  ReSkate.dll\n")
        binaries = self.root / "bin"
        binaries.mkdir()
        (binaries / "python3").symlink_to(sys.executable)
        pgrep = binaries / "pgrep"
        pgrep.write_text("#!/bin/sh\nexit 1\n")
        pgrep.chmod(0o755)
        env = dict(os.environ, PATH=str(binaries) + os.pathsep + os.environ.get("PATH", ""))
        subprocess.run(["bash", str(HERE / "update_arcade_dev.sh"), str(self.game), str(source)],
                       env=env, check=True, capture_output=True)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        subprocess.run(["bash", str(HERE / "restore_stock.sh"), str(self.game)],
                       env=env, check=True, capture_output=True)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_report_schema_rejects_bad_identity_and_discards_unknowns(self):
        self.installed()
        _, _, report = self.lab.collect(publish=False)
        report.update(unrelated_log="secret", unexpected_crash="secret")
        clean = controller.sanitize(report)
        self.assertNotIn("unrelated_log", clean)
        self.assertIsNone(clean["unexpected_crash"])
        self.assertEqual(clean["criteria"]["no_unexpected_crash"], "UNVERIFIED")
        report["source_commit"] = "../../bad"
        with self.assertRaises(ValueError):
            controller.sanitize(report)

    def test_invalid_manifest_values(self):
        for value in (None, {}, {"commit": [], "sha256": "a" * 64},
                      {"commit": "../bad", "sha256": "a" * 64}, {"commit": "a" * 40, "sha256": "bad"}):
            with self.assertRaises(ValueError):
                installer.validate_manifest(value)

    def test_pe_rejects_non_x64_non_dll_non_pe32plus(self):
        for offset, value in ((68, 0x14c), (86, 0), (88, 0x10b)):
            dll = bytearray(self.dll)
            struct.pack_into("<H", dll, offset, value)
            with self.assertRaises(RuntimeError):
                installer.validate_dll(bytes(dll))

    def test_ancestor_symlink_refused(self):
        link = self.root / "linked"
        link.symlink_to(self.game.parent, target_is_directory=True)
        with self.assertRaises(RuntimeError):
            installer.change_installation(link / self.game.name, self.dll, self.commit)

    def test_backup_receipt_journal_symlinks_refused(self):
        for name in (installer.BACKUP, installer.RECEIPT, installer.JOURNAL, installer.LOCK):
            path = self.game / name
            path.symlink_to(self.root / "victim")
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)
            path.unlink()
        self.assertFalse((self.root / "victim").exists())

    def test_flock_concurrency_refused(self):
        with (self.game / installer.LOCK).open("w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)

    def test_destination_corruption_rolls_back_all(self):
        before = installer.load_settings(self.game)
        real = installer.atomic_write
        once = [False]
        def corrupt(path, data):
            if path.name == "ReSkate.dll" and not once[0]:
                once[0] = True
                return real(path, b"corrupt")
            return real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=corrupt):
            with self.assertRaises(RuntimeError):
                installer.change_installation(self.game, self.dll, self.commit)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertEqual(installer.load_settings(self.game), before)
        self.assertFalse((self.game / installer.RECEIPT).exists())

    def test_interrupted_settings_write_rolls_back(self):
        original = (self.game / installer.SETTINGS).read_bytes()
        real = installer.atomic_write
        once = [False]
        def interrupt(path, data):
            if path.name == installer.SETTINGS and not once[0]:
                once[0] = True
                real(path, data)
                raise KeyboardInterrupt()
            real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=interrupt):
            with self.assertRaises(KeyboardInterrupt):
                installer.change_installation(self.game, self.dll, self.commit)
        self.assertEqual((self.game / installer.SETTINGS).read_bytes(), original)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_killed_transaction_recovers_on_explicit_restore(self):
        self.installed()
        before = {name: base64.b64encode((self.game / name).read_bytes()).decode()
                  for name in ("ReSkate.dll", installer.SETTINGS, installer.RECEIPT)}
        (self.game / installer.JOURNAL).write_text(json.dumps(before))
        (self.game / "ReSkate.dll").write_bytes(b"partial transaction")
        self.assertFalse(self.snap()["pass"])
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertFalse((self.game / installer.JOURNAL).exists())

    def test_actual_process_exit_during_transaction_recovers(self):
        code = '''import sys, os
sys.path.insert(0, sys.argv[1])
import deck_installer as i
from pathlib import Path
i.ensure_stopped = lambda: None
real = i.atomic_write
def die(path, data):
    real(path, data)
    if path.name == 'ReSkate.dll': os._exit(77)
i.atomic_write = die
i.change_installation(Path(sys.argv[2]), Path(sys.argv[3]).read_bytes(), 'a'*40)
'''
        result = subprocess.run([sys.executable, "-B", "-c", code, str(HERE), str(self.game), str(self.dll_path)])
        self.assertEqual(result.returncode, 77)
        self.assertTrue((self.game / installer.JOURNAL).exists())
        self.lab.remote("restore")
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertFalse(installer.load_settings(self.game)["updates"])

    def test_restore_write_failure_rolls_back_development(self):
        self.installed()
        real = installer.atomic_write
        def fail(path, data):
            if path.name == installer.SETTINGS:
                raise OSError("disk full")
            real(path, data)
        with patch.object(installer, "ensure_stopped"), patch.object(installer, "atomic_write", side_effect=fail):
            with self.assertRaises(OSError):
                installer.change_installation(self.game)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), self.dll)
        self.assertTrue(json.loads((self.game / installer.RECEIPT).read_text())["active"])

    def test_changed_backup_refuses_restore(self):
        self.installed()
        (self.game / installer.BACKUP).write_bytes(b"tampered")
        with patch.object(installer, "ensure_stopped"), self.assertRaises(RuntimeError):
            installer.change_installation(self.game)

    def test_changed_backup_after_restore_refuses_reinstall(self):
        self.installed()
        with patch.object(installer, "ensure_stopped"):
            installer.change_installation(self.game)
        (self.game / installer.BACKUP).write_bytes(b"tampered")
        with patch.object(installer, "ensure_stopped"), self.assertRaises(RuntimeError):
            installer.change_installation(self.game, self.dll, self.commit)

    def test_unreadable_settings_report_failed_preflight(self):
        (self.game / installer.SETTINGS).write_text("invalid JSON")
        result = self.snap()
        self.assertFalse(result["pass"])
        self.assertFalse(result["settings_readable"])

    def test_backup_exclusive_creation(self):
        path = self.game / installer.BACKUP
        path.write_bytes(b"first")
        with self.assertRaises(RuntimeError):
            installer.preserve_backup(path, b"second")
        self.assertEqual(path.read_bytes(), b"first")

    def test_release_wrong_origin_hash_commit(self):
        for channel in (dict(self.channel, url="https://github.com/other/ReSkate/fake"),
                        dict(self.channel, sha256="0" * 64), dict(self.channel, commit=None)):
            with self.assertRaises(ValueError):
                updater.verified_release(lambda url, *a: json.dumps(channel).encode() if url == updater.CHANNEL else self.desktop.read_bytes())

    def test_release_validated_before_remote_use(self):
        self.channel["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            self.lab.deploy()
        self.assertEqual(self.commands, [])

    def test_low_space_preflight_refused(self):
        with patch.object(probe.os, "statvfs", return_value=type("Space", (), {"f_bavail": 0, "f_frsize": 4096})()):
            self.assertFalse(self.snap()["pass"])

    def test_process_detection_from_fake_proc(self):
        proc = self.root / "proc"
        (proc / "123").mkdir(parents=True)
        (proc / "123/cmdline").write_bytes(b"wine\0Z:\\games\\Skate.exe\0")
        self.assertTrue(probe.processes(proc)["skate"])
        self.assertFalse(probe.processes(proc)["launcher"])


class NativeTraceIntegrationTests(Fixture):
    def test_actual_serialized_trace_and_dll_identity(self):
        self.installed()
        self.log("Arcade Mode ready\nARCADE NATIVE CAPTURE: id=1 begin=true.\n"
                 "ARCADE NATIVE SAMPLE: id=1 ms=0 valid=true state=100 crouch_mode=2 words="
                 "00000001,00000002,00000003,00000004,00000005,00000006,00000007,00000008.\n"
                 "ARCADE NATIVE CAPTURE: id=1 end=true samples=1.\n"
                 "ARCADE STICK SAMPLE: seq=3 ms=125 tick=7 user=0 x=12000 y=20000 phase=1.\n"
                 "ARCADE PLAYBACK SAMPLE: seq=3 poll=4 tick=7 mode=2 input_packet=10 output_packet=11.\n"
                 "ARCADE ACTION SAMPLE: seq=3 ms=125 tick=7 user=0 mode=2 requested=true kind=1 trick=21 result=0 owned=true lx=0 ly=0 lt=0 rt=255 packet=11.\n"
                 "ARCADE SCORE EDGE: seq=4 observed=126 sampled=125 labels=8 uncertainty=0.\n"
                 "private unrelated text\n")
        report = self.lab.remote("trace")
        self.assertEqual(report["source_commit"], self.commit)
        self.assertEqual(report["captures"][0]["samples"][0]["words"], list(range(1, 9)))
        self.assertTrue(report["captures"][0]["complete"])
        playback = report['playback_observations']['samples'][0]
        stick = report['stick_observations']['samples'][0]
        self.assertEqual((playback['seq'], playback['tick']), (stick['seq'], stick['tick']))
        self.assertEqual(playback['output_packet'], 11)
        action = report['action_observations']['samples'][0]
        self.assertEqual((action['seq'], action['ms'], action['tick']), (stick['seq'], stick['ms'], stick['tick']))
        self.assertTrue(action['requested'])
        self.assertTrue(action['owned'])
        self.assertEqual(action['packet'], 11)
        self.assertEqual(report['score_edge_observations']['samples'][0],
                         dict(seq=4, observed=126, sampled=125, labels=8, uncertainty=0))
        self.assertNotIn("private", json.dumps(report))
        (self.game / "ReSkate.dll").write_bytes(b"wrong DLL")
        with self.assertRaises(ValueError):
            self.lab.remote("trace")


class NativeTraceTests(unittest.TestCase):
    def test_numeric_allowlist_and_invalid_owner(self):
        text = "ARCADE NATIVE CAPTURE: id=2 begin=true.\n"
        text += "unrelated secret must never appear\n"
        text += "ARCADE NATIVE SAMPLE: id=2 ms=100 valid=false state=103 crouch_mode=2 words=" + ",".join(["ffffffff"]*8) + ".\n"
        text += "ARCADE NATIVE CAPTURE: id=2 end=true samples=1."
        result = probe.parse_native_captures(text)
        self.assertTrue(result[0]["complete"])
        self.assertEqual(result[0]["samples"][0]["words"], [0]*8)
        self.assertEqual(result[0]["samples"][0]["state"], 0)
        self.assertNotIn("secret", json.dumps(result))

    def test_capture_and_sample_bounds(self):
        text = ""
        for capture in range(10):
            text += f"ARCADE NATIVE CAPTURE: id={capture} begin=true.\n"
            text += (f"ARCADE NATIVE SAMPLE: id={capture} ms=1 valid=true state=103 crouch_mode=2 words=" + ",".join(["00000001"]*8) + ".\n")*130
        result = probe.parse_native_captures(text)
        self.assertEqual(len(result), 8)
        self.assertEqual(len(result[-1]["samples"]), 128)
        self.assertFalse(result[-1]["complete"])

    def test_wrong_id_and_expired_sample_rejected(self):
        text = "ARCADE NATIVE CAPTURE: id=2 begin=true.\n"
        for identity, milliseconds in ((1, 10), (2, 5000)):
            text += f"ARCADE NATIVE SAMPLE: id={identity} ms={milliseconds} valid=true state=100 crouch_mode=0 words=" + ",".join(["00000000"]*8) + ".\n"
        self.assertEqual(probe.parse_native_captures(text)[0]["samples"], [])

    def test_trace_transport_is_read_only(self):
        code = controller.remote_code("trace")
        self.assertNotIn("change_installation", code)
        self.assertIn("native_trace(json.load(sys.stdin))", code)
        with patch.object(probe, "evidence", return_value={"fresh_log": False}):
            with self.assertRaises(ValueError):
                probe.native_trace({})


if __name__ == "__main__":
    unittest.main()
