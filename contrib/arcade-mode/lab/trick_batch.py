"""Finite data-only HIL batches. Never deploy, restore, publish, or execute data as code."""
import argparse
import base64
import hashlib
import json
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path

MAX_PLAN = 65536
MAX_TRIALS = 24
AXIS_CODES = {"lx": 0, "ly": 1, "lt": 2, "rx": 3, "ry": 4, "rt": 5, "dx": 16, "dy": 17}
BUTTON_CODES = {"A": 0x130, "B": 0x131, "X": 0x133, "Y": 0x134, "LB": 0x136, "RB": 0x137}
HIL_ROW = re.compile(r"ARCADE HIL STATE: seq=([0-9]{1,20}) valid=(true|false) state=([0-9]{1,10}) mode=([0-9]{1,10})\.")
BUTTON_ROW = re.compile(r"ARCADE MODE DEV: ([ABXY]) press #([0-9]{1,20}) seen on ReSkate client tick\.")


class HardStop(RuntimeError):
    pass


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError("Integer outside bounded range")
    return value


def keys(value, required, optional=()):
    if not isinstance(value, dict) or set(value) - set(required) - set(optional) or set(required) - set(value):
        raise ValueError("Unknown or missing data fields")


def validate_phase_wait(value):
    keys(value, ('condition', 'timeout_ms'), ('max_speed_mm_s',))
    if value['condition'] != 'native-riding':
        raise ValueError('Unknown phase condition')
    integer(value['timeout_ms'], 100, 3000)
    if 'max_speed_mm_s' in value:
        integer(value['max_speed_mm_s'], 0, 20000)


def validate_plan(plan):
    keys(plan, ("schema", "source_commit", "dll_sha256", "stock_sha256", "candidates"))
    if type(plan["schema"]) is not int or plan["schema"] != 1:
        raise ValueError("Unsupported schema")
    for field, length in (("source_commit", 40), ("dll_sha256", 64), ("stock_sha256", 64)):
        if not isinstance(plan[field], str) or not re.fullmatch("[0-9a-f]{%d}" % length, plan[field]):
            raise ValueError("Invalid source or hash")
    cases = plan["candidates"]
    if not isinstance(cases, list) or not 1 <= len(cases) <= MAX_TRIALS:
        raise ValueError("Batch must contain 1..24 candidates")
    names, trials = set(), 0
    for case in cases:
        keys(case, ("name", "family", "phases", "expected", "timeout_ms", "reset_ms"),
             ("repeat", "capture_frames", "baseline_max_speed_mm_s", "fixture_reset", "score_capture_ready", "trajectory_capture"))
        if 'trajectory_capture' in case and (type(case['trajectory_capture']) is not bool or
                (case['trajectory_capture'] and (case.get('fixture_reset') is not True or case.get('score_capture_ready')))):
            raise ValueError('Trajectory capture requires fixture reset and separate scoring capture')
        if "fixture_reset" in case and type(case["fixture_reset"]) is not bool:
            raise ValueError("Fixture reset must be boolean")
        if 'score_capture_ready' in case and (type(case['score_capture_ready']) is not bool or
                (case['score_capture_ready'] and case.get('fixture_reset') is not True)):
            raise ValueError('Score capture readiness requires a boolean and validated fixture baseline')
        for field in ("name", "family"):
            if not isinstance(case[field], str) or not re.fullmatch(r"[a-z][a-z0-9-]{0,47}", case[field]):
                raise ValueError("Invalid candidate label")
        if case["name"] in names:
            raise ValueError("Duplicate candidate")
        names.add(case["name"])
        trials += integer(case.get("repeat", 1), 1, 20)
        if integer(case.get("capture_frames", 4), 0, 4) not in (0, 1, 4):
            raise ValueError("Capture count must be zero, one or four")
        timeout = integer(case["timeout_ms"], 3000, 12000)
        integer(case["reset_ms"], 2000, 10000)
        if "baseline_max_speed_mm_s" in case:
            integer(case["baseline_max_speed_mm_s"], 0, 20000)
        keys(case["expected"], ("states",), ("trick", "after_grind", "chain"))
        if 'chain' in case['expected']:
            chain = case['expected']['chain']
            keys(chain, ('poses', 'air_ids'), ('minimum_balance_ms', 'grace_ms'))
            if not isinstance(chain['poses'], list) or not 2 <= len(chain['poses']) <= 12:
                raise ValueError('Invalid native chain poses')
            for pose in chain['poses']:
                integer(pose, 1, 5)
            if (not isinstance(chain['air_ids'], list) or
                    len(chain['air_ids']) != chain['poses'].count(2)):
                raise ValueError('Native air identities must match chain takeoffs')
            for identity in chain['air_ids']:
                integer(identity, 1, 63)
            integer(chain.get('minimum_balance_ms', 200), 0, 2000)
            grace = chain.get('grace_ms', [350, 650])
            if not isinstance(grace, list) or len(grace) != 2:
                raise ValueError('Invalid lifecycle grace window')
            integer(grace[0], 0, 2000)
            integer(grace[1], grace[0], 2000)
        if "after_grind" in case["expected"] and (type(case["expected"]["after_grind"]) is not bool or
                "trick" not in case["expected"]):
            raise ValueError("Post-grind identity requires a boolean scope and named trick")
        states = case["expected"]["states"]
        if not isinstance(states, list) or len(states) > 12:
            raise ValueError("Invalid expected states")
        for state in states:
            integer(state, 0, 10000)
        if "trick" in case["expected"] and (not isinstance(case["expected"]["trick"], str) or
                not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,47}", case["expected"]["trick"])):
            raise ValueError("Invalid expected trick")
        phases = case["phases"]
        if not isinstance(phases, list) or not 1 <= len(phases) <= 64:
            raise ValueError("Invalid phases")
        previous, waits = -1, 0
        for phase in phases:
            keys(phase, ("ms",), tuple(AXIS_CODES) + ("buttons", "wait"))
            if 'wait' in phase:
                validate_phase_wait(phase['wait'])
                waits += 1
            ms = integer(phase["ms"], 0, timeout - 1000)
            if ms <= previous:
                raise ValueError("Phases must increase strictly")
            previous = ms
            for axis in AXIS_CODES:
                limit = (0, 255) if axis in ("lt", "rt") else (-1, 1) if axis in ("dx", "dy") else (-32768, 32767)
                integer(phase.get(axis, 0), *limit)
            buttons = phase.get("buttons", [])
            if (not isinstance(buttons, list) or any(not isinstance(b, str) or b not in BUTTON_CODES for b in buttons)
                    or len(set(buttons)) != len(buttons)):
                raise ValueError("Invalid buttons")
        if waits > 2 or (waits and case.get('capture_frames', 4) != 0):
            raise ValueError('Conditioned trials require zero captures and at most two waits')
    if trials > MAX_TRIALS:
        raise ValueError("At most 24 trials per batch")
    return plan


def phase_events(case):
    # Full states, not deltas. Linux Y is down-positive, unlike XInput.
    return [(p["ms"] / 1000, [(3, code, p.get(axis, 0)) for axis, code in AXIS_CODES.items()] +
             [(1, code, int(button in p.get("buttons", []))) for button, code in BUTTON_CODES.items()])
            for p in case["phases"]]


def condition_observations(text):
    """Only the existing native fields used by the fixed riding predicate."""
    rows = [{"seq": int(m[1]), "valid": m[2] == "true", "state": int(m[3]), "mode": int(m[4])}
            for m in HIL_ROW.finditer(text)]
    return {'states': rows[-128:], 'combo': parse_combo_samples(text),
            'push': parse_push_samples(text), 'grind': parse_grind_observations(text)}


def observations(text):
    buttons = [{"button": m[1], "count": int(m[2])} for m in BUTTON_ROW.finditer(text)]
    return dict(condition_observations(text), **{"buttons": buttons[-32:],
            "score": parse_score_samples(text), "pad": parse_pad_samples(text),
            "flip": parse_flip_observations(text), "stick": parse_stick_samples(text),
            "playback": parse_playback_samples(text), "actions": parse_action_samples(text),
            "score_edges": parse_score_edges(text), "score_labels": parse_score_presence(text), "score_numbers": parse_score_numbers(text),
            "native_score": parse_native_score_events(text), "fixture": parse_fixture_status(text),
            "fixture_trajectory": parse_fixture_trajectory(text)})


def subsequence(expected, rows):
    index = 0
    for row in rows:
        if index < len(expected) and row["valid"] and row["state"] == expected[index]:
            index += 1
    return index == len(expected)


