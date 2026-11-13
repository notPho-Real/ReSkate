# Portable custom flatland proof

Run the focused tests with a C++20 compiler and Python 3 (no network or dependencies):

```sh
python3 Extension/ArcadeMode/Test/custom_flatland/run.py --output /path/to/small-build
```

Alternatively configure this directory directly with CMake 3.24+, build all
three targets, then run CTest. The targets are `casper_spike_tests`,
`casper_runtime_tests` and `casper_release_tests`. The full Windows runtime
requires a separate supported Windows build.

`--integration` additionally runs six related existing suites and records
`integration-results.json`. It returns failure when any suite fails. The candidate now passes all six suites. The previously failing grab
category/adjacency assertion exposed a neutral fallback that consumed rejected
directional input; that fallback now respects the existing shared journal.
See CASPER_SPIKE.md for historical results and the candidate test procedure.

The tests supply original-step observations and a simulated animation endpoint.
They execute the real InputSession/UnifiedInput, ManualActuation, ManualEpisode,
ManualPhysics, ComboSession, GroundMotionStep, GroundResistance and anim_core.
The runtime target also compiles and executes the existing client/native mailbox.
It checks pending submission, continuous ticket replacement, replacement refusal,
competing playback, native-time hold/exit, duplicate steps and lifecycle loss.
The release target runs the same production mailbox checks with simulated
animation endpoints and without DINGOSDK_DEV_TOOLS. Both targets cover default
entry without a console command, Arcade OFF cancellation and ON recovery.

The native skater_animation.cpp implementation and the outer arcade_mode.cpp /
client_noclip.cpp call sites are not compiled by these portable tests. Their
Windows build and real hook/render/native-physics validation remain required.
Synthetic poses are fixtures, not production animation assets.

The flatland bridge obtains a fresh owned native seed after Manual admission,
then sends animation work to the client thread. Physics remains inside the
existing owner-checked body transaction, selecting one resistance budget.
The bridge never changes native trick identity, balance identity or score.
Flatland is enabled by default with Arcade Mode in both build configurations.
Arcade OFF still cancels all custom ownership. The unsaved arcadeflatland
command remains a developer diagnostic override, not an activation requirement.

X→Y enters; X→X after entry plays the 0.6-second flip and returns to Manual;
another X→Y returns directly. The controlled player holds at 0.3 seconds using
native physics elapsed time, beyond the portable clip's 15-second duration.
The flip replaces the current controlled clip under the same ticket and retains
blend progress; its first body/board pose must match the held pose.
Hold LT or RT after entry to turn the displayed body and board at 480 degrees
per native second. Opposing triggers or release stop further turning. This is
a display turn; exit returns to the native heading. The trigger already used
for rotation remains consumed through a normal exit until release, while the
same Manual continues. This continuation cannot admit another Casper and ends
on stale input, owner/episode changes, gameplay loss or competing ownership.
The runtime regression holds both the exit button and each rotation trigger
across subsequent polls; checking only the initial exit poll misses a dropped
Manual on the next input sample.

Live static wheel-contact material selects the Casper slide resistance request.
The measured slowdown must be checked with native horizontal velocity on each
real surface; the request coefficient alone does not prove the result after
the existing ground-resistance compensation and native physics.

A cancels immediately and leaves the ordinary launch path. Up/Down still feeds
existing Manual balance. Nose Manual, shoulder-spin compatibility, goofy poses,
banked contacts, live surface-material detection and native score acceptance
are not established by these tests.

The isolated worktree is sparse to conserve disk. Before a full build,
`git sparse-checkout disable` restores tracked vendored/assets directories;
it is unnecessary for these portable tests.
