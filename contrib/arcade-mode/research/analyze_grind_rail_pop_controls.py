"""Offline audit of one pinned native control trial, not production acceptance.

No process/network access. Board velocity is a bracketed native buffer, not a
trajectory-call trace or atomic physics observation. The physical A event in
this receipt belongs to the approach; the rail release was external RS input.
"""
import gzip
import hashlib
import json
from pathlib import Path


def analyze(root):
    path = root / 'grind-native-rail-pop-c048914-v1/results/two-leg-lateral.json.gz'
    packed = path.read_bytes()
    assert hashlib.sha256(packed).hexdigest() == '07cb954934c5c1ba9e8aba607b65ec0838f2abbb8b2eee23ff7369af553bbac7'
    envelope = json.loads(gzip.decompress(packed))
    assert envelope['returncode'] == 0
    d = json.loads(envelope['stdout'])
    assert not any('error' in key for key in d)
    assert d['gate_returncode'] == 0 and d['gate_restored']
    assert d['original_fixture_absence_restored']
    assert d['after']['generation'] == 32 and d['after']['revision'] == 515
    assert d['after']['arcade'] and not d['after']['present']
    assert not d['after']['editor_open'] and not d['after']['native_in_menu']
    rows = d['native_samples']
    assert len(rows) == 456
    assert all(abs(r[k] - 1 / 30) < 1e-7 for r in rows for k in ('physics_dt', 'cached_dt'))
    timing = d['trials'][-1]['delivery_timing']
    assert not timing['condition_failed'] and timing['dropped'] == 0
    conditions = timing['conditions']
    assert [c['phase'] for c in conditions] == [4, 6]
    assert all(c['status'] == 'READY' for c in conditions)
    rail, prepared = (c['observation'] for c in conditions)
    assert rail['state'] == 401 and rail['named'] == 20
    assert prepared['state'] == 403 and prepared['named'] == 24 and prepared['original_completed']
    assert rail['segment']['segment_identity'] == prepared['segment']['segment_identity']
    assert rail['segment']['endpoints'] == prepared['segment']['endpoints']
    ends = rail['segment']['endpoints']
    assert abs(abs(ends[0][2] - ends[1][2]) - 12) < .01
    gate = next(json.loads(s.split(' ', 1)[1]) for s in d['gate_log'] if s.startswith('GRIND_GATE_RESULT '))
    assert gate['reason'] == 'first_native_5_0_verified' and gate['legs'] == 1
    assert len(gate['completed']) == 1
    assert gate['completed'][0]['source'] == 401 and gate['completed'][0]['target'] == 403
    assert gate['completed'][0]['ns'] < prepared['ns']
    writes = timing['samples']
    assert len(writes) == 10
    assert abs(writes[7]['write_ms'] - writes[6]['write_ms'] - 300) < 1
    assert abs(writes[8]['write_ms'] - writes[7]['write_ms'] - 1000 / 30) < 1
    assert abs(writes[9]['write_ms'] - writes[7]['write_ms'] - 100) < 1
    # Writer and reader have different origins. The native condition's ns is
    # collected just before its completion; this is an estimate, not an atomic
    # synchronization claim. Native event ordering below uses ns exclusively.
    origin_estimate = prepared['ns'] - conditions[1]['completed_ms'] * 1e6
    release_estimate = origin_estimate + writes[7]['write_ms'] * 1e6
    hold = [r for r in rows if prepared['ns'] <= r['ns'] < release_estimate]
    assert hold and all(r['state'] == 403 and r['named'] == 24 for r in hold)
    assert all(r['allowed'] == r['contact'][0] == 1 for r in hold)
    assert all(r['active_segment']['segment_identity'] == rail['segment']['segment_identity'] for r in hold)
    assert all(r['board_motion']['crouch_mode'] == 1 and r['flip_selection']['flip'] == -1 for r in hold)
    assert all(r['board_motion']['velocity_buffer_m_s'][1] < .3 for r in hold)
    fresh = next(r for r in rows if r['ns'] > prepared['ns'] and r['flip_selection']['flip'] == 1)
    assert fresh['ns'] > release_estimate and fresh['state'] == 403
    assert fresh['flip_selection']['stable_reads'] and fresh['board_motion']['crouch_mode'] == 0
    air = next(r for r in rows if r['ns'] > fresh['ns'] and r['state'] == 200)
    motion = air['board_motion']
    assert motion['stable_reads'] and motion['state'] == 200 and motion['crouch_mode'] == 2
    assert not motion['velocity_pending'] and motion['velocity_buffer_m_s'][1] > 3
    assert air['flip_selection']['stable_reads'] and air['flip_selection']['flip'] == 1
    assert not any(r['state'] == 701 for r in rows if prepared['ns'] <= r['ns'] <= air['ns'])
    flight = [r for r in rows if air['ns'] <= r['ns'] < rows[0]['ns'] + 3900e6]
    assert any(r['state'] == 201 and r['flip_selection']['flip'] == 1 for r in flight)
    assert any(r['board_motion']['velocity_pending'] for r in flight)
    assert any(not r['board_motion']['stable_reads'] for r in rows)  # Preserve mixed transition observation.
    assert all(r['flip_selection']['selection'] == fresh['flip_selection']['selection'] for r in rows)
    t0 = rows[0]['ns']
    return dict(samples=len(rows), stable_motion=sum(r['board_motion']['stable_reads'] for r in rows),
                source_segment=rail['segment']['segment_identity'], endpoints=ends,
                writer_down_ms=writes[6]['write_ms'], writer_release_ms=writes[7]['write_ms'],
                writer_upper_arc_ms=writes[8]['write_ms'], writer_neutral_ms=writes[9]['write_ms'],
                reader_fresh_ollie_ms=(fresh['ns']-t0)/1e6,
                reader_air200_ms=(air['ns']-t0)/1e6,
                air200_buffer_velocity=motion['velocity_buffer_m_s'],
                physical_A_rail_acceptance=False, early_flip_acceptance=False,
                trajectory_call_observed=False, integrated_production_acceptance=False)


if __name__ == '__main__':
    import sys
    root = Path(sys.argv[1]) if len(sys.argv) == 2 else Path.home() / '.local/share/reskate-arcade-lab'
    print(json.dumps(analyze(root), indent=2))
