# Manual lifecycle source composition

This standalone test uses the current production `update_combo`, HUD
publication/pending functions and native observer/input delivery/hop call
transactions. `generate.py` extracts them at build time into one controlled
fixture. Marker changes fail generation instead of silently testing an old
copy. The source-correspondence test regenerates in memory and compares every
output, including a manifest of the original chunks and generated hashes.

The fixture supplies clocks, full owner/probe snapshots, presenter/history
observations and outer permissions. Capture/telemetry/impact sinks do not
actuate. Both targets use the complete production mode toggle with the extracted
camera epoch state. The optional HUD target also extracts the draw function,
then executes actual ImGui frames and final draw data.
Neither target executes native scripts or the native solver, or establishes
native initialization/out timing, physical action success or player acceptance.

The original 7,936 assertions preserve the original schedules: Manual/Nose, 17/34 ms
physics steps, duplicate/skipped client callbacks, original request deadlines,
contact loss/recovery, native exits, pending physical release, owner/stale/
pause/OFF loss, and qualified hop return versus ordinary landing.
Camera toggle cases additionally require the old lease to retire before an
OFF-to-ON reacquisition, retain retirement on unavailable adapter/phase, and
reset old orbit state, and reject toggles during camera input read or native
transform publication. The native camera adapter is controlled in this portable
fixture; guarded native lease consumers are exercised separately. Large fixture
objects live in static storage and are reconstructed in place between cases.

The composition target is distinct from the older
`dingosdk_arcade_manual_lifecycle_tests` / `arcade_manual_lifecycle` suite.
Both composition executables retain the original lifecycle and camera checks.
The HUD target adds 2,432 checks through admitted Manual/Nose state,
publication and rendering at exact 30/60 Hz
physics intervals. It covers original source timestamps and expiry between
scheduling and draw, missing/invalid contacts, owner loss, real mode-toggle
clearing/re-enable, qualified hop return, ordinary landing, failed-section exit
and distinct rail presentation. Airborne requests and unadmitted contacts must
submit no balance geometry. No production adapter or balance tuning is changed.

The main composition target also builds seven lifecycle/camera rollback
controls. Each removes one correction from the same generated source.
The Python test runner requires exit
code 1 and the specific regression assertion; crashes, unrelated failures and
unexpected passes fail the control. The bail control gives the mutated class a
distinct test-only name so other production headers retain their normal types.
Three additional HUD controls replace the original physics timestamp with client
publication time, use the horizontal cursor coordinate for a vertical meter, or
remove the mode toggle's immediate HUD clear.
Each must fail at its specific presentation assertion; an unrelated failure or
crash is rejected. The renderer uses only bundled ImGui core in the standalone
configuration, with no platform backend or dependency download.

With `DINGOSDK_BUILD_ARCADE_MODE_TESTS=ON` and Python3 available, build
`dingosdk_arcade_manual_lifecycle_composition_tests` and select
`^arcade_manual_lifecycle_composition` in CTest. Missing Python produces an
explicit warning in the root configuration; the standalone configuration
requires Python. The existing development workflow builds this target and
selects all thirteen composition tests plus the older lifecycle suite. To
build only the current renderer composition, select
`dingosdk_arcade_manual_lifecycle_composition_hud_tests`.

A focused configuration avoids runtime dependencies:

```sh
cmake -S Extension/ArcadeMode/Test/manual_lifecycle -B build/manual-lifecycle
cmake --build build/manual-lifecycle --target dingosdk_arcade_manual_lifecycle_composition_tests
ctest --test-dir build/manual-lifecycle -R '^arcade_manual_lifecycle_composition' --output-on-failure
```

Generation and correspondence checking can also run directly with Python3:

```sh
python3 Extension/ArcadeMode/Test/manual_lifecycle/generate.py --root . --output build/manual-lifecycle/generated
python3 Extension/ArcadeMode/Test/manual_lifecycle/generate.py --root . --output build/manual-lifecycle/generated --check
```
