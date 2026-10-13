"""Bounded SSH gamepad experiments. Preflight is read-only; run is explicit.

No dependencies, sudo, game writes, deployment, persistent daemon, or publication.
The remote worker is sent in memory. Linux x86_64 uinput ABI is deliberate.
"""
import argparse
import base64
import concurrent.futures
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
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
                 close=os.close, lock=fcntl.flock, fstat=os.fstat):
        self.open_fd, self.ioctl, self.write, self.close_fd = open_fd, ioctl, write, close
        self.lock, self.fstat, self.fd = lock, fstat, None

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
            self.ioctl(self.fd, SETUP, struct.pack("<HHHH80sI", 3, 0x045e, 0x028e, 1,
                                                 b"ReSkate Arcade Lab Virtual Controller", 0))
            self.ioctl(self.fd, CREATE)
            self.neutral()
            return self
        except BaseException:
            self.close_fd(self.fd)
            self.fd = None
            raise

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

    def neutral(self):
        self.frame([(1, key, 0) for key in KEYS] + [(3, axis, 0) for axis in AXES])

    def __exit__(self, *unused):
        try:
            self.neutral()
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


def replay(pad, events, duration, alive, sleep=time.sleep, clock=time.monotonic):
    start = clock()
    for offset, changes in events + [(duration, [])]:
        while clock() - start < offset:
            if not alive():
                raise RuntimeError("Skate exited; test stopped")
            sleep(max(0, min(0.05, offset - (clock() - start))))
        if not alive():
            raise RuntimeError("Skate exited; test stopped")
        if changes:
            pad.frame(changes)


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


def periodic(pid, duration, stop):
    frames = []
    start = time.monotonic()
    for offset in (0, duration / 3, duration * 2 / 3, duration):
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


def experiment_events(name, events, duration):
    """Repository-generated, validated batch events share single-run cleanup."""
    status = capabilities()
    if not status.get("ready"):
        raise RuntimeError("Input/capture preflight failed; no input sent: " + json.dumps(status))
    def interrupted(signum, frame):
        raise RuntimeError("Bounded experiment interrupted")
    handlers = {sig: signal.signal(sig, interrupted) for sig in
                (signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGALRM)}
    signal.alarm(25)  # Device lifetime stays bounded even if the SSH client disappears.
    try:
        with Gamepad() as pad:
            time.sleep(2)  # Allow Steam's controller hotplug path to settle.
            # Prove screenshot transport before any non-neutral gameplay input.
            screenshot(status["gamescope_pids"][0])
            with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
                stop = threading.Event()
                future = pool.submit(periodic, status["gamescope_pids"][0], duration, stop)
                def alive():
                    if future.done():
                        future.result()  # Stop input if capture failed.
                    return capabilities().get("ready", False)
                try:
                    replay(pad, events, duration, alive)
                    frames = future.result()
                finally:
                    # Neutralize BEFORE joining the screenshot worker, otherwise
                    # a failed replay could leave a button held until its next frame.
                    stop.set()
                    pad.neutral()
        return {"schema": 1, "recipe": name, "input_sent": True,
                "game_response": "UNVERIFIED", "frames": frames}
    finally:
        signal.alarm(0)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)


def remote_source(operation, name):
    if operation not in ("preflight", "run", "capture"):
        raise ValueError("Unknown operation")
    recipe(name)
    source = Path(__file__).read_text().split('\nif __name__ == "__main__":')[0]
    call = ("capabilities()" if operation == "preflight" else "capture()" if operation == "capture"
            else "experiment(" + repr(name) + ")")
    return source + "\nprint(json.dumps(" + call + "))\n"


def save_run(result, output):
    """Keep full frames local only. Nothing in this runner publishes to GitHub."""
    if result.get("schema") != 1 or len(result.get("frames", [])) != (1 if result.get("recipe") == "capture" else 4):
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
