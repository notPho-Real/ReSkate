"""Bounded SSH gamepad experiments. Preflight is read-only; run is explicit.

No dependencies, sudo, game writes, deployment, persistent daemon, or publication.
The remote worker is sent in memory. Linux x86_64 uinput ABI is deliberate.
"""
import argparse
import base64
import concurrent.futures
import copy
import ctypes
import fcntl
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import shlex
import signal
import stat
import struct
import subprocess
import sys
import threading
import time

BUTTONS = {"A": 0x130, "B": 0x131, "X": 0x133, "Y": 0x134}
# Linux BTN_X/BTN_Y aliases are 0x133/0x134 (Xbox labels).
AXES = {0: (-32768, 32767), 1: (-32768, 32767), 2: (0, 255),
        3: (-32768, 32767), 4: (-32768, 32767), 5: (0, 255),
        16: (-1, 1), 17: (-1, 1)}
KEYS = tuple(BUTTONS.values()) + (0x136, 0x137, 0x13a, 0x13b, 0x13c, 0x13d, 0x13e)
CREATE, DESTROY = 0x5501, 0x5502
SETUP, ABS_SETUP = 0x405c5503, 0x401c5504
EVBIT, KEYBIT = 0x40045564, 0x40045565
EVENT = struct.Struct("<qqHHi")
MAX_IMAGE = 8 * 1024 * 1024


def capabilities(proc=Path("/proc"), device=Path("/dev/uinput")):
    """Only reads: no open of uinput, signal, directory creation or game access."""
    gamescope, game = [], []
    for path in proc.glob("[0-9]*"):
        try:
            if path.stat().st_uid != os.getuid():
                continue
            comm = (path / "comm").read_text().strip()
            argv = (path / "cmdline").read_bytes().split(b"\0")
            # /proc/PID/exe is EACCES on the observed SteamOS compositor. Use
            # comm + argv0 + UID + caught handler; never change permissions.
            if comm in ("gamescope", "gamescope-wl") and argv[0].rsplit(b"/", 1)[-1] == b"gamescope":
                status = (path / "status").read_text().splitlines()
                caught = next(int(line.split()[1], 16) for line in status if line.startswith("SigCgt:"))
                if caught & (1 << (signal.SIGUSR2 - 1)):
                    gamescope.append(int(path.name))
            if any(arg.replace(b"\\", b"/").rsplit(b"/", 1)[-1].lower() == b"skate.exe" for arg in argv):
                game.append(int(path.name))
        except FileNotFoundError:
            continue
        except (OSError, ValueError, StopIteration):
            return {"ready": False, "reason": "Process inspection incomplete"}
    try:
        mode = device.lstat().st_mode
        writable = stat.S_ISCHR(mode) and os.access(device, os.W_OK)
    except FileNotFoundError:
        writable = False
    supported = sys.platform == "linux" and platform.machine() == "x86_64"
    pidfd = hasattr(os, "pidfd_open") and hasattr(signal, "pidfd_send_signal")
    return {"ready": supported and writable and pidfd and len(gamescope) == 1 and bool(game),
            "linux_x64": supported, "uinput_writable": writable, "pidfd_available": pidfd,
            "gamescope_pids": gamescope, "skate_running": bool(game)}


