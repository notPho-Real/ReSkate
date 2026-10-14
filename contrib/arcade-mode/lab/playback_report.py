"""Read-only comparison evidence from sanitized trial JSON; never runs a trial."""
import argparse
import json
from pathlib import Path
import sys

MAX_FILE = 64 * 1024 * 1024
MAX_TRIALS = 24
U64, U32 = 0xffffffffffffffff, 0xffffffff


def number(value, low=0, high=U64):
    return type(value) is int and low <= value <= high


def analyze_trial(trial):
    """Report observed values, never translate an input request into a success."""
    reasons = set()
    trial = trial if isinstance(trial, dict) else {}
    obs = trial.get('observations', {})
    if not isinstance(obs, dict):
        obs = {}
    if obs.get('evidence_gap') or trial.get('evidence_gap'):
        reasons.add('evidence_gap')
    streams, drops = {}, {}
    fields = {
        'playback': {'seq': (0, U64), 'poll': (0, U64), 'tick': (0, U64),
                     'mode': (0, 2), 'input_packet': (0, U32), 'output_packet': (0, U32)},
        'stick': {'seq': (0, U64), 'ms': (0, U64), 'tick': (0, U64), 'user': (0, 3),
                  'x': (-32768, 32767), 'y': (-32768, 32767), 'phase': (0, 3)},
        'flip': {'ms': (0, U64), 'id': (-1, 63)},
    }
    for name in ('playback', 'stick', 'flip', 'pad'):
        block = obs.get(name)
        if not isinstance(block, dict):
            reasons.add('missing_' + name)
            block = {}
        dropped = block.get('dropped')
        drops[name] = dropped if number(dropped) else None
        if dropped is None or not number(dropped):
            reasons.add('unknown_' + name + '_loss')
        elif dropped:
            reasons.add('dropped_' + name)
        if name == 'pad':
            continue
        rows = block.get('samples', [])
        if not isinstance(rows, list):
            reasons.add('invalid_' + name)
            rows = []
        if len(rows) > 128:
            reasons.add('oversized_' + name)
            rows = rows[-128:]
        kept = []
        for row in rows:
            if (not isinstance(row, dict) or
                    any(not number(row.get(key), *bounds) for key, bounds in fields[name].items()) or
                    (name == 'flip' and type(row.get('valid')) is not bool)):
                reasons.add('invalid_' + name)
                continue
            clean = {key: row[key] for key in fields[name]}
            if name == 'flip':
                clean['valid'] = row['valid']
            kept.append(clean)
        streams[name] = kept

    playback, sticks, flips = (streams[key] for key in ('playback', 'stick', 'flip'))
    for name, rows in streams.items():
        for before, after in zip(rows, rows[1:]):
            for key in ('seq', 'tick', 'ms', 'poll'):
                if key in before and (after[key] < before[key] or
                                      (key in ('seq', 'poll') and after[key] == before[key])):
                    reasons.add('nonmonotonic_' + name + '_' + key)
    by_seq = {row['seq']: row for row in sticks}
    joined = []
    for row in playback:
        stick = by_seq.get(row['seq'])
        if stick is None:
            reasons.add('missing_stick_join')
            continue
        if stick['tick'] != row['tick']:
            reasons.add('mismatched_join_tick')
        joined.append({**stick, **row})
    modes = sorted({row['mode'] for row in playback})
    users = sorted({row['user'] for row in joined})
    if len(modes) > 1:
        reasons.add('mixed_modes')
    if len(users) > 1:
        reasons.add('mixed_users')
    if len(joined) < 2:
        reasons.add('insufficient_timing')
    tick_span = joined[-1]['tick'] - joined[0]['tick'] if joined else None
    ms_span = joined[-1]['ms'] - joined[0]['ms'] if joined else None
    if tick_span is not None and tick_span <= 0:
        reasons.add('no_tick_progress')
    if ms_span is not None and ms_span <= 0:
        reasons.add('no_time_progress')
    poll_gaps = [b['poll'] - a['poll'] for a, b in zip(playback, playback[1:])]
    # Runtime samples each active tick and changed output. Cumulative poll
    # counters include reads between rows; those reads are not lost evidence.
    if modes == [2] and any(a['phase'] == b['phase'] == 1 and
                            ((b['output_packet'] - a['output_packet']) & U32) !=
                            ((b['poll'] - a['poll']) & U32)
                            for a, b in zip(joined, joined[1:])):
        reasons.add('reference_packet_poll_mismatch')
    tick_gaps = [b['tick'] - a['tick'] for a, b in zip(joined, joined[1:])]
    changes = []
    for row in joined:
        if not changes or (row['x'], row['y']) != (changes[-1]['x'], changes[-1]['y']):
            changes.append({key: row[key] for key in ('seq', 'poll', 'ms', 'tick', 'x', 'y', 'phase')})
    native = [row for row in flips if row['valid'] and row['id'] > 0]
    fresh = [row for row in native if joined and joined[0]['ms'] <= row['ms'] <= joined[-1]['ms']]
    timing_complete = not reasons
    if not fresh:
        reasons.add('no_fresh_native_rows')
    criteria = trial.get('criteria', {})
    reset = criteria.get('returned_to_idle') if isinstance(criteria, dict) else None
    return {
        'evidence_complete': not reasons, 'timing_complete': timing_complete,
        'incomplete_reasons': sorted(reasons), 'dropped': drops,
        'mode': modes[0] if len(modes) == 1 else None, 'modes': modes, 'users': users,
        'playback_rows': len(playback), 'joined_rows': len(joined),
        'first_ms': joined[0]['ms'] if joined else None,
        'last_ms': joined[-1]['ms'] if joined else None, 'ms_span': ms_span,
        'first_tick': joined[0]['tick'] if joined else None,
        'last_tick': joined[-1]['tick'] if joined else None, 'tick_span': tick_span,
        'ms_per_tick': ms_span / tick_span if tick_span and tick_span > 0 and ms_span is not None and ms_span >= 0 else None,
        'first_poll': playback[0]['poll'] if playback else None,
        'last_poll': playback[-1]['poll'] if playback else None,
        'poll_count_span': playback[-1]['poll'] - playback[0]['poll'] + 1 if playback else None,
        'additional_polls': sum(gap - 1 for gap in poll_gaps)
            if playback and all(gap > 0 for gap in poll_gaps) else None,
        'first_input_packet': playback[0]['input_packet'] if playback else None,
        'last_input_packet': playback[-1]['input_packet'] if playback else None,
        'first_output_packet': playback[0]['output_packet'] if playback else None,
        'last_output_packet': playback[-1]['output_packet'] if playback else None,
        'max_tick_gap': max(tick_gaps, default=None),
        'same_tick_samples': sum(gap == 0 for gap in tick_gaps),
        'output_packet_wraps': sum(a['phase'] == b['phase'] == 1 and
                                   b['output_packet'] < a['output_packet'] and
                                   ((b['output_packet'] - a['output_packet']) & U32) ==
                                   ((b['poll'] - a['poll']) & U32)
                                   for a, b in zip(joined, joined[1:]))
            if modes == [2] else None,
        'changed_xy': changes, 'native_rows': flips,
        'observed_native_ids': sorted({row['id'] for row in native}),
        'fresh_native_ids': sorted({row['id'] for row in fresh}),
        'neutralized': trial.get('neutralized') if type(trial.get('neutralized')) is bool else None,
        'reported_reset': reset if reset in ('PASS', 'FAIL', 'UNVERIFIED') else None,
    }


def analyze_document(document):
    trials = document.get('trials', [document]) if isinstance(document, dict) else document
    if (not isinstance(trials, list) or len(trials) > MAX_TRIALS or
            any(not isinstance(trial, dict) for trial in trials)):
        raise ValueError('Expected a trial object, trial list, or summary with at most 24 trials')
    return {'schema': 1, 'trials': [dict(index=i, **analyze_trial(trial)) for i, trial in enumerate(trials)]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', type=Path)
    args = parser.parse_args(argv)
    with args.path.open('rb') as stream:
        payload = stream.read(MAX_FILE + 1)
    if len(payload) > MAX_FILE:
        raise ValueError('Trial JSON exceeds 64 MiB')
    print(json.dumps(analyze_document(json.loads(payload)), indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, RecursionError):
        print('Playback report rejected invalid, oversized, or unreadable trial JSON.', file=sys.stderr)
        sys.exit(1)
