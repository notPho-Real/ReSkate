# Arcade Mode

Arcade Mode is experimental and built inside ReSkate.dll on this fork. The live
preference is available at **Pause → Mod Options → Player → Arcade Mode** and
through `arcademode`, `arcademode on`, and `arcademode off`. It defaults OFF and
persists through the normal local preference store.

OFF preserves native controller input. A transition cancels synthetic gestures,
clears combo/HUD state and requires release before held commands can rearm. Native
menus, lost focus and stale observations suppress translation. The real Deck
passed ON → OFF → ON in one process on source 3634e30: A produced an observed
native air/landing sequence in both ON trials and stayed native while OFF. See
[the current checkpoint](../../contrib/arcade-mode/STATE.md).

Held shoulder spin is the default. `arcadetapspin on` saves tap mode; `off`
restores held mode. Tap mode accumulates signed half turns and advances them on
actual qualified physics steps, retaining the full last step on overshoot.
Directional spin requires a physical hold longer than100ms. The live ordinary-air
path uses the existing body/target writer and original owner/flight proof; legacy
stick/prewind recipes are outside that path. NoSpin, orientation-control and
transfer flags remain explicitly unknown without native producers. Their full
contextual behavior, native handedness and player acceptance remain unverified.

## Controls under development

The A/X adapter uses the game's authored gestures. A hold/release has recorded
20-trial acceptance; several exact native flip IDs are discovered, but flip
repeatability remains incomplete. The unsaved `arcadeplayback` experiment compares
delivery, timeline and reference scheduling. See the current checkpoint for
installed source identity and the controlled Deck comparison. Delivery and
reference each produced five exact IDs in eight initial trials; one reference
trial had incomplete evidence. Neither mode has repeatability acceptance.

Automatic skating uses left-stick movement intent and the existing native held
push adapter. `arcadeautokick on|off` saves the ordinary-ground preference;
it defaults ON. With it OFF, hold X to request a push. Ordinary ground drive
and resistance use the existing guarded body writer, preserve vertical and
relative body velocities, and credit observed native speed loss. Physical Down,
turning, reverse-facing motion and low speed select the brake policy. Missing
support-plane and locked-vector facts do not select their special branches.
Native resistance remains authoritative when it exceeds the selected budget.
Manual, rail, air, bail, menu, owner loss and stale evidence exclude ground drive.

LB+RB within 120 ms requests one native board toggle from a verified safe riding
or grounded offboard state. Both bumpers must release before another request;
held or late chords cannot repeat. Individual bumpers retain native behavior.
Recorded board-toggle evidence includes 20 long and five quick cycles; preserve
that result while testing interactions with later controls.

## Remaining action plumbing

The components below are tested by the existing Arcade test target. With Arcade
Mode enabled, the new startup `classic` action selection maps RT to the existing
native revert recipe in the inspected regular context. LT+RT and uninspected
stances remain unavailable; LT still selects authored nollie A/X. The recipe has
native moving-context evidence, while this new default selection awaits Deck
acceptance. Exact side identity and monetary scoring remain separate requirements.

The unsaved `arcadeactions classic|off|observe|grabs|revert-fs|revert-bs` selector
changes only while Arcade Mode is OFF. `observe` reports B/Y intent; `grabs`
selects deferred grab candidates, and the two side selectors preserve the existing
revert experiments. These selections do not enable Arcade Mode themselves.

- `arcade_input_events.h` records one bounded circular event history per controller.
  `arcade_input_dispatch.h` applies context tables, usage masks and bounded queues.
  `arcade_input_session.h` connects those requests to native execution adapters;
  recognized input alone never proves an action. Unsupported balance branches
  remain explicit and cannot install a native pose or award combo credit.
  The optional lip context likewise requires independent admission and has no
  native execution adapter. It is never inferred from a rail request.
  Tests retain frozen baseline recognizers only under `Test/ReferenceInput`;
  production contains no legacy decoder path. Identical traces exercise both
  paths, with separately tested source-style matching and queue contracts.
- `arcade_action_plan.h` contains 12 independently written grab candidates from
  the installed native guide. Ordering is documented; 50 ms stages and 300 ms holds
  are experimental timing, not measured acceptance. Missing named methods and
  uninspected stances remain unavailable.
- `arcade_revert_input.h` emits the sourced 235 ms native revert candidate with
  stance/nollie context. It emits no native recognition or combo credit.
- `arcade_action_dispatch.h` arbitrates these candidates with explicit ownership
  for sticks, triggers and face buttons. It requires eligible context and cancels on stale
  context, landing, conflicting ownership or time reversal. The shared input session
  connects it to the remap with packet progression and ordinary input on unowned
  channels, preserving the physical camera path. The inspected clear-bit stance
  is required. Native crouch state 103 stays eligible during revert setup; manual
  eligibility has its own deadline so it cannot accidentally cancel that setup.
- `arcade_action_observation.h` correlates bounded requests with independent native
  events. Event sequence numbers must describe actual native occurrences, not
  repeated polling of a persistent selection. Only an observed revert may feed
  the existing combo continuation hook; request delivery never does.

