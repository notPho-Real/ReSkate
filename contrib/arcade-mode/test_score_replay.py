"""Offline replay of provenance-tagged native captures; no hardware access."""
import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'lab'))
import score_replay


class ScoreReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.evidence = json.loads((ROOT / 'research/score-lifecycle-replay-input.json').read_text())

    def test_recorded_mechanic_passes_do_not_prove_monetary_ownership(self):
        result = score_replay.report(self.evidence)
        self.assertEqual(len(result['trials']), 7)
        self.assertFalse(result['new_hardware_acceptance'])
        for item in result['trials']:
            row = item['recorded']
            self.assertEqual(row['monetary_status'], 'UNVERIFIED')
            self.assertIsNone(row['banked_points'])
            self.assertIsNone(row['lost_points'])
            self.assertEqual(row['authoritative_operations'], 0)
            self.assertIn('payload_owner_unverified', row['reasons'])
            self.assertEqual(len(item['provenance']['result_sha256']), 64)
        self.assertEqual(result['trials'][0]['recorded']['recorded_chain']['continuous'], 'PASS')
        self.assertEqual(result['trials'][1]['recorded']['recorded_chain']['continuous'], 'PASS')
        self.assertEqual(result['trials'][2]['recorded']['physical_bail'], 'PASS')
        self.assertEqual(result['trials'][5]['recorded']['recorded_chain']['continuous'], 'UNVERIFIED')
        self.assertEqual(result['trials'][6]['recorded']['recorded_chain']['continuous'], 'FAIL')
        self.assertIn('telemetry_incomplete', result['trials'][5]['recorded']['reasons'])
        self.assertIn('missing_score_events', result['trials'][4]['recorded']['reasons'])

    def test_fault_injection_never_becomes_new_native_acceptance(self):
        for item in self.evidence['trials']:
            original = copy.deepcopy(item['trial'])
            faults = list(score_replay.fault_cases(item['trial']))
            self.assertEqual(len(faults), 5)
            self.assertEqual(item['trial'], original)
            for fault in faults:
                self.assertTrue(fault['synthetic_mutation'])
                self.assertEqual(fault['result']['monetary_status'], 'UNVERIFIED')
                self.assertEqual(fault['result']['authoritative_operations'], 0)
                self.assertIsNone(fault['result']['recorded_chain'])
            if original['observations'].get('native_score', {}).get('samples'):
                self.assertEqual(faults[0]['result']['exact_capture_replays'], 1)
            self.assertIn('missing_score_events', faults[1]['result']['reasons'])
            self.assertIn('telemetry_incomplete', faults[2]['result']['reasons'])

    def test_equal_points_are_not_native_identity_and_modified_owner_is_rejected(self):
        trial = copy.deepcopy(self.evidence['trials'][3]['trial'])
        rows = trial['observations']['native_score']['samples']
        extra = copy.deepcopy(rows[-1]); extra['seq'] += 1; extra['ms'] += 1
        rows.append(extra)
        self.assertEqual(score_replay.replay_trial(trial)['exact_capture_replays'], 0)
        rows[-1]['owner_verified'] = True
        self.assertIn('unsupported_or_malformed_capture', score_replay.replay_trial(trial)['reasons'])
        rows[-1]['owner_verified'] = False
        rows[-1]['seq'] = rows[0]['seq']
        self.assertIn('conflicting_or_reordered_capture', score_replay.replay_trial(trial)['reasons'])

    def test_input_and_hud_fields_cannot_supply_missing_score(self):
        trial = copy.deepcopy(self.evidence['trials'][4]['trial'])
        trial['observations']['score_numbers'] = {'score': 9999, 'multiplier': 20}
        trial['observations']['requested'] = 'bank'
        result = score_replay.replay_trial(trial)
        self.assertIsNone(result['banked_points'])
        self.assertIn('missing_score_events', result['reasons'])

    def test_revert_replay_uses_existing_grader(self):
        trial = self.evidence['trials'][3]['trial']
        result = score_replay.replay_trial(trial)
        self.assertEqual(result['revert_link'], score_replay.native_revert_link_result(trial['observations']))
        self.assertNotEqual(result['revert_link']['revert_continuation'], 'PASS')

    def test_cpp_replay_columns_exactly_match_the_original_captures(self):
        expected = []
        for index, item in enumerate(self.evidence['trials']):
            for row in item['trial']['observations'].get('native_score', {}).get('samples', []):
                expected.append([index, row['seq'], row['ms'], row['epoch'], row['route'],
                                 row['score'], row['reason'], int(row['success']), int(row['owner_verified'])])
        lines = (ROOT / 'research/score-lifecycle-captures.tsv').read_text().splitlines()
        actual = []
        for line in lines:
            columns = line.split('\t')
            self.assertEqual(len(columns), 9)
            actual.append([float(value) if i == 5 else int(value) for i, value in enumerate(columns)])
        self.assertEqual(actual, expected)


if __name__ == '__main__':
    unittest.main()