def named_flip_result(expected, observation, log_boundary, after_grind=False):
    # Exact current-game FlipTrickType values from the reviewed native mapping.
    identifiers = {"ollie": 1, "kickflip": 2, "heelflip": 3, "pop-shove-it": 4,
                   "varial-kickflip": 5, "inward-heelflip": 6, "varial-heelflip": 8,
                   "hardflip": 9, "360-shove-it": 10, "360-flip": 11,
                   "360-inward-heelflip": 12, "360-heelflip": 14, "360-hardflip": 15,
                   "nollie": 16, "nollie-kickflip": 17, "nollie-heelflip": 18,
                   "nollie-pop-shove-it": 19, "nollie-hardflip": 20, "nollie-varial-heelflip": 21,
                   "nollie-varial-kickflip": 23, "nollie-inward-heelflip": 24,
                   "nollie-360-hardflip": 26, "nollie-360-heelflip": 27,
                   "nollie-360-shove-it": 28, "nollie-360-flip": 29, "nollie-360-inward-heelflip": 30,
                   "impossible": 36, "nollie-impossible": 38}
    wanted = identifiers.get(expected)
    native = observation.get("flip", {})
    if wanted is None or log_boundary is None or observation.get("evidence_gap") or native.get("dropped", 0):
        return "UNVERIFIED"
    after_ms = -1
    if after_grind:
        grade = observation.get('grind_result', {})
        if grade.get('evidence') != 'COMPLETE' or grade.get('native_grind') != 'PASS':
            return 'UNVERIFIED'
        rows = observation.get('grind', {}).get('samples', [])
        entries = [r['ms'] for r in rows if r.get('valid') and r.get('edge') == 1]
        if not entries:
            return 'UNVERIFIED'
        after_ms = max(entries)
        if not any(r.get('valid') and r.get('edge') == 2 and r['ms'] > after_ms for r in rows):
            return 'UNVERIFIED'
    seen = {row["id"] for row in native.get("samples", []) if row.get("valid") and row["id"] > 0
            and (not after_grind or row.get('ms', -1) > after_ms)}
    return "PASS" if wanted in seen else "FAIL" if seen else "UNVERIFIED"


def grind_result(observation, baseline, reset_ok):
    current, before = observation.get('grind', {}), baseline.get('grind', {})
    rows = current.get('samples', [])
    pads = current.get('pad', [])
    old_status, status = before.get('status', []), current.get('status', [])
    old = old_status[-1] if old_status else None
    # The background scanner captures its generation before scanning, then logs
    # health after draining the queue. A menu handoff can make that health row
    # older than already-delivered physics. Only discard provably older health
    # whose loss counters were already covered by the validated baseline.
    stale = [s for s in status if old and s['generation'] < old['generation'] and
             s['dropped'] <= old['dropped'] and s['rejected'] <= old['rejected']]
    health = [s for s in status if s not in stale]
    generations = {r['generation'] for r in rows}
    uncertain = (not rows or not pads or old is None or not old['active'] or not health or observation.get('evidence_gap') or
                 current.get('dropped', 0) or len(generations) != 1 or
                 any(not r['valid'] or r['edge'] == 3 for r in rows) or
                 any(r['generation'] != old['generation'] or r['dropped'] != old['dropped'] for r in rows) or
                 any(p['generation'] != old['generation'] or p['dropped'] != old['dropped'] for p in pads) or
                 any(not s['active'] or s['generation'] != old['generation'] or s['dropped'] != old['dropped'] or
                     s['rejected'] != old['rejected'] for s in health))
    active = [r for r in rows if r['valid'] and 400 <= r['state'] < 500]
    # A brief300 state may fall between grind samples. The independently
    # observed wipeout edge still proves a bail; input intent never does.
    combo = observation.get('combo', {})
    wipeout = (combo.get('baseline_available', False) and not combo.get('dropped', 0) and
               not observation.get('evidence_gap') and
               any(r.get('pose') == 6 for r in combo.get('events', [])))
    physics_bail = any(r['valid'] and r['state'] == 300 for r in rows)
    return dict(evidence='INCOMPLETE' if uncertain else 'COMPLETE',
                native_grind='UNVERIFIED' if uncertain else 'PASS' if active else 'FAIL',
                entries=sum(r['edge'] == 1 for r in rows), exits=sum(r['edge'] == 2 for r in rows),
                duration_ms=max((r['duration'] for r in rows), default=0),
                raw_enum_pairs=sorted({(r['grind'] if r['grind_valid'] else None,
                                       r['skate'] if r['skate_valid'] else None) for r in active}, key=str),
                entry_speed_mm_s=[r['speed_mm_s'] if r['speed_valid'] else None for r in rows if r['edge'] == 1],
                named_identity='UNKNOWN', contact_without_grind='UNVERIFIED',
                stale_health_rows=len(stale),
                bail=physics_bail or bool(wipeout),
                bail_evidence='native_physics' if physics_bail else 'native_wipeout' if wipeout else 'UNVERIFIED',
                reset='PASS' if reset_ok else 'FAIL')


def native_revert_link_result(observation):
    """Grade one landing/revert/balance link from HIL41 evidence, never RT intent.

    This is a postprocessor for a single continuation trial. It does not assert
    score ownership, side identity, or multiplier behavior.
    """
    result = dict(native_revert='UNVERIFIED', revert_continuation='UNVERIFIED')
    combo = observation.get('combo', {})
    edges = observation.get('score_edges', {})
    events = combo.get('events', [])
    if (observation.get('evidence_gap') or not combo.get('baseline_available') or
            combo.get('dropped') != 0 or edges.get('dropped') != 0 or
            observation.get('grind_result', {}).get('evidence') != 'COMPLETE' or
            not events or any(b['ms'] < a['ms'] for a, b in zip(events, events[1:]))):
        return result
    links = [r for r in combo.get('revert_links', []) if r.get('accepted') is True]
    if not links:
        result['revert_continuation'] = 'FAIL'
        return result
    # A single landing grants one continuation. Duplicate acceptance is a failure,
    # even if the later attempt happens to reach a manual before its new deadline.
    if len(links) != 1:
        result['revert_continuation'] = 'FAIL'
        return result
    link = links[0]
    matching = [e for e in edges.get('samples', [])
                if e.get('seq') == link.get('sequence') and e.get('seq', 0) > 0]
    if len(matching) != 1:
        return result
    edge = matching[0]
    if (edge.get('labels') not in (4, 8, 16777216) or
            edge.get('labels') != link.get('labels') or edge.get('uncertainty') != 0 or
            not 0 <= edge['sampled'] - edge['observed'] <= 150 or
            not 0 <= link['ms'] - edge['sampled'] <= 150):
        return result
    result['native_revert'] = 'PASS'
    if not 0 < link.get('remaining_ms', 0) <= 1100:
        result['revert_continuation'] = 'FAIL'
        return result
    before = [e for e in events if e['ms'] <= link['ms']]
    if not before or before[-1]['pose'] != 1 or not before[-1]['active']:
        result['revert_continuation'] = 'FAIL'
        return result
    deadline = link['ms'] + link['remaining_ms']
    following = [e for e in events if link['ms'] < e['ms'] <= deadline]
    consumed = next((e for e in following if e['pose'] in (2, 3, 4, 5)), None)
    if consumed is None:
        result['revert_continuation'] = 'FAIL'
        return result
    segment = [before[-1]] + [e for e in following if e['ms'] <= consumed['ms']]
    modes = ('fatigue', 'grind_fatigue', 'manual_fatigue')
    continuous = (all(e['active'] and not e['failed'] for e in segment) and
                  all(b[k] + 1e-6 >= a[k] for a, b in zip(segment, segment[1:]) for k in modes))
    result['revert_continuation'] = 'PASS' if continuous else 'FAIL'
    result['revert_to_action_ms'] = consumed['ms'] - link['ms']
    return result


def combo_chain_result(expected, observation):
    """Qualify one observed lifecycle, never stitch successes across a bank/bail."""
    result = dict(continuous='UNVERIFIED', native_air_ids='UNVERIFIED',
                  fatigue='UNVERIFIED', lifecycle_grace='UNVERIFIED', score_banking='UNVERIFIED')
    combo, flip = observation.get('combo', {}), observation.get('flip', {})
    events = combo.get('events', [])
    if (observation.get('evidence_gap') or not combo.get('baseline_available') or combo.get('dropped') or flip.get('dropped') or
            observation.get('grind_result', {}).get('evidence') != 'COMPLETE' or not events or
            any(b['ms'] < a['ms'] for a, b in zip(events, events[1:]))):
        return result
    first = next((i for i, e in enumerate(events) if e['active']), None)
    if first is None:
        result['continuous'] = 'FAIL'
        return result
    end = next((i for i in range(first + 1, len(events)) if not events[i]['active']), len(events))
    segment = events[first:end]
    matched, cursor = [], 0
    minimum = expected.get('minimum_balance_ms', 200)
    for pose in expected['poses']:
        found = None
        for i in range(cursor, len(segment)):
            event = segment[i]
            following = next((e['ms'] for e in events[first + i + 1:] if e['pose'] != pose or not e['active']), event['ms'])
            if event['pose'] == pose and (pose not in (3, 4, 5) or following - event['ms'] >= minimum):
                found = i
                break
        if found is None:
            result['continuous'] = 'FAIL'
            return result
        matched.append(segment[found])
        cursor = found + 1
    result['continuous'] = 'FAIL' if any(e['failed'] for e in segment) else 'PASS'
    modes = ('fatigue', 'grind_fatigue', 'manual_fatigue')
    result['fatigue'] = 'PASS' if all(b[k] + 1e-6 >= a[k] for a, b in zip(segment, segment[1:]) for k in modes) else 'FAIL'
    if result['fatigue'] == 'FAIL':
        result['continuous'] = 'FAIL' # Bank/restart can happen within one sampled tick.
    grades = []
    for event, wanted in zip((e for e in matched if e['pose'] == 2), expected['air_ids']):
        # Native selection precedes the observed pop; never credit a much older approach ID.
        seen = {r['id'] for r in flip.get('samples', []) if r.get('valid') and r['id'] > 0 and
                event['ms'] - 200 <= r['ms'] <= event['ms'] + 150}
        grades.append('PASS' if wanted in seen else 'FAIL' if seen else 'UNVERIFIED')
    result['native_air_ids'] = 'FAIL' if 'FAIL' in grades else 'UNVERIFIED' if 'UNVERIFIED' in grades else 'PASS'
    if end < len(events) and matched[-1]['pose'] == 1:
        delay = events[end]['ms'] - matched[-1]['ms']
        low, high = expected.get('grace_ms', [350, 650])
        result['lifecycle_grace'] = 'PASS' if (low <= delay <= high and not events[end]['failed'] and
                                            all(events[end][k] == 0 for k in modes)) else 'FAIL'
        result['grace_ms'] = delay
    return result


