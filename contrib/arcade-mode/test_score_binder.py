"""Offline binder diagnostic parsing; no Deck access."""
import json
from pathlib import Path
import unittest
from lab import probe


def row(route=1, **changes):
    values = dict(ms=1000, epoch=3, active='true', prepared='true', attached='true', route=route,
                  contract_rejected=0, mapping_unreadable=0, count_rejected=0, layout_rejected=0,
                  value_mismatch=0, relationship_matches=0, unverified=0, stale=0, overflow='false')
    values.update(changes)
    return 'ARCADE NATIVE SCORE BINDER: ' + ' '.join(f'{key}={value}' for key, value in values.items()) + ' owner_verified=false.'


def health(epoch=3, active='true', ms=1001):
    return (f'ARCADE NATIVE SCORE HEALTH: ms={ms} epoch={epoch} active={active} bound=false '
            'route=1 unbound=0 expired=0 changed=0 overflow=false.')


def layout_row(route=1, reasons=(0, 0, 0, 0), **changes):
    keys = ('layout_alignment', 'layout_range', 'layout_metadata', 'layout_duplicate')
    fields = ''.join(f' {key}={value}' for key, value in zip(keys, reasons))
    return row(route, **changes).replace(' overflow=', fields + ' overflow=')


class ScoreBinderTests(unittest.TestCase):
    @staticmethod
    def record(**changes):
        values = dict(seq=1, ms=1000, epoch=3, delivered=1001, current='true', route=2,
                      offset0=0, low0=255, high0='true', offset1=8, low1=0, high1='false',
                      offset2=0xffffffff, low2=1, high2='true')
        values.update(changes)
        return 'ARCADE BINDER RECORD: ' + ' '.join(f'{key}={value}' for key, value in values.items()) + \
            ' relationship=UNVERIFIED owner_verified=false.'

    @staticmethod
    def record_status(**changes):
        values = dict(ms=1001, epoch=3, dropped=0, duplicates=2, stale=0, overflow='false')
        values.update(changes)
        return 'ARCADE BINDER RECORD STATUS: ' + ' '.join(f'{key}={value}' for key, value in values.items()) + \
            ' relationship=UNVERIFIED owner_verified=false.'

    def records(self, extra):
        return self.parse([row(route) for route in range(1, 5)] + [health()] + extra)

    def test_static_records_keep_all_three_shapes_without_relationship_authority(self):
        result = self.records([self.record(), self.record_status()])
        self.assertEqual(len(result['binder_records_current']), 1)
        self.assertEqual(result['binder_records'][0]['offset2'], 0xffffffff)
        self.assertEqual(result['binder_records'][0]['low0'], 255)
        self.assertTrue(result['binder_records'][0]['high0'])
        self.assertFalse(result['binder_records_evidence_gap'])
        self.assertEqual(result['binder_records_relationship'], 'UNVERIFIED')
        self.assertEqual(result['binder_records_capture_completeness'], 'UNVERIFIED')
        self.assertEqual(result, json.loads(json.dumps(result)))

    def test_static_records_reject_malformed_bounds_partial_and_sequence_gaps(self):
        for malformed in (self.record(low1=256), self.record(offset0=0x100000000),
                          self.record(epoch=0), self.record(seq=0), self.record(route=1),
                          self.record(high2='1'), self.record(ms=2**64), self.record(delivered=999),
                          self.record(delivered=7001), self.record().replace(' low0=255', ''),
                          self.record().replace('owner_verified=false', 'owner_verified=true')):
            result = self.records([malformed, self.record_status()])
            self.assertEqual(result['binder_records_malformed'], 1)
            self.assertFalse(result['binder_records_current'])
        for seq in (1, 3):
            result = self.records([self.record(), self.record(seq=seq), self.record_status()])
            self.assertEqual(result['binder_records_malformed'], 1)
            self.assertFalse(result['binder_records_current'])

    def test_static_records_stale_loss_epoch_and_restart_are_explicit(self):
        for status in (None, self.record_status(dropped=1), self.record_status(stale=1),
                       self.record_status(overflow='true'), self.record_status(epoch=4),
                       self.record_status(ms=7001)):
            result = self.records([self.record()] + ([status] if status else []))
            self.assertTrue(result['binder_records_evidence_gap'])
            self.assertFalse(result['binder_records_current'])
        for record in (self.record(current='false'), self.record(epoch=2)):
            result = self.records([record, self.record_status()])
            self.assertEqual(len(result['binder_records']), 1)
            self.assertFalse(result['binder_records_current'])
        restarted = self.records([self.record(), self.record_status(), 'Arcade Mode ready'])
        self.assertFalse(restarted['binder_records'])
        self.assertFalse(restarted['binder_records_current'])

    def test_static_records_bounded_and_status_counter_regressions_reject(self):
        result = self.records([self.record(seq=i + 1) for i in range(129)] + [self.record_status()])
        self.assertEqual(len(result['binder_records']), 128)
        self.assertEqual(result['binder_records_dropped'], 1)
        self.assertFalse(result['binder_records_current'])
        result = self.records([self.record(), self.record_status(duplicates=3), self.record_status()])
        self.assertEqual(result['binder_records_malformed'], 1)
        self.assertFalse(result['binder_records_current'])

    def test_diagnostic_hook_cannot_satisfy_progression_or_replace_primary_observer(self):
        root = Path(__file__).resolve().parents[2]
        implementation = (root / 'Extension/Progression/neighborhood_unlock_override.cpp').read_text()
        score = (root / 'Extension/ArcadeMode/arcade_native_score.cpp').read_text()
        startup = (root / 'Runtime/runtime_init.cpp').read_text()
        self.assertNotIn('prepare_neighborhood_unlock_override(', score)
        self.assertIn('prepare_expression_binder_diagnostics(runtime.base.load())', score)
        hook_only = implementation.split('bool prepare_expression_binder_diagnostics(', 1)[1].split(
            'bool prepare_neighborhood_unlock_override(', 1)[0]
        self.assertNotIn('authority.', hook_only)
        provider = implementation.split('bool prepare_neighborhood_unlock_override(', 1)[1].split(
            'bool set_neighborhood_unlock_override_enabled(', 1)[0]
        self.assertIn('authority.provider_prepared(prepared)', provider)
        self.assertIn('state.authority.request(enabled,', implementation)
        self.assertIn('observation.prepared = state.authority.prepared(', implementation)
        self.assertRegex(startup, r'\(global_offline_hook && progression_guard_armed\) &&\s*'
                                 r'dingosdk::prepare_neighborhood_unlock_override\(r.base\)')
        self.assertIn('set_expression_binder_observer(&observe_player_card_expression)', startup)
        attachment = implementation.split('bool try_attach_expression_binder_observer(', 1)[1].split(
            'bool prepare_expression_binder_diagnostics(', 1)[0]
        self.assertIn('.diagnostic_observer.compare_exchange_strong', attachment)
        self.assertNotIn('.observer.', attachment)
        hook = implementation.split('void expression_argument_binder_hook(', 1)[1].split('\n} // namespace', 1)[0]
        self.assertIn('state.observer.load(', hook)
        self.assertIn('state.diagnostic_observer.load(', hook)
        self.assertIn('void** forwarded_inputs = inputs;', hook)
        self.assertIn('if (state.authority.enabled())', hook)
        self.assertIn('original(serialized_graph, runtime_graph, outputs, forwarded_inputs, allocator)', hook)

    def parse(self, rows):
        return probe.parse_native_score_events('\n'.join(rows))

    def test_complete_group_keeps_numeric_outcomes_without_authority(self):
        result = self.parse([row(route, relationship_matches=4) for route in range(1, 5)] + [health()])
        self.assertEqual(result['binder_status'], 'COMPLETE')
        self.assertEqual(len(result['binder_snapshot']), 4)
        self.assertEqual(result['binder_snapshot'][1]['relationship_matches'], 4)
        self.assertEqual(result['binder_capture_completeness'], 'UNVERIFIED')
        self.assertEqual(result['binder_freshness'], 'UNVERIFIED')
        self.assertEqual(result['binder_owner'], 'UNVERIFIED')
        self.assertFalse(result['binder_snapshot'][0]['layout_reasons_available'])
        self.assertNotIn('layout_alignment', result['binder_snapshot'][0])
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_layout_reasons_preserve_each_cumulative_counter_without_authority(self):
        result = self.parse([layout_row(route, reasons=(1, 2, 3, 4), layout_rejected=4)
                             for route in range(1, 5)] + [health()])
        self.assertEqual(result['binder_status'], 'COMPLETE')
        for sample in result['binder_snapshot']:
            self.assertTrue(sample['layout_reasons_available'])
            self.assertEqual([sample[key] for key in
                ('layout_alignment', 'layout_range', 'layout_metadata', 'layout_duplicate')], [1, 2, 3, 4])
        self.assertEqual(result['binder_owner'], 'UNVERIFIED')
        self.assertEqual(result['binder_capture_completeness'], 'UNVERIFIED')
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_partial_invalid_regressed_or_mixed_layout_reasons_reject_snapshot(self):
        full = [layout_row(route, reasons=(1, 1, 1, 1), layout_rejected=1) for route in range(1, 5)]
        for bad in (full[0].replace(' layout_range=1', ''),
                    full[0].replace('layout_range=1', 'layout_range=-1'),
                    full[0].replace('layout_range=1', 'layout_range=' + str(2**64)),
                    full[0].replace('layout_range=1', 'layout_range=PRIVATE_0x12345678'), row()):
            result = self.parse([bad, *full[1:], health()])
            self.assertIsNone(result['binder_snapshot'])
            self.assertGreater(result['binder_malformed'], 0)
            self.assertNotIn('PRIVATE_', json.dumps(result))
        regressed = [layout_row(route, reasons=(1, 0, 1, 1), layout_rejected=2, ms=6000)
                     for route in range(1, 5)]
        result = self.parse(full + regressed + [health(ms=6001)])
        self.assertIsNone(result['binder_snapshot'])
        self.assertGreater(result['binder_malformed'], 0)

    def test_missing_partial_duplicate_reordered_and_unavailable(self):
        full = [row(route) for route in range(1, 5)]
        self.assertEqual(self.parse([])['binder_status'], 'MISSING')
        for lines in [full, full[:-1] + [health()], full + [row(4), health()],
                      [full[1], full[0], *full[2:], health()],
                      [row(route, prepared='false', attached='false') for route in range(1, 5)] + [health()],
                      [row(route, active='false') for route in range(1, 5)] + [health()],
                      full + [health(4)], full + [health(active='false')],
                      full + [health(ms=7001)],
                      full + [health(), health().replace('unbound=0', 'unbound=INVALID')],
                      [row(route, overflow='true') for route in range(1, 5)] + [health()]]:
            with self.subTest(lines=lines):
                result = self.parse(lines)
                self.assertIsNone(result['binder_snapshot'])
                self.assertEqual(result['binder_status'], 'UNVERIFIED')

    def test_independently_phased_health_retains_complete_diagnostic_snapshot(self):
        # Exact timestamps from the first deployed binder capture; no raw log or
        # native payload is needed to reproduce this sampling-phase regression.
        result = self.parse([health(epoch=1, ms=51365127)] +
                            [row(route, epoch=1, ms=51366533) for route in range(1, 5)])
        self.assertEqual(result['binder_status'], 'COMPLETE')
        self.assertEqual(len(result['binder_snapshot']), 4)
        self.assertEqual(result['binder_health_age_ms'], -1406)
        self.assertEqual(result['binder_health_abs_age_ms'], 1406)
        self.assertTrue(result['binder_context_current'])
        self.assertFalse(result['binder_evidence_gap'])
        for field in ('binder_capture_completeness', 'binder_freshness', 'binder_owner'):
            self.assertEqual(result[field], 'UNVERIFIED')
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_health_proximity_bounds_and_clock_lifecycle_rejections(self):
        full = [row(route, ms=10000) for route in range(1, 5)]
        for age in (-6001, -6000, -1, 0, 1, 6000, 6001):
            with self.subTest(age=age):
                result = self.parse(full + [health(ms=10000 + age)])
                self.assertEqual(result['binder_context_current'], abs(age) <= 6000)
                self.assertEqual(result['binder_snapshot'] is not None, abs(age) <= 6000)
                self.assertEqual(result['binder_health_age_ms'], age)
                self.assertEqual(result['binder_health_abs_age_ms'], abs(age))
        for rows in ([health(epoch=4, ms=9999)], [health(active='false', ms=9999)],
                     [health(ms=10000), health(ms=9999)],
                     [health(epoch=4, ms=9998), health(epoch=3, ms=9999)],
                     [health(ms=2**64)],
                     [health(ms=9999)] * 129):
            with self.subTest(rows=rows):
                result = self.parse(full + rows)
                self.assertIsNone(result['binder_snapshot'])
                self.assertFalse(result['binder_context_current'])
                self.assertTrue(result['binder_evidence_gap'])

    def test_old_session_and_newer_incomplete_cannot_replay_prior_snapshot(self):
        full = [row(route) for route in range(1, 5)] + [health()]
        self.assertEqual(self.parse(full + ['Arcade Mode ready'])['binder_status'], 'MISSING')
        result = self.parse(full + [row(1, ms=6000), health()])
        self.assertIsNone(result['binder_snapshot'])
        self.assertEqual(result['binder_incomplete'], 1)
        self.assertEqual(result['binder_latest_status']['ms'], 6000)

    def test_counter_and_clock_regression_are_loss(self):
        first = [row(route, relationship_matches=4) for route in range(1, 5)]
        for changes in [dict(ms=6000, relationship_matches=3), dict(ms=999, relationship_matches=5),
                        dict(ms=6000, epoch=2, relationship_matches=5)]:
            result = self.parse(first + [row(route, **changes) for route in range(1, 5)] + [health()])
            self.assertIsNone(result['binder_snapshot'])
            self.assertGreater(result['binder_malformed'], 0)

    def test_bounds_and_no_raw_text(self):
        for changes in [dict(epoch=0), dict(ms=2**64), dict(stale=2**64), dict(route=5),
                        dict(active='yes'), dict(prepared='true', attached='false'),
                        dict(relationship_matches='PRIVATE_0x123456789')]:
            result = self.parse([row(**changes), row(2), row(3), row(4), health()])
            self.assertIsNone(result['binder_snapshot'])
            self.assertGreater(result['binder_malformed'], 0)
            self.assertNotIn('PRIVATE_', json.dumps(result))
        lines = [row(route, ms=1000 + batch * 5000) for batch in range(32) for route in range(1, 5)]
        result = self.parse(lines + [health(ms=156001)])
        self.assertEqual(result['binder_status'], 'COMPLETE')
        self.assertEqual(len(result['binder_snapshot']), 4)
        result = self.parse(lines + [row(ms=200000), health()])
        self.assertIsNone(result['binder_snapshot'])
        self.assertEqual(result['binder_dropped'], 1)


if __name__ == '__main__':
    unittest.main()
