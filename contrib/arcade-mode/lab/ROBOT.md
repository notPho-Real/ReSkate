# SSH controller experiments

`robot.py` sends a temporary Python worker over the existing dedicated-key,
strict-host-key SSH connection. No bootstrap, deployment, or installation occurs.
It requires Linux x86_64, a writable `/dev/uinput` character device, Python pidfd
support, a running Skate.exe, and exactly one same-user Gamescope process with a
caught SIGUSR2 handler. If these are unavailable, stop; do not use sudo, install
drivers, change permissions, or disable SteamOS read-only mode.

From the repository root:

```sh
python3 -B contrib/arcade-mode/lab/robot.py preflight
python3 -B contrib/arcade-mode/lab/robot.py capture --output artifacts/robot-screen-01
python3 -B contrib/arcade-mode/lab/robot.py run --recipe idle --output artifacts/robot-idle-01
python3 -B contrib/arcade-mode/lab/robot.py run --recipe stick-ollie --output artifacts/robot-stick-01
python3 -B contrib/arcade-mode/lab/robot.py run --recipe observer --output artifacts/robot-observer-01
python3 -B contrib/arcade-mode/lab/robot.py run --recipe candidate-flip-left --output artifacts/robot-flip-left-01
python3 -B contrib/arcade-mode/lab/robot.py run --recipe candidate-flip-right --output artifacts/robot-flip-right-01
python3 -B contrib/arcade-mode/lab/controller.py trace
```

Run only after Skate is intentionally launched and visible. These inputs go
through Steam's normal device routing; focus and player assignment are NOT yet
automatically verified. A virtual controller may be assigned player two. The
first hardware experiment must check delivery from fresh observer counters and
screenshots before using input as evidence. This tool does not launch/close the
game, navigate Steam menus, change controller order, or deploy a DLL.

Preflight performs reads only, never opening uinput or signalling a process.
`capture` requests a single private display screenshot, even when Skate is closed;
it never creates a controller or sends input.
An explicit run creates an Xbox-style gamepad, waits two seconds for hotplug,
checks screenshot capture, then runs one fixed recipe. Idle taps A and waits five
seconds. Stick-ollie taps A to arm the diagnostic trace, then moves right-stick Y
down/up/neutral; this is a candidate native gesture, not an implemented A ollie.
Observer holds each A/B/X/Y for three seconds with one press/release each.
The two `candidate-flip-*` recipes are stock-game research gestures: crouch,
then flick the right stick to an upper diagonal. They are deliberately named
*candidate* because the resulting native move has not been verified on this
Deck. Compare fresh private frames with the in-game trick HUD before assigning
a trick name. No game files or lab release are changed by these recipes.
No keyboard events, arbitrary commands or user-defined long scripts are accepted.

Four screenshots run alongside input, distributed across the recipe, plus one
initial capture check. SIGUSR2 targets a rechecked same-owner Gamescope through a
pidfd. Only fresh, same-owner, regular PNG/AVIF files under `/tmp/gamescope*` are read;
symlinks, stale/incomplete/oversized images fail. Each frame is limited to 8 MiB.
Gamescope owns its temporary screenshots; the runner does not delete files there.
The real Deck emits `gamescope_<timestamp>.avif`, verified by a captured frame.
AVIF requires complete ISO BMFF boxes; both formats require stable file metadata.
On macOS, inspect an AVIF as PNG with the standard utility:
`sips -s format png artifacts/robot-screen-01/0.avif --out artifacts/robot-screen-01/0.png`.

The device is neutralized and destroyed at exit, including exceptions and handled
signals. A 25-second alarm bounds an orphaned worker; kernel fd cleanup also
removes the virtual device on termination. Concurrent lab runners refuse the
nonblocking device-inode lock. Failure to capture or losing Skate stops input.

Frames and their SHA-256 checksums are saved in a new local directory, normally
under ignored `artifacts/`. Full screenshots may contain overlays/personal content:
they are never automatically published. `run.json` records **input sent**, not a
gameplay PASS. Use `controller.py trace` separately for fresh, source/hash-bound
numeric traces (requires the diagnostic development build to have been explicitly
deployed). A screenshot alone cannot establish native trick state or no crash.
No lab report criteria are automatically promoted by this runner.

Sources: [Linux uinput documentation](https://docs.kernel.org/input/uinput.html),
[Linux ABI header](https://github.com/torvalds/linux/blob/master/include/uapi/linux/uinput.h),
[Valve Gamescope signal handler](https://github.com/ValveSoftware/gamescope/blob/master/src/main.cpp),
[Gamescope screenshot behavior](https://github.com/ValveSoftware/gamescope).

Tests run with the existing Linux/macOS release gates and bootstrap self-test:
`python3 -B -m unittest discover -s contrib/arcade-mode -p 'test_*.py' -v`.
All devices, transport and screenshots in unit tests are simulated. Separately,
real Deck neutral device creation/destruction and a Gamescope screenshot have
passed. Steam game delivery and periodic in-game capture remain unverified;
hardware support must never be inferred from unit-test passes.