def bounded_partial_run(error):
    """Only fixed writer diagnostics, never exception messages or raw strings."""
    value = getattr(error, 'partial_run', None)
    required = {'schema', 'outcome', 'game_response', 'input_sent', 'input_may_have_been_sent',
                'neutralized', 'neutralization', 'delivery_timing', 'kernel_pad', 'sdl_pad'}
    allowed_keys = required | {
        'attempted', 'write_completed', 'elapsed_ms', 'samples', 'dropped', 'scope',
        'conditions', 'condition_failed', 'interrupted', 'interrupted_phase',
        'scheduled_ms', 'write_ms', 'ready_check_ms', 'write_and_observe_ms',
        'effective_scheduled_ms', 'phase', 'nominal_ms', 'started_ms', 'completed_ms',
        'wait_ms', 'shift_ms', 'status', 'cause', 'last_native', 'heartbeat_seq',
        'native_ms', 'generation', 'state', 'mode', 'valid', 'speed_valid',
        'speed_mm_s', 'contacts', 'available', 'stage', 'errno', 'ns', 'lt', 'rt'}
    allowed_strings = {'INTERRUPTED', 'UNVERIFIED', 'uinput_write_not_game_poll',
        'READY', 'TIMEOUT', 'fresh_native_riding', 'native_not_riding',
        'waiting_fresh_heartbeat', 'waiting_current_owner_state', 'speed_above_bound',
        'native_context_only_not_action_result'}
    def bounded(node, depth=0):
        if depth > 7:
            return False
        if node is None or type(node) is bool:
            return True
        if type(node) in (int, float):
            return -2**63 <= node <= 2**64 - 1
        if type(node) is str:
            return node in allowed_strings
        if type(node) is list:
            return len(node) <= 128 and all(bounded(row, depth + 1) for row in node)
        return (type(node) is dict and set(node) <= allowed_keys and
                all(bounded(row, depth + 1) for row in node.values()))
    if (not isinstance(value, dict) or set(value) != required or value['schema'] != 1 or
            value['outcome'] != 'INTERRUPTED' or value['game_response'] != 'UNVERIFIED' or
            any(type(value[key]) is not bool for key in
                ('input_sent', 'input_may_have_been_sent', 'neutralized')) or not bounded(value)):
        return None
    encoded = json.dumps(value, allow_nan=False)
    return json.loads(encoded) if len(encoded.encode()) <= 262144 else None


def collect_fixture_trajectory(env, baseline):
    # Re-read only this diagnostic from its original boundary. Other native
    # streams retain their post-handoff trial boundary and grading semantics.
    unavailable = dict(samples=[], status=None, malformed=0, dropped=0, sequence_missing=0,
                       evidence_gap=True, capture_completeness='UNVERIFIED', owner_proof='UNVERIFIED',
                       prefix_status='UNVERIFIED')
    try:
        boundary, prefix = baseline['log_start'], baseline['prefix']
        if type(boundary) is not int or boundary < 0 or not isinstance(prefix, list) or not 1 <= len(prefix) <= 80:
            return unavailable
        observed = env.observe_trajectory(since=boundary)
        result = observed.get('fixture_trajectory')
        if not isinstance(result, dict) or not isinstance(result.get('samples'), list):
            return unavailable
        result = dict(result)
        matches = (result['samples'][:len(prefix)] == prefix and
                   all(row.get('session') == baseline['session'] for row in result['samples']))
        result['prefix_status'] = 'MATCHED' if matches else 'UNVERIFIED'
        result['evidence_gap'] = bool(result.get('evidence_gap', True) or observed.get('evidence_gap') or not matches)
        return result
    except (KeyError, TypeError, ValueError, OSError):
        return unavailable


def perform_case(plan, case, env):
    env.integrity(plan)
    fixture_start = None
    if case.get('fixture_reset'):
        fixture_start = env.fixture_baseline(case['reset_ms'])
    readiness = ({"max_speed_mm_s": case["baseline_max_speed_mm_s"]}
                 if "baseline_max_speed_mm_s" in case else {})
    baseline = env.wait_idle(case["reset_ms"], **readiness)
    if baseline is None:
        return {"name": case["name"], "outcome": "SKIPPED_CONTEXT", "input_sent": False,
                "reason": "No fresh on-board baseline satisfying the requested speed bound" if readiness else
                          "No fresh stable on-board heartbeat", "frames": [], "observations": {}}
    baseline_observation = env.observe()
    if case.get('fixture_reset'):
        baseline_observation = env.wait_grind_ready(case['reset_ms'])
    score_readiness = None
    if case.get('score_capture_ready'):
        baseline_observation, score_readiness = env.wait_score_capture_ready(
            case['reset_ms'], baseline_observation, **readiness)
    trajectory_readiness = None
    if case.get('trajectory_capture'):
        baseline_observation, trajectory_readiness = env.arm_fixture_trajectory(baseline_observation, fixture_start)
    before = baseline_observation.get("combo", {}).get("samples", [])
    watermark = before[-1]["ms"] if before else None
    try:
        result = env.gesture(case)
        if trajectory_readiness is not None:
            result['trajectory_baseline'] = trajectory_readiness
        if score_readiness is not None:
            result['score_capture_baseline'] = score_readiness
    except Exception as error:
        partial = bounded_partial_run(error)
        if partial is not None:
            partial.update(observations={}, observation_status='BASELINE_UNAVAILABLE',
                criteria={key: 'UNVERIFIED' for key in ('native_state_sequence',
                    'heartbeat_state_sequence', 'physics_state_sequence', 'named_trick',
                    'returned_to_idle', 'score_banking')}, reset_status='NOT_ATTEMPTED')
            if score_readiness is not None:
                partial['score_capture_baseline'] = score_readiness
            if trajectory_readiness is not None:
                partial['trajectory_baseline'] = trajectory_readiness
            boundary = baseline_observation.get('log_end')
            if type(boundary) is int:
                try:
                    # One read only. Never recover, reset or re-arm after a stop.
                    observed = env.observe(since=boundary)
                    if trajectory_readiness is not None:
                        observed['fixture_trajectory'] = collect_fixture_trajectory(env, trajectory_readiness)
                    encoded = json.dumps(observed, allow_nan=False)
                    if len(encoded.encode()) <= 1024 * 1024:
                        partial['observations'] = observed
                        partial['observation_status'] = 'AVAILABLE'
                    else:
                        partial['observation_status'] = 'BOUND_EXCEEDED'
                except Exception:
                    partial['observation_status'] = 'UNAVAILABLE'
            error.partial_trial = partial
        raise
    # Gesture's finally neutralizes BEFORE waiting on screenshots or reset.
    env.integrity(plan)
    # A persistent manual need not end before the verified native fixture reset.
    idle_ms = min(case["reset_ms"], 2000) if case.get('fixture_reset') else case["reset_ms"]
    reset = env.wait_idle(idle_ms, **readiness)
    log_boundary = baseline_observation.get("log_end")
    obs = env.observe(since=log_boundary)
    if trajectory_readiness is not None:
        obs['fixture_trajectory'] = collect_fixture_trajectory(env, trajectory_readiness)
    obs["states"] = [r for r in obs["states"] if r["seq"] > baseline]
    combo = obs.get("combo", {})
    # Changed-state samples need their own timestamp watermark: heartbeat seq
    # cannot delimit them. Without a baseline do not attribute historical rows.
    if log_boundary is None:
        combo["samples"] = [r for r in combo.get("samples", [])
                            if watermark is not None and r["ms"] > watermark]
    combo["baseline_available"] = log_boundary is not None or watermark is not None
    combo["native_event_identity"] = "UNVERIFIED"
    obs["combo"] = combo
    if 'grind' in obs:
        obs['grind_result'] = grind_result(obs, baseline_observation, reset is not None)
    if 'chain' in case['expected']:
        obs['chain_result'] = combo_chain_result(case['expected']['chain'], obs)
    wanted = case["expected"]["states"]
    heartbeat_match = bool(wanted) and subsequence(wanted, obs["states"])
    physics = [{"valid": True, "state": row["state"]} for row in combo["samples"]]
    physics_complete = combo["baseline_available"] and not combo.get("dropped", 0) and not obs.get("evidence_gap")
    physics_match = bool(wanted) and physics_complete and subsequence(wanted, physics)
    # A 250ms heartbeat routinely misses a short pop. Absence in a sampled
    # trace is not proof of failed gameplay; a complete observed sequence is
    # positive evidence. Named tricks still require a separate native signal.
    state_result = "PASS" if heartbeat_match or physics_match else "UNVERIFIED"
    result.update(name=case["name"], outcome="MISSED_CONDITION" if
                  result.get('delivery_timing', {}).get('condition_failed') else "OBSERVED", observations=obs,
                  criteria={"native_state_sequence": state_result,
                            "heartbeat_state_sequence": "PASS" if heartbeat_match else "UNVERIFIED",
                            "physics_state_sequence": "PASS" if physics_match else "UNVERIFIED",
                            "returned_to_idle": "PASS" if reset is not None else "FAIL",
                            "named_trick": named_flip_result(case["expected"].get("trick"), obs, log_boundary,
                                                            case['expected'].get('after_grind', False))}, neutralized=True)
    if case.get('fixture_reset'):
        # Preserve trial evidence before the console/native reset changes owner generation.
        result['fixture_start'] = fixture_start
        try:
            result['fixture_reset'] = env.fixture_reset(case['reset_ms'])
        except Exception as error:
            result['fixture_reset'] = {'reset_ready': False, 'reason': stop_reason(error)}
    # Named success requires an independent native identifier from this trial.
    return result


