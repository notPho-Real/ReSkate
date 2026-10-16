"""Offline A-H evidence grading. The live telemetry adapter is not implemented.

Schema 1 is deliberately semantic, not a parser for controller requests. A trusted
observer adapter must supply {schema, source_commit, dll_sha256, trials}. Each trial
contains session_id, trial_id, context_id, chain, dropped_events, events. Context
identifies measured stance/location/simulation conditions. Each event contains seq,
time_ms, kind, observed, combo_id, unbanked_score, fatigue (global/grind/manual).
The enclosing trial supplies event session/trial identity. Sequence is contiguous;
time is monotonic milliseconds. Native events also require native_confirmed=true.

Flip events carry trick (kickflip or another sanitized identity). Grind/manual
events carry difficulty. Reverts/landing in G carry link_deadline_ms and multiplier.
Banking requires a native-confirmed land or grounded_exit after the latest active
air/grind/manual mechanic. grounded_exit means independently observed riding on
the ground after leaving a balance mode, not merely release of its command.
Schema 1 keeps the model-default 450 ms landing/revert limits. Schema 2 requires
combo_policy={id, source_commit, dll_sha256}, matching the enclosing build pins.
Only named, versioned policies below are supported; arbitrary limits are rejected.
The runtime policy follows native_combo_tuning() in arcade_native_events.h:
750 ms landing / 1100 ms revert. These are declared policies, not verification
that the supplied binary implements them. G adds no score or multiplier and its
first revert must occur before the original landing deadline.
Bank events carry banked_delta; bail events carry banked_delta=0 and hud_visible=false.
Neutralized events require input_neutral=true; reset events require riding=true,
hud_visible=false. Bank/bail/reset fatigue and unbanked_score must be zero.
No event is inferred from intent; missing fields fail. Passing synthetic fixtures
proves only the grader. This report does not cover HUD screenshots or stress runs.
"""
import argparse
import json
import math
import re
from pathlib import Path

CHAINS = tuple("ABCDEFGH")
MIN_TRIALS = 20
DEFAULT_POLICY = "model-default-450-v1"
COMBO_POLICIES = {DEFAULT_POLICY: (450, 450), "arcade-runtime-750-1100-v1": (750, 1100)}
ACTIVE_MECHANICS = frozenset(("ollie", "flip", "grind", "manual"))
NATIVE = ACTIVE_MECHANICS | {"revert", "land", "grounded_exit", "bail"}
KINDS = NATIVE | {"bank", "neutralized", "reset", "wait"}
FATIGUE = ("global", "grind", "manual")
PATTERNS = {
    "A": ("ollie", "kickflip", "grind"),
    "B": ("air", "land", "revert", "manual"),
    "C": ("kickflip", "manual", "ollie", "flip", "land"),
    "D": ("grind", "flip", "land"),
    "E": ("grind", "flip", "manual"),
    "F": ("grind", "flip", "manual", "flip", "grind", "flip", "manual"),
    "G": ("land", "revert", "revert", "revert", "wait", "bank"),
}


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def number(value):
    return type(value) in (int, float) and 0 <= value <= 1e15 and math.isfinite(value)


def identity(value):
    return isinstance(value, str) and re.fullmatch(r"[A-Za-z0-9_.:-]{1,96}", value)


def zeroed(event):
    return event["unbanked_score"] == 0 and all(event["fatigue"][k] == 0 for k in FATIGUE)


def matches(event, wanted):
    if wanted == "air":
        return event["kind"] in ("flip", "ollie")
    if wanted == "kickflip":
        return event["kind"] == "flip" and event.get("trick") == "kickflip"
    return event["kind"] == wanted


def ordered(events, pattern):
    result, start = [], 0
    for wanted in pattern:
        found = next((i for i in range(start, len(events)) if matches(events[i], wanted)), None)
        require(found is not None, f"missing ordered observed {wanted}")
        result.append(found)
        start = found + 1
    return result


