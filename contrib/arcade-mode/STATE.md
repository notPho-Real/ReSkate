# Arcade Mode persistent project state

Updated 2026-10-12 UTC (2026-10-11 local) after real Deck observer acceptance.
Read this file before work. Infrastructure implementation and the current observer
hardware milestone are complete: 13/13 acceptance criteria PASS.

## Current handoff: hardware acceptance PASS, Deck restored to stock
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
