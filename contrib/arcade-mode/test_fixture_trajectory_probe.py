import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / 'lab'))
from probe import parse_fixture_trajectory


def row(**overrides):
    fields = dict(session=1, seq=1, ms=1000, sampled=999, generation=2, revision=3,
                  status=2, dropped=0, x_mm=-1234, y_mm=20, z_mm=3000,
                  speed_mm_s=2500, heading_mdeg=90000, state=201)
    fields.update(overrides)
    return 'prefix ARCADE FIXTURE TRAJECTORY: ' + ' '.join(f'{k}={v}' for k, v in fields.items()) + '.'


def terminal(**overrides):
    fields = dict(seq=2, ms=13000, sampled=0, status=3, x_mm=0, y_mm=0, z_mm=0,
                  speed_mm_s=0, heading_mdeg=0, state=0)
    fields.update(overrides)
    return row(**fields)


def reason_row(**overrides):
    fields = dict(session=1, seq=2, ms=13000, status=4, reason=9)
    fields.update(overrides)
    return 'ARCADE FIXTURE TRAJECTORY REASON: ' + ' '.join(f'{k}={v}' for k, v in fields.items()) + '.'


class FixtureTrajectoryProbeTests(unittest.TestCase):
    def test_terminal_reason_joins_exact_receipt_without_inventing_values(self):
        legacy = parse_fixture_trajectory(row() + '\n' + terminal(status=4))
        self.assertIsNone(legacy['samples'][-1]['reason'])
        self.assertEqual(legacy['reason_diagnostics'], [])
        for reason, name in ((1, 'context_changed'), (2, 'context_invalid'), (3, 'sample_invalid'),
                             (9, 'heading_invalid'), (10, 'position_invalid')):
            result = parse_fixture_trajectory(row() + '\n' + terminal(status=4) + '\n' + reason_row(reason=reason))
            self.assertEqual(result['malformed'], 0)
            self.assertEqual(result['samples'][-1]['reason'], name)
            self.assertIsNone(result['samples'][-1]['heading_mdeg'])
            self.assertIsNone(result['samples'][-1]['sampled'])
            self.assertTrue(result['evidence_gap'])
            self.assertEqual(result['owner_proof'], 'UNVERIFIED')

    def test_reason_bounds_identity_duplicates_and_status_mismatch_fail_closed(self):
        prefix = row() + '\n' + terminal(status=4) + '\n'
        for changes in (dict(reason=0), dict(reason=14), dict(reason=6), dict(reason=-1), dict(session=2),
                        dict(seq=3), dict(ms=13001), dict(status=3), dict(ms=2**64), dict(raw=123)):
            result = parse_fixture_trajectory(prefix + reason_row(**changes))
            self.assertEqual(result['malformed'], 1)
            self.assertIsNone(result['samples'][-1]['reason'])
        result = parse_fixture_trajectory(prefix + reason_row() + '\n' + reason_row())
        self.assertEqual(result['malformed'], 1)
        self.assertEqual(len(result['reason_diagnostics']), 1)
        self.assertEqual(parse_fixture_trajectory(reason_row())['malformed'], 1)

    def test_reason_gap_timeout_cancel_and_output_cap(self):
        for status, reason in ((5, 4), (5, 6), (6, 12), (7, 13)):
            result = parse_fixture_trajectory(row() + '\n' + terminal(status=status) + '\n' +
                                              reason_row(status=status, reason=reason))
            self.assertEqual(result['malformed'], 0)
        text = '\n'.join(row(session=i) + '\n' + terminal(session=i, status=4) + '\n' + reason_row(session=i)
                         for i in range(1, 130))
        result = parse_fixture_trajectory(text)
        self.assertEqual(len(result['reason_diagnostics']), 128)
        self.assertEqual(result['reason_dropped'], 1)
        self.assertTrue(result['evidence_gap'])
    def test_relative_samples_and_terminal_are_diagnostic_only(self):
        r = parse_fixture_trajectory(row() + '\n' + terminal())
        self.assertEqual(r['samples'][0]['x_mm'], -1234)
        self.assertEqual(r['samples'][0]['sampled'], 999)
        self.assertTrue(r['samples'][0]['valid'])
        self.assertEqual(r['samples'][1]['status'], 'complete')
        self.assertIsNone(r['samples'][1]['x_mm'])
        self.assertFalse(r['evidence_gap'])
        self.assertEqual(r['owner_proof'], 'UNVERIFIED')
        self.assertEqual(r['capture_completeness'], 'UNVERIFIED')

    def test_bounds_stale_invalid_and_unknown_fields(self):
        bad = [dict(session=0), dict(seq=0), dict(seq=483), dict(generation=0), dict(status=1),
               dict(sampled=1001), dict(sampled=849), dict(x_mm=100001), dict(y_mm=20001),
               dict(z_mm=-100001), dict(speed_mm_s=50001), dict(heading_mdeg=360001),
               dict(state=10001), dict(dropped=1), dict(ms=2**64), dict(owner=123)]
        for args in bad:
            with self.subTest(args=args):
                r = parse_fixture_trajectory(row(**args))
                self.assertEqual(r['malformed'], 1)
                self.assertEqual(r['samples'], [])
        self.assertEqual(parse_fixture_trajectory(terminal(x_mm=1))['malformed'], 1)

    def test_sequence_and_context_changes_fail_closed(self):
        for second in (row(), row(seq=2, sampled=999, ms=1050), row(seq=2, revision=4),
                       row(seq=2, ms=999), row(seq=2, generation=4)):
            self.assertEqual(parse_fixture_trajectory(row() + '\n' + second)['malformed'], 1)
        r = parse_fixture_trajectory(row() + '\n' + row(seq=3, sampled=1050, ms=1050))
        self.assertEqual(r['sequence_missing'], 1)
        self.assertTrue(r['evidence_gap'])
        r = parse_fixture_trajectory(terminal() + '\n' + row(seq=3, sampled=14000, ms=14000))
        self.assertEqual(r['malformed'], 1)

    def test_explicit_failure_and_clock_gap_have_no_values(self):
        for status in range(4, 9):
            r = parse_fixture_trajectory(row() + '\n' + terminal(status=status))
            self.assertTrue(r['evidence_gap'])
            self.assertEqual(r['malformed'], 0)
        r = parse_fixture_trajectory(row() + '\n' + terminal(status=5, ms=900))
        self.assertEqual(r['malformed'], 0)
        self.assertEqual(r['samples'][-1]['status'], 'gap')

    def test_status_retains_delivery_failure_without_raw_values(self):
        s = dict(session=1, status=8, samples=3, dropped=1, duration_ms=24000, sample_limit=481)
        r = parse_fixture_trajectory('ARCADE FIXTURE TRAJECTORY STATUS: ' + json.dumps(s))
        self.assertEqual(r['status']['status'], 'dropped')
        self.assertTrue(r['evidence_gap'])
        for raw in (json.dumps(dict(s, raw_handle=123)), '{"session":1,"session":2}', json.dumps(dict(s, samples=True))):
            self.assertEqual(parse_fixture_trajectory('ARCADE FIXTURE TRAJECTORY STATUS: ' + raw)['malformed'], 1)

    def test_multiple_sessions_bounded_tail_and_missing_initial_rows(self):
        lines = [row(session=i) + '\n' + terminal(session=i) for i in range(1, 259)]
        r = parse_fixture_trajectory('\n'.join(lines))
        self.assertEqual(len(r['samples']), 512)
        self.assertEqual(r['dropped'], 4)
        self.assertTrue(r['evidence_gap'])
        self.assertEqual(r['malformed'], 0)
        self.assertEqual(parse_fixture_trajectory(row(seq=5))['sequence_missing'], 4)


if __name__ == '__main__':
    unittest.main()
