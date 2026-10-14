"""Offline playback evidence analysis; no hardware or runner changes."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent / 'lab'))
import playback_report as report


def trial():
    return {'neutralized': True, 'criteria': {'returned_to_idle': 'PASS', 'named_trick': 'PASS'},
            'observations': {
                'playback': {'dropped': 0, 'samples': [
                    dict(seq=i + 10, poll=i + 100, tick=i + 20, mode=2,
                         input_packet=1, output_packet=(report.U32 + i) & report.U32) for i in range(3)]},
                'stick': {'dropped': 0, 'samples': [
                    dict(seq=i + 10, ms=100 + i * 17, tick=i + 20, user=0,
                         x=0, y=-32767 if i == 0 else 0, phase=1) for i in range(3)]},
                'flip': {'dropped': 0, 'samples': [dict(ms=120, valid=True, id=9)]},
                'pad': {'dropped': 0, 'samples': []}, 'evidence_gap': None}}


class PlaybackReportTests(unittest.TestCase):
    def test_complete_observed_timing_and_wrap_are_not_a_requested_trick_verdict(self):
        result = report.analyze_trial(trial())
        self.assertTrue(result['evidence_complete'])
        self.assertEqual(result['observed_native_ids'], [9])
        self.assertEqual(result['fresh_native_ids'], [9])
        self.assertEqual(result['ms_per_tick'], 17)
        self.assertEqual(result['poll_count_span'], 3)
        self.assertEqual(result['output_packet_wraps'], 1)
        self.assertEqual(len(result['changed_xy']), 2)
        self.assertNotIn('named_trick', result)
        self.assertTrue(result['neutralized'])
        self.assertEqual(result['reported_reset'], 'PASS')

    def test_missing_join_and_different_tick_are_incomplete(self):
        data = trial()
        data['observations']['stick']['samples'].pop(1)
        self.assertIn('missing_stick_join', report.analyze_trial(data)['incomplete_reasons'])
        data = trial()
        data['observations']['stick']['samples'][1]['tick'] += 1
        self.assertIn('mismatched_join_tick', report.analyze_trial(data)['incomplete_reasons'])

    def test_drops_and_log_gap_are_explicit(self):
        for name in ('playback', 'stick', 'flip', 'pad'):
            data = trial()
            data['observations'][name]['dropped'] = 1
            result = report.analyze_trial(data)
            self.assertFalse(result['evidence_complete'])
            self.assertEqual(result['dropped'][name], 1)
        data = trial()
        data['observations']['evidence_gap'] = 'private arbitrary value'
        result = report.analyze_trial(data)
        self.assertIn('evidence_gap', result['incomplete_reasons'])
        self.assertNotIn('private', json.dumps(result))

    def test_mixed_modes_users_and_reversed_clocks(self):
        for stream, field, value, reason in (
                ('playback', 'mode', 1, 'mixed_modes'), ('stick', 'user', 1, 'mixed_users'),
                ('playback', 'seq', 0, 'nonmonotonic_playback_seq'),
                ('playback', 'poll', 0, 'nonmonotonic_playback_poll'),
                ('playback', 'tick', 0, 'nonmonotonic_playback_tick'),
                ('stick', 'ms', 0, 'nonmonotonic_stick_ms')):
            data = trial()
            data['observations'][stream]['samples'][1][field] = value
            self.assertIn(reason, report.analyze_trial(data)['incomplete_reasons'])

    def test_native_rows_must_be_valid_and_fresh(self):
        for row in (dict(ms=99, valid=True, id=9), dict(ms=135, valid=True, id=9),
                    dict(ms=120, valid=False, id=9), dict(ms=120, valid=True, id=0)):
            data = trial()
            data['observations']['flip']['samples'] = [row]
            result = report.analyze_trial(data)
            self.assertFalse(result['evidence_complete'])
            self.assertEqual(result['fresh_native_ids'], [])

    def test_old_artifacts_remain_incomplete_despite_success_criteria(self):
        data = trial()
        del data['observations']['playback']
        result = report.analyze_trial(data)
        self.assertFalse(result['evidence_complete'])
        self.assertIn('missing_playback', result['incomplete_reasons'])
        self.assertEqual(result['observed_native_ids'], [9])

    def test_trials_are_never_pooled_and_private_fields_never_escape(self):
        first, second = trial(), trial()
        first['observations']['flip']['samples'] = []
        second['observations']['playback']['samples'] = []
        first['name'] = 'private'
        first['frames'] = [{'image_base64': 'private'}]
        result = report.analyze_document({'trials': [first, second]})
        self.assertTrue(all(not item['evidence_complete'] for item in result['trials']))
        self.assertEqual(result['trials'][0]['observed_native_ids'], [])
        self.assertEqual(result['trials'][1]['observed_native_ids'], [9])
        self.assertNotIn('private', json.dumps(result))

    def test_invalid_numeric_and_oversized_rows_are_not_silently_complete(self):
        data = trial()
        data['observations']['playback']['samples'][0]['input_packet'] = report.U32 + 1
        self.assertIn('invalid_playback', report.analyze_trial(data)['incomplete_reasons'])
        data = trial()
        data['observations']['stick']['samples'] *= 44
        self.assertIn('oversized_stick', report.analyze_trial(data)['incomplete_reasons'])

    def test_missing_loss_counter_and_reference_packet_repetition(self):
        data = trial()
        del data['observations']['pad']['dropped']
        self.assertIn('unknown_pad_loss', report.analyze_trial(data)['incomplete_reasons'])
        data = trial()
        data['observations']['playback']['samples'][1]['output_packet'] = report.U32
        self.assertIn('reference_packet_poll_mismatch', report.analyze_trial(data)['incomplete_reasons'])

    def test_reference_exit_packet_returns_to_native_without_false_loss(self):
        data = trial()
        data['observations']['stick']['samples'][-1]['phase'] = 0
        data['observations']['playback']['samples'][-1]['output_packet'] = 24
        self.assertTrue(report.analyze_trial(data)['evidence_complete'])

    def test_same_tick_samples_and_packet_poll_mismatch(self):
        data = trial()
        data['observations']['playback']['samples'][1]['tick'] = 20
        data['observations']['stick']['samples'][1]['tick'] = 20
        self.assertEqual(report.analyze_trial(data)['same_tick_samples'], 1)
        data['observations']['playback']['samples'][2]['poll'] = 104
        self.assertIn('reference_packet_poll_mismatch', report.analyze_trial(data)['incomplete_reasons'])

    def test_four_polls_per_tick_are_complete_sampled_cadence(self):
        data = trial()
        for i, row in enumerate(data['observations']['playback']['samples']):
            row['poll'] = 100 + i * 4
            row['output_packet'] = (report.U32 - 2 + i * 4) & report.U32
        result = report.analyze_trial(data)
        self.assertTrue(result['evidence_complete'])
        self.assertEqual(result['poll_count_span'], 9)
        self.assertEqual(result['additional_polls'], 6)
        self.assertEqual(result['output_packet_wraps'], 1)
        self.assertEqual(result['same_tick_samples'], 0)
        data['observations']['playback']['samples'][1]['output_packet'] += 1
        result = report.analyze_trial(data)
        self.assertFalse(result['evidence_complete'])
        self.assertIn('reference_packet_poll_mismatch', result['incomplete_reasons'])
        self.assertEqual(result['output_packet_wraps'], 0)

    def test_timeline_sampled_poll_counts_do_not_imply_lost_evidence(self):
        data = trial()
        for i, row in enumerate(data['observations']['playback']['samples']):
            row['mode'] = 1
            row['poll'] = 100 + i * 4
        result = report.analyze_trial(data)
        self.assertTrue(result['evidence_complete'])
        self.assertEqual(result['additional_polls'], 6)

    def test_trial_count_is_bounded_before_output_expansion(self):
        self.assertEqual(len(report.analyze_document([{}] * 24)['trials']), 24)
        with self.assertRaises(ValueError):
            report.analyze_document([{}] * 25)
        with self.assertRaises(ValueError):
            report.analyze_document({'trials': [{}] * 25})

    def test_cli_has_bounded_read_and_whitelisted_output(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'trial.json'
            path.write_text(json.dumps(trial()))
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(report.main([str(path)]), 0)
            self.assertTrue(json.loads(output.getvalue())['trials'][0]['evidence_complete'])
            with patch.object(report, 'MAX_FILE', 4), self.assertRaises(ValueError):
                report.main([str(path)])
        with self.assertRaises(ValueError):
            report.analyze_document('private')


if __name__ == '__main__':
    unittest.main()