`arcade_manual_input.h` retains native manual actuation and contact settling;
the session journal recognizes directional commands. Balance corrections cannot start a new manual while already in
one; confirmed takeoff, incompatible poses and competing commands release
ownership. New command repeatability and combined gameplay still need Deck tests.

Basic ground Manual/Nose balance can start before a history name appears. The
existing native gesture must first be delivered on two-wheel ground; a later
fresh physics step must acquire the requested wheel contact under the same full
owner. A push, existing one-wheel pose, expired delivery or reused command cannot
start balance. The acquired section uses the existing correction, failure and
neutral-putdown paths. Direct airborne-to-one-wheel and advanced entries retain
the separate history admission route; no native animation phase is inferred.

Continuous sanitized HUD-label appearance tracking supplies independent evidence.
Persistent rereads never emit another occurrence; ambiguous histories, coalesced
edges and stale samples are explicit. These edges are collected but do not yet
extend combos: native HUD slot semantics still need verification.

The source-derived score observer also reads line score and multiplier from native
HUD fields identified in upstream PR #217. Only validated numbers and per-field
availability enter telemetry. Local ownership, formatting on Deck and score/bank
semantics remain unverified; presentation values do not create combo events.
The pinned fork survey also found reusable grind-state readers and native bail
requests. See `contrib/arcade-mode/research/upstream-method-survey.json` before
starting new reverse engineering of those mechanics.

Basic Y now holds native neutral alignment or partial nose/tail pressure through
air-to-rail contact. A/X out selects the existing authored from-grind patterns;
there is no ground-pattern fallback. Actual engagement, named variants and exit
reliability still require Deck verification. Physical balance-failure actuation
remains outstanding. LB/RB translate continuous airborne rotation and crouched prewind
to native LS, after the existing120ms board-chord window. A held pair wins;
release, landing, bail, menus and conflicting owners release rotation. The
native method produced BS/FS180s and360s on Deck (six trials, all sketchy);
the new shoulder mapping itself still requires installation and Deck acceptance.
Production RT context selection, native score amounts and persistent
telemetry transport still require integration. The existing model remains in
place. The HUD now uses 30% axis lengths and visible danger zones, driven by its
published balance position; full Gamescope screenshots remain required.

`contrib/arcade-mode/lab/chain_report.py` grades the eight mandatory chain evidence
contracts offline. It requires observed native outcomes, continuous event
sequences, coherent combo/fatigue/banking state, recovery, and at least 20 passing
trials across two contexts per chain. The live semantic telemetry adapter is
pending; the synthetic fixtures test the grader only. Existing raw trial logs
cannot be promoted to chain acceptance by supplying requested action names.

## Read-only observations

The client tick records bounded native state, button edges and numeric speed
observations. Button diagnostics are logs, not player-facing notices. Native
physics-step observations and score signals are evidence; requested synthetic
input is never proof that a named trick succeeded. Full captures and local
machine details remain outside tracked project content.

## Independent model and tests

`arcade_model.h` is header-only, with no Windows or ReSkate dependencies. `Model` owns `State`
and sanitized `Tuning`. First trick starts multiplier 1, further tricks add one, and bank returns
raw points times multiplier then resets; bail resets without points. Integer scores saturate on
overflow. Fatigue grows throughout an active combo, persists through balance exit/re-entry, and
increases on links. Drift scales with fatigue; signed correction acts along the balance axis.
`tick()` returns true and bails when absolute balance reaches 1. Initial balance at a limit also
bails. Balance requires an active combo. Nonfinite inputs become zero, dt is bounded to [0, 1]
seconds, fatigue to [0, 10], and age to one day. Grace counts down but does not automatically
bank: future observed landing/bail events will decide settlement. The current experimental runtime observes native poses and publishes balance state;
physical failure, exact trick identity and complete combo acceptance remain unverified.

Defaults: grace 0.45 s, fatigue/second 0.08, fatigue/link 0.12, restoration 1.8,
drift 0.20, fatigue drift gain 0.85.

```sh
cmake --preset vs2022-x64 -DDINGOSDK_BUILD_ARCADE_MODE_TESTS=ON
cmake --build --preset release --parallel 4 --target dingosdk_runtime dingosdk_arcade_mode_tests
ctest --test-dir build/vs2022-x64 -C Release -R "^arcade_mode$" --output-on-failure
```

The pure model can also be tested without configuring the Windows runtime:

```sh
c++ -std=c++20 -I. Extension/ArcadeMode/Test/arcade_model_tests.cpp -o /tmp/arcade_model_tests
/tmp/arcade_model_tests
```

## Development artifact and Steam Deck

For the graphical, single-file Deck installer, see [Steam Deck installation](../../contrib/arcade-mode/README.md). It handles backup and the update setting automatically. The commands below are the manual alternative.

The `Arcade Mode development` Actions workflow runs on pushes to `arcade-mode` and manual
dispatch on that branch, only in `notPho-Real/ReSkate`. Windows 2022 builds and tests Release x64.
In the successful run's **Artifacts** section, download `ReSkate-arcade-dev-<full commit SHA>`
(30-day retention). It contains exactly `ReSkate.dll`, `ReSkate.dll.sha256`, and `commit.txt`.
Tested builds also publish immutable GitHub Releases after Windows, Linux and
hosted macOS lab gates pass. See contrib/arcade-mode/LAB_PROTOCOL.md. For manual
artifact installation, extract the ZIP, check `commit.txt`, and run
`sha256sum -c ReSkate.dll.sha256` from the extracted directory before installing.

