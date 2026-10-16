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
        self.assertEqual(len(result['trials']), 15)
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
        self.assertIsNone(result['trials'][0]['recorded']['revert_link'])
        self.assertIsNotNone(result['trials'][3]['recorded']['revert_link'])

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

    def test_recorded_native_timer_end_cannot_bank_an_expired_arcade_combo(self):
        trial = self.evidence['trials'][8]['trial']
        rows = trial['observations']['native_score']['samples']
        start = next(row for row in rows if row['route'] == 1)
        end = next(row for row in rows if row['route'] == 2)
        self.assertEqual(end['reason'], 1)  # Observed identifier, not a guessed reason name.
        self.assertEqual(end['ms'] - start['ms'], 10034)
        inactive = next(e for e in trial['observations']['combo']['events'] if not e['active'])
        self.assertGreater(end['ms'] - inactive['ms'], 9000)
        result = score_replay.replay_trial(trial)
        self.assertEqual(result['authoritative_operations'], 0)
        self.assertIsNone(result['banked_points'])

    def test_recorded_post_bail_success_payload_is_not_a_clean_bank(self):
        for item in self.evidence['trials'][9:11]:
            trial = item['trial']
            bail = next(e for e in trial['observations']['combo']['events'] if e['pose'] == 6)
            self.assertTrue(any(row['ms'] > bail['ms'] and row['success'] and row['score'] > 0
                                for row in trial['observations']['native_score']['samples']))
            result = score_replay.replay_trial(trial)
            self.assertEqual(result['physical_bail'], 'PASS')
            self.assertEqual(result['authoritative_operations'], 0)
            self.assertIsNone(result['banked_points'])
            self.assertIsNone(result['lost_points'])

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
        result = score_replay.replay_trial(trial, evaluate_revert_link=True)
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

    def diagnostic_trial(self):
        return copy.deepcopy(self.evidence['trials'][13]['trial'])

    def test_new_recorded_details_are_separate_trials_without_monetary_authority(self):
        for index, item in enumerate(self.evidence['trials'][11:]):
            self.assertEqual(item['provenance']['trial_index'], index)
            self.assertEqual(item['provenance']['result'], '1792158343465402000')
            self.assertEqual(item['provenance']['result_sha256'],
                             '026606de69e72192b737b2d6b23cae89c7be3d899138c20ba1fba6f8714424e1')
            result = score_replay.replay_trial(item['trial'])
            self.assertEqual(result['monetary_status'], 'UNVERIFIED')
            self.assertNotEqual(result['native_diagnostic_evidence'], 'COMPLETE')
            self.assertEqual(result['authoritative_operations'], 0)
        unbound = score_replay.replay_trial(self.evidence['trials'][12]['trial'])
        self.assertIn('observer_unbound', unbound['reasons'])
        self.assertEqual(unbound['native_diagnostic_evidence'], 'INCOMPLETE')
        result = score_replay.replay_trial(self.diagnostic_trial())
        self.assertEqual(result['sequence_details']['matched_details'], 3)
        self.assertEqual(result['sequence_details']['missing_details'], 0)
        self.assertEqual(result['sequence_details']['association'], 'capture_serial_only')

    def test_only_sequence_end_requires_details_and_missing_never_means_zero(self):
        no_sequence = score_replay.replay_trial(self.evidence['trials'][11]['trial'])
        self.assertEqual(no_sequence['sequence_details']['status'], 'NOT_APPLICABLE')
        trial = self.diagnostic_trial()
        trial['observations']['native_score']['details'].pop()
        result = score_replay.replay_trial(trial)
        self.assertEqual(result['sequence_details']['missing_details'], 1)
        self.assertEqual(result['sequence_details']['status'], 'UNVERIFIED')
        self.assertIn('missing_sequence_details', result['reasons'])
        self.assertIsNone(result['banked_points'])

    def test_detail_drop_and_malformed_counters_are_independently_incomplete(self):
        for field in ('detail_dropped', 'detail_malformed', 'health_dropped', 'health_malformed'):
            with self.subTest(field=field):
                trial = self.diagnostic_trial()
                trial['observations']['native_score'][field] = 1
                result = score_replay.replay_trial(trial)
                self.assertIn(field, result['reasons'])
                self.assertIn('telemetry_incomplete', result['reasons'])
                self.assertEqual(result['native_diagnostic_evidence'], 'INCOMPLETE')
                self.assertEqual(result['authoritative_operations'], 0)

    def test_malformed_detail_values_duplicates_orphans_and_caps_fail_closed(self):
        for mutation in ('owner', 'nan', 'bool', 'missing', 'duplicate', 'orphan', 'cap', 'container', 'huge_integer'):
            with self.subTest(mutation=mutation):
                trial = self.diagnostic_trial()
                native = trial['observations']['native_score']; detail = native['details'][0]
                if mutation == 'owner': detail['authoritative'] = True
                if mutation == 'nan': detail['landing'] = float('nan')
                if mutation == 'bool': detail['current_category'] = True
                if mutation == 'missing': del detail['action_score']
                if mutation == 'duplicate': native['details'].append(copy.deepcopy(detail))
                if mutation == 'orphan': detail['seq'] = 9999
                if mutation == 'cap': native['details'] = [detail] * 129
                if mutation == 'container': native['details'] = {'seq': 46}
                if mutation == 'huge_integer': detail['landing'] = 10**1000
                result = score_replay.replay_trial(trial)
                self.assertEqual(result['sequence_details']['status'], 'INCOMPLETE')
                self.assertEqual(result['monetary_status'], 'UNVERIFIED')

    def test_unavailable_detail_fields_remain_null_and_unverified(self):
        trial = self.diagnostic_trial(); detail = trial['observations']['native_score']['details'][0]
        detail.update(scalar_status='read_failed', category_status='absent', sequence_kind=None,
                      action_score=None, base_score=None, landing=None, current_category=None)
        result = score_replay.replay_trial(trial)
        self.assertIn('detail_fields_unverified', result['reasons'])
        self.assertEqual(result['sequence_details']['status'], 'UNVERIFIED')
        self.assertIsNone(detail['action_score'])

    def test_sampled_health_distinguishes_history_from_between_sample_loss(self):
        trial = self.diagnostic_trial(); native = trial['observations']['native_score']
        row = dict(ms=10, epoch=125, active=True, bound=True, route=4,
                   unbound=7, expired=0, changed=0, overflow=False)
        native.update(health=[row], health_dropped=0, health_malformed=0)
        result = score_replay.replay_trial(trial)
        self.assertIn('health_loss_history_unlocated', result['reasons'])
        self.assertNotIn('health_loss_between_samples', result['reasons'])
        self.assertEqual(result['native_diagnostic_evidence'], 'UNVERIFIED')
        native['health'].append(dict(row, ms=20, route=2, unbound=99))
        self.assertNotIn('health_loss_between_samples', score_replay.replay_trial(trial)['reasons'])
        native['health'].append(dict(row, ms=30, unbound=8))
        result = score_replay.replay_trial(trial)
        self.assertIn('health_loss_between_samples', result['reasons'])
        self.assertEqual(result['native_diagnostic_evidence'], 'INCOMPLETE')

    def test_health_false_bound_overflow_malformed_and_counter_reset_are_explicit(self):
        for mutation, expected in [('bound', 'observer_unbound'), ('overflow', 'health_counter_overflow'),
                                   ('bad', 'malformed_capture_health'), ('reset', 'health_counter_reset_or_reordered'),
                                   ('epoch', 'health_generation_changed')]:
            with self.subTest(mutation=mutation):
                trial = self.diagnostic_trial(); native = trial['observations']['native_score']
                row = dict(ms=10, epoch=125, active=True, bound=True, route=4,
                           unbound=2, expired=0, changed=0, overflow=False)
                native.update(health=[row], health_dropped=0, health_malformed=0)
                if mutation == 'bound': row['bound'] = False
                if mutation == 'overflow': row['overflow'] = True
                if mutation == 'bad': row['unbound'] = -1
                if mutation == 'reset': native['health'].append(dict(row, ms=20, unbound=0))
                if mutation == 'epoch': native['health'].append(dict(row, ms=20, epoch=126))
                result = score_replay.replay_trial(trial)
                self.assertIn(expected, result['reasons'])
                self.assertEqual(result['native_diagnostic_evidence'], 'INCOMPLETE')


if __name__ == '__main__':
    unittest.main()
