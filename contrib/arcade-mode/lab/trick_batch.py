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
             ("repeat", "capture_frames", "baseline_max_speed_mm_s", "fixture_reset"))
        if "fixture_reset" in case and type(case["fixture_reset"]) is not bool:
            raise ValueError("Fixture reset must be boolean")
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
        keys(case["expected"], ("states",), ("trick",))
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
        previous = -1
        for phase in phases:
            keys(phase, ("ms",), tuple(AXIS_CODES) + ("buttons",))
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
    if trials > MAX_TRIALS:
        raise ValueError("At most 24 trials per batch")
    return plan


def phase_events(case):
    # Full states, not deltas. Linux Y is down-positive, unlike XInput.
    return [(p["ms"] / 1000, [(3, code, p.get(axis, 0)) for axis, code in AXIS_CODES.items()] +
             [(1, code, int(button in p.get("buttons", []))) for button, code in BUTTON_CODES.items()])
            for p in case["phases"]]


def observations(text):
    rows = [{"seq": int(m[1]), "valid": m[2] == "true", "state": int(m[3]), "mode": int(m[4])}
            for m in HIL_ROW.finditer(text)]
    buttons = [{"button": m[1], "count": int(m[2])} for m in BUTTON_ROW.finditer(text)]
    return {"states": rows[-128:], "buttons": buttons[-32:],
            "combo": parse_combo_samples(text), "score": parse_score_samples(text),
            "push": parse_push_samples(text), "pad": parse_pad_samples(text),
            "flip": parse_flip_observations(text), "stick": parse_stick_samples(text),
            "playback": parse_playback_samples(text), "actions": parse_action_samples(text),
            "score_edges": parse_score_edges(text), "score_numbers": parse_score_numbers(text),
            "native_score": parse_native_score_events(text), "grind": parse_grind_observations(text),
            "fixture": parse_fixture_status(text)}


def subsequence(expected, rows):
    index = 0
    for row in rows:
        if index < len(expected) and row["valid"] and row["state"] == expected[index]:
            index += 1
    return index == len(expected)