class Gamepad:
    def __init__(self, open_fd=os.open, ioctl=fcntl.ioctl, write=os.write,
                 close=os.close, lock=fcntl.flock, fstat=os.fstat, observe_kernel=False):
        self.open_fd, self.ioctl, self.write, self.close_fd = open_fd, ioctl, write, close
        self.lock, self.fstat, self.fd = lock, fstat, None
        self.observe_kernel, self.observer_fd = observe_kernel, None
        self.kernel_pad = {'available': False, 'stage': 0, 'errno': 0, 'samples': []}
        self.event_path, self.sdl, self.sdl_pad = None, None, None
        self.sdl_observation = {'available': False, 'stage': 0, 'samples': []}

    def __enter__(self):
        self.fd = self.open_fd("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK | os.O_NOFOLLOW | os.O_CLOEXEC)
        try:
            if not stat.S_ISCHR(self.fstat(self.fd).st_mode):
                raise RuntimeError("uinput is not a character device")
            # Cooperating lab runners serialize on the device inode, without a lock file.
            self.lock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            for kind in (1, 3):
                self.ioctl(self.fd, EVBIT, kind)
            for key in KEYS:
                self.ioctl(self.fd, KEYBIT, key)
            for axis, (low, high) in AXES.items():
                self.ioctl(self.fd, ABS_SETUP, struct.pack("<H2xiiiiii", axis, 0, low, high, 0, 0, 0))
            # Match the standard Linux controller GUID, including its version.
            # SDL's mapping assigns trigger axes 2/5 to this 0x0114 identity.
            self.ioctl(self.fd, SETUP, struct.pack("<HHHH80sI", 3, 0x045e, 0x028e, 0x0114,
                                                 b"Microsoft X-Box 360 pad", 0))
            self.ioctl(self.fd, CREATE)
            self.neutral()
            if self.observe_kernel:
                self.attach_observer()
            return self
        except BaseException:
            self.close_fd(self.fd)
            self.fd = None
            raise

    def attach_observer(self):
        """Read only the event device created by this live uinput descriptor."""
        try:
            self.kernel_pad['stage'] = 1
            name = bytearray(64)
            self.ioctl(self.fd, 0x8040552c, name, True)  # UI_GET_SYSNAME(64)
            identity = bytes(name).split(b'\0', 1)[0].decode('ascii')
            if not re.fullmatch(r'input[0-9]{1,8}', identity):
                return
            self.kernel_pad['stage'] = 2
            parent = Path('/sys/devices/virtual/input') / identity
            entries = [p for p in parent.glob('event*') if re.fullmatch(r'event[0-9]{1,8}', p.name)]
            if len(entries) != 1:
                return
            self.kernel_pad['stage'] = 3
            expected = (entries[0] / 'dev').read_text().strip()
            if not re.fullmatch(r'[0-9]{1,8}:[0-9]{1,8}', expected):
                return
            self.kernel_pad['stage'] = 4
            fd = self.open_fd(str(Path('/dev/input') / entries[0].name),
                              os.O_RDONLY | os.O_NONBLOCK | os.O_NOFOLLOW | os.O_CLOEXEC)
            try:
                info = self.fstat(fd)
                self.kernel_pad['stage'] = 5
                device = tuple(int(v) for v in expected.split(':'))
                if not stat.S_ISCHR(info.st_mode) or (os.major(info.st_rdev), os.minor(info.st_rdev)) != device:
                    return
                observed_name = bytearray(80)
                self.kernel_pad['stage'] = 6
                self.ioctl(fd, 0x80504506, observed_name, True)  # EVIOCGNAME(80)
                if bytes(observed_name).split(b'\0', 1)[0] != b'Microsoft X-Box 360 pad':
                    return
                self.observer_fd = fd
                self.event_path = str(Path('/dev/input') / entries[0].name)
                self.kernel_pad['available'] = True
                self.kernel_pad['stage'] = 7
                self.kernel_pad['errno'] = 0
            finally:
                if self.observer_fd != fd:
                    self.close_fd(fd)
        except OSError as error:
            self.kernel_pad['errno'] = error.errno if type(error.errno) is int and 0 <= error.errno < 4096 else 0
        except (ValueError, UnicodeError):
            pass  # Missing observation is explicit, never treated as input proof.

    def observe(self):
        if self.observer_fd is None or len(self.kernel_pad['samples']) >= 128:
            return
        try:
            values = []
            for axis in (2, 5):
                data = bytearray(24)
                self.ioctl(self.observer_fd, 0x80184540 + axis, data, True)  # EVIOCGABS
                value, low, high, _, _, _ = struct.unpack('<iiiiii', data)
                if (low, high) != AXES[axis] or not low <= value <= high:
                    raise ValueError('Kernel trigger range differs')
                values.append(value)
            self.kernel_pad['samples'].append({'ns': time.monotonic_ns(), 'lt': values[0], 'rt': values[1]})
        except (OSError, ValueError):
            self.kernel_pad['available'] = False

    def attach_sdl_observer(self):
        """Open only our already-validated event path through the system SDL API."""
        if not self.event_path or self.observer_fd is None:
            return
        try:
            self.sdl_observation['stage'] = 1
            sdl = ctypes.CDLL('/usr/lib/libSDL2-2.0.so.0')
            declarations = {
                'SDL_InitSubSystem': ([ctypes.c_uint32], ctypes.c_int),
                'SDL_QuitSubSystem': ([ctypes.c_uint32], None),
                'SDL_NumJoysticks': ([], ctypes.c_int),
                'SDL_JoystickPathForIndex': ([ctypes.c_int], ctypes.c_char_p),
                'SDL_IsGameController': ([ctypes.c_int], ctypes.c_int),
                'SDL_GameControllerOpen': ([ctypes.c_int], ctypes.c_void_p),
                'SDL_GameControllerClose': ([ctypes.c_void_p], None),
                'SDL_GameControllerUpdate': ([], None),
                'SDL_GameControllerGetAxis': ([ctypes.c_void_p, ctypes.c_int], ctypes.c_int16),
            }
            for name, (arguments, result) in declarations.items():
                function = getattr(sdl, name)
                function.argtypes, function.restype = arguments, result
            self.sdl_observation['stage'] = 2
            if sdl.SDL_InitSubSystem(0x2000) != 0:
                return
            self.sdl = sdl
            self.sdl_observation['stage'] = 3
            count = sdl.SDL_NumJoysticks()
            if not 0 <= count <= 32:
                return
            matches = [i for i in range(count)
                       if sdl.SDL_JoystickPathForIndex(i) == self.event_path.encode('ascii')]
            if len(matches) != 1:
                return
            self.sdl_observation['stage'] = 4
            if not sdl.SDL_IsGameController(matches[0]):
                return
            self.sdl_pad = sdl.SDL_GameControllerOpen(matches[0])
            if self.sdl_pad:
                self.sdl_observation.update(available=True, stage=5)
        except (OSError, AttributeError):
            pass

    def observe_sdl(self):
        if not self.sdl_pad or len(self.sdl_observation['samples']) >= 128:
            return
        self.sdl.SDL_GameControllerUpdate()
        self.sdl_observation['samples'].append({
            'ns': time.monotonic_ns(),
            'lt': int(self.sdl.SDL_GameControllerGetAxis(self.sdl_pad, 4)),
            'rt': int(self.sdl.SDL_GameControllerGetAxis(self.sdl_pad, 5)),
        })

    def frame(self, changes):
        events = []
        for kind, code, value in changes:
            if kind == 1:
                valid = code in KEYS and value in (0, 1)
            else:
                valid = kind == 3 and code in AXES and AXES[code][0] <= value <= AXES[code][1]
            if not valid:
                raise ValueError("Invalid gamepad event")
            events.append(EVENT.pack(0, 0, kind, code, value))
        payload = b"".join(events) + EVENT.pack(0, 0, 0, 0, 0)
        if self.write(self.fd, payload) != len(payload):
            raise RuntimeError("Incomplete uinput write")
        self.observe()
        self.observe_sdl()

    def neutral(self):
        self.frame([(1, key, 0) for key in KEYS] + [(3, axis, 0) for axis in AXES])

    def __exit__(self, *unused):
        try:
            self.neutral()
        finally:
            try:
                if self.sdl_pad:
                    self.sdl.SDL_GameControllerClose(self.sdl_pad)
                    self.sdl_pad = None
                if self.sdl:
                    self.sdl.SDL_QuitSubSystem(0x2000)
                    self.sdl = None
                if self.observer_fd is not None:
                    self.close_fd(self.observer_fd)
                    self.observer_fd = None
            finally:
                try:
                    self.ioctl(self.fd, DESTROY)
                finally:
                    self.close_fd(self.fd)
                    self.fd = None


def recipe(name):
    """(offset seconds, changes) relative to settled device creation."""
    if name == "observer":
        events = []
        for index, button in enumerate("ABXY"):
            events += [(index * 4, [(1, BUTTONS[button], 1)]),
                       (index * 4 + 3, [(1, BUTTONS[button], 0)])]
        return events, 16
    if name == "stick-ollie":
        # Candidate default Skate gesture, NOT proof of native ollie or A->ollie.
        return [(0, [(1, BUTTONS["A"], 1)]), (0.1, [(1, BUTTONS["A"], 0)]),
                (1, [(3, 4, 32767)]), (1.6, [(3, 4, -32768)]), (1.75, [(3, 4, 0)])], 5
    if name == "idle":
        return [(0, [(1, BUTTONS["A"], 1)]), (0.1, [(1, BUTTONS["A"], 0)])], 5
    if name in ("exit-hub", "mount-board"):
        button = BUTTONS["B" if name == "exit-hub" else "Y"]
        return [(0, [(1, button, 1)]), (1, [(1, button, 0)])], 6
    if name == "open-menu":
        return [(0, [(1, 0x13b, 1)]), (1, [(1, 0x13b, 0)])], 6
    if name in ("candidate-flip-left", "candidate-flip-right"):
        # Research only: crouch, then flick toward one upper diagonal. Do not
        # label a native trick until a fresh game HUD/frame confirms its name.
        side = -28000 if name.endswith("left") else 28000
        return [(1, [(3, 4, 32767)]),
                (1.6, [(3, 3, side), (3, 4, -28000)]),
                (1.85, [(3, 3, 0), (3, 4, 0)])], 6
    if name in ("candidate-side-left", "candidate-side-right"):
        side = -32768 if name.endswith("left") else 32767
        return [(1, [(3, 4, 32767)]),
                (1.6, [(3, 3, side), (3, 4, 0)]),
                (1.85, [(3, 3, 0)])], 6
    if name in ("candidate-arc-left", "candidate-arc-right"):
        side = -32768 if name.endswith("left") else 32767
        half = -23000 if side < 0 else 23000
        return [(1, [(3, 4, 32767)]),
                (1.52, [(3, 3, half), (3, 4, 23000)]),
                (1.62, [(3, 3, side), (3, 4, 0)]),
                (1.8, [(3, 3, 0)])], 6
    if name in ("candidate-half-left", "candidate-half-right"):
        side = -32768 if name.endswith("left") else 32767
        half = -23000 if side < 0 else 23000
        return [(1, [(3, 4, 32767)]),
                (1.52, [(3, 3, half), (3, 4, 23000)]),
                (1.60, [(3, 3, side), (3, 4, 0)]),
                (1.68, [(3, 3, half), (3, 4, -23000)]),
                (1.76, [(3, 3, 0), (3, 4, -32768)]),
                (1.93, [(3, 4, 0)])], 6
    if name in ("candidate-scoop-left", "candidate-scoop-right"):
        side = -28000 if name.endswith("left") else 28000
        return [(1, [(3, 4, 32767)]),
                (1.55, [(3, 3, side), (3, 4, 14000)]),
                (1.75, [(3, 3, 0), (3, 4, 0)])], 6
    raise ValueError("Unknown fixed recipe")


def validate_phase_conditions(events, conditions, waiter):
    if not isinstance(conditions, dict) or len(conditions) > 2 or not callable(waiter):
        raise ValueError('Invalid bounded phase conditions')
    for index, condition in conditions.items():
        if (type(index) is not int or not 0 <= index < len(events) or not isinstance(condition, dict) or
                set(condition) - {'condition', 'timeout_ms', 'max_speed_mm_s'} or
                not {'condition', 'timeout_ms'} <= set(condition) or condition['condition'] != 'native-riding' or
                type(condition['timeout_ms']) is not int or not 100 <= condition['timeout_ms'] <= 3000 or
                ('max_speed_mm_s' in condition and (type(condition['max_speed_mm_s']) is not int or
                                                   not 0 <= condition['max_speed_mm_s'] <= 20000))):
            raise ValueError('Invalid bounded phase condition')
    return sum(c['timeout_ms'] for c in conditions.values()) / 1000


def replay(pad, events, duration, alive, sleep=time.sleep, clock=time.monotonic,
           phase_conditions=None, condition_waiter=None):
    if phase_conditions:
        validate_phase_conditions(events, phase_conditions, condition_waiter)
    start = clock()
    shift = 0.0
    timing = {"samples": [], "dropped": 0, "scope": "uinput_write_not_game_poll"}
    if phase_conditions:
        timing.update(conditions=[], condition_failed=False)
    input_attempted = False
    index = 0
    try:
        for index, (offset, changes) in enumerate(events + [(duration, [])]):
            while clock() - start < offset + shift:
                if not alive():
                    raise RuntimeError("Skate exited; test stopped")
                sleep(max(0, min(0.05, offset + shift - (clock() - start))))
            if phase_conditions and index in phase_conditions:
                began = clock()
                pad.neutral()  # The preceding pose is never held during a condition wait.
                result = condition_waiter(phase_conditions[index], alive)
                completed = clock()
                if (completed < began or completed - began > phase_conditions[index]['timeout_ms'] / 1000 + .100 or
                        not isinstance(result, dict) or result.get('status') not in ('READY', 'TIMEOUT')):
                    raise RuntimeError('Invalid phase condition result')
                # Anchor the resumed phase to completion, including prior writer
                # lateness; every later offset retains its nominal relative spacing.
                shift = completed - start - offset
                timing['conditions'].append(dict(result, phase=index, nominal_ms=round(offset * 1000, 3),
                    started_ms=round((began - start) * 1000, 3), completed_ms=round((completed - start) * 1000, 3),
                    wait_ms=round((completed - began) * 1000, 3), shift_ms=round(shift * 1000, 3)))
                if result['status'] == 'TIMEOUT':
                    timing['condition_failed'] = True
                    return timing
            check_at = clock()
            if not alive():
                raise RuntimeError("Skate exited; test stopped")
            if changes:
                write_at = clock()
                input_attempted = True
                pad.frame(changes)
                finished_at = clock()
                if len(timing["samples"]) < 128:
                    timing["samples"].append({"scheduled_ms": round(offset * 1000, 3),
                        "write_ms": round((write_at - start) * 1000, 3),
                        "ready_check_ms": round((write_at - check_at) * 1000, 3),
                        "write_and_observe_ms": round((finished_at - write_at) * 1000, 3)})
                    if phase_conditions:
                        timing['samples'][-1]['effective_scheduled_ms'] = round((offset + shift) * 1000, 3)
                else:
                    timing["dropped"] += 1
        if phase_conditions:
            timing['elapsed_ms'] = round((clock() - start) * 1000, 3)
        return timing
    except BaseException as error:
        timing.update(interrupted=True, interrupted_phase=min(index, 255),
                      elapsed_ms=round(max(0, min(120000, (clock() - start) * 1000)), 3))
        error.delivery_timing = copy.deepcopy(timing)
        error.input_may_have_been_sent = input_attempted
        raise


def image_format(data):
    if data.startswith(b"\x89PNG\r\n\x1a\n") and data.endswith(b"\x00\x00\x00\x00IEND\xaeB`\x82"):
        return "png"
    # AVIF is ISO BMFF: require complete boxes and an AVIF brand, not just an
    # extension. SteamOS observed on this Deck emits AVIF for SIGUSR2 captures.
    position, boxes = 0, []
    while position + 8 <= len(data):
        size, kind = struct.unpack_from(">I4s", data, position)
        header = 8
        if size == 1:
            if position + 16 > len(data):
                break
            size = struct.unpack_from(">Q", data, position + 8)[0]
            header = 16
        if size == 0:
            size = len(data) - position
        if size < header or position + size > len(data):
            break
        boxes.append(kind)
        position += size
    if (position == len(data) and boxes and boxes[0] == b"ftyp" and b"meta" in boxes
            and b"mdat" in boxes and b"avif" in data[8:32]):
        return "avif"
    raise ValueError("Incomplete or unsupported screenshot")


def read_image(path):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_uid != os.getuid() or not 32 <= info.st_size <= MAX_IMAGE:
            raise ValueError("Unsafe screenshot")
        with os.fdopen(fd, "rb", closefd=False) as stream:
            data = stream.read(MAX_IMAGE + 1)
        if len(data) != info.st_size:
            raise ValueError("Incomplete screenshot")
        return image_format(data), data
    finally:
        os.close(fd)


def screenshot(pid, tmp=Path("/tmp"), sleep=time.sleep, clock=time.monotonic):
    """Request one Gamescope debug screenshot; never return old files or raw paths.

    SIGUSR2 is documented by Gamescope source. Only signal a same-owner gamescope
    with a caught handler, via pidfd so PID reuse cannot target another process.
    Files are left in Gamescope's own temporary location, never blindly deleted.
    """
    fd = os.pidfd_open(pid)
    try:
        current = capabilities()
        if current.get("gamescope_pids") != [pid]:
            raise RuntimeError("Gamescope changed; screenshot refused")
        def candidates():
            return list(tmp.glob("gamescope*.png")) + list(tmp.glob("gamescope*.avif"))
        previous = {p: p.lstat().st_mtime_ns for p in candidates()}
        stable = {}
        signal.pidfd_send_signal(fd, signal.SIGUSR2)
        start = clock()
        while clock() - start < 2:
            for path in candidates():
                try:
                    info = path.lstat()
                    identity = (info.st_ino, info.st_size, info.st_mtime_ns)
                    if info.st_mtime_ns != previous.get(path):
                        if stable.get(path) == identity:
                            return read_image(path)
                        stable[path] = identity
                except (OSError, ValueError):
                    continue
            sleep(0.1)
        raise RuntimeError("No fresh Gamescope PNG/AVIF; capture interface unverified")
    finally:
        os.close(fd)


def fixture_console(operation):
    """Two fixed practice operations only; never accept console text from a plan."""
    if operation not in ('reset', 'status'):
        raise ValueError('Unknown fixture operation')
    environment = dict(os.environ, DISPLAY=':1', XAUTHORITY='/run/pressure-vessel/Xauthority')
    def run(*args):
        return subprocess.run(['/usr/bin/xdotool', *args], env=environment,
                              capture_output=True, text=True, check=True, timeout=3).stdout.strip()
    windows = run('search', '--onlyvisible', '--name', '^skate\\.$').splitlines()
    if len(windows) != 1 or not windows[0].isdecimal() or run('getwindowname', windows[0]) != 'skate.':
        raise RuntimeError('fixture_window_invalid')
    window = windows[0]
    geometry = run('getwindowgeometry', '--shell', window).splitlines()
    if 'WIDTH=1280' not in geometry or 'HEIGHT=800' not in geometry:
        raise RuntimeError('fixture_window_invalid')
    pid = run('getwindowpid', window)
    if not pid.isdecimal():
        raise RuntimeError('fixture_window_invalid')
    proc = Path('/proc') / pid
    argv = (proc / 'cmdline').read_bytes().decode().split('\0')
    if proc.stat().st_uid != os.getuid() or not any(a.replace('\\', '/').rsplit('/', 1)[-1].lower() == 'skate.exe' for a in argv):
        raise RuntimeError('fixture_window_invalid')
    run('windowfocus', '--sync', window)
    run('key', '--clearmodifiers', '--delay', '100', 'grave')
    try:
        time.sleep(.1)  # One UI handoff; native reset completion is observed below.
        run('key', '--clearmodifiers', '--delay', '100', 'ctrl+a')
        run('type', '--clearmodifiers', '--delay', '100', '--', 'arcadefixture ' + operation)
        run('key', '--clearmodifiers', '--delay', '100', 'Return')
        time.sleep(.1)
    finally:
        run('key', '--clearmodifiers', '--delay', '100', 'Escape')


def periodic(pid, duration, stop, capture_frames=4):
    if type(capture_frames) is not int or capture_frames not in (0, 1, 4):
        raise ValueError("Capture count must be zero, one or four")
    frames = []
    start = time.monotonic()
    offsets = () if capture_frames == 0 else (duration,) if capture_frames == 1 else (0, duration / 3, duration * 2 / 3, duration)
    for offset in offsets:
        if stop.wait(max(0, offset - (time.monotonic() - start))):
            break
        format_name, data = screenshot(pid)
        frames.append({"elapsed_ms": round((time.monotonic() - start) * 1000),
                       "sha256": hashlib.sha256(data).hexdigest(),
                       "format": format_name, "image_base64": base64.b64encode(data).decode()})
    return frames


def capture():
    """One explicitly requested display screenshot, including when game is closed."""
    pids = capabilities().get("gamescope_pids", [])
    if len(pids) != 1:
        raise RuntimeError("No unambiguous Gamescope capture target")
    format_name, data = screenshot(pids[0])
    return {"schema": 1, "recipe": "capture", "input_sent": False,
            "game_response": "UNVERIFIED", "frames": [{"elapsed_ms": 0,
            "sha256": hashlib.sha256(data).hexdigest(), "format": format_name,
            "image_base64": base64.b64encode(data).decode()}]}


def experiment(name):
    events, duration = recipe(name)  # Reject arbitrary recipes before side effects.
    return experiment_events(name, events, duration)


class ReadinessMonitor:
    """Read-only process checks off the timed writer; stale evidence stops input."""
    def __init__(self, initial, pid, checked_at, check=capabilities, clock=time.monotonic):
        self.pid, self.check, self.clock = pid, check, clock
        self.snapshot = (checked_at, initial)
        self.last_check = {}
        self.stop, self.failed = threading.Event(), threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def _run(self):
        while not self.stop.is_set():
            began = self.clock()
            try:
                current = self.check()
            except Exception:
                current = {}
            self.snapshot = (began, current)
            if not current.get("ready") or current.get("gamescope_pids") != [self.pid]:
                self.failed.set()
                return
            self.stop.wait(0.020)

    def start(self):
        self.thread.start()

    def ready(self, now=None):
        sampled_at, current = self.snapshot
        # Read the clock after the snapshot. The worker can publish between a
        # caller's clock read and this read; that is not a backwards clock.
        if now is None:
            now = self.clock()
        self.last_check = {
            'age_ms': max(-100000, min(100000, round((now - sampled_at) * 1000))),
            'failed': self.failed.is_set(),
            'context_ready': bool(current.get('ready', False)),
            'owner_matches': current.get('gamescope_pids') == [self.pid]}
        return (not self.failed.is_set() and 0 <= now - sampled_at <= 0.100
                and current.get("ready", False) and current.get("gamescope_pids") == [self.pid])

    def wait_initial_ready(self, alive):
        """Neutral startup only: allow a fresh check, never relabel stale evidence."""
        began = previous = self.clock()
        deadline = began + 0.250
        while True:
            alive()  # Session deadline and capture errors still stop the wait.
            now = self.clock()
            if now < previous or now >= deadline:
                return False
            previous = now
            if self.ready():
                return True
            if (self.last_check['failed'] or not self.last_check['context_ready'] or
                    not self.last_check['owner_matches'] or self.last_check['age_ms'] < 0):
                return False
            self.stop.wait(min(0.005, max(0, deadline - self.clock())))

    def close(self):
        self.stop.set()
        self.thread.join(timeout=0.2)
        if self.thread.is_alive():
            raise RuntimeError("Readiness inspection did not stop")


class ExperimentSession:
    """One neutral-idle controller for a bounded batch, never a persistent daemon."""
    def __init__(self, max_seconds=600, max_trials=24):
        if type(max_seconds) is not int or not 1 <= max_seconds <= 1100:
            raise ValueError("Session duration must be 1..1100 seconds")
        if type(max_trials) is not int or not 1 <= max_trials <= 24:
            raise ValueError("Session trial limit must be 1..24")
        self.max_seconds, self.max_trials = max_seconds, max_trials
        self.pad, self.manager, self.handlers = None, None, {}
        self.deadline = self.last_clock = self.armed_until = None
        self.used = False
        self.trials = 0
        self.stop = None

    def _clock(self):
        now = time.monotonic()
        if self.last_clock is not None and now < self.last_clock:
            raise RuntimeError("Session clock moved backwards")
        self.last_clock = now
        if self.deadline is not None and now >= self.deadline:
            raise RuntimeError("Bounded session deadline reached")
        return now

    def _arm(self, deadline):
        now = self._clock()
        if deadline != self.armed_until:
            signal.alarm(max(1, math.ceil(deadline - now)))
            self.armed_until = deadline

    def _interrupted(self, signum, frame):
        try:
            self.close()
        finally:
            raise RuntimeError("Bounded experiment interrupted")

    def __enter__(self):
        if self.used:
            raise RuntimeError("Experiment session cannot be reopened")
        self.used = True
        status = capabilities()
        if not status.get("ready"):
            raise RuntimeError("Input/capture preflight failed; no input sent: " + json.dumps(status))
        self.pid = status["gamescope_pids"][0]
        self.deadline = self._clock() + self.max_seconds
        try:
            for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGALRM):
                self.handlers[sig] = signal.signal(sig, self._interrupted)
            self._arm(min(self.deadline, self._clock() + 25))
            self.manager = Gamepad(observe_kernel=True)
            self.pad = self.manager.__enter__()
            time.sleep(2)  # Hotplug settles once for the entire batch.
            if self.pad.observer_fd is None:
                self.pad.attach_observer()
            self.pad.attach_sdl_observer()
            screenshot(self.pid)  # Prove capture before any gameplay input.
            self._arm(self.deadline)
            return self
        except BaseException:
            self.close()
            raise

    def close(self):
        if self.stop is not None:
            self.stop.set()
        pad, manager = self.pad, self.manager
        self.pad = self.manager = None
        try:
            if pad is not None:
                manager.__exit__(None, None, None)
        finally:
            if self.handlers:
                signal.alarm(0)
                for sig, handler in self.handlers.items():
                    signal.signal(sig, handler)
                self.handlers = {}
                self.armed_until = None

    def __exit__(self, *unused):
        self.close()

    def run(self, name, events, duration, capture_frames=4, phase_conditions=None, condition_waiter=None):
        """Run validated events; native outcomes remain independently UNVERIFIED."""
        if self.pad is None:
            raise RuntimeError("Experiment session is not active")
        pad, delivery_timing = None, None
        neutral_receipt = {'attempted': False, 'write_completed': False, 'elapsed_ms': None}
        try:
            extra = validate_phase_conditions(events, phase_conditions, condition_waiter) if phase_conditions else 0
            if extra and capture_frames != 0:
                raise ValueError('Conditioned trials require zero captures')
            if (type(duration) not in (int, float) or not math.isfinite(duration) or
                    not 0 < duration <= 20 or self.trials >= self.max_trials or
                    type(capture_frames) is not int or capture_frames not in (0, 1, 4)):
                raise ValueError("Invalid or exhausted bounded trial")
            now = self._clock()
            if duration + extra > 20 or duration + extra > self.deadline - now:
                raise RuntimeError("Insufficient session time for trial")
            status = capabilities()
            if not status.get("ready") or status.get("gamescope_pids") != [self.pid]:
                raise RuntimeError("Session input/capture context changed")
            self._arm(min(self.deadline, now + 25))
            self.trials += 1
            pad = self.pad
            for observation in (pad.kernel_pad, pad.sdl_observation):
                if isinstance(observation, dict):
                    observation['samples'] = []
            with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
                self.stop = stop = threading.Event()
                future = pool.submit(periodic, self.pid, duration, stop, capture_frames)
                watch = None
                def context_alive():
                    self._clock()
                    if future.done():
                        future.result()  # Stop input if capture failed.

                def readiness_failure():
                    error = RuntimeError('Skate exited; test stopped')
                    error.readiness = dict(watch.last_check, capture_pending=not future.done())
                    return error

                def alive():
                    context_alive()
                    if not watch.ready():
                        raise readiness_failure()
                    return True
                failure = None
                try:
                    # A slow preflight remains timestamped at its start. Obtain
                    # fresh evidence while neutral, before starting the timeline.
                    watch = ReadinessMonitor(status, self.pid, now, check=capabilities, clock=time.monotonic)
                    watch.start()
                    if not watch.wait_initial_ready(context_alive):
                        raise readiness_failure()
                    if duration + extra > self.deadline - self._clock():
                        raise RuntimeError('Insufficient session time for trial')
                    if phase_conditions:
                        delivery_timing = replay(pad, events, duration, alive,
                            phase_conditions=phase_conditions, condition_waiter=condition_waiter)
                    else:
                        delivery_timing = replay(pad, events, duration, alive)
                    pad.neutral()  # Never hold the final phase while capture drains.
                    frames = future.result()
                except BaseException as error:
                    failure = error
                    raise
                finally:
                    # Record only a completed neutral write, before joining any
                    # worker. Closing a device is not a game-facing neutral ACK.
                    stop.set()
                    cleanup_error = None
                    if self.pad is not None:
                        began = time.monotonic()
                        neutral_receipt['attempted'] = True
                        try:
                            pad.neutral()
                            neutral_receipt['write_completed'] = True
                        except BaseException as error:
                            cleanup_error = error
                        finally:
                            neutral_receipt['elapsed_ms'] = round(max(0, min(120000,
                                (time.monotonic() - began) * 1000)), 3)
                    if watch is not None:
                        try:
                            watch.close()
                        except BaseException as error:
                            cleanup_error = cleanup_error or error
                    if cleanup_error is not None:
                        if failure is not None:
                            for field in ('delivery_timing', 'input_may_have_been_sent', 'readiness'):
                                if hasattr(failure, field):
                                    setattr(cleanup_error, field, getattr(failure, field))
                        raise cleanup_error
            self.stop = None
            self._arm(self.deadline)  # Idle time between trials stays bounded too.
            return {"schema": 1, "recipe": name, "input_sent": bool(delivery_timing['samples']) if phase_conditions else True,
                    "game_response": "UNVERIFIED", "frames": frames,
                    "delivery_timing": delivery_timing,
                    "kernel_pad": copy.deepcopy(pad.kernel_pad),
                    "sdl_pad": copy.deepcopy(pad.sdl_observation)}
        except BaseException as error:
            # Keep bounded sanitized writer evidence even when cleanup raises.
            timing = getattr(error, 'delivery_timing', delivery_timing)
            if not isinstance(timing, dict):
                timing = {'samples': [], 'dropped': 0, 'scope': 'uinput_write_not_game_poll'}
            partial = None
            if pad is not None:
                partial = {
                    'schema': 1, 'outcome': 'INTERRUPTED', 'game_response': 'UNVERIFIED',
                    'input_sent': bool(timing.get('samples')),
                    'input_may_have_been_sent': bool(getattr(error, 'input_may_have_been_sent',
                                                            timing.get('samples'))),
                    'neutralized': neutral_receipt['write_completed'],
                    'neutralization': dict(neutral_receipt),
                    'delivery_timing': copy.deepcopy(timing),
                    'kernel_pad': copy.deepcopy(pad.kernel_pad),
                    'sdl_pad': copy.deepcopy(pad.sdl_observation)}
            try:
                self.close()  # Infrastructure failures cannot reuse the controller.
            except BaseException as cleanup_error:
                if hasattr(error, 'readiness'):
                    cleanup_error.readiness = error.readiness
                if partial is not None:
                    cleanup_error.partial_run = partial
                raise
            if partial is not None:
                error.partial_run = partial
            raise


