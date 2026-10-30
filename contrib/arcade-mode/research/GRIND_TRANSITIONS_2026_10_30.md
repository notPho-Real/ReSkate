# Grind exit investigation and isolated routing candidate

Baseline: `65d34a0bc06034125b28779d10490e479df37fba`, the user-verified
`arcade-stage-controller-ownership` flip baseline. Work belongs only to
`arcade-stage-grind-transitions`. No installation, release, manual branch,
StateFlow/PTM flip implementation, balance model, or scoring changes.

**Status: incomplete gameplay feature.** The rail A routing defect is reproduced
and corrected offline. Neither a live pop on this revision nor grind-to-grind
execution is accepted. No named two-button combination is confirmed working.

## Source review

Searched Arcade code and the complete tracked official source at local
`official/main` (`e88853add8a78fb79ec4dd5c7fe50ee61e7b9598`). Reviewed the
pinned fork survey in `upstream-method-survey.json`, the cached
andrewnakas/reskate-trainer `606ba5ab` game-state implementation, the cached
worthlessnorms `f6d7e5a` gesture implementation, and merged grind-exit contracts.
The source survey's coverage limits still apply; this is not a new live survey
of every public fork.

The local research executable independently hashes to
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
No binary or proprietary payload is committed. Cached level-root asset indexes
identify the following native assets (asset index and uncompressed SHA-1):

| Asset | Index | SHA-1 |
| --- | ---: | --- |
| `gesture_fliptrick_ollie_fromgrind` | 1495 | `af79b67e413c94accb6efbef88d887ce511fce41` |
| `gesture_fliptrick_nollie_fromgrind` | 14489 | `2e7a665ba8980ee2747f7ef41980d75b0304f688` |
| `ptm_filter_trickfromgrind` | 24039 | `1401b063945db1f2cb44432f48fd73219e442fc7` |
| `ptm_filter_trickfromgrind_reselect` | 9545 | `0a1c97b2d24d0800f7bcc0407f501dacec93f9d2` |
| `gameplay/input/inputs/input_grindtransition` | 33107 | `315a3cc4865b0d45eb73405d69e4005c67f36d55` |
| `enableexperimentalgrindtransitions` | 37122 | `7ddbf73dbbd5a7e0e89993b00a417a78b4b3cceb` |

The last two names do not establish an enabled or callable transition API.
Their payloads are absent from the inspected local cache. The index also
contains `grindstates_grinding_ag_a0148c8`, `grindstates_trick_ag_b2daa3f`,
their conditions/actions, and native grind/popout animation sequences.

## Ollie-out failure and correction

`arcade_mode.cpp` supplies `may_start || may_exit_grind`, but
`ArcadeInput::poll` and `InputSession::poll` both unconditionally zeroed
`may_start` on a coherent native 400-family frame. `UnifiedInput` therefore
discarded A as a crouch request. The existing from-grind actor was unreachable
through the production facade. A production-composed test fails on the baseline
at the first A preparation assertion.

The candidate admits the existing exact `_fromgrind` authored stick recipe
from fresh owner-correlated 401/403 physics. It retains an admitted hold across
the measured brief 402/404 phases, releases its authored flick on physical A
release, and cancels on lost contact before release. Released playback may
finish through 701 and AIR; landing retires it. Missing recipes do not fall
back to ground. OFF/menu/authority loss/stale evidence withdraw ownership.

The descriptor is explicitly `ordinary/rail`. `ordinary_writer()` and
`NativeCharge` still reject it. There is no flat-ground impulse, position
write, forced state, or new hook. `ordinary_writer()` was not the original
first blocker: admission previously started as flat and was changed to rail
only after setup. The correction makes the distinction explicit at admission.

The native mechanism used is input to the game's authored rail gesture and
its existing grind-exit/jump machinery. The measured contract in
`Engine/Game/Build/20260929/grind_exit.h` identifies the native trajectory
setter RVA `0x4752aa0`, grind-pop caller `0x482f646`, and 401-to-200 pop with
positive upward launch (historically about 2.03 m/s). The flat-ground caller
is distinct (`0x4824f7f`). The native game owns geometry and velocity.
Historical evidence in STATE.md records 401-to-200/201-to-100 with Ollie1;
that evidence is not acceptance of this revision. The fixed-rail
403-to-701-to-200/201 corridor already has combo continuity handling; 701
alone is neither an Ollie acknowledgment nor permission to launch.

