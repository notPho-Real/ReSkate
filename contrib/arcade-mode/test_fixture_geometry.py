"""Offline fixture geometry parsing; no Deck or game access."""
import inspect
import json
import unittest
from lab import probe


def complete():
    return dict(status=2, samples=129, budget=129, step=.25, radius=8,
                contact_verified=False, context_current=True, session=1, started_ms=100,
                generation=3, revision=4, object=6, rail_position=[1, 2, 3],
                rail_rotation=[0, 0, 0, 1], rail_scale=1,
                sampled_low=[-.25, -3], sampled_high=[.25, 3], height_min=.5,
                height_max=.75, edge_uncertainty=.25)


def wire(row):
    return 'Command: ARCADE FIXTURE GEOMETRY: ' + json.dumps(row)


def phase(status, samples):
    row = complete()
    for key in ('sampled_low', 'sampled_high', 'height_min', 'height_max', 'edge_uncertainty'):
        del row[key]
    row.update(status=status, samples=samples, context_current=False)
    return row


class FixtureGeometryParser(unittest.TestCase):
    def test_complete_and_historical_geometry_preserve_no_authority(self):
        row = complete()
        self.assertEqual(probe.parse_fixture_geometry(wire(row)), row)
        row['context_current'] = False
        self.assertEqual(probe.parse_fixture_geometry(wire(row)), row)
        self.assertIs(probe.parse_fixture_geometry(wire(row))['contact_verified'], False)

    def test_actual_idle_and_all_noncomplete_statuses(self):
        idle = dict(status=0, samples=0, budget=129, step=.25, radius=8,
                    contact_verified=False, context_current=False, session=0, started_ms=0,
                    generation=0, revision=0, object=0)
        self.assertEqual(probe.parse_fixture_geometry(wire(idle)), idle)
        for status, counts in ((1, (0, 128)), (3, (0, 128, 129)), (4, (0, 128)),
                               (5, (0, 128)), (6, (129,)), (7, (129,)), (8, (0,))):
            for count in counts:
                with self.subTest(status=status, count=count):
                    row = phase(status, count)
                    self.assertEqual(probe.parse_fixture_geometry(wire(row)), row)

    def test_newest_bad_row_never_reuses_completed_result(self):
        first = wire(complete())
        for bad in ('ARCADE FIXTURE GEOMETRY: {', 'ARCADE FIXTURE GEOMETRY:{}',
                    'ARCADE FIXTURE GEOMETRY: []', 'ARCADE FIXTURE GEOMETRY: ' + 'x' * 4097,
                    wire(dict(complete(), private='not exported'))):
            with self.subTest(bad=bad[:80]):
                self.assertIsNone(probe.parse_fixture_geometry(first + '\n' + bad))
        self.assertEqual(probe.parse_fixture_geometry(first + '\nunrelated log'), complete())
        newer = phase(8, 0); newer['session'] = 2
        self.assertEqual(probe.parse_fixture_geometry(first + '\n' + wire(newer)), newer)
        self.assertIsNone(probe.parse_fixture_geometry(wire(newer) + '\n' + first))

    def test_schema_context_and_count_rejections(self):
        changes = dict(status=9, samples=128, budget=130, step=.5, radius=9,
                       contact_verified=True, context_current=1, session=0, started_ms=0,
                       generation=0, object=0, revision=-1, rail_scale=0,
                       rail_rotation=[0, 0, 0, 0], rail_position=[0, 0],
                       sampled_low=[-.25, 1], sampled_high=[.25, 8],
                       height_min=1, height_max=3, edge_uncertainty=0)
        for key, value in changes.items():
            with self.subTest(key=key):
                self.assertIsNone(probe.parse_fixture_geometry(wire(dict(complete(), **{key: value}))))
        for key in complete():
            row = complete(); del row[key]
            with self.subTest(missing=key):
                self.assertIsNone(probe.parse_fixture_geometry(wire(row)))
        for status, count in ((1, 129), (2, 128), (4, 129), (5, 129), (6, 128), (7, 0), (8, 1)):
            row = complete() if status == 2 else phase(status, count)
            row['samples'] = count
            self.assertIsNone(probe.parse_fixture_geometry(wire(row)))
        self.assertIsNone(probe.parse_fixture_geometry(wire(dict(phase(8, 0), context_current=True))))

    def test_numeric_types_bounds_and_duplicate_keys(self):
        for key in ('status', 'samples', 'budget', 'session', 'started_ms', 'generation', 'revision', 'object'):
            for value in (True, 1.5, -1, 2**64):
                with self.subTest(key=key, value=value):
                    self.assertIsNone(probe.parse_fixture_geometry(wire(dict(complete(), **{key: value}))))
        self.assertIsNone(probe.parse_fixture_geometry(wire(dict(complete(), session=0x1000000))))
        for key in ('step', 'radius', 'height_min', 'height_max', 'rail_scale', 'edge_uncertainty'):
            for value in (True, float('nan'), float('inf'), float('-inf'), '1'):
                self.assertIsNone(probe.parse_fixture_geometry(wire(dict(complete(), **{key: value}))))
        for key in ('rail_position', 'rail_rotation', 'sampled_low', 'sampled_high'):
            row = complete(); row[key][0] = float('nan')
            self.assertIsNone(probe.parse_fixture_geometry(wire(row)))
        duplicate = wire(complete()).replace('"status": 2', '"status": 2, "status": 2')
        self.assertIsNone(probe.parse_fixture_geometry(duplicate))
        self.assertIsNone(probe.parse_fixture_geometry(wire(complete()) + ' trailing'))

    def test_grid_and_native_float_boundary(self):
        self.assertIsNone(probe.parse_fixture_geometry(wire(dict(complete(), sampled_low=[-.1, -3]))))
        row = complete(); row.update(sampled_low=[-7.75, -7.75], sampled_high=[7.75, 7.75],
                                     height_min=-0.10000000149011612, height_max=2)
        self.assertEqual(probe.parse_fixture_geometry(wire(row)), row)
        self.assertIsNone(probe.parse_fixture_geometry(wire(dict(row, height_min=-.1001))))

    def test_native_trace_exposes_diagnostic_parser(self):
        self.assertIn('"fixture_geometry": parse_fixture_geometry(', inspect.getsource(probe.native_trace))


if __name__ == '__main__':
    unittest.main()