def named_flip_result(expected, observation, log_boundary):
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
    seen = {row["id"] for row in native.get("samples", []) if row.get("valid") and row["id"] > 0}
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
    return dict(evidence='INCOMPLETE' if uncertain else 'COMPLETE',
                native_grind='UNVERIFIED' if uncertain else 'PASS' if active else 'FAIL',
                entries=sum(r['edge'] == 1 for r in rows), exits=sum(r['edge'] == 2 for r in rows),
                duration_ms=max((r['duration'] for r in rows), default=0),
                raw_enum_pairs=sorted({(r['grind'] if r['grind_valid'] else None,
                                       r['skate'] if r['skate_valid'] else None) for r in active}, key=str),
                entry_speed_mm_s=[r['speed_mm_s'] if r['speed_valid'] else None for r in rows if r['edge'] == 1],
                named_identity='UNKNOWN', contact_without_grind='UNVERIFIED',
                stale_health_rows=len(stale),
                bail=any(r['valid'] and r['state'] == 300 for r in rows), reset='PASS' if reset_ok else 'FAIL')


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
    before = baseline_observation.get("combo", {}).get("samples", [])
    watermark = before[-1]["ms"] if before else None
    result = env.gesture(case)
    # Gesture's finally neutralizes BEFORE waiting on screenshots or reset.
    env.integrity(plan)
    reset = env.wait_idle(case["reset_ms"], **readiness)
    log_boundary = baseline_observation.get("log_end")
    obs = env.observe(since=log_boundary)
    if 'grind' in obs:
        obs['grind_result'] = grind_result(obs, baseline_observation, reset is not None)
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
    wanted = case["expected"]["states"]
    heartbeat_match = bool(wanted) and subsequence(wanted, obs["states"])
    physics = [{"valid": True, "state": row["state"]} for row in combo["samples"]]
    physics_complete = combo["baseline_available"] and not combo.get("dropped", 0) and not obs.get("evidence_gap")
    physics_match = bool(wanted) and physics_complete and subsequence(wanted, physics)
    # A 250ms heartbeat routinely misses a short pop. Absence in a sampled
    # trace is not proof of failed gameplay; a complete observed sequence is
    # positive evidence. Named tricks still require a separate native signal.
    state_result = "PASS" if heartbeat_match or physics_match else "UNVERIFIED"
    result.update(name=case["name"], outcome="OBSERVED", observations=obs,
                  criteria={"native_state_sequence": state_result,
                            "heartbeat_state_sequence": "PASS" if heartbeat_match else "UNVERIFIED",
                            "physics_state_sequence": "PASS" if physics_match else "UNVERIFIED",
                            "returned_to_idle": "PASS" if reset is not None else "FAIL",
                            "named_trick": named_flip_result(case["expected"].get("trick"), obs, log_boundary)}, neutralized=True)
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
        "grind_evidence_incomplete": "grind_evidence_incomplete",
    }
    return categories.get(str(error), "integrity_or_transport_failure")


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
                if case.get('fixture_reset') and (not result.get('input_sent') or
                        result.get('observations', {}).get('grind_result', {}).get('evidence') != 'COMPLETE'):
                    raise HardStop('grind_evidence_incomplete')
            except Exception as error:
                stopped = {"hard_stop": True, "reason": stop_reason(error),
                           "name": case["name"], "repetition": repetition + 1}
                diagnostic = getattr(error, 'readiness', None)
                flags = ('failed', 'context_ready', 'owner_matches', 'capture_pending')
                if (isinstance(diagnostic, dict) and set(diagnostic) == {'age_ms', *flags} and
                        type(diagnostic['age_ms']) is int and -100000 <= diagnostic['age_ms'] <= 100000 and
                        all(type(diagnostic[key]) is bool for key in flags)):
                    stopped['readiness'] = diagnostic
                results.append(stopped)
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
                result = observations(data.decode(errors="replace"))
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
        if status['failed'] or status['editor_open'] or not status['present']:
            raise HardStop('fixture_reset_unverified')
        if status['reset_ready']:
            # The preceding trial already reset. Revalidate instead of teleporting
            # and remounting again; fresh idle and observer checks still follow.
            return status
        return self.fixture_reset(milliseconds)

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

    def gesture(self, case):
        run = self.session.run if self.session is not None else experiment_events
        return run(case["name"], phase_events(case), case["timeout_ms"] / 1000,
                   capture_frames=case.get("capture_frames", 4))


# LOCAL TRANSPORT: excluded from the remote payload.
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
import controller
import robot
from probe import parse_combo_samples, parse_score_samples, parse_push_samples, parse_pad_samples, parse_flip_observations, parse_stick_samples, parse_playback_samples, parse_action_samples, parse_score_edges, parse_score_numbers, parse_native_score_events, parse_grind_observations, parse_fixture_status
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
        "except Exception as exc:\n result={'hard_stop':type(exc).__name__,'reason':str(exc) if isinstance(exc,HardStop) else 'remote_safety_failure'}\n" + \
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
                    raise HardStop(result.get("reason", "remote_safety_failure"))
                if result.get("input_sent"):
                    if result.get("neutralized") is not True:
                        raise HardStop("neutralization_unconfirmed")
                    robot.save_run(result, output / ("trial-%03d" % len(summary["trials"])))
                elif result.get("outcome") != "SKIPPED_CONTEXT":
                    raise HardStop("invalid_response")
                summary["trials"].append({"name": case["name"], "repeat": repetition + 1,
                    "outcome": result["outcome"], "input_sent": result["input_sent"],
                    "criteria": result.get("criteria", {}), "observations": result.get("observations", {}),
                    "frames": [{k: f[k] for k in ("sha256", "format", "elapsed_ms")} for f in result.get("frames", [])]})
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
