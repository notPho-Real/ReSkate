from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).parent/'lab'))
from probe import parse_action_history


def row(**updates):
    fields = dict(seq=1, ms=1000, status=1, available='true', comparable='true', added=0, removed=0, relabels=0)
    fields.update(updates)
    return 'prefix ARCADE ACTION HISTORY: ' + ' '.join(f'{k}={v}' for k,v in fields.items()) + '.'


class ActionHistoryTests(unittest.TestCase):
    def test_fresh_basic_grab_and_persistent_history_remain_distinct(self):
        result = parse_action_history('\n'.join((row(), row(seq=2, ms=1050, added=1 << 26), row(seq=3, ms=1100))))
        self.assertEqual([s['added'] for s in result['samples']], [0, 1 << 26, 0])
        self.assertEqual((result['malformed'], result['sequence_gaps']), (0, 0))
        self.assertFalse(result['standalone_proof'])

    def test_unknown_labels_unavailable_deltas_and_overflow_fail_closed(self):
        for update in (dict(added=1 << 29), dict(ms=2**64), dict(seq=0), dict(relabels=17),
                       dict(available='false'), dict(status=2), dict(comparable='false', added=1)):
            with self.subTest(update=update):
                self.assertEqual(parse_action_history(row(**update))['malformed'], 1)
        result = parse_action_history(row(status=5, comparable='false', available='false'))
        self.assertFalse(result['samples'][0]['available'])

    def test_gaps_relabels_and_tail_loss_are_explicit(self):
        result = parse_action_history(row()+'\n'+row(seq=3, ms=1050, added=1 << 28, relabels=1))
        self.assertEqual(result['sequence_gaps'], 1)
        self.assertEqual(result['samples'][-1]['relabels'], 1)
        result = parse_action_history('\n'.join(row(seq=i+1, ms=1000+i) for i in range(513)))
        self.assertEqual((len(result['samples']), result['dropped']), (512, 1))


if __name__ == '__main__':
    unittest.main()
