"""No actual uinput, network, screenshots, or real HOME in these tests."""
import ast
import base64
import hashlib
import json
import os
from pathlib import Path
import signal
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lab import robot, controller


class RobotTests(unittest.TestCase):
    def pad(self):
        self.calls = []
        self.closed = []
        self.writes = []
        def write(fd, payload):
            self.writes.append(payload)
            return len(payload)
        return robot.Gamepad(open_fd=Mock(return_value=7),
                             ioctl=lambda *args: self.calls.append(args), write=write,
                             close=self.closed.append, lock=Mock(),
                             fstat=Mock(return_value=Mock(st_mode=stat.S_IFCHR)))

    def test_abi_setup_and_button_mapping(self):
        with self.pad() as pad:
            pad.frame([(1, robot.BUTTONS["X"], 1)])
        self.assertEqual(robot.EVENT.size, 24)
        setup = next(c[2] for c in self.calls if c[1] == robot.SETUP)
        self.assertEqual(len(setup), 92)
        self.assertEqual(struct.unpack("<HHHH", setup[:8]), (3, 0x45e, 0x28e, 0x0114))
        axes = [c[2] for c in self.calls if c[1] == robot.ABS_SETUP]
        self.assertEqual(len(axes), 8)
        self.assertTrue(all(len(a) == 28 for a in axes))
        self.assertEqual(robot.EVENT.unpack(self.writes[1][:24])[2:], (1, 0x133, 1))
        self.assertEqual(self.closed, [7])
        self.assertEqual(self.calls[-1][1], robot.DESTROY)

    def test_release_after_exception(self):
        with self.assertRaisesRegex(RuntimeError, "interrupted"):
            with self.pad() as pad:
                pad.frame([(1, robot.BUTTONS["A"], 1), (3, 4, 32767)])
                raise RuntimeError("interrupted")
        last = list(robot.EVENT.iter_unpack(self.writes[-1]))
        self.assertTrue(all(event[-1] == 0 for event in last))
        self.assertEqual(self.closed, [7])

    def test_kernel_observer_rejects_unrelated_or_path_shaped_identity(self):
        with self.pad() as pad:
            original = pad.ioctl
            for identity in (b'../event0', b'/dev/input/event0', b'event0', b'input1/../../event0'):
                def ioctl(fd, request, data=None, mutate=False):
                    if request == 0x8040552c:
                        data[:len(identity)] = identity
                    else:
                        return original(fd, request, data)
                pad.ioctl = ioctl
                pad.attach_observer()
                self.assertIsNone(pad.observer_fd)
                self.assertFalse(pad.kernel_pad['available'])
            pad.open_fd.assert_called_once()  # Only our /dev/uinput; no event device was opened.

    def test_kernel_trigger_observation_is_numeric_bounded_and_closed(self):
        with self.pad() as pad:
            original = pad.ioctl
            pad.observer_fd = 8
            pad.kernel_pad['available'] = True
            def ioctl(fd, request, data=None, mutate=False):
                if request in (0x80184542, 0x80184545):
                    data[:] = struct.pack('<iiiiii', 255 if request == 0x80184542 else 64, 0, 255, 0, 0, 0)
                else:
                    return original(fd, request, data)
            pad.ioctl = ioctl
            for _ in range(140):
                pad.observe()
            self.assertEqual(len(pad.kernel_pad['samples']), 128)
            row = pad.kernel_pad['samples'][0]
            self.assertEqual(set(row), {'ns', 'lt', 'rt'})
            self.assertEqual((row['lt'], row['rt']), (255, 64))
        self.assertEqual(self.closed, [8, 7])

    def test_trigger_plan_values_match_declared_device_range(self):
        with self.pad() as pad:
            pad.frame([(3, 2, 64), (3, 5, 255)])
            events = list(robot.EVENT.iter_unpack(self.writes[-1]))
            self.assertEqual([event[2:] for event in events[:2]], [(3, 2, 64), (3, 5, 255)])
            with self.assertRaises(ValueError):
                pad.frame([(3, 2, 256)])

    def test_kernel_observer_checks_owned_device_and_retries_hotplug_permission(self):
        with self.pad() as pad:
            original = pad.ioctl
            def ioctl(fd, request, data=None, mutate=False):
                if request == 0x8040552c:
                    data[:8] = b'input123'
                elif request == 0x80504506:
                    name = b'Microsoft X-Box 360 pad'
                    data[:len(name)] = name
                else:
                    return original(fd, request, data)
            pad.ioctl = ioctl
            pad.open_fd = Mock(side_effect=[PermissionError(13, 'pending'), 8])
            pad.fstat = Mock(return_value=Mock(st_mode=stat.S_IFCHR, st_rdev=os.makedev(13, 77)))
            with patch.object(Path, 'glob', return_value=[Path('/sys/devices/virtual/input/input123/event13')]), \
                 patch.object(Path, 'read_text', return_value='13:77'):
                pad.attach_observer()
                self.assertFalse(pad.kernel_pad['available'])
                self.assertEqual((pad.kernel_pad['stage'], pad.kernel_pad['errno']), (4, 13))
                pad.attach_observer()
            self.assertTrue(pad.kernel_pad['available'])
            self.assertEqual(pad.kernel_pad['errno'], 0)
            self.assertEqual([c.args[0] for c in pad.open_fd.call_args_list], ['/dev/input/event13'] * 2)
        self.assertEqual(self.closed, [8, 7])

    def test_kernel_observer_rejects_replaced_event_node(self):
        with self.pad() as pad:
            original = pad.ioctl
            def ioctl(fd, request, data=None, mutate=False):
                if request == 0x8040552c:
                    data[:8] = b'input123'
                else:
                    return original(fd, request, data)
            pad.ioctl = ioctl
            pad.open_fd = Mock(return_value=8)
            pad.fstat = Mock(return_value=Mock(st_mode=stat.S_IFCHR, st_rdev=os.makedev(13, 78)))
            with patch.object(Path, 'glob', return_value=[Path('/sys/devices/virtual/input/input123/event13')]), \
                 patch.object(Path, 'read_text', return_value='13:77'):
                pad.attach_observer()
            self.assertIsNone(pad.observer_fd)
            self.assertFalse(pad.kernel_pad['available'])
            self.assertEqual(self.closed, [8])

    def test_sdl_observer_opens_only_own_validated_event_path(self):
        sdl = Mock()
        sdl.SDL_InitSubSystem.return_value = 0
        sdl.SDL_NumJoysticks.return_value = 3
        sdl.SDL_JoystickPathForIndex.side_effect = [b'/dev/input/event1', b'/dev/input/event13', b'/dev/input/event2']
        sdl.SDL_IsGameController.return_value = 1
        sdl.SDL_GameControllerOpen.return_value = 123
        sdl.SDL_GameControllerGetAxis.side_effect = [32767, 0, 0, 0]
        with self.pad() as pad:
            pad.event_path, pad.observer_fd = '/dev/input/event13', 8
            with patch.object(robot.ctypes, 'CDLL', return_value=sdl) as library:
                pad.attach_sdl_observer()
            library.assert_called_once_with('/usr/lib/libSDL2-2.0.so.0')
            sdl.SDL_GameControllerOpen.assert_called_once_with(1)
            pad.observe_sdl()
            self.assertEqual(pad.sdl_observation['samples'][0]['lt'], 32767)
        sdl.SDL_GameControllerClose.assert_called_once_with(123)
        sdl.SDL_QuitSubSystem.assert_called_once_with(0x2000)

    def test_sdl_observer_refuses_other_paths(self):
        sdl = Mock()
        sdl.SDL_InitSubSystem.return_value = 0
        sdl.SDL_NumJoysticks.return_value = 1
        sdl.SDL_JoystickPathForIndex.return_value = b'/dev/input/event14'
        with self.pad() as pad:
            pad.event_path, pad.observer_fd = '/dev/input/event13', 8
            with patch.object(robot.ctypes, 'CDLL', return_value=sdl):
                pad.attach_sdl_observer()
            self.assertFalse(pad.sdl_observation['available'])
            sdl.SDL_GameControllerOpen.assert_not_called()

    def test_setup_failure_closes_device(self):
        pad = self.pad()
        pad.ioctl = Mock(side_effect=OSError("denied"))
        with self.assertRaises(OSError):
            with pad:
                self.fail("unreachable")
        self.assertEqual(self.closed, [7])

    def test_lock_conflict_does_not_create_device(self):
        pad = self.pad()
        pad.lock = Mock(side_effect=BlockingIOError())
        with self.assertRaises(BlockingIOError):
            with pad:
                self.fail("unreachable")
        self.assertEqual(self.calls, [])
        self.assertEqual(self.closed, [7])

    def test_cleanup_write_failure_still_destroys(self):
        with self.assertRaises(OSError):
            with self.pad() as pad:
                pad.write = Mock(side_effect=OSError())
        self.assertEqual(self.calls[-1][1], robot.DESTROY)
        self.assertEqual(self.closed, [7])

    def test_regular_file_and_invalid_events_refused(self):
        pad = self.pad()
        pad.fstat = Mock(return_value=Mock(st_mode=stat.S_IFREG))
        with self.assertRaises(RuntimeError):
            pad.__enter__()
        self.assertFalse(self.calls)
        with self.pad() as pad:
            for event in ((1, 9999, 1), (3, 4, 40000), (2, 0, 1)):
                with self.assertRaises(ValueError):
                    pad.frame([event])

    def test_short_write_refused(self):
        with self.pad() as pad:
            original = pad.write
            pad.write = Mock(return_value=1)
            with self.assertRaises(RuntimeError):
                pad.frame([(1, 0x130, 1)])
            pad.write = original

    def test_replay_holds_once_and_stops_on_exit(self):
        now = [0.0]
        events, duration = robot.recipe("observer")
        pad = Mock()
        robot.replay(pad, events, duration, lambda: True,
                     sleep=lambda sec: now.__setitem__(0, now[0] + sec), clock=lambda: now[0])
        self.assertEqual(pad.frame.call_count, 8)
        self.assertEqual(now[0], 16)
        with self.assertRaises(RuntimeError):
            robot.replay(pad, events, duration, lambda: False, clock=lambda: 0)
        self.assertEqual(pad.frame.call_count, 8)

    def test_fixed_recipes_and_preflight_refusal(self):
        with patch.object(robot, "capabilities", return_value={"ready": False}), patch.object(robot, "Gamepad") as pad:
            with self.assertRaises(RuntimeError):
                robot.experiment("idle")
            pad.assert_not_called()
        for name in ("idle", "stick-ollie", "observer", "exit-hub", "mount-board",
                     "candidate-flip-left", "candidate-flip-right"):
            events, duration = robot.recipe(name)
            self.assertLessEqual(duration, 16)
            self.assertEqual(events, sorted(events, key=lambda event: event[0]))
        left, duration = robot.recipe("candidate-flip-left")
        right, other_duration = robot.recipe("candidate-flip-right")
        self.assertEqual((duration, other_duration), (6, 6))
        self.assertEqual(left[1][1], [(3, 3, -28000), (3, 4, -28000)])
        self.assertEqual(right[1][1], [(3, 3, 28000), (3, 4, -28000)])
        self.assertEqual(robot.recipe("exit-hub")[0][0][1], [(1, robot.BUTTONS["B"], 1)])
        self.assertEqual(robot.recipe("mount-board")[0][0][1], [(1, robot.BUTTONS["Y"], 1)])
        self.assertEqual(robot.recipe("open-menu")[0][0][1], [(1, 0x13b, 1)])
        self.assertEqual(robot.recipe("candidate-side-left")[0][1][1], [(3, 3, -32768), (3, 4, 0)])
        self.assertEqual(robot.recipe("candidate-side-right")[0][1][1], [(3, 3, 32767), (3, 4, 0)])
        self.assertEqual(robot.recipe("candidate-arc-left")[0][1][1], [(3, 3, -23000), (3, 4, 23000)])
        self.assertEqual(robot.recipe("candidate-arc-right")[0][1][1], [(3, 3, 23000), (3, 4, 23000)])
        for direction in ("left", "right"):
            events, duration = robot.recipe("candidate-half-" + direction)
            self.assertEqual((len(events), duration), (6, 6))
            self.assertEqual(events[-1][1], [(3, 4, 0)])
        self.assertEqual(robot.recipe("candidate-scoop-left")[0][1][1], [(3, 3, -28000), (3, 4, 14000)])
        self.assertEqual(robot.recipe("candidate-scoop-right")[0][1][1], [(3, 3, 28000), (3, 4, 14000)])
        with self.assertRaises(ValueError):
            robot.recipe("arbitrary-command")

    def test_experiment_orchestration_and_alarm_cleanup(self):
        ready = {"ready": True, "gamescope_pids": [12], "skate_running": True}
        pad = self.pad()
        with patch.object(robot, "capabilities", return_value=ready), \
             patch.object(robot, "Gamepad", return_value=pad), \
             patch.object(robot.time, "sleep"), \
             patch.object(robot, "screenshot", return_value=b"png") as screenshot, \
             patch.object(robot, "periodic", return_value=[{"test": True}] * 4), \
             patch.object(robot, "replay") as replay, \
             patch.object(robot.signal, "alarm") as alarm, \
             patch.object(robot.signal, "signal") as handlers:
            result = robot.experiment("idle")
        self.assertTrue(result["input_sent"])
        self.assertEqual(result["game_response"], "UNVERIFIED")
        self.assertEqual(len(result["frames"]), 4)
        screenshot.assert_called_once_with(12)
        replay.assert_called_once()
        self.assertEqual([c.args[0] for c in alarm.call_args_list], [25, 0])
        self.assertEqual(handlers.call_count, 8)
        self.assertEqual(self.closed, [7])

    def test_capture_failure_prevents_gameplay_and_cleans_up(self):
        with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [12]}), \
             patch.object(robot, "Gamepad", return_value=self.pad()), \
             patch.object(robot.time, "sleep"), \
             patch.object(robot, "screenshot", side_effect=RuntimeError("capture unavailable")), \
             patch.object(robot, "replay") as replay, \
             patch.object(robot.signal, "alarm"), patch.object(robot.signal, "signal"):
            with self.assertRaisesRegex(RuntimeError, "capture unavailable"):
                robot.experiment("observer")
        replay.assert_not_called()
        self.assertEqual(self.closed, [7])
        self.assertTrue(all(event[-1] == 0 for payload in self.writes for event in robot.EVENT.iter_unpack(payload)))

    def test_failed_replay_cancels_capture_before_join(self):
        sequence = []
        future = Mock()
        pad = Mock()
        pad.__enter__ = Mock(return_value=pad)
        pad.__exit__ = Mock(return_value=False)
        pad.neutral.side_effect = lambda: sequence.append("neutral")
        pool = Mock()
        pool.__enter__ = Mock(return_value=pool)
        pool.__exit__ = Mock(side_effect=lambda *args: sequence.append("join"))
        pool.submit.return_value = future
        with patch.object(robot, "capabilities", return_value={"ready": True, "gamescope_pids": [12]}), \
             patch.object(robot, "Gamepad", return_value=pad), patch.object(robot.time, "sleep"), \
             patch.object(robot, "screenshot"), patch.object(robot, "replay", side_effect=RuntimeError("exit")), \
             patch.object(robot.concurrent.futures, "ThreadPoolExecutor", return_value=pool), \
             patch.object(robot.signal, "alarm"), patch.object(robot.signal, "signal"):
            with self.assertRaises(RuntimeError):
                robot.experiment("observer")
        self.assertEqual(sequence, ["neutral", "join"])
        self.assertTrue(pool.submit.call_args.args[-1].is_set())

    def test_cancelled_periodic_capture_sends_no_signal(self):
        stop = robot.threading.Event()
        stop.set()
        with patch.object(robot, "screenshot") as shot:
            self.assertEqual(robot.periodic(12, 16, stop), [])
        shot.assert_not_called()

    def test_capabilities_fake_proc_handler_and_ambiguity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            def process(pid, comm, argv, handler=0):
                folder = root / str(pid)
                folder.mkdir()
                (folder / "comm").write_text(comm)
                (folder / "cmdline").write_bytes(argv)
                (folder / "status").write_text("SigCgt:\t%x\n" % handler)
            process(1, "gamescope", b"gamescope\0", 1 << (signal.SIGUSR2 - 1))
            process(2, "Skate.exe", b"Z:\\game\\Skate.exe\0")
            with patch.object(robot.sys, "platform", "linux"), \
                 patch.object(robot.platform, "machine", return_value="x86_64"), \
                 patch.object(robot.os, "pidfd_open", create=True), \
                 patch.object(robot.signal, "pidfd_send_signal", create=True):
                self.assertTrue(robot.capabilities(root, Path("/dev/null"))["ready"])
                (root / "1/comm").write_text("gamescope-wl")
                self.assertTrue(robot.capabilities(root, Path("/dev/null"))["ready"])
                (root / "1/cmdline").write_bytes(b"unrelated\0")
                self.assertFalse(robot.capabilities(root, Path("/dev/null"))["ready"])
                (root / "1/cmdline").write_bytes(b"/usr/bin/gamescope\0")
                (root / "1/status").write_text("SigCgt:\t0\n")
                self.assertFalse(robot.capabilities(root, Path("/dev/null"))["ready"])
                (root / "1/status").write_text("SigCgt:\t%x\n" % (1 << (signal.SIGUSR2 - 1)))
                process(3, "gamescope", b"gamescope\0", 1 << (signal.SIGUSR2 - 1))
                self.assertFalse(robot.capabilities(root, Path("/dev/null"))["ready"])

    def test_preflight_serialization_is_read_only(self):
        source = robot.remote_source("preflight", "idle")
        module = ast.parse(source)
        function = next(n for n in module.body if isinstance(n, ast.FunctionDef) and n.name == "capabilities")
        forbidden = {"open", "open_fd", "write", "write_text", "mkdir", "kill", "pidfd_send_signal", "ioctl", "unlink"}
        calls = [n.func.attr if isinstance(n.func, ast.Attribute) else n.func.id
                 for n in ast.walk(function) if isinstance(n, ast.Call) and isinstance(n.func, (ast.Attribute, ast.Name))]
        self.assertFalse(forbidden.intersection(calls))
        result = subprocess.run([sys.executable, "-B", "-c", source], capture_output=True, text=True, check=True)
        self.assertIn("ready", json.loads(result.stdout))
        self.assertTrue(source.endswith("print(json.dumps(capabilities()))\n"))
        args = controller.ssh_arguments("deck@steamdeck.local", Path("/fake"))
        self.assertIn("StrictHostKeyChecking=yes", args)
        self.assertIn("BatchMode=yes", args)

    def test_png_symlink_stale_and_size_refusal(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "gamescope.png"
            path.write_bytes(b"not a png" * 8)
            with self.assertRaises(ValueError):
                robot.read_image(path)
            link = Path(temp) / "gamescope-link.png"
            link.symlink_to(path)
            with self.assertRaises(OSError):
                robot.read_image(link)

    def test_screenshot_refuses_changed_process(self):
        with patch.object(robot.os, "pidfd_open", create=True, return_value=7), \
             patch.object(robot.os, "close") as close, \
             patch.object(robot, "capabilities", return_value={"gamescope_pids": [99], "skate_running": True}), \
             patch.object(robot.signal, "pidfd_send_signal", create=True) as send:
            with self.assertRaises(RuntimeError):
                robot.screenshot(12)
            send.assert_not_called()
            close.assert_called_once_with(7)

    def test_capture_without_game_never_creates_controller(self):
        with patch.object(robot, "capabilities", return_value={"gamescope_pids": [12], "skate_running": False}), \
             patch.object(robot, "Gamepad") as pad, patch.object(robot, "screenshot", return_value=("png", b"png")):
            result = robot.capture()
        pad.assert_not_called()
        self.assertFalse(result["input_sent"])
        self.assertEqual(len(result["frames"]), 1)
        self.assertEqual(result["recipe"], "capture")

    def test_screenshot_rejects_old_accepts_fresh(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "gamescope_test.png"
            png = b"\x89PNG\r\n\x1a\n" + b"0" * 20 + b"\0\0\0\0IEND\xaeB`\x82"
            path.write_bytes(png)
            now = [0.0]
            def sleep(sec):
                now[0] += sec
            with patch.object(robot.os, "pidfd_open", create=True, return_value=7), \
                 patch.object(robot.os, "close"), \
                 patch.object(robot, "capabilities", return_value={"gamescope_pids": [12], "skate_running": True}), \
                 patch.object(robot.signal, "pidfd_send_signal", create=True) as send:
                with self.assertRaises(RuntimeError):
                    robot.screenshot(12, Path(temp), sleep, lambda: now[0])
                send.assert_called_once_with(7, signal.SIGUSR2)
                stamp = path.stat().st_mtime_ns
                send.side_effect = lambda *unused: os.utime(path, ns=(stamp + 1, stamp + 1))
                self.assertEqual(robot.screenshot(12, Path(temp), sleep, lambda: now[0]), ("png", png))

    def test_local_frames_hash_verified_no_publish(self):
        png = b"\x89PNG\r\n\x1a\n" + b"0" * 20 + b"\0\0\0\0IEND\xaeB`\x82"
        def result():
            return {"schema": 1, "frames": [{"image_base64": base64.b64encode(png).decode(), "format": "png",
                    "sha256": hashlib.sha256(png).hexdigest()} for _ in range(4)]}
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp).resolve() / "run"
            robot.save_run(result(), output)
            self.assertEqual(len(list(output.glob("*.png"))), 4)
            self.assertNotIn('"image_base64"', (output / "run.json").read_text())
            with self.assertRaises(FileExistsError):
                robot.save_run(result(), output)
            broken = result()
            broken["frames"][0]["sha256"] = "0" * 64
            with self.assertRaises(ValueError):
                robot.save_run(broken, Path(temp).resolve() / "bad")
            self.assertFalse((Path(temp) / "bad").exists())

    def test_avif_complete_boxes_and_truncation(self):
        avif = struct.pack(">I4s4sI4s", 20, b"ftyp", b"avif", 0, b"avif")
        avif += struct.pack(">I4sI", 12, b"meta", 0) + struct.pack(">I4sI", 12, b"mdat", 0)
        self.assertEqual(robot.image_format(avif), "avif")
        for bad in (avif[:-1], avif.replace(b"avif", b"junk"), b"random"):
            with self.assertRaises(ValueError):
                robot.image_format(bad)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "gamescope.avif"
            path.write_bytes(avif)
            self.assertEqual(robot.read_image(path), ("avif", avif))


if __name__ == "__main__":
    unittest.main()
