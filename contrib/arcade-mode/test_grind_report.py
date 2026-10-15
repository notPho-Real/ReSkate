"""Offline denominator and native-evidence checks; no hardware acceptance."""
import copy
import unittest
from lab.grind_report import report


def trial():
    return dict(name='native-control', repetition=1, input_sent=True, neutralized=True,
                fixture_start={'reset_ready': True}, fixture_reset={'reset_ready': True},
                observations=dict(grind_result={'evidence': 'COMPLETE', 'native_grind': 'PASS'},
                                  grind={'samples': [dict(valid=True, state=100, edge=0, duration=0)]}))


class GrindReportTests(unittest.TestCase):
    def test_intent_or_success_label_cannot_create_native_grind(self):
        result = report([{'trials': [trial()]}])
        self.assertEqual(result['valid_attempts'], 1)
        self.assertEqual(result['native_grinds'], 0)
        self.assertEqual(result['contact_classification'], 'UNVERIFIED')

    def test_invalid_attempts_and_hard_stop_do_not_inflate_denominator(self):
        missing = trial()
        missing['observations']['grind_result']['evidence'] = 'INCOMPLETE'
        skipped = dict(input_sent=False)
        result = report([{'trials': [missing, skipped, {'hard_stop': True, 'reason': 'grind_evidence_incomplete'}]}])
        self.assertEqual(result['input_attempts'], 1)
        self.assertEqual(result['excluded_attempts'], 1)
        self.assertEqual(result['valid_attempts'], 0)
        self.assertIsNone(result['native_grind_rate'])

    def test_native_grind_exit_and_bail_remain_separate_from_reset(self):
        value = trial()
        rows = value['observations']['grind']['samples']
        rows.extend([dict(valid=True, state=401, edge=1, duration=0, grind_valid=True, grind=7,
                          skate_valid=False, skate=0, speed_valid=True, speed_mm_s=5200),
                     dict(valid=True, state=300, edge=2, duration=830),
                     dict(valid=True, state=100, edge=0, duration=0)])
        value['fixture_reset']['reset_ready'] = False
        result = report([{'trials': [value]}])
        self.assertEqual((result['valid_attempts'], result['native_grinds'], result['native_exits'],
                          result['observed_bails'], result['reset_failures']), (1, 1, 1, 1, 1))
        measured = result['batches'][0]['trials'][0]
        self.assertEqual(measured['raw_enum_pairs'], [(7, None)])
        self.assertEqual(measured['longest_grind_ms'], 830)
        self.assertEqual(measured['named_identity'], 'UNKNOWN')
        self.assertTrue(measured['riding_after_exit'])

    def test_missing_or_failed_fixture_baseline_is_excluded(self):
        value = trial()
        for baseline in ({}, {'reset_ready': False}):
            changed = copy.deepcopy(value)
            changed['fixture_start'] = baseline
            self.assertEqual(report([{'trials': [changed]}])['valid_attempts'], 0)
