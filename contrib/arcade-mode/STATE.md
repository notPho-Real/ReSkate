# Arcade Mode persistent project state

## X tricks continuation — 2026-10-13

Starting HEAD 5f479ba. Deck connection verified read-only: active f669926,
game/launcher running, expected dev hash and original stock hash intact,
updates=false. Temporary mDNS failure resolved after wake; IP was checked
against the existing paired hostname key, with no new key acceptance.
Upstream fetched read-only at 28118efc2735a71a0c38bda2e7955d35cb2e7d24:
71 commits behind, including relevant grind-exit gamepad filtering. Integrated
on arcade-mode before new trick work, preserving both upstream filter and our
outer-game-read remap. Local 114 Python tests and C++ model pass after merge;
Windows integration still needs CI. No deployment or gameplay input yet.
Merge checkpoint: 25507d6. Shared bounded A/X gesture engine now implemented
locally: X neutral/left = discovered kickflip, X right = discovered heelflip,
direction selected by D-pad before left stick. Preserves measured A timing,
physical right-stick cancellation, no queued/repeated hold, menu release gate.
Only grounded state100/mode1 starts; midair ollie->flip remains unsupported and
must be characterized rather than counted as integrated. C++ tests include 20
repetitions per flip profile in simulation plus exact A boundary equivalence.
X implementation commit 7eb6b3b. Its Windows CI configure failed before compile:
upstream Runtime.cmake globbed an unfetched windows_x86_64_msvc Cargo import
library. Linux/macOS passed. Workflow now fetches locked Windows dependencies
before CMake (preserving upstream features). Next: rerun CI, then deploy/test
and record actual hardware outcomes. Other
directional trick names remain selection-only
until native gestures are measured. Do not call the flips repeatable/integrated.

## Autonomy implementation — 2026-10-12

Starting HEAD for this task: `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`.
Repository remains notPho-Real/ReSkate, branch arcade-mode. Preserve the older
flip selector milestone and uncommitted research recipes; no upstream edits.

Added `.codex/config.toml` (workspace-write, network off, automatic escalation
review, high reasoning) and absolute-path command rules. They require a trusted
project reload and cannot override this session's managed read-only policy.
No global configuration is modified. `inspect_project.py` exposes fixed git,
fork Actions/release and robot/controller read-only commands without forwarded
host/options. `trick_batch.py` accepts only bounded JSON controller data and
pins the target, source hash, DLL hash and original stock hash. See
`lab/TRICK_BATCH.md` and `.codex/README.md` for exact interfaces and limitations.

The batch runner reuses single-run robot cleanup/locking/capture, checks every
candidate, continues ordinary criterion failures, checkpoints private summaries,
and never deploys/restores/publishes. New read-only native heartbeat every 250 ms
provides fresh numeric state/mode for conservative on-board context gating.
No heartbeat means SKIPPED_CONTEXT. Named trick remains UNVERIFIED; sampled
state sequence is not proof of trick identity or integration. Recovery currently
waits boundedly for existing game/HIL readiness; no automatic launcher clicking.

Local verification: 114 Python tests passed (23 new batch tests), C++ pure model
executable passed. Includes fake Deck exact stock roundtrip, existing 21 robot
tests and new failed-candidate continuation/integrity/interrupt/evidence bounds.
Current CLI accepts auto_review/workspace-write and parses the rules; positive
batch rule matched. Python compileall, shell syntax and git diff checks passed;
arbitrary python, ssh and a same-named script outside this checkout matched no
project allow rule. Both workflow YAML files parsed. 15 Python files and 11
shell files passed syntax validation.
No new batch was executed on the real Deck during this task.
Last known installed build remains f669926 and stock backup remains preserved.

Implementation commits: `5c1be16b78bbb856c6d831a7d9d99bf52b2eb32f` and
`e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60`, pushed only to origin arcade-mode,
along with the preserved earlier 8ff309f milestone. Final code CI
https://github.com/notPho-Real/ReSkate/actions/runs/38356744624 passed all four
jobs: Windows build/model, Linux tests, macOS lab and gated release. Separate
installer run 38356744628 passed too. Local suite: 114 passed, zero failures;
pure C++ executable passed. Hosted jobs include the same batch tests.

Immutable release `arcade-build-e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60` and
rolling channel were downloaded and verified with the existing pinned-origin,
commit, archive hash, DLL hash and x64 PE validators. DLL SHA-256:
`c44fd019365bed73ab97e1bb0f9ee4912e743d5a1e5f75bf27c18bcdf0223f45`.
Example JSON now pins this build. This final handoff/data-pin commit does not
change executable code and deliberately skips another identical CI build.

Exact next action: reload this trusted project so Codex can apply local policy
(managed host policy still wins), use the established standing-authorized
deployment procedure to update the active development installation to the
verified e63dbe1 build, launch Skate into on-board skating, then run the command
in lab/TRICK_BATCH.md. No new runner operation or SSH access occurred during
this autonomy implementation; hardware behavior of heartbeat/batching remains
UNVERIFIED. Last known installed DLL remains f669926; do not mistake release
publication for deployment. Keep dev installed. Resume controlled repeatability
and chain tests, not claims of completed flips. Recovery is bounded readiness
waiting, not automatic launcher navigation. Skipped-context trials and named
trick UNVERIFIED must not be promoted to passes.

## Live Deck research session (update immediately after each milestone)

Latest user instructions establish **ACTIVE HARDWARE-IN-THE-LOOP DEVELOPMENT**.
Leave the current tested Arcade development DLL installed across iterations;
the preserved stock backup is emergency/final rollback, not the default between
experiments. The user grants standing authorization for bounded HIL candidates,
repo edits/tests/CI, and atomically updating an active dev installation with
a newly CI-verified development DLL. Restore only on explicit user request,
declared end of development, inability to start the game, failed integrity
requiring recovery, or a concrete safety invariant. Never restore because a
trick probe fails or a handoff is being written. Do not claim any trick
implemented from one HUD observation.

