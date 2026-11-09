# Portable custom flatland proof

Run only this target first (C++20 compiler, Python 3; no network/dependencies):

```sh
python3 Extension/ArcadeMode/Test/custom_flatland/run.py --output /path/to/small-build
```

Alternatively configure this directory directly with CMake 3.24+, build
`casper_spike_tests`, then run CTest. Do not configure the full Windows runtime.
`--integration` additionally runs six relevant existing source tests and records
`integration-results.json`. It deliberately returns failure for the independently
reproduced RC5 contextual-input assertion; see CASPER_SPIKE.md. It does not hide
or mark that failure expected in CI.

The test supplies original-step observations and a simulated animation endpoint.
It executes the real InputSession/UnifiedInput, ManualActuation, ManualEpisode,
ManualPhysics, ComboSession, GroundMotionStep, GroundResistance and anim_core.
The live playback wrapper is compiled but not executed. Animation bytes are
newly authored from synthetic poses; no native assets are needed or committed.

Live integration remains deliberately absent. A future producer must call
`CustomFlatland::observe` before every input/native step, using an independently
admitted Manual episode and nonfailed balance; bind the lease-selected input table
through `custom_flatland_branch` (never `observed_balance_branch`); and cancel on
mode/owner/episode/animation loss. Gate the opt-in capability on playback readiness.
Apply the returned drag only inside the existing owner-checked body transaction,
replacing that step's ordinary resistance selection rather than adding a second
writer. The prototype conservatively supports ordinary Manual, not Nose Manual.

The finite clip permits at most 15 seconds including entry/return. X→Y enters;
X→X after entry plays the 0.6-second flip and returns to Manual; another X→Y
returns directly. A cancels immediately and leaves the ordinary launch path.
Up/Down still feeds existing Manual balance. Shoulder spin is not newly implemented
or claimed as native-verified. There is no native score receipt for this visual
subtype, and no live surface-material producer.

The isolated worktree is sparse to conserve disk. Before a future full build,
`git sparse-checkout disable` restores the tracked vendored/assets directories;
it is unnecessary for these portable tests.
