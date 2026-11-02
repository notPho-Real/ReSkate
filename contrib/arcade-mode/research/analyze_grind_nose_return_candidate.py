"""Offline prior Nosegrind admission and subsequent native return receipts.

Usage: python analyze_grind_nose_return_candidate.py /path/to/reskate-arcade-lab
The first pinned trial delivered a lateral second leg; its neutral window is
only admission evidence. The second receipt independently executes the return.
Neither establishes production animation/HUD/combo or Ollie acceptance.
"""
import gzip
import hashlib
import json
from pathlib import Path
import sys


def analyze(root):
    path = root / 'grind-native-nose-lateral-c048914-v2/results/two-leg-lateral.json.gz'
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == 'a47227610d824030473948b7dc1641d55b67ebde3891e65fdff8194f2d59d3b6'
    envelope = json.loads(gzip.decompress(blob))
    assert envelope['returncode'] == 0
    data = json.loads(envelope['stdout'])
    rows = data['native_samples']
    segment = [2130303778855, 0]
    eligible, held = [], []
    for row in rows:
        s = row.get('admission_sample')
        if not s or not s['stable_reads'] or s['state'] != 403 or s['named'] != 23:
            continue
        if (s['physical_contact'] != 1 or s['published_contact'] != 1 or
                s['missing_segment'] or s['segment_identity'] != segment):
            continue
        heading, pitch, roll = s['requested_orientation_raw']
        if pitch == 0 and roll == 0:
            assert .312 < heading < .313
            assert s['actor_active'] == s['published_admission'] == 1
            eligible.append(row)
        elif .399 < pitch < .401 and .351 < heading < .352:
            assert s['actor_active'] == s['published_admission'] == 0
            held.append(row)
    assert len(eligible) == 7 and len(held) == 5
    times = [(r['ns'] - rows[0]['ns']) / 1e6 for r in eligible]
    assert abs(times[0] - 2658.839163) < .000001
    assert abs(times[-1] - 2735.068375) < .000001
    assert len({r['admission_sample']['actor'] for r in eligible}) == 1
    for r in eligible:
        outer = r['active_segment']
        assert r['state'] == outer['state'] == 403 and r['named'] == 23
        assert outer['segment_identity'] == segment and not outer['missing']
        a, b = outer['endpoints']
        assert a[1] == b[1] and sum((x-y)**2 for x, y in zip(a, b)) == 144
        assert abs(r['physics_dt'] - 1/30) < .000001
        assert abs(r['cached_dt'] - 1/30) < .000001
    completions = [json.loads(line.split(' ', 1)[1]) for line in data['gate_log']
                   if line.startswith('GRIND_LEG_COMPLETED ')]
    assert [(c['source'], c['target']) for c in completions] == [(401, 403), (403, 402)]
    assert data['gate_returncode'] == 0 and data['gate_restored']
    assert data['original_fixture_absence_restored']
    return dict(neutral_eligible_samples=7, held_ineligible_samples=5,
                first_ms=times[0], last_ms=times[-1], segment=segment,
                observed_completions=[[c['source'], c['target']] for c in completions],
                return_to_50_50_verified=False,
                limits=['Equal bracket reads are not atomic physics frames.',
                        'Cached dt alone is not a fresh clock-cadence verification.',
                        'Neutral admission is not execution, destination identity, animation or combo acceptance.'])


def verified_return(root):
    path = root / 'grind-native-nose-return-c048914-v2/results/two-leg-lateral.json.gz'
    blob = path.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == 'df44e2faebdcb3991bd93f33eec18c96ea56bb9a263f83bf77ba0d9433a26853'
    envelope = json.loads(gzip.decompress(blob))
    assert envelope['returncode'] == 0
    d = json.loads(envelope['stdout'])
    assert not {k:v for k,v in d.items() if 'error' in k}
    rows = d['native_samples'];assert len(rows) == 504
    gate = next(json.loads(line.split(' ',1)[1]) for line in d['gate_log']
                if line.startswith('GRIND_GATE_RESULT '))
    assert gate['reason'] == 'second_native_identity_verified' and gate['legs'] == 2
    assert [(c['source'],c['target']) for c in gate['completed']] == [(401,403),(403,401)]
    boundaries = [e for e in gate['actor_events'] if e['boundary'] == 'completed']
    assert len(boundaries) == 2 and all(e['timing'][-1] == 1 for e in boundaries)
    assert abs(boundaries[-1]['timing'][6] - .9333329200744629) < .000001
    segment = [4191888080935,0]
    destination = [r for r in rows if r['ns'] >= gate['completed'][1]['ns']
                   and r['state'] == 401 and r['named'] == 20]
    assert len(destination) == 77
    for r in destination:
        s = r['admission_sample']; outer = r['active_segment']
        assert s['stable_reads'] and s['state'] == 401 and s['named'] == 20
        assert s['physical_contact'] == s['published_contact'] == r['allowed'] == r['contact'][0] == 1
        assert s['segment_identity'] == outer['segment_identity'] == segment
        assert s['missing_segment'] == outer['missing'] == 0
        a,b = outer['endpoints']
        assert a[1] == b[1] and sum((x-y)**2 for x,y in zip(a,b)) == 144
    assert all(abs(r['physics_dt']-1/30)<.000001 and abs(r['cached_dt']-1/30)<.000001 for r in rows)
    trial = d['trials'][-1]['delivery_timing']
    assert not trial['condition_failed'] and not trial['dropped']
    condition = trial['conditions'][-1]
    assert condition['status'] == 'READY' and condition['observation']['original_completed']
    assert condition['observation']['state'] == 403 and condition['observation']['named'] == 23
    assert condition['observation']['segment']['segment_identity'] == segment
    assert abs(trial['samples'][-1]['write_ms'] - 2658.692) < .000001
    # Retain the non-atomic disagreement during406. Destination evidence above
    # does not erase it or establish uninterrupted physical contact in transit.
    zero = [r for r in rows if r['state'] in (401,403,406) and r['allowed'] == 0]
    assert len(zero) == 1 and zero[0]['state'] == 406
    assert zero[0]['admission_sample']['stable_reads'] and zero[0]['admission_sample']['physical_contact'] == 1
    assert d['gate_returncode'] == 0 and d['gate_restored'] and d['original_fixture_absence_restored']
    final = d['after']
    assert final['generation'] == 23 and final['revision'] == 482 and final['arcade']
    assert final['state'] == 100 and final['speed'] < .001
    assert not final['present'] and not final['editor_open'] and not final['native_in_menu']
    return dict(native_return_verified=True, destination_samples=77, segment=segment,
                first_ms=(destination[0]['ns']-rows[0]['ns'])/1e6,
                last_ms=(destination[-1]['ns']-rows[0]['ns'])/1e6,
                mixed_contact_ms=(zero[0]['ns']-rows[0]['ns'])/1e6,
                production_animation_hud_combo_verified=False)


if __name__ == '__main__':
    assert len(sys.argv) == 2, __doc__
    root = Path(sys.argv[1])
    print(json.dumps(dict(prior_admission=analyze(root),native_return=verified_return(root)), indent=2))