def experiment_events(name, events, duration, capture_frames=4, phase_conditions=None, condition_waiter=None):
    """Single-run compatibility wrapper with the original 25-second lifetime."""
    with ExperimentSession(max_seconds=25, max_trials=1) as session:
        if phase_conditions:
            return session.run(name, events, duration, capture_frames, phase_conditions, condition_waiter)
        return session.run(name, events, duration, capture_frames)


def remote_source(operation, name):
    if operation not in ("preflight", "run", "capture"):
        raise ValueError("Unknown operation")
    recipe(name)
    source = Path(__file__).read_text().split('\nif __name__ == "__main__":')[0]
    call = ("capabilities()" if operation == "preflight" else "capture()" if operation == "capture"
            else "experiment(" + repr(name) + ")")
    return source + "\nprint(json.dumps(" + call + "))\n"


def save_run(result, output, expected_frames=None):
    """Keep full frames local only. Nothing in this runner publishes to GitHub."""
    if expected_frames is None:
        expected_frames = 1 if result.get("recipe") == "capture" else 4
    if (type(expected_frames) is not int or expected_frames not in (0, 1, 4) or
            result.get("schema") != 1 or len(result.get("frames", [])) != expected_frames):
        raise ValueError("Invalid experiment response")
    decoded = []
    for frame in result["frames"]:
        data = base64.b64decode(frame["image_base64"], validate=True)
        if (len(data) > MAX_IMAGE or image_format(data) != frame["format"]
                or hashlib.sha256(data).hexdigest() != frame["sha256"]):
            raise ValueError("Screenshot verification failed")
        decoded.append(data)
    from deck_installer import safe_path
    safe_path(output)
    output.mkdir(parents=True, exist_ok=False, mode=0o700)
    for index, (frame, data) in enumerate(zip(result["frames"], decoded)):
        with (output / (str(index) + "." + frame["format"])).open("xb") as stream:
            stream.write(data)
        del frame["image_base64"]
    (output / "run.json").write_text(json.dumps(result, indent=2) + "\n")


