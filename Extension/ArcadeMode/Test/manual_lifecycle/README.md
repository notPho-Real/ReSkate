# Manual lifecycle source composition

This standalone test uses the current production `update_combo`, HUD
publication/pending functions and native observer/input delivery/hop call
transactions. `generate.py` extracts them at build time into one controlled
fixture. Marker changes fail generation instead of silently testing an old
copy. The source-correspondence test regenerates in memory and compares every
output, including a manifest of the original chunks and generated hashes.

The fixture supplies clocks, full owner/probe snapshots, presenter/history
observations and outer permissions. Capture/telemetry/impact sinks do not
actuate. It does not execute the renderer, native scripts or native solver, and
cannot establish native initialization/out timing or player acceptance.

The 7,936 assertions preserve the original schedules: Manual/Nose, 17/34 ms
physics steps, duplicate/skipped client callbacks, original request deadlines,
contact loss/recovery, native exits, pending physical release, owner/stale/
pause/OFF loss, and qualified hop return versus ordinary landing. Large fixture
objects live in static storage and are reconstructed in place between cases.

The main target also builds four separate rollback controls. Each removes one
correction from the same generated source. The Python test runner requires exit
code 1 and the specific regression assertion; crashes, unrelated failures and
unexpected passes fail the control. The bail control gives the mutated class a
distinct test-only name so other production headers retain their normal types.

With `DINGOSDK_BUILD_ARCADE_MODE_TESTS=ON` and Python3 available, build
`dingosdk_arcade_manual_lifecycle_tests` and select
`^arcade_manual_lifecycle` in CTest. Missing Python produces an explicit warning.
The existing development workflow includes this target and all six tests.

A focused configuration avoids runtime dependencies:

```sh
cmake -S Extension/ArcadeMode/Test/manual_lifecycle -B build/manual-lifecycle
cmake --build build/manual-lifecycle --target dingosdk_arcade_manual_lifecycle_tests
ctest --test-dir build/manual-lifecycle -R '^arcade_manual_lifecycle' --output-on-failure
```

Generation and correspondence checking can also run directly with Python3:

```sh
python3 Extension/ArcadeMode/Test/manual_lifecycle/generate.py --root . --output build/manual-lifecycle/generated
python3 Extension/ArcadeMode/Test/manual_lifecycle/generate.py --root . --output build/manual-lifecycle/generated --check
```
