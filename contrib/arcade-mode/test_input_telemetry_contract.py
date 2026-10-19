"""Wire compatibility and hot-path boundary checks; never accesses hardware."""
import importlib.util
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("arcade_probe", ROOT / "contrib/arcade-mode/lab/probe.py")
PROBE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PROBE)


class InputTelemetryContract(unittest.TestCase):
    def test_repeated_gap_rows_are_counted_regardless_of_channel(self):
        for channel in ("Skater", "Command"):
            text = "\n".join(f"[2026-10-16 12:00:00.000] [7:8] {channel}: ARCADE {kind} GAP: dropped=1."
                             for kind in ("PAD", "COMBO", "PAD", "COMBO"))
            self.assertEqual(PROBE.parse_pad_samples(text)["dropped"], 2)
            self.assertEqual(PROBE.parse_combo_samples(text)["dropped"], 2)

    def test_gap_sink_bypasses_identical_message_suppression(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_mode.cpp").read_text()
        gap = source.split("void write_input_gap(", 1)[1].split("DWORD WINAPI input_telemetry_worker", 1)[0]
        for kind in ("PAD", "COMBO"):
            self.assertRegex(gap, rf'logging::Channel::command,\s*"ARCADE {kind} GAP: dropped=\{{\}}\."')
        logger = (ROOT / "Engine/Core/Log/logger.cpp").read_text()
        self.assertIn("channel != Channel::command && level != Level::critical", logger)

    def test_continuous_producers_do_not_format_or_log(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_mode.cpp").read_text()
        regions = [source.split("void update_combo(", 1)[1].split("void write_client_telemetry", 1)[0],
                   source.split("void observe_combo_step(", 1)[1].split("bool validate_request_probe", 1)[0],
                   source.split("void observe_xinput_source(", 1)[1]]
        for region in regions:
            self.assertNotIn("logging::", region)
            self.assertNotIn("std::format", region)
        client = source.split("void on_client_tick()", 1)[1].split("void observe_xinput_source", 1)[0]
        # Bounded startup catalog emission is the only direct client log left.
        self.assertEqual(re.findall(r'"(ARCADE [^":]+):', client),
                         ["ARCADE AUTHORED GESTURE", "ARCADE AUTHORED LABEL", "ARCADE AUTHORED ARC"])

    def test_native_ui_observer_only_publishes_detached_rows(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_score_probe.cpp").read_text()
        self.assertNotIn("logging::", source)
        self.assertNotIn("std::format", source)
        self.assertNotIn("score_probe_path_text(", source)
        self.assertIn("game::ModelWriteLock lock(manager);", source)
        self.assertIn("enqueue_client_telemetry(ScorePresenceEvidence", source)

    def test_existing_score_parser_accepts_unchanged_bounded_paths(self):
        path = "9b74fc7c.00000000.ffffffff.000123ab"
        text = "\n".join([
            f"Skater: ARCADE SCORE NODE: path={path} kind=7 size=8.",
            f"Skater: ARCADE SCORE LABEL: ms=101 path={path} id=25.",
            f"Skater: ARCADE SCORE VALUE: ms=101 path={path} value=25.",
            "Skater: ARCADE SCORE PRESENCE: ms=100 source=1 rows=5 stage=0 parts=1 labels=4.",
            "Skater: ARCADE SCORE LIMIT: reached=true."])
        result = PROBE.parse_score_samples(text)
        self.assertEqual(result["nodes"][0]["path"], path)
        self.assertEqual(result["labels"][0]["id"], 25)
        self.assertEqual(result["values"][0]["value"], 25)
        self.assertTrue(result["limit_reached"])
        self.assertEqual(PROBE.parse_score_presence(text)["samples"][0]["rows"], 5)


class CaptureCorrelationContract(unittest.TestCase):
    @staticmethod
    def fixture():
        base = dict.fromkeys(PROBE.CAPTURE_CORRELATION_V2_NUMBERS, 0)
        base.update(schema=2, capture=1, required_sources=31, ms=100, native_ms=100,
                    owner=1, owner_generation=1, contact_generation=1, action_generation=1, physics_step=1)
        base.update(native_valid=True, events_known=True, complete=False, overflow=False, queue_admitted=False)
        rows = [base.copy()]
        for source in range(1, 6):
            row = dict(base, kind=1, source=source, source_sequence=1, ms=100+source,
                       native_ms=100+source, physics_step=1+source, first_event=source, last_event=source,
                       first_ms=100+source, last_ms=100+source)
            if source == 2:
                row.update(recognized_ms=row['ms'], recognition_id=1, recognition_session=1,
                           action=7, context=1, footprint_kind=1, member_count=1, queue_admitted=True)
            rows.append(row)
            if source == 2:
                rows.append(dict(row, member_index=1, member_event=2, source_sequence=2))
        for ordinal, row in enumerate(rows):
            row['ordinal'] = ordinal
        rows.append(dict(base, kind=2, ordinal=7, ms=106, native_ms=106, physics_step=7,
                         expected=6, drained=6, first_ordinal=1, last_ordinal=6, complete=True))
        return rows

    @staticmethod
    def wire(rows):
        lines = []
        for row in rows:
            numbers = PROBE.CAPTURE_CORRELATION_V2_NUMBERS if row['schema'] == 2 else PROBE.CAPTURE_CORRELATION_NUMBERS
            flags = PROBE.CAPTURE_CORRELATION_V2_FLAGS if row['schema'] == 2 else PROBE.CAPTURE_CORRELATION_FLAGS
            lines.append('Command: ARCADE INPUT CAPTURE: ' + ' '.join(f'{key}={row[key]}' for key in numbers) +
                         ' ' + ' '.join(f'{key}={str(row[key]).lower()}' for key in flags) + '.')
        return "\n".join(lines)

    def parse(self, rows):
        return PROBE.parse_capture_correlation(self.wire(rows))

    def assert_incomplete(self, rows, error=None):
        result = self.parse(rows)
        self.assertEqual(result['capture_completeness'], 'INCOMPLETE')
        self.assertTrue(result['evidence_gap'])
        if error:
            self.assertIn(error, result['captures'][0]['errors'])
        self.assertEqual(result['native_acceptance'], 'UNVERIFIED')
        self.assertEqual(result['causal_acceptance'], 'UNVERIFIED')

    def test_complete_and_cross_source_drain_order(self):
        rows = self.fixture()
        for ordered in (rows, [rows[0], *reversed(rows[1:-1]), rows[-1]]):
            result = self.parse(ordered)
            self.assertEqual(result['capture_completeness'], 'COMPLETE')
            self.assertFalse(result['evidence_gap'])
            self.assertEqual(result['native_acceptance'], 'UNVERIFIED')
            self.assertEqual([r['ordinal'] for r in result['captures'][0]['events']], list(range(1, 7)))

    def test_loss_and_truncation(self):
        rows = self.fixture()
        self.assert_incomplete(rows[:-1], 'missing_end')
        self.assert_incomplete(rows[:2]+rows[3:], 'accounting')
        for changes, error in ((dict(drained=4, dropped=1), 'dropped'),
                               (dict(drop_end=1), 'dropped'), (dict(complete=False), 'incomplete_end'),
                               (dict(first_ordinal=2), 'event_bounds'), (dict(expected=(1<<64)-1), 'accounting')):
            with self.subTest(changes=changes):
                altered = self.fixture(); altered[-1].update(changes)
                self.assert_incomplete(altered, error)
        many = self.fixture()
        many[1:-1] = [dict(many[1], ordinal=i+1, source_sequence=i+1) for i in range(4097)]
        result = self.parse(many)
        self.assertEqual(len(result['captures'][0]['events']), 4096)
        self.assertGreater(result['truncated'], 0)
        self.assertEqual(result['capture_completeness'], 'INCOMPLETE')

    def test_missing_malformed_and_untrusted_fields(self):
        for transform in (lambda t: t.replace('owner=1 ', '', 1),
                          lambda t: t.replace('owner=1 ', 'owner=1 owner=1 ', 1),
                          lambda t: t.replace('schema=2', 'schema=18446744073709551616', 1),
                          lambda t: t.replace('queue_admitted=false.', 'queue_admitted=false. extra=opaque', 1)):
            result = PROBE.parse_capture_correlation(transform(self.wire(self.fixture())))
            self.assertEqual(result['capture_completeness'], 'INCOMPLETE')
            self.assertGreater(result['malformed'], 0)
            self.assertNotIn('opaque', repr(result))
        self.assertEqual(PROBE.parse_capture_correlation('unrelated')['capture_completeness'], 'MISSING')

    def test_owner_and_timestamp_provenance(self):
        cases = ((dict(owner=2), 'owner_changed'), (dict(owner_generation=2), 'owner_changed'),
                 (dict(native_ms=103), 'native_provenance'), (dict(ms=252), 'native_provenance'),
                 (dict(native_valid=False), 'unavailable_provenance'),
                 (dict(first_ms=104), 'event_provenance'), (dict(recognized_ms=99), 'recognition_provenance'),
                 (dict(events_known=False), 'event_provenance'), (dict(overflow=True), 'unavailable_provenance'))
        for changes, error in cases:
            with self.subTest(changes=changes):
                rows = self.fixture(); rows[1].update(changes)
                self.assert_incomplete(rows, error)
        rows = self.fixture(); rows[4]['ms'] += 1
        self.assert_incomplete(rows, 'native_restamped')
        rows = self.fixture(); rows[1].update(ms=250, native_ms=100)
        rows[-1].update(ms=260, native_ms=260)
        self.assertEqual(self.parse(rows)['capture_completeness'], 'COMPLETE')
        rows[1]['ms'] = 251
        self.assert_incomplete(rows, 'native_provenance')
        # A delayed worker cannot restamp the producer's original proof.
        rows = self.fixture(); rows[2].update(ms=150, recognized_ms=102)
        rows[-1].update(ms=160)
        self.assert_incomplete(rows, 'recognition_shape')
        original = PROBE.parse_capture_correlation(self.wire(self.fixture()).replace('Command:', 'late sink Command:'))
        self.assertEqual(original['capture_completeness'], 'COMPLETE')
        self.assertEqual(original['captures'][0]['events'][1]['recognized_ms'], 102)

    def test_counter_rollover_reuse_and_scope(self):
        rows = self.fixture(); rows.insert(-1, dict(rows[1], ordinal=6, source_sequence=1))
        rows[-1].update(ordinal=7, expected=6, drained=6, last_ordinal=6)
        self.assert_incomplete(rows, 'counter_rollback')
        rows[1]['source_sequence'] = (1<<64)-1
        self.assert_incomplete(rows, 'counter_rollback')
        rows = self.fixture(); rows[-1].update(native_ms=100, physics_step=1)
        self.assert_incomplete(rows, 'end_provenance')
        rows = self.fixture(); rows[0].update(drop_begin=(1<<64)-1, drop_end=(1<<64)-1)
        rows[-1].update(drop_begin=(1<<64)-1, drop_end=0)
        self.assert_incomplete(rows, 'drop_counter_rollback')
        rows = self.fixture(); rows[4]['physics_step'] = 1
        self.assert_incomplete(rows, 'step_provenance')
        rows = self.fixture(); rows[4]['physics_step'] = (1<<64)-1
        self.assert_incomplete(rows, 'step_rollback')
        self.assert_incomplete(self.fixture()+self.fixture(), 'begin_shape')
        rolled = [dict(row, capture=(1<<64)-1) for row in self.fixture()] + self.fixture()
        self.assertEqual(self.parse(rolled)['capture_completeness'], 'INCOMPLETE')
        self.assert_incomplete(self.fixture()[:4]+self.fixture()[5:], 'missing_source')
        all_rows = []
        for cid in range(1, 18):
            all_rows.extend(dict(row, capture=cid) for row in self.fixture())
        result = self.parse(all_rows)
        self.assertEqual(len(result['captures']), 16)
        self.assertEqual(result['truncated'], 1)
        self.assertEqual(result['capture_completeness'], 'INCOMPLETE')
        rows = self.fixture()[:-1]+[dict(row, capture=2) for row in self.fixture()]
        self.assert_incomplete(rows, 'missing_end')

    @staticmethod
    def renumber(rows):
        for ordinal, row in enumerate(rows[:-1]):
            row['ordinal'] = ordinal
        count = len(rows)-2
        rows[-1].update(ordinal=count+1, expected=count, drained=count, dropped=0,
                        first_ordinal=1 if count else 0, last_ordinal=count)
        return rows

    def test_exact_members_cannot_be_replaced_by_bounds(self):
        rows = self.fixture()
        del rows[3]
        self.assert_incomplete(self.renumber(rows), 'missing_recognition_member')
        rows = self.fixture()
        rows.insert(4, dict(rows[3], source_sequence=3))
        self.assert_incomplete(self.renumber(rows), 'duplicate_recognition_member')
        rows = self.fixture(); rows[3]['member_event'] = 1
        self.assert_incomplete(rows, 'recognition_members')
        rows = self.fixture(); rows[3]['recognition_id'] = 2
        self.assert_incomplete(rows, 'missing_recognition_summary')

    def test_matcher_order_held_reuse_and_admission(self):
        rows = self.fixture(); summary = rows[2]
        summary.update(first_event=4, last_event=9, first_ms=1, member_count=3, held_reuse=4)
        rows[3:4] = [dict(summary, member_index=i, member_event=event, source_sequence=i+1)
                     for i, event in enumerate((9, 4, 6), 1)]
        self.renumber(rows)
        result = self.parse(rows)
        self.assertEqual(result['capture_completeness'], 'COMPLETE')
        recognition = result['captures'][0]['recognitions'][0]
        self.assertEqual(recognition['events'], [9, 4, 6])
        self.assertEqual(recognition['held_reuse'], 4)
        for row in rows:
            if row['source'] == 2:
                row['queue_admitted'] = False
        self.assertEqual(self.parse(rows)['capture_completeness'], 'COMPLETE')
        self.assertFalse(self.parse(rows)['captures'][0]['recognitions'][0]['queue_admitted'])
        rows[3]['queue_admitted'] = True
        self.assert_incomplete(rows, 'recognition_member_provenance')
        rows[3]['queue_admitted'] = False
        for row in rows:
            if row['source'] == 2:
                row['held_reuse'] = 7
        self.assert_incomplete(rows, 'recognition_members')

    def test_legacy_decodes_without_claiming_exact_recognition(self):
        rows = self.fixture()
        for row in rows:
            row['schema'] = 1
        self.assert_incomplete(rows, 'recognition_members_unavailable')
        scoped = [row for row in rows if row['source'] in (0, 1, 3)]
        for row in scoped:
            row['required_sources'] = 5
        result = self.parse(self.renumber(scoped))
        self.assertEqual(result['capture_completeness'], 'COMPLETE')
        self.assertEqual(result['captures'][0]['begin']['required_sources'], 5)
        self.assertEqual(result['native_acceptance'], 'UNVERIFIED')
        rows = self.fixture(); rows[1]['schema'] = 1
        self.assert_incomplete(rows, 'mixed_schema')

    def test_unknown_and_input_free_remain_distinct(self):
        rows = self.fixture(); del rows[3]
        rows[2].update(footprint_kind=2, member_count=0, first_event=0, last_event=0, first_ms=0, last_ms=0)
        self.assertEqual(self.parse(self.renumber(rows))['capture_completeness'], 'COMPLETE')
        rows[2].update(first_event=1, last_event=1)
        self.assert_incomplete(rows, 'recognition_shape')
        rows[2].update(first_event=0, last_event=0)
        rows[2]['footprint_kind'] = 0
        rows[2]['events_known'] = False
        rows[-1]['complete'] = False
        self.assert_incomplete(rows, 'unavailable_provenance')


if __name__ == "__main__":
    unittest.main()