def main(argv=None):
    from controller import ssh_arguments, HOME
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("preflight", "run", "capture"))
    parser.add_argument("--recipe", choices=("observer", "idle", "stick-ollie",
                                             "exit-hub", "mount-board", "open-menu", "candidate-flip-left",
                                             "candidate-flip-right", "candidate-side-left",
                                             "candidate-side-right", "candidate-arc-left",
                                             "candidate-arc-right", "candidate-half-left",
                                             "candidate-half-right", "candidate-scoop-left",
                                             "candidate-scoop-right"), default="idle")
    parser.add_argument("--host", default="deck@steamdeck.local")
    parser.add_argument("--output", type=Path, help="New local directory, required for run; screenshots stay private")
    args = parser.parse_args(argv)
    if args.operation != "preflight" and args.output is None:
        parser.error("run/capture requires --output")
    if args.operation != "preflight":
        from deck_installer import safe_path
        safe_path(args.output)
        if args.output.exists():
            parser.error("--output must be a new directory; no input sent")
    command = "python3 -B -c " + shlex.quote(remote_source(args.operation, args.recipe))
    response = subprocess.run(ssh_arguments(args.host, HOME) + [command],
                              capture_output=True, text=True, check=True, timeout=40)
    result = json.loads(response.stdout)
    if args.operation != "preflight":
        save_run(result, args.output)
        print(("INPUT SENT; GAME RESPONSE UNVERIFIED. " if args.operation == "run" else "SCREENSHOT SAVED. ")
              + "Local evidence: " + str(args.output))
    else:
        print(json.dumps(result, indent=2))
    return 0 if args.operation != "preflight" or result.get("ready") else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError):
        print("Robot stopped. No deployment performed; check SSH/capabilities.", file=sys.stderr)
        sys.exit(1)