def stop_reason(error):
    # Retain only exact reviewed categories, never exception text containing
    # process information, paths, command output, or player data.
    categories = {
        "installation_integrity": "installation_integrity",
        "stock_backup_integrity": "stock_backup_integrity",
        "log_path_integrity": "log_path_integrity",
        "game_or_hil_unavailable_after_recovery_wait": "game_or_hil_unavailable",
        "Skate exited; test stopped": "input_readiness_lost",
        "Readiness inspection did not stop": "readiness_worker_timeout",
        "Session input/capture context changed": "input_capture_context_changed",
        "Gamescope changed; screenshot refused": "capture_owner_changed",
        "No fresh Gamescope PNG/AVIF; capture interface unverified": "capture_timeout",
        "Bounded experiment interrupted": "watchdog_or_interruption",
        "Bounded session deadline reached": "session_deadline",
        "Incomplete uinput write": "input_write_incomplete",
        "fixture_window_invalid": "fixture_window_invalid",
        "fixture_reset_unverified": "fixture_reset_unverified",
        "fixture_native_menu_open": "fixture_native_menu_open",
        "fixture_native_menu_unverified": "fixture_native_menu_unverified",
        "trajectory_capture_unverified": "trajectory_capture_unverified",
        "grind_evidence_incomplete": "grind_evidence_incomplete",
        "chain_evidence_incomplete": "chain_evidence_incomplete",
        "condition_evidence_lost": "condition_evidence_lost",
        "condition_owner_changed": "condition_owner_changed",
        "condition_heartbeat_stale": "condition_heartbeat_stale",
        "score_capture_baseline_timeout": "score_capture_baseline_timeout",
        "score_capture_evidence_lost": "score_capture_evidence_lost",
    }
    return categories.get(str(error), "integrity_or_transport_failure")


def stopped_trial(error, case, repetition):
    stopped = {'hard_stop': True, 'reason': stop_reason(error),
               'name': case['name'], 'repetition': repetition}
    if stopped['reason'] in ('fixture_native_menu_open', 'fixture_native_menu_unverified'):
        # A previous reset iteration may already have issued one mount chord.
        # This exception alone does not establish that no input was sent.
        stopped.update(game_response='UNVERIFIED', stage='fixture_baseline')
    if stopped['reason'] == 'trajectory_capture_unverified':
        stopped.update(input_sent=False, game_response='UNVERIFIED', stage='trajectory_baseline')
    if stopped['reason'] in ('score_capture_baseline_timeout', 'score_capture_evidence_lost'):
        stopped.update(input_sent=False, game_response='UNVERIFIED', stage='score_capture_baseline')
        receipt = getattr(error, 'score_capture_failure', None)
        if (isinstance(receipt, dict) and type(receipt.get('schema')) is int and receipt['schema'] == 1 and
                set(receipt) == {'schema', 'cause', 'owner', 'native', 'grind_health',
                                 'heartbeat', 'health', 'timing', 'initial', 'pad', 'parser'}):
            clean = score_capture_failure_receipt(receipt['cause'], **{k: receipt[k] for k in
                ('owner', 'native', 'grind_health', 'heartbeat', 'health', 'timing', 'initial', 'pad', 'parser')})
            if clean is not None and clean == receipt:
                stopped['score_capture_failure'] = clean
    diagnostic = getattr(error, 'readiness', None)
    flags = ('failed', 'context_ready', 'owner_matches', 'capture_pending')
    if (isinstance(diagnostic, dict) and set(diagnostic) == {'age_ms', *flags} and
            type(diagnostic['age_ms']) is int and -100000 <= diagnostic['age_ms'] <= 100000 and
            all(type(diagnostic[key]) is bool for key in flags)):
        stopped['readiness'] = diagnostic
    partial = getattr(error, 'partial_trial', None)
    if (isinstance(partial, dict) and partial.get('outcome') == 'INTERRUPTED' and
            bounded_partial_run(error) is not None and partial.get('reset_status') == 'NOT_ATTEMPTED' and
            partial.get('criteria') and all(v == 'UNVERIFIED' for v in partial['criteria'].values())):
        stopped['partial_trial'] = partial
    return stopped


def perform_batch(plan, env):
    """Reuse one environment; ordinary recognition failures never abort a batch."""
    validate_plan(plan)
    results = []
    for case in plan["candidates"]:
        for repetition in range(case.get("repeat", 1)):
            try:
                result = perform_case(plan, case, env)
                result["repetition"] = repetition + 1
                results.append(result)
                if case.get('fixture_reset') and not result.get('fixture_reset', {}).get('reset_ready'):
                    raise HardStop('fixture_reset_unverified')
                if case.get('fixture_reset') and ((not result.get('input_sent') and
                        not result.get('delivery_timing', {}).get('condition_failed')) or
                        result.get('observations', {}).get('grind_result', {}).get('evidence') != 'COMPLETE'):
                    raise HardStop('grind_evidence_incomplete')
                observation = result.get('observations', {})
                # Ordinary missed actions continue. Lost required native evidence
                # must stop after safe neutralization/reset, even if rail rows survived.
                if (observation.get('evidence_gap') or
                        observation.get('combo', {}).get('dropped') or
                        (('trick' in case['expected'] or 'chain' in case['expected']) and
                         observation.get('flip', {}).get('dropped'))):
                    raise HardStop('chain_evidence_incomplete')
            except Exception as error:
                results.append(stopped_trial(error, case, repetition + 1))
                break
        if results and results[-1].get("hard_stop"):
            break
    return {"trials": results}


def ready_baseline(observation, watermark, max_speed_mm_s=None):
    rows = [r for r in observation["states"] if r["seq"] > watermark]
    if len(rows) < 3 or not all(r["valid"] and r["state"] == 100 and r["mode"] == 1 for r in rows[-3:]):
        return None
    if max_speed_mm_s is not None:
        # The two observers share a heartbeat sequence. Old speed samples or
        # an idle pose while coasting cannot establish a stationary baseline.
        speeds = {r["seq"]: r for r in observation.get("push", [])}
        for row in rows[-3:]:
            speed = speeds.get(row["seq"])
            if (not speed or not speed["valid"] or speed["contacts"] != 0xc000 or
                    not 0 <= speed["speed"] * 1000 <= max_speed_mm_s):
                return None
    return rows[-1]["seq"]


def phase_condition_owner(observation):
    """Detached native owner generation; never infer it from the requested action."""
    health = observation.get('grind', {}).get('status', [])[-1:]
    rows = observation.get('grind', {}).get('samples', [])[-1:]
    if (observation.get('evidence_gap') or not health or not rows or not health[0]['active'] or
            not rows[0]['valid'] or not rows[0]['arcade'] or
            rows[0]['generation'] != health[0]['generation'] or rows[0]['dropped'] != health[0]['dropped']):
        raise HardStop('condition_evidence_lost')
    return {key: health[0][key] for key in ('generation', 'dropped', 'rejected')}


