"""Summarize sanitized fixture batches without running input or interpreting images.

Supply only the four acceptance batches to report the 50-trial denominator.
Setup pilots remain separate. No contact sensor or verified enum dictionary is
available yet, so a missing grind cannot distinguish a miss from failed contact.
"""
import argparse
import json
from pathlib import Path


def trial_result(trial):
    obs = trial.get('observations', {})
    grind = obs.get('grind', {})
    rows = grind.get('samples', [])
    grade = obs.get('grind_result', {})
    valid = (trial.get('input_sent') is True and
             trial.get('fixture_start', {}).get('reset_ready') is True and
             grade.get('evidence') == 'COMPLETE' and not obs.get('evidence_gap') and
             bool(rows) and all(row.get('valid') is True for row in rows))
    active = [row for row in rows if row.get('valid') and 400 <= row['state'] < 500]
    exits = [i for i, row in enumerate(rows) if row.get('valid') and row.get('edge') == 2]
    pairs = {(row['grind'] if row.get('grind_valid') else None,
              row['skate'] if row.get('skate_valid') else None) for row in active}
    return dict(
        name=trial.get('name'), repetition=trial.get('repetition'),
        input_sent=trial.get('input_sent') is True, valid_attempt=valid,
        native_grind=bool(active) if valid else None,
        native_exit=bool(exits) if valid else None,
        riding_after_exit=(any(row['state'] == 100 for row in rows[exits[-1]+1:])
                           if valid and exits else None),
        bail=(any(row['state'] == 300 for row in rows) if valid else None),
        longest_grind_ms=max((row.get('duration', 0) for row in rows), default=0) if valid else None,
        entry_speed_mm_s=[row['speed_mm_s'] if row.get('speed_valid') else None
                          for row in rows if row.get('edge') == 1] if valid else [],
        raw_enum_pairs=sorted(pairs, key=str) if valid else [],
        named_identity='UNKNOWN', miss_vs_contact='UNVERIFIED',
        neutralized=trial.get('neutralized') is True,
        reset_ready=trial.get('fixture_reset', {}).get('reset_ready') is True)


def report(documents):
    batches = []
    for document in documents:
        trials = document.get('trials')
        if not isinstance(trials, list) or len(trials) > 25:
            raise ValueError('Expected a bounded batch result')
        # A hard-stop row can follow its failed trial; it is not another attempt.
        batches.append(dict(trials=[trial_result(t) for t in trials if not t.get('hard_stop')],
                            hard_stops=[t.get('reason') for t in trials if t.get('hard_stop')]))
    trials = [trial for batch in batches for trial in batch['trials']]
    valid = [trial for trial in trials if trial['valid_attempt']]
    successes = sum(trial['native_grind'] is True for trial in valid)
    return dict(schema=1, batches=batches, input_attempts=sum(t['input_sent'] for t in trials),
                valid_attempts=len(valid), excluded_attempts=sum(t['input_sent'] and not t['valid_attempt'] for t in trials),
                native_grinds=successes, native_grind_rate=successes / len(valid) if valid else None,
                native_exits=sum(t['native_exit'] is True for t in valid),
                observed_bails=sum(t['bail'] is True for t in valid),
                neutralization_failures=sum(t['input_sent'] and not t['neutralized'] for t in trials),
                reset_failures=sum(t['input_sent'] and not t['reset_ready'] for t in trials),
                contact_classification='UNVERIFIED', named_enum_dictionary='UNVERIFIED')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results', type=Path, nargs='+')
    args = parser.parse_args()
    documents = []
    if len(args.results) > 16 or len(set(p.resolve() for p in args.results)) != len(args.results):
        raise ValueError('Use at most 16 distinct batch files')
    for path in args.results:
        with path.open('rb') as stream:
            payload = stream.read(64 * 1024 * 1024 + 1)
        if len(payload) > 64 * 1024 * 1024:
            raise ValueError('Batch exceeds 64 MiB')
        documents.append(json.loads(payload))
    print(json.dumps(report(documents), indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
