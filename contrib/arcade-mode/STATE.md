# Arcade Mode persistent project state

Updated 2026-10-11 after Arcade Lab implementation and hosted CI/release verification.
Read this file before work. Implementation is complete; real hardware acceptance
has NOT been performed.

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

## Blockers, assumptions and exact next action
No remaining implementation/CI blocker. Hardware acceptance requires the user.
Old tools/arcade-deck is empty; no existing project SSH credentials were available
there for a read-only preflight. Deck reachability, SSH enablement, correct game
directory, first backup being genuine stock, actual Proton log path, focus/controller
delivery, native behavior and absence of crashes remain unverified assumptions.
Publication transport is tested with mocks; no live hardware result was invented
or published, and arcade-lab-results will be created only by an actual collection.
Mac gh authentication/publication permission will be checked when collecting.

Exact next user action: wake the Deck and double-click
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