def check_phase_condition_integrity(observation, owner):
    grind = observation.get('grind', {})
    if observation.get('evidence_gap') or grind.get('dropped') or observation.get('combo', {}).get('dropped'):
        raise HardStop('condition_evidence_lost')
    for row in grind.get('samples', []):
        if row['generation'] != owner['generation']:
            raise HardStop('condition_owner_changed')
        if not row['valid'] or not row['arcade'] or row['edge'] == 3 or row['dropped'] != owner['dropped']:
            raise HardStop('condition_evidence_lost')
    for row in grind.get('status', []):
        if row['generation'] != owner['generation']:
            raise HardStop('condition_owner_changed')
        if not row['active'] or any(row[key] != owner[key] for key in ('dropped', 'rejected')):
            raise HardStop('condition_evidence_lost')


def wait_native_riding(condition, observe, owner, alive, clock=None, sleep=None):
    """One fixed predicate: fresh native100/mode1, two wheels, unchanged owner.

    Native and host clocks have different epochs. Freshness requires a heartbeat
    newer than the wait-entry watermark, correlated native timestamps, advancing
    native time and bounded host arrival age. No cached pre-wait row can satisfy it.
    """
    validate_phase_wait(condition)
    clock, sleep = clock or time.monotonic, sleep or time.sleep
    started = clock()
    if not alive():
        raise HardStop('condition_evidence_lost')
    initial = observe()
    current_owner = phase_condition_owner(initial)
    if current_owner['generation'] != owner['generation']:
        raise HardStop('condition_owner_changed')
    if current_owner != owner or type(initial.get('log_end')) is not int:
        raise HardStop('condition_evidence_lost')
    states, speeds = initial.get('states', []), initial.get('push', [])
    if not states or not speeds:
        raise HardStop('condition_evidence_lost')
    watermark = states[-1]['seq']
    last_seq, native_ms = watermark, speeds[-1]['ms']
    boundary = initial['log_end']
    heartbeat_at = grind_at = started
    last_grind_seq = initial['grind']['samples'][-1]['seq']
    last = {'heartbeat_seq': watermark, 'native_ms': native_ms, 'generation': owner['generation'],
            'state': states[-1]['state'], 'mode': states[-1]['mode'], 'valid': states[-1]['valid']}
    cause = 'waiting_fresh_heartbeat'
    previous_clock = started
    deadline = started + condition['timeout_ms'] / 1000
    while True:
        now = clock()
        if now < previous_clock:
            raise HardStop('condition_evidence_lost')
        previous_clock = now
        if not alive():
            raise HardStop('condition_evidence_lost')
        if now >= deadline:
            return {'status': 'TIMEOUT', 'cause': cause, 'last_native': last,
                    'scope': 'native_context_only_not_action_result'}
        observation = observe(since=boundary)
        read_at = clock()
        if read_at < now:
            raise HardStop('condition_evidence_lost')
        check_phase_condition_integrity(observation, owner)
        rows = observation.get('states', [])
        speed_rows = {r['seq']: r for r in observation.get('push', [])}
        grind = observation.get('grind', {}).get('samples', [])
        if grind and grind[-1]['seq'] < last_grind_seq:
            raise HardStop('condition_evidence_lost')
        if grind and grind[-1]['seq'] > last_grind_seq:
            last_grind_seq, grind_at = grind[-1]['seq'], read_at
        row = rows[-1] if rows else None
        speed = speed_rows.get(row['seq']) if row else None
        if row and row['seq'] < last_seq:
            raise HardStop('condition_evidence_lost')
        if row and speed and row['seq'] > last_seq:
            if speed['ms'] <= native_ms:
                raise HardStop('condition_evidence_lost')
            last_seq, native_ms, heartbeat_at = row['seq'], speed['ms'], read_at
            last = {'heartbeat_seq': last_seq, 'native_ms': native_ms, 'generation': owner['generation'],
                    'state': row['state'], 'mode': row['mode'], 'valid': row['valid'],
                    'speed_valid': speed['valid'], 'speed_mm_s': round(speed['speed'] * 1000),
                    'contacts': speed['contacts']}
        if read_at - heartbeat_at > .750 or read_at - grind_at > .750:
            raise HardStop('condition_heartbeat_stale')
        cause = 'waiting_fresh_heartbeat'
        if row and speed and row['seq'] > watermark:
            cause = 'native_not_riding'
            coherent = (grind and grind[-1]['state'] == 100 and
                        abs(grind[-1]['ms'] - speed['ms']) <= 150 and read_at - heartbeat_at <= .150 and
                        read_at - grind_at <= .150 and read_at - now <= .100)
            # Changed physics may already contradict the latest250ms heartbeat.
            latest = observation.get('combo', {}).get('samples', [])[-1:]
            if latest and latest[0]['ms'] > speed['ms'] and (latest[0]['state'] != 100 or latest[0]['mode'] != 1):
                coherent = False
            if row['valid'] and row['state'] == 100 and row['mode'] == 1 and speed['valid'] and speed['contacts'] == 0xc000:
                cause = 'waiting_current_owner_state' if not coherent else 'speed_above_bound'
                if coherent and ('max_speed_mm_s' not in condition or speed['speed'] * 1000 <= condition['max_speed_mm_s']):
                    if not alive():
                        raise HardStop('condition_evidence_lost')
                    if clock() < deadline:
                        return {'status': 'READY', 'cause': 'fresh_native_riding', 'last_native': last,
                                'scope': 'native_context_only_not_action_result'}
        sleep(min(.020, max(0, deadline - clock())))


def score_capture_failure_receipt(cause, **context):
    """Fixed numeric diagnostic schema; never exception text or arbitrary data."""
    causes = ('initial_log_boundary', 'initial_grind_evidence', 'malformed_fields', 'clock_reversed',
              'session_unavailable', 'timeout', 'observation_gap', 'log_boundary_invalid', 'combo_loss',
              'grind_parser_loss', 'grind_status_inactive', 'grind_status_generation', 'grind_status_counters',
              'grind_sample_invalid', 'grind_sample_generation', 'grind_sample_loss', 'arcade_context_changed',
              'grind_sample_reordered', 'grind_pad_generation', 'grind_pad_loss', 'heartbeat_reordered',
              'health_parser_invalid', 'health_parser_loss', 'health_rows_invalid', 'health_row_malformed',
              'health_overflow', 'health_time_reordered', 'health_epoch_mixed', 'health_route_duplicate',
              'heartbeat_stale', 'grind_stale', 'session_unavailable_final', 'clock_reversed_final')
    if cause not in causes:
        return None
    fields = {'owner': ('generation', 'dropped', 'rejected'),
              'native': ('seq', 'ms', 'generation', 'state', 'edge', 'dropped', 'valid', 'arcade', 'speed_mm_s'),
              'grind_health': ('generation', 'active', 'dropped', 'rejected'),
              'pad': ('seq', 'ms', 'generation', 'dropped'),
              'parser': ('health_dropped', 'health_malformed', 'grind_dropped', 'combo_dropped'),
              'heartbeat': ('seq', 'valid', 'state', 'mode'),
              'health': ('ms', 'epoch', 'route', 'active', 'bound', 'unbound', 'expired', 'changed', 'overflow'),
              'timing': ('elapsed_ms', 'read_ms', 'heartbeat_age_ms', 'grind_age_ms', 'snapshot_age_ms',
                         'snapshot_ms', 'snapshot_epoch', 'routes_mask'),
              'initial': ('sample_count', 'status_count', 'generation_count', 'first_generation', 'last_generation')}
    booleans = ('active', 'bound', 'valid', 'arcade', 'overflow')
    result = {'schema': 1, 'cause': cause}
    for group, names in fields.items():
        source = context.get(group)
        source = source if isinstance(source, dict) else {}
        result[group] = {}
        for key in names:
            value = source.get(key)
            valid = type(value) is bool if key in booleans else type(value) is int and 0 <= value <= 0xffffffffffffffff
            result[group][key] = value if valid else None
    return result