**Rail pop to early flip remains unverified and incomplete.** The current
ordinary-Ollie recognition lease and `native_flip::arm` integration explicitly
exclude rail releases. The rail `TrickFromGrind` filter is distinct from the
verified ordinary `TrickLaunch` path. This patch does not relax those protected
checks or assume equivalent actor/tag initialization. Trace that native rail
filter and verify its ownership before integrating early flip reselection.

## Two-button failure and exact blocker

`recognize_rail_extras()` has the nine established 300 ms pairs, but production
does not populate `InputAdmission::rail` with fresh/ready/established phase,
frontside/backside context, geometry, or current-variant exclusion. The first
guard therefore returns before recognition. The tables' variant numbers are
semantic request IDs, not verified native `CurrentGrindType` values.

Even with test-supplied admission, `rail_change` reaches `request_extension()`;
production supplies no `extensions.request`. It returns unsupported. The
separate generic `RailAdapter` explicitly rejects `named_grind`, and only
delivers alignment/nose/tail input. Adding another recognizer cannot fix this.

`arcade_grind_observer.h` resolves local AntDriven state pages and reads
`CurrentGrindType` / `CurrentSkateGrindType`; both semantics remain unknown.
`arcade_grind_runtime.cpp` discovers bindings and publishes observations;
`GrindTransitions` detects contact entry/exit, not commanded named transitions.
The trainer fork also exposes `GrindControlLR/UD`, `GrindStabilityNudge`,
`EnableGrindExitToGrind`, `EnableGrindExitDoubleGrinds`, `GrindToBoardslide`
and related intent switches. Its forced-switch experiments do not supply
named-target mapping, native action admission, initialization, cleanup, or
acceptance. No such writes were imported.

The missing contract is the owner-scoped native consumer of
`input_grindtransition`, its conditions/settings, target mapping, and native
motion initialization/cleanup. Need the indexed asset payloads and a native
control trace proving whether each transition maintains contact or briefly
leaves it. Neither executable addresses nor calling conventions for a new
grind actor are established. No fake executor or success acknowledgment was
added. Pair order/simultaneous/held-input acceptance remains pending that
executor and mapping; existing semantic-table tests alone are insufficient.

## Verification and limits

Local macOS checks:

- 360 new production-facade rail routing checks at 30/60/120 Hz, including
  original timestamps, duplicate polls, distinct rail recipe, held/released A,
  missing recipe/evidence, phase changes, owner loss, OFF/menu and landing.
- 5645 production input-session checks and 326 jump-idle checks.
- 2486 ordinary native flip timing/motion-entry checks; 2749 air-motion proofs;
  1620 launch-descriptor checks; 303 readiness checks.
- Existing rail facade (40), producer composition (10), adapter (59), native
  input state (67), control-state (67042), and combo traces (1736) pass.
- Actual HUD publisher/pending/draw composition: 7010 checks pass.
- Lab self-test: 459 tests, one skipped, no hardware calls.

The unmodified grind-observer concurrent-queue test fails locally at
`each producer publishes complete ordered payloads`. Its test and included
grind observer/capture code are byte-identical to the baseline; this candidate
does not address the pre-existing bounded-contention/drop assertion. Preserve
the failure in the build report. Hosted Windows/Linux/macOS build results and
DLL hash must be recorded separately after the isolated CI run.

The new target is included in both full Windows preflight and targeted CI.
Branch CI may stage artifacts, but release publishing requires `arcade-mode`;
the grind branch cannot update that release channel.

## Required live acceptance

Before accepting: validate executable/DLL identity and original physics cadence
and cached skater step at 30/60/120 Hz. Record baseline grind/HUD, original A
timestamps, delivered rail recipe, native trajectory caller and positive Y
velocity, 400-family departure to AIR, real Ollie identity, and combo continuity.
Then verify early native flip actor selection/initialization, landing/re-entry,
and native score history without fabricated credit. Negative controls include
no A, missing recipe, owner loss, OFF/menu, bail/landing and incompatible rail
geometry. Grind changes additionally require actual changed native identity
and animation, contact/momentum evidence, uninterrupted HUD owner/model, and
one acknowledgment per physical pattern. No Deck installation or trial is
authorized by this document; the user explicitly prohibited deployment.
