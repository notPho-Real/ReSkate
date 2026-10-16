"""Offline tests of bounded, sanitized native declaration diagnostics."""
import json
import unittest
from lab import probe, trick_batch


def field(seq=1, epoch=7, role=1, index=0, **changes):
    row = dict(seq=seq, epoch=epoch, role=role, index=index,
               type_hash={1: 0xf95bfe33, 2: 1606121107}[role], type_size=64,
               field_hash=100 + index, kind=15, size=4, ref_type_hash=123)
    row.update(changes)
    return 'ARCADE NATIVE SCORE SCHEMA FIELD: ' + ' '.join(f'{key}={value}' for key, value in row.items()) + '.'


def footer(seq=1, **changes):
    row = dict(seq=seq, ms=900, epoch=7, status=0, line_count=1, sequence_count=1, emitted=2, incomplete=0)
    row.update(changes)
    return 'ARCADE NATIVE SCORE SCHEMA: ' + ' '.join(f'{key}={value}' for key, value in row.items()) + ' owner_verified=false.'


def pair(seq=1):
    return [field(seq=seq), field(seq=seq, role=2), footer(seq)]


class ScoreSchemaTests(unittest.TestCase):
    def parse(self, lines):
        return probe.parse_native_score_schema('\n'.join(lines))

    def test_complete_pair_exposed_through_existing_native_score_object(self):
        result = trick_batch.observations('\n'.join(pair()))['native_score']
        self.assertEqual(result['schema_status'], 'COMPLETE')
        self.assertFalse(result['schema_evidence_gap'])
        self.assertEqual(result['schema_snapshot']['footer']['emitted'], 2)
        self.assertEqual([row['role'] for row in result['schema_snapshot']['fields']], [1, 2])
        self.assertFalse(result['schema_snapshot']['owner_verified'])
        self.assertEqual(result['native_event_identity'], 'UNVERIFIED')
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_missing_footer_partial_reordered_duplicate_and_epoch_mismatch(self):
        cases = [pair()[:-1], pair()[1:], [field(), footer()],
                 [field(role=2), field(), footer()],
                 [field(index=1), field(), field(role=2), footer(line_count=2, emitted=3)],
                 [field(), field(), field(role=2), footer()],
                 pair() + [footer()], pair() + [field()],
                 [field(), field(role=2, epoch=8), footer()],
                 [field(), field(role=2), footer(epoch=8)]]
        for lines in cases:
            with self.subTest(lines=lines):
                result = self.parse(lines)
                self.assertEqual(result['schema_status'], 'UNVERIFIED')
                self.assertIsNone(result['schema_snapshot'])
                self.assertTrue(result['schema_evidence_gap'])

    def test_invalid_numeric_fields_are_never_returned(self):
        changes = [dict(seq=0), dict(seq=25), dict(epoch=2**64), dict(epoch=0), dict(index=96),
                   dict(type_hash=123), dict(type_size=65536), dict(type_size=0), dict(size=65),
                   dict(size=0), dict(kind=0), dict(kind=29), dict(field_hash=2**32), dict(ref_type_hash=2**32)]
        for change in changes:
            with self.subTest(change=change):
                result = self.parse([field(**change), field(role=2), footer()])
                self.assertIsNone(result['schema_snapshot'])
                self.assertGreater(result['schema_malformed'], 0)

    def test_invalid_footer_counts_and_status(self):
        for changes in [dict(line_count=0), dict(sequence_count=97), dict(emitted=3), dict(emitted=193),
                        dict(status=5), dict(ms=2**64), dict(incomplete=2), dict(line_count=2)]:
            with self.subTest(changes=changes):
                self.assertIsNone(self.parse([field(), field(role=2), footer(**changes)])['schema_snapshot'])

    def test_incomplete_footer_discards_even_matching_member_rows(self):
        for status in range(1, 5):
            result = self.parse([field(), field(role=2), footer(status=status, incomplete=1)])
            self.assertIsNone(result['schema_snapshot'])
            self.assertEqual(result['schema_latest_footer']['status'], status)
            self.assertEqual(result['schema_incomplete'], 1)

    def test_only_latest_pair_returned_and_newer_failure_never_uses_old_fields(self):
        result = self.parse(pair() + pair(2))
        self.assertEqual(result['schema_snapshot']['footer']['seq'], 2)
        self.assertEqual(len(result['schema_snapshot']['fields']), 2)
        self.assertEqual(result['schema_superseded'], 1)
        result = self.parse(pair() + [footer(2, status=1, line_count=0, sequence_count=0, emitted=0, incomplete=1)])
        self.assertIsNone(result['schema_snapshot'])
        self.assertEqual(result['schema_latest_footer']['seq'], 2)
        result = self.parse([field()] + pair(2))
        self.assertEqual(result['schema_status'], 'COMPLETE')
        self.assertTrue(result['schema_evidence_gap'])
        self.assertEqual(result['schema_incomplete'], 1)
        result = self.parse([field(seq=2), field(seq=2, role=2), footer(2, incomplete=1)])
        self.assertEqual(result['schema_status'], 'COMPLETE')
        self.assertTrue(result['schema_evidence_gap'])
        self.assertEqual(result['schema_latest_footer']['incomplete'], 1)

    def test_lifetime_and_member_limits(self):
        lines = []
        for seq in range(1, 25):
            for role in (1, 2):
                lines.extend(field(seq=seq, role=role, index=index) for index in range(96))
            lines.append(footer(seq, line_count=96, sequence_count=96, emitted=192))
        result = self.parse(lines)
        self.assertEqual(result['schema_status'], 'COMPLETE')
        self.assertEqual(len(result['schema_snapshot']['fields']), 192)
        self.assertEqual(result['schema_snapshots_seen'], 24)
        self.assertEqual(result['schema_dropped'], 0)
        result = self.parse(lines + [field(seq=24)])
        self.assertEqual(result['schema_dropped'], 1)
        self.assertIsNone(result['schema_snapshot'])

    def test_bad_strings_and_unknown_lines_cannot_escape_parser(self):
        secret = 'PRIVATE_ADDRESS_0x123456789'
        for line in [field().replace('size=4 ', f'size={secret} '),
                     field()[:-1] + f' owner={secret}.',
                     f'ARCADE NATIVE SCORE SCHEMA: {secret}',
                     footer().replace('owner_verified=false', 'owner_verified=true')]:
            result = self.parse(pair() + [line])
            self.assertIsNone(result['schema_snapshot'])
            self.assertNotIn(secret, json.dumps(result))
        self.assertEqual(self.parse(['unrelated log line'])['schema_status'], 'MISSING')


if __name__ == '__main__':
    unittest.main()