def wait_score_capture_ready(milliseconds, initial, observe, alive, max_speed_mm_s=None, clock=None, sleep=None):
    """Neutral pre-input observer binding gate; never scoring ownership proof.

    Four routes must share one newly delivered native snapshot timestamp. Skip
    counters accumulated before this gate opens become the baseline, not a loss
    attributed to a trial which has not started.
    """
    integer(milliseconds, 2000, 10000)
    if max_speed_mm_s is not None:
        integer(max_speed_mm_s, 0, 20000)
    clock, sleep = clock or time.monotonic, sleep or time.sleep
    owner = last_grind = current_native = current_grind_health = state = current_state = current_health = current_pad = None
    started = host_last = read_started = heartbeat_at = grind_at = arrival = None
    snapshot_ms = snapshot_epoch = None
    routes, initial_summary, parser_state = {}, {}, {}
    def fail(cause):
        def age(then):
            try:
                return round((host_last - then) * 1000) if host_last is not None and then is not None else None
            except (TypeError, ValueError, OverflowError):
                return None
        error = HardStop('score_capture_baseline_timeout' if cause == 'timeout' else 'score_capture_evidence_lost')
        error.score_capture_failure = score_capture_failure_receipt(cause,
            owner=owner, native=current_native, grind_health=current_grind_health, heartbeat=current_state, health=current_health,
            pad=current_pad, parser=parser_state, initial=initial_summary, timing={'elapsed_ms': age(started), 'read_ms': age(read_started),
                'heartbeat_age_ms': age(heartbeat_at), 'grind_age_ms': age(grind_at), 'snapshot_age_ms': age(arrival),
                'snapshot_ms': snapshot_ms, 'snapshot_epoch': snapshot_epoch,
                'routes_mask': sum(1 << (r - 1) for r in routes)})
        raise error
    try:
        if type(initial.get('log_end')) is not int or initial['log_end'] < 0:
            fail('initial_log_boundary')
        owner = initial['grind']['status'][-1]
        baseline_row = initial['grind']['samples'][-1]
        last_grind, current_native, current_grind_health = baseline_row, baseline_row, owner
        initial_rows = initial['grind']['samples']
        initial_summary = {'sample_count': len(initial_rows), 'status_count': len(initial['grind']['status']),
            'generation_count': len({r['generation'] for r in initial_rows}),
            'first_generation': initial_rows[0]['generation'], 'last_generation': baseline_row['generation']}
        if grind_result(initial, initial, True)['evidence'] != 'COMPLETE':
            fail('initial_grind_evidence')
        arcade = baseline_row['arcade']
        native_start = baseline_row['ms']
        boundary = initial['log_end']
        state = initial['states'][-1]
        current_state = state
        speed = {r['seq']: r for r in initial.get('push', [])}.get(state['seq'])
        last_grind = baseline_row
        last_pad = initial['grind']['pad'][-1]
        current_pad = last_pad
        last_combo = initial.get('combo', {}).get('samples', [])[-1:]
        started = prior_clock = grind_at = heartbeat_at = clock()
        host_last = started
        deadline = started + milliseconds / 1000
        routes, snapshot_ms, snapshot_epoch, arrival = {}, None, None, None
        while True:
            now = clock()
            host_last = now
            if now < prior_clock:
                fail('clock_reversed')
            if not alive():
                fail('session_unavailable')
            prior_clock = now
            if now >= deadline:
                fail('timeout')
            read_started = now
            current = observe(since=boundary)
            read_at = clock()
            host_last = read_at
            parser_state = {'health_dropped': current.get('native_score', {}).get('health_dropped'),
                            'health_malformed': current.get('native_score', {}).get('health_malformed'),
                            'grind_dropped': current.get('grind', {}).get('dropped'),
                            'combo_dropped': current.get('combo', {}).get('dropped')}
            if read_at < now: fail('clock_reversed')
            if current.get('evidence_gap'): fail('observation_gap')
            if type(current.get('log_end')) is not int or current['log_end'] < boundary: fail('log_boundary_invalid')
            if current.get('combo', {}).get('dropped'): fail('combo_loss')
            prior_clock = read_at
            old_boundary, boundary = boundary, current['log_end']
            grind = current.get('grind', {})
            if grind.get('dropped'):
                fail('grind_parser_loss')
            for row in grind.get('status', []):
                current_grind_health = row
                # Match grind_result: worker health can arrive after newer
                # physics. Only ignore older health whose loss is covered by
                # the validated owner; it never refreshes native evidence.
                if (row['generation'] < owner['generation'] and
                    row['dropped'] <= owner['dropped'] and row['rejected'] <= owner['rejected']):
                    continue
                if not row['active']: fail('grind_status_inactive')
                if row['generation'] != owner['generation']: fail('grind_status_generation')
                if any(row[k] != owner[k] for k in ('dropped', 'rejected')): fail('grind_status_counters')
            for row in grind.get('samples', []):
                current_native = row
                if not row['valid'] or row['edge'] == 3: fail('grind_sample_invalid')
                if row['generation'] != owner['generation']: fail('grind_sample_generation')
                if row['dropped'] != owner['dropped']: fail('grind_sample_loss')
                if row['arcade'] != arcade: fail('arcade_context_changed')
                if row['seq'] < last_grind['seq'] or row['ms'] < last_grind['ms']: fail('grind_sample_reordered')
                if row['seq'] > last_grind['seq']:
                    last_grind, grind_at = row, read_at
            for row in grind.get('pad', []):
                current_pad = row
                if row['generation'] != owner['generation']: fail('grind_pad_generation')
                if row['dropped'] != owner['dropped']: fail('grind_pad_loss')
                last_pad = row
            if current.get('combo', {}).get('samples'):
                last_combo = current['combo']['samples'][-1:]
            speeds = {r['seq']: r for r in current.get('push', [])}
            for row in current.get('states', []):
                current_state = row
                if row['seq'] < state['seq']:
                    fail('heartbeat_reordered')
                if row['seq'] > state['seq']:
                    state, speed, heartbeat_at = row, speeds.get(row['seq']), read_at
            native = current.get('native_score', {})
            if any(type(native.get(k)) is not int or native[k] < 0 for k in ('health_dropped', 'health_malformed')):
                fail('health_parser_invalid')
            if any(native[k] != 0 for k in ('health_dropped', 'health_malformed')):
                fail('health_parser_loss')
            health = native.get('health', [])
            if not isinstance(health, list) or len(health) > 128:
                fail('health_rows_invalid')
            for row in health:
                current_health = row
                if (not isinstance(row, dict) or any(type(row.get(k)) is not int or not 0 <= row[k] <= 0xffffffffffffffff
                        for k in ('ms', 'epoch', 'route', 'unbound', 'expired', 'changed')) or
                        not row['epoch'] or row['route'] not in (1, 2, 3, 4) or
                        any(type(row.get(k)) is not bool for k in ('active', 'bound', 'overflow')) or
                        (row['bound'] and not row['active'])):
                    fail('health_row_malformed')
                if row['overflow']: fail('health_overflow')
                if row['ms'] <= native_start or boundary == old_boundary:
                    continue
                if snapshot_ms is not None and row['ms'] < snapshot_ms:
                    fail('health_time_reordered')
                if row['ms'] != snapshot_ms:
                    routes, snapshot_ms, snapshot_epoch, arrival = {}, row['ms'], row['epoch'], read_at
                if row['epoch'] != snapshot_epoch: fail('health_epoch_mixed')
                if row['route'] in routes: fail('health_route_duplicate')
                routes[row['route']] = row
            if read_at - heartbeat_at > .750: fail('heartbeat_stale')
            if read_at - grind_at > .750: fail('grind_stale')
            ready = (len(routes) == 4 and all(r['active'] and r['bound'] for r in routes.values()) and
                     state['valid'] and state['state'] == 100 and state['mode'] == 1 and
                     speed and speed['valid'] and speed['contacts'] == 0xc000 and last_grind['state'] == 100 and
                     abs(last_grind['ms'] - snapshot_ms) <= 250 and abs(speed['ms'] - snapshot_ms) <= 350 and
                     (max_speed_mm_s is None or 0 <= speed['speed'] * 1000 <= max_speed_mm_s) and
                     read_at - arrival <= .250 and read_at - now <= .100)
            if ready:
                if not alive():
                    fail('session_unavailable_final')
                finish = clock()
                host_last = finish
                if finish < read_at:
                    fail('clock_reversed_final')
                if finish < deadline and finish - arrival <= .250:
                    # Health can arrive in a separate slice from physics. Retain
                    # actual validated rows and their original native timestamps.
                    current['grind'] = dict(grind, status=[owner], samples=[last_grind], pad=[last_pad])
                    current['states'] = [state]
                    current['push'] = [speed]
                    current['combo'] = dict(current.get('combo', {}), samples=last_combo)
                    return current, {'status': 'READY', 'scope': 'observer_binding_only',
                        'owner_verified': False, 'capture_completeness': 'UNVERIFIED',
                        'generation': owner['generation'], 'epoch': snapshot_epoch, 'ms': snapshot_ms,
                        'health': [routes[r] for r in range(1, 5)], 'wait_ms': round((finish - started) * 1000)}
            sleep(min(.050, max(0, deadline - clock())))
    except (KeyError, TypeError, IndexError, ValueError):
        fail('malformed_fields')


def require_fixture_menu_closed(status):
    # Old runtimes did not export this observation. Their legacy path is not
    # promoted to menu verification; new unknown/invalid evidence stops input.
    if 'native_menu' not in status:
        return
    if status['native_menu'] == 'open':
        raise HardStop('fixture_native_menu_open')
    if status['native_menu'] != 'closed':
        raise HardStop('fixture_native_menu_unverified')


