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
    def test_combo_chain_requires_one_lifecycle_and_native_ids_at_each_takeoff(self):
        expected = {'poses': [2, 3, 2, 4, 2, 1], 'air_ids': [1, 1, 1]}
        events = []
        for i, (ms, pose) in enumerate([(0, 2), (1000, 3), (1500, 2), (2300, 4),
                                        (2800, 1), (3200, 2), (4000, 1), (4450, 1)]):
            active = i < 7
            events.append(dict(ms=ms, pose=pose, active=active, failed=False,
                               fatigue=i * .1 if active else 0,
                               grind_fatigue=.05 if active and i >= 2 else 0,
                               manual_fatigue=.04 if active and i >= 4 else 0))
        observation = {'combo': {'events': events, 'dropped': 0, 'baseline_available': True},
                       'flip': {'samples': [dict(ms=ms, id=1, valid=True) for ms in (0, 1500, 3200)], 'dropped': 0},
                       'grind_result': {'evidence': 'COMPLETE'}}
        result = batch.combo_chain_result(expected, observation)
        self.assertTrue(all(result[k] == 'PASS' for k in ('continuous', 'native_air_ids', 'fatigue', 'lifecycle_grace')))
        self.assertEqual(result['score_banking'], 'UNVERIFIED')
        missing = copy.deepcopy(observation)
        missing['flip']['samples'].pop()
        self.assertEqual(batch.combo_chain_result(expected, missing)['native_air_ids'], 'UNVERIFIED')
        restarted = copy.deepcopy(observation)
        restarted['combo']['events'][4]['active'] = False
        self.assertEqual(batch.combo_chain_result(expected, restarted)['continuous'], 'FAIL')
        silent_restart = copy.deepcopy(observation)
        silent_restart['combo']['events'][4]['grind_fatigue'] = 0
        self.assertEqual(batch.combo_chain_result(expected, silent_restart)['continuous'], 'FAIL')
        bounce = copy.deepcopy(observation)
        bounce['combo']['events'][4]['ms'] = 2317
        self.assertEqual(batch.combo_chain_result(expected, bounce)['continuous'], 'FAIL')
        lost = copy.deepcopy(observation)
        lost['grind_result']['evidence'] = 'INCOMPLETE'
        self.assertEqual(batch.combo_chain_result(expected, lost)['continuous'], 'UNVERIFIED')
        early_bank = copy.deepcopy(observation)
        early_bank['combo']['events'][-1]['ms'] = 4100
        self.assertEqual(batch.combo_chain_result(expected, early_bank)['lifecycle_grace'], 'FAIL')
        value = plan()
        value['candidates'][0]['expected']['chain'] = expected
        batch.validate_plan(value)
        for bad in ({'poses': [True, 1], 'air_ids': []}, {'poses': [2, 1], 'air_ids': []},
                    {'poses': [2, 1], 'air_ids': [64]}, {'poses': [2, 1], 'air_ids': [1], 'grace_ms': [650, 350]}):
            value['candidates'][0]['expected']['chain'] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_fixture_baseline_reuses_only_fresh_confirmed_reset(self):
        env = batch.DeckEnvironment(Mock())
        ready = dict(reset_ready=True, present=True, failed=False, editor_open=False)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'fixture': ready}])
        with patch.object(batch, 'fixture_console') as console, patch.object(env, 'fixture_reset') as reset:
            self.assertEqual(env.fixture_baseline(1000), ready)
            console.assert_called_once_with('status')
            reset.assert_not_called()
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        env.observe = Mock(side_effect=[{'log_end': 100}, {'fixture': dict(ready, reset_ready=False)}])
        with patch.object(batch, 'fixture_console'), patch.object(env, 'fixture_reset', return_value=ready) as reset:
            self.assertEqual(env.fixture_baseline(1000), ready)
            reset.assert_called_once_with(1000)
        for observation in ({}, {'fixture': ready, 'evidence_gap': 'log_truncated'},
                            {'fixture': dict(ready, present=False)}, {'fixture': dict(ready, editor_open=True)}):
            env.observe = Mock(side_effect=[{'log_end': 100}, observation])
            with patch.object(batch, 'fixture_console'), patch.object(env, 'fixture_reset') as reset, \
                    self.assertRaises(batch.HardStop):
                env.fixture_baseline(1000)
            reset.assert_not_called()

    def test_fixture_trials_continue_misses_but_stop_lost_evidence_or_reset(self):
        value = plan()
        value['candidates'][0].update(fixture_reset=True, repeat=3)
        valid = dict(input_sent=True, fixture_reset={'reset_ready': True},
                     observations={'grind_result': {'evidence': 'COMPLETE', 'native_grind': 'FAIL'}})
        with patch.object(batch, 'perform_case', side_effect=lambda *_: copy.deepcopy(valid)) as run:
            self.assertEqual(len(batch.perform_batch(value, object())['trials']), 3)
            self.assertEqual(run.call_count, 3)
        for failed, reason in [(dict(valid, fixture_reset={'reset_ready': False}), 'fixture_reset_unverified'),
                               (dict(valid, observations={}), 'grind_evidence_incomplete')]:
            with patch.object(batch, 'perform_case', return_value=failed) as run:
                results = batch.perform_batch(value, object())['trials']
            self.assertEqual(run.call_count, 1)
            self.assertEqual(results[-1]['reason'], reason)
            self.assertIn('observations', results[0])

    def test_fixture_reset_neutralizes_and_requires_fresh_native_confirmation(self):
        session = Mock()
        env = batch.DeckEnvironment(session)
        status = dict(reset_ready=True, present=True, failed=False, editor_open=False)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'log_end': 200, 'fixture': status}])
        with patch.object(batch, 'fixture_console') as console:
            self.assertEqual(env.fixture_reset(1000), status)
        session.pad.neutral.assert_called_once()
        self.assertEqual([c.args for c in console.call_args_list], [('reset',), ('status',)])
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        env.observe = Mock(side_effect=[{'log_end': 200}, {'fixture': dict(status, reset_ready=False, present=False)}])
        with patch.object(batch, 'fixture_console'), self.assertRaises(batch.HardStop):
            env.fixture_reset(1000)

    def test_fixture_console_rejects_plan_commands_without_running_tools(self):
        with patch.object(robot.subprocess, 'run') as run:
            for command in ('place', 'save', 'reset; exit', 'status\nother'):
                with self.assertRaises(ValueError):
                    robot.fixture_console(command)
            run.assert_not_called()

    def test_fixture_native_remount_is_contextual_bounded_and_always_neutralized(self):
        status = dict(arcade=False, configured=True, present=True, busy=False, failed=False,
                      editor_open=False, skater=True, state=504, speed=0, heading=0,
                      position=[1, 2, 3], start=[1, 2, 3])
        session = Mock(pid=123)
        env = batch.DeckEnvironment(session)
        with patch.object(batch, 'capabilities', return_value=dict(ready=True, gamescope_pids=[123]), create=True), \
                patch.object(batch.time, 'sleep'):
            for arcade, keys in ((False, [0x134]), (True, [0x136, 0x137])):
                env.fixture_mount(dict(status, arcade=arcade))
                session.pad.frame.assert_called_with([(1, key, 1) for key in keys])
            for change in ({'state': 100}, {'position': [4, 2, 3]}, {'heading': 90},
                           {'arcade': None}, {'editor_open': True}, {'speed': 1}):
                session.pad.frame.reset_mock()
                with self.assertRaises(batch.HardStop):
                    env.fixture_mount(dict(status, **change))
                session.pad.frame.assert_not_called()
            session.pad.frame.side_effect = OSError('interrupted input')
            session.pad.neutral.reset_mock()
            with self.assertRaises(OSError):
                env.fixture_mount(status)
            session.pad.neutral.assert_called_once()

    def test_fixture_remount_waits_for_new_native_readiness_without_repeated_presses(self):
        session = Mock()
        env = batch.DeckEnvironment(session)
        foot = dict(reset_ready=False, present=True, failed=False, editor_open=False, state=504)
        riding = dict(foot, state=100, reset_ready=True)
        env.observe = Mock(side_effect=[{'log_end': 100}, {'log_end': 200, 'fixture': foot},
                                       {'log_end': 300, 'fixture': foot}, {'log_end': 400, 'fixture': riding}])
        with patch.object(batch, 'fixture_console'), patch.object(batch.time, 'sleep'), \
                patch.object(env, 'fixture_mount') as mount:
            self.assertEqual(env.fixture_reset(1000), riding)
            mount.assert_called_once_with(foot)

    def test_fixture_readiness_accepts_coalesced_health_only_with_matching_fresh_rows(self):
        status = dict(generation=8, active=True, dropped=0, rejected=0)
        row = dict(generation=8, dropped=0, valid=True, state=100, edge=0, duration=0)
        fresh = dict(log_end=200, grind=dict(samples=[dict(row) for _ in range(3)],
                     pad=[dict(generation=8, dropped=0)], status=[], dropped=0))
        env = batch.DeckEnvironment(Mock())
        env.observe = Mock(side_effect=[dict(log_end=100, grind={'status': [status]}), fresh])
        self.assertEqual(env.wait_grind_ready(1000)['log_end'], 200)
        self.assertEqual(env.observe.call_args.kwargs, {'since': 100})
        for change in ({'generation': 9}, {'dropped': 1}, {'active': False}):
            old = dict(status, **change)
            env.observe = Mock(side_effect=[dict(log_end=100, grind={'status': [old]}),
                                           dict(fresh, grind=dict(fresh['grind'], status=[]))])
            with patch.object(batch.time, 'monotonic', side_effect=[0, 0, 2]), patch.object(batch.time, 'sleep'), \
                 self.assertRaises(batch.HardStop):
                env.wait_grind_ready(1000)

    def test_hard_stop_reports_only_allowlisted_cause_and_trial(self):
        value = plan()
        for message, reason in [("Skate exited; test stopped", "input_readiness_lost"),
                                ("No fresh Gamescope PNG/AVIF; capture interface unverified", "capture_timeout"),
                                ("private path and arbitrary output", "integrity_or_transport_failure")]:
            with patch.object(batch, "perform_case", side_effect=RuntimeError(message)):
                result = batch.perform_batch(value, object())["trials"]
            self.assertEqual(result, [dict(hard_stop=True, reason=reason, name="kickflip", repetition=1)])

    def test_readiness_failure_diagnostic_is_numeric_and_bounded(self):
        diagnostic = dict(age_ms=121, failed=False, context_ready=True,
                          owner_matches=True, capture_pending=True)
        for supplied, accepted in [(diagnostic, True), (dict(diagnostic, age_ms=100001), False),
                                   (dict(diagnostic, path='private'), False),
                                   (dict(diagnostic, failed='private'), False)]:
            error = RuntimeError('Skate exited; test stopped')
            error.readiness = supplied
            with patch.object(batch, 'perform_case', side_effect=error):
                stopped = batch.perform_batch(plan(), object())['trials'][0]
            self.assertEqual('readiness' in stopped, accepted)
            if accepted:
                self.assertEqual(stopped['readiness'], diagnostic)

    def test_stationary_baseline_requires_matching_fresh_speed(self):
        observation = {"states": [dict(seq=i, valid=True, state=100, mode=1) for i in range(1, 4)],
                       "push": [dict(seq=i, valid=True, speed=0.1, contacts=0xc000) for i in range(1, 4)]}
        self.assertEqual(batch.ready_baseline(observation, 0, 100), 3)
        self.assertIsNone(batch.ready_baseline(observation, 1, 100))
        for change in ({"speed": 2.38}, {"speed": float("nan")}, {"speed": -1},
                       {"seq": 0}, {"valid": False}, {"contacts": 0x4000}):
            bad = copy.deepcopy(observation)
            bad["push"][1].update(change)
            self.assertIsNone(batch.ready_baseline(bad, 0, 100))
            self.assertEqual(batch.ready_baseline(bad, 0), 3)
        observation.pop("push")
        self.assertIsNone(batch.ready_baseline(observation, 0, 100))

    def test_speed_bound_is_validated_and_applies_to_reset(self):
        value = plan()
        case = value["candidates"][0]
        for bound in (0, 100, 20000):
            case["baseline_max_speed_mm_s"] = bound
            batch.validate_plan(value)
        for bound in (-1, 20001, True, "100", 0.1):
            case["baseline_max_speed_mm_s"] = bound
            with self.assertRaises(ValueError):
                batch.validate_plan(value)
        case["baseline_max_speed_mm_s"] = 100
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {"input_sent": True, "frames": []}
        env.observe.return_value = {"states": [], "buttons": []}
        result = batch.perform_case(value, case, env)
        self.assertEqual(result["criteria"]["returned_to_idle"], "FAIL")
        self.assertTrue(all(call.kwargs == {"max_speed_mm_s": 100} for call in env.wait_idle.call_args_list))
        env.reset_mock()
        env.wait_idle.side_effect = [None]
        result = batch.perform_case(value, case, env)
        self.assertEqual(result["outcome"], "SKIPPED_CONTEXT")
        env.gesture.assert_not_called()

    def test_fixture_shortens_only_pre_reset_idle_wait(self):
        value = plan()
        case = value['candidates'][0]
        case.update(fixture_reset=True, reset_ms=10000, baseline_max_speed_mm_s=100)
        env = Mock()
        env.wait_idle.side_effect = [1, None]
        env.gesture.return_value = {'input_sent': True, 'frames': []}
        env.observe.return_value = {'states': [], 'buttons': [], 'combo': {'samples': []}}
        env.wait_grind_ready.return_value = {'log_end': 100}
        env.fixture_reset.return_value = {'reset_ready': True}
        result = batch.perform_case(value, case, env)
        self.assertEqual([c.args for c in env.wait_idle.call_args_list], [(10000,), (2000,)])
        self.assertTrue(all(c.kwargs == {'max_speed_mm_s': 100} for c in env.wait_idle.call_args_list))
        env.fixture_baseline.assert_called_once_with(10000)
        env.wait_grind_ready.assert_called_once_with(10000)
        env.fixture_reset.assert_called_once_with(10000)
        self.assertEqual(result['criteria']['returned_to_idle'], 'FAIL')
        self.assertTrue(result['fixture_reset']['reset_ready'])

    def test_bounded_capture_policy_and_session_forwarding(self):
        value = plan()
        case = value["candidates"][0]
        for count in (0, 1, 4):
            case["capture_frames"] = count
            batch.validate_plan(value)
            session = Mock()
            batch.DeckEnvironment(session=session).gesture(case)
            self.assertEqual(session.run.call_args.kwargs["capture_frames"], count)
        for count in (2, 3, 5, True, "1"):
            case["capture_frames"] = count
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

    def test_failure_window_retains_more_than_old_tail_and_still_marks_overflow(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "ReSkate.log"
            marker = "ARCADE HIL STATE: seq=1 valid=true state=100 mode=1.\n"
            path.write_text(marker + "x" * 70000)
            env = batch.DeckEnvironment()
            env.log = path
            result = env.observe(since=0)
            self.assertEqual(len(result["states"]), 1)
            self.assertIsNone(result["evidence_gap"])
            path.write_text(marker + "x" * 270000)
            self.assertEqual(env.observe(since=0)["evidence_gap"], "tail_limit")

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
        session.run.assert_called_once_with(case["name"], batch.phase_events(case), case["timeout_ms"] / 1000, capture_frames=4)

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

    def test_exit_identity_cannot_be_satisfied_by_the_approach_ollie(self):
        observed = {'flip': {'samples': [{'ms': 12, 'valid': True, 'id': 1}], 'dropped': 0},
                    'grind_result': {'evidence': 'COMPLETE', 'native_grind': 'PASS'},
                    'grind': {'samples': [{'ms': 20, 'valid': True, 'edge': 1},
                                          {'ms': 30, 'valid': True, 'edge': 2}]}}
        self.assertEqual(batch.named_flip_result('ollie', observed, 100, True), 'UNVERIFIED')
        observed['flip']['samples'].append({'ms': 28, 'valid': True, 'id': 2})
        self.assertEqual(batch.named_flip_result('ollie', observed, 100, True), 'FAIL')
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'PASS')
        observed['grind']['samples'].append({'ms': 40, 'valid': True, 'edge': 1})
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'UNVERIFIED')
        observed['grind_result']['evidence'] = 'INCOMPLETE'
        self.assertEqual(batch.named_flip_result('kickflip', observed, 100, True), 'UNVERIFIED')
        value = plan()
        value['candidates'][0]['expected']['after_grind'] = True
        batch.validate_plan(value)
        for bad in (1, 'true', None):
            value['candidates'][0]['expected']['after_grind'] = bad
            with self.assertRaises(ValueError):
                batch.validate_plan(value)

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
