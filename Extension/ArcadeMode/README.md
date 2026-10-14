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

## Controls under development

The current A adapter uses the previously observed timed native crouch/flick
recipe. Physical X supports the measured basic flip candidates; this is not the
complete exact trick table. Hold/release ollie, remaining controls and final
combo chaining still have their ordered acceptance gates ahead.

Automatic skating uses left-stick movement intent and the existing guarded
board-root velocity reader. It emits bounded native push-button pulses below
cruise speed, with separate resume and stop thresholds. It does not write or
clamp velocity. Both wheel contacts and ordinary riding state are required;
menus, manual/grind/air/bail states, stale evidence and conflicting actions
suppress pushing. Movement must release/rearm across mode and menu boundaries.
This adapter is locally tested; real Deck speed/cadence acceptance is pending.

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