def validate_events(trial):
    require(isinstance(trial, dict), "invalid trial")
    for key in ("session_id", "trial_id", "context_id"):
        require(identity(trial.get(key)), f"missing or invalid {key}")
    require(trial.get("chain") in CHAINS, "unknown chain")
    require(type(trial.get("dropped_events")) is int and trial["dropped_events"] == 0,
            "missing or dropped telemetry")
    events = trial.get("events")
    require(isinstance(events, list) and 1 <= len(events) <= 4096, "missing or excessive events")
    previous_seq, previous_time = None, None
    for event in events:
        require(isinstance(event, dict), "invalid event")
        seq, timestamp = event.get("seq"), event.get("time_ms")
        require(type(seq) is int and seq >= 0 and (previous_seq is None or seq == previous_seq + 1),
                "missing, duplicate, or reordered event sequence")
        require(type(timestamp) is int and timestamp >= 0 and
                (previous_time is None or timestamp >= previous_time), "invalid event clock")
        previous_seq, previous_time = seq, timestamp
        kind = event.get("kind")
        require(kind in KINDS, "unsupported event kind; requests are not observations")
        require(event.get("observed") is True, "event lacks observed evidence")
        require(identity(event.get("combo_id")), "event lacks combo identity")
        require(number(event.get("unbanked_score")), "missing score snapshot")
        fatigue = event.get("fatigue")
        require(isinstance(fatigue, dict) and set(fatigue) == set(FATIGUE) and
                all(number(fatigue[k]) for k in FATIGUE), "missing or invalid fatigue snapshot")
        if kind in NATIVE:
            require(event.get("native_confirmed") is True, "native outcome unconfirmed")
        if kind == "flip":
            require(identity(event.get("trick")), "flip lacks exact observed identity")
        if kind in ("grind", "manual"):
            require(number(event.get("difficulty")), "balance entry lacks measured difficulty")
        if kind in ("bank", "bail", "reset"):
            require(zeroed(event), "bank/bail/reset failed to clear score or fatigue")
        if kind in ("bank", "bail"):
            require(number(event.get("banked_delta")), "missing banking observation")
        if kind == "neutralized":
            require(event.get("input_neutral") is True, "synthetic input not confirmed neutral")
        if kind == "reset":
            require(event.get("riding") is True and event.get("hud_visible") is False,
                    "reset lacks riding state or HUD clearance")
    return events


def consistent_combo(events, stop):
    combo = events[0]["combo_id"]
    require(all(e["combo_id"] == combo for e in events[:stop + 1]), "combo split during chain")
    require(not any(e["kind"] in ("bank", "bail", "reset") for e in events[:stop]),
            "premature bank, bail, or reset")
    for before, after in zip(events[:stop], events[1:stop]):
        require(all(after["fatigue"][k] >= before["fatigue"][k] for k in FATIGUE),
                "fatigue reset or decreased within active combo")


def terminal_bank(events, after):
    bank = next((i for i in range(after + 1, len(events)) if events[i]["kind"] == "bank"), None)
    require(bank is not None, "missing final banking observation")
    consistent_combo(events, bank)
    last_active = max((i for i in range(bank) if events[i]["kind"] in ACTIVE_MECHANICS), default=-1)
    require(any(events[i]["kind"] in ("land", "grounded_exit")
                for i in range(last_active + 1, bank)),
            "bank lacks observed grounded exit after the last active mechanic")
    require(events[bank - 1]["unbanked_score"] > 0 and
            events[bank]["banked_delta"] == events[bank - 1]["unbanked_score"],
            "banked amount does not match observed unbanked score")
    return bank


def reset_after(events, terminal):
    pair = ordered(events[terminal + 1:], ("neutralized", "reset"))
    reset = terminal + 1 + pair[-1]
    require(all(e["kind"] in ("neutralized", "reset", "wait") for e in events[terminal + 1:reset + 1]),
            "unexpected gameplay before confirmed recovery")
    return reset


def grade_trial(trial, *, policy_id=DEFAULT_POLICY):
    """Return a failure reason, never turn absent or malformed evidence into PASS."""
    try:
        require(isinstance(policy_id, str) and policy_id in COMBO_POLICIES, "unsupported combo policy")
        landing_limit, revert_limit = COMBO_POLICIES[policy_id]
        events = validate_events(trial)
        chain = trial["chain"]
        if chain == "H":
            bail = next((i for i, e in enumerate(events) if e["kind"] == "bail"), None)
            require(bail is not None and bail >= 2, "missing active multi-action combo bail")
            require(sum(e["kind"] in ("ollie", "flip", "grind", "manual") for e in events[:bail]) >= 2,
                    "bail was not during a multi-action combo")
            consistent_combo(events, bail)
            require(events[bail - 1]["unbanked_score"] > 0 and events[bail]["banked_delta"] == 0,
                    "bail did not lose unbanked score")
            require(events[bail].get("hud_visible") is False, "bail did not clear HUD")
            reset = reset_after(events, bail)
            fresh = next((i for i in range(reset + 1, len(events))
                          if events[i]["kind"] in ("ollie", "flip", "grind", "manual")), None)
            require(fresh is not None and events[fresh]["combo_id"] != events[bail]["combo_id"] and
                    events[fresh]["unbanked_score"] > 0, "no confirmed functioning fresh combo after bail")
            bank = terminal_bank(events[fresh:], 0) + fresh
            require(reset_after(events, bank) == len(events) - 1, "ungraded events after final reset")
        else:
            indices = ordered(events, PATTERNS[chain])
            stop = indices[-1]
            if chain == "G":
                land, first, second, third, wait, bank = (events[i] for i in indices)
                prefix = events[:indices[-1]]
                require(prefix[0] is land and sum(e["kind"] == "land" for e in prefix) == 1 and
                        all(e["kind"] in ("land", "revert", "wait") for e in prefix),
                        "revert abuse trial contains another linking action")
                reverts = [e for e in prefix if e["kind"] == "revert"]
                for event in [land, *reverts]:
                    require(type(event.get("link_deadline_ms")) is int and
                            event["link_deadline_ms"] >= event["time_ms"] and
                            number(event.get("multiplier")), "missing revert grace/multiplier observation")
                require(first["time_ms"] < land["link_deadline_ms"],
                        "first revert occurred after original landing grace expired")
                require(land["link_deadline_ms"] <= land["time_ms"] + landing_limit and
                        first["link_deadline_ms"] <= first["time_ms"] + revert_limit,
                        "landing or renewed revert grace exceeds " +
                        ("pinned 450 ms bound" if policy_id == DEFAULT_POLICY else
                         f"declared {landing_limit}/{revert_limit} ms policy bounds"))
                require(all(e["link_deadline_ms"] == first["link_deadline_ms"] and
                            e["multiplier"] == land["multiplier"] and
                            e["unbanked_score"] == land["unbanked_score"] for e in reverts),
                        "repeated revert extended grace or farmed multiplier")
                require(wait["time_ms"] >= first["link_deadline_ms"] and
                        bank["time_ms"] >= first["link_deadline_ms"], "grace expiry not observed")
                stop = terminal_bank(events, indices[-2])
            else:
                stop = terminal_bank(events, stop)
            if chain == "F":
                for kind in ("grind", "manual"):
                    entries = [events[i] for i in indices if events[i]["kind"] == kind]
                    require(entries[1]["fatigue"][kind] > entries[0]["fatigue"][kind] and
                            entries[1]["fatigue"]["global"] > entries[0]["fatigue"]["global"] and
                            entries[1]["difficulty"] > entries[0]["difficulty"],
                            f"repeated {kind} lacks persistent fatigue and increased difficulty")
            require(reset_after(events, stop) == len(events) - 1, "ungraded events after final reset")
        return {"pass": True, "reason": None}
    except (ValueError, KeyError, TypeError, IndexError) as error:
        return {"pass": False, "reason": str(error)}


