"""Synthetic wire-contract tests; no native occurrence or hardware acceptance."""
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent / 'lab'))
import probe
import trick_batch


def line(**changes):
    fields = dict(seq=1, ms=1000, status=0, available=True, comparable=False, rows=2, manual=1, nose=0,
                  distinct=2, manual_distinct=1, nose_distinct=0, manual_added=0, manual_removed=0,
                  nose_added=0, nose_removed=0, duplicates=0, relabels=0)
    fields.update(changes)
    return '[skater] ARCADE MANUAL HISTORY: ' + ' '.join(
        f'{k}={str(v).lower() if type(v) is bool else v}' for k, v in fields.items()) + '.'


class ManualHistoryProbeTests(unittest.TestCase):
    def test_existing_observations_expose_diagnostic_without_admission(self):
        text = line()
        parsed = probe.parse_manual_history(text)
        self.assertEqual(trick_batch.observations(text)['score_labels']['manual_history'], parsed)
        self.assertEqual(parsed['samples'][0]['manual'], 1)
        self.assertEqual(parsed['samples'][0]['status'], 'baseline')
        self.assertFalse(parsed['authoritative'])
        self.assertEqual(parsed['native_identity'], 'UNVERIFIED')
        self.assertEqual(parsed['capture_completeness'], 'UNVERIFIED')
        self.assertEqual(trick_batch.observations(text)['score_edges']['samples'], [])

    def test_same_label_count_increase_duplicate_relabel_and_removal_are_only_diagnostics(self):
        text = '\n'.join((line(), line(seq=2, ms=1050, status=1, comparable=True,
            rows=3, manual=2, distinct=3, manual_distinct=2, manual_added=1),
            line(seq=3, ms=1100, status=1, comparable=True, rows=4, manual=3, distinct=3,
                 manual_distinct=2, duplicates=1),
            line(seq=4, ms=1150, status=1, comparable=True, nose=1, nose_distinct=1,
                 manual_removed=1, nose_added=1, relabels=1),
            line(seq=5, ms=1200, status=1, comparable=True, rows=0, manual=0, distinct=0,
                 manual_distinct=0, manual_removed=1, nose_removed=1)))
        result = probe.parse_manual_history(text)
        self.assertEqual(len(result['samples']), 5)
        self.assertEqual(result['malformed'], 0)
        self.assertEqual(result['samples'][1]['manual_added'], 1)
        self.assertEqual(result['samples'][2]['duplicates'], 1)
        self.assertEqual(result['samples'][3]['relabels'], 1)
        self.assertEqual(result['samples'][4]['manual'], 0)

    def test_all_noncomparative_statuses_keep_absence_distinct_from_zero(self):
        for status in range(7):
            extra = dict(status=status, comparable=status == 1, available=status <= 4)
            if status > 4:
                extra.update(rows=0, manual=0, distinct=0, manual_distinct=0)
            result = probe.parse_manual_history(line(**extra))
            with self.subTest(status=status):
                self.assertEqual(result['malformed'], 0)
                self.assertEqual(result['samples'][0]['manual'], 1 if status <= 4 else None)
        self.assertEqual(probe.parse_manual_history('unrelated log')['presence'], 'missing')

    def test_malformed_bounds_consistency_and_raw_fields_are_rejected(self):
        cases = [dict(seq=0), dict(seq=2**64), dict(ms=2**64), dict(status=7), dict(available=1),
                 dict(comparable=True), dict(rows=17), dict(manual=3), dict(nose=2), dict(distinct=1),
                 dict(manual_distinct=2), dict(nose_distinct=1), dict(manual_added=1),
                 dict(status=1, comparable=True, manual_added=2), dict(status=1, comparable=True, relabels=3),
                 dict(status=5, available=False), dict(status=5, available=False, comparable=True)]
        texts = [line(**c) for c in cases]
        texts += [line().replace('ms=1000', 'ms=-1'), line().replace('manual=1', 'manual=private'),
                  line()[:-1] + ' handle=123.', line().replace(' nose=0', ''), line() + ' arbitrary text']
        for text in texts:
            with self.subTest(text=text):
                result = probe.parse_manual_history(text)
                self.assertEqual(result['samples'], [])
                self.assertEqual(result['malformed'], 1)
                self.assertEqual(result['presence'], 'invalid')
                self.assertNotIn('private', json.dumps(result))
                self.assertNotIn('handle', json.dumps(result))

    def test_sequence_gaps_duplicate_reordered_rows_and_clock_reset_remain_visible(self):
        text = '\n'.join((line(), line(seq=2, ms=1050, status=1, comparable=True),
                          line(seq=2, ms=1050, status=1, comparable=True),
                          line(seq=1, ms=1000), line(seq=4, ms=1150, status=1, comparable=True),
                          line(seq=5, ms=100, status=1, comparable=True),
                          line(seq=6, ms=100, status=3),
                          line(seq=7, ms=150, status=1, comparable=True)))
        result = probe.parse_manual_history(text)
        self.assertEqual([r['seq'] for r in result['samples']], [1, 2, 4, 6, 7])
        self.assertEqual(result['malformed'], 3)
        self.assertEqual(result['sequence_gaps'], 2)
        self.assertEqual(result['sequence_missing'], 2)
        self.assertEqual(result['samples'][-2]['status'], 'clock_reset')

    def test_malformed_intermediate_row_does_not_hide_a_wire_gap(self):
        result = probe.parse_manual_history('\n'.join((line(), line(seq=2, manual='bad'),
                                                      line(seq=3, ms=1100, status=1, comparable=True))))
        self.assertEqual(result['malformed'], 1)
        self.assertEqual(result['sequence_missing'], 1)

    def test_sample_cap_and_sampled_heartbeat_do_not_infer_complete_capture(self):
        text = '\n'.join(line(seq=i+1, ms=1000+i*1000, status=1, comparable=True) for i in range(130))
        result = probe.parse_manual_history(text)
        self.assertEqual(len(result['samples']), 128)
        self.assertEqual(result['dropped'], 2)
        self.assertEqual(result['sequence_missing'], 0)
        self.assertEqual(result['malformed'], 0)
        self.assertEqual(result['capture_completeness'], 'UNVERIFIED')


if __name__ == '__main__':
    unittest.main()
