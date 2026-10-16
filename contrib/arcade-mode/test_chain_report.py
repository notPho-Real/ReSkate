"""Synthetic contract tests only. These fixtures are not real-game acceptance."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent / "lab"))
import chain_report as report


def trial(chain="A", trial_id="t1", context="regular-location-a"):
    events = []
    fatigue = dict.fromkeys(report.FATIGUE, 0)
    score = 0

    def add(kind, combo="combo1", **extra):
        nonlocal score, fatigue
        previous_score = score
        if kind in ("ollie", "flip", "grind", "manual"):
            score += 100
            fatigue["global"] += 1
            if kind in ("grind", "manual"):
                fatigue[kind] += 1
        if kind in ("bank", "bail", "reset"):
            score, fatigue = 0, dict.fromkeys(report.FATIGUE, 0)
        event = {"seq": len(events) + 1, "time_ms": 10 * len(events), "kind": kind,
                 "observed": True, "combo_id": combo, "unbanked_score": score,
                 "fatigue": fatigue.copy()}
        if kind in report.NATIVE:
            event["native_confirmed"] = True
        if kind == "flip":
            event["trick"] = "kickflip"
        if kind in ("grind", "manual"):
            event["difficulty"] = fatigue[kind]
        if kind in ("bank", "bail"):
            event["banked_delta"] = previous_score if kind == "bank" else 0
        if kind in ("bail", "reset"):
            event["hud_visible"] = False
        if kind == "neutralized":
            event["input_neutral"] = True
        if kind == "reset":
            event["riding"] = True
        event.update(extra)
        events.append(event)

    if chain == "H":
        add("flip")
        add("manual")
        add("bail")
        add("neutralized")
        add("reset")
        add("flip", "combo2")
        add("land", "combo2")
        add("bank", "combo2")
        add("neutralized", "combo2")
        add("reset", "combo2")
    elif chain == "G":
        score = 100
        add("land", link_deadline_ms=40, multiplier=1)
        for _ in range(3):
            add("revert", link_deadline_ms=40, multiplier=1)
        add("wait")
        add("bank")
        add("neutralized")
        add("reset")
    else:
        for kind in report.PATTERNS[chain]:
            add("flip" if kind in ("kickflip", "air") else kind)
        if events[-1]["kind"] != "land":
            add("grounded_exit")
        add("bank")
        add("neutralized")
        add("reset")
    return {"session_id": "session1", "trial_id": trial_id, "context_id": context,
            "chain": chain, "dropped_events": 0, "events": events}


def document(trials):
    return {"schema": 1, "source_commit": "a" * 40, "dll_sha256": "b" * 64, "trials": trials}


def runtime_document():
    # Policy-injected synthetic trace only; no shipped binary/native outcome proof.
    candidate = trial("G")
    for event, ms in zip(candidate['events'], (0, 500, 600, 700, 1600, 1601, 1602, 1603)):
        event['time_ms'] = ms
    candidate['events'][0]['link_deadline_ms'] = 750
    for event in candidate['events'][1:4]:
        event['link_deadline_ms'] = 1600
    value = document([candidate])
    value.update(schema=2, combo_policy=dict(id='arcade-runtime-750-1100-v1',
                 source_commit=value['source_commit'], dll_sha256=value['dll_sha256']))
    return value


class ChainReportTests(unittest.TestCase):
    def assert_fails(self, candidate, reason):
        result = report.grade_trial(candidate)
        self.assertFalse(result["pass"])
        self.assertIn(reason, result["reason"])

    def test_all_patterns_have_synthetic_contract_fixture(self):
        for chain in report.CHAINS:
            with self.subTest(chain=chain):
                self.assertTrue(report.grade_trial(trial(chain))["pass"])

    def test_repetition_context_and_complete_matrix_requirements(self):
        rows = [trial("A", f"t{i}", f"context{i % 2}") for i in range(19)]
        self.assertFalse(report.evaluate(document(rows))["chains"]["A"]["repeatable"])
        rows.append(trial("A", "t19", "context1"))
        result = report.evaluate(document(rows))
        self.assertTrue(result["chains"]["A"]["repeatable"])
        self.assertFalse(result["all_chains_repeatable"])
        for row in rows:
            row["context_id"] = "one-context"
        self.assertFalse(report.evaluate(document(rows))["chains"]["A"]["repeatable"])
        rows = [trial(chain, f"{chain}{i}", f"context{i % 2}")
                for chain in report.CHAINS for i in range(20)]
        result = report.evaluate(document(rows))
        self.assertTrue(result["all_chains_repeatable"])
        self.assertEqual(result["live_adapter"], "pending")
        self.assertEqual(result["scope"], "offline-semantic-evidence-only")
        rows.append(trial("A", "ordinary-failure"))
        rows[-1]["events"][0]["native_confirmed"] = False
        result = report.evaluate(document(rows))
        self.assertFalse(result["chains"]["A"]["repeatable"])
        self.assertEqual(result["chains"]["A"]["failed"], 1)

    def test_requests_and_unconfirmed_results_never_pass(self):
        for key, value, reason in (("kind", "requested_flip", "requests"),
                                   ("native_confirmed", False, "unconfirmed"),
                                   ("observed", False, "observed evidence")):
            candidate = trial()
            candidate["events"][0][key] = value
            self.assert_fails(candidate, reason)
        candidate = trial()
        del candidate["events"][0]["native_confirmed"]
        self.assert_fails(candidate, "unconfirmed")
        candidate = trial()
        candidate["events"][1]["trick"] = "heelflip"
        self.assert_fails(candidate, "kickflip")

    def test_telemetry_integrity_and_provenance(self):
        for mutation, reason in ((lambda t: t.update(dropped_events=1), "dropped"),
                                 (lambda t: t["events"][1].update(seq=3), "sequence"),
                                 (lambda t: t["events"][1].update(time_ms=-1), "clock"),
                                 (lambda t: t["events"][0].update(fatigue={}), "fatigue"),
                                 (lambda t: t["events"][0].update(unbanked_score=float("nan")), "score")):
            candidate = trial()
            mutation(candidate)
            self.assert_fails(candidate, reason)
        with self.assertRaisesRegex(ValueError, "duplicate"):
            report.evaluate(document([trial(), trial()]))
        with self.assertRaisesRegex(ValueError, "pinned"):
            report.evaluate({"schema": 1, "trials": []})

    def test_combo_and_banking_observations(self):
        candidate = trial("C")
        candidate["events"][1]["combo_id"] = "unexpected-new-combo"
        self.assert_fails(candidate, "combo split")
        candidate = trial("C")
        candidate["events"][2]["kind"] = "bank"
        candidate["events"][2].update(fatigue=dict.fromkeys(report.FATIGUE, 0), unbanked_score=0, banked_delta=1)
        self.assert_fails(candidate, "missing ordered")
        candidate = trial("C")
        candidate["events"][-3]["banked_delta"] += 1
        self.assert_fails(candidate, "banked amount")
        candidate = trial("C")
        candidate["events"][-2]["input_neutral"] = False
        self.assert_fails(candidate, "neutral")

    def test_banking_requires_observed_grounded_exit_after_active_mechanic(self):
        for chain in ("A", "B", "E", "F"):
            candidate = trial(chain)
            del candidate["events"][-4]  # Remove the actual grounded exit.
            for i, event in enumerate(candidate["events"]):
                event["seq"] = i + 1
            self.assert_fails(candidate, "grounded exit")
        candidate = trial("H")
        candidate["events"][6]["kind"] = "wait"
        self.assert_fails(candidate, "grounded exit")
        candidate = trial("A")
        candidate["events"][-4]["native_confirmed"] = False
        self.assert_fails(candidate, "unconfirmed")

    def test_fatigue_must_persist_and_raise_both_mode_difficulties(self):
        for mode in ("manual", "grind"):
            for field in ("fatigue", "difficulty"):
                candidate = trial("F")
                entries = [e for e in candidate["events"] if e["kind"] == mode]
                entries[1][field] = copy.deepcopy(entries[0][field])
                self.assert_fails(candidate, "fatigue")
        candidate = trial("F")
        candidate["events"][3]["fatigue"]["global"] = 0
        self.assert_fails(candidate, "fatigue reset")

    def test_revert_abuse_and_expiry(self):
        for field in ("link_deadline_ms", "multiplier", "unbanked_score"):
            candidate = trial("G")
            candidate["events"][2][field] += 1
            self.assert_fails(candidate, "repeated revert")
        candidate = trial("G")
        candidate["events"][4]["time_ms"] = 39
        self.assert_fails(candidate, "expiry")
        candidate = trial("G")
        del candidate["events"][2]["link_deadline_ms"]
        self.assert_fails(candidate, "grace/multiplier")
        candidate = trial("G")
        extra = copy.deepcopy(candidate["events"][3])
        extra["multiplier"] += 1
        candidate["events"].insert(4, extra)
        for i, event in enumerate(candidate["events"]):
            event["seq"] = i + 1
        self.assert_fails(candidate, "repeated revert")

    def test_first_revert_cannot_score_or_create_unbounded_grace(self):
        for field in ("multiplier", "unbanked_score"):
            candidate = trial("G")
            for event in candidate["events"][1:4]:
                event[field] += 1
            self.assert_fails(candidate, "repeated revert")
        candidate = trial("G")
        candidate["events"][0]["link_deadline_ms"] = 10
        self.assert_fails(candidate, "original landing grace")
        candidate = trial("G")
        for event in candidate["events"][1:4]:
            event["link_deadline_ms"] = 100000
        self.assert_fails(candidate, "450 ms bound")
        candidate = trial("G")
        candidate["events"][0]["link_deadline_ms"] = 100000
        self.assert_fails(candidate, "450 ms bound")

    def test_explicit_runtime_policy_resolves_synthetic_default_policy_mismatch(self):
        value = runtime_document()
        self.assert_fails(value['trials'][0], '450 ms bound')
        result = report.evaluate(value)
        self.assertTrue(result['trials'][0]['pass'])
        self.assertFalse(result['all_chains_repeatable'])
        self.assertEqual(result['live_adapter'], 'pending')
        self.assertEqual(result['combo_policy']['verification'], 'declared-only')
        self.assertEqual(result['combo_policy']['source_commit'], value['source_commit'])
        self.assertEqual(result['combo_policy']['dll_sha256'], value['dll_sha256'])
        self.assertEqual(result['combo_policy']['landing_grace_ms'], 750)
        self.assertEqual(result['combo_policy']['revert_grace_ms'], 1100)
        legacy = report.evaluate(document([trial('G')]))
        self.assertTrue(legacy['trials'][0]['pass'])
        self.assertEqual(legacy['combo_policy']['landing_grace_ms'], 450)
        self.assertEqual(legacy['combo_policy']['revert_grace_ms'], 450)

    def test_runtime_policy_preserves_abuse_and_strict_deadline_checks(self):
        for failure in ('landing_bound', 'revert_bound', 'late_first', 'extended', 'multiplier', 'score'):
            value = runtime_document()
            events = value['trials'][0]['events']
            if failure == 'landing_bound': events[0]['link_deadline_ms'] = 751
            elif failure == 'revert_bound':
                for event in events[1:4]: event['link_deadline_ms'] = 1601
            elif failure == 'late_first': events[0]['link_deadline_ms'] = 500
            elif failure == 'extended': events[2]['link_deadline_ms'] += 1
            elif failure == 'multiplier': events[2]['multiplier'] += 1
            else: events[2]['unbanked_score'] += 1
            with self.subTest(failure=failure):
                self.assertFalse(report.evaluate(value)['trials'][0]['pass'])

    def test_policy_requires_explicit_version_and_matching_build_pins(self):
        for failure in ('legacy', 'missing', 'unknown', 'source', 'dll', 'arbitrary_limit', 'wrong_type'):
            value = runtime_document()
            if failure == 'legacy': value['schema'] = 1
            elif failure == 'missing': del value['combo_policy']
            elif failure == 'unknown': value['combo_policy']['id'] = 'unreviewed-v9'
            elif failure == 'source': value['combo_policy']['source_commit'] = 'c' * 40
            elif failure == 'dll': value['combo_policy']['dll_sha256'] = 'c' * 64
            elif failure == 'arbitrary_limit': value['combo_policy']['revert_grace_ms'] = 100000
            else: value['combo_policy']['id'] = []
            with self.subTest(failure=failure), self.assertRaises(ValueError):
                report.evaluate(value)
        for policy in ('unknown', None, {}, True):
            self.assertFalse(report.grade_trial(trial('G'), policy_id=policy)['pass'])

    def test_bail_reset_and_fresh_combo_are_required(self):
        for index, key, value, reason in ((2, "banked_delta", 1, "lose unbanked"),
                                          (2, "hud_visible", True, "HUD"),
                                          (3, "input_neutral", False, "neutral"),
                                          (4, "riding", False, "riding"),
                                          (5, "combo_id", "combo1", "fresh combo"),
                                          (5, "native_confirmed", False, "unconfirmed")):
            candidate = trial("H")
            candidate["events"][index][key] = value
            self.assert_fails(candidate, reason)
        candidate = trial("H")
        candidate["events"][2]["fatigue"]["manual"] = 1
        self.assert_fails(candidate, "clear score or fatigue")


if __name__ == "__main__":
    unittest.main()
