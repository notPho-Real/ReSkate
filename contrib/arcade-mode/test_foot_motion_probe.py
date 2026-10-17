import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent / 'lab'))
from probe import parse_foot_motion


def row(**updates):
    values = dict(seq=1, ms=1000, position_ms=980, epoch=1, valid='true',
                  airborne='false', ragdoll='false', wipeout='false', y=47.2, vy=0)
    values.update(updates)
    return 'prefix ARCADE FOOT MOTION: ' + ' '.join(f'{k}={v}' for k, v in values.items()) + '.'


class FootMotionProbeTests(unittest.TestCase):
    def test_ground_takeoff_descent_landing_are_observations_not_inferred_jumps(self):
        text = '\n'.join((row(), row(seq=2, ms=1050, position_ms=1030, airborne='true', vy=4.5),
                          row(seq=3, ms=1300, position_ms=1280, airborne='true', y=47.7, vy=-1.2),
                          row(seq=4, ms=1550, position_ms=1530)))
        result = parse_foot_motion(text)
        self.assertEqual((result['malformed'], result['sequence_gaps'], result['truncated']), (0, 0, 0))
        self.assertEqual([s['airborne'] for s in result['samples']], [False, True, True, False])
        self.assertEqual(result['samples'][1]['position_ms'], 1030)
        self.assertNotIn('jump', result)

    def test_malformed_stale_nonfinite_and_overflow_fail_closed(self):
        for changes in (dict(position_ms=1001), dict(position_ms=849), dict(position_ms=0),
                        dict(epoch=0), dict(seq=2**64), dict(y='nan'), dict(vy='1e999'), dict(vy=1000)):
            with self.subTest(changes=changes):
                self.assertEqual(parse_foot_motion(row(**changes))['malformed'], 1)
        self.assertFalse(parse_foot_motion(row(valid='false', position_ms=0))['samples'][0]['valid'])

    def test_gaps_ragdoll_and_bounded_tail_remain_explicit(self):
        result = parse_foot_motion(row() + '\n' + row(seq=3, ms=1050, ragdoll='true', wipeout='true'))
        self.assertEqual(result['sequence_gaps'], 1)
        self.assertTrue(result['samples'][-1]['ragdoll'])
        text = '\n'.join(row(seq=i+1, ms=1000+i, position_ms=1000+i) for i in range(513))
        result = parse_foot_motion(text)
        self.assertEqual((len(result['samples']), result['truncated']), (512, 1))
        self.assertEqual(result['samples'][0]['seq'], 2)


if __name__ == '__main__':
    unittest.main()
