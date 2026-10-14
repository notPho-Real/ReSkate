"""Offline batch validation, transport and cleanup regressions. No real Deck."""
import copy
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent / "lab"))
import trick_batch as batch
import robot
import inspect_project


def plan():
    return {"schema": 1, "source_commit": "a" * 40, "dll_sha256": "b" * 64, "stock_sha256": "c" * 64,
            "candidates": [{"name": "kickflip", "family": "flip", "phases": [
                {"ms": 0}, {"ms": 1000, "ry": 32767}, {"ms": 1600, "rx": 28000, "ry": -28000},
                {"ms": 1850}], "expected": {"states": [100, 200, 100], "trick": "kickflip"},
                "timeout_ms": 6000, "reset_ms": 3000}]}


class BatchTests(unittest.TestCase):
    def test_persistent_batch_reuses_environment_after_ordinary_failure(self):
        value = plan()
        value["candidates"][0]["repeat"] = 3
        environment = object()
        outcomes = [{"criteria": {"named_trick": "FAIL"}},
                    {"criteria": {"returned_to_idle": "FAIL"}},
                    {"criteria": {"named_trick": "PASS"}}]
        with patch.object(batch, "perform_case", side_effect=outcomes) as run:
            result = batch.perform_batch(value, environment)
        self.assertEqual(len(result["trials"]), 3)
        self.assertEqual([r["repetition"] for r in result["trials"]], [1, 2, 3])
        self.assertTrue(all(call.args[2] is environment for call in run.call_args_list))
        self.assertEqual(result["trials"][0]["criteria"]["named_trick"], "FAIL")

    def test_persistent_batch_stops_for_infrastructure_failure(self):
        value = plan()
        value["candidates"][0]["repeat"] = 3
        with patch.object(batch, "perform_case", side_effect=batch.HardStop("neutralization")) as run:
            result = batch.perform_batch(value, object())
        self.assertEqual(run.call_count, 1)
        self.assertTrue(result["trials"][0]["hard_stop"])

    def test_deck_environment_uses_existing_session(self):
        session = Mock()
        case = plan()["candidates"][0]
        environment = batch.DeckEnvironment(session=session)
        environment.gesture(case)
        session.run.assert_called_once_with(case["name"], batch.phase_events(case), case["timeout_ms"] / 1000)

    def test_trusted_batch_dispatch_uses_one_context_for_all_trials(self):
        spec = importlib.util.spec_from_file_location("batch_runner_test", Path(batch.__file__).with_name("trusted_runner.py"))
        runner = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(runner)
        value = plan()
        value["candidates"][0]["repeat"] = 3
        pins = {k: value[k] for k in ("source_commit", "dll_sha256", "stock_sha256")}
        request = {"operation": "batch", "plan": value, "pins": pins, "game_sha256": "d" * 64}
        session = Mock()
        session.__enter__ = Mock(return_value=session)
        session.__exit__ = Mock(return_value=False)
        factory = Mock(return_value=session)
        namespace = {"json": json, "sys": sys, "Path": Path, "HardStop": batch.HardStop,
                     "snapshot": lambda _: {"safe_paths": True, "game": "/fixture",
                         "installed_commit": pins["source_commit"], "dll_sha256": pins["dll_sha256"],
                         "backup_sha256": pins["stock_sha256"]},
                     "sha": lambda _: "d" * 64, "validate_plan": batch.validate_plan,
                     "ExperimentSession": factory, "DeckEnvironment": lambda session: session,
                     "perform_batch": batch.perform_batch}
        output = io.StringIO()
        with patch.object(sys, "stdin", io.StringIO(json.dumps(request))), contextlib.redirect_stdout(output), \
             patch.object(batch, "perform_case", side_effect=lambda *_: {"criteria": {"named_trick": "FAIL"}}) as run:
            exec(compile(runner.REMOTE, "trusted-batch-dispatch", "exec"), namespace)
        factory.assert_called_once_with(max_seconds=1100)
        session.__enter__.assert_called_once()
        session.__exit__.assert_called_once()
        self.assertEqual(run.call_count, 3)
        self.assertTrue(all(call.args[2] is session for call in run.call_args_list))
        self.assertEqual(len(json.loads(output.getvalue())["trials"]), 3)

    def test_inspections_have_no_forwarded_arguments(self):
        self.assertIn("notPho-Real/ReSkate", inspect_project.command("runs"))
        self.assertIn("arcade-mode", inspect_project.command("runs"))
        self.assertEqual(inspect_project.command("lab-status")[-1], "status")
        with self.assertRaises(KeyError):
            inspect_project.command("status; touch /tmp/owned")

    def test_screenshot_count_and_hash_checked(self):
        with tempfile.TemporaryDirectory() as temp:
            for frames in ([], [{}] * 5, [{"image_base64": "AA==", "sha256": "0" * 64,
                                        "format": "png"}] * 4):
                with self.assertRaises(ValueError):
                    robot.save_run({"schema": 1, "recipe": "test", "frames": frames},
                                   Path(temp).resolve() / "out")

    def test_cleanup_success_and_failure(self):
        for failure in (None, RuntimeError("failed replay")):
            pad = Mock()
            pad.__enter__ = Mock(return_value=pad)
            pad.__exit__ = Mock(return_value=False)
            with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [1]}), \
                 patch.object(robot, "Gamepad", return_value=pad), patch.object(robot.time, "sleep"), \
                 patch.object(robot, "screenshot"), patch.object(robot, "periodic", return_value=[]), \
                 patch.object(robot, "replay", side_effect=failure), \
                 patch.object(robot.signal, "signal"), patch.object(robot.signal, "alarm"):
                if failure:
                    with self.assertRaises(RuntimeError):
                        robot.experiment_events("test", [], 3)
                else:
                    self.assertTrue(robot.experiment_events("test", [], 3)["input_sent"])
            pad.neutral.assert_called()
            pad.__exit__.assert_called_once()

    def test_valid_and_full_neutral_phases(self):
        data = batch.validate_plan(plan())
        events = batch.phase_events(data["candidates"][0])
        self.assertTrue(all(event[2] == 0 for event in events[-1][1]))
        self.assertEqual(len(events[-1][1]), 14)

    def test_unknown_fields_reject_code(self):
        for field in ("command", "shell", "code", "host", "output", "restore"):
            value = plan()
            value["candidates"][0][field] = "touch /tmp/owned"
            with self.subTest(field=field), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_labels_cannot_inject(self):
        for label in ("$(id)", "';import os", "../escape", "a\nb", "x;sudo", "x`id`"):
            value = plan()
            value["candidates"][0]["name"] = label
            with self.subTest(label=label), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_numeric_trick_names_keep_the_same_data_only_boundary(self):
        value = plan()
        value["candidates"][0]["expected"]["trick"] = "360-flip"
        self.assertEqual(batch.validate_plan(value), value)
        for bad in ("360;id", "360$(id)", "360/escape", "360\nflip", "", "3" * 49):
            value["candidates"][0]["expected"]["trick"] = bad
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_phase_limits(self):
        for key, bad in (("ms", -1), ("ms", True), ("rx", 32768), ("ry", float("nan")),
                         ("lt", -1), ("dx", 2), ("buttons", ["START"]), ("buttons", "A")):
            value = plan()
            value["candidates"][0]["phases"][0][key] = bad
            with self.subTest(key=key, bad=bad), self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_batch_and_time_bounds(self):
        for field, bad in (("repeat", 21), ("timeout_ms", 12001), ("reset_ms", 10001)):
            value = plan()
            value["candidates"][0][field] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)
        value = plan()
        value["candidates"] *= 25
        with self.assertRaises(ValueError):
            batch.validate_plan(value)

    def test_duplicate_and_nonmonotonic(self):
        value = plan()
        value["candidates"][0]["phases"][1]["ms"] = 0
        with self.assertRaises(ValueError):
            batch.validate_plan(value)

    def test_remote_payload_uses_stdin_not_code(self):
        value = plan()
        source = batch.remote_payload(value, value["candidates"][0])
        compile(source, "remote-batch", "exec")
        self.assertIn("request=json.load(sys.stdin)", source)
        self.assertNotIn("change_installation(", source)
        self.assertNotIn("publish_report(", source)
        self.assertNotIn(value["source_commit"], source)

    def test_remote_code_is_frozen_for_batch_process(self):
        value = plan()
        with patch.object(Path, "read_text", side_effect=AssertionError("mid-batch read")):
            source = batch.remote_payload(value, value["candidates"][0])
        compile(source, "frozen-remote-batch", "exec")

    def test_observations_sanitize_and_bound(self):
        text = "secret password and personal path\n" + "\n".join(
            "ARCADE HIL STATE: seq=%d valid=true state=100 mode=1." % i for i in range(200))
        value = batch.observations(text)
        self.assertEqual(len(value["states"]), 128)
        self.assertNotIn("secret", json.dumps(value))

    def test_wrong_trick_not_promoted_and_reset_failure_continues(self):
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.return_value = {"states": [{"seq": 2, "valid": True, "state": 100, "mode": 1}], "buttons": []}
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["criteria"]["native_state_sequence"], "UNVERIFIED")
        self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")
        self.assertEqual(result["criteria"]["returned_to_idle"], "FAIL")
        self.assertEqual(env.integrity.call_count, 2)

    def test_named_flip_requires_exact_fresh_independent_identifier(self):
        observed = {"flip": {"samples": [{"ms": 12, "valid": True, "id": 2}], "dropped": 0}}
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("heelflip", observed, 100), "FAIL")
        self.assertEqual(batch.named_flip_result("double-kickflip", observed, 100), "UNVERIFIED")
        self.assertEqual(batch.named_flip_result("kickflip", observed, None), "UNVERIFIED")
        observed["evidence_gap"] = "tail_limit"
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")
        observed.pop("evidence_gap")
        observed["flip"]["dropped"] = 1
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")
        observed["flip"]["dropped"] = 0
        observed["flip"]["samples"][0]["valid"] = False
        self.assertEqual(batch.named_flip_result("kickflip", observed, 100), "UNVERIFIED")

    def test_impossible_requires_its_current_game_identifier_and_context(self):
        observed = {"flip": {"samples": [{"ms": 12, "valid": True, "id": 36}], "dropped": 0}}
        self.assertEqual(batch.named_flip_result("impossible", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("nollie-impossible", observed, 100), "FAIL")
        observed["flip"]["samples"][0]["id"] = 38
        self.assertEqual(batch.named_flip_result("nollie-impossible", observed, 100), "PASS")
        self.assertEqual(batch.named_flip_result("double-impossible", observed, 100), "UNVERIFIED")

    def test_combo_capture_is_bounded_and_not_named_from_state(self):
        text = "private unrelated log\n" + "\n".join(
            "ARCADE COMBO SAMPLE: ms=%d state=100 mode=1 words=%s." %
            (i, ",".join(["00000000"] * 8)) for i in range(200))
        value = batch.observations(text)
        self.assertEqual(len(value["combo"]["samples"]), 128)
        self.assertEqual(value["combo"]["native_event_identity"], "UNVERIFIED")
        self.assertNotIn("private", json.dumps(value))

    def test_combo_capture_excludes_previous_candidate(self):
        env = Mock()
        env.wait_idle.side_effect = [1, 3]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.side_effect = [
            {"combo": {"samples": [{"ms": 100}]}},
            {"states": [], "combo": {"samples": [{"ms": 99, "state": 100}, {"ms": 100, "state": 100}, {"ms": 101, "state": 100}]}}]
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["observations"]["combo"]["samples"], [{"ms": 101, "state": 100}])
        self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")

    def test_physics_sequence_can_prove_pop_missed_by_heartbeat(self):
        for dropped in (0, 1):
            env = Mock()
            env.wait_idle.side_effect = [1, 3]
            env.gesture.return_value = {"input_sent": True, "frames": []}
            env.observe.side_effect = [
                {"log_end": 100},
                {"states": [], "combo": {"dropped": dropped, "samples": [
                    {"ms": i, "state": state} for i, state in enumerate((100, 200, 100))]}}]
            value = plan()
            result = batch.perform_case(value, value["candidates"][0], env)
            self.assertEqual(result["criteria"]["native_state_sequence"], "PASS" if dropped == 0 else "UNVERIFIED")
            self.assertEqual(result["criteria"]["heartbeat_state_sequence"], "UNVERIFIED")
            self.assertEqual(result["criteria"]["named_trick"], "UNVERIFIED")

    def test_quiet_baseline_captures_new_physics_rows_by_byte_boundary(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "ReSkate.log"
            path.write_text("quiet heartbeat\n")
            env = batch.DeckEnvironment()
            env.log = path
            boundary = env.observe()["log_end"]
            with path.open("a") as stream:
                stream.write("ARCADE COMBO SAMPLE: ms=10 state=100 mode=1 words=" +
                             ",".join(["00000000"] * 8) + ".\n")
            result = env.observe(since=boundary)
            self.assertEqual(len(result["combo"]["samples"]), 1)
            self.assertIsNone(result["evidence_gap"])
            path.write_text("")
            self.assertEqual(env.observe(since=boundary)["evidence_gap"], "log_truncated")

    def test_invalid_context_sends_no_input(self):
        env = Mock()
        env.wait_idle.return_value = None
        value = plan()
        result = batch.perform_case(value, value["candidates"][0], env)
        self.assertEqual(result["outcome"], "SKIPPED_CONTEXT")
        env.gesture.assert_not_called()

    def run_fake(self, dispatch, check=lambda p: None):
        value = plan()
        value["candidates"][0]["repeat"] = 2
        with tempfile.TemporaryDirectory() as temp:
            with patch.object(robot, "save_run"):
                return batch.run_batch(value, Path(temp).resolve() / "out", dispatch, check)

    def test_failed_candidate_continues(self):
        dispatch = Mock(return_value={"input_sent": True, "neutralized": True, "outcome": "OBSERVED",
            "criteria": {"native_state_sequence": "FAIL"}, "frames": []})
        result = self.run_fake(dispatch)
        self.assertEqual(len(result["trials"]), 2)
        self.assertEqual(result["stop"], "candidates_exhausted")

    def test_integrity_failure_stops_before_input(self):
        dispatch = Mock()
        result = self.run_fake(dispatch, Mock(side_effect=batch.HardStop("wrong_hash")))
        dispatch.assert_not_called()
        self.assertEqual(result["stop"], "wrong_hash")

    def test_unconfirmed_neutralization_stops(self):
        dispatch = Mock(return_value={"input_sent": True, "outcome": "OBSERVED"})
        self.assertEqual(self.run_fake(dispatch)["stop"], "neutralization_unconfirmed")
        self.assertEqual(dispatch.call_count, 1)

    def test_lost_ack_never_replayed(self):
        dispatch = Mock(side_effect=subprocess.TimeoutExpired("ssh", 55))
        self.assertEqual(self.run_fake(dispatch)["stop"], "transport_or_evidence_integrity_unconfirmed")
        self.assertEqual(dispatch.call_count, 1)

    def test_interrupt_preserves_summary(self):
        result = self.run_fake(Mock(side_effect=KeyboardInterrupt))
        self.assertEqual(result["stop"], "transport_or_evidence_integrity_unconfirmed")

    def test_preflight_retries_reads_only(self):
        lab = Mock()
        lab.remote.side_effect = OSError("offline")
        with self.assertRaises(batch.HardStop):
            batch.preflight(plan(), lab, sleep=lambda _: None)
        self.assertEqual(lab.remote.call_count, 3)
        self.assertTrue(all(c.args == ("status",) for c in lab.remote.call_args_list))

    def test_wrong_hash_and_backup_refuse(self):
        value = plan()
        status = dict(safe_paths=True, active=True, updates=False, recovery_pending=False,
                      installed_commit=value["source_commit"], dll_sha256=value["dll_sha256"], backup_sha256=value["stock_sha256"])
        for key in ("dll_sha256", "backup_sha256", "installed_commit"):
            lab = Mock()
            lab.remote.return_value = dict(status, **{key: "0" * 64})
            with self.assertRaises(batch.HardStop):
                batch.preflight(value, lab)

    def test_no_restore_path(self):
        source = Path(batch.__file__).read_text()
        self.assertNotIn('remote("restore"', source)
        self.assertNotIn("change_installation", source)

    def test_scoped_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp).resolve()
            with self.assertRaises(ValueError):
                batch.scoped(root / "../escape", root, False)
            (root / "link").symlink_to(root.parent)
            with self.assertRaises((ValueError, RuntimeError)):
                batch.scoped(root / "link/escape", root, False)

    def test_batch_cleanup_uses_robot_finally_on_interrupt(self):
        pad = Mock()
        pad.__enter__ = Mock(return_value=pad)
        pad.__exit__ = Mock(return_value=False)
        with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [1]}), \
             patch.object(robot, "Gamepad", return_value=pad), patch.object(robot.time, "sleep"), \
             patch.object(robot, "screenshot"), patch.object(robot, "periodic", return_value=[]), \
             patch.object(robot, "replay", side_effect=KeyboardInterrupt), \
             patch.object(robot.signal, "signal"), patch.object(robot.signal, "alarm"):
            with self.assertRaises(KeyboardInterrupt):
                robot.experiment_events("test", [], 3)
        pad.neutral.assert_called()
        pad.__exit__.assert_called_once()


if __name__ == "__main__":
    unittest.main()
