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


def diagnostic_evidence(native, rows):
    """Audit optional sequence-end details without promoting serials to identity."""
    reasons = set()
    loss = False
    for key in ('detail_dropped', 'detail_malformed'):
        value = native.get(key)
        if value is None:
            reasons.add('detail_health_unverified')
        elif type(value) is not int or not 0 <= value <= 0xffffffffffffffff:
            reasons.add('malformed_detail_health'); loss = True
        elif value:
            reasons.add(key); loss = True
    statuses = native.get('status', [])
    if not isinstance(statuses, list) or len(statuses) > 32:
        reasons.add('malformed_observer_status'); statuses = []; loss = True
    if not statuses:
        reasons.add('observer_status_unverified')
    for row in statuses:
        if (not isinstance(row, dict) or type(row.get('epoch')) is not int or not 0 < row['epoch'] <= 0xffffffffffffffff
                or any(type(row.get(k)) is not bool for k in ('active', 'bound'))
                or row.get('owner_verified') is not False
                or any(type(row.get(k)) is not int or not 0 <= row[k] <= 0xffffffffffffffff
                       for k in ('dropped', 'rejected', 'contended', 'stale'))
                or (row['bound'] and not row['active'])):
            reasons.add('malformed_observer_status'); loss = True; continue
        if not row['bound']:
            reasons.add('observer_unbound'); loss = True
        if not row['active']:
            reasons.add('observer_inactive'); loss = True
        if any(row[k] for k in ('dropped', 'rejected', 'contended', 'stale')):
            reasons.add('observer_loss_history_unlocated')
    # Optional newer health rows are sampled, lifetime cumulative counters.
    # A nonzero first sample cannot locate the loss inside this trial.
    for key in ('health_dropped', 'health_malformed'):
        if key in native and (type(native[key]) is not int or not 0 <= native[key] <= 0xffffffffffffffff):
            reasons.add('malformed_capture_health'); loss = True
        elif native.get(key):
            reasons.add(key); loss = True
    health = native.get('health', [])
    if not isinstance(health, list) or len(health) > 128:
        reasons.add('malformed_capture_health'); health = []; loss = True
    previous = {}
    for row in health:
        if (not isinstance(row, dict) or any(type(row.get(k)) is not int or not 0 <= row[k] <= 0xffffffffffffffff
                for k in ('ms', 'epoch', 'route', 'unbound', 'expired', 'changed'))
                or not row['epoch'] or row['route'] not in (1, 2, 3, 4)
                or any(type(row.get(k)) is not bool for k in ('active', 'bound', 'overflow'))
                or (row['bound'] and not row['active'])):
            reasons.add('malformed_capture_health'); loss = True; continue
        if not row['bound']:
            reasons.add('observer_unbound'); loss = True
        if not row['active']:
            reasons.add('observer_inactive'); loss = True
        if row['overflow']:
            reasons.add('health_counter_overflow'); loss = True
        prior = previous.get(row['route'])
        counters = ('unbound', 'expired', 'changed')
        if prior is not None and row['epoch'] != prior['epoch']:
            reasons.add('health_generation_changed'); loss = True
        if prior is None:
            if any(row[k] for k in counters):
                reasons.add('health_loss_history_unlocated')
        elif row['ms'] < prior['ms'] or any(row[k] < prior[k] for k in counters):
            reasons.add('health_counter_reset_or_reordered'); loss = True
        elif any(row[k] > prior[k] for k in counters):
            reasons.add('health_loss_between_samples'); loss = True
        previous[row['route']] = row
    expected = {row['seq'] for row in rows if isinstance(row, dict) and row.get('route') == 4
                and type(row.get('seq')) is int and 0 < row['seq'] <= 0xffffffffffffffff}
    details = native.get('details', [])
    if not isinstance(details, list) or len(details) > 128:
        reasons.add('malformed_detail_capture'); details = []; loss = True
    matched, unavailable = set(), False
    for row in details:
        valid = (isinstance(row, dict) and type(row.get('seq')) is int and 0 < row['seq'] <= 0xffffffffffffffff
                 and row.get('authoritative') is False and row.get('verification') == 'UNVERIFIED'
                 and row.get('scalar_status') in ('schema_unavailable', 'read_failed', 'invalid_value', 'available')
                 and row.get('category_status') in ('schema_unavailable', 'read_failed', 'absent', 'available'))
        if valid:
            scalar_available = row['scalar_status'] == 'available'
            category_available = row['category_status'] == 'available'
            if scalar_available:
                valid = (type(row.get('sequence_kind')) is int and 0 <= row['sequence_kind'] <= 0xffffffff
                         and all(type(row.get(k)) in (int, float) and 0 <= row[k] <= 2147483647 and
                                 math.isfinite(row[k]) and float(row[k]).is_integer()
                                 for k in ('action_score', 'base_score'))
                         and type(row.get('landing')) in (int, float) and abs(row['landing']) < 2**128
                         and math.isfinite(row['landing']))
            else:
                valid = all(k in row and row[k] is None for k in ('sequence_kind', 'action_score', 'base_score', 'landing'))
            valid = valid and ((type(row.get('current_category')) is int and 0 <= row['current_category'] <= 0xffffffff)
                               if category_available else ('current_category' in row and row['current_category'] is None))
        if not valid:
            reasons.add('malformed_detail_capture'); loss = True; continue
        if row['seq'] in matched:
            reasons.add('ambiguous_detail_capture'); loss = True
        if row['seq'] not in expected:
            reasons.add('orphan_detail_capture'); loss = True
        else:
            matched.add(row['seq'])
        if not scalar_available or not category_available:
            reasons.add('detail_fields_unverified'); unavailable = True
    missing = len(expected - matched)
    if missing:
        reasons.add('missing_sequence_details')
    if loss:
        reasons.add('telemetry_incomplete')
    detail_status = ('INCOMPLETE' if loss else 'UNVERIFIED' if missing or unavailable or
                     'detail_health_unverified' in reasons else 'COMPLETE' if expected else 'NOT_APPLICABLE')
    return {'status': detail_status, 'sequence_captures': len(expected), 'matched_details': len(matched),
            'missing_details': missing, 'association': 'capture_serial_only'}, reasons, loss