class DeckEnvironment:
    """Uses only trusted robot/probe functions supplied by the remote payload."""
    def __init__(self, session=None):
        self.session = session

    def integrity(self, plan):
        current = snapshot({})
        required = (current.get("safe_paths"), all(current.get("files", {}).values()),
                    current.get("active"), current.get("updates") is False,
                    not current.get("recovery_pending"), current.get("processes", {}).get("check_complete"),
                    current.get("installed_commit") == plan["source_commit"],
                    current.get("dll_sha256") == plan["dll_sha256"],
                    current.get("backup_sha256") == plan["stock_sha256"])
        if not all(required):
            raise HardStop("installation_integrity")
        receipt = read_json(Path(current["game"]) / RECEIPT)
        if receipt.get("stock_sha256") != plan["stock_sha256"]:
            raise HardStop("stock_backup_integrity")
        self.log = Path(receipt.get("log_path", ""))
        if self.log.name != "ReSkate.log" or not self.log.is_absolute() or any(p.is_symlink() for p in (self.log, *self.log.parents)):
            raise HardStop("log_path_integrity")
        # Recovery is bounded waiting for an already-started restart. No guessed
        # launcher click, root operation, update or stock restore is performed.
        for attempt in range(3):
            ready = capabilities()
            if ready.get("ready"):
                return
            time.sleep(1)
        raise HardStop("game_or_hil_unavailable_after_recovery_wait")

    def observe(self, since=None):
        return self._read_observation(since, observations)

    def observe_condition(self, since=None):
        return self._read_observation(since, condition_observations)

    def observe_trajectory(self, since):
        return self._read_observation(since, lambda text: {'fixture_trajectory': parse_fixture_trajectory(text)})

    def _read_observation(self, since, parser):
        try:
            with self.log.open("rb") as stream:
                size = self.log.stat().st_size
                start = max(0, size - 262144)
                if since is not None:
                    if since > size:
                        return {"states": [], "buttons": [], "log_end": size,
                                "evidence_gap": "log_truncated"}
                    start = max(start, since)
                stream.seek(start)
                data = stream.read(262144)
                result = parser(data.decode(errors="replace"))
                result["log_end"] = start + len(data)
                result["evidence_gap"] = "tail_limit" if since is not None and start > since else None
                return result
        except OSError:
            return {"states": [], "buttons": []}

    def wait_idle(self, milliseconds, max_speed_mm_s=None):
        before = self.observe()["states"]
        watermark = before[-1]["seq"] if before else 0
        deadline = time.monotonic() + milliseconds / 1000
        unavailable = 0
        while time.monotonic() < deadline:
            if not capabilities().get("ready"):
                unavailable += 1
                if unavailable >= 3:
                    raise HardStop("game_or_hil_unavailable_after_recovery_wait")
                time.sleep(1)
                continue
            unavailable = 0
            ready = ready_baseline(self.observe(), watermark, max_speed_mm_s)
            if ready is not None:
                return ready
            time.sleep(0.25)
        return None

    def fixture_baseline(self, milliseconds):
        if self.session is None or self.session.pad is None:
            raise HardStop('fixture_reset_unverified')
        self.session.pad.neutral()
        boundary = self.observe().get('log_end')
        if boundary is None:
            raise HardStop('fixture_reset_unverified')
        self.session._clock()
        fixture_console('status')
        observation = self.observe(since=boundary)
        status = observation.get('fixture')
        if not status or observation.get('evidence_gap'):
            raise HardStop('fixture_reset_unverified')
        require_fixture_menu_closed(status)
        if status['failed'] or status['editor_open'] or not status['present']:
            raise HardStop('fixture_reset_unverified')
        if status['reset_ready']:
            # The preceding trial already reset. Revalidate instead of teleporting
            # and remounting again; fresh idle and observer checks still follow.
            return status
        return self.fixture_reset(milliseconds)

    def arm_fixture_trajectory(self, baseline, fixture):
        # Final console handoff only. No reset, remount or second arm is attempted.
        if self.session is None or self.session.pad is None or not fixture or not fixture.get('reset_ready'):
            raise HardStop('trajectory_capture_unverified')
        self.session.pad.neutral()
        boundary = self.observe().get('log_end')
        if type(boundary) is not int:
            raise HardStop('trajectory_capture_unverified')
        self.session._clock()
        fixture_console('trajectory')
        handoff = time.monotonic()
        if self.wait_idle(3000, max_speed_mm_s=100) is None:
            raise HardStop('trajectory_capture_unverified')
        remaining = int((handoff + 3 - time.monotonic()) * 1000)
        if remaining <= 0:
            raise HardStop('trajectory_capture_unverified')
        try:
            ready = self.wait_grind_ready(remaining)
        except HardStop as error:
            raise HardStop('trajectory_capture_unverified') from error
        self.session._clock()
        current = self.observe(since=boundary)
        diagnostic = current.get('fixture_trajectory', {})
        status, rows = diagnostic.get('status'), diagnostic.get('samples', [])
        if (time.monotonic() - handoff > 3 or not status or status.get('status') != 'armed' or status.get('dropped') or
                diagnostic.get('evidence_gap') or not rows or len(rows) > 80 or any(not r.get('valid') for r in rows) or
                any(r.get('session') != status['session'] or r.get('generation') != fixture['generation'] or
                    r.get('revision') != fixture['revision'] or r.get('state') != 100 or r.get('speed_mm_s', 101) > 100 for r in rows)):
            raise HardStop('trajectory_capture_unverified')
        # Reuse the existing post-console native baseline, with its actual
        # timestamps. The sampler independently pins full owner/fixture identity
        # across this handoff; observer readiness alone does not establish it.
        native_rows = ready.get('grind', {}).get('samples', [])
        old_health = baseline.get('grind', {}).get('status', [])[-1:]
        new_health = ready.get('grind', {}).get('status', [])[-1:]
        if (not native_rows or abs(native_rows[-1]['ms'] - rows[-1]['sampled']) > 150 or
                not old_health or not new_health or not old_health[0].get('active') or
                new_health[0]['generation'] < old_health[0]['generation'] or
                any(new_health[0][k] != old_health[0][k] for k in ('dropped', 'rejected')) or
                grind_result(ready, ready, True)['evidence'] != 'COMPLETE'):
            raise HardStop('trajectory_capture_unverified')
        ready = dict(ready, log_end=current.get('log_end'))
        if current.get('combo', {}).get('samples'):
            ready['combo'] = current['combo']
        return ready, {'session': status['session'], 'sampled': rows[-1]['sampled'],
                         'generation': rows[-1]['generation'], 'revision': rows[-1]['revision'],
                         'log_start': boundary, 'prefix': rows,
                         'scope': 'relative_motion_diagnostic_only'}

    def fixture_reset(self, milliseconds):
        if self.session is None or self.session.pad is None:
            raise HardStop('fixture_reset_unverified')
        self.session.pad.neutral()
        boundary = self.observe().get('log_end')
        if boundary is None:
            raise HardStop('fixture_reset_unverified')
        fixture_console('reset')
        deadline = time.monotonic() + milliseconds / 1000
        mounted = False
        while time.monotonic() < deadline:
            self.session._clock()
            fixture_console('status')
            observation = self.observe(since=boundary)
            status = observation.get('fixture')
            if status and not observation.get('evidence_gap'):
                require_fixture_menu_closed(status)
            if status and not observation.get('evidence_gap') and status['reset_ready']:
                return status
            if status and (status['failed'] or status['editor_open'] or not status['present']):
                break
            if status and not observation.get('evidence_gap') and not mounted and status.get('state') == 504:
                # Native teleport returns on foot. Mount once, then require a new
                # measured riding status; a button request is never readiness.
                self.fixture_mount(status)
                mounted = True
            boundary = observation.get('log_end', boundary)
            time.sleep(.1)
        raise HardStop('fixture_reset_unverified')

    def fixture_mount(self, status):
        require_fixture_menu_closed(status)
        if (type(status.get('arcade')) is not bool or not status.get('configured') or
                not status.get('present') or status.get('busy') or status.get('failed') or
                status.get('editor_open') or not status.get('skater') or status.get('state') != 504 or
                status.get('speed', 1) > .1 or abs((status.get('heading', 180) + 180) % 360 - 180) > 3 or
                sum((a-b)**2 for a, b in zip(status.get('position', []), status.get('start', []))) > .25 or
                len(status.get('position', [])) != 3 or len(status.get('start', [])) != 3):
            raise HardStop('fixture_reset_unverified')
        context = capabilities()
        if not context.get('ready') or context.get('gamescope_pids') != [self.session.pid]:
            raise HardStop('fixture_reset_unverified')
        self.session._clock()
        buttons = (0x136, 0x137) if status['arcade'] else (0x134,)
        try:
            self.session.pad.frame([(1, button, 1) for button in buttons])
            time.sleep(.25)  # Bounded native press; includes the 120ms board chord window.
            self.session._clock()
        finally:
            self.session.pad.neutral()

    def wait_grind_ready(self, milliseconds):
        before = self.observe()
        boundary = before.get('log_end')
        health = before.get('grind', {}).get('status', [])[-1:]
        if boundary is None:
            raise HardStop('grind_evidence_incomplete')
        deadline = time.monotonic() + milliseconds / 1000
        while time.monotonic() < deadline:
            self.session._clock()
            observation = self.observe(since=boundary)
            grind = observation.get('grind', {})
            rows = grind.get('samples', [])
            # Identical health lines may be coalesced by the existing logger.
            # Fresh physics/pad rows must still match the same owner generation
            # and loss counters; the trial's later health must also match them.
            status = grind.get('status', []) or health
            grind['status'] = status
            if len(rows) >= 3 and status and status[-1]['active']:
                current = dict(observation, grind=dict(grind, samples=rows[-3:], status=status[-1:]))
                if (all(r['valid'] and r['state'] == 100 for r in rows[-3:]) and
                        grind_result(current, current, True)['evidence'] == 'COMPLETE'):
                    return observation
            time.sleep(.1)
        raise HardStop('grind_evidence_incomplete')

    def wait_score_capture_ready(self, milliseconds, baseline, max_speed_mm_s=None):
        if self.session is None or self.session.pad is None:
            error = HardStop('score_capture_evidence_lost')
            error.score_capture_failure = score_capture_failure_receipt('session_unavailable')
            raise error
        self.session.pad.neutral()
        def alive():
            self.session._clock()
            context = capabilities()
            return context.get('ready') is True and context.get('gamescope_pids') == [self.session.pid]
        return wait_score_capture_ready(milliseconds, baseline, self.observe, alive, max_speed_mm_s)

    def gesture(self, case):
        run = self.session.run if self.session is not None else experiment_events
        conditions = {index: phase['wait'] for index, phase in enumerate(case['phases']) if 'wait' in phase}
        if conditions:
            owner = phase_condition_owner(self.observe_condition())
            return run(case['name'], phase_events(case), case['timeout_ms'] / 1000,
                capture_frames=case.get('capture_frames', 4), phase_conditions=conditions,
                condition_waiter=lambda spec, alive: wait_native_riding(spec, self.observe_condition, owner, alive))
        return run(case["name"], phase_events(case), case["timeout_ms"] / 1000,
                   capture_frames=case.get("capture_frames", 4))


