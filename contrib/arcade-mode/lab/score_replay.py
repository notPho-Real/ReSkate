"""Offline audit of existing sanitized, unowned native score captures.

This schema cannot establish monetary authority. It deliberately cannot upgrade
owner_verified, capture serials or HUD labels to owner/occurrence/point semantics.
Existing mechanic grades remain separate from banking and bail-loss authority.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path

from trick_batch import native_revert_link_result


def replay_trial(trial):
    obs = trial.get('observations', {})
    native = obs.get('native_score', {})
    rows = native.get('samples', [])
    reasons = {'payload_owner_unverified', 'native_occurrence_unverified', 'point_semantics_unverified'}
    if not rows:
        reasons.add('missing_score_events')
    if obs.get('evidence_gap') or any(isinstance(v, dict) and v.get('dropped', 0) for v in obs.values()):
        reasons.add('telemetry_incomplete')
    if native.get('status_dropped') or native.get('guard_dropped') or native.get('malformed'):
        reasons.add('telemetry_incomplete')
    # Old health records have no timestamps delimiting the trial. Equal counters
    # cannot prove that every required event was captured between its boundaries.
    reasons.add('health_boundaries_unverified')
    prior = None
    exact_replays = 0
    distinct_epochs = set()
    for row in rows:
        valid = (isinstance(row, dict) and all(type(row.get(k)) is int and 0 <= row[k] <= 0xffffffffffffffff
                    for k in ('seq', 'ms', 'epoch', 'route', 'reason')) and row.get('seq', 0) > 0 and
                 row.get('epoch', 0) > 0 and row.get('route') in (1, 2, 3, 4) and
                 type(row.get('score')) in (int, float) and math.isfinite(row['score']) and
                 0 <= row['score'] <= 2147483647 and type(row.get('success')) is bool and
                 row.get('owner_verified') is False)
        if not valid:
            reasons.add('unsupported_or_malformed_capture')
            continue
        distinct_epochs.add(row['epoch'])
        if prior:
            if row == prior:
                exact_replays += 1
                continue
            if row['seq'] <= prior['seq'] or row['ms'] < prior['ms']:
                reasons.add('conflicting_or_reordered_capture')
            elif row['seq'] != prior['seq'] + 1:
                reasons.add('capture_gap')
        prior = row
    if len(distinct_epochs) > 1:
        reasons.add('generation_changed')
    guards = native.get('guards', [])
    if len(guards) != len(rows) or not rows:
        reasons.add('delivery_freshness_unverified')
    if any(g.get('decision') in (2, 5) for g in guards):
        reasons.add('rejected_capture')
    combo = obs.get('combo', {})
    complete = (not obs.get('evidence_gap') and combo.get('baseline_available') is True and
                combo.get('dropped') == 0 and obs.get('grind_result', {}).get('evidence') == 'COMPLETE')
    physical_bail = 'PASS' if complete and any(e.get('pose') == 6 for e in combo.get('events', [])) else 'UNVERIFIED'
    return {'name': trial.get('name'), 'monetary_status': 'UNVERIFIED',
            'banked_points': None, 'lost_points': None, 'authoritative_operations': 0,
            'reasons': sorted(reasons), 'native_rows': len(rows), 'exact_capture_replays': exact_replays,
            'native_duplicate_identity': 'UNVERIFIED',
            'recorded_chain': obs.get('chain_result'), 'recorded_grind': obs.get('grind_result'),
            'physical_bail': physical_bail, 'revert_link': native_revert_link_result(obs),
            'neutralized': trial.get('neutralized'), 'reset_ready': trial.get('fixture_reset', {}).get('reset_ready')}


def fault_cases(trial):
    """Fault injection derives from one recorded trial; never new native evidence."""
    for name in ('duplicate_capture', 'missing_score', 'dropped_score', 'truncated_combo', 'interrupted_combo'):
        mutated = copy.deepcopy(trial)
        obs = mutated.setdefault('observations', {})
        native = obs.setdefault('native_score', {})
        if name == 'duplicate_capture' and native.get('samples'):
            native['samples'].insert(1, copy.deepcopy(native['samples'][0]))
        elif name == 'missing_score':
            native['samples'] = []
        elif name == 'dropped_score':
            native['dropped'] = native.get('dropped', 0) + 1
        elif name == 'truncated_combo':
            obs['evidence_gap'] = True
            obs.setdefault('combo', {})['events'] = obs.get('combo', {}).get('events', [])[:-1]
        elif name == 'interrupted_combo':
            events = obs.get('combo', {}).get('events', [])
            if len(events) > 1:
                events[len(events) // 2]['active'] = False
        # Precomputed historical grades no longer describe an altered trial.
        obs.pop('chain_result', None)
        if name in ('truncated_combo', 'interrupted_combo'):
            obs['grind_result'] = {'evidence': 'INCOMPLETE'}
        yield {'fault': name, 'synthetic_mutation': True, 'result': replay_trial(mutated)}


def report(document):
    if document.get('schema') != 1 or not isinstance(document.get('trials'), list) or len(document['trials']) > 64:
        raise ValueError('Expected bounded recorded trial excerpts')
    rows = []
    for item in document['trials']:
        trial = item['trial']
        rows.append({'provenance': item['provenance'], 'recorded': replay_trial(trial),
                     'fault_injection': list(fault_cases(trial))})
    return {'schema': 1, 'new_hardware_acceptance': False,
            'scope': 'Recorded mechanic grades; monetary authority remains UNVERIFIED. Mutations are offline tests.',
            'trials': rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.input.stat().st_size > 8 * 1024 * 1024:
        raise ValueError('Evidence exceeds bound')
    raw = args.input.read_bytes()
    result = report(json.loads(raw))
    result['input_sha256'] = hashlib.sha256(raw).hexdigest()
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Replayed {len(result['trials'])} recorded trials; no authoritative monetary operation established.")


if __name__ == '__main__':
    main()