Trick maturity: DISCOVERED (>=1 confirmed), REPEATABLE (>=20 controlled trials
plus varied speed/respawn), INTEGRATED (composes with ollie/landing/grind/manual
contexts, boundaries and physical-input precedence characterized). Current A
ollie is hardware demonstrated twice, Kickflip once, Heelflip at least once;
NONE meets the full new integrated standard. Immediate chain baseline is
ollie -> land; kickflip -> land; heelflip -> land; ollie -> kickflip -> land;
ollie -> heelflip -> land. Grind/manual chains follow once controllable.
Use data-driven bounded candidate tables and machine result files under
`contrib/arcade-mode/research/`. Keep game/dev build hot, neutralize temporary
virtual input after each candidate, and do not publish raw frames/logs.

The user explicitly approved redeploying the previously tested f669926 DLL
for native flip research, then restoring stock. Read-only preflight passed:
both processes stopped, tested release source f669926 and expected DLL hash
`c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7`.
`controller.py deploy` then succeeded and verified this exact installed hash,
preserved original backup hash
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`,
active=true, updates=false. The later standing authorization above supersedes
the original restore-after-research instruction. Keep development installed.
The new X selector code remains only in
local commit `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`, not on the Deck.

Live update: ReSkate launcher READY with updates off; Play launched Skate.
The tested virtual controller exited Hub with B and mounted the board with Y.
On flat ground, right stick down (XInput Y=-32768) for 600 ms then upper-left
(-28000,+28000) for 250 ms produced the in-game HUD **Heelflip Sketchy**.
The mirror upper-right (+28000,+28000) produced **Kickflip Sketchy**. Four
private Gamescope frames per run; the named HUD is visible in frame 2 of each
`artifacts/flip-candidate-left-01` and `...right-01`. These are REAL Deck
observations, but each is one attempt and "Sketchy" means the current timing
may need refinement. The classic THPS mapping is left X=Kickflip/right
X=Heelflip, so the native right-stick **direction is opposite** the THPS
direction for these two names. No X runtime build has been deployed.

## Active X flip-trick milestone (2026-10-12 Pacific)

Starting HEAD `970b2d93048d46c41506ed35a8a5df05076aefaf`, branch
`arcade-mode`, clean at start. User requested actual THUG/THPS flip tricks on
X plus left stick or D-pad. Activision's official THPS1+2 controls table
(https://support.activision.com/tony-hawks-pro-skater-1-2/articles/controls-and-tricks-in-tony-hawks-pro-skater-1-2)
specifies Xbox X + left/right/up/down = Kickflip/Heelflip/Impossible/Pop
Shove-It, and top-left/top-right/bottom-left/bottom-right = Hardflip/Inward
Heelflip/Varial Kickflip/Varial Heelflip. This is a verified classic input
mapping; the native Skate right-stick sequences for those eight moves are
**not yet verified**. The real Deck was confirmed reachable and STOCK by a
read-only `controller.py status`; game and launcher stopped. Do not deploy a
new DLL without explicit approval for the tested source. Immediate work:
derive bounded native gesture candidates and test with the stock game where
possible, then implement pure direction selection and verified native X
remapping. Preserve A behavior and rerun local/CI gates.

Progress: `arcade_flip.h` now has a pure eight-direction selector for both
XInput D-pad and left stick, with D-pad precedence, a stick deadzone, and
invalid opposing D-pad rejection. C++ model tests pass locally. `lab/robot.py`
has two bounded, explicitly experimental upper-diagonal right-stick recipes,
with fake-device tests and instructions in `lab/ROBOT.md`. Python Deck/Mac
suite passes 91/91 with permitted temporary filesystem access. The initial
read-only sandbox run failed 71 tests because the sandbox disallowed *all*
temporary directories; the exact suite passed unsandboxed. No X runtime hook
or native trick claim yet; no new Deck game mutation. Await stock Skate being
launched before attempting screen-based gesture identification.
Read-only remote robot preflight found the Deck connected with Gamescope and
writable uinput, but `skate_running=false`, so no gesture was sent and no
native trick identity was asserted. The existing ReSkate Steam shortcut was
identified by read-only lookup; shortcuts.vdf was not edited. Python syntax
passes for changed files. These changes are research and pure selection only,
not an X gameplay implementation; do not call this milestone complete.

Stock-game attempt: launched the existing ReSkate Steam shortcut solely for
gesture research; its launcher displayed **"A ReSkate update is ready"** and
offered UPDATE, so no button was selected. Launched Skate's own installed Steam
app separately; private Gamescope evidence showed startup error E111000B:
"Wine, Proton, and Steam Deck are not supported by this application." Skate
never reached gameplay. The launcher window opened by this attempt was closed
via its verified X window ID, without pressing update. Final read-only status
showed game=false, launcher=false, active Arcade=false, updates=true, current
DLL and original backup still exactly
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No candidate flip gesture was sent. Private images are ignored under
`artifacts/flip-*` and must never be published.

Current external blocker for **native trick identification**: stock Skate
cannot start directly under Proton, while the ReSkate launcher currently
requires a pending stock update to expose Play. Do not click UPDATE or deploy
a development DLL implicitly. Next action is to resolve that launcher state
through an explicitly authorized safe operation (or obtain native gesture
evidence through another primary route), then run the candidate recipes on
the actual game, identify HUD results, add only confirmed gesture profiles to
the guarded runtime X remapper, and pass C++/Python/Windows/Linux/macOS gates.

Updated 2026-10-13 UTC (2026-10-12 Pacific). This first section is authoritative.
The historical sections below are retained for provenance and are superseded.

## Current handoff

Repository: notPho-Real/ReSkate, branch `arcade-mode` ONLY. Never change
main/master/upstream, replace Skate.exe, use Deck root/sudo, disable SteamOS
read-only mode, save a Deck password, or edit shortcuts.vdf. Origin is
https://github.com/notPho-Real/ReSkate.git. Starting HEAD for the most recent
A-ollie implementation was 2a410720c61aecc2895f10bbc213bfff8ade16bb.
Feature commit f669926ab790d1328bc234a9828c290fb46ee276 was pushed only
to arcade-mode; a documentation-only final handoff commit may follow. No PR.

The REAL Steam Deck is currently STOCK: ReSkate.dll and preserved first-stock
backup both hash SHA-256
5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Skate and ReSkateLauncher were both stopped after the test. Arcade receipt
active=false, recovery_pending=false, and the original ReSkate updater
preference was restored to true. User explicitly approved deploying and testing
feature f669926, then restoring; the approved session is finished. Another
development DLL deployment requires an explicit user operation/authorization.

## Implemented architecture

Arcade Mode is native in ReSkate.dll because stock ReSkate does not load
arbitrary Plugins/*.dll and custom Lua scripts are currently disabled. Its
startup and client-tick hooks already exist; the overlay controller reader
provides A/B/X/Y edges and the runtime observer. A=ollie, X=flip/trick family,
B=grab, Y=grind/context are intended controls; only A ollie has gameplay
implementation. The existing pure `arcade_model.h` handles combo scoring,
banking, fatigue, balance/grace, and bail reset without affecting game state.
Future multiplayer knockdowns should be host-authoritative events that invoke
native bail, not player rigid-body collisions.

Feature f669926 adds pure `arcade_ollie.h` and hooks the already-present
XInput game-read paths, including Steam's front hook. A triggers a native
right-stick gesture only when guarded client-tick reads show a fresh local
on-board idle state (state 100, rig mode 1), game focus and no interactive
overlay. It suppresses the triggering game A, waits 1 s, holds the right stick
down 600 ms, releases up 150 ms, then returns to neutral. Holding A starts once;
physical right-stick movement or menu capture cancels it. The binary signatures,
owner checks and five-second bounded numeric diagnostic trace remain in
`arcade_native_probe.h` and `arcade_mode.cpp`. This path does not write game
memory or guess a native trick function. The major technical risk remains
commanding Skate's native trick state machine cleanly across contexts.

## Build, release and tests

Feature source f669926 passed local 91/91 Python Deck/Mac simulations, pure C++
model tests, and diff check. GitHub run 38338735186 passed Windows x64 DLL build
and C++ tests, Linux simulations/shell syntax, hosted macOS lab tests/Python
syntax, and gated Deck packaging. Installer workflow 38338735189 passed.
The public immutable `arcade-build-f669926ab790d1328bc234a9828c290fb46ee276`
and rolling tested `arcade-deck-latest` point to source f669926 and DLL SHA-256
c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7
(13049344 bytes). The channel was verified with commit/hash/PE/origin checks.
There is no expiring artifact or Deck GitHub-auth requirement. CI advances the
rolling channel only after Windows, Linux and macOS gates; older late workflows
cannot roll it backward.

Mac lab controller is `contrib/arcade-mode/lab/controller.py`, Deck installer
and updater are `deck_installer.py` and `deck_updater.py`, and autonomous
virtual pad/screenshot runner is `lab/robot.py`. The controller offers
self-test, pair, read-only preflight, explicit deploy, status, collect, restore,
and numeric trace. Pairing uses a dedicated ed25519 key and known_hosts under
the user's HOME; password goes only to the SSH terminal. Host-key checking
stays strict. Preflight performs only reads. Deploy validates pinned release
metadata, refuses running processes, preserves the first stock DLL and previous
updater preference, atomically installs and verifies source/hash. Restore
requires stopped processes and recovers exact stock. Bootstrap runs its tests
before installing the HOME controller and never deploys automatically.
`LAB_PROTOCOL.md` and `lab/ROBOT.md` specify the interfaces and safety rules.

## Real hardware evidence

Diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 established a native
right-stick ollie baseline on this Deck and was restored to stock. That result
is at
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/bbd1def2eaf42945d43f4e0708574852eee3b817.json .
A private Gamescope frame showed Skate's “Ollie”; complete numeric capture
transitioned state/mode 100/1 -> 100/0 -> 103/0 -> 200/2 -> 201/2 -> 100/1.
A prior A-only board baseline was contaminated by an in-game traffic collision,
not an application crash; do not use it as a clean control.

For feature f669926, a fresh read-only preflight passed with the game closed,
stock and backup matching, and sufficient space. After explicit authorization,
`Lab.deploy` installed and verified the feature DLL, retained the backup and
disabled updates. ReSkate launcher showed READY and updates-off; Skate started,
`arcade_mode_initialized` was true, “Arcade Mode ready” logged, and B exited
Hub. Y mounted the skater on the board on a sidewalk. The bounded virtual-pad
`idle` recipe sent ONLY a 100 ms A tap with NO right-stick input. Four private
Gamescope frames showed crouch, Skate's native “Ollie” HUD, and board landing.
Complete native capture id 1 had 21 samples and the same state/mode sequence as
the ordinary right-stick ollie. A continuous 3000 ms A hold generated exactly
one additional complete capture id 2 (21 samples), with the same native
sequence, and no repeated rising edge. Skate did not unexpectedly crash.
A/B/Y observer events registered; a short 120 ms X tap was missed after virtual
pad hotplug, while a later one-second X hold registered. Do NOT claim that short
X tap passed.

Live evidence was saved before graceful game close. After both game processes
stopped, restore verified the exact stock SHA above, active=false and
updates=true. Final sanitized observer report has 13/13 PASS (holds,
native-controls and no-crash include operator observation):
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276.json .
Separate sanitized machine-readable A-specific result, with no full logs,
screenshots, credentials, IPs or personal paths:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276-ollie.json .
Private frames remain ignored under `artifacts/robot-feature-*`; Mac HOME
contains sanitized results at `~/.local/share/reskate-arcade-lab/results`.
No Mac controller bootstrap/pair/config was installed or changed in this run.
The Deck game DLL/updater were modified only for the expressly approved test
and restored; transient virtual input devices and Gamescope temporary frames
were created by the test.

## Exact next action and unknowns

A -> native-quality Skate ollie is VERIFIED for two bounded Deck inputs in this
single session. Next gameplay milestone: X -> one fixed native flip, followed
by grounded/air/grind/manual/landing/bail observation. First review this state
and the published result, then design the fixed X action against native input
evidence; run pure/CI gates before any new explicit deployment. The A mapping
still needs testing with physical controllers, varied terrain, repeated sessions
and online contexts. Short X taps near virtual-controller hotplug are
unreliable in this setup; use a settled pad or longer hold when testing.
No current external blocker. Never claim broader hardware compatibility from
this one session.

## Historical robot implementation progression (superseded by current handoff)
LATEST WORK (2026-10-12): user requested autonomous virtual controller input over
SSH with periodic screenshots. Start HEAD 80ec088f0906482dcff926798825deec5b9bef1d,
clean arcade-mode, origin pinned to notPho-Real/ReSkate. Strict read-only SSH
capability attempts failed: hostname resolution, then last known IP timed out.
No remote command ran. Deck input/capture permissions are UNKNOWN, not denied.
Implementing lab/robot.py: stdlib temporary Linux x86_64 uinput Xbox-style device,
fixed bounded experiments, neutralization/destruction on failure, same-owner
Gamescope SIGUSR2 screenshots via pidfd, private local frames, no auto publication
or deployment. No sudo, permission changes, system installation or game writes.
Upstream Linux uinput headers/docs and Valve Gamescope main.cpp confirm interfaces;
installed SteamOS version/support and Steam player assignment remain UNVERIFIED.
Tests are in progress. Exact next action: finish offline tests, then retry read-only
robot preflight; run a bounded test only when game is intentionally open/visible.
The diagnostic DLL remains undeployed pending the prior explicit deploy question.
UPDATE: after user woke Deck, real read-only SSH succeeded. Linux x86_64,
/dev/uinput character-device write access and Python pidfd support are VERIFIED.
Skate was closed. Display is same-user gamescope-wl, argv0 gamescope, SIGUSR2
handler installed. /proc/PID/exe returns EACCES; do not change permissions to read
it. Identity check uses comm+argv0+UID+caught handler and pidfd for signalling.
Input creation, game delivery and screenshots not yet verified. User asked to
open Skate at a skating scene. Offline 87/87 PASS, 12 Python syntax/11 shell syntax
PASS, C++ model/capture suite PASS; no CI run yet for this work.
REAL HARDWARE UPDATE: temporary neutral virtual gamepad successfully created using
the real Gamepad implementation. /sys/devices/virtual/input matching name was
absent before, present during, absent after (exit 0). No non-neutral input sent,
no game files changed. This proves rootless device creation/cleanup, not Steam
player routing or gameplay. New explicit `robot.py capture --output <new-dir>`
can verify display capture while Skate is closed; it never creates a controller.
REAL SCREENSHOT VERIFIED: this SteamOS emits /tmp/gamescope_<timestamp>.avif,
not PNG. First capture attempts correctly returned no image rather than using
stale files; adapted collector to PNG/AVIF with freshness, ownership, no-symlink,
size, complete-box and stable-metadata checks. Actual AVIF downloaded, hash-checked
and saved under ignored artifacts/robot-screen-01; converted locally with macOS
sips and visually inspected: Steam ReSkateLauncher.exe page, Play button visible.
No game running, so non-neutral input/Steam player routing and periodic in-game
capture still UNVERIFIED. Deck changes: transient neutral input device (removed)
and Gamescope-owned temporary screenshots only. No DLL/config/settings changed.
No Mac HOME installation changed; preview/evidence files stay inside repository
ignored artifacts. Exact next action: once Skate is at a skating scene, run robot
idle experiment, inspect four frames and actual response before any further input.
Latest local validation: 89/89 Python tests PASS (70 existing + 19 robot tests),
12 Python compile checks PASS, 11 shell syntax checks PASS, one C++ model/capture
executable PASS, git diff --check clean. Existing Linux/macOS CI discover the new
tests automatically; bootstrap now includes test_robot.py. CI/channel status will
be recorded after pushing; no workflow definition or runtime DLL code changed.
Implementation commit 367f67da09c686bc5755efdf5644aa552c539c33 pushed only to
origin arcade-mode. Hosted Linux/macOS and installer passed; Windows was building
at last check (development run 38334453685, installer 38334453702). Follow-up
review found screenshot-worker joining could delay neutralization after a replay
error. Fixed by neutralizing before joining and cancelling screenshot waits; two
regression tests added. Latest local suite 91/91 PASS, 12 compile checks PASS.
This supersedes the earlier 89-test count. User has not yet confirmed Skate open;
last real preflight still reports game closed. No non-neutral input has been sent.
SUBSEQUENT AUTONOMOUS STARTUP: under the user's request to perform tests remotely,
captured and inspected a fresh Steam screen showing existing ReSkateLauncher.exe
Play entry selected. Sent exactly one virtual A press (120 ms) through tested
Gamepad API after rechecking no Skate process, then neutralized/destroyed device.
The following screenshot shows a black launch transition. Launcher/game arrival
is not yet verified. No DLL deployment performed. This supersedes the preceding
"no non-neutral input" statement; no gameplay acceptance is inferred from it.
Launcher arrival now visually verified: ReSkate 2.0.7 window with UPDATE button,
not Play. Did not activate update. User EXPLICITLY APPROVED deployment of tested
diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 and continued automated tests,
including closing launcher and relaunching. This supersedes pending approval.
Next: verify stock/hash, close only launcher, pin immutable bbd1def release,
rerun installer preflight/deploy, relaunch, run bounded inputs/screenshots/traces.
DEPLOYMENT VERIFIED: immutable bbd1def manifest/hash/PE validated; two exact
same-user ReSkateLauncher processes asked to close via SIGTERM/pidfd. Fresh
preflight passed with both processes stopped. Shared Lab.deploy installed DLL
082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468, receipt active,
updates=false. Stock backup remains 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Deck now has DIAGNOSTIC development DLL, not stock. Prior stock/updater preference
preserved. Next: inspect Steam screen, relaunch existing shortcut, enter game,
verify runtime and virtual input delivery with bounded captures. No ollie feature
implemented; still observational native-request instrumentation.

NEW ACTIVE TASK: user authorized proceeding with A -> native-quality ollie.
LATEST VERIFIED BUILD: bbd1def2eaf42945d43f4e0708574852eee3b817. CI run
38313781791 PASS for Windows build/model, Linux, macOS, and gated Deck packaging;
installer run 38313781774 PASS. Earlier runtime-only run 38313325123 also PASS.
Public tested release was downloaded/validated in memory: channel commit bbd1def,
DLL SHA-256 082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468,
13045760 bytes. No deployment. Local Python tests 70/70 PASS, C++ model/capture
suite PASS normally and under ASan/UBSan, 10 Python syntax checks PASS, diff check
clean. No workflow definitions changed. Only local ignored research artifacts
were added outside tracked code; real Deck operations in this task were reads.
The user has been asked for explicit diagnostic deployment under the original
deploy-only safety rule. Pending that answer, Deck remains stock. After authorized
deployment: idle five-second A-armed capture, then A-armed ordinary stick ollie,
repeat, `controller.py trace`, and interpret numeric evidence. This is a hardware
input dependency, not completion of A-to-ollie. The unknown native request API
remains the primary blocker; never present this diagnostic as the gameplay feature.

Runtime probe commit: 270a532c01ad39ddfbcfbbe8515f5ed484b032a4, pushed only to
origin arcade-mode. CI 38313325123: Linux/macOS passed; Windows still running
at last check. Installer workflow 38313325113 passed. Follow-up adds read-only
`controller.py trace` collection with numeric allowlist and source/hash/freshness
binding; local suite now 70/70 PASS, including serialized fake-Deck trace and
wrong-DLL refusal. These supersede the earlier 65-test count for this new work.
The first implementation is a focused READ-ONLY native request probe, not an
ollie trigger. See NATIVE_OLLIE.md for exact static evidence and hardware protocol.
Two 32-byte contracts (rig crouch predicate RVA 0x476f130 and producer 0x1009e29)
match the actual binary; runtime disables the new probe on mismatch. A starts a
five-second capture limited to 128 changed samples, no game memory writes or
new hooks. Pure tests cover inactive capture, deduplication, ownership loss,
recovery, time expiry/backwards clock, cap and rearming. Local C++ model/probe
suite PASS with -Wall -Wextra -pedantic; existing Python simulations 65/65 PASS.
Next: Windows/Linux/macOS gates for this build, then explicit diagnostic deployment
and user-performed normal stick ollie to correlate native requests. No safe native
ollie command is established yet. Deck remains stock; never attribute old hardware
acceptance to this new probe build. No new real-target writes in this task.

Active task authorization context: user authorized proceeding after
observer acceptance. Work starts from f814aec2cbdbacfaee2582a5175cc546f8bcd0de,
branch arcade-mode, clean worktree. Native-path investigation is now in scope.
Read-only SSH copied installed Skate.exe to ignored repository artifact
artifacts/arcade-native-research/Skate.exe (144567168 bytes); SHA-256
fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9 matches the
existing 20260929 build contracts exactly. Never commit/publish this binary.
No new DLL deployed; Deck still stock. Existing trainer_jump.cpp changes no-comply
and boneless launch velocities, not an ollie trigger. Existing selector at RVA
0x4745a70 and context request flags are analysis anchors, not an established
ollie command. Next: trace native crouch/pop request producers and consumers in
this verified binary; do not force state IDs or invent a function signature.

Final sanitized report timestamp: 2026-10-12T06:31:01Z; source build
f09872f179434cd3d474c9952b8325363424c308, installation ID
0c790805de1b44d69785aaacac95493e. Report:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
All 13 criteria PASS, observer_acceptance=PASS. Runtime process/initialization/
ready/fresh log and A/B/X/Y events came from actual SSH evidence; normal controls
and no crashes during the session were user-reported. Holds were user-executed
and verified by exact +1 counter deltas on all four buttons. This is bounded
session evidence, not a guarantee against future crashes.

After user closed the game, restore verified both processes stopped, no pending
recovery, receipt inactive, and installed DLL AND preserved backup both equal
original SHA-256 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Updater preference restored to true. The final collector correctly retains prior
runtime evidence for this same installation ID; reskate_process_seen=true in
the combined report describes the tested session, not a currently running game.
No game is running at restoration; the Deck currently has STOCK ReSkate, not
the development DLL. Do not redeploy implicitly. Tested release remains f09872f;
documentation changes do not advance the rolling channel. Report publication
changed arcade-lab-results only; STATE.md handoff belongs on arcade-mode.

No current infrastructure/hardware blocker. Exact next development milestone is
A -> native-quality Skate ollie, but native trick reverse engineering is outside
this completed task and requires a new task instruction. Following milestones:
X -> one fixed native flip, then grounded/air/grind/manual/landing/bail observation.
Historical intermediate notes below preserve provenance and are superseded by
this current handoff wherever they describe incomplete pairing/testing.

## Historical deployment and acceptance progression (completed; see current handoff)
All controlled button holds VERIFIED: after user's A/B/Y test the actual counters
are A=16/B=9/X=8/Y=6, exactly +1 each for A/B/Y with X unchanged. Combined with
the prior controlled X test, all four holds generated one rising edge and no
repeats. User confirms normal controls work and previously no crashes so far.
Published report 2026-10-12T06:29:45Z has 11/13 criteria PASS; only stock_restore
and updater_restore remain UNVERIFIED. Overall acceptance correctly stays
UNVERIFIED until restoration is checked. Same source/installation ID and result
URL as below. Holds are marked operator-reported in schema and additionally
supported by the controlled before/after counts documented here.
Exact next action: user exits Skate AND launcher; verify both stopped, run the
existing restore operation, verify original stock hash and updates=true, then
collect/publish the combined report for this same installation. Do not stop the
user's game remotely or replace any DLL while either process is running.

Controlled X hold VERIFIED on real hardware: user released all buttons, held X
once for three seconds, released, and reported done. Read-only log comparison:
X event count/latest counter rose from 7 to 8; A=15, B=8, Y=5 unchanged. Exactly
one X rising-edge event occurred; no repeated events during this controlled hold.
Overall holds remains UNVERIFIED until equivalent A/B/Y holds are checked.
Next action: from current A=15/B=8/Y=5 baseline, user separately holds each once
for three seconds with full releases; expected counters A=16/B=9/Y=6, X=8.

Latest report 2026-10-12T06:26:53Z: user reports no crashes so far; collector
published no_unexpected_crash=PASS as OPERATOR-REPORTED, bringing total to 9/13
PASS, 4 UNVERIFIED. User says holding three seconds produced no new notification;
this is ambiguous between expected absence of repeats and absence of initial edge,
so holds remains UNVERIFIED. Read-only current-session event counts: A=15, B=8,
X=7, Y=5, each latest counter equal to event count. Counts alone cannot establish
which events correspond to holds. Code emits only rising edges, not at three
seconds. Next controlled check: release all buttons, hold X once for three seconds,
release, and compare X counter against baseline 7 (avoid other X presses).
Native controls and actual stock/updater restore remain unverified.

REAL RUNTIME EVIDENCE obtained 2026-10-12T06:24:45Z after user launched Skate.
Actual SSH collection verifies running Skate, fresh session log, Arcade Mode
initialized active, ready marker, and ALL A/B/X/Y observer events: 8/13 criteria
PASS. Remaining 5 criteria UNVERIFIED: holds without repeated rising edges,
native controls, no unexpected crash over test session, exact stock restore,
updater setting restore. Overall observer acceptance remains UNVERIFIED.
Sanitized report successfully published through the tested collector to
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
Publication changed only arcade-lab-results, without force; no raw logs published.
Local report is under ~/.local/share/reskate-arcade-lab/results/ with the same SHA
filename. Current next action: verify each three-second hold produces one edge,
obtain operator confirmation of normal controls/no crash, then close game and
perform explicitly authorized stock restoration to verify the last two criteria.
Do not redeploy or restore while the game is running. Historical pending-launch
statements below are superseded by this actual runtime result.

USER-AUTHORIZED REAL DEPLOYMENT COMPLETED after the preflight below. The user
explicitly requested updating the Deck to our build and testing it. Actual
`controller.py deploy` exited 0 with `DEPLOY VERIFIED`: installed commit
f09872f179434cd3d474c9952b8325363424c308 and destination DLL hash
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01` match
the verified release. First stock backup hash matches the pre-deploy DLL exactly:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
Receipt exists and active=true, updates=false (previous setting true), no recovery
pending. Both game and launcher stopped at deployment verification. Game files
HAVE NOW BEEN CHANGED by this explicit deployment; prior no-change statements
below describe earlier preflight only. No game launch or runtime acceptance yet.
Exact next action: user launches Skate normally and exercises A/B/X/Y taps and
holds; collect actual fresh runtime evidence over SSH. Do not deploy again just
to collect. Restore and updater restoration remain unverified on hardware.