# LOCAL TRANSPORT: excluded from the remote payload.
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
import controller
import robot
from probe import parse_combo_samples, parse_score_samples, parse_push_samples, parse_pad_samples, parse_flip_observations, parse_stick_samples, parse_playback_samples, parse_action_samples, parse_score_edges, parse_score_presence, parse_score_numbers, parse_native_score_events, parse_grind_observations, parse_fixture_status, parse_fixture_trajectory
from robot import fixture_console

# Freeze trusted code for the lifetime of this process. Editing the worktree
# during analysis must never change the remote implementation mid-batch.
REMOTE_SOURCES = (robot.remote_source("preflight", "idle").rsplit("\nprint(json.dumps(", 1)[0],
                  (HERE / "probe.py").read_text(),
                  Path(__file__).read_text().split("# LOCAL TRANSPORT:")[0])


def remote_payload(plan, case):
    validate_plan(plan)
    if case not in plan["candidates"]:
        raise ValueError("Candidate not in validated plan")
    # Data travels on stdin as JSON. Neither labels nor phases are shell/code.
    return "\n".join(REMOTE_SOURCES) + "\nrequest=json.load(sys.stdin)\nvalidate_plan(request['plan'])\n" + \
        "case=request['plan']['candidates'][request['index']]\n" + \
        "try:\n result=perform_case(request['plan'],case,DeckEnvironment())\n" + \
        "except Exception as exc:\n result=stopped_trial(exc,case,1)\n" + \
        "print(json.dumps(result))\n"


def transport(plan, case):
    command = "python3 -B -c " + shlex.quote(remote_payload(plan, case))
    response = subprocess.run(controller.ssh_arguments("deck@steamdeck.local", controller.HOME) + [command],
        input=json.dumps({"plan": plan, "index": plan["candidates"].index(case)}),
        capture_output=True, text=True, check=True, timeout=55)
    if len(response.stdout) > 48 * 1024 * 1024:
        raise HardStop("response_bound")
    return json.loads(response.stdout)


def preflight(plan, lab=None, sleep=time.sleep):
    lab = lab or controller.Lab()
    for attempt in range(3):
        try:
            status = lab.remote("status")
            break
        except (OSError, subprocess.SubprocessError):
            if attempt == 2:
                raise HardStop("ssh_unavailable_after_3_attempts") from None
            sleep(1)
    if (not status.get("safe_paths") or not status.get("active") or status.get("updates") is not False
            or status.get("recovery_pending") or status.get("installed_commit") != plan["source_commit"]
            or status.get("dll_sha256") != plan["dll_sha256"] or status.get("backup_sha256") != plan["stock_sha256"]):
        raise HardStop("installation_integrity")


def run_batch(plan, output, dispatch=transport, check=preflight):
    validate_plan(plan)
    from deck_installer import safe_path
    safe_path(output)
    output.mkdir(parents=True, exist_ok=False, mode=0o700)
    summary = {"schema": 1, "source_commit": plan["source_commit"], "dll_sha256": plan["dll_sha256"],
               "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "trials": [], "stop": None}
    (output / "plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    def save():
        (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    save()
    try:
        for case in plan["candidates"]:
            for repetition in range(case.get("repeat", 1)):
                check(plan)
                result = dispatch(plan, case)  # Never retry uncertain gameplay input.
                if result.get("hard_stop"):
                    summary['trials'].append(result)
                    save()
                    raise HardStop(result.get("reason", "remote_safety_failure"))
                missed = result.get("outcome") == "MISSED_CONDITION"
                timing = result.get("delivery_timing", {})
                if missed and not result.get("input_sent"):
                    conditions = timing.get('conditions', []) if isinstance(timing, dict) else []
                    if (result.get('input_sent') is not False or not isinstance(timing, dict) or
                            timing.get('condition_failed') is not True or timing.get('samples') != [] or
                            not isinstance(conditions, list) or len(conditions) != 1 or
                            not isinstance(conditions[0], dict) or conditions[0].get('status') != 'TIMEOUT' or
                            conditions[0].get('phase') != 0 or 'wait' not in case['phases'][0]):
                        raise HardStop('invalid_response')
                if result.get("input_sent") or missed:
                    if result.get("neutralized") is not True:
                        raise HardStop("neutralization_unconfirmed")
                    trial_output = output / ("trial-%03d" % len(summary["trials"]))
                    if case.get('capture_frames', 4) == 4:
                        robot.save_run(result, trial_output)
                    else:
                        robot.save_run(result, trial_output, expected_frames=case['capture_frames'])
                elif result.get("outcome") != "SKIPPED_CONTEXT":
                    raise HardStop("invalid_response")
                summary["trials"].append({"name": case["name"], "repeat": repetition + 1,
                    "outcome": result["outcome"], "input_sent": result["input_sent"],
                    "criteria": result.get("criteria", {}), "observations": result.get("observations", {}),
                    "frames": [{k: f[k] for k in ("sha256", "format", "elapsed_ms")} for f in result.get("frames", [])]})
                if isinstance(timing, dict) and 'conditions' in timing:
                    summary['trials'][-1].update(delivery_timing=timing, neutralized=result.get('neutralized'))
                save()
        summary["stop"] = "candidates_exhausted"
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError, KeyboardInterrupt) as exc:
        # Remote alarm and fd close bound input if transport is interrupted.
        # No second controller or automatic replay is started after lost ACK.
        summary["stop"] = str(exc) if isinstance(exc, HardStop) else "transport_or_evidence_integrity_unconfirmed"
    finally:
        save()
    return summary


def scoped(path, root, exists=True):
    from deck_installer import safe_path
    path = Path(path).absolute()
    safe_path(path)
    resolved = path.resolve()
    if root.resolve() not in resolved.parents or (exists and not resolved.is_file()):
        raise ValueError("Path must remain inside the project scope")
    return resolved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("plan", type=Path)
    parser.add_argument("--validate-only", action="store_true")
    args = parser.parse_args()
    path = scoped(args.plan, ROOT)
    if path.stat().st_size > MAX_PLAN:
        raise ValueError("Candidate file too large")
    plan = validate_plan(json.loads(path.read_text()))
    if args.validate_only:
        print("VALID: %d bounded candidates; no Deck access" % len(plan["candidates"]))
        return 0
    output = scoped(ROOT / "artifacts" / ("trick-batch-" + time.strftime("%Y%m%dT%H%M%S") + "-" + str(__import__('os').getpid())), ROOT / "artifacts", False)
    summary = run_batch(plan, output)
    print("Batch: " + summary["stop"] + "; summary: " + str(output / "summary.json"))
    return 0 if summary["stop"] == "candidates_exhausted" else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError) as exc:
        print("Batch rejected: " + str(exc), file=sys.stderr)
        sys.exit(1)
