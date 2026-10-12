# Arcade Mode persistent project state

Updated 2026-10-11 during Arcade Lab implementation. Read this file before work.

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
logging::event, so initialized active and ready are in the same log mechanism.
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
Final pre-push offline suite: 63 tests passed, 0 failures (macOS execution, fake targets).
This includes actual generated SSH payload execution, exact stock restore, AST
preflight non-mutation, failed writes, flock contention, bootstrap failure gating,
stale log refusal, mocked GitHub publication, an actual child-process exit during
transaction, explicit fallback-log baselining, and legacy shell round-trip.
Python compileall: 10 files passed. Shell bash -n: 11 files passed.
Standalone C++20 model/observation executable: 1 passed, 0 failed.
Both workflow YAML files parsed; explicit needs/build/Linux/macOS runner assertions
passed. Git diff whitespace check passed. Report/channel/manifest schema rejection
and allowlist tests passed within the 63-test suite.
Exact next action: commit the reviewed implementation and push only origin
arcade-mode, then inspect hosted Windows/Linux/macOS results and public releases.
Update this section with their real outcome before handoff. No hardware acceptance
may be inferred from CI. Old tools/arcade-deck directory is empty; no existing
project SSH credentials were available there for read-only hardware preflight.
No real Mac installation, key creation, pairing, or Deck access has occurred.
Filesystem writes require execution approval in this session (read-only sandbox).