On Steam Deck / Proton, `ReSkateLauncher.exe` and `ReSkate.dll` sit beside `Skate.exe`.
Turn **Install ReSkate updates OFF** before testing, or the normal updater may overwrite the
custom DLL. Close the game and launcher, then from this checkout:

```sh
bash contrib/arcade-mode/update_arcade_dev.sh "/path/to/Skate" "/path/to/extracted/ReSkate.dll"
bash contrib/arcade-mode/restore_stock.sh "/path/to/Skate"
```

The helper requires Bash, Python 3 and `pgrep` (standard Deck tools). It validates the supplied
PE x64 DLL header, saves the current DLL once as `ReSkate.dll.arcade-stock.bak`, and atomically
replaces the DLL in the same directory. Start with a known stock DLL: the script cannot establish
its provenance. Subsequent installs never overwrite that backup; restore retains it. Both refuse
running game/launcher processes, symlink destinations, or a concurrent installation. Keep the game
closed until completion. The shared installer now uses a persistent regular flock
file and recovery journal; do not remove a live lock. See LAB_PROTOCOL.md for
interrupted-install recovery. Header validation is not an authenticity check;
use the artifact hash and commit from your successful run.

Keep the normal launch option, approximately:

```sh
bash -c 'exec "${@/EAAntiCheat.GameServiceLauncher.exe/ReSkateLauncher.exe}"' -- %command%
```

The permanent **Update Arcade Mode** and **Restore Stock ReSkate** buttons now handle
future tested builds. CI publishes a checksummed public update channel only after Windows
and Deck tests pass. See the Deck installation guide above for one-time setup and SSH
deployment. The old manual scripts are still available.

## In-game acceptance (requires Skate)

1. Confirm `arcade_mode_initialized` is active and `Arcade Mode ready` appears in the log.
2. With the game focused and overlay menu closed, tap A/B/X/Y individually: each gets its own
   numbered notice and log line. Hold each: no repeated notice. Release and tap: number increases.
3. Press multiple face buttons: each is reported. Disconnect/reconnect and change focus: confirm
   the documented unavailable-input behavior. Confirm native controls still behave normally.
4. On Deck, repeat using Proton/controller translation with updates disabled, then restore stock.

CI proves compilation and the pure model, not runtime initialization, controller delivery, notice
visibility, or native-game compatibility. Those require the above in-game checks. The reader and
API signatures are verified against this repository, not inferred from another runtime.

## Milestone sequence

1. Prove controller observation.
2. A -> actual native Skate ollie.
3. X -> one fixed native flip.
4. Observe grounded/air/grind/manual/landing/bail state.
5. Combo score + revert/manual linking + balance + persistent fatigue.
6. Host-validated multiplayer knockdown event.

Milestones 1 and 2 are hardware verified in the bounded Deck session. Next,
investigate whether the native trick state machine supports a fixed flip,
revert -> manual, and
natural grind release/failure. Later controls may use B for grab and Y for grind/context action.
Future knockdown should be a host-authoritative ATTACK/KNOCKDOWN event plus native bail, rather
than rigid-body player collision. Long unbanked combos should become increasingly unstable.

The read-only native score-event adapter validates four pinned graph/schema
contracts and publishes bounded line/sequence observations through a nonblocking
queue. Discovery and diagnostic logging run on a background worker. Ownership,
sequence category and duplicate line-end routes are unresolved, so these
observations do not award or bank Arcade points. Windows/Deck verification is
pending; local sanitizer checks do not establish native acceptance.


The unsaved `arcadeinputcapture <milliseconds> [source-mask]` diagnostic records a
finite capture through the existing 64-slot client channel; duration0 stops it.
Duration is bounded to1..5000ms. Source bits are1 input journal,2 recognition,
4 native physics,8 admitted Manual,16 Manual termination (default31). It requires
fresh native ownership and active telemetry. Only anonymous capture-local owner
tokens, original times/sequences and native generations/steps reach the log.
The existing worker counts confirmed synchronous file writes before the footer;
queue, sink, boundary, provenance or required-source loss remains incomplete.
Schema2 recognition rows preserve each actual attempt's stable ID, session,
queue result, original observation and exact matched-event IDs in matcher order.
A summary is followed by its numbered member rows on that same channel. Held
reuse retains its original event ID/time; input-free and unknown footprints
remain distinct. Metadata allocation is opt-in; ordinary input requests do not
embed a100-ID evidence array. Omitted diagnostic batches and missed polls are
charged once, while a recorded queue rejection is a complete attempt outcome.
Schema1 remains readable, but its recognition bounds cannot prove membership.
Missing/duplicate members and exhausted identities prevent complete accounting.
Input scope covers the shared physical-event journal; ordinary pad diagnostics
remain separate. Capture completeness never verifies a native action or cause.
