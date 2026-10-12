# Arcade Mode v0.1

Experimental, always-active controller observation built natively inside ReSkate.dll on
`notPho-Real/ReSkate:arcade-mode`. No plugin loading, Lua, profile toggle, console command,
input remapping/suppression, trick injection, or gameplay changes. Original code and generic
skating mechanics only; no third-party game branding, copied UI, art, logos, music, or assets.

## Controller observation

`start(r.base)` publishes readiness and logs initialization next to Hall of Meat. Every
validated client update calls `on_client_tick()` immediately before Hall of Meat, before the
500 ms model throttle. It reads `ControllerInput` through `DingoSDKOverlayReadControllerInput`.
A/B/X/Y use bits 0x1000/0x2000/0x4000/0x8000. Rising edges update separate per-button counters
(starting at 1 per process) and produce an existing ReSkate overlay notice:

- Title: `ARCADE MODE DEV`
- Body: `A press #1 seen on ReSkate client tick.`

The same press goes to the skater log channel. Holding does not repeat; simultaneous buttons
each report. Unavailable input clears the held mask. The existing input reader requires game
focus and hides input while the interactive overlay is open. A held button seen again after
unavailability counts as a new edge. No new XInput reader or game memory writes are used.
Reporting is best effort: the overlay retains at most six notices, so rapid presses can evict
older notices. Logging and notification APIs are already nonthrowing; allocation/input exceptions
are contained by the observer. A failed notice cannot replay an edge or escape the client tick.

## Independent model and tests

`arcade_model.h` is header-only, with no Windows or ReSkate dependencies. `Model` owns `State`
and sanitized `Tuning`. First trick starts multiplier 1, further tricks add one, and bank returns
raw points times multiplier then resets; bail resets without points. Integer scores saturate on
overflow. Fatigue grows throughout an active combo, persists through balance exit/re-entry, and
increases on links. Drift scales with fatigue; signed correction acts along the balance axis.
`tick()` returns true and bails when absolute balance reaches 1. Initial balance at a limit also
bails. Balance requires an active combo. Nonfinite inputs become zero, dt is bounded to [0, 1]
seconds, fatigue to [0, 10], and age to one day. Grace counts down but does not automatically
bank: future observed landing/bail events will decide settlement. None of this model drives the
game in v0.1.

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

Only milestone 1 is implemented here. After observation works in-game, investigate whether the
native trick state machine supports a native-quality ollie, fixed flip, revert -> manual, and
natural grind release/failure. Later controls may use B for grab and Y for grind/context action.
Future knockdown should be a host-authoritative ATTACK/KNOCKDOWN event plus native bail, rather
than rigid-body player collision. Long unbanked combos should become increasingly unstable.