After the user completed interactive pairing, the actual controller `preflight`
exited 0 and printed `PREFLIGHT PASS — NO GAME FILES CHANGED`. This verifies
noninteractive dedicated-key SSH with strict host checking on the real Deck.
Authenticated user/host: deck/steamdeck. Discovered Steam library:
`/home/deck/.local/share/Steam`; game:
`/home/deck/.local/share/Steam/steamapps/common/Skate`.
Skate.exe, ReSkateLauncher.exe and ReSkate.dll all present; safe paths pass.
Neither Skate nor launcher running; process check complete. Current DLL SHA-256:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No stock backup, Arcade receipt or config exists; no recovery pending; Arcade
inactive and installed commit unknown. Launcher settings readable; updater true.
Free bytes 24475693056, required bytes 220581888. Verified latest tested release
is f09872f179434cd3d474c9952b8325363424c308, expected development DLL SHA-256
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01`.
Pairing necessarily created local SSH identity/trust and authorized the public key
on Deck; this preflight performed only reads. No game files were changed and no
development DLL deployed. Runtime observer, native controls and restore acceptance
remain UNVERIFIED on hardware. Exact next action: obtain an explicit deployment
operation from the user before any game mutation, then intentional launch and
observer validation. Do not repeat pairing or diagnose old reachability failures.

Current tested implementation/release: f09872f179434cd3d474c9952b8325363424c308.
Final HEAD is a subsequent STATE.md-only handoff commit marked [skip ci]; use
`git rev-parse HEAD` for its exact SHA. It does not change the tested code or advance
the release. No essential continuation state depends on the conversation.

## Boundaries and starting state
Only `notPho-Real/ReSkate`, branch `arcade-mode`. Never main/master, upstream,
upstream PRs, root on Deck, SteamOS read-only changes, Skate.exe replacement,
manual shortcuts.vdf edits, password storage, or implicit deployment.
Starting HEAD: f46994894addece6b6c111bae29e474ae9e929f2.
The clean worktree initially used arcade-stage-02-state-probe. arcade-mode was
fast-forwarded from d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc to preserve both newer
commits (b62fe7c isolated stage CI; f469948 read-only native-state correlation).
Current HEAD: use git rev-parse HEAD; this document cannot contain its own commit SHA.
Last known tested release SHA before this run's push:
d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc; Windows/Linux Actions runs
38176087200 and 38176087218 succeeded. Public channel/archive/commit/PE validation
was executed in memory: DLL SHA-256
5a3d6a79fe8e8c2662e570dd5fad68c41f8d2fa551ec9e647d88ed89e1ed05ca,
13,039,616 bytes. This baseline predates the new macOS gate. No hardware evidence.

## Architecture and verified repository facts
Arcade Mode lives natively in ReSkate.dll: stock ReSkate has no arbitrary
Plugins/*.dll loading contract; custom Lua is explicitly disabled by
Extension/Scripting/custom_script_loader.cpp custom_scripts_enabled=false.
Runtime/runtime_init.cpp calls start(base), records arcade_mode_initialized;
Runtime/client_tick.cpp invokes on_client_tick before the 500 ms model throttle.
CMake includes runtime sources and optional standalone arcade_mode model tests.
The observer uses DingoSDKOverlayReadControllerInput, bits A/B/X/Y =
0x1000/0x2000/0x4000/0x8000. Rising edges log and notify individually; holds do
not repeat. Input unavailable clears held mask (refocus can produce a new edge).
Input requires focus and a closed interactive overlay. No suppression or trick
injection. Existing watched_physics_state diagnostics are sampled, not a
transactional gameplay event stream; preserve them without extending research.

Pure arcade_model.h owns combo, saturating points/multiplier, bank/bail,
balance, correction/drift and fatigue persistent throughout an unbanked combo.
Grace is informational pending actual landing events. None drives Skate yet.
Intended controls: A=ollie, X=flip/trick family, B=grab, Y=grind/context.
Future knockdown: host-authoritative ATTACK/KNOCKDOWN validation and native bail;
reject stale/duplicate/unauthorized requests, not rigid-body player collision.
Major risk: commanding Skate's native trick state machine cleanly.
Next gameplay milestone AFTER observer hardware acceptance: A -> native-quality
Skate ollie, then X -> one fixed native flip, then observe grounded/air/grind/
manual/landing/bail. No trick reverse engineering belongs in this lab task.

## Existing build and deployment
Windows CI builds Release x64 ReSkate.dll and pure model tests. Linux tests
package the offline installer. Immutable arcade-build-<40-char-SHA> releases
feed arcade-deck-latest/channel.json; pinned fork, archive SHA, DLL SHA, commit
checks. Serialized publishing plus branch HEAD check prevents late rollback.
Existing deck_installer.py preserves first stock backup and updater preference;
deck_updater.py fetches public releases without Deck GitHub authentication.
Fixed: old deploy_to_deck.py --pair now returns after pairing, never deploys.

## Evidence and acceptance
Actual log code: Engine/Core/Log/paths.cpp chooses game/logs/ReSkate.log, with
LocalAppData/ReSkate/logs fallback; RESKATE_LOG_DIRECTORY can override. Logger
truncates at session start in the default path. Crash marker comes from
Engine/Core/Log/crash.cpp: ReSkate unhandled exception. Do not invent a Proton
prefix path or publish full logs. Runtime/runtime_state.cpp routes record() to
logging::event. Important review correction: Engine/Core/Log/events.cpp formats
events as text (`Arcade mode initialized: active=true`); raw JSON is trace-only.
The original record() default is debug and hidden at the launcher's normal info
level. Runtime/runtime_init.cpp now emits this one event at info, and the collector
matches its actual text (also supports trace JSON). No gameplay behavior changed.
Hardware acceptance requires actual startup, initialized active, ready, each
A/B/X/Y tap observed, no repeat on holds, normal native controls, exact stock
restore and previous updater preference restored. All UNVERIFIED on hardware.
Absence of a crash log alone cannot prove no unexpected crash.

## Lab implementation
lab/controller.py provides self-test, pair, preflight, deploy, status, collect,
restore and test-first bootstrap. Seven .command actions expose setup/operations.
HOME destination is ~/.local/share/reskate-arcade-lab; dedicated SSH key and
known_hosts live there. No keys were created during development. SSH host policy
is ask only during pairing, strict yes thereafter. Password goes only to SSH tty.
lab/probe.py is a separate read-only payload sent into Python -B memory over SSH;
tests inspect the generated code and run it against fake HOME/library fixtures.
Deploy validates pinned release/archive/commit/DLL hashes and x64 PE, preflights
twice and uses the local trusted shared installer, not downloaded Python code.
Shared installer now uses nonblocking flock, exclusive immutable backup, fsynced
rollback journal, atomic writes, destination verification and interrupted recovery.
Legacy shell helpers are migrated to this same lock/transaction through manual_install.py.
Results allowlist booleans/enums/source SHA/DLL hash/time and unique installation ID.
Log baseline/freshness and latest-ready scoping prevent old/combined-session evidence.
Holds/native behavior/no-crash require labeled operator observations, never inference
from absence. Post-restore collection can join only the same installation's evidence.
GitHub Git Data API publishes only sanitized results to orphan arcade-lab-results;
updates are fast-forward only, no force push, no development checkout mutation.
Publication failure leaves local JSON and installation untouched.
LAB_PROTOCOL.md documents all interfaces, operation semantics and recovery details.
arcade-dev.yml now gates deck packaging/publication on build, linux-tests, macos-lab.

## Tests and remaining milestones
Final offline suite after actual log-format correction: 65 tests passed,
0 failures (macOS execution, fake targets).
This includes actual generated SSH payload execution, exact stock restore, AST
preflight non-mutation, failed writes, flock contention, bootstrap failure gating,
stale log refusal, mocked GitHub publication, an actual child-process exit during
transaction, explicit fallback-log baselining, and legacy shell round-trip.
Python compileall: 10 files passed. Shell bash -n: 11 files passed.
Standalone C++20 model/observation executable: 1 passed, 0 failed.
Both workflow YAML files parsed; explicit needs/build/Linux/macOS runner assertions
passed. Git diff whitespace check passed. Report/channel/manifest schema rejection
and allowlist tests passed within the 65-test suite.
Implementation committed and pushed only to origin arcade-mode:
b4fb40f37acc52f99d61da562b8e43b8a63fdf48, "Add tested Arcade Lab controller, safe
deployment and hardware evidence protocol" (22 files). Credential/key/log audit passed.
Hosted run 38199528643 (separate Linux Deck workflow) passed. Run 38199528667
(Windows + Linux + macOS release-gated workflow) passed all four jobs.
Logging correction committed/pushed as e39227940ef9f536b61fd425bea0cd2823bc31a2.
Its run 38199870622 passed, including Windows and the release job.
Separate Deck run 38199870610 passed. Final self-test presentation safeguard
suppresses fixture success messages (so fake LAB READY cannot confuse users) and
refuses incomplete test installations. Committed/pushed as
f09872f179434cd3d474c9952b8325363424c308 (final implementation source).
Final local rerun: 65/65 passed, compileall and diff check passed.
Its gated workflow is [38200147202](https://github.com/notPho-Real/ReSkate/actions/runs/38200147202):
all four jobs passed. Actual job logs confirm Windows x64 build + CTest 1 passed,
0 failed; linux-tests 65 passed, 0 failed; macos-lab 65 passed, 0 failed;
deck packaging/release tests 65 passed, 0 failed; publication step succeeded.
Separate [Linux Deck run 38200147179](https://github.com/notPho-Real/ReSkate/actions/runs/38200147179)
passed another 65 tests, 0 failed. No hardware or user credentials used by CI.
Earlier run 38199528667 completed all four jobs successfully. Its publication log
explicitly said "Newer arcade-mode commit exists; leaving the update channel
unchanged." Public channel remained d83e1e6, verifying late-workflow protection
in a real hosted run (not a simulated Deck result).
b4fb40f/e392279 are superseded implementation checkpoints.

## Verified release and channel
Public channel was downloaded and validated after final CI success:
`arcade-deck-latest/channel.json` points to source
f09872f179434cd3d474c9952b8325363424c308 and immutable release
`arcade-build-f09872f179434cd3d474c9952b8325363424c308`.
Installer archive SHA-256:
9ff9be15724f977ebc7b3e9bd26c09f33e1657c2df516a307296317e4c5103c4.
DLL SHA-256:
d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01.
DLL size: 13,041,152 bytes. Public archive hash, pinned origin/URL, embedded source
commit, DLL hash and x64 PE validation all passed using verified_release().
Bytes were verified in memory only; no DLL was deployed to the user's Mac or Deck.

## SIMULATED/CI VERIFIED versus REAL HARDWARE VERIFIED
SIMULATED/CI VERIFIED: all local/hosted tests above, fake Deck exact stock-byte
round trip, preferences, interruption recovery, non-mutating preflight, fake SSH,
pairing idempotency, release rejection, sanitized collection and GitHub API mocks.
Real GitHub CI/release integrity and late-workflow protection were also verified.
REAL HARDWARE VERIFIED: NONE. No real Deck SSH connection, preflight, pairing,
installation, game launch, controller event collection or restore occurred.
No persistent Mac lab installation or key was created. Outside the repository,
only temporary test fixtures/tool temporary files were used; fake HOME trees were
cleaned up. Repository-local ignored tools contain the standalone model executable.
No keys, credentials or full runtime logs are tracked.

## Historical setup blockers and actions (resolved; see current handoff)
The following records earlier states, not current blockers or instructions to repeat.
Follow-up after the user reported the Deck powered on: attempted read-only TCP/SSH
to deck@steamdeck.local. Both the socket lookup and actual OpenSSH attempt failed
to resolve steamdeck.local (SSH exit 255). No authenticated connection or Deck-side
command occurred, and no Deck files changed. The default Mac lab directory
~/.local/share/reskate-arcade-lab does not exist, so its dedicated SSH key and
known_hosts are also absent. No pairing or bootstrap was run during this attempt.
Immediate next action: obtain the Deck's current Wi-Fi IP and confirm Mac/Deck are
on the same network; retry read-only connectivity to that specific address. Pairing
will still be required before authenticated preflight unless a valid project key
is subsequently provisioned. Do not bypass host-key verification or deploy.
Subsequent user network-settings photo supplied a concrete private IPv4 address.
Targeted TCP and SSH attempts to that address returned "No route to host";
SSH exited 255. The Mac has an address in the same /24, a direct route via en0,
and an ARP neighbor entry whose MAC matches the photographed Deck. Two targeted
pings received no replies; a second SSH-port check also failed. This establishes
an addressing/neighbor match, NOT authenticated SSH or hardware acceptance.
Possible blockers remain Mac local-network permission, network filtering/isolation,
or Deck availability. Do not assert any one cause as proven. No network settings,
host keys, pairing state, or Deck files were changed. Current private IP/MAC are
omitted from tracked state; obtain the latest address from the user if needed.
Router comparison subsequently performed at the user's request: read the actual
default gateway from the Mac route table, then tested router and Deck in parallel.
Router: 3/3 ICMP replies, 0% loss, TCP port 80 reachable. Deck: 0/3 replies,
TCP port 22 still reports "No route to host"; neighbor MAC still matches photo.
Therefore this process DOES have working local-network access to the router;
a blanket Mac local-network permission block is not supported by these results.
The Deck-specific reachability failure remains unexplained; no authenticated SSH
has occurred. Next diagnosis must distinguish Deck availability/current address
from host-specific filtering or Wi-Fi client isolation, without changing settings
or assuming the router's cached neighbor entry proves the Deck is currently awake.
Important subsequent correction: user enabled Codex's Mac Local Network permission
and supplied a photo showing it on. Immediate retry then received 2/2 Deck ping
replies and successfully connected to TCP port 22. Router reachability had NOT
ruled out this permission issue; do not repeat that inference. Network reachability
and an open SSH port are now verified on the real Deck, but not authentication or
game/runtime behavior. Strict SSH stopped with an unknown ED25519 host key; the
dedicated lab private key is still absent. No host key was accepted or stored,
no public key added, and no remote command/game operation ran. Next prerequisite
is explicit first-time pairing with user host-key verification and the Deck password
entered directly into SSH's terminal prompt, followed by read-only preflight.
Latest pairing follow-up: user reported a launcher permission error. The repository
Pair Steam Deck.command is owned by the current user, mode 0755, with no quarantine
attribute; the source of that reported error is not established. Opened this exact
launcher with macOS `open -a Terminal`, which returned success. A subsequent
metadata-only check confirms the dedicated HOME lab directory and private-key file
now exist, so local pairing setup has started. Key contents were not read. Terminal
UI inspection was refused by the computer-use tool; do not bypass that restriction.
Host trust, successful authentication and authorized_keys changes are NOT verified.
Next action: user completes the visible SSH prompt in Terminal (never sends the
password in chat), then run read-only preflight. No game deployment was performed.
Follow-up connection check after the user's key-creation question: actual SSH with
the dedicated key, BatchMode=yes and StrictHostKeyChecking=yes again reached host
verification and stopped because steamdeck.local has no trusted ED25519 host key
in the dedicated known_hosts. Authentication was not attempted; key acceptance
by the Deck remains unverified. Local key creation is complete, first host trust
is still incomplete. Do not conflate these steps or regenerate the existing key.
Further local cryptographic verification: ssh-keygen successfully parsed the
existing private key; its derived public key exactly matches the saved public key.
Private-key permissions are 0600. No key material was printed. The dedicated
known_hosts file does not exist. A strict, noninteractive SSH test to the user's
supplied numeric Deck address also stopped at unknown ED25519 host identity
(exit 255), before authentication. Thus the key pair is verified valid locally,
but Deck authorization remains untested until first host trust is completed.
Old tools/arcade-deck is empty; no existing project SSH credentials were available
there for a read-only preflight. Deck reachability, SSH enablement, correct game
directory, first backup being genuine stock, actual Proton log path, focus/controller
delivery, native behavior and absence of crashes remain unverified assumptions.
Publication transport is tested with mocks; no live hardware result was invented
or published, and arcade-lab-results will be created only by an actual collection.
Mac gh authentication/publication permission will be checked when collecting.

After resolving reachability, the setup action is to double-click
`contrib/arcade-mode/Install Arcade Lab.command`. It reruns all 65 offline tests,
installs under HOME, optionally pairs via SSH's password prompt, then performs ONLY
read-only preflight. It must print LAB READY before proceeding. Pairing is optional
at setup but necessary before strict SSH preflight if no key was authorized.
The user then chooses Deploy Latest Arcade Mode explicitly, launches Skate, tests
A/B/X/Y taps/holds/native controls, collects, closes the game, restores stock, and
collects again. See LAB_PROTOCOL.md for action locations and precise semantics.
Do not begin A/ollie native reverse engineering until observer acceptance is backed
by real evidence. Following gameplay milestone remains A -> native-quality ollie,
then X -> one fixed native flip, then complete grounded/air/grind/manual/landing/bail
observation. Primary technical risk remains commanding Skate's native trick state machine.