def evaluate(document):
    require(isinstance(document, dict) and type(document.get("schema")) is int and
            document["schema"] in (1, 2), "unsupported report schema")
    for field, size in (("source_commit", 40), ("dll_sha256", 64)):
        require(isinstance(document.get(field), str) and
                re.fullmatch(r"[0-9a-f]{%d}" % size, document[field]), "missing pinned build identity")
    policy_id = DEFAULT_POLICY
    if document["schema"] == 1:
        require("combo_policy" not in document, "explicit combo policy requires report schema 2")
    else:
        policy = document.get("combo_policy")
        require(isinstance(policy, dict) and set(policy) == {"id", "source_commit", "dll_sha256"},
                "missing or invalid pinned combo policy")
        policy_id = policy["id"]
        require(isinstance(policy_id, str) and policy_id in COMBO_POLICIES, "unsupported combo policy")
        require(all(policy[k] == document[k] for k in ("source_commit", "dll_sha256")),
                "combo policy build pins do not match evidence")
    trials = document.get("trials")
    require(isinstance(trials, list) and len(trials) <= 10000, "invalid trial collection")
    results, seen = [], set()
    for trial in trials:
        require(isinstance(trial, dict), "invalid trial")
        pair = (trial.get("session_id"), trial.get("trial_id"))
        require(all(isinstance(v, str) for v in pair) and pair not in seen, "duplicate or missing trial identity")
        seen.add(pair)
        result = grade_trial(trial, policy_id=policy_id)
        results.append({"session_id": pair[0], "trial_id": pair[1], "chain": trial.get("chain"),
                        "context_id": trial.get("context_id"), **result})
    chains = {}
    for chain in CHAINS:
        rows = [r for r in results if r["chain"] == chain]
        passed = sum(r["pass"] for r in rows)
        contexts = len({r["context_id"] for r in rows if r["pass"]})
        chains[chain] = {"trials": len(rows), "passed": passed, "failed": len(rows) - passed,
                         "contexts": contexts,
                         "repeatable": passed >= MIN_TRIALS and passed == len(rows) and contexts >= 2}
    return {"schema": document["schema"], "source_commit": document["source_commit"], "dll_sha256": document["dll_sha256"],
            "combo_policy": {"id": policy_id, "source_commit": document["source_commit"],
                             "dll_sha256": document["dll_sha256"], "verification": "declared-only",
                             "landing_grace_ms": COMBO_POLICIES[policy_id][0],
                             "revert_grace_ms": COMBO_POLICIES[policy_id][1]},
            "scope": "offline-semantic-evidence-only", "live_adapter": "pending",
            "all_chains_repeatable": all(c["repeatable"] for c in chains.values()) and
                                     all(r["pass"] for r in results),
            "chains": chains, "trials": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence", type=Path)
    args = parser.parse_args()
    try:
        report = evaluate(json.loads(args.evidence.read_text()))
    except (ValueError, OSError) as error:
        parser.error(str(error))
    print(json.dumps(report, indent=2))
    return 0 if report["all_chains_repeatable"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
