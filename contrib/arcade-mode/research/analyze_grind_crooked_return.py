"""Offline negative Crooked neutral-return evidence; performs no hardware I/O.

Usage: python analyze_grind_crooked_return.py /path/to/reskate-arcade-lab
Equal read brackets are not atomic frames. Admission failure is not a recipe.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def analyze(root):
    path = root / 'grind-native-crooked-return-c048914-v1/results/two-leg-lateral.json.gz'
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == '3af936376205882c0c1e57dfde04e52f9dd5332f3744debd202b825750e12ded'
    envelope = json.loads(gzip.decompress(blob))
    assert envelope['returncode'] == 0
    data = json.loads(envelope['stdout'])
    assert not {k: v for k, v in data.items() if 'error' in k}
    rows = data['native_samples']
    assert len(rows) == 485
    gate = next(json.loads(line.split(' ', 1)[1]) for line in data['gate_log']
                if line.startswith('GRIND_GATE_RESULT '))
    assert gate['reason'] == 'AssertionError: ' and gate['legs'] == 1
    assert [(c['source'], c['target']) for c in gate['completed']] == [(401, 403)]
    completed = [e for e in gate['actor_events'] if e['boundary'] == 'completed']
    assert len(completed) == 1 and completed[0]['timing'][-1] == 1
    segment = [1151051235367, 0]
    neutral = []
    for row in rows:
        assert abs(row['physics_dt'] - 1/30) < .000001
        assert abs(row['cached_dt'] - 1/30) < .000001
        s = row.get('admission_sample')
        if not s or not s['stable_reads'] or s['state'] != 403 or s['named'] != 32:
            continue
        if (s['physical_contact'] != 1 or s['published_contact'] != 1 or
                s['segment_identity'] != segment or s['missing_segment']):
            continue
        heading, pitch, roll = s['requested_orientation_raw']
        if pitch != 0 or roll != 0:
            continue
        assert .388 < heading < .389
        assert s['actor_active'] == s['published_admission'] == 0
        outer = row['active_segment']
        assert outer['segment_identity'] == segment and not outer['missing']
        a, b = outer['endpoints']
        assert a[1] == b[1] and sum((x-y)**2 for x, y in zip(a, b)) == 144
        neutral.append(row)
    assert len(neutral) == 205
    times = [(r['ns'] - rows[0]['ns']) / 1e6 for r in neutral]
    assert abs(times[0] - 2622.272815) < .000001
    assert abs(times[-1] - 5251.215494) < .000001
    assert len({r['admission_sample']['actor'] for r in neutral}) == 1
    first_crooked = next(r for r in rows if r['state'] == 403 and r['named'] == 32)
    assert not any(r['state'] in (401, 406) for r in rows if r['ns'] > first_crooked['ns'])
    timing = data['trials'][-1]['delivery_timing']
    assert not timing['condition_failed'] and not timing['dropped']
    condition = timing['conditions'][-1]
    assert condition['status'] == 'READY'
    observation = condition['observation']
    assert observation['original_completed'] and observation['state'] == 403 and observation['named'] == 32
    assert observation['segment']['segment_identity'] == segment
    assert abs(timing['samples'][-1]['write_ms'] - 2644.691) < .000001
    assert data['gate_returncode'] == 0 and data['gate_restored']
    assert data['original_fixture_absence_restored']
    final = data['after']
    assert final['generation'] == 32 and final['revision'] == 539
    assert final['arcade'] and final['state'] == 100 and final['speed'] < .001
    assert not final['present'] and not final['editor_open'] and not final['native_in_menu']
    return dict(neutral_ineligible_brackets=len(neutral), first_ms=times[0], last_ms=times[-1],
                segment=segment, original_completions=[[401, 403]],
                native_return_verified=False, production_acceptance=False,
                gate_reason=gate['reason'],
                limits=['The gate assertion has no captured assertion value or traceback.',
                        'The worker also neutralizes during its conditional wait.',
                        'Equal brackets do not prove atomic-frame contact continuity.',
                        'No production source32 return is qualified.'])


if __name__ == '__main__':
    assert len(sys.argv) == 2, __doc__
    print(json.dumps(analyze(Path(sys.argv[1])), indent=2))