def replay_trial(trial, *, evaluate_revert_link=False):
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
                 type(row.get('score')) in (int, float) and 0 <= row['score'] <= 2147483647 and
                 math.isfinite(row['score']) and type(row.get('success')) is bool and
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
    diagnostics, diagnostic_reasons, diagnostic_loss = diagnostic_evidence(native, rows)
    reasons.update(diagnostic_reasons)
    if len(distinct_epochs) > 1 and native.get('details'):
        reasons.add('detail_epoch_association_unverified')
        if diagnostics['status'] == 'COMPLETE':
            diagnostics['status'] = 'UNVERIFIED'
    guards = native.get('guards', [])
    if len(guards) != len(rows) or not rows:
        reasons.add('delivery_freshness_unverified')
    if any(g.get('decision') in (2, 5) for g in guards):
        reasons.add('rejected_capture')
    combo = obs.get('combo', {})
    complete = (not obs.get('evidence_gap') and combo.get('baseline_available') is True and
                combo.get('dropped') == 0 and obs.get('grind_result', {}).get('evidence') == 'COMPLETE')
    physical_bail = 'PASS' if complete and any(e.get('pose') == 6 for e in combo.get('events', [])) else 'UNVERIFIED'
    capture_incomplete = bool(reasons & {'telemetry_incomplete', 'unsupported_or_malformed_capture',
                                       'conflicting_or_reordered_capture', 'capture_gap', 'rejected_capture'})
    return {'name': trial.get('name'), 'monetary_status': 'UNVERIFIED',
            'banked_points': None, 'lost_points': None, 'authoritative_operations': 0,
            'reasons': sorted(reasons), 'native_rows': len(rows), 'exact_capture_replays': exact_replays,
            'native_diagnostic_evidence': 'INCOMPLETE' if diagnostic_loss or capture_incomplete else 'UNVERIFIED',
            'sequence_details': diagnostics,
            'native_duplicate_identity': 'UNVERIFIED',
            'recorded_chain': obs.get('chain_result'), 'recorded_grind': obs.get('grind_result'),
            'physical_bail': physical_bail,
            'revert_link': native_revert_link_result(obs) if evaluate_revert_link else None,
            'neutralized': trial.get('neutralized'), 'reset_ready': trial.get('fixture_reset', {}).get('reset_ready')}


def fault_cases(trial, *, evaluate_revert_link=False):
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
        yield {'fault': name, 'synthetic_mutation': True,
               'result': replay_trial(mutated, evaluate_revert_link=evaluate_revert_link)}


def report(document):
    if document.get('schema') != 1 or not isinstance(document.get('trials'), list) or len(document['trials']) > 64:
        raise ValueError('Expected bounded recorded trial excerpts')
    rows = []
    for item in document['trials']:
        trial = item['trial']
        # Test selection is metadata, not success evidence. An ordinary landing
        # trial must not become a failed revert test merely because it ends.
        evaluate_revert = item.get('evaluate_revert_link') is True
        rows.append({'provenance': item['provenance'],
                     'recorded': replay_trial(trial, evaluate_revert_link=evaluate_revert),
                     'fault_injection': list(fault_cases(trial, evaluate_revert_link=evaluate_revert))})
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
