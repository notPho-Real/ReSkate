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
        keys(case, ("name", "family", "phases", "expected", "timeout_ms", "reset_ms"), ("repeat",))
        for field in ("name", "family"):
            if not isinstance(case[field], str) or not re.fullmatch(r"[a-z][a-z0-9-]{0,47}", case[field]):
                raise ValueError("Invalid candidate label")
        if case["name"] in names:
            raise ValueError("Duplicate candidate")
        names.add(case["name"])
        trials += integer(case.get("repeat", 1), 1, 20)
        timeout = integer(case["timeout_ms"], 3000, 12000)
        integer(case["reset_ms"], 2000, 10000)
        keys(case["expected"], ("states",), ("trick",))
        states = case["expected"]["states"]
        if not isinstance(states, list) or len(states) > 12:
            raise ValueError("Invalid expected states")
        for state in states:
            integer(state, 0, 10000)
        if "trick" in case["expected"] and (not isinstance(case["expected"]["trick"], str) or
                not re.fullmatch(r"[a-z][a-z0-9-]{0,47}", case["expected"]["trick"])):
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
            "combo": parse_combo_samples(text), "score": parse_score_samples(text)}


def subsequence(expected, rows):
    index = 0
    for row in rows:
        if index < len(expected) and row["valid"] and row["state"] == expected[index]:
            index += 1
    return index == len(expected)


def perform_case(plan, case, env):
    env.integrity(plan)
    baseline = env.wait_idle(case["reset_ms"])
    if baseline is None:
        return {"name": case["name"], "outcome": "SKIPPED_CONTEXT", "input_sent": False,
                "reason": "No fresh stable on-board heartbeat", "frames": [], "observations": {}}
    baseline_observation = env.observe()
    before = baseline_observation.get("combo", {}).get("samples", [])
    watermark = before[-1]["ms"] if before else None
    result = env.gesture(case)
    # Gesture's finally neutralizes BEFORE waiting on screenshots or reset.
    env.integrity(plan)
    reset = env.wait_idle(case["reset_ms"])
    log_boundary = baseline_observation.get("log_end")
    obs = env.observe(since=log_boundary)
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
                            "named_trick": "UNVERIFIED"}, neutralized=True)
    # State numbers alone cannot distinguish named flips. No false trick PASS.
    return result


class DeckEnvironment:
    """Uses only trusted robot/probe functions supplied by the remote payload."""
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
                start = max(0, size - 65536)
                if since is not None:
                    if since > size:
                        return {"states": [], "buttons": [], "log_end": size,
                                "evidence_gap": "log_truncated"}
                    start = max(start, since)
                stream.seek(start)
                data = stream.read(65536)
                result = observations(data.decode(errors="replace"))
                result["log_end"] = start + len(data)
                result["evidence_gap"] = "tail_limit" if since is not None and start > since else None
                return result
        except OSError:
            return {"states": [], "buttons": []}

    def wait_idle(self, milliseconds):
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
            rows = [r for r in self.observe()["states"] if r["seq"] > watermark]
            if len(rows) >= 3 and all(r["valid"] and r["state"] == 100 and r["mode"] == 1 for r in rows[-3:]):
                return rows[-1]["seq"]
            time.sleep(0.25)
        return None

    def gesture(self, case):
        return experiment_events(case["name"], phase_events(case), case["timeout_ms"] / 1000)


# LOCAL TRANSPORT: excluded from the remote payload.
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
import controller
import robot
from probe import parse_combo_samples, parse_score_samples

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
