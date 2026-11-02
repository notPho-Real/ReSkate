# Grind exit investigation and isolated routing candidate

Baseline: `65d34a0bc06034125b28779d10490e479df37fba`, the user-verified
`arcade-stage-controller-ownership` flip baseline. Work belongs only to
`arcade-stage-grind-transitions`. No installation, release, manual branch,
StateFlow/PTM flip implementation, balance model, or scoring changes.

**Status: incomplete gameplay feature.** The rail A routing and early-flip
handoff candidates are implemented locally but lack live acceptance. All nine
named bindings have executors in the documented narrow source contexts, with
composed routes from401/fine20. From402/fine31, ordered X->B returns to403/fine24;
Y->Y now composes that preparation with the existing return to401/fine20.
From403/fine24, Nosegrind and Crooked requests now compose the measured
401/fine20 return with their existing native sweep.
Y->Y now also returns established403/fine23 Nosegrind to401/fine20, following
an independent native two-leg qualification on the same twelve-metre segment.
Independent native trials verify the constituent physics actions and retained
segments; the newly composed full return and production animation/HUD/combo
acceptance remain unverified. Broader source contexts and arbitrary repeated
sequences remain incomplete. The sections below record the
initial failures and subsequent findings chronologically; later qualification
does not establish acceptance of the whole requested feature set.

**User build hold:** do not start another DLL/CI build or offer a test handoff
until named grind changes and early flips after rail exit are implemented and
ready for testing. Keep further work local: pushing this staging branch starts
CI automatically. Commit `87ee71c3e60e565a83d4df52007f0a76055d15ec` was
already uploaded before this instruction, starting development run
`41109697293` and targeted run `41109697352`. Cancellation was unavailable in
the connector and the browser was not authenticated. No artifact from these
runs was downloaded, staged, installed or offered for acceptance.

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
Their payloads were absent during the initial inspection; the bounded read-only
follow-up below recovered them. The index also
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

**Rail pop to early flip is implemented locally, not gameplay accepted.** The
follow-up below establishes that the distinct rail filters select samples in
the same native TrickLaunch PTM and Kickflip sequence. A separate rail release
lease now connects the input session to the unchanged native animation bridge.
The ordinary recognition-continuity predicate, ground launch eligibility and
velocity writer remain unchanged. No DLL is offered while named transitions
are incomplete.

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
`CurrentGrindType` / `CurrentSkateGrindType`; their semantics were unknown at
the initial inspection. The native enumeration follow-up below resolves the
nine requested identities without making them executable actions.
`arcade_grind_runtime.cpp` discovers bindings and publishes observations;
`GrindTransitions` detects contact entry/exit, not commanded named transitions.
The trainer fork also exposes `GrindControlLR/UD`, `GrindStabilityNudge`,
`EnableGrindExitToGrind`, `EnableGrindExitDoubleGrinds`, `GrindToBoardslide`
and related intent switches. Its forced-switch experiments do not supply
named-target mapping, native action admission, initialization, cleanup, or
acceptance. No such writes were imported.

The missing contract is the owner-scoped native consumer of
`input_grindtransition`, its conditions/settings, target mapping, and native
motion initialization/cleanup. The follow-up established the animation
reselection path below; a native control trace must still prove which input
selects each physics target and whether each transition maintains contact or briefly
leaves it. Neither executable addresses nor calling conventions for a new
grind actor are established. No fake executor or success acknowledgment was
added. Pair order/simultaneous/held-input acceptance remains pending that
executor and mapping; existing semantic-table tests alone are insufficient.

## Follow-up: native reselection and named request definitions

The user selected the classic ordered two-button layout. Existing semantic
variants now have explicit `GrindChangeTarget` names, available through
`InputRequest::grind_target()`. No native enum conversion is implied:

| Input | Target | Semantic variant |
| --- | --- | ---: |
| Y, Y | 50-50 | 1 |
| Y, X | Noseslide | 2 |
| Y, B | Nosegrind | 3 |
| B, B | Crooked | 4 |
| B, X | Bluntslide | 5 |
| B, Y | Nosebluntslide | 6 |
| X, X | Smith | 7 |
| X, B | 5-0 | 8 |
| X, Y | Tailslide | 9 |

The requested layout was checked against the user-selected control reference;
its external citation is retained with the private investigation artifacts.
The existing recognizer, strict 300 ms window, front/back precedence and
consumption remain unchanged. Early and Lip variants cannot alias these targets.
These are defined bindings, **not confirmed working transitions**.

Before the user requested offline-only continuation, a bounded read-only
inspection retrieved 87 indexed EBX assets (262,563 compressed bytes), checked
their manifest SHA-1 values and the executable SHA-256, and decoded them locally.
A separate read-only registry inspection resolved six exact native GUIDs plus
the reselection expression's dependencies. No native functions, input, hooks,
game file writes or installation were performed. Raw game assets remain in
ignored local research artifacts and are not included in source publication.

The actual native animation path is now established:

- `Transition_Grind.Loop.Node_to_Grind.Loop.Node`, GUID
  `54919b5b-1690-9a7a-5491-9b5b16909a7a`, re-enters
  `Grind.Loop.Node` (`8cc0ec9b-0e9f-ea14-8cc0-ec9b0e9fea14`).
  Its condition is `EBool.Anim.InCorrectGrind`
  (`5d2236d7-69c7-271e-5d22-36d769c7271e`), required false. The native
  DiffBlend has BlendTime 30 in asset units, not an inferred millisecond duration.
- The loaded expression's exact dependencies are `Enum.GS.SkateGrindType`
  (`0844b155-be0e-9228-0844-b155be0e9228`) and
  `Enum.GS.Anim.CurrentGrindType`. Its kernel reads both, compares the first
  with zero and with the second, and ORs the comparisons. Equality at RVA
  `0x1458e50` and boolean OR at `0x16eb6e0` were checked in the pinned executable.
  Thus reselection is requested when the physics type is nonzero and differs
  from the animated type. This is **not** a callable named-target selector.
- `PTM_Match_Grind_Loop` uses `CDB_PTM_Filter_GrindLoop`; that context database
  selects `PTM_Filter_GrindFromInto` versus `PTM_Filter_GrindLoopReselect`
  using `Bool.State.Is.GrindInto`. The reselection filter includes 32769 and
  excludes 0. No animation ID or filter is overwritten by this branch.
- `SkaterCorePhysicsGrindInputConfig` binds native heading, pitch, LR/UD,
  boardslide, double-grind, exit, and lip target inputs. Tuning includes minimum
  transition heading velocity/change and separate board/tip/5-0 pitch curves.
  These do not establish a target-independent stick recipe.
- `Input_GrindTransition` is a boolean input (NameHash 3069668163), consumed
  by the `GrindStates_Trick` condition graph. The experimental setting's key is
  `exp_grind_transitions`, default false. Neither is a named-grind API.

Production diagnostics now sample the independently selected physics grind
alongside both existing animation identities, through the same owner-validated
reader. Missing optional data leaves the original observations available.
Late-arriving or withdrawn bindings are republished independently. A separate
`ARCADE GRIND SELECTION` line preserves the old capture protocol and reports
unavailable/no-target/matched/mismatch; none is an action acknowledgment.
Balance, combo, scoring and native flip implementation are untouched.

The final input-ownership pass also reproduced a retained semantic Y request
after a successful rail A setup. The adapter withdrew its physical channel,
but the session could still report a busy rail owner until Y release or expiry,
blocking auxiliary actions during that interval. Successful from-grind setup
now retires that old request. Missing/unsupported rail recipes preserve it.
The physical event baseline is unchanged: holding Y cannot restart the old
request; a new Y press can prepare the next grind. The production facade and
native rail adapter are composed in 144 additional checks at 30/60/120 Hz,
covering first contact, A setup/release, 701/AIR, completed playback and re-entry.
The test failed on retained ownership before the correction. It does not claim
an observed native launch or successful early native flip.

Remaining execution blocker: resolve and verify the native input/geometry path
from each named target to the physics-selected grind, including stance,
front/back context, admissible starting grinds and rollback. The animation path
cannot safely substitute for that physical transition. No production named
executor is enabled. The user declined a live native-control trial, so work
continued offline and no gameplay acceptance is claimed.

The user subsequently authorized shared Deck access. Static retrieval and
read-only readiness inspection are recorded below; no controller trial has
run under that authorization yet.

## Local follow-up: early flip after native rail exit

Cached original assets close the animation-ownership question without hardware:

- `PTM_Trick_OnBoard_GroundTricks` (2987, GUID
  `cb61fbfa-59db-ec3c-cb61-fbfa59dbec3c`) is the exact PTM already guarded by
  the native bridge. Its motion filter points to `CDB_PTM_Filter_Tricks`
  (13595, GUID `4ee3f36c-b986-a6e6-4ee3-f36cb986a6e6`). That database reads
  `Bool.Anim.TrickFromGrind` and `Bool.Anim.WasFlipTrickChanged`, and references
  both previously decoded rail filters (24039 and 9545).
- The original Kickflip-high sequence (28986) contains a `TrickFromGrind`
  motion tag at tick 7 with duration 1, and `TrickFromGrind_Reselect` at tick 7
  with duration 5. The original motion enum (18539) resolves those tag GUIDs
  to values 9 and 14. The same sequence's Pop and Jump tags both start at 12
  with duration 2. Thus rail reselection samples precede the same Pop/Jump
  interval already handled by the existing post-pop initialization code.

The input session admits a rail follow-up lease only after a successful,
released, non-Nollie rail setup with a known native selection object and an
available native flip callback. Every native physics publication validates
401–404 → optional 701 → 200/201 continuity, owner, selection object, stance,
mode/contact/action generations and flight episode. Input polls cannot hide an
intermediate landing/bail or renew that native evidence. This lease observes
an actual native Ollie before allowing the existing X request path to submit.

The runtime arms the existing bridge from that rail-specific release lease,
without using a `NativeCharge` ticket. Losing the lease cancels only a bridge
armed by this rail path. Once a flip request is submitted, ownership survives
its native motion initialization and ends on flight/owner loss. A missing
callback cannot fall back to another authored rail flick. Native ascent checks,
StateFlow transition evaluation, clip selection, post-pop actor initialization,
blending and cleanup remain in the unchanged working bridge.

Shared-component dependency: `arcade_gesture.h` publishes each native frame to
the rail lease; `arcade_input_session.h` distinguishes its continuity from the
unchanged ordinary lease; `arcade_mode.cpp` adds rail-only arm/withdraw routing.
The native flip implementation and its ABI contract are byte-identical to the
baseline. No scoring, balance/HUD or manual lifecycle policy is changed.

Local verification: all ten focused suites pass, including 660 rail-pop checks
at 30/60/120 Hz, 2,486 existing ordinary flip checks, 5,645 input-session checks,
326 jump-idle checks, 1,736 combo trace checks and the existing rail/control
suites. The offline lab passes 459 tests with one skip. These are routing,
ownership and motion-entry checks, not an observed native animation or launch.

Named-grind execution remains blocked on original control-program data absent
from the offline cache. For example, `GrindStates_Trick_CG_B2DAA3F` references
resource `49c453ebf6363dbd` (RES 16986), whose 1,209 compressed bytes start at
offset 952121425 in `Data/Win32/configurations/layout/initialinstallpackage/cas_45.cas`
(manifest SHA-1 `cd8ba60df76ce4b5b96250e3cbe1116737c7daa2`). Related
`AirGround_Grinding` input/control actions also require their resource kernels.
The available EBX files contain references and patch metadata, not those
instructions. A local game-data path was requested; no Deck access or new CI
build was started. The missing bytes may establish additional prerequisites;
their availability alone is not a claim that native named execution works.

Follow-up local verification: 40,089 observer checks (including selector/animation
lag at 30/60/120 Hz), 771 existing selector checks, 504 rail-pop checks, 5,645
production input-session plus 326 jump-idle checks, 2,486 ordinary flip checks,
342 native Lip probe checks, 40 native rail facade checks, 67,042 control-state
checks, 1,736 combo trace checks and 166 rail handoff checks pass. The concurrent
queue check passed this run; the earlier baseline failure remains recorded below
and no queue algorithm change is claimed.

## Verification and limits

### Shared Deck static-data follow-up

The user subsequently authorized sharing Deck access with the manuals task.
After notifying that task, a bounded read copied 169 indexed static CAS spans
(212,592 compressed bytes). Every span matched its previously indexed SHA-1.
The request used read-only file descriptors and positional reads; it performed
no controller input, console commands, process access, remote writes, restart,
installation, or build. The manuals task was notified when the read completed.
Raw game assets remain in ignored local investigation storage.

All 57 EBX assets and 112 expression kernels decoded successfully. This resolves
the missing-file blocker above, including RES 16986. The newly inspected
`AirGround_Grinding_AG_B12391C` actions establish these additional dependencies:

- `SetFloatIntentGameStateData_6/7` initializes `Float.Intent.GrindControlUD`
  and updates it from `Input_GrindControl`.
- `SetFloatIntentGameStateData_9/10` initializes `Float.Intent.GrindControlLR`
  and updates it from `Input_GrindControl`, with additional conditions.
- `SetFloatIntentGameStateData_15/16` and `_18/19` initialize the heading and
  pitch control multipliers. Both update programs reference
  `Input_GrindTransition` plus two further dependencies whose semantics are
  not yet resolved (GUIDs `9eaa146b-f270-4468-bf4d-1489c11a3b21` and
  `701c5884-eb0b-414b-a220-7911d90ea021`).
- The intent graph installs `OnboardGrindGestureSetCollection`; that collection
  alone does not establish a named grind-change actor.

These are static control dependencies, not verified target recipes. The
remaining execution blocker is establishing the native control-to-physical-
grind selection contract, including stance, geometry, admission and named
identity. No two-button executor has been enabled on this evidence alone, and
no new DLL or CI build was started. Early rail-to-flip routing remains locally
implemented and tested as described above, with live acceptance outstanding.

### Native identities and control dependencies

Further bounded reads retrieved the grind orientation routines (70 spans,
144,476 compressed bytes), two enumeration assets (4,069 bytes), the original
input/settings definitions (289 spans, 334,682 bytes), and the active gamepad
routing graph (33 spans, 71,600 bytes). Every span matched its manifest SHA-1.
These were static reads; no executable or game configuration was changed.

`Animation/Dingo/Enum.SkateGrindType`, GUID
`75934470-c85c-df18-7593-4470c85cdf18`, has 78 explicit entries. Global gameconfig
EBX 1767 has compressed SHA-1 `2938c5adc35526f70447d4063bb0cd88f65d01ff`.
All three observed grind fields reference this same enumeration. The requested
identities are:

| Target | FS | BS | BF_FS | BF_BS |
| --- | ---: | ---: | ---: | ---: |
| 50-50 | 3 | 20 | 37 | 54 |
| Noseslide | 4 | 21 | 38 | 55 |
| Nosegrind | 6 | 23 | 40 | 57 |
| Crooked | 15 | 32 | 49 | 66 |
| Bluntslide | 14 | 31 | 48 | 65 |
| Nosebluntslide | 13 | 30 | 47 | 64 |
| Smith | 9 | 26 | 43 | 60 |
| 5-0 | 7 | 24 | 41 | 58 |
| Tailslide | 5 | 22 | 39 | 56 |

The new `arcade_grind_identity.h` resolves these exact values and target IDs
within a supported current identity's native block. It retains the asset's
`BF` distinction without guessing its physical meaning. Invalid (0), Exit
(77), unknown values and other unimplemented origins cannot alias the nine
targets. `NativeGrindSnapshot` exposes independently guarded selected and
animated identities. It does not promote them into scoring, contact ownership,
actor readiness or a completed transition. The legacy capture protocol and
balance/HUD code remain unchanged.

The observer suite passes 40,190 checks, including independent enum rows,
selector/animation disagreement at 30/60/120 Hz, missing fields, invalid owner
evidence and rail departure. These verify interpretation of real asset IDs;
they are not native grind execution tests.

The previously unresolved multiplier dependencies are now identified:
`701c5884-eb0b-414b-a220-7911d90ea021` is `EnableLegacyGrinds`, and
`9eaa146b-f270-4468-bf4d-1489c11a3b21` is
`EnableHybridSimplifiedFlickItTricks`. Both assets default to false. The native
multiplier programs contain values 8.0, 1.0 and 0.2; settings affect the control
path, so a fixed guessed pressure duration is not a verified recipe.

The gamepad update kernel (RES 7573) publishes `Input_GrindControl` at PC
0x272c, assembled from its LR/UD outputs. At PC 0x2844 it calls the boolean
input producer for `Input_GrindTransition` with false (constant offset 0x781).
The descriptor for call hash `f40d569c` points to native RVA 0x830840, whose
first test returns immediately for a false third argument. This does not
establish a safe callable named-grind actor: the context/owner contract and
target control sequence still need proof. No hook or direct state writer was
introduced from that address alone.

Read-only Deck readiness inspection confirmed the original game executable
and the manuals candidate DLL (`62844b58c57a2c64fa46cb055db4e9748c286529`,
SHA-256 `adae9ab43a31806ed90ea217df3d7620f45075598be0feab3970eecb928c6d42`).
The display showed Steam's idle artwork and the existing trace contained no
fresh native observations. A visible rail approach was requested before any
native control trial. At that readiness check no input was sent and no new
build was started.

The user subsequently authorized a short native control trial and reported
the game ready. Fresh compositor properties still identified Steam (app 769)
in front of Skate; a bounded one-pixel pointer movement and return did not
dismiss the idle artwork. A later capture showed Skate, but the skater was
fallen in the stadium, with no nearby rail. A reviewed, installed external
diagnostic pinned to the installed executable/DLL then ran only
`arcadefixture status` and `arcadeinputcapture 5000 96`, serialized with the
shared device lock. It reported map `bam`, native state 504, Arcade ON,
`configured=false`, `present=false`, and native UI outside menus. The capture
also reported no available controller source without a virtual trial device.
No controller actuation or settings change occurred; the finite observation
finished and the shared control window was released. This is readiness
evidence, not a native grind trial. The local diagnostic receipt is
`grind-readiness-62844b5-v1/results/1793389644898445000.json` in the external
lab directory. A valid rail approach remains necessary to establish the
named transition control sequence. No new DLL or CI build was started.

The user then explicitly authorized autonomous game/rail setup. The setup
runner was reviewed and installed outside the checkout with the current DLL
pins, shared device lock, bounded lifetime and input cleanup. The initial
native mount attempt did not immediately leave state 504; the next observation
already showed stopped riding state 100, so the guarded recovery operation
correctly sent no teleport. The native editor placed one original flat rail
without clearing the course. At the user's subsequent request, the same rail
(persisted editor object 1) was moved to ten metres ahead of the current skater
and aligned to its measured heading, using `editor move` with current map,
generation and revision. The first move was rejected after incomplete console
delivery; no placement was accepted from it. Corrected bounded delivery
completed the move and closed the editor.

The verified placement receipt is
`grind-setup-62844b5-v5/results/1793393234162054000-move10.json` in the external
lab directory. Anchor: approximately (143.6600, 1.0287, -91.2541), heading
88.6113 degrees; rail centre: (153.6571, -0.0000068, -91.01176), quaternion
(0, 0.6984856, 0, 0.7156241), scale 1. Read-only object-database verification
and a fresh display capture agree. The original fixture helper retains its
old pose and now reports `present=false`; do not use its reset or trajectory
readiness as authority for this deliberately moved rail. No DLL was installed,
no gameplay tuning was changed, and rail placement is not grind acceptance.
A subsequent 45-second read-only observation
(`grind-watch-62844b5-v1/results/1793393316234404000.json`) recorded 403 native
state samples, all riding 100, and no grind interval. It sent no input; the
shared lock was released afterward. The user was invited to try the rail.

The user reported that both requested actions still fail. Installed identity
remained `62844b58`/`adae9ab4`, which does not contain this branch's fixes. The
rail setup must not be presented as a candidate-feature test. The existing
trace and bounded recent logs did not retain those grind attempts.

A subsequent native baseline setup was aborted before any controller trial:
the world had changed (editor generation 11 versus the placement's generation
8), while the editor still reported map `bam` and retained the saved object
row. The original trial runner incorrectly checked that persistent row without
checking the placement generation before its native Trainer reset. Its
post-reset approach guard rejected the resulting off-board state. The skater
was restored through Trainer to the exact pretrial horizontal position and
heading, (522.381104, 1047.331909), 262.792664 degrees, in Hedgemont. Arcade
remained ON throughout; the trial never reached its OFF toggle or push/Ollie
sequence. Retire external `native-rail-62844b5-v1`; the local trial source now
requires placement generation 8 and revision 12 before reset, and cannot be
reused in the new world. A persisted editor object is not native in-world
geometry evidence. No grind transition or feature acceptance was established.
Restoration completed with a native Y pulse, the actual native action emitted
by `BoardChord`; the earlier shoulder chord reached game-facing XInput but did
not mount while Arcade admission was unavailable. Receipt
`grind-setup-62844b5-v6/results/1793394020129578000-mount.json` confirms riding
100 near the pretrial position (within 0.11 m horizontally), Arcade ON, editor
closed, outside native menus, with input released. The capture independently
reported physics/cached dt 0.03333333 s. These are setup/input-route findings,
not acceptance of rail exit or named grind execution.

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

## Offline continuation: transition input is also a trick-entry inhibitor

Further inspection of the already cached original resources narrows the role
of `Input_GrindTransition`. In `GrindStates_Trick_CG_B2DAA3F` (EBX 15051,
RES 16986), PCs 0x00c0–0x0134 read this input and combine it with
`!EnableLegacyGrinds`. PCs 0x0670–0x075c additionally require the absence of
the graph's recognized gesture allowlist result and `!EnableDoubleStickInput`.
When all four terms are true, PC 0x076c writes false to the trick-entry result
and skips the subsequent ordinary, simplified, hybrid, double-stick and
Ollie-button gesture checks. When false, execution continues at PC 0x0780.
The allowlist's individual gesture names are not all resolved, so no broader
claim about which gestures bypass this gate is made.

This is instruction-level evidence, not inference from the asset's name:
native input-presence reader hash `adef306d` resolves to RVA `0x8314c0`;
boolean AND hash `8eb2196d` resolves through its registration at `0x16e7508`
to RVA `0x16e7130`; NOT hash `1b74206c` resolves to `0x16eb600`.
Interpreter opcode 38 at RVA `0x16d1a4c` follows its encoded next PC for true
and its explicit branch target for false. The constant at resource offset
0x204 is false. These were checked against the pinned executable above.
The input's producer at `0x830840` and consumer at `0x8314c0` operate on
owner-context input collections; neither takes a named grind target.

Consequently, raising this boolean cannot be treated as a standalone named
grind executor, and leaving it active could suppress the separate rail-exit
gesture path. Any eventual native integration must establish its intended
control consumer and clear ownership before Ollie preparation. The blocker
remains the native control-to-physics-target contract and its geometry/owner
requirements, not missing button recognition. This continuation performed no
Deck access, native writes, DLL/CI build, installation or gameplay acceptance.

## Offline continuation: native BoardAdjust control path

The original root intent graph resolves the branch previously hidden behind
`EATExecuteActionData`. EBX 13508, `gameplay/skaterintenttree`, has root GUID
`280d5160-4463-11ea-abb6-d30e7db36987`. Its 66,574 compressed bytes matched
manifest SHA-1 `ef5efab332771abb760f0d1d00ac14428a8b7bb8`; decoding all three
CAS blocks produced 154,216 bytes with SHA-256
`b6c90d6c46d142cb931da75f689b2d7f11902a3a93603248726476447f20e778`.
A further bounded static read retrieved 144 BoardAdjust/raw-input spans
(268,482 compressed bytes), all matching manifest SHA-1s: 78 EBX assets and
66 expression kernels. These reads did not inspect or control the game process.

The root graph's grinding subtree (instance 346) contains `Includes` (508)
and `GrindStates` (259). `Includes` is `DirectQueriesOnly` and contains
`GrindOrientationCalculation` (65), **BoardAdjust (288)** and instance 51.
BoardAdjust's instance GUID is `bb29bf65-9074-4c2c-89a2-2fa614737d23`, matching
the orientation action's referenced branch. It owns `BA_States` (191), whose
children are Idle, Up, Down, Left and Right. This establishes the queried
native directional-control branch; it does not establish a standalone
callable named-grind actor.

- BoardAdjust admission is `!EnableLegacyGrinds` (EBX 27264 / RES 10819).
  It is not an `Input_GrindTransition` presence check.
- Its update (EBX 9321 / RES 7348) samples presence of `Input_GrindControl`
  and writes the grind-control data's boolean at offset 0x58 through the
  existing native ECS command. This corresponds to the `HasActiveInput`
  dependency used by the orientation calculations.
- Up admission (16995 / 6260) reads EAT variable GUID
  `aab6701f-9575-43e9-94b1-28ace48a542c` and requires value 1. Down requires 2
  (24837 / 6056), Left 3 (7611 / 6254), Right 4 (29353 / 6251), and Idle 0
  (33952 / 6557). Raw-input state entry writes these values; these are
  directional-state values, **not named grind IDs**.
- Raw Nose entry (14655 / 10444) reads `Input_BoardAdjust`, then captures a
  scalar through native call hash `6b7bcbbe`. The input reader `f0b94cb2`
  resolves to RVA 0x831010: it passes the input vector's two components to
  `atan2f` and returns one float. Thus this capture is an angle, not a vector.
  The import thunk at RVA 0x546da28 resolves through IAT 0x5feb798 to
  `api-ms-win-crt-math-l1-1-0.dll!atan2f`. Raw Idle entry (28215 / 13962)
  writes zero to the directional variable and the scalar capture.
- In the gamepad kernel (34108 / 7573), PCs 0x2500–0x2534 publish
  `Input_BoardAdjust`. PCs 0x2560–0x272c derive and publish
  `Input_GrindControl` from the same two source components, with the legacy
  setting additionally admitting the body-input component into LR. The
  separately published `Input_GrindLean` uses the body-input components.
  A named transition must preserve this distinction to avoid taking over
  the balance/exit channel.

Up's native update (4105 / 14002) reads `Input_GrindControl` and
`EATVariable_GrindControlZero`, calculates angular heading and pitch inputs,
then submits two floats at grind-control data offsets 0x48 and 0x4c through
the existing ECS command. It does not request an enum ID. The math helpers
were checked in the pinned executable: `776a1eb2` / RVA 0x16e1be0 computes
2D magnitude; `5367d052` / 0x16e28c0 normalizes; `7414b0ad` / 0x16ed5c0 is
the 2D dot product; `4a80cf2b` / 0x16d8ad0 divides scalars; `a5d218c2` /
0x16d8aa0 divides a vector by a scalar; `0530f8a8` / 0x16e2220 is scalar
maximum. `4ee80402` / 0x16ece50 calls `acosf`, verified via IAT 0x5feb710.
These confirmations prevent treating the angular inputs as raw stick axes.

The orientation action (32849 / 11786) also owns **control-reference
maintenance**. At PCs 0x0550–0x0574 it captures the current grind-control vector
into `EATVariable_GrindControlZero`; at 0x0188–0x01e0 it records native base
heading, base pitch and last grind type. On the differing-type path
0x07bc–0x0ab8, it transforms the saved control reference using the base-heading
change and stores the transformed vector. At 0x0adc–0x0b58, absent native
control input clears the saved vector. The initial capture condition also
depends on input-edge evidence, directional state, and an unresolved native
component flag at offset 0xc4. Do not bypass this native initialization by
writing `GrindControlZero`, the heading/pitch fields or grind identities.

Default tuning was decoded from the original schema and schematics assets
(35573 / 36255), including boxed curve bindings. The 50-50 heading routine
(8165 / 13453) uses `5_0_HeadingInputCurve` (field hash 0x1233fd42), wrapping
the input angle, evaluating its absolute degree value, restoring sign,
converting back to radians and adding base heading. Its default knots are
(0,0), (90,90), (180,180). Boardslide heading instead has knots (0,0),
(30,0), (90,90), (150,180), (180,180). These are schema defaults, not proof
of the currently selected live tuning or of interpolation between knots.
Other defaults include minimum transition heading change 15 degrees and
minimum heading velocity 50 degrees/second. Consequently, neither a fixed
stick vector nor a guessed pulse duration is a verified named transition.

The remaining execution contract is now narrower: relate the native
directional state, captured angular/control references and geometry-dependent
physical grind selection to each requested named target. Establish the
owner-valid control request, its neutralization/cancellation lifecycle, and
independent physical-selector/animation changes before enabling the production
extension executor. Source tracing still has not proved that every requested
pair can maintain contact on every geometry, or whether some pairs require
a brief native airborne transition. No named executor, synthetic identity
write, claimed gameplay acceptance, DLL build or CI run was added here.

Local decoded resources, instruction annotations and curve bindings remain
under ignored `build/grind-investigation/`; proprietary payloads are not
committed. The implementation HEAD remains
`2bc0d1e822e044d446bdc0c697127c5f9be81acb`.

### Shared Deck identity update

The manuals chat reported no active Deck job and a newly installed source
`c2dbaf58a98a5b1ab06cea8b3039f17ef2c6b8ea`, DLL SHA-256
`d057a7211e1a264bd73210bd2d26aa28acf821c4fcd9f5d673eb7787fa16b186`.
Its installation is verified there; loading by the running game is still
unconfirmed. All earlier 62844-pinned grind trial runners are retired. This
continuation used static CAS reads only and released coordination without
input, console operations or installation. Any later native trial needs a
reviewed runner with current identity pins, the shared lock, and fresh loaded
process, world-generation and in-world rail evidence; previous scene poses
cannot be reused.

### Physics-output ownership and current readiness (2026-10-31 UTC)

Five additional original assets (EBX 18354, 16420, 3836, 4186 and 25898)
were retrieved as fixed, hash-checked CAS spans totaling 8,120 bytes. Their
decoded evidence is in ignored `build/grind-investigation/native-physics-output/`.
No production input or process mutation was involved.

The binding direction is now independently established:

- `SkaterCorePhysicsConfig` (3836) binds `GrindInputs` to
  `37904965-8e32-f646-3790-49658e32f646` and `GrindOutputs` to
  `b5c634a6-9cb9-d803-b5c6-34a69cb9d803`.
- That output asset (18354) binds `SkateGrindingType` to
  `0844b155-be0e-9228-0844-b155be0e9228`, the existing independently observed
  `Enum.GS.SkateGrindType`. Its separate `GrindingType` binding is
  `c834899c-1ead-9ed8-c834-899c1ead9ed8` (`Enum.GS.GrindType`, the coarse family).
  Consequently the named selector is a **physics output**, not a target request
  that Arcade may write to execute a transition.
- Asset 25898 names the previously opaque directional-state variable
  `aab6701f-9575-43e9-94b1-28ace48a542c` as
  `Gameplay/SkaterVariables/EATVariable_RawBoardAdjustInput_State`.
  This confirms the directional-state interpretation above.

Executable inspection also found `GrindEntryType_StayInGrind`,
`GrindEntryType_GrindToGrind` and `GrindEntryType_AirToGrind` diagnostic names
alongside coarse grind-target/contact names. Strings alone do not establish
enum values, callable entry points, geometry admission or actor ownership.
Instruction-boundary disassembly rejected several apparent +17/+34/+51
selector leads: 0x47c8950 is registration work, 0x47e4580 is construction,
and 0x4766030 contains unrolled vector operations. None is a verified named
transition executor. The component flag at +0xc4 remains unresolved; its
type metadata and vtable references do not yet supply a safe actor interface.

A newly reviewed read-only identity probe independently verified installed
source `c2dbaf58a98a5b1ab06cea8b3039f17ef2c6b8ea` and DLL SHA-256
`d057a7211e1a264bd73210bd2d26aa28acf821c4fcd9f5d673eb7787fa16b186`.
The original executable SHA-256 is
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
At observation time 1793405271, only launcher processes 48177/48184 existed;
there was no running game process, loaded-DLL evidence or current skater/rail
sample. Read-only display inspection likewise showed Steam and the ReSkate
launcher. No input, console command, focus change or installation was made.
The other chat reported no active Deck job. The external probe receipt is
`grind-identity-c2dbaf5-v1/results/1793405271792700000.json` under the lab root;
the reviewed runner digest is
`8e71cf24825a20c7369c55282529e3fd04dab161ac516e9b215fd95fb4d81dea`.

The exact remaining implementation blocker is the legitimate control/actor
request that drives a requested named target from current native orientation
and rail geometry, with reference initialization, cancellation, and independently
changed physics identity. The existing input-to-orientation and
physics-identity-to-animation paths are evidence for parts of that mechanism;
neither supplies the missing target execution contract. The next native trial
must first launch/inspect the game under the shared input lock, revalidate
loaded identity and current world/geometry, and observe native control effects.
No old world pose or retired runner may substitute for those checks. No new
production executor, DLL build or CI run was added in this continuation.

### Native transition admission resolved (2026-10-31 UTC)

The local Ollie request gate incorrectly treated 401/403 as setup phases and
401..404 as hold phases. The pinned native state getters identify 400 Boardslide,
401 5050, 402 Tipslide, 403 5_O, 404 Backslash, 405 Darkslide and 406 Grind
Transition. All seven enter the selector's rail-exit branch at RVA 0x47466f3.
The local request/follow-up gate now accepts this exact whitelist; 407 and other
unknown family values remain rejected. This changes neither native admission
nor the flat-ground launch writer. The focused rail-pop suite passes 1,326
checks at 30/60/120 Hz. Freshly compiled input-session (5,645 plus 326 jump-idle),
native-rail facade (40) and production composition (10) checks also pass.
These are offline checks, not rail-pop gameplay acceptance.

The actual contact-preserving native transition actor is now identified:

- State 406, vtable RVA 0x65e6f18, entry 0x4817b50, cleanup 0x4818430.
- Entry first calls native common initialization (0x4817770), captures the
  current heading/pitch/roll from context +4044/+4048/+4052, and reads requested
  heading/pitch/roll from GrindInputs +24/+60/+52 (context +5248).
- It determines the destination physical class through 0x48140e0, initializes
  the native transition curves and three smoothing objects, and leaves native
  contact/translation/update/cleanup in control. Target identities are outputs;
  neither CurrentGrindType nor CurrentSkateGrindType is an execution request.
- Native selector 0x474687a requires the feature component at context +5280,
  byte +26, output +419, and existing contact at context +4016 (or current 406).
  Other native exit/geometry conditions still precede this branch.

During the coordinated Deck window, read-only probes independently verified
the loaded and installed source `fbcc0e9ef1be8620fab4e2b2fe20adf68a4ebc32`, DLL
SHA-256 `fbe516a134440e81af03dd557d4e7b9ae46951223b40950a1ee1958183b45f2c`, and
original executable SHA-256
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
The on-board state was 100, physics and cached step both 0.0333333313 seconds.
This installed branch does not contain the isolated rail-pop candidate.

A bounded hardware access watch traced the feature byte's real producer:
native ECS field command 0x1895730, invoked by interpreter 0x16d2010 from
expression resource key **0x0542dc73**. The resource is 3,280 bytes with constant
start 0xb0 and instruction start 0x2c0. Its command at PC 0x04ac supplies
page 2 offset 0x4b and field descriptor 0x0001001a (one byte at offset 26).
The value originates from the setting reference at constant offset 0x100:
`Gameplay/Settings/ExperimentalSettings/EnableExperimentalGrindTransitions`,
key **exp_grind_transitions**. The script getter 0x5955520 calls 0x837f40,
which calls the existing applied-settings bool getter 0x804f60. Thus the
previously unknown admission flag is conclusively this native setting, and it
was false in the captured running game. The adjacent wallride setting uses
component byte 31; it must not be confused with the grind flag.

Reviewed external observer receipts under the lab root:

- `grind-flag-watch-fbcc0e9-v3/results/1793410540744026000.json`, runner SHA-256
  `3a122e7efb9e72c962af800a560dad0c807472517606d4859cc7d9fe65bbcbe2`.
- `grind-flag-watch-fbcc0e9-v4/results/1793410610476515000.json`, runner SHA-256
  `3125899950f14365764be5e3f67b835e18ac3f3850816f729467190d33ab8aee`.

Both revalidated native ownership, detached the debugger and released the
shared lock. No input, console command, process data write or installation
occurred. The Deck window was subsequently released to the grab chat.

The original native fine-identity lookup is also recovered completely:
initializer 0x47bd410/registration 0x47c8950 produces all 384 entries at
0x778a090 from descriptors at 0x778ac90. Native classification 0x47dd090 writes
the six-element contact tuple; 0x47db980 maps it to physics output +192.
The index is `(((((a*2+b)*2+c)*2+d)*4+e)*6+f)`. This supplies independent
destination evidence and illustrates why matching animation labels alone is
insufficient. The local extractor asserts that every entry was reconstructed.

The remaining executor work is to scope activation to the local pending
grind request and translate the named target into the native orientation
control request with valid reference initialization and geometry. The setting
finding establishes admission, not the nine target control sequences. No
global setting override, direct grind-identity write, guessed stick mapping,
named production executor or new DLL/CI build has been added on that evidence.
The original AI grind action was checked as a possible reusable request path:
23 hash-checked spans (40,296 compressed bytes) include 11 expression kernels;
its apparent native command 0x98a93db2 resolves to the empty retail stub at
0x1f9630 and cannot serve as an executor. Proprietary captures and decoded
payloads remain in ignored local research files.

### Fresh native control baseline and setting probe correction (2026-10-31 UTC)

The coordinated ec35a8e window produced a fresh native rail baseline in the
current `bam` world. The old near-origin coordinates were not reused. Native
lane collision checks rejected two approaches before placement succeeded at
`[633.076416,198.743729,1082.585815]`, with start
`[633.076416,199.772415,1074.585815]`. The native geometry observer completed
129 samples and found a 0.499374 m high rail, with longitudinal extent -2.0 to
1.75 m (0.25 m uncertainty). These coordinates are historical evidence, not
permission to reuse them after a world or editor generation change.

Native controls, Arcade OFF, produced state 401 with fine physics identity 20
(BS 50-50), tuple `[1,0,1,0,1,0]`, independent rail contact and context +4016.
Both the physics clock and cached local skater step were 0.0333333313 seconds.
The rail section lasted about 0.6 seconds. Identity 24 at the end of the rail
was followed by 701 and airborne states; this is not a commanded grind change.
The receipt is `grind-native-control-ec35a8e-v8/results/1793413288498599000-baseline.json`
under the external lab root.

A right-stick negative control retained identity 20 during established contact
(`v9/results/1793413436306041000-right.json`). Temporarily supplying feature
component byte 26 through its qualified native writer also retained that
identity (`v10/results/1793413521443073000-enabled_right.json`). The bounded
experiment recorded 17 qualified writes and restored the original false value.
Neither experiment showed state 406. The separate orientation expression also
reads the setting, so the physics feature alone is insufficient.

The orientation expression is resource key `0x3d9f9b54` (EBX 1648, RES 7881),
`Includes_GrindOrientationCalculation_AG_A6EB92C_EATCommandActionData_13_ss_OnUpdate`.
It reads the same setting at PC 0x18e8 and gates its additional orientation
calculation at 0x1930. Interpreter opcode 38 at RVA 0x16d1a4c confirms the
direction: true follows encoded next PC 0x1b58; false branches to 0x1940 and
performs the additional calculation/write before returning to 0x1434. Enabling
the setting therefore skips this extra write, rather than enabling it. The
native bool getter is already intercepted by
`Extension/Settings/local_gameplay_settings.cpp::gameplay_get_bool`; any eventual
Arcade override must be scoped to the pending request and verified local
expression owner, without saving a profile value.

Two subsequent getter-return experiments did not arm and made zero overrides
(`v11/results/1793413816566814000-enabled_right.json` and
`v12/results/1793413939507067000-enabled_right.json`). Offline inspection found
a probe defect: file GUID `f2e21361-e275-4c24-950e-a2d375dbf98a` was incorrectly
compared against the native object's exported instance GUID at -16. Decoding
the original EBX instance independently confirms the correct object GUID is
`29b0b052-84d4-4fc9-9921-c95ab3ed069b`; both importing assets agree. The probe
is corrected locally, with explicit asset/owner discovery counters. These
failed trials are not negative evidence about the native transition itself.

The fresh fixture was saved through native `arcadefixture save` as a new
`Arcade Rail Fixture` mod (two objects, generation 5/revision 8). Save receipt:
`grind-fixture-ec35a8e-v6/results/1793414156216837000-save.json`. Despite that
runner's inherited read-only metadata, its command list records the console
input and new mod creation; treat it as a mutation receipt. All debugger jobs
were detached, controls neutralized, feature value restored, Arcade ON and
the shared lock released before the grab chat's next installation. This chat
installed nothing. The grab chat subsequently reported installed/loaded source
`dd3e8ebaac4a99102e26296b9c6c373744b28660`, DLL SHA-256
`390e1db09244a62ae8268f1d3f132655fb638a5a05967f28adfda8a2784f7918`.
All ec35a8e-pinned runners are retired. New trials require independent identity
and current-world geometry validation after the other chat releases control.

The local rail-release lease now tolerates an unchanged approach-trick
selection during rail setup/701, because the real new Ollie may take ownership
only on departure. It still rejects an unrelated changed selection, requires
the new native Ollie identity 1 for early flip admission, and cannot use a
lingering approach selection to admit an airborne flip. Production-composed
tests at 30/60/120 Hz cover those cases: 1,362 rail-pop checks pass. Existing
input-session checks pass (5,645 plus 326 jump-idle). The existing pose adapter
maps physical state 406 to the same grind pose, and ComboSession changes its
balance lifecycle only on pose changes; no balance/HUD or combo code changes
are needed on that source evidence. Native continuity still needs live proof.

The next dd3e8eb window independently verified its loaded mapping and resumed
the native Hub before console work. The saved folder is actually `Arcade Rail
Fixture 2`; the first unsuffixed folder has no `bam` park. Read-only inspection
of the saved JSON found two authored rails: obsolete object 1 near
`[153.6571,0,-91.0118]` and current object 2 at the fresh lane above. Loading
the suffixed mod and removing only obsolete authored object 1 through the
undoable native editor allowed adoption of object 2. Saved mod files and stock
world objects were unchanged. Restoration receipt:
`grind-fixture-dd3e8eb-v4/results/1793415242618748000-restore_one.json`.

The corrected getter experiment still did not run: its new mandatory geometry
preflight timed out while armed, with zero samples (`GeometryStatus::timeout`,
numeric **4**, not cancelled). Receipt:
`grind-native-control-dd3e8eb-v2/results/1793415302088816000-enabled_right.json`.
No GDB was launched or setting value overridden. Reset/remount did qualify
state 100 within 0.25 m of the adopted start. Source inspection isolates a
runner ordering defect: geometry activation receives `on_client_tick`'s
`gameplay`, which requires an admitted controller route (`input.available &&
pad.valid`). The runner bound that route only *after* geometry completed.
The next runner binds slot 0 and arms geometry in the same console visit;
opening another console between them would revoke the route again. It still
requires a completed current geometry result; no geometry check is bypassed.

The Deck was explicitly released to the manual chat after this attempt,
mounted and stopped at `[633.193848,199.772415,1074.785645]`, native state 100,
Arcade ON, console/editor closed, controls neutral and shared lock released.
No debugger was attached. The manual chat is independently installing its own
candidate and will return new identity evidence before further grind work.

### Native transition admission reproduced (2026-10-31 UTC)

After the manual chat explicitly released the Deck, an independent identity
probe verified installed and mapped source
`0dfa41e284deaf9653eec41ccbb03cff7d354d70`, DLL SHA-256
`ab585e37f1fbb2f0a5e336c4c017b2ba4deaf38a04e45dffa9777fe5ecb9aff7`,
PID 54951 and the pinned original executable. The authored saved fixture was
restored/adopted in current `bam`, generation 5/revision 9; saved files and
stock objects were unchanged. Binding the controller before arming geometry
resolved the previous preflight failure: 129 native samples completed with
current context and rail height 0.499374 m.

The corrected exported-GUID getter probe armed after 54 getter calls and one
exact asset/owner match. It supplied 36 transient true returns to the two
verified local expressions, without process-data writes or profile changes.
Native right-stick input then produced heading request **1.5704 radians** and
physical transitions **401 → 406 → 401**, twice. Rail contact remained true,
but fine physics identity stayed **20 (BS 50-50)** and actual heading stayed
near zero. The ordinary rail-end 403/identity 24 and subsequent 701/AIR are
not credited as a requested grind change. Physics and cached step remained
0.0333333313 seconds.

Receipt under the external lab root:
`grind-native-control-0dfa41e-v1/results/1793416187038838000-enabled_right.json`.
The runner SHA-256 is
`558dff16dd1f307279560f453735d8400a8c6ea1b82e666b069d0536ff9a3c77`.
GDB exited cleanly and explicitly detached; the feature byte returned false,
controls were neutralized and the shared lock released. Arcade was restored
ON, console/editor closed, state 100. The Deck was explicitly handed to the
grab chat for its separately authorized diagnostic deployment.

This proves native transition admission through the original setting and
control pipeline, **not successful named execution**. The next investigation
targets the one-step return: read-only hardware observations at completed
native initialization (0x4817df0), legitimate completion clear (0x48269e9),
and cleanup (0x4818430), qualified to the local transition actor. Capture its
source/target, requested/actual angles, phase/timing and native contact class.
Do not keep the actor alive by writing its active byte or replace its target
state. Native output 0x4818d50 deliberately publishes the source coarse grind
while transitioning, so unchanged identity during 406 alone is expected;
returning to the source with no rotation is the failure to explain.

No named executor or new DLL/CI build is claimed on this evidence.

### Native transition lifetime defect reproduced (2026-10-31 UTC)

The c8f7971 diagnostic window independently verified the installed/mapped DLL
`09942bc8c76ed4a9ed4145cc7735f15cd011954274ced9deb2c3742b7b2a5f6b`,
PID 56286. A fresh Gamescope capture showed the native Hub, which was dismissed
before restoring/adopting the saved authored fixture in current `bam`, generation
5/revision 9. The trial again required completed current geometry and delivered
native controls. Receipt: `grind-native-control-c8f7971-v1/results/1793417123069659000-enabled_right.json`
under the external lab root.

Read-only hardware breakpoints captured state 406's completed initialization
and cleanup. The first entry had source 401, target 400, requested heading
1.5707963 radians, native active byte 1 and phase 0. Cleanup followed 33.4 ms
later with active byte 0, phase still 0, elapsed 0.03333333 and computed duration
0.39234048. Contact remained true and fine identity remained 20. The separate
native completion-clear breakpoint at 0x48269e9 was never reached. This rules
out legitimate native completion as the reason for the immediate return.

Original executable disassembly explains the loss: state setter 0x47d3fa0
resets context +6200's elapsed-state timer. Transition entry 0x4817b50 sets
actor +766 true. Shared orientation helper 0x4815790 subsequently overwrites
that byte at 0x48158e9 using a predicate and elapsed-state timer. Transition
406's predicate at 0x3b8830 returns **true**, which takes the unconditional-clear
branch. Ordinary actors with a false predicate use the 0.083333-second gate.
The common caller 0x4825525 runs this helper before transition-specific update
0x48264d0. Native output 0x4818b7d copies the cleared byte to output +419 and
the selector abandons 406. Native completion is a distinct later writer.

The next bounded experiment will preserve the actor's already initialized
flag only at this shared writer, leaving native completion, contact/geometry
admission and cleanup untouched. It must not force true after completion or
write an identity, target, state, position or velocity. A successful experiment
still needs actual orientation and independent changed identity before a
production executor can be claimed. Function-level integration must validate
the exact actor, current owner, caller, executable and request lifetime.

The trial restored the setting false, exited GDB cleanly, detached, neutralized
controls and restored Arcade ON. The Deck was explicitly released to the manual
chat. No installation, DLL/CI build or named-executor acceptance occurred.

The subsequent c8 experiment supplied the already initialized flag as the
argument to that one native store. Receipt:
`grind-preserve-control-c8-v1/results/1793417309470490000-enabled_right.json`.
It kept 406 active for approximately 0.33 seconds, advanced the native phase
to 0.67968, and produced actual heading/pitch changes through the original
curves. Ten observations found an existing true flag and a proposed false
shared-writer argument, including after 0.083333 seconds; the predicate
disassembly above explains why timer expiry does not repair this.

The experimental observer then stopped at its strict instantaneous-contact
guard before native completion. This is not evidence of a named transition.
The setting-return experiment also required instantaneous contact even in
406, whereas the original selector explicitly accepts current state 406
without context +4016 contact. The next probe must distinguish initial contact
admission from the native transition actor's ownership during its brief lift:
retain the scoped feature while the verified initialized 406 actor owns the
request, leaving the original selector's geometry/exit checks and bounded
deadline intact. Do not infer rail-exit solely from a transient contact bit.
This trial restored false, detached GDB, neutralized controls and explicitly
released the Deck to the manual chat again. No further hardware action started.

The isolated source now contains a native lifetime/admission integration:
`arcade_native_grind_transition.cpp` uses the existing Detours service at the
verified three-argument orientation function, and the portable preservation
wrapper validates its exact caller, core, active actor, vtable, context and
selected 406 state before and after forwarding. It preserves the existing
boolean only; native completion remains outside the wrapper. Activation needs
a bounded request scope and fresh local gameplay owner. The applied-settings
hook can return true only for the exact exported setting GUID and the two
verified local expression layouts/owners. It never persists a setting. OFF,
controller withdrawal, stale tick, owner loss, timeout and native exit cancel
the scope. With no request, the original function/getter behavior is retained.

This is production support for the executor, **not the named executor itself**:
no input dispatch currently arms the scope, and no named control recipe or
identity acknowledgment is claimed. That wiring remains gated on the corrected
native completion experiment and target control contract. The focused native
lifetime tests pass 464 checks at 30/60/120 Hz under address/undefined behavior
sanitizers; existing settings-source checks (7) and runtime reset composition
(69) pass. Windows compilation and gameplay acceptance remain outstanding.

### Native target selection constraints (continued source trace)

The original jump table at 0x481436c resolves source states 400..405 to
0x481432e, 0x4814187, 0x48141d3, 0x48142a5, 0x48141d3 and
0x4814333. Consequently a source boardslide (400) selects 401 through
this transition, while source 401 selects 400 for a requested absolute
heading strictly inside the native 55–125 degree band and 403 otherwise.
Source 403 can select a tip/backslash class only with the additional native
pitch and geometry conditions. Thus a single arbitrary cardinal stick request
cannot be claimed to implement every named destination from every origin.
The original class selector and its geometry choices must remain authoritative.

The fine-identity lookup adds another constraint: holding the baseline
backwards-orientation tuple fixed does not admit all nine requested names.
For example, baseline side=1/orientation=1 contains native identity 23
(nosegrind), but not 21 (noseslide), 31 (bluntslide), or 32 (crooked).
A request must account for the actual orientation/contact tuple; a fixed
numeric offset from the starting identity is not sufficient execution proof.
No control recipe is inferred solely from those enum values.

The function-level lifetime wrapper was checked against the intervening
original orientation calculation: the post-store calls at 0x48158f9 and
0x481591e perform geometry/curve calculation, and transition completion
remains in the later actor update at 0x48269e9. The live shared-store probe
and production whole-function preservation still require comparison in the
finished candidate; their tests are not interchangeable gameplay acceptance.

Manual released the Deck to the grab chat for its bounded c69dd24 Method
trial. Grind remains offline until that chat explicitly releases it. Reviewed
identity/fixture/control runners were staged for source
c69dd24af196e2a43eb3b290ad92232d132b45d2 and DLL
6cf3320e335d499106eff9473a981748dd767b401d9fbbad8cc9400359cce8bc;
staging did not execute a Deck action or install a grind build.

### Corrected brief-lift experiment and geometry exit

The c69dd24 window verified PID 58428 and the installed/mapped hash above.
The authored fixture was restored in current bam generation 5/revision 9;
fresh native collision sampling completed all 129 samples at height
0.499374 m. Identity receipt: `grind-identity-c69dd24-v1/results/1793419042096040000.json`.
Fixture receipt: `grind-fixture-c69dd24-v1/results/1793419073274770000-restore_one.json`.

An immediate-contact conditioned request produced no 406 entry and no input
heading change. It delivered right stick at first native 401/contact/identity20
(2.023 s), showing that first contact alone is too early to assume the native
control reference is established. Receipt:
`grind-preserve-contact-c69dd24-v1/results/1793419136940239000-enabled_right.json`.
No named change is credited to the later ordinary rail-end 403/identity24.

Repeating the previously reproduced 2.1 s right-stick request with the fixed
brief-lift guard did enter 406 at 2.202 s and retained it for 0.4 seconds.
The native curves reached phase 0.93476 and actual heading 1.062394 radians,
with requested heading 1.570451. The separate native completion writer was
never reached. The original selector chose AIR200 at 2.605 s; cleanup found
active=1, source401, target400, and identity still20. Receipt:
`grind-preserve-control-c69dd24-v1/results/1793419209645540000-enabled_right.json`.
Both physics cadence and cached step were 0.0333333313 seconds.

The observed output+417 rose before exit. Source establishes that this field
copies actor+912 (0x4818b59..0x4818b65), which transition update sets when the
native segment lookup 0x481f350 returns null (0x4826632..0x4826643). That
lookup enumerates the original rail segments and checks the local body's
longitudinal position between segment endpoints. The selector can then take
0x47468d8 to AIR200. The short rail is a plausible cause, but the next probe
adds the actor endpoints, correction vector and native contact point to
separate an actual rail end from another missing-segment condition. No exit
condition was suppressed and no named execution success is claimed.

The second trial's optional log collector encountered automatic log rotation;
its direct native samples and debugger receipt remained intact, and final
fresh console status succeeded. Both trials returned GDB0, explicitly
detached, restored the feature false, neutralized/detached controls and
restored Arcade ON. The Deck was explicitly released back to the manual chat.
No new DLL/CI build, installation, source upload or merge occurred.

### Original native completion observed

The subsequent up-control trial reached the original completion writer:
`grind-preserve-geometry-c69dd24-v1/results/1793419918078273000-enabled_up.json`.
It entered 401 at 1.938 s, 406 at 2.169 s and successor 403 at 2.466 s.
Native source/target were 401/403, requested pitch was 0.400000095 radians,
computed duration 0.100000024 s and settle time 0.25 s. At the original
0x48269e9 completion writer, phase was 1, elapsed 0.29999998 s, actual pitch
0.35557747 radians and native contact class 1. Cleanup observed active=0.
This establishes original actor execution and completion with the scoped
admission/lifetime correction; no target, identity, position or velocity was
written. Physics cadence and cached step were both 0.0333333313 s.

The original actor reported endpoints at Z1080.58496 and Z1084.58496,
entry contact point Z1082.6973 and forward velocity 4.708291 m/s. The
successor had only about 65 ms before state 701 at the rail end. Fine identity
remained 20 during that successor contact. A subsequent brief re-entry with
identity 26 is not credited as the requested named transition. Longer settled
contact remains necessary for independent named identity and animation proof.

The production integration now separately observes the original actor update
at 0x48264d0, from exact caller return 0x4825590. It publishes a bounded
owner/request-scoped completion only for an original active 1-to-0 edge with
phase 1 and finite orientation, revalidating actor ownership before and after.
Cleanup, shared-helper clearing, stale owners and repeated polls cannot create
completion. This read-only evidence is distinct from named grind acknowledgment;
the named executor is still unwired. The strict sanitizer test passes 597
checks, including completion and cancellation cases at 30/60/120 Hz.

The successful trial returned GDB0, explicitly detached, restored the feature
false, restored Arcade ON and neutralized/detached controls. The Deck was
explicitly released to the grab chat. No DLL/CI build or installation occurred.

### Slower approach negative controls

After Grab explicitly released the Deck, identity receipt
`grind-identity-c69dd24-v1/results/1793421077039441000.json` verified the same
installed/mapped c69 DLL in new PID 60959. The saved fixture was restored in
the fresh bam generation 5/revision 9, then its native collision was measured
again. All 129 samples completed at height 0.499374 m; stale coordinates alone
were not used as eligibility.

Two bounded original-control trials attempted to preserve more contact time
by applying native B braking before the native stick Ollie. The first receipt,
`grind-slow-control-c69dd24-v2/results/1793421171886714000-enabled_up.json`,
measured braking to 3.98 m/s, above its 3.8 m/s approach gate. It withheld the
jump and named control, then timed out. The second receipt,
`grind-slow-control-c69dd24-v3/results/1793421321188084000-enabled_up.json`,
admitted the approach at 3.940675 m/s and Z1078.233887, with the speed ceiling
adjusted to 4.2 m/s. Preparation began at physical write 1257.795 ms and the
flick at 1657.887 ms. Native state became 103 at 1.774 s, then 504 at 1.841 s;
there was no established rail contact, AIR200, 406 entry or setting override.
Its independent rail gate withheld the later Up request. Neither trial proves
or disproves named execution; the brake/approach did not provide a valid origin.

Both trials returned GDB0, explicitly detached, restored the experimental
feature false, neutralized/detached controls and restored Arcade ON. The Deck
was explicitly returned to Manual after the agreed two trials. Final fresh
state was 504, speed 0.002888, position (633.757507,198.835953,1083.494019).
Grind did not install a DLL, change the rate or save a park file.

The next fixture candidate uses the original catalog's long round steel rail
at native scale 1 and the previously working approach. The old short rail's
ray sample can establish only the approach, not the replacement's geometry.
Before delivering any transition input, a new read-only gate must verify the
active native grind actor, contact bits and actual segment endpoints. Offline
vtable inspection confirms states 400..405 share update 0x4821b80 and the
base layout: vtables 0x65e69c8, 0x65e6ad8, 0x65e6be8, 0x65e6cf8,
0x65e6e08, 0x65e68b8 respectively; virtual +64 returns the exact class.
Prepared temporary editor operations use native transactions and bounded undo
with exact generation/revision acknowledgment. This longer fixture runner has
not run and supplies no new native acceptance.

### Held native control branch source trace

The Raw Nose condition (EBX30997/RES14548) provides the missing distinction
between a fresh cardinal press and rotating an already held control. Original
native call registrations resolve `f3918ee3` to 0x16eb250 (scalar less-than),
`02dd315a` to 0x16eb190 (greater-than), `1b74206c` to 0x16eb600 (not),
`eabfec6f` to 0x16eb6e0 (or) and `8eb2196d` to 0x16e7130 (and).
The angular-range helper `ef02c056` is 0x831ee0 and wraps angles before
the inclusive range comparison, optionally converting degree bounds.

The Raw Nose kernel compares magnitude with 0.5 and uses a +/-60-degree
acquisition window on the fresh directional path. On the retained-state path,
it compares the current raw enum with Nose=1; the relevant normalized-axis
escape comparison uses the out-of-range +/-2 threshold. This supports tracing
a held angular sweep separately from a release/repress. Up's update computes
heading from the signed angle to the captured reference and pitch from the
stick magnitude (scaled by pi, with native sign conditions). Thus a fresh
Right request's observed pitch=0 does not establish what Up-then-Right while
held will request. The native raw-state/target-angle sequence and resulting
named grind still need a live positive control; no nine-target recipe is
inferred or enabled from this source trace alone.

### Current contact admission and next bounded trial

The production transition support now reads a validated current native segment
before arming or granting initial feature admission. It brackets the active
400..405 actor/vtable/context, selector output, fine identity, both contact
bits, missing-segment flag and finite nondegenerate endpoints with fresh owner
checks. Replaced actors, selectors, segments or identities invalidate the
sample. A 406 lift cannot be used as an initial contact, but an already admitted
406 continues to use its separately verified lifetime scope. Nothing is written
by this reader; the original native geometry/exit policy remains authoritative.

The original shared initializer at 0x48178d9 copies the context's segment data
to actor +784, and 0x4817919 clears its missing-segment byte +912. Common update
0x4821cd1 passes that same segment layout to the original geometry function.
These supplement the vtable evidence above. Live endpoint readings on active
400..405 actors, rather than the already observed 406 actor, still need the
next trial. The reader is admission evidence, not an atomic physics snapshot,
proof of remaining rail length, or named-grind acknowledgment.

Strict address/undefined-behavior sanitizer checks now pass 625 assertions,
including native lifetime/completion at 30/60/120 Hz and contact/geometry
ownership changes. Existing reset/dispatch composition passes 69 checks and
settings-source checks pass 7. The native lifetime/contact test was added to
the existing Windows preflight and portable targeted test lists; no workflow
was dispatched and no DLL was built.

Manual has installed source c45f8832da783c4824b8ca8f31eebcd0a422e241 with DLL
SHA-256 9081eb1f6231ef87713e016e2778c291faf53aa4b0b24822ef096749f473e31d,
but has not confirmed it loaded. Manual explicitly retained Deck priority
while waiting for the user's availability response. Grind remains offline;
an idle chat is not a hardware release.

The next runner was prepared locally with those exact identity pins. Source
review caught that native layout edits require the park editor to be open;
the candidate now opens it around its two bounded transactions, closes it
before input, and includes failure cleanup. It uses original native undo to
restore its own edits only after exact generation/revision validation. The
prepared files remain under build/grind-investigation/prepared-c45-long-rail:

- Identity runner: 8657ec878ca3fe9ad3b011c60d356686b29d65088deb5762f7c67fb6742c9d24.
- Fixture runner: 59cb3b112c333cd536f33dc06db9259b0a6e188ce87b7958d470e24c3daff3dd.
- Long-rail runner: 6de529c96575de6dbcc5253027ca3dbcc0935af8d84faf0e3757909b82fb667d.

They have not been staged into the external lab or executed. Execution still
requires explicit hardware release, review/staging of immutable runners, fresh
loaded identity and current world/geometry. The missing production piece is
unchanged: no named control recipe or dispatcher calls native_grind::arm.
The next acceptance observation must distinguish a completed native actor
from an actual new fine identity and corresponding animation during sustained
contact. No new build is ready for testing yet.

### c45 window: setup stopped before native transition input

Manual explicitly released one bounded long-rail window after its revert tests.
Fresh identity receipt `grind-identity-c45f883-v1/results/1793461619951390000.json`
verified the c45 source/DLL above, mapped to PID 64185/window 56623105. The first
status query returned no fresh console response. A fresh Gamescope capture
showed the native Hub; the reviewed resume/status action dismissed it and
verified bam generation 5/revision 5, state 504, stationary, native menu closed.
No compositor setting was changed.

Fixture preparation revealed two distinctions that the previous offline review
missed. `arcadefixture adopt` explicitly requires an editor project beginning
with `Arcade Rail Fixture`; `mod-load` preserves the old project and therefore
cannot satisfy that requirement. The original `mod-open` path then adopted the
fixture. Neither operation saved a fixture file. Also, the command's UI
availability predicate mentions an open editor, but `edit_local_park` itself
admits native transactions through its session/generation/revision/idle guards.
In this window `parkeditor on` returned `Debug request rejected` while the
explicit native delete transactions succeeded with the editor closed. Thus the
prior claim that opening the UI is necessary for these console transactions
was too broad. No editor-UI requirement was bypassed in production code.

The single long-rail invocation was
`grind-long-control-c45-v2/results/1793462022118894000-enabled_up.json`.
It reset/mounted through native controls and measured the current short rail:
all 129 collision samples completed, height 0.499374 m, generation 5/revision
13. Its attempted `editor delete bam 5 13 2` produced no fresh console echo or
revision change. The runner's object-command typing used 20 ms instead of the
previously working 100 ms; this is a delivery suspect, not proven causation.
The revision guard rejected the operation before creating the long rail,
starting the approach, attaching GDB or delivering any named transition input.
There were no 406/native completion/identity observations to accept.

The runner neutralized/detached and restored Arcade ON. The four preparation
transactions (two loads and two duplicate deletions) were then each undone
through original native history, with exact revision acknowledgments
13 -> 15 -> 17 -> 19 -> 21. The cleanup's final `present == false` assertion was
incorrect: initial `configured == false` makes fixture status report no bound
fixture even when the matching rail already exists in the world. It is not an
inventory assertion. All four undos had completed before that assertion; no
additional undo was sent. Fresh final receipt
`grind-fixture-c45f883-v1/results/1793462160817634000-status.json` independently
confirmed generation 5/revision 21, native state 100/mode 1, speed 0.000919 m/s,
position (633.198120,199.772415,1074.786621), Arcade ON, closed editor/menu and
the original matching rail present. The prior layout was restored by native
history; fixture-adoption metadata/editor project selection now references the
fixture. This caveat was explicitly handed back to Manual. No rate change,
saved-fixture write, DLL installation or new build occurred.

Manual acknowledged release and resumed ownership for its next candidate.
Do not reuse the c45 pin after its forthcoming restart without checking the
new installed/mapped identity. There is no further hardware window granted.

Offline preparation now restores 100 ms typing, allows a bounded 20-second
subprocess deadline for the longer text command, requires a fresh exact native
console echo, rejects native error responses and preserves diagnostic output
if cleanup raises. Old log lines are captured before a command so they cannot
supply its acknowledgment. Native revision/contact/geometry gates remain
required. Candidate `build/grind-investigation/grind-native-long-c45-v3.py`
has SHA-256 0f13b2b9afaa43b377571e450fa9c7bfc40ee89f15da90bc172424826b84198d.
It is locally prepared only, not staged or executed, and still needs review
against the next live identity and window. The named production executor
remains absent; this failed setup provides no new gameplay acceptance. Five
inert checks of the actual prepared console helper pass: fresh echo succeeds,
missing/stale echo, wrong echo, native error and partial batch delivery are
rejected, with console-close cleanup retained. These validate runner delivery
checks only and send no network or hardware input.

### Offline classification publication trace (2026-10-31 16 UTC)

While Manual retains Deck ownership for its next candidate, further tracing of
the original classifier separates three different observations that the next
trial must not conflate. In `0x47dd090`, the six-word candidate at classifier
+208 is published to physics output +328; accepted +160 is published to +352;
settled +184 is published to +376. The exact tuple comparator `0x47e0c40`
compares all six words. When the classifier's previous state word at +16 is
400, a changed candidate resets counter +264 to zero and leaves the accepted
tuple unchanged. A subsequent equal candidate increments that counter and can
publish the accepted tuple; only a counter greater than 12 also publishes the
settled tuple. The alternate previous-state branch sets the counter to 13 and
copies immediately. This state word comes from the selector's page +56, offset
+24; it must not be assumed identical to the active 400..405 actor class.

The consumer `0x47db980` calls that classifier first. At `0x47db9c7` it selects
output +352, and the lookup at `0x47dba04..0x47dba4f` indexes that accepted
tuple and writes fine identity +192. Output +376 also supplies a separate
formatted classification string, but does not supply this identity lookup.
Thus neither a pending candidate tuple nor the later settled tuple is a
substitute for the independent fine identity. Nor should an executor add a
fixed 13-frame delay to every acknowledgment. The native publication behavior
must be observed together with animation, original completion and contact.
This trace does not establish why the earlier short-rail trial retained
identity 20; that receipt did not capture all three stages.

The locally prepared v4 long-rail observer adds a bounded read of these three
tuples and both selector state words, bracketing owner, output/page pointers,
contact, physics state, identity and tuple bytes. Changed reads are marked
unstable, not accepted as a new action. Stable repeated reads still do not
prove an atomic physics frame. Five inert checks pass for stable data, a state
change, a tuple change, owner loss and an out-of-range tuple. No process or
hardware access occurred. Candidate
`build/grind-investigation/grind-native-long-c45-v4.py` has SHA-256
`2188b78f5c9b526079d83f91baaa78a0e669d6b9666bf6a9c40d7faaa41a6021`.
It remains unstaged, unexecuted and pinned to the obsolete c45 candidate;
fresh pins and review are required after Manual's restart. No named recipe,
production execution, gameplay acceptance or build readiness is claimed.

### Native control-reference reset provenance (2026-10-31 22 UTC)

The previously unresolved +0xc4 flag in the control-reference capture condition
now has an original producer chain. Type GUID
`b4bf4485-b964-948e-eeed-101f95da4762` resolves in the pinned executable to
`AnimationSkaterShared` (descriptor 0x7fb3600, size 0xd0). Its native field
table at 0x8320cd0 has 27 entries; field hash 0x2602a1a5 is the bool at +196.
The distinct GUID `7110f29d-629e-e036-b6e4-5347dafa899b` names
`SkatePhysicsShared`. They are not interchangeable component owners.

At 0x102f284/0x102f28b the original publisher copies physics output byte +420
to `AnimationSkaterShared` +196. The grind output writer at 0x4818b8f..0x4818b9b
copies active grind actor +768 to that output byte. Shared update 0x4821b95
clears +768 each update. The segment-change handler 0x481dfc0 then compares
the actor's existing segment at +784 with the incoming segment by calling
0x47e0da0, inverts the equality result and writes +768 at 0x481dfe8. The native
equality function compares two opaque 64-bit words at segment +80/+88; it does
not compare just the endpoints. This establishes a native segment-change
route into control-reference recapture. It does not justify writing the flag
or treating it as a requested grind identity.

The production admission reader now includes those two opaque segment identity
words (actor +864/+872) in its bracketed sample. A changed word invalidates the
sample even if endpoints, active actor and selected grind remain equal. Failed
reads reject admission. No invented nonzero/pointer requirement is applied to
the words; original native equality supplies the contract. This only strengthens
the existing transition support's contact admission; it does not activate a
named executor or change first-grind balance, input routing or physics.

The focused strict C++20 test passed 629 assertions with address and undefined
behavior sanitizers, including the existing 30/60/120 Hz lifetime/completion
checks and added equal-endpoint segment replacement cases. DLL/CI builds remain
withheld. An unstaged local v5 observer also records these segment words, the
actor's change flag and output +420 for the next native trace. Its SHA-256 is
`7e1c9c26a687491e75ce8f65a943aab641f4354d268b58ff20209a1daebcb11a`;
it still has obsolete c45 pins and must be repinned/reviewed before use. Manual
continues to own the Deck and is moving to another park, so earlier fixture
coordinates cannot authorize the next trial. No hardware access occurred.

### Short 2ad20da handoff: fresh scene readiness only

Manual explicitly granted a maximum six-minute window, with reclaim priority,
while its next CI run was building. The handoff reported the custom
`skate3_megapark` loaded and the skater offboard near its spawn wall. The reviewed
read-only identity runner confirmed source
`2ad20da1d5696d69a7fcfc58637a8f0808afb07d`, DLL SHA-256
`855c5b691c5d313b26290770b2c88ab53c2271f5a9e82bf44df7fc857ce72383`,
and PID68008 mapping the installed DLL inode. Original executable hash matched.
Receipt: `grind-readiness-2ad20da-v1/results/1793487084338357000.json`.

Fresh Gamescope capture showed Steam idle artwork. The first status query
received no fresh response. A reviewed single Shift press/release on the
identity-checked Skate window dismissed the idle artwork; a subsequent capture
showed the native Hub. The restricted resume/status runner dismissed the Hub
and obtained two consistent native responses. Final receipt
`grind-readiness-2ad20da-v1/results/1793487228023698000-resume_status.json`
reported native map key bam, generation8/revision8, unconfigured fixture,
position (130.811523,0.949989,-135.971436), heading226.238358, state504,
speed0, ArcadeON, closed editor/menu. The native map key alone does not prove
which custom park is loaded; no world/mod load command was issued.

The Deck was explicitly released before the six-minute deadline, and Manual
acknowledged release before assigning its next window to Grab. All operations
completed and locks/keyboard keys were released; no controller device was
created, debugger attached, fixture edited, gameplay input sent, setting/rate
changed, or DLL built/installed. No native grind trial occurred. The old saved
fixture and approach coordinates remain ineligible in this scene.

Follow-up source review also corrects an earlier diagnostic interpretation:
`ok: Debug request rejected.` does **not** prove that `parkeditor on` was
rejected. `Runtime/console_bridge.cpp::queue_console_debug` passes that text
as a fallback to `console_callback_result`; when the queue succeeds without
result text, it still prints the fallback with an `ok:` prefix. Execution is
asynchronous. Actual editor admission requires the subsequent native editor
state; `debug_action` also requires local camera/control phase and host object
placement permission. Unlike generic native editor transactions,
`arcadefixture place` itself explicitly requires editor-open, stopped riding
state100 and its twelve native ground-ray lane checks. No fixture runner should
treat queue text as proof of execution or bypass these requirements. This
clarification changes no production editor/camera behavior.

### Successor publication and classifier family (offline follow-up)

The original output publisher now resolves the previously ambiguous selector
state words. In core output function `0x47da450`, the active actor's virtual
`+64` return is written to state page `+28` at `0x47da93b`. The same return is
passed to `0x481bec0` at `0x47daaa9`, and the resulting family is written to
state page `+24` at `0x47dab1a`. The original family function maps every state
400..499 to 400. Consequently the classifier's previous-family-400 branch
also applies to 401, 403 and 406 during uninterrupted grind publication; its
comparison is not limited to the boardslide actor 400. A changed six-word
candidate must survive a subsequent identical classifier update before the
accepted tuple changes. A changed component resets that progress again.
This is a native update condition, not a fixed wall-time acknowledgment delay.

The actor output call is virtual `+80` at `0x47db0ff`. Vtable inspection and
the original callees independently establish the coarse-family publisher:
401 uses `0x4818880` and writes output `+180 = 0`; 403 uses `0x48188b0` and
writes `+180 = 1`. During 406, `0x4818d50` deliberately maps the source actor
at `+1040` instead. Thus a 401 -> 406 -> 403 request has distinct milestones:
original completion, successor selection, successor coarse publication,
candidate tuple, accepted tuple/fine identity, and animation. Neither the
selected state nor the completion hook substitutes for the later observations.

The static lookup's runtime provenance was also checked: initializer
`0x47bd410` calls `0x47c8950`, then `0x47bd527..0x47bd539` copies each 64-byte
descriptor's integer from table `0x778ac90` into the compact table `0x778a090`
used by `0x47dba47`. All reconstructed rows that yield fine identity 20 have
coarse tuple element zero. None of the coarse-one rows yields 20. Therefore
the earlier short-rail successor's lingering identity 20 is not an alternative
name for the completed 403 action. That old trace lacks the necessary coarse
and candidate/accepted observations to determine where publication stopped;
it still cannot qualify a named control recipe.

The locally prepared v6 reader records coarse output `+180` with the existing
bracketed tuple sample and labels the two published state words separately.
Seven inert memory cases pass, including changed coarse output and changed
published actor state. Candidate
`build/grind-investigation/grind-native-long-c45-v6.py` has SHA-256
`4b499fe0e488819db92e0e4cf64b227e0ada2bd26937919406c93a734c3eb9ad`.
It remains unexecuted with obsolete c45 pins; fresh identity, geometry and
external runner review are still required. This is read-only evidence
preparation, not a production executor or gameplay acceptance. No additional
Deck access, DLL/CI build or protected-system change occurred.

### be22b1a window: native fixture admission rejected this lane

After Grab released its hardware window, Manual explicitly granted Grind five
minutes beginning approximately 23:02:38 UTC. The immutable identity runner
verified source `be22b1a2f731485f8175c5fc8d8645598bb6a322`, DLL SHA-256
`98aa97e1c4322b99023d286d0ccdce7bf3be87248e56d9195150147897e5b179`,
PID69119 mapping the installed inode, and the original executable hash.
The handoff reported San Vansterdam/Hedgemont Blue Light Beach after restart.
Fresh native status showed bam generation5/revision5, stopped riding state100,
position approximately (626.613159,199.972504,1065.305298), heading341.799255,
ArcadeON, native menu/editor closed, and no configured fixture. Receipt:
`grind-readiness-be22b1a-v1/results/1793487810169901000-status.json`.

A separately reviewed bounded probe then opened the native editor and checked
its actual state before requesting placement. This independently confirmed the
queue-message finding: `parkeditor on` printed `ok: Debug request rejected.`,
but the next native status reported `editor_open:true`. The subsequent native
`arcadefixture place` explicitly rejected the lane: `Fixture lane lacks a
continuous flat native collision surface.` No bypass or fallback placement was
attempted. No rail was created; generation/revision remained 5/5 throughout,
and native cleanup restored editor/menu closed. Final state100, speed0.00138,
position (626.613159,199.972504,1065.305420), ArcadeON. The probe contains bounded
undo for its own acknowledged placement, but that branch was not needed.
Receipt: `grind-readiness-be22b1a-v1/results/1793487896287498000-fixture_probe.json`.
Runner SHA-256: `a1a203c5dd27a6226dba46ece552385c88328b5637103fb69f63541f2d256ed5`.

The Deck was explicitly released to Manual early at approximately 23:05:35 UTC.
No virtual controller, debugger, gameplay input, teleport, world/layout/save,
rate, persistent setting, DLL installation or new build occurred. This narrows
the next setup requirement to a freshly collision-qualified lane; it supplies
no 406 completion, named identity change or Ollie-out acceptance.

### Native completion boundary and target-dependent heading margin

Further offline inspection of the hash-verified executable identifies an
over-permissive completion observation in the local candidate. The original
406 update loads exactly 1.0 from `0x606e4e8`, clamps phase to it at
`0x48265c4`, and at `0x48268e6..0x4826900` skips the completion branch when
phase is below 1. The observer previously accepted phase >=0.99. A focused
regression failed against that implementation for 0.99 with an active 1->0
edge. The observer now requires exactly 1.0; it still forwards the original
update and never changes its result or lifetime byte. The next representable
float below/above 1 and NaN are also rejected. All 637 strict C++20 sanitizer
checks pass, including the existing 30/60/120 Hz completion/lifetime cases.
This tightens completion evidence only; it does not turn completion into named
identity or animation acknowledgment.

The heading-margin helper `0x48281b0` is also now resolved. It reads the
transition's target class at +1044, returning +5 degrees for target403 and
-5 otherwise. During initial target701 it instead reads source +1040,
returning -5 for source403 and +5 otherwise. The pinned constants are +5 at
`0x606f8c0` and -5 at `0x61964c8`. This is a target-dependent angular margin,
not a measurement of rail geometry as the earlier shorthand suggested.
`0x48140e0` adds/subtracts it from its 55/125-degree bounds, yielding strict
60..120 or 50..130-degree bands on the affected source branches. Separate
pitch, side-vector and contact conditions still select the physical class.
No named stick mapping is inferred from these thresholds alone.

Manual supplied the next candidate's source/DLL/size for offline preparation:
`56f6c7b4caad7da15d0ce672aebca07021c42a0f`,
`fbf20e96ac392abc4f496e5d8c09ea3344aff8a1cd6e6982aa4bd6653de7ed0c`,
26604032 bytes; later reported mapped PID70570 and receipt
`fc68fb0fa0564d9a985ba19218b10f75`. These are coordination evidence, not an
independent Grind check or hardware release. Identity/status/fixture runners
are prepared locally against the new pins and remain unstaged/unexecuted.
Manual retains the Deck and is moving to skate3_megapark. No hardware access
or DLL/CI build occurred in this follow-up. The blocker remains a qualified
native control sequence producing the requested identity and animation during
sustained contact; the production named dispatcher remains unwired.

### 56f6c7b megapark window: fresh flat lane admitted

Manual explicitly released a three-minute window at approximately 23:26:29 UTC.
Independent read-only verification confirmed the supplied 56f6c7b source,
fbf20e96 DLL hash/26604032-byte size, PID70570 mapped inode and original exe hash.
Receipt: `grind-readiness-56f6c7b-v1/results/1793489202731653000.json`.
Native status reported map bam generation8/revision8, state100, ArcadeON,
closed menu/editor and position (133.628235,1.028666,-134.388840). The handoff
identified the current world as skate3_megapark; Grind issued no world load.

This position passed the original twelve-ray flat-lane admission. One original
short rail spawned at (133.628250,-0.0000101,-126.388855), identity rotation and
scale1, with revision8 -> 10. The separate `arcadefixture geometry` request
was rejected at admission and recorded zero samples; native placement success
is not rail-contact/segment evidence. The existing source requires gameplay
admission for geometry, which this keyboard-only probe did not establish.
The prepared controller trial must acquire its own fresh route/capture and
native geometry evidence; it cannot promote this placement into acceptance.

The probe undid exactly its own placement through `editor undo bam 8 10`.
Native status independently confirmed revision12 and fixture present=false.
Runtime fixture metadata remains configured=true after undo; no saved park or
layout modification remains. Final state100, speed0.000193, original position
and heading198.037842, ArcadeON, menu/editor closed. Receipt:
`grind-readiness-56f6c7b-v1/results/1793489245345725000-fixture_probe.json`.
The Deck was explicitly handed back early at approximately 23:27:45 UTC.
No virtual pad, debugger, gameplay input, teleport, setting/rate change or DLL
operation occurred. This identifies a fresh candidate lane in the preserved
world, but proves neither a native grind transition nor rail-exit Ollie.

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


### Follow-up: exact geometry admission and owned redo plan

The prior geometry probe also failed a definite source requirement: heading
must be within three degrees of zero, but the observed heading was198.037842.
Controller availability alone cannot repair that. The next candidate uses the
existing native fixture reset and checks the resulting alignment before geometry.

Prepared immutable runner grind-native-long-56f6c7b-v1/trial.py SHA-256
`d3414eb32066e765e294ef3608629b14025f1be20658c3b7e11eba0ab2bff9c7`
permits baseline/enabled_up only. It requires exact PID70570, bam generation8/
revision12 and the original fixture metadata before redoing its own placement.
The original native history independently compares the entire layout. Redo,
short deletion and long placement are undone in reverse; changed ownership
prevents unrelated undo. Thirteen inert cases cover admission, interrupted edit
acknowledgment and cleanup. Forward edits stop after95s to reserve cleanup time
inside the160s session. This is an investigative control trial, not a named
production executor.

Host ENOSPC temporarily blocked coordination review and even document appends.
Only4,407,240 bytes of reproducible local test executables were removed, with
source and result logs retained. At23:37UTC a4MiB durable write and coordination
message succeeded. Manual released the Deck; Grind acknowledged ownership.
The first trial verified installed identity but stopped before edits/gameplay
at native_in_menu=true. Receipt1793489921974898000-enabled_up.json has zero
native action samples. A separately reviewed single-Escape runner then verified
menu closed at the same bam generation8/revision12; skater position was now
(201.708847,1.028667,-119.683701). Native redo/reset must revalidate the original
fixture lane; no previous position is promoted to current collision evidence.

### Long-rail native completion reproduced; named publication stalls

The aligned retry completed all 129 original geometry samples, measuring the
short rail at height0.499924 with sampled Z bounds -1.75..2.0. It then correctly
failed at `editor delete`: the command layer, unlike undo/redo, requires the
editor UI open (`park_editor_commands.cpp`, object-command inspection). Its
own redo was undone, ending revision16 with the fixture absent. The next
immutable runner opened and verified the editor only during replacement,
closed it before input, and retained all owner/revision/undo checks.

Two actual native control trials then ran on the original long round rail:

* v2 runner SHA-256
  `41586f4866fc23579d31bfa7f8befa53e91bebf1205d82c1d5a5b9c8154f6336`,
  receipt `grind-native-long-56f6c7b-v2/results/1793490167088765000-enabled_up.json`,
  receipt SHA-256
  `24f1dfe84b4e0c68833c7191094f02649fd26f7dfcc76bd292a989d7ad288e8b`.
* v3 runner SHA-256
  `b6a6403fc724f8d3b83372abe9402df2109202d45e6cc2d70e58a7750a1c649f`,
  receipt `grind-native-long-56f6c7b-v3/results/1793490345418881000-enabled_up.json`,
  receipt SHA-256
  `41e8a2972bac71eb357c52b4bffeb70c622d65e762bd66dc5f015d339f5b629c`.

Both independently acquired native401 contact on the six-metre segment from
(133.627930,0.490000,-121.388672) to (133.627930,0.490000,-127.388672), with
missing-segment=false and a stable opaque segment identity before control.
Both original physics dt and cached skater dt were0.0333333313; these are
30Hz trials, not 60/120Hz acceptance. The native action ran401 ->406 ->403,
requested pitch0.4, completed at phase1/elapsed0.29999998, and cleaned up with
active=false. The original native completion writer remained untouched.
The successor retained contact for approximately0.7s (58 samples spanning
688ms/681ms), substantially longer than the prior short-rail observation.

This finally locates the named-publication failure in the captured path.
Output coarse +180 changes to1 and the new candidate has coarse tuple element1,
but its orientation element alternates0/1 each native frame. Candidates
`[1,0,1,0,0,1]` and `[1,0,1,0,1,1]` both map to native identity23 in the original
lookup. Nevertheless, the original exact six-word comparator at0x47e0c40 rejects
every alternating pair. The0x47dd26b branch repeatedly resets the publication
counter and retains the old accepted tuple, so actual fine identity stays20
through sustained403 contact. Native identity26 appears only after701 exit and
brief re-entry; it is explicitly excluded from transition success.

v3 supplied X=2048 alongside Up as a diagnostic angle bias. It reproduced the
same stall. The native406 request heading remained approximately-0.000345;
the trace does not demonstrate a useful native heading bias. A uinput write
receipt is not proof of game-side X consumption, and the retained capture did
not contain the corresponding raw-X row. Do not infer either dead-zone loss or
a working named recipe from this negative control. The previously traced
captured-reference/held-sweep input path still needs direct native evidence.
The classifier's0/1 orientation branch is a native dot-product sign test
(0x47c8425..0x47c8465), but its exact input-vector cause remains unmeasured.
No comparator override, identity write or synthetic acknowledgment was added.

Both debuggers exited0, the transient native feature restored to false, and
all three owned native transactions were undone per trial. The final state was
ArcadeON, editor/menu closed, bam generation8/revision40, fixture absent with
configured metadata retained, state100, position
(131.685287,1.028672,-41.105190), heading358.553558, speed0.193049.
Manual reclaimed the Deck during v3; Grind finished its bounded cleanup and
explicitly handed hardware back with no pending job. No further hardware
operation is authorized by that concluded window.

These trials establish actual actor execution and a concrete classification
publication blocker, not named-grind gameplay acceptance. Animation/HUD/combo
continuity was not established in these ArcadeOFF native controls. The named
production dispatcher is still unwired; the Ollie/early-flip candidate remains
offline-only. No DLL/CI build, installation, merge or release occurred.

### Held sweep: actual named publication and first production executor

The next bounded native control supplied Up first, then changed X to12000
after200ms while continuing to hold Up. The initial evdev vector was
(0,-32767), followed by(12000,-32767), released after1000ms. This exercises
the original captured-reference path rather than supplying an initial diagonal.
The immutable v4 runner SHA-256 is
`08b6f05990f4f63de53f34e317dc6843b1609a1e417dcce268e46a3e9a1c3f05`.
Its lossless receipt is
`~/.local/share/reskate-arcade-lab/grind-native-long-56f6c7b-v4/results/held-sweep.json.gz`,
SHA-256 `af29ff22cd89a46db955665127fc9472107897211087d5c43caf4d189545ff9e`.
It independently verified installed56f6c7b/fbf20e96, executablefbce74d5,
PID70570 and the mapped DLL inode before input. Original physics and cached
step were both0.0333333313: this remains30Hz native evidence.

Relative to the first observer sample, native401/fine20 begins2049.400ms,
406 begins2213.359ms, successor403 begins2551.747ms, and **fine23 publishes
at2590.283ms while still403**. It remains23 until701 exit at3216.578ms.
Source and successor retain the same six-metre endpoints and opaque native
segment identity[1460288880679,0], missing=false, geometry1. The original
actor requested heading0.351071119/pitch0.4, completed at phase1, and cleaned
up with active=false. Actual completion orientation was approximately
(0.311739445,0.399101734,0.000016451). The original identity classifier and
completion writer were untouched. No target/identity/animation field was set.
This establishes an actual BS50-50 ->BSNosegrind physics identity change,
unlike the earlier Up-only and initial-biased negative controls.

The receipt ran ArcadeOFF and did not establish animation/HUD/combo acceptance.
Cleanup restored the feature, detached GDB, neutralized/closed the pad,
released the shared lock, and undid all own temporary objects. Final world
was bam generation8/revision52, fixture absent, ArcadeON. Hardware was returned
to Manual; subsequent coordination released it to Grab. These coordinates and
installed identity must be revalidated before another trial.

`arcade_grind_change.h` now provides the first production named executor:
existing Y->B recognition calls `InputSession::request_extension`, through
the facade and controller remapper, into the exact-owner native admission
lease and measured held RS sweep. XInput Y is+32767 (evdev Y is inverted).
Generic Y ownership is retired only when the named request starts. Final
face/RS masking and packet ownership include the named RS owner, so the
delivered sweep is not erased by the final controller filter. A cancels this
owner before the existing rail-exit Ollie path runs.

Qualification is deliberately narrow: independently established401/fine20,
geometry1, horizontal segment at least six metres, same owner and original
physics progress for80ms. The length is a tested-scope restriction, not a
claim about native minimum length or remaining forward travel. Native missing
segment and geometry remain authoritative. Other names/source identities have
no qualified recipe and cannot arm this executor. All nine combinations are
therefore **not implemented**. Repeated named changes are not ready yet.

The lease rechecks exact contact/segment/identity before initial native feature
admission, preserves its own406 transition, and cancels on A, OFF/menu,
owner loss, stale observation, incompatible geometry, bail/landing or real
rail exit. Native completion is separate from destination physics identity
and independently resolved CurrentGrindType. An unchanged animation cannot
be acknowledged. These evidence flags do not write score or combo state.
Simultaneous pairs now use existing table priority rather than numeric
button-scan order; distinct timestamps preserve ordered bindings. Same-time
Y+B selects the earlier Y->B table row, while sequential B->Y remains the
unsupported Nosebluntslide request.

Validation:2006 new strict C++20/ASAN/UBSAN production-composed checks cover
30/60/120Hz, held/released sequential and simultaneous input, exact original
timestamps, strict300ms boundaries, repeated polls, bounded RS execution,
independent identity/animation evidence and cancellation/A handoff. Also pass:
639 native lifetime/contact,1362 rail-pop,5645 input-session plus326 jump-idle,
2486 ordinary native flip,40 rail facade and10 rail composition checks.
The unchanged combo/HUD targets pass1736/7010 checks with their normal portable
build configuration. The initial strict combo compile rejected existing float
truth tests; the initial HUD command omitted ImGui. Corrected invocations
passed without changing either protected source.

`arcade_input_pattern_dispatch_tests` fails an unrelated contextual-air
press-age assertion on both this tree and a separate header snapshot of
HEAD c9c0581f; this is not claimed green and was not changed as grind work.
Local logs are under `build/grind-investigation/named-executor-checks`.
Windows runtime compilation, native animation/HUD/combo acceptance, live
Ollie/early-flip verification and eight remaining named recipes are outstanding.
No DLL/CI build, upload, installation, merge or release was performed.

### Follow-up: reject unchanged animation and narrow the next native probe

Review found a false-positive evidence route in cfc7a4d: requiring destination
physics23 plus CurrentGrindType23 after actor completion did not prove the
animation changed. If CurrentGrindType was already23 before the request, that
lingering value could satisfy the check. The executor now records a fresh,
owner-bracketed source401/physics20/animation20 observation before arming.
Missing, stale, mismatched or already-target source animation permits the
physical control investigation but cannot acknowledge animation change.
Only that recorded20 baseline followed by independently observed403/23 can
set animation_changed. The production remapper reuses one bracketed identity
sample per poll.2014 strict sanitizer composition checks pass, including
four regressions for the missing/unchanged baseline. No score/HUD writer changed.

Offline classifier inspection also narrows the next control probe. At
0x47dd155..0x47dd236 the original code projects the vector at source+320
off output+48, normalizes the remainder when its length exceeds0.01, takes
the absolute dot product against output+0, and sets tuple element2 using
strict greater-than0.9397000074 (constant RVA0x65e73ec). The degenerate
branch sets it to1. This is an alignment predicate, not a stick-angle or
named-action selector. Tuple element3 independently tests another vector's
dot product against-0.1 (RVA0x60ebf60).

The unchanged384-entry native lookup contains BSNosegrind23 at
[1,0,1,0,0,1] and[1,0,1,0,1,1], but BSCrooked32 only at[1,0,0,0,0,1].
The v4 positive control's first fine23 sample has candidate/accepted
[1,0,1,0,0,1], actual heading0.3117813 and pitch0.3987983. Increasing the
held heading sweep is therefore a source-derived experiment for crossing
the alignment predicate, not an established Crooked recipe. A candidate
Up then RX24000/RY-32767 after200ms should be evaluated against actual
native tuple/completion/contact/animation evidence before enabling B->B.
The source401 selector jump table at RVA0x481436c dispatches to0x4814187;
source403 dispatches to0x48142a5. Their separate55/125-degree and pitch
conditions must still be respected. No new recipe was enabled from this
static inference.

Grab's separate authorized installation/restart changed the live process.
After both peers released hardware, Grind ran only the existing bounded
read-only identity inspector. Receipt
`grind-readiness-be22b1a-v1/results/1793492143669900000.json` independently
verified sourcebe22b1a2f731485f8175c5fc8d8645598bb6a322,
DLL98aa97e1c4322b99023d286d0ccdce7bf3be87248e56d9195150147897e5b179,
the unchanged executable hash, gamePID73485/launcher73493, and matching
installed inode for both mapped DLL rows. Fresh native state is504/mode0.
Grind sent no input/console command and changed no telemetry, fixture or
world state. The short read-only window was explicitly released. Prior
Mega Park geometry, fixture history and controller route are not qualified
for this restarted process. No new DLL build or gameplay acceptance occurred.

### November 1 follow-up: fresh-world setup rejected before native sweep

The next bounded trial retained the source-derived RX24000 hypothesis above;
it did not enable another production recipe. After coordination with Manual
and Grab, separately reviewed immutable runners dismissed Steam idle artwork
with one released Shift key and closed the native Hub with one Escape. Fresh
native status established bam generation5/revision5, no fixture, off-board504
near (627.27,199.93,1062.48). This is a different physical lane from the prior
Mega Park receipt despite the same map key. No stored fixture coordinates were
reused. The installed source remained be22b1a and DLL98aa97e1..., gamePID73485.

Three guarded setup attempts produced **zero native grind samples**:

* `grind-native-city-be22b1a-v1/trial.py`, SHA-256
  `5a448f15da3c01fdc333e26ebdf59f1f18bbac55d7b0f70fefefd195532e6b51`,
  rejected the changed readiness position before controller input or edits.
  Receipt `results/alignment-sweep.json.gz` SHA-256
  `24cde6531ef94ef7e02a88786dc06726f04b5a2e18cc9aef063cb10ebb779197`.
* v2, SHA-256
  `4f2fbe6e0d4260b02e2374c020e2e890de6bf1aad12f3cdfc4a420527dbbcafe`,
  pinned the fresh position and mounted through the existing native shoulder
  chord. Riding100 at speed0.636792 failed the stopped-baseline gate before
  placement. Receipt SHA-256
  `e01746825bcdd20f668ace8e0ede56b52d663e10fccad3b1d1ed25616f1e0a8e`.
* v3, SHA-256
  `579b2024987548b015de6ef5e909627f4c809102e2bbb77dac4a0c37232a35d6`,
  added bounded native braking with Arcade OFF only if needed for setup,
  retained all placement/ownership guards, and passed11 inert setup/negative
  cases. The skater had already stopped, so neither mounting nor braking ran.
  Original `arcadefixture place` rejected the lane: "Fixture lane lacks a
  continuous flat native collision surface." Receipt SHA-256
  `6654cc35f95e7416ba84ac5f6364ec476036df7dc88cad7a003aba86ee060ce5`.

All paths are under the external reskate-arcade-lab directory; v2/v3 receipts
use the same relative results filename. Each wrapper completed, but its inner
error records setup rejection, not a successful trial. No debugger, grind
control sequence or rail transaction ran. Revision stayed5 throughout; fixture
absence was verified, not restored by an undo of a newly placed rail. Editor
was closed, temporary controller neutralized/closed, Arcade restored ON and
the route withdrawn. Final native state100, speed0.0018054, position
(626.163391,199.972519,1066.713867), heading342.811371, bam generation5/revision5,
menu closed and fixture absent. The lock was released and hardware explicitly
handed to Grab for its separately authorized restart/deployment. Manual was
informed. This chat installed nothing and has no pending hardware job.

Source inspection locates the immediate blocker precisely:
`arcade_grind_fixture.cpp::flat_start` requires a central native ground ray
and twelve lane rays at X offsets{-1.5,0,1.5}, Z offsets{-1,3,8,13}, within
0.08m height agreement. Native placement rejects before `edit_local_park`
when these fail. Another freshly verified flat lane is needed before testing
the alignment hypothesis; bypassing this guard would invalidate the trial.
`trainer ground` is an existing native fixture-navigation option, but its
`land_at` fallback can initially use hint height while collision streams in;
command success alone would not prove safe/stopped ground or a flat lane.
Any future use must await native riding/ground evidence and then pass the
unchanged fixture guard. It is not an Ollie or grind execution mechanism.

After Grab's window, revalidate installed identity, process, world, native
owner and current geometry before proposing the next bounded runner. Eight
named recipes and repeated changes remain unimplemented; the first qualified
recipe still lacks animation/HUD/combo acceptance and the local Ollie/early
flip candidate lacks live acceptance. No additional recipe, gameplay claim,
DLL/CI build, upload, merge or release resulted from these setup attempts.

### October 31 evening: second qualified native recipe, Crooked

Manual subsequently released its separately installed c048914 candidate after
returning to the custom skate3_megapark. Grab confirmed no pending hardware
job. Grind freshly verified c0489142ef8f1380606688d02d5e3c8f833909b5,
DLL7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
the unchanged executable hash, PID75680/launcher75688 and all three DLL
mapping rows against the installed inode. Status receipt:
`grind-readiness-c048914-v1/results/1793497707806347000-status.json`.
Fresh native status was504, speed0, bam generation8/revision8, fixture absent,
at (166.776093,0.949989,-134.904388). This observation, rather than the old
world's coordinates, admitted setup.

The immutable runner `grind-native-megapark-c048914-v1/trial.py`, SHA-256
`cda2154555ca30ea7d9deb500ca66b0cedca811d39604305a3079a5be3f5fca3`,
retained the earlier160-second budget, exact native owner, shared lock,
original placement/collision guards, scoped feature/lifetime interception,
independent physics reader and owned-edit undo. Only installed/world pins
changed from city v3. Eleven inert admission/transaction cases passed before
external staging. Native mount/stop, fresh short-rail geometry and temporary
long-rail replacement all succeeded. No installed DLL was changed.

Receipt `grind-native-megapark-c048914-v1/results/alignment-sweep.json.gz`,
SHA-256 `e9c74f826cd80449773333a40bfaa23f40fa0f9fe232b95422636afe2104971f`,
contains499 native samples and no runner/observer/cleanup error. Relative to
the observer start, native401/fine20/contact1 appeared at2055.03ms,
406 at2219.00ms, 403 at2581.86ms, and **403/fine32/contact1 at2639.51ms**.
Fine32 remained until native701 exit at3180.88ms. Both the original physics
delta and local cached step were0.0333333313 (30Hz).

The conditional input waiter independently found the established native
six-metre segment before the first Up sample. The held RX24000 sweep arrived
200.155ms later with Up retained; neutral followed at1000.082ms. The original
actor completed source401 ->target403 at phase1, with requested heading
0.437060118/pitch0.4, retarget heading0.631842375, actual final heading
0.388042450/pitch0.415385008. Original cleanup observed active0. The feature
observer ended at `native_transition_end`; it did not write named identity,
animation, position or score. Source/destination contacts retained opaque
segment[970662608935,0], identical endpoints
(166.516602,0.49,-121.988281)/(166.516602,0.49,-127.988281), missing0,
geometry1. The classifier accepted[1,0,0,0,0,1] and published32: **actual
BS50-50 ->BSCrooked native physics**, consistent with the alignment predicate
traced above. The uinput receipt alone remains explicitly UNVERIFIED; the
separate native observations establish this narrower result.

`arcade_grind_change.h` now dispatches the existing B->B binding through this
measured recipe. It shares the already qualified401/fine20/geometry1/segment
and owner gates with Y->B, retains Up, and selects24000 versus12000 for the
200ms heading sweep. Verification requires the requested destination32 or23;
another real native name cannot satisfy the request. Source animation must
still be independently known20 before either target can acknowledge an
animation change. All cancellation, native completion, HUD and scoring
ownership remain as before. No speculative recipe was enabled.

The composed test now covers both recipes at30/60/120Hz: released B then B,
sequential/simultaneous Y+B, original edge timestamps, repeat suppression,
bounded control and rejection of the other destination identity.3007 checks
pass strict C++20 warnings and address/undefined-behavior sanitizers. These
are offline checks, not native animation/HUD/combo acceptance. The two native
recipes remain qualified only from source401/fine20; seven bindings and
repeated named changes are not implemented. Live Ollie/early-flip acceptance
and complete platform checks remain outstanding. No new DLL/CI build started.

All own native fixture transactions were undone (generation8/revision20,
fixture metadata configured but object absent), GDB detached with return0,
controls neutralized/destroyed, editor/menu closed and ArcadeON with route
withdrawn. Final native100, speed0.584957, position(135.266632,1.031418,-41.038578),
heading337.878876. Hardware was explicitly released to both peers; Manual
then reclaimed it for its priority tests. Grind has no pending hardware job.
The final position differs from the trial lane: future work must revalidate
owner/world/geometry and may not treat this as a stopped fixture approach.

### Follow-up: trace the longitudinal classifier before the next control trial

While Manual retained exclusive Deck control, offline disassembly resolved
the producer of classifier tuple element1. `0x47dd0f3..0x47dd0f6` copies it
from classifier object+32. The preceding call to `0x47dfa80` writes that
field at `0x47dffa2`: for coarse classes1..3, compare output byte+402 with
the predicate `dot(source+96,source+32)>0`; element1 is1 when they differ,
otherwise0. Other coarse classes force0. The source page is `*pages` and
the output page is `*(pages+32)`, as in the existing independently bracketed
reader. This is a native geometry/orientation comparison, not a name request
or a direct mapping from the stick's vertical sign.

The native lookup distinguishes BSNosegrind23 from BS5-0 24 by this element:
23 admits[1,0,1,0,0/1,1], while24 admits[1,1,1,0,0/1,1]. Existing positive
Nosegrind and Crooked receipts both show output+402 becoming1, with accepted
element1 remaining0. Those receipts do not record the two source vectors,
so they cannot independently establish the comparison's operands.

A local, **unexecuted and unstaged** next-probe candidate now records both
source vectors and output+402 with repeat-read race checks, alongside the
existing owner/contact/tuple/actor evidence. Its proposed control is held
Down followed200ms later by RX12000 with Down retained, neutral at1000ms.
This is a hypothesis for reversing the longitudinal predicate, not a qualified
5-0 implementation. Acceptance must observe original actor completion,
actual fine24, the intended classifier tuple and unchanged segment/contact;
an exit, another name or an unchanged identity is a negative result.

Candidate preparation is `build/grind-investigation/prepare_tail_sweep.py`;
generated candidate SHA-256
`498bf81d56964a769371c3e8b462c82194ee6a0fd4f5844d2415bb432aa0f5e7`.
It validates the earlier Crooked receipt before deriving exact owned-fixture
metadata and permits only the corresponding native redo at generation8/
revision20. Fresh stopped native100, unchanged process/world/metadata,
native reset collision validation and a new completed geometry measurement
still gate the trial. No stale pose is used as a geometry substitute. Peer
hardware handback and fresh installed identity must precede external review,
staging or execution; any changed pin invalidates this candidate.
No Deck call, new production recipe or DLL/CI build occurred in this follow-up.

### Subsequent runtime handback: readiness only, no tail-sweep trial

Grab's separately authorized diagnostic installation/restart invalidated the
prepared c048914 probe. After both peers released hardware, fixed external
identity/status runners independently verified installed and mapped
dbaa45a1ad63103a2926c84a50bcb11c898a6c38,
DLL4a54bf245ce4fe00b58602c98b9f0770382d7c09b68e561608f9a6bfe66a9356,
PID78791/launcher78799 and three matching DLL mapping rows. Receipt
`grind-readiness-dbaa45a-v1/results/1793500694596826000-status.json` shows
the city spawn near(626.651367,199.972504,1065.314697), native100, speed below
0.002, bam generation5/revision5, fixture unconfigured/absent. This is not the
previous Mega Park fixture world, so the old native redo was not attempted.

The source `Extension/World/console_commands.cpp::register_world_commands`
provides native `levels`/`load` routing. A bounded `levels` query confirmed the
installed destination `skate3_megapark` with asset
`levels/game/dingolevel_reskate_skate3_megapark/dingolevel_reskate_skate3_megapark`.
Receipt `grind-readiness-dbaa45a-v1/results/1793500740408231000-levels.json`.
No world load was issued: Manual's corrected air batch became ready, and Grind
released at this clean boundary for its priority restoration/testing. Only
status/levels commands ran, all jobs finished and lock/console closed. No
controller, debugger, capture, fixture or installed-file mutation occurred.
Last native100 speed0.001568, ArcadeON, editor/menu closed, same generation/
revision and absent fixture. Both peers received the explicit handback.
The 5-0 probe remains unexecuted and must be repinned after the next handback;
two production recipes remain qualified, with no new DLL/CI build.

### Third qualified recipe: native BS5-0 from BS50-50

After both peers released hardware, fresh identity checks verified Manual's
restored c0489142ef8f1380606688d02d5e3c8f833909b5, unchanged DLL
7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
PID79762/launcher79771 and three matching DLL mapping rows. The first fixed
tail runner stopped before fixture/controller-transition setup because the
native mount attempt remained504. No recipe was qualified by that attempt.
Receipt `grind-native-tail-c048914-return1-v1/results/tail-sweep.json.gz`,
SHA-256 bc27c203ee277ed824d30fe73ba75a356e6a37dfa0b88fd3d0eaeb630e1e71a8.

Source inspection of native world `load` routing and the installed `levels`
list justified an ordinary reload of the same custom skate3_megapark. The
bounded reload receipt is
`grind-readiness-c048914-return1-v1/results/1793501910168445000-reload.json`.
Console text said "Load request rejected", but independent observations
showed generation8 becoming11 and the new settled spawn pose
(142.998047,0.949993,-88.574219). Only those observations, followed by a fresh
successful native mount and unchanged collision/geometry gates, admitted the
next trial. No teleport or synthetic recovery write was used.

Immutable `grind-native-tail-c048914-return1-v2/trial.py`, SHA-256
311f02e1f19c6384b0c8ec74db508921af9ed5bec7d92f37224aa62a97dd7dec,
used the existing shared lock,160-second watchdog, owner/image checks,
temporary native fixture transactions and bounded original406 observation.
Eleven inert setup/negative checks passed before external staging. Receipt
`grind-native-tail-c048914-return1-v2/results/tail-sweep.json.gz`, SHA-256
b1a17730521110018a47da282882a4a8c3635baa1576e2c79c88feebb082d8f6,
has499 samples and no runner, observer or cleanup error. Native401/fine20/
contact1 appeared at2040.679ms,406 at2233.664ms,403 at2542.190ms, and
**403/fine24/contact1 at2577.806ms**. Fine24 persisted in61 samples until
native701/contact0 at3300.044ms. Both original physics dt and cached step
were0.0333333313, proving30Hz for this receipt only.

The conditional input waiter observed established401 contact before Down.
Evdev RS(0,+32767) arrived at2105.529ms, RS(+12000,+32767) at2305.599ms,
and neutral at3105.602ms: a200.070ms held heading sweep and1000.073ms lease.
The native actor completed401->403 at phase1 with requested heading
-0.351061344/pitch-0.4 and final orientation heading-0.305821657/
pitch-0.355577469; original cleanup cleared active0. Source and destination
retained segment[970662608935,0], geometry1, missing0 and endpoints
(142.839844,0.49,-75.779297)/(142.839844,0.49,-81.779297). All61 destination
samples retained that segment. The independent classifier published24 with
accepted tuple[1,1,1,0,1,1]. Its bracketed source vectors were equal with
dot approximately1.000000015 and contact byte402=0, matching the previously
traced longitudinal comparison. This confirms BS50-50->BS5-0 physics, not
merely delivery of the proposed stick values.

Production `GrindChange` now admits the existing ordered X->B target24
through this measured Down recipe (XInput Y=-32767), retaining the existing
401/fine20 source, owner, six-metre geometry, establishment and cancellation
gates. No target identity, animation, HUD or score is written. Y->B and B->B
retain their measured Up recipes. Simultaneous X+B remains the existing
table's B->X Bluntslide priority; it is not silently remapped to5-0. That
binding remains unimplemented pending its native recipe.

4029 strict C++20 address/undefined-behavior sanitizer checks pass for all
three recipes at30/60/120Hz, covering
original physical timestamps, held/repeated polls, unrelated destination
rejection, independent animation evidence, Down cancellation and the strict
300ms released-edge boundary. Native animation/HUD/combo acceptance and
Ollie/early-flip live acceptance remain outstanding. Six named bindings,
additional source contexts and repeated named changes are still missing.
The baseline contextual-air press-age failure remains separate; full checks
are not green. No new DLL/CI build, upload, merge, release or install occurred.

All owned fixture edits were undone to generation11/revision23, metadata
configured but object absent. GDB returned0/detached, native gate restored,
controls neutralized/destroyed, route withdrawn, editor/menu closed, ArcadeON.
Final native100 speed0.000124 at(145.004608,1.028666,-63.076706), heading
10.387815. Both peers received explicit hardware release and acknowledged;
Grind has no pending hardware job. Revalidate identity/world/geometry before
any later input rather than treating this pose as a new fixture approach.

### Offline continuation while Manuals holds priority hardware

Manuals explicitly reclaimed the Deck after the preceding clean release to
complete its user-prioritized work. Grind confirmed no active/pending job or
reservation and will make no input, console or telemetry call before Manual's
clean handback. This continuation used only local source, retained executable
bytes and inert reader fixtures. No native recipe or build was added.

The hash-verified original executable separates the remaining control problem
more precisely. The six dispatch entries at RVA0x481436c map source states
400..405 to 0x481432e,0x4814187,0x48141d3,0x48142a5,0x48141d3,0x4814333.
Thus an initial401 request selects only400 or403; reaching402/404 from that
source requires another native transition. For source403, the existing
target-dependent heading band selects400 when absolute pitch is at most
0.1745329201, or402/404 through additional native side/contact predicates
when pitch is larger. Outside that heading band, its near-neutral pitch and
heading branch can return401. These are destination-class conditions, not
validated named recipes. The production source401/fine20 gate must not be
widened just because those branches exist.

The native lookup also rules out treating the measured 5-0 heading sweep as
a Smith recipe. BS5-0 24 accepts[1,1,1,0,0/1,1]; BSSmith26 requires
[1,1,0/1,1,1,1]. At0x47dd103..151, tuple element3 compares the dot product
of signed source+96 and output+48 against -0.10000000149. The source vector
is negated only when tuple element1 is zero; element3 is1 for a finite dot
at or below the threshold. Changing alignment element2 alone cannot turn
the measured5-0 intoSmith. Earlier receipts record neither output+48 nor
source+320/output+0, so they cannot explain/control this additional predicate.

Tuple element4 is returned by0x47c8290 and stored at classifier+36. It is not
a direct left/right stick bit: that routine combines vectors from source+0,
the page at pages+40 offsets256/272, output+0, classifier history and its
selected history entry+96. For coarse0/1, it also uses classifier+240 and
stores an intermediate flag at+256. For larger coarse classes, the history
flag and a separate page+16 offset260 threshold120 select the0/1 or2/3
return range. Retain this distinction before interpreting the Noseslide21,
Tailslide22, Noseblunt30 and Blunt31 table entries; those depend on coarse
class2/3 and element4 as well as longitudinal identity.

Prepared only a local read-only observer fragment that adds source+320,
output+0 and output+48 to the successful tail observer's existing repeated
read bracket. It does not derive a name, alter an input sequence or contain
a runnable hardware entry point. Preparation script
`build/grind-investigation/prepare_classifier_vectors.py` verifies the exact
successful tail-runner SHA before extracting the function. Result fragment
SHA-2567a625bfb32e08185dd50d08bae176919561b60deafcf454c14ffdc2e3d73545b.
Seven inert checks cover stable data, each newly/previously read vector
changing during the bracket, and owner loss. These checks establish reader
rejection behavior only; repeated equal reads still are not an atomic frame.
Any actual probe must first be repinned, independently reviewed and staged
after hardware handback. Six bindings and repeated changes remain unresolved.

### Repeated-transition admission is separate from destination selection

Manual's completed handback records no pending job, neutral/destroyed pad,
stopped capture, automatic routing and native100/mode1 at30Hz in Mega Park.
Grab then explicitly claimed the next window for its separately authorized
5387765 deployment and early-B/control batch. Grind acknowledged and remains
offline; the c048/PID79762 pins must be treated as stale after that window.
No hardware or console call was issued by Grind in this continuation.

Offline tracing identifies an additional gate needed before a second native
transition can be qualified. `0x48189a0` publishes actor+766 to output+419 at
`0x4818b7d..89`, which is the byte consumed by selector0x4746896. It also
publishes actor+112 to output+48, the reference vector used by the Smith
classifier predicate identified above. Neither output field is an input
command. The common orientation helper0x4815790 writes actor+766 only when
the actor's virtual+248 predicate returns false and native state elapsed
context+6200 exceeds0.0833333358. Destination selection0x48140e0 alone is
therefore insufficient to prove the game will admit the next request.

The pinned vtable slot+248 resolves to0x481d8f0 for401,0x481d910 for403,
0x481da50 for400,0x481da30 for402/404, and the constant-true0x3b8830 for406.
The403 predicate additionally compares actor+767 with the sign of the current
context pitch+4048. Its near-straight branch uses requested pitch magnitude
at most0.15 and heading boundaries20/160 and21/159 degrees before returning
false. Its other branch uses the native tuning value at config+216 and the
35-degree constant0x6439c34. Thus simply replaying the first401 recipe or
releasing the stick is not yet a verified return-to50-50 control.

Common initialization0x4817770 captures actor+767 from bit2 of context+4068
at0x481780c..821. This is an entry-time reference bit, not the requested
grind identity; its higher-level meaning is not established here. A future
two-transition probe must record that bit, actor+766/output+419, native
elapsed time, current pitch, requested orientation, actual completion and
the independently named destination for each leg. It must bound both legs
and stop on loss of owner/contact/segment, rather than keeping the existing
single-transition experimental feature lease enabled indefinitely. No new
recipe, state write, acknowledgment or production generalization was made
from these static conditions.

### 5387765 window: opposite tail sweep produces Salad, not Smith

Grab released its separately installed5387765b070a08acf4ccaea600867664abd8abd5
at21:28 Pacific. Manual's completed handback also confirmed no pending job or
reservation. Grind coordinated its window and independently verified DLL
0e0b256ca26cc60b0694765bbad4799b80034d4d6ea4824dae66b0e3f03cd8ad,
unchanged original executable, PID83858/launcher83866 and three matching
installed DLL inode/device mappings. The fresh status receipt
`grind-readiness-5387765-v1/results/1793507350178541000-status.json`
showed native100, stopped, city-world bam generation5/revision5, no fixture.
An ordinary native load of the installed Mega Park then changed generation
to8/revision8 and produced a settled504 spawn at(142.998047,0.949993,-88.574219).
Receipt `grind-readiness-5387765-v1/results/1793507425415698000-reload.json`.
As previously, the console's "rejected" text was not used as acceptance:
fresh world/pose, then native mount and unchanged collision checks admitted
the actual trial. Grind installed nothing.

The reviewed immutable runner
`grind-native-signed-tail-5387765-v1/trial.py`, SHA-256
ad7e6ef84baeb5b0724dcf9384c15e87ce39a635debe287a4d71689e685cdb0b,
kept the existing160-second watchdog,95-second forward edit budget, shared
lock, exact process/owner gates, original406 feature/lifetime interception,
single transition boundary, physical contact waiter and owned transaction
undo. The control hypothesis was Down then RX-24000 after200ms with Down
retained, neutral after1000ms. The new read-only vector fragment was included;
11 inert setup cases and7 vector-bracket cases passed before staging. A
2MiB durable result slot was reserved before input.

Receipt `grind-native-signed-tail-5387765-v1/results/signed-tail-sweep.json.gz`,
SHA-256879f876df64f94124d8a07e148ff73c5b947c7ac933001e20a8b9759975afd1c,
contains505 samples and no runner/observer/cleanup error. Relative to the
observer start,401/fine20/contact1 appeared at2023.450ms,406 at2256.990ms,
403 at2598.840ms and **403/fine25/contact1 at2648.894ms**. The original enum
asset `native-identity/ebx-1767.txt` explicitly names25 **BS_Salad**; Smith is26
and Feeble is27. Neither a generic diagonal label nor a related grind name
can satisfy the Smith request. No production recipe was added.

The original actor completed401->403 at phase1, requested heading0.437060118/
pitch-0.4, retarget heading0.632153273, actual final heading0.388094664/
pitch-0.399101496. Original cleanup cleared active0. All53 fine25 destination
samples retained contact, geometry1, missing0 and the same six-metre segment
[970662608935,0], endpoints(142.8515625,0.49,-75.771484375) and
(142.8515625,0.49,-81.771484375). Native701 exit appeared at3288.677ms.
Original physics dt and cached step were both0.0333333313 (30Hz).
Down arrived2165.474ms, the heading sweep2365.568ms, neutral3165.528ms.

Independent bracketed vectors explain the failed Smith predicate: in all53
destination samples output+48 remained(0,1,0), longitudinal byte402 was0 and
tuple[1,1,0,0,0,1] published25. The source+96 Y component stayed between
0.387360 and0.389244, so the signed dot was positive, far from the <=-0.1
needed for Smith's element3. Reversing the heading did not change that
predicate. This rejects that specific Smith hypothesis while confirming a
real different native action; it is not a failed input-delivery experiment.

Further source tracing ties the coarse1 publisher0x48188b0 to the shared
context flags:0x48188dc..ec copies bit2 of context+4068 into output+402 on
each publication. Common entry captures the same bit in actor+767. A future
trial must observe the original producer of this bit and its relationship
to the board's physical pitch/contact before proposing a Smith correction.
Directly changing the bit, classifier tuple or identity would bypass the
native mechanism and is not an implementation option.

Cleanup returned GDB0/detached, restored the native gate and original fixture
absence, neutralized/destroyed the pad, withdrew the route with OFF/ON and
left ArcadeON/menu/editor closed. Final bam generation8/revision20, fixture
metadata configured but object absent; native100 speed0.000222 at
(137.587067,1.028670,-51.380939), heading347.405548. Both peers received the
explicit clean handback. No pending Grind hardware job remains. Production
still has three qualified narrow recipes; six requested bindings and repeated
changes remain unresolved, with no new DLL/CI build or gameplay acceptance.

### Contact-bit producer traced; repeated-transition observation prepared

Continued offline against the same independently rehashed executable. The
missing writer of context+4068 is a substructure copy, which explains why
searching for direct stores at displacement4068 did not find it. At
0x4801e72..8c the native contact updater copies its result at updater+96 into
context+3024 through0x47e8120. That copy preserves the individual bits of
substructure+1044; bit2 is copied at0x47e8466..73. Thus3024+1044=4068.
This is physical contact-selection output, not a named-action command.

The original403 contact candidate routine0x47ed610 supplies the bit. Selecting
the candidate with point+80 and segment index+96 clears bit2 at0x47edbcc;
selecting point+32/index+48 sets it at0x47edc40. Both branches copy the actual
segment and publish contact at substructure+992. The402 candidate routine
0x47f0180 uses the same distinction at0x47f022b/0x47f0285. The contact query
0x47e1950 constructs these opposing probes from the actual context transform:
position+112, longitudinal axis+96, lateral axis+64 and up axis+80. Probe
dimensions include runtime values0x77a2370/0x77a25e0; their current values
cannot be recovered from the file's unbacked data section. No static default
or new geometry was assumed for them.

The403 candidate selection also checks physical separation, segment heading,
native tuning and direction of travel (dot(context+976,context+96)), then
chooses between eligible contacts. This explains why a heading input alone
does not guarantee the longitudinal bit or Smith's signed-axis predicate.
The exact remaining Smith blocker is a native control trajectory that keeps
the required physical contact while achieving the negative signed dot and
correct side predicate. The producer is now identified; a successful Smith
recipe still is not established.

Prepared a read-only repeated-admission observer fragment locally at
`build/grind-investigation/admission_observer_fragment.py`, SHA-256
13d497290855d6b85429e25054ab165d47c6291758ff28a692ca9ffffc7f482c.
It brackets owner, actor/vtable, selector/output, state, segment identity,
contact, context flags/orientation/elapsed time, actor+766/+767 and the
published output+419 admission byte. Twenty-seven inert memory cases passed,
including owner loss, invalid actors and changed fields/pointers during the
bracket. This is observation preparation only, not a production recipe or
native acceptance. The fragment was not staged or run against the Deck.

The next bounded native observation can retain the measured5-0 entry, release
the stick earlier after verified403/fine24, and measure whether original403
admission becomes eligible while the same contact/segment remains. That
observation must precede extending the single-transition gate to a second
leg; a high admission byte alone will still not prove an executed50-50.
Grab reported no reservation or changes since handback. Manual is reclaiming
priority hardware, so Grind explicitly released any tentative window and
issued no hardware, console or telemetry operation. Latest installed/world
pins above are retained evidence, not fresh observations. The three production
recipes and the DLL/CI build hold remain unchanged.

### Existing receipt establishes a narrow native re-admission window

Re-analysis of the immutable 5-0 receipt above found that it already contains
the published admission byte: contact[19] is output+419. After the delivered
neutral event, eleven samples retained403/fine24, physical contact and the
same six-metre segment while output+419 was1. The first was45.980ms after
neutral, the last161.644ms. Requested pitch had returned to0, while actual
pitch remained approximately-0.397; requested heading was-0.30582 radians.
Both native physics and cached step were0.0333333313. This is evidence of
original403 re-admission after neutral in this specific 5-0 context. It is
not evidence of another executed transition: the scoped gate had ended with
`native_transition_end`, and no second406 occurred.

The signed-tail Salad25 receipt provides a useful counterexample. Its final
contact sample100.273ms after neutral had requested pitch0 and requested
heading+0.388114, but output+419 remained0; there were no post-neutral admitted
contact samples. Thus neutral cannot be generalized to re-admission for every
403 grind. These headings are consistent with the previously traced20-degree
boundary; the samples alone do not establish which predicate branch ran.

The hash-checking offline re-analysis is saved in ignored
`build/grind-investigation/summarize_repeated_admission.py`, with compact
results in `repeated-admission-summary.json`. It reads existing receipts only.
The next hardware test can therefore target a bounded second native execution
leg after the measured5-0, rather than spending a separate trial rediscovering
the already recorded admission window. Require independently observed first
completion and fine24/contact/segment before neutral and second admission;
require a second original406 completion and actual401/fine20 with retained
contact to qualify the return. A high admission byte remains insufficient.

Tracing the admission call also found a gap in the prepared observer:
actor+72 is the smoothed output, not the request examined by virtual+248.
At0x482549f..0x4825525, the common update reads GrindInputs at context+5248,
with heading+24, pitch+60 and roll+52, wraps heading, and passes those values
to0x4815790. That helper passes the unsmoothed request to virtual+248, before
the caller writes the smoothed result to actor+72 at0x4825537. The update is
conditional on context+5084 bit30, so merely reading inputs does not prove
this branch executed in the sampled frame.

The read-only observer now brackets that input pointer and the three raw
request fields separately, records the update flag, and rejects invalid
selector/page/output/input pointers before dereferencing them. Its new SHA-256
is0f2395abcb4d44c909a83cd505d22b30c23d1e98774ef7ca3c59311349253a8b;
48 inert cases pass. It remains unstaged and unrun on hardware. Manual's
current task is active in its diagnostic capture work and has not handed back
the Deck; Grind issued no hardware calls. No production recipe, new DLL or
CI run was added, and the requested feature milestone is still incomplete.

### Two original transition legs execute; six-metre rail ends before return

After Manual's explicit clean handback and Grab's no-conflict response, a
fresh read-only inspection verified installed/mappedc0489142ef8f1380606688d02d5e3c8f833909b5,
DLL7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
game85401/launcher85410 and the unchanged pinned executable. The first
fixture-status attempt lacked a console echo; a fresh capture identified Hub.
One reviewed Escape/grave sequence visibly opened the console, after which
two status queries independently established Mega Park bam generation20/
revision20, no fixture metadata/object, stopped504 at(142.998047,.949993,-88.574219).
The console key remained grave. No user intervention or installation occurred.

The reviewed immutable runner `grind-native-two-leg-c048914-return2-v1/trial.py`
(SHA-2564036b153202d97ee5e59454ef1d599f645b8c0e045b6cfc020800b3892ae1928)
extended the original scoped gate to at most two native406 lifetimes. It
requires first original completion401->403 and fine24/contact/same segment
before a conditional neutral event. The second original completion must be
403->401 at phase1; only actual401/fine20 with contact can qualify a return.
No target, orientation, identity or position is written. Twenty inert lease/
geometry, ten conditional-neutral, eleven fixture-transaction and48 read-only
observer cases passed before staging. A2MiB result slot was reserved.

Receipt `grind-native-two-leg-c048914-return2-v1/results/two-leg.json.gz`,
SHA-256c59a084b716c50f22be8989ab5c7720d83a6af767ee0755eb7936a00c391593d,
has486 samples, both clocks30Hz, and no runner/observer/cleanup error. Native
401/fine20 appeared2056.385ms, first4062251.906ms,4032582.513ms, fine24
2622.084ms, and a genuine **second4062723.599ms**. Both original completion
callbacks ran at phase1, with sources/targets401->403 and403->401. The second
completion's native elapsed time was0.933333 seconds; its configured blend
included0.5 seconds, versus the first leg's faster native curve.

This did **not** produce an accepted return to50-50. The six-metre segment
[1168231104551,0] ended first. At second entry its native contact point was
(142.866211,.49,-78.335602), forward velocity4.674191; endpointZ was-75.764648.
At3285.606ms the original actor reported missing_segment1 and physical
contact0, while still406. Native completion then cleaned up into701 at
3652.839ms, not401/fine20. The gate ended with `native_exit`. Its cleanup
detached GDB0, restored the feature, neutralized/destroyed the pad, withdrew
the route and undid all fixture transactions. This resolves second-leg
execution/admission, but not retained-contact destination qualification.

One longer-geometry retry was prepared using the existing native editor's
uniform scale2 body/transform setters and a required independently observed
12m segment. Its fixture setup stopped before geometry, debugger or grind
input: same-level reload retained fixture metadata. Restoring the absent
short rail succeeded, but `arcadefixture relocate` returned `No changes.`
and advanced revision37->38 rather than+2. Source explains the discrepancy:
`placement_same_pose` accepts positions within1cm; editor `begin` increments
revision once without a history entry on that no-op; the fixture updates its
metadata to the slightly different new start and then uses exact position
equality to find the object. Thus status can say absent although the restored
rail still exists. This is test-setup evidence, not a grind regression or a
reason to relax transaction ownership indiscriminately.

The aborted retry receipt is
`grind-native-two-leg-c048914-return2-v2/results/two-leg-long.json.gz`,
SHA-256f56fed6b49dc4fafe5ac4934e5fba8650d91456c160086091385cb7341927469.
A separately reviewed exact-owned undo, gated on freshc048/PID85401/gen23/
rev38, removed that sole placement and verified rev40. No extra history entry
was invented for the no-op. Cleanup receipt is
`grind-readiness-c048914-return2-v1/results/1793518177228662000-status.json`.
Both peers received clean handback: ArcadeON, automatic routing, stopped100
speed0.000046 at(142.868713,1.028669,-88.762909), bam23/rev40, fixture metadata
configured but actual object absent, menus/editor closed, no pending job.

Next repair the external setup ledger for the *specific observed* relocate
no-op (+1 plus exact `No changes.` echo, no undo count). If its updated metadata
does not match the restored object exactly, undo only that owned placement
and recreate it at the freshly collision-checked metadata position. Then
require fresh native12m geometry and repeat the two-leg test. Do not replay
either consumed runner or assume these world pins survive another peer's work.
No fourth production recipe, full gameplay acceptance, new DLL or CI build
is claimed from the completed second actor or the aborted geometry retry.

### Exact no-op repair succeeds; direct create does not apply native scale

Both peers released hardware. Fresh identity and two console status reads
confirmed the samec048914/PID85401/launcher85410/DLL7853ae1a identity,
Mega Park bam23/rev40, stopped100 and absent fixture. The first status attempt
again had no echo; capture showed Hub. The reviewed Escape/grave recovery
visibly opened console and the open-console status variant recovered without
user intervention. Treat console visibility as current evidence, not a durable
assumption across idle periods.

The immutable v3 runner
`grind-native-two-leg-c048914-return2-v3/trial.py`, SHA-256
a6a4f8248ccc3830a6333b33da7404d958dcc822e0508c38669400e522e6836e,
repaired the external setup ledger. Nineteen inert tests cover exact no-op
echo/revision, mismatching metadata, owned undo/recreate and rejected receipts.
Native restore40->42, relocate `No changes.`42->43, owned undo43->45 and
recreation45->47 succeeded. The new rail exactly matched the freshly
collision-checked fixture metadata. No history entry was invented for the
no-op, and no production fixture code was changed.

The subsequent direct scale2 long-rail create failed the strict receipt:
delete47->49 succeeded, but create49 advanced to52 rather than51. The runner
stopped before debugger or grind input, with zero native grind samples; it
correctly refused to guess an undo after an unexplained extra revision.
Receipt `grind-native-two-leg-c048914-return2-v3/results/two-leg-long.json.gz`
has SHA-256
2d71d7ab2450aae62f8c137c552041933a42f8d0e9890d826fe01e8c61af2115.
No twelve-metre geometry was observed and no destination was qualified.

Source inspection identifies the setup error. These files are identical at
installedc048914 and this branch. `PlacementCreateRecipe` in
`placement_create_queue.h` serializes item, position and rotation, but never
scale. `park_editor_runtime.cpp::begin` adds only a create step for a new
object; the ordinary completed create commits without an additional move.
`update_placement_poses` later reads actual matrix scale and calls
`park_editor_external_change` on a discrepancy, incrementing revision and
clearing undo/redo. That source path explains the extra revision; the trial
did not capture a native matrix scale read, so scale1 is a source-derived
explanation rather than an independently measured geometry result.

The existing native **move** path does invoke both the body setter and
transform setter, then requires `object_has_pose` on the following tick.
The next external setup candidate therefore creates at unit scale and only
then submits a separate move to scale2 for its exact owned ID. It must still
require original twelve-metre segment/contact readback; a scale command or
matrix alone does not establish that the grind segment length changed.
`prepare_two_leg_scaled_move.py` produced the unstaged v4 draft SHA-256
7d8f248e91474d3f33845dbb416172f15c1cda3be017f1b98d49a263793bedd5.
It has not received fresh setup tests or hardware review; re-pin its world
before staging. The v1/v2/v3 trial result slots are all consumed.

Cleanup used a separately reviewed immutable exact-ID deletion, not undo.
Read-only SQLite inspection identified only our new long railID6 at
(142.86871337890625,-.4900070130825043,-75.76290130615234), distinct from
existing short railID2 at the unrelated older coordinates. The cleanup
rechecked installed identity, bam23/rev52 and the exact ID/item/pose, deleted
ID6 through the original native editor, verified its persisted absence and
revision54, and closed the editor. Receipt:
`grind-owned-scaled-cleanup-c048914-v1/results/1793522450136082000-status.json`.
Its initial invocation had stopped at missing console echo before deletion;
fresh capture/recovery preceded the successful invocation. StockID2 was
untouched. Both peers received clean handback: no job/reservation, ArcadeON,
automatic routing, pad destroyed, no debugger attached, menus/editorclosed,
state100 speed.000284 at(142.982758,1.028669,-88.561417), bam23/rev54,
fixture metadata configured but actual object absent. Revalidate all pins
before future input. Production remains three narrowly qualified recipes;
no new DLL, CI build, repeated-change acceptance or fourth recipe is claimed.

### Native twelve-metre rail verifies the return; Y→Y executor added

After both peers released the Deck, the existing immutable identity probe
freshly confirmed c048914/PID85401/launcher85410 and the same installed DLL
and executable hashes. Capture showed Steam idle artwork; a separately
reviewed one-Shift wake restored visible Skate gameplay. Two status reads
then confirmed bam23/rev54, stopped100 and absent fixture. No installation
or world reload was needed.

The v4 setup passed19 exact-no-op/ownership tests and13 unit-create/owned-ID
lookup/native-geometry requirements. Its immutable runner SHA-256 is
7d8f248e91474d3f33845dbb416172f15c1cda3be017f1b98d49a263793bedd5.
Unit-scale creation followed by the original editor move to scale2 succeeded;
the actual contact actor independently reported a twelve-metre segment:
endpoints(142.982422,.500742,-69.560547) and
(142.982422,.500742,-81.560547), identity[1511828488231,0].
The move command itself was not used as geometry acceptance.

Receipt `grind-native-two-leg-c048914-return2-v4/results/two-leg-long.json.gz`
has SHA-256
c7bd573dbba106902c6cc9cc3fad20348f612a22c17907c1278190088df448a0.
It contains505 read-only samples, both native/cached clock intervals
.03333333135 seconds, no runner/observer/cleanup errors and no dropped
controller writes. Relative to the observer, native401/fine20/contact began
2056.548ms; first4062261.347ms;4032584.567ms; fine242620.306ms.
Conditional neutral was sent2677.604ms, only after first original completion
and independently observed fine24/contact on the original segment. Second406
began2722.307ms, original completion403→401 reached phase1 after.933333s,
native401 appeared3650.757ms and independently published fine203687.113ms.

The gate ended `second_native_identity_verified` with exactly two original
completions and restored its feature. There are74 reader samples of returned
401/fine20 from3687.113 through4569.901ms, all with physical/published
contact1, missing-segment0 and the original segment. These are repeated
reader samples, not74 distinct physics updates. This is actual native
50-50→5-0→50-50 execution with retained destination contact. A transient
physical-contact0 sample during the first406 remains part of the native lift;
it is not hidden or replaced with synthetic ground evidence.

Later, after gate retirement, ordinary physics selected403/fine24 and then
402/fine31 on a different short end segment before701. Those incidental
edge events do not qualify another named recipe or a requested Bluntslide.
This Arcade-OFF control experiment establishes physics execution only;
animation, Arcade HUD/balance and combo continuity still lack live acceptance.

Production `GrindChange` now admits the existing Y→Y binding from the narrow
source403/fine24 with horizontal geometry1 at least12m long. It submits a
fresh existing native lease and owns a neutral RS sample; original native
admission/initialization selects401. It neither assigns the native target nor
writes an identity. Return verification requires the new request's original
403→401 phase1 completion, actual401/fine20 and the same segment/output,
plus independently changed native animation before setting animation evidence.
The slower return owns neutral up to the existing1500ms lease, closing after
verified completion/animation at or after1000ms. The three source401 sweep
recipes keep their original1000ms control duration and200ms heading sweep.

Source-specific exclusion now prevents unchanged5-0 input on403/fine24;
other targets from that source remain unsupported. A returned401/fine20 can
re-establish and dispatch the existing named sweep with fresh physical edges.
This makes that limited repeated route executable in production, not a
claim that arbitrary named sequences have been qualified. The original native
hook retires each lease after its own406 completion; no persistent setting or
new multi-leg hook was introduced. Flat Ollie, flips, manuals and HUD code
were untouched.4536 strict production-composed address/undefined-behavior
sanitizer checks pass at30/60/120Hz, including both directions, original event
timestamps, held/polled deduplication, source/geometry rejection, wrong-leg
completion, missing segment, unchanged animation, rearming and cancellation.
This does not supersede the known baseline input_pattern_dispatch failure.

Cleanup detached GDB0, restored the native feature, neutralized/destroyed the
pad, restored ArcadeON/automatic routing and undid every owned transaction.
Both peers received clean handback. Last fresh observation: bam23/rev74,
menus/editorclosed, fixture metadata configured but object absent, state100
speed.000069 at(143.253113,1.028671,-66.207054), heading24.645342. No job or
reservation remains; revalidate before future hardware work. The fixture's
recorded old start is now over22m away, so the current restore candidate's
four-metre travel guard will correctly reject it. Prepare a new source-checked
setup for the current stopped position instead of weakening ownership checks
or replaying the consumed v4 slot. Five bindings, broader source contexts,
all-feature gameplay acceptance and the isolated build remain outstanding.

### Lateral continuation: neutral evidence wait exposed an external-runner defect

The fresh-position lateral trial used the same measured twelve-metre native
setup after both peers released hardware. The immutable runner
`grind-native-two-leg-lateral-c048914-v1/trial.py`, SHA-256
198452e4bb5d8009029b8b03768e1254f194d1979f3573cd6e1d6f4e602658ec,
completed the original401→403 leg and independently read fine24/contact on
the same segment. It did **not** send the intended second lateral controls.
Receipt `results/two-leg-lateral.json.gz`, SHA-256
f4728bac4d3ecc16794a68aa05a767e382a9d12a92ea0dd7648200423a0c4649,
contains225 reader samples with both clocks.03333333135 seconds. Fine24
appeared2582.118ms; neutral pitch0 appeared2649.091ms; an unintended second406
began2688.647ms. The gate rejected its non-lateral target and detached.

Source identifies the exact cause: the trusted worker's `replay` always calls
`pad.neutral()` before a conditioned phase. The external two-leg gate still
enabled admission during that wait. Neutral therefore triggered the already
qualified return path before the requested lateral event could be delivered.
The waiter then raised `Native gate ended before second input`. This receipt
is not evidence that the proposed lateral control fails native admission;
that control never ran. No fifth named recipe follows from it.

A second external cleanup defect was exposed: `ExperimentSession.run` closes
and clears its pad on infrastructure failure, but the trial's first `finally`
statement unconditionally dereferenced `session.pad`. Its resulting None
exception skipped Arcade restoration and owned fixture undo. The worker had
neutralized/destroyed its pad; the native gate's log showed GDB detachment.
Independent status nevertheless confirmed ArcadeOFF and the owned long rail
remaining at bam23/rev84. No clean handback was claimed at that point.

Separately reviewed immutable cleanup
`grind-lateral-cleanup-c048914-v1/cleanup.py`, SHA-256
ad2dd5d5b781cad615ea61ae43d7d351c9273ff3c637dd7f1be9f4d2d4ec5d71,
rechecked installed identity, exact ownedID10/item/pose in read-only SQLite,
world revision and stopped rail state. It reversed exactly five owned native
transactions84→94, restored ArcadeON, closed the editor and verified IDs9/10
absent, TracerPid0 and the original native feature byte0. StockID2 was untouched.
Receipt `grind-lateral-cleanup-c048914-v1/results/1793528935478529000-status.json`
records native100 speed.000148 at(143.424789,1.028672,-51.628483), heading
14.997158, fixture metadata configured but actual object absent. Both peers
received explicit clean release.

The next external candidate leaves the trusted worker's neutral-on-wait
behavior intact. Instead, the second native lease stays disabled until fresh
raw GrindInputs show the intended negative lateral heading band and negative
pitch above the native destination-class threshold. First completion and
fine24/contact/segment evidence remain prerequisites. The gate observes these
inputs; it never writes target, contact, identity or animation. Original native
admission and initialization must still choose402/404 and complete normally.
Destroyed-pad cleanup now preserves the failure's partial delivery receipt,
detaches its gate, reacquires the shared lock before console/undo operations,
and continues even if no pad remains. Thirty-five inert control/neutral-wait/
cleanup cases and52 existing lease/setup cases pass. These changes affect only
the external experiment; production remains the four narrow recipes above.

The corrected immutable lateral v2 runner (SHA-256
28cd12dd851cf14f4c333f91ca95200559364d63ddda2baba510f1ce23918d1b)
was reviewed and executed after both peers released hardware again. Native
relocation rejected the current forward lane with `Fixture lane lacks a
continuous flat native collision surface.` It stopped before debugger/grind
input with zero native samples, and successfully undid its only owned short
placement96→98. Receipt
`grind-native-two-leg-lateral-c048914-v2/results/two-leg-lateral.json.gz`,
SHA-256 f294bff18c641f4697401961b562398f5ed2ffca2b07dd5c61b65d6ebff92336,
therefore does not validate the lateral gate correction in the game. Both
lateral receipt slots are consumed; never replay them.

Source-checked approach recovery is available through the existing fixture
reset, not a new mechanic: restore the exact current fixture metadata, close
the editor, let `flat_start(fixture.start,...,false)` recheck native ground,
then use the existing trainer/native teleport manager. Independently verify
the resulting approach, undo the owned restore and run full-lane validation
again before any trial. The fixture implementation is identical at installed
c048914 and this branch. This is setup recovery only, never rail-exit evidence.

Two bounded recovery attempts did not reach reset. The first's four-second
typing timeout stopped before Return; fresh status confirmed revision98 and
no new object. The corrected typing timeout allowed a placement, but exact
fixture matching failed at revision100. The read-only command log establishes
an input-delivery error: requested height `-5.00679016e-06` arrived in the
console as `-5.0079016e-06`. Persisted owned rail12 had height
-0.000005007901563658379, so it correctly did not match fixture metadata.
This is not evidence of a native float parser or editor reconciliation bug.
The standalone recovery helper lacked the main trial's exact echo guard and
its cleanup incorrectly required fixture presence before undoing its owned
create. It must be corrected before reuse, with a new reviewed immutable slot.

Separately reviewed cleanup `grind-reset-cleanup-c048914-v1/cleanup.py`
(SHA-2566cd62f19b550f4bc72c69a71b0ab987453602f7147637639ea5f854e96313680)
verified exactID12/item/persisted pose and reversed only transaction100→102,
then closed the editor. Receipt
`results/1793529576361022000-status.json` verifies ArcadeON, TracerPid0,
native feature0, absent fixture and stopped100 speed.000099 at
(143.424805,1.028672,-51.555122), heading14.997146. A final read-only database
and echo receipt `grind-reset-log-read-c048914-v1/results/1793529603661888000.json`
confirmed only preexisting stock rail2 remained. Both peers received clean
handback; no job or reservation remains. Revalidate c048914/PID85401 and
bam23/rev102 before further setup. No fifth recipe, new production behavior,
native lateral acceptance, DLL or CI build was added in this continuation.

Offline recovery v3 draft
`build/grind-investigation/grind-native-approach-reset-c048914-v3.py`
has SHA-256243173d1fb881eb1c60fe0f8c6c4970dbdb663e8abdf7ff6ce8d2a93fdfa552a.
It uses a slower bounded placement typing interval, requires exact command
echoes, retains structured errors, closes its own editor even on failure and
allows undo of the single acknowledged create even when its pose fails exact
fixture matching. Twelve positive/negative ownership cases pass, including
wrong generation/revision and busy/failed editor. It remains unstaged and
unexecuted; exact typing is not proven by these checks. Revalidate its pins
before review. The lateral v3 draft is also unexecuted and its expected
approach has not been reached; do not run it merely because revision102 now
exists after cleanup. A successful recovery will advance the revision again.

### Native approach recovery succeeds; actual lateral input exposes signed-guard error

Both peers released hardware again. Fresh identity verified the unchanged
c048914 DLL/executable and PID85401/launcher85410. A fresh capture showed
Steam idle artwork; the reviewed one-Shift wake restored visible gameplay.
Recovery v3's slower placement typing and exact echo checks succeeded. Native
fixture reset independently reached its existing start through the original
ground-checked teleport manager, and its single owned restore was undone
102→106. Receipt
`grind-native-approach-reset-c048914-v3/results/1793532050285587000-status.json`
records state504 at(143.253113,.949994,-66.207054), heading.003893, speed0,
ArcadeON and absent fixture. No raw transform was written.

The re-pinned lateral v3 runner SHA-256
65251c8319f142849a91ea86398d290c064941eb5fe5d4b8e5185b55c81efff6
then completed setup and delivered both requested controls. Receipt
`grind-native-two-leg-lateral-c048914-v3/results/two-leg-lateral.json.gz`,
SHA-25634a587e21b63a4e1ae2cd7a3e3e4218e921a9e1d069baf337f8e08d330d84134,
contains468 read-only samples, both native clocks.03333333135 seconds. The
first original401→403 completion and fine24/contact/segment were verified
before the second event. The neutral evidence wait no longer triggered an
unintended return. Its40.744ms wait completed before the second input write.

The actual raw lateral request was **positive** heading1.50407398 radians,
pitch-.400000006 and roll0. Forty bracket-stable source403/fine24 samples
from2744.910 through3266.093ms retained segment[1838246002727,0] and published
native admission1. The external gate incorrectly required a negative heading,
so it never enabled a second actor. The native403 predicate at0x481d910 wraps
and takes the absolute heading before its admission comparisons; the sign of
an earlier small sweep is not a valid assumption for this captured request.
Correcting the experimental guard to the absolute band preserves the neutral
rejection and still leaves original native destination selection authoritative.

Afterwards ordinary controls changed contact to a side facet and fine25,
causing the strict same-segment gate to retire. Later fine22 at an end segment
does not qualify a requested Tailslide: there was only one original406
completion and intervening segment changes. No fifth production recipe was
added. Cleanup returned GDB0, restored native feature0, removed all owned
fixture transactions and left ArcadeON, editor/menuclosed, state100 speed
.000159 at(143.691406,1.028672,-46.757153), bam23/rev126. Both peers received
that clean handback before a further bounded retry was requested.

### Fifth qualified control recipe: native 5-0 to Bluntslide

Both peers released hardware for the next bounded trial. Source-checked
approach recovery v4 succeeded126→130; its immutable runner SHA-256 is
8056ee5d6ad1fb0ca9a1de0ab1ea17eceeacca240d3d9f200177fc9a4007df50 and receipt
is `grind-native-approach-reset-c048914-v4/results/1793532390248816000-status.json`.
The absolute-heading correction then succeeded in immutable runner
`grind-native-two-leg-lateral-c048914-v4/trial.py`, SHA-256
70b84f6449d68978d6e3a5f7aeb94918dc3020aa7033fb3e88450f1f8d4d3b81.
Receipt `results/two-leg-lateral.json.gz`, SHA-256
06523b2ba624d8c35319db57f3b1f7ba4be5f2092b67d942d58046f375b1471e,
contains473 independent reader samples with both clocks.03333333135 seconds.
This slot is consumed; never replay it.

The native sequence was401/fine20 at2042.324ms, first406 at2211.506ms,
403 at2543.889ms and fine24 at2572.199ms. After original first completion
and verified fine24/contact/segment, the conditional neutral wait lasted
51.301ms. The second input was evdev RS(32767,8192), corresponding to native
XInput(32767,-8192). Raw native GrindInputs became heading1.5040742 radians,
pitch-.400000006 and roll0 at2740.115ms. Only then did the scoped feature
override admit another original406, at2777.116ms. Original initialization
selected402; its original update completed403→402 with phase1 after
.666666508 seconds. There were exactly two original completions.

Independent physics reached402 at3439.775ms and **fine31 BS_Bluntslide** at
3492.549ms. The initial402/fine59 interval had physical and published contact1,
missing-segment0 and unchanged endpoints; it was not accepted as Bluntslide.
Fine31 then persisted until native701 at4697.997ms, approximately1.2 seconds
of retained destination contact. Segment identity[1984274890791,0] and the
same measured12m endpoints remained throughout the accepted destination:
(143.4765625,.500742197,-46.8037109375) and
(143.4765625,.500742197,-58.8037109375). Native406 temporarily lifted physical
contact and published intermediate identities25/59; neither is fabricated
exit evidence or a requested trick acknowledgment. The gate's early changed
identity observation alone is insufficient: the separate reader establishes31.

Production now includes **B→X Bluntslide from established403/fine24**, plus
the existing simultaneous B+X priority. It retains the narrow horizontal
geometry1/at-least12m source qualification. The bounded RS actor owns neutral
for50ms before the measured lateral tail input. A new per-request admission
policy reads the original raw GrindInputs pointer at context+5248 (heading+24,
pitch+60,roll+52), brackets those reads against unchanged owner/contact and
rejects torn/nonfinite input or a missing/replaced GrindInputs pointer. Initial native admission stays disabled until
the neutral interval has elapsed and the actual request matches absolute
wrapped heading strictly between55–125 degrees, pitch(-.45,-.1745329201), and abs(roll)<.001.
This prevents the v1 failure where neutral accidentally admitted a50-50 return.
Original native admission and initialization still select the destination;
the code never writes a target state, name, animation or contact field.
In-flight406 uses the existing verified lifetime path. Only402/fine31 with
the original segment, original403→402 completion, and independently changed
animation can satisfy the request. A native404 result cancels instead.

The other four recipes retain their original controls and ordinary admission.
The new guard is bypassed for those requests. Focused strict warnings plus
ASan/UBSan passed **6027 production-composed checks at30/60/120Hz**, including
sequential/simultaneous B-X, original physical timestamps, repeated polls,
neutral admission rejection, both heading signs, torn/stale owner/control
evidence, intermediate59 rejection and independent animation acknowledgment.
Additional sanitizer checks passed:639 native lifetime,1362 rail-pop routing,
1736 combo trace. These are offline checks. Windows/backend integration
compilation, new DLL/CI and live Arcade animation/HUD/balance/combo acceptance
remain pending. The known unchanged-baseline contextual-air press-age failure
remains; this is not a claim of complete green checks.

The native receipt was ArcadeOFF control evidence. Four bindings still lack
qualified executors: Y→X Noseslide, B→Y Nosebluntslide, X→X Smith and
X→Y Tailslide. Source402 exits, arbitrary repeated sequences, broader source
contexts and the local Ollie/early-flip candidate's live acceptance remain
unresolved. No new DLL or CI build was started.

Cleanup returned GDB0, restored native feature0 and undid all owned fixture
transactions to bam generation23/revision150. ArcadeON, automatic routing,
pad destroyed, menus/editor closed and fixture metadata configured/object
absent were verified. Last stopped native100 speed.000430502 at
(148.082764,1.028666,-43.454994), heading70.433243. Identity remained
c0489142ef8f1380606688d02d5e3c8f833909b5, DLL
7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
game85401/launcher85410. Both Manual and Grab received clean handback;
no Grind job/reservation remains. Re-coordinate and revalidate identity,
world and source-checked setup before further input; these are historical pins.

### Sixth qualified recipe: Nosegrind to Noseslide, not Nosebluntslide

Both peers released another bounded window. Fresh identity confirmed the same
c048914 DLL/PID85401; capture showed Steam idle artwork, and the reviewed
one-Shift wake restored visible gameplay. Native status independently pinned
bam23/rev150 and current fixture metadata. The exact-echo native approach
reset succeeded150→154 using the existing ground-checked teleport manager,
then undid its sole owned placement. Reset runner SHA-256
18593e2c42e02f7d63dac0187ba895c0f244a1be8d0786dc3b6faba5429fd03b;
receipt `grind-native-approach-reset-nose-c048914-v1/results/1793535992059409000-status.json`.

The first nose-lateral runner's outer operation was enabled_up but its inner
trial selector still required enabled_down. It performed status only, with
zero native samples, no fixture change and unchanged revision154. Receipt
`grind-native-nose-lateral-c048914-v1/results/two-leg-lateral.json.gz`, SHA-256
ee6d1a41a8cc8eccde2bd84f444a7fa2510e79ba34f03b3b752e492cdcea245f,
is consumed and is not native evidence. A new immutable v2 changes that exact
selector;71 extracted inert setup/ownership/control/reachability checks pass.

Reviewed v2 runner SHA-256
9a312a225f8b23cd8082ec8c2641fb5c6df8b73de660bf8db8120aebf571d516
completed both original native legs. Receipt
`grind-native-nose-lateral-c048914-v2/results/two-leg-lateral.json.gz`, SHA-256
a47227610d824030473948b7dc1641d55b67ebde3891e65fdff8194f2d59d3b6,
contains490 independent samples, both clocks.03333333135 seconds. First406
at2196.152ms completed401→403; fine23 Nosegrind appeared2594.324ms. The
conditional neutral wait lasted44.730ms after verified original completion
and fine23/contact/segment. The second physical input was evdev(32767,-8192),
native XInput(32767,8192). Raw heading1.637928 radians, pitch+.400000006,
roll0 appeared2750.156ms; second406 started2787.297ms. Original403→402
completed at phase1 after.499999970 seconds, with orientation approximately
(1.664835,.412159,0). No target, identity or contact was assigned.

Native402 initially published intermediate66 at3294.840ms, then actual
**BS_Noseslide21 at3343.864ms**, not Nosebluntslide30. There are105 independent
402/fine21 samples through4672.427ms, all with physical/published contact1,
missing-segment0 and original segment[2130303778855,0]. The measured12m
endpoints remained(143.5869140625,.500742197,-46.60546875) and
(143.5869140625,.500742197,-58.60546875). Later native701 at4688.205ms was
the natural rail exit. Intermediate32/66 in the transition are not accepted
names. Both original actor completions and cleanup were observed separately.

Production now routes **Y→X Noseslide from established403/fine23** through
the measured lateral nose request. Simultaneous Y+X keeps the existing table
priority. It uses the shared bounded neutral interval and lateral heading
guard, with a separate positive-pitch policy and source23 check. It requires
horizontal geometry1 at least12m, fresh owner/contact, original403→402
completion, same-segment fine21 and independently changed animation. Negative
pitch, intermediate66, actual30 or a different source/segment cannot satisfy
it. The existing five recipes retain their controls. Ordered B→Y remains
unsupported: a Noseslide result cannot be advertised as Nosebluntslide.

Strict C++20 warnings and ASan/UBSan pass7522 production-composed checks
at30/60/120Hz, including both lateral recipes, sequential/simultaneous input,
polarity-specific native gates, cancellation, intermediate/wrong-name rejection
and independent animation evidence. This is not DLL/native production or
Arcade HUD/combo acceptance. No build, install, merge or release occurred.
Three bindings still lack recipes: Nosebluntslide, Smith and Tailslide;
broader source contexts, arbitrary repeated changes and live Ollie/early-flip
acceptance also remain incomplete.

The trial restored native feature0/GDB exit0, destroyed its controller,
restored ArcadeON/automatic routing, closed editor/menu and undid all owned
edits to bam23/rev174. Metadata remains configured/object absent. Stopped
native100 speed.0000684065 at(143.825363,1.028673,-45.094643), heading277.297485.
Both peers received clean handback, no job/reservation remaining. A subsequent
opposite-heading candidate requires a newly coordinated window and fresh pins;
none of the consumed reset/trial slots may be replayed.

### Seventh qualified recipe: opposite heading produces Nosebluntslide

After a new release from both peers and a fresh identity-pinned UI capture,
the native approach reset succeeded174→178. Reset runner SHA-256
b50f44fac3fbd1ff0fcac65a0adb5c5419d5785599d949e136e3593126ec0b64,
receipt `grind-native-approach-reset-nose-opposite-c048914-v1/results/1793536540959874000-status.json`.
Only the second lateral X sign and fresh pins changed from the successful
Noseslide experiment; no destination class or identity was assigned.

Immutable opposite-heading runner SHA-256
71542dfcbe10bf07a742c65789dcd5fa5def19d642d75d3be8cc312b93598da6
produced receipt `grind-native-nose-opposite-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-256 eb8fdaa09ee61f5a126c129c7ea6e3920382dddd8d77dc8acf24c3f62a3629da.
It contains505 independent samples with both clocks30Hz. Original first406
at2158.724ms completed401→403, then fine23 appeared2559.451ms. After the
47.762ms conditional neutral wait, the second physical event was
evdev(-32767,-8192), native XInput(-32767,8192). Raw native heading was
-1.014080 radians, pitch+.400000006 and roll0 at2725.532ms. Original second406
started2797.099ms and completed403→402 at phase1 after.699999809 seconds.
Its original initialized request heading was-1.308997 radians; the running
retarget was-1.014080 and completed orientation approximately(-1.310732,.335029,0).

Independent native physics initially published402/fine26 at3493.008ms,
then **BS_Nosebluntslide30 at3540.360ms**. There are85 matching retained
destination samples through4545.499ms before natural701 at4556.814ms.
All have physical/published contact1, missing0 and the original segment
[2276332666919,0], endpoints(143.6982421875,.500742197,-46.4052734375) and
(143.6982421875,.500742197,-58.4052734375). Native segment_changed was1 at
first402 while actual endpoints/identity were unchanged, demonstrating why
that initialization flag alone is not evidence of a real rail exit.
Intermediate33/26 and the temporary physical lift during406 are recorded.
**This does not qualify Smith26:** that identity was transient during the
other requested action and did not persist as the requested destination.

Production now executes ordered **B→Y Nosebluntslide from403/fine23**,
horizontal geometry1 at least12m, through its own negative-heading/positive-
pitch admission policy and the measured opposite RS input. It requires
original403→402 completion, actual same-segment30 and independent changed
animation. Noseslide21 and transient26 cannot satisfy it. Simultaneous B+Y
retains the existing table's Y→B Nosegrind priority and is not an invented
ordered Nosebluntslide alias. Strict warnings and ASan/UBSan pass8283
production-composed checks at30/60/120Hz, including wrong-polarity rejection,
control cancellation, held/repeated polls and separate animation evidence.
The native receipt is still ArcadeOFF physics evidence; HUD/balance/combo
and live production/Ollie acceptance remain outstanding. Smith and Tailslide
are the remaining two unqualified bindings; broader sources/repeated changes
remain incomplete. No DLL/CI build or deployment occurred.

Cleanup verified GDB0/native feature restored, destroyed pad, ArcadeON and
automatic routing, menus/editor closed, and all owned edits undone to
bam23/rev198. Configured metadata/object absent, native100 speed.0000309014
at(148.036606,1.028673,-40.816639), heading78.319359; c048914/PID85401 unchanged.
Both peers received clean handback before any further bounded window request.
This reset and trial slot are consumed.

### Opposite tail reaches Tailslide, but held controls do not retain it

A newly coordinated window revalidated c048914/PID85401 and reset the approach
through the original ground-checked fixture manager, revision198→202. Reset
runner SHA-256 63e49fb8380dc2d791a259cc7aadd5da1d7595be37c9353646e64038979730b3;
receipt `grind-native-approach-reset-tail-opposite-c048914-v1/results/1793536996656794000-status.json`.
The immutable tail-opposite runner SHA-256
c4a7cfeed772f7ec0ee80ab763b643d2652fc4a753c8d80808379d9f50bf7715
changed the second lateral X sign after measured5-0. Receipt
`grind-native-tail-opposite-c048914-v1/results/two-leg-lateral.json.gz`, SHA-256
28a533fe7c3c752ae23b4695dfec42c918ed700191714abfb9b3e0a8ce8ffac5,
contains484 independent samples with both clocks.03333333135 seconds.

Original401→403 completed and fine24 appeared2574.371ms. Following43.598ms
conditional neutral, native XInput(-32767,-8192) produced raw heading
-2.12752366, pitch-.4 at2747.746ms. Second406 started2811.848ms; original
403→402 completed at phase1 after.533333302 seconds. Independent physics
initially published402/fine71 at3339.657ms, then **BS_Tailslide22 at3383.743ms**.
The original segment[2422361554983,0] remained unchanged, with measured12m
endpoints(143.8076171875,.500742197,-46.2041015625) and
(143.8076171875,.500742197,-58.2041015625), physical/published contact1.
However, the held lateral controls subsequently produced403/fine22 at4013.433ms
and403/fine71 at4040.322ms. Later physical-contact loss and fine72 preceded
natural701 at4746.263ms. The roughly.63-second Tailslide interval is real,
but **does not qualify a reliable production Tailslide recipe**. No eighth
recipe was added. A bounded earlier control withdrawal needs qualification.

The experiment gate stopped with AssertionError on intermediate71: its
assumed upper identity bound68 was wrong. Both original completions were
already recorded, and the independent reader continued through actual22
and the subsequent drift. Native enum asset
`build/grind-investigation/native-identity/ebx-1767.txt`, exported GUID
75934470-c85c-df18-7593-4470c85cdf18, defines69–72 as Suski variants and73–76
as Darklip variants;71 is BS_Suski. Invalid is0 and Exit is77. Thus71 alone
must not be interpreted as a physical rail exit or as an accepted named target.

The production native contact reader now accepts real enum identities1–76,
while retaining all independent owner/vtable/contact/segment guards and
rejecting0,77 and out-of-range values. Named source/target qualification is
unchanged: none of69–76 can admit a requested recipe or acknowledge one.
Strict ASan/UBSan checks pass651 contact/lifetime cases and8291 composed
grind-change cases, including all eight additional real contact identities,
exit/out-of-range rejection and unqualified-source rejection. This correction
prevents a native identity change from masquerading as contact loss; it does
not claim HUD/balance/combo or live production acceptance.

Cleanup independently returned GDB0/native feature restored, destroyed the
pad, restored ArcadeON/automatic routing, closed menus/editor and undid all
owned transactions to bam23/rev222. Fixture metadata is configured/object
absent. Native100 speed.000158463 at(139.824631,1.028673,-40.812485), heading
265.599609; installed identity unchanged. Both peers received clean handback;
no job or reservation remains. This reset/trial slot is consumed. Seven narrow
recipes remain implemented; Tailslide stability, Smith, broader source contexts,
arbitrary repeated changes and live Ollie/early-flip acceptance remain open.
No DLL/CI build, installation, merge or release occurred.

### Eighth qualified recipe: bounded opposite-tail pulse retains Tailslide

Both peers released another short window. Fresh identity-pinned capture showed
Steam idle artwork; the reviewed single-Shift wake restored visible gameplay.
The exact native approach reset222→226 succeeded, runner SHA-256
00372c5cc426f1307299f9830fdd01ddb7c47a465e4ebb98bf4b9db24affda3e,
receipt `grind-native-approach-reset-tail-release-c048914-v1/results/1793537743803919000-status.json`.

The new trial preserves the previous opposite-tail controls but explicitly
sends neutral1000ms after the second lateral event. This is a fixed bounded
pulse; it does not add a conditioned phase, which would cause the existing
worker to neutralize before its evidence wait. Three inert shifted writer
timelines preserve that1000ms spacing, plus20 lease/geometry and80 real-enum
destination checks. The gate admits no third transition, tolerates genuine
intermediate1–76 identities, and ends only after original403→402 completion
and actual402/fine22 with the original contact/segment. It never assigns22.

Immutable runner SHA-256
9bfc1528c0edf9166b38a9f8190e3bf86f9047bdbff51be7a34273c0efc5b200;
receipt `grind-native-tail-release-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-25622d17f6ec2af2d21364e8a7099c1b27a53639255c7631e36ab111e76a3692743.
There are504 independent samples, both clocks.03333333135 seconds. First406
at2158.586ms completed401→403; actual5-0 appeared2561.805ms. After45.244ms
conditional neutral, opposite lateral input produced raw heading-2.127533
and pitch-.4 at2730.427ms. Second406 started2792.056ms, and original403→402
completed at phase1 after.533333302 seconds. Initial402/fine71 at3333.293ms
was correctly treated as a real intermediate contact, not destination success.

Actual **BS_Tailslide22 appeared3368.459ms**. The physical writer sent its
lateral event2640.789ms and neutral3640.933ms on its own clock, a1000.144ms
interval. Independent raw controls reflected neutral's native heading target
-pi/2 at3688.480ms, with native pitch settling near-.349066. There are116
402/fine22 samples through4748.106ms, all physical/published contact1,
missing0, same segment[2568390443047,0] and measured12m endpoints
(143.9140625,.500742197,-45.99609375) and
(143.9140625,.500742197,-57.99609375). Natural701 begins4759.304ms.
Unlike the held-input negative control, there was no403/Suski drift before
the rail ended. Gate reason is second_native_tailslide_observed, exactly two
original completions; native initialization and cleanup remain original.

Production now executes ordered **X→Y Tailslide from403/fine24**, horizontal
geometry1 at least12m, using its own negative-heading/negative-pitch admission
policy. It owns neutral50ms, native RS(-32767,-8192) for1000ms, then neutral
through the existing1500ms request bound. Early animation acknowledgment
cannot skip this neutral tail. Only original403→402 completion, actual
same-segment22 and independently changed matching animation acknowledge the
request. Intermediate71 is not accepted. Simultaneous X+Y retains the existing
Noseslide priority, not an invented ordered Tailslide alias. The other seven
recipes keep their original controls, admission and completion timing.

Strict warnings and ASan/UBSan pass **9124 production-composed checks** at
30/60/120Hz, including physical timestamps, ordered-versus-simultaneous
priority, held polls, source/geometry/owner rejection, admission polarity,
the exact neutral boundary, early animation evidence and bounded cleanup.
Contact/lifetime remains651 checks. This is implementation plus ArcadeOFF
native physics evidence, not live production/animation/HUD/balance/combo
acceptance. Smith remains the ninth unqualified binding; broader source
contexts and arbitrary repeated changes remain incomplete. The local
Ollie/early-flip candidate is still unaccepted live. The known unchanged-baseline
contextual-air press-age assertion remains; no complete-green claim or new
DLL/CI build is made.

Cleanup verified GDB0/native feature restored, destroyed pad, ArcadeON and
automatic routing, closed menus/editor and all owned edits undone to
bam23/rev246. Fixture configured/object absent. Final native100 speed.000003915
at(141.996872,1.028673,-43.620945), heading113.796249. Identity remains
c048914/PID85401. Both Manual and Grab received clean handback; no job or
reservation remains. These reset/trial slots are consumed. Any next trial
requires new peer coordination, fresh identity/world pins and a new immutable
runner; old coordinates and receipts may not be replayed.

### Smith investigation: separate native board pitch from a jump gesture

The next source pass follows the original 5-0 heading and pitch programs,
not a guessed named-state assignment. The heading asset is EBX34197 paired
with RES7329 (`Gameplay/SkaterIntentTree/Grinds/Heading/CalculateTargetHeading_5_0`).
Its pitch/physical-side branches and heading constraints suggest examining
original403 orientation while maintaining contact. Native403 may also select
403 when outside the lateral heading band with non-neutral pitch; the earlier
402/404/401 summary was not exhaustive. Trainer/schema defaults include
MinSmithPitch=.0125, MinSmithHeading=.2 and Max5_0TargetPitch=.4. These defaults
are source evidence, not a live tuning readback or a qualified input recipe.

Bounded native disassembly also resolves call hash c07cd1f5 at RVA16eb5c0
as floating not-equal with epsilon, not boolean XOR; ce900afc at16d7890 is
addition,30364d4d at16e25d0 multiplication, and7bec9f8f at16e4d00 subtraction.
The corrected local annotation is `build/grind-investigation/grind-heading-5o-annotated.txt`.
Smith still requires the actual physical contact tuple and negative signed
axis/reference dot recorded above. Neither a tuning name nor requested pitch
establishes that result.

Both peers released a bounded window. Reset runner SHA-256
e500873c85d78af8844f3550ab5d6db131d442fbc834d80aabd08de9d09ad7f0
successfully restored the native approach246→250; receipt
`grind-native-approach-reset-smith-pitch-c048914-v1/results/1793539681090995000-status.json`.
The subsequent trial enables only the already qualified original401→403
preparation. It waits for original completion, actual5-0/contact/same segment
and debugger detachment before delivering opposing pitch. No second native
transition lease is admitted, and no contact, identity or target is written.
Seventeen inert lease/waiter checks passed before the trial.

Immutable trial SHA-256
d16f0e113dc105bdcc7f14b7c5d5d8cd9ba2b7f4aa4c58db44d4d4aabc8c5c16;
receipt `grind-native-smith-pitch-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-256de0f0cbd830eaee51596460fe92cbf5dcb7d664f51bc7cc1c585ccede528c10d.
The484 independent samples have both clocks.03333333135 seconds. Original406
began2224.559ms and completed401→403 at phase1 after.3333333135s; actual5-0
appeared2587.979ms. Gate reason first_native_5_0_verified, one completion,
then detached gate. The writer sent nativeUp2675.920ms after the first
destination wait and neutral3675.964ms, a1000.044ms pulse. Raw requested pitch
became+.4 at2746.915ms, but native200 began2783.968ms and201 at2820.332ms
with lost physical contact. Native401/fine20 re-entered the same12m segment
at3648.489ms. There was no fine26 sample. Later incidental403/24 and402/31
occurred on a different short end segment and cannot acknowledge Smith.

This is a negative Smith control and evidence of a native air transition
following abrupt opposing input. It does **not** verify the A-button Ollie
actor, upward launch velocity, early flip handoff, animation, HUD or combo:
the collector did not record launch velocity and Arcade was OFF. The segment
was[2714419331111,0], endpoints(144.0263671875,.500742197,-45.7958984375)
and(144.0263671875,.500742197,-57.7958984375). Cleanup restored the native
feature, detached GDB0, destroyed the pad, restored ArcadeON/automatic route,
closed menus/editor and undid all owned edits to bam23/rev270. Both peers
received clean handback. No ninth production recipe was added.

### Slow opposing pitch avoids the air gesture but does not produce Smith

A separately coordinated window tested whether the abrupt input above was
being interpreted as a jump flick. The source defaults GestureInputBufferTime=.2
and FastFlipInputSpeed=.0025 informed a slower candidate; neither was read
back as active live tuning. After original401→403 completion, actual5-0 and
same segment, the new waiter additionally requires detached debugger and
native flags+26==0. It then owns neutral300ms, five nativeUp steps of roughly
.2 normalized magnitude100ms apart, fullUp800ms, then neutral. The original
worker and first preparation lease are unchanged; there is no second lease.
Eighteen inert lease/waiter cases and eight timeline/ramp checks passed.

Reset runner SHA-256
7480ee88d2e2be97748a906682a64f31758389f0696434320f9bb36d619ea4b9
successfully recovered the approach270→274, receipt
`grind-native-approach-reset-smith-ramp-c048914-v1/results/1793540164242451000-status.json`.
Immutable trial SHA-256
1a87ce6c4dbca7a125fdedc512c81e5cceb002dc23bc4d18305d684761533551;
receipt `grind-native-smith-ramp-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-256670166b22f09e2542cb2137cdd657962e07099c882f1bf1f65a34926df6dcebf.
There are471 independent samples, both clocks.03333333135 seconds, one original
401→403 completion and gate reason first_native_5_0_verified. Actual5-0 appears
2587.654ms. The writer delivers neutral2665.954ms, ramp steps2966.091,
3066.948,3166.019,3266.035,3366.029ms, then neutral4166.187ms. These are
physical write timestamps, not assumed game polls.

Unlike the abrupt negative control, this sequence produces no200/201 exit
after5-0. It still produces **no Smith26 anywhere**. Raw requested pitch becomes
.110400 at3247.100ms, .335949 at3312.075ms and .4 by3407.448ms. Actual pitch
approaches-.051 rather than immediately following that request. At3407.448ms
native401 selects a different physical segment, followed by fine20 at3445.213ms;
403 reacquires the original segment at3483.389ms and fine24 at3521.864ms.
This alternating401/20 and403/24 pattern repeats through natural701 at5016.212ms.
The original12m segment[2860448219175,0] has endpoints
(144.1455078125,.500742197,-45.5947265625/-57.5947265625); the alternative
segment[2860448219203,0] is offset to x144.23211669921875,y.4507421851 with
the same endpoint z values. This is a real change in physical contact geometry,
not a successful retained-segment named transition.

Published contact remains1 during the post-ramp rail samples, but physical
contact is0 in one sample at4238.751ms. Some independent segment brackets also
return unavailable during changes. Do not hide these observations behind a
claim of uninterrupted contact. Stable admission samples continue to report
selected physical contact bit0, and the5-0 classifier retains tuple element3=0
instead of Smith's1. Changing requested pitch alone has therefore not established
the necessary contact/axis conditions. A next source pass must explain the
native candidate switch and heading/contact constraints before another
experiment; do not add a control-only executor by fabricating406 completion.

Cleanup independently verifies GDB0/restored feature, destroyed pad,
ArcadeON/automatic routing, closed menus/editor and exact-owned undo to
bam23/rev294. Native100 speed.000127937 at(145.228806,1.028673,-42.913170),
heading15.657194. Fixture configured/object absent; new metadata start
(144.14633178710938,1.028671145439148,-64.5953140258789), rail
(144.14633178710938,-.00000476837158203125,-56.595314025878906), unit scale.
Installed c048914/PID85401, launcher85410 and recorded DLL hash are unchanged.
Both peers received clean handback; no Grind job/reservation remains. Both
new reset/trial slots are consumed. Production remains9d61704 with eight narrow
recipes and the same offline checks. Smith, broader contexts/repetition and
live production/Ollie/early-flip/HUD/combo acceptance remain open. No DLL/CI
build, installation, merge or release occurred.

### Smith reference and physical contact producers; heading-pulse negative

The original common rail update at0x4822e26..37 copies context+3920 to
actor+160 and actor+112. Publication at0x4818a05..0e copies actor+112 to
output+48, the Smith classifier's reference vector. Context+3920 belongs to
the contact substructure (3024+896); it is not the requested pitch. Actor+96
comes from context+3056 and actor+128 is a cross product of the two basis
vectors. Keep those physical outputs separate from raw GrindInputs.

Further original403 candidate tracing at0x47ed610 shows projected separation
and segment-direction tests against context+64/+80 before selecting the
opposing contact candidates. At0x47eda93..dad3, the direction-of-travel dot
context+976 dot context+96 orders candidate preference. Another branch at
0x47edb03..53 uses context+5064 bit30 or context+6368; neither is an identified
stick command. At0x47d82be..e4, bit30 is copied from byte21 of the cached
step-input object at core+968. Its constructor0xfd61b0 clears bytes18..25 and
sets cached dt at+32. This narrows the producer chain but does not establish
a writable Smith command. The physical401/403 query also evaluates both
probe points and their segment directions at0x47ef2f8 onward; the alternate
segment seen in the receipts must not be hidden as a mere label change.

A fresh coordinated window tested native tail heading before the prior slow
opposing-pitch ramp. Reset runner SHA-256
406b108742b9a1ce10a45b5eb0aed04e25a8380dd364667675c40216c1a6a93a
recovered294→298; receipt
`grind-native-approach-reset-smith-heading-c048914-v1/results/1793543514027261000-status.json`.
Immutable trial SHA-256
51bf193a4b4c25fdd168d586f5cb50a870910040f3f0d4001e87781521b45dc7
adds evdevRS(24000,32767)300ms after the detached original5-0 wait, then
neutral300ms and the previous five-step Up ramp. The original preparation,
lease and worker are unchanged. Eighteen lease/waiter,12 timeline and7 inert
classifier-vector bracket checks passed. Newly included passive vectors
source+320/output+0/output+48 remain owner/pointer/data bracketed.

Receipt `grind-native-smith-heading-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-25650a1e54a26617cc2133b8c6bdfa714ab7ccd23a324155fa80e37189599d56eb7,
contains471 independent samples, both clocks.03333333135 seconds. There is
exactly one original401→403 completion; native5-0 appears2597.423ms. NoSmith26
occurs. The writer sends the diagonal2711.902ms, neutral3012.000ms, ramp
3311.972 through3712.052ms, final neutral4511.993ms. Raw native heading stays
about-.31173 during the diagonal; it does not follow the new physical X axis.
Pitch responds, but at3721.235ms native401 changes to offset segment
3006477107267;403 reacquires original3006477107239 at3783.332ms. Alternating
401/20 and403/24 continues through native701 at5320.963ms. Two transient
invalid0 identity publications are retained in the receipt, not counted as
successful changes. The original endpoints are x144.25390625,y.500742197,
z-45.396484375/-57.396484375; the offset segment is x144.34051514,y.450742185.

In the2720–5300ms analysis window, all199 samples retain physical and
published contact but alternate those segments. Of147 native403 samples,
144 have stable classifier-vector brackets. Their signed-axis/reference
dot ranges-.040562 to+.385269; the52 native401 samples range-.069061 to
+.068608. Neither reaches Smith's<=-.1 threshold. Measured output+48 is
(0,1,0). This establishes the failed physical predicate directly rather than
assuming requested pitch controls it.

The heading result needs a control-reference correction in the experiment,
not an enum write. The original orientation action32849/11786 captures
current control at PCs0550–0574 and clears its reference on absence at
0adc–0b58. The trusted worker neutralizes before the conditioned evidence
wait. Therefore starting immediately with the diagonal may recapture it as
zero rather than generate a heading change. This is a source-derived
explanation, not a recorded read of the saved control-reference variable.
A bounded follow-up must first establish native Down, then change heading
without a neutral gap and verify actual raw heading before interpreting
Smith evidence. No production recipe is added from this negative result.

Cleanup independently verifies GDB0/restored feature, pad destroyed,
ArcadeON/automatic route, menus/editorclosed and all owned edits undone to
bam23/rev318. Native100 speed.000027947 at(145.278259,1.028673,-42.633465),
heading14.461284. Fixture configured/object absent; metadata start
(144.25428771972656,1.028671145439148,-64.3973388671875), rail
(144.25428771972656,-.00000476837158203125,-56.3973388671875), scale1.
Installed c048914/PID85401/launcher85410 and DLL hash are unchanged. Both
peers received clean handback before any follow-up coordination.

### Original403 control-reference sequence produces Smith; ninth narrow executor

Both peers confirmed the next bounded window. Fresh capture showed gameplay
and unchanged installed identity. Reset SHA-256
0be54d22f13f10427207fccb4ca62fe2470bce253ec86578fd5e64b4e6f557d7
recovered318→322; receipt
`grind-native-approach-reset-smith-reference-c048914-v1/results/1793544124916489000-status.json`.
The new immutable trial SHA-256
3b9faa2721dfe35d8d371f57c21d9c0ac6c6292c09fde95b4a04c4de3acef831
holds native Down200ms after the verified5-0/detached-gate wait, then
RX24000/Down300ms, neutral300ms, five Up steps100ms apart, fullUp800ms,
and neutral. Eighteen unchanged lease/waiter and13 timeline/reference checks
passed. Original preparation, worker, passive reader and one-lease gate are
unchanged. There is **no second406**, assigned destination or native data write.

Receipt `grind-native-smith-reference-c048914-v1/results/two-leg-lateral.json.gz`,
SHA-2566fbed3da9095ade50932c2fa9d997a08db85f0b36c02cc0795f830294a9c15f8,
contains467 samples with both clocks.03333333135 seconds. Original401→403
preparation completes once; fine24 appears2609.393ms. Writer Down2695.867ms,
diagonal2895.945ms and neutral3195.935ms now produce raw native heading
-.968297 at2946.383ms, unlike the prior diagonal-first negative control.
Intermediate native403/fine71 appears3336.184ms. Ramp writes3495.954,
3595.984,3695.962,3796.549,3895.928ms followed by neutral4695.935ms.
Actual403/fine26 appears4077.057ms and remains on original12m segment
[3152505995303,0] through5588.380ms:115 samples,113 stable classifier brackets,
one original403 actor4137961440. The bracketed candidate and accepted tuples
are actual Smith[1,1,0/1,1,1,1], and signed-axis/reference dots range
-.384697 to-.121755, satisfying the native<=-.1 predicate. Endpoints are
x144.3740234375,y.500742197,z-45.1982421875/-57.1982421875.

This is sustained native named physics evidence, **not uninterrupted physical
contact acceptance**: physical contact is0 at4366.607ms despite published
contact1 and the retained segment. All other114 Smith samples report physical
contact1. The production executor continues to cancel on such a loss; it
does not relax contact guards to hide this observation. Near rail-end, later
fine25 on a different short segment and701 at5742.895ms are not Smith evidence.
There is no animation/HUD/combo observation in this ArcadeOFF receipt.

`GrindChange` now handles ordered X→X from established native403/fine24 on
horizontal geometry1 at least12m using this native control-only trajectory.
Its production sequence uses native XInput Y signs (opposite evdev):
Down200ms, RX24000/Down300ms, neutral300ms, Up magnitudes6553/13106/19660/
26213/32767 at100ms intervals, fullUp until2000ms, then owned neutral until
the2200ms bound. It never arms the experimental transition feature or invents
a406 completion. A separately owner/pointer/data-bracketed native reader
requires state403 and original flags+26==0; entry additionally requires
observed neutral raw pitch/roll and physical RS. Missing native evidence,
feature activation, another actor, owner/segment/contact loss, stale frames,
OFF/menu, A or another input owner cancels delivery. The eight existing
transition recipes retain their1500ms lease and completion behavior.

Same-segment actual26 and independent matching changed animation are still
required for success. `Evidence.control_only` distinguishes this path;
`Evidence.completed` remains false because no406 ran. Suski71, stale source
labels, another segment and fabricated completion records cannot acknowledge
Smith. Early acknowledgment cannot skip the final neutral interval. Runtime
integration bypasses only the406-pending requirement for this control-only
actor; the existing adapter, RS priority, masks and native flip paths are
unchanged. No balance, combo, manual, prewind or camera code was modified.

Strict C++20 ASan/UBSan production composition passes10722 checks at30/60/120Hz,
including repeated polls/held X, original physical timestamps, control phases,
negative reader races/owners and cancellation.651 existing native-lifetime
and1362 rail-pop checks also pass. The1736 combo trace checks pass with the
existing portable warning profile. An additional -Wconversion/-Werror attempt
rejects unchanged combo-test float-to-bool assertions; those protected tests
were not edited. The previously recorded contextual-air baseline assertion
also remains unresolved. These are focused offline checks, not full platform
CI or gameplay acceptance; Windows/backend compilation remains deferred.

Cleanup independently verifies native feature restored, GDB0/detached,
destroyed pad, ArcadeON/automatic routing, closed menus/editor and exact owned
undo to bam23/rev342. Native100 speed.000355877 at(144.894974,1.028673,-44.659241),
heading29.290298. Fixture configured/object absent; metadata start
(144.3743896484375,1.028671145439148,-64.19857788085938), rail
(144.3743896484375,-.00000476837158203125,-56.198577880859375), scale1.
Source c048914/DLL7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
game85401/launcher85410 unchanged. Both peers received explicit clean handback;
no job/reservation remains. Every reset/trial slot above is consumed.

All nine bindings now have a narrowly scoped production executor, but broader
source contexts/arbitrary repeated changes and live production animation,
HUD/balance/combo, Ollie/early-flip acceptance remain incomplete. In particular,
Smith's transient physical-contact loss may cancel its conservative executor
and needs production testing. No new DLL/CI build, installation, merge or
release was performed; the user's full-feature build gate remains closed.

### Smith observation boundary and native Tipslide re-admission

An offline re-analysis qualifies the contact-loss wording above. At
4366.606578ms the Smith receipt's outer `allowed` read is0, but its later
`admission_sample` has `stable_reads=true`, physical contact1, the same403
actor and original segment, and actual26. The context orientation differs
between those reads, so the enclosing row is not an atomic native update.
This does **not** erase the observed0 or establish uninterrupted contact. It
also does not establish that a completed physics step lost contact. The
production reader still rejects a missing contact sample; no grace interval,
contact substitution or cancellation relaxation has been introduced.

Static tracing separates scratch computation from publication. The original
0x47ed610 routine clears destination+992 at0x47ed636 and sets it at0x47edc55
only after a physical candidate succeeds. All three direct callers at
0x47ef1b3,0x47ef865,0x47ef9f6 pass stack scratch at rsp+736 as destination,
not context+3024. Therefore that clear/set pair is not evidence of a visible
per-frame clear of context+4016. The independently traced publication at
0x4801e72 passes updater+96 to0x47e8120, which copies source+992 into
context+4016 at0x47e839c..83a3. Its copy routine does not first clear that
byte. Further measurement must distinguish a completed updater result from
interleaved observation before changing production cancellation policy.

The same offline pass identifies the specific obstacle to repeating a change
after a named slide. Retained positive receipts contain the following stable,
same-segment, physically contacting native402 destination samples:113
Tailslide22,104 Noseslide21,82 Nosebluntslide30 and89 Bluntslide31. **All388**
have actor+766=0 and published admission+419=0. They cannot authorize another
native406 merely because the preceding named transition completed.

In particular, Tailslide's measured neutral withdrawal retains raw requested
heading -1.57079637 and pitch -.34906590 radians, rather than requesting a
straight, flat board. Original heading asset18531/RES7410 explains this:
its inactive-input branch at PCs0298..039c selects +pi/2 or -pi/2 from a
native vector-dot sign. Active input instead evaluates the heading curve and
adds BaseHeading atPC0274. The original schema's Tipslide heading curve has
knots(0,0),(180,180); its heading-to-pitch curve has(0,36.0066),(90,20).
Those are schema defaults, not a live tuning readback or a substitute for
the complete curve interpolation.

The402 and404 vtable+248 predicates both resolve to0x481da30, which invokes
0x481db80 with both direction enables true. That predicate tests distance
from the corresponding +pi/2 or -pi/2 heading against
asin(config+216)+35degrees. The common orientation helper admits a change
only after this predicate becomes false and the existing elapsed-time guard
passes. Separately, selector0x48140e0's402/404 branch at0x48141d3 selects403
outside its lateral heading band; it does not directly select401. Therefore
copying the5-0 neutral-to50-50 recipe to a slide would be an unsupported
implementation. The next bounded repeated-change probe needs actual active
heading movement outside the native lateral band, independently observed
admission, original402→403 completion and a measured named destination on
the same segment. Returning to50-50 would require another verified native
leg. No destination identity or named recipe is inferred from selection alone.

`analyze_grind_readmission.py` in this directory reproduces the contact-read
disagreement and all388 closed-admission samples from five SHA-256-pinned
receipts. It is strictly offline and accepts only the local receipt root;
it has no process, controller or console operations. Existing production and
test results remain at88c59dd; no new gameplay check is claimed. No hardware
window was requested or used, and no DLL/CI build, install, merge or release
was performed in this continuation. Broader execution and live acceptance
remain open.

### Compose the measured slide and Smith routes from established50-50

The existing source401/fine20 executor previously rejected Noseslide,
Nosebluntslide, Bluntslide, Tailslide and Smith requests even though retained
native receipts already establish the required original401→403 preparation
and subsequent named execution on one12m segment. This is a missing production
sequencing path, separate from the unresolved402 re-admission mechanism above.
`GrindChange` now composes those measured paths from established401/fine20 on
horizontal geometry1 of at least12m. It retains the existing nine mappings,
simultaneous priorities and first-grind actor.

Y→X and orderedB→Y first run the original Nosegrind23 preparation; B→X,
orderedX→Y and X→X first run the original5-0 identity24 preparation. The first
lease still has its1500ms bound and ordinary native admission. Original
401→403 completion and independently read intermediate name, contact and
original segment must both be present before input changes to owned neutral.
The preparation is explicitly separate evidence: it cannot set the final
requested-name completion, physics or animation flags.

The lateral routes then require retirement of the first native lease and an
unchanged intermediate actor/contact/segment before arming a fresh1500ms lease
with the existing raw-control admission guard. The final native sequence is
unchanged: neutral50ms then the qualified lateral controls, including the
Tailslide1000ms pulse and neutral withdrawal. The final403→402 completion,
actual requested name on the same segment and independently matching changed
animation remain necessary. Retained supporting receipts are the Nosegrind→
Noseslide and Nosebluntslide receipts,5-0→Bluntslide lateralv4, and the
Tailslide withdrawal receipt already recorded above; no new native receipt is
claimed for this integrated production sequencing.

Smith instead waits for original lease retirement **and** the existing native
feature-off/neutral-control reader. It starts the previously qualified2200ms
original403 control sequence without arming a second406. Its final evidence
remains `control_only=true`, `completed=false`; the real first406 completion
is recorded only as preparation. Intermediate Suski71 cannot satisfy Smith.
The positive Smith reference receipt above supplies the native route evidence,
including its unresolved mixed-time contact observation.

Continuation preserves the original physical pair's first/last timestamps and
request start. It does not submit another input request, synthesize an event,
re-date an old press or bypass the recognizer's300ms rule. Each execution leg
has a distinct serial and timing origin. A150ms handoff bound prevents stale
preparation from waiting indefinitely; the preparation's original1500ms
deadline also continues to apply until handoff. The composed maximum is3000ms
for a lateral route or3700ms for Smith. A live prior lease, unknown native
controls, wrong intermediate identity, replacement actor, stale frame,
owner/contact/segment loss, A, OFF or conflicting authored/action ownership
prevents or cancels continuation. Native arm failure withdraws controls.

The only shared-runtime edit is the named executor's `advance` call after its
independent verification and before sampling its RS output. It uses the current
owner/contact and existing native `arm`; it retains no callback beyond that
poll. Native406 lifetime, ordinary input routing, balance/HUD/combo, manual,
flip, prewind and camera implementations are unchanged.

Strict C++20 ASan/UBSan production-composed checks pass22480 cases at30/60/120Hz,
including all four ordered slide pairs, their supported simultaneous aliases,
Smith's ordered double press, original timestamps, retired-lease handoff,
wrong/absent intermediate evidence, held/repeated polls, final independent
physics/animation and cancellation.651 native-lifetime and1362 rail-pop checks
pass;1736 combo trace checks pass with the existing portable warning profile.
The previous contextual-air baseline failure and extra-Wconversion combo-test
limitation remain; this is not full platform CI or gameplay acceptance.

No Deck input, console, installation, DLL/CI build, merge or release occurred.
Compact peer snapshots still report idle after the prior clean handback; no
hardware window is reserved. Historical identity/world pins remainrev342 and
must be revalidated. Broader sources, repeated changes out of native402, and
live production/Ollie/early-flip/HUD/combo acceptance remain open. The new paths
expand the usable initial50-50 requests; they do not establish build readiness.

### Active native402 heading opens re-admission on the same segment

The source-derived active-heading hypothesis above now has a native positive
receipt. `grind-native-slide-heading-c048914-v4/results/two-leg-lateral.json.gz`
has SHA-256 `93b8d400a057045c7beb30b22806fe439f00e94a2b01149f21c6859e92dff379`;
the immutable runner has SHA-256
`55a85b2df23f1489c68087b17775aba5fa92a798d1e56761806dfa1b6e5c1dd8`.
It records616 independent samples with both native physics and cached-step
clocks at30Hz. This is ArcadeOFF physics/control evidence, not integrated
production, animation, HUD, balance, combo or rail-Ollie acceptance.

The existing original401→403/fine24→402/fine31 preparation completed exactly
two original406 actors. The third conditioned phase required both original
completions, actual Bluntslide31 with physical/published contact on the original
12m segment, debugger exit0 and native feature-off. Only then did it send
native Down for200ms, evdev RS(32767,8192) for600ms, and neutral. Writer times
are3596.233,3796.310 and4396.309ms respectively. These are writer-relative
times; reader-relative times below use the independent reader's first sample.
There was no third feature lease, debugger override,406 initialization or
completion. The native state, contact, names and animation were never assigned.

From reader3838.651068 through4414.822863ms,47 stable native402/fine31 brackets
show both actor+766 and published output+419 admission1, physical and published
contact1, missing-segment0, actor4138022496 and original segment
`[3753801416743,0]`. Requested raw heading is0.1917705535888672 radians;
pitch ranges from-0.3494592607021332 to-0.4839797019958496 and roll remains0.
The measured endpoints are x144.7177734375,y0.5007421970367432,
z-44.58984375/-56.58984375. Thus the active native control moves requested
heading out of the lateral band and opens admission without losing the
measured segment. This is different from the388 neutral/held-slide brackets
with admission0 recorded above.

With no third406 admitted, original control motion eventually changes physical
state402→403 at4477.862ms. It publishes actual Salad25 at4503.722636ms;
18 stable same-segment Salad brackets continue through4716.618396ms, all with
admission0. It reaches a different short end segment at4743.287ms and exits701
at4834.951ms. Salad is not a requested destination, and this in-place motion
does not prove a named repeated-change executor. The next qualification should
use the measured active402 admission window for a bounded original402→406→403
leg and independently identify the resulting name/contact/segment. Do not
assign5-0,50-50 or an original completion based on the selector alone. A later
50-50 return would still need its own qualified native continuation.

The outer reader reports physical-contact0 at3505.072331,3564.751528,
3898.563757,4132.943389 and4465.858809ms. Each corresponding later stable
admission bracket reads contact1 on the same segment. Preserve both readings:
the sequential reader is not atomic and these data prove neither continuous
completed-frame contact nor a completed-frame loss. No production contact
cancellation guard is relaxed. A separate offline check of the existing Smith
receipt finds113 stable same-segment403/26 physical-contact brackets with
admission0; Smith neutral is not a qualified general return path either.

The preceding setup attempts are retained as failures, not gameplay evidence:

- `grind-native-slide-heading-c048914-v1` receipt SHA
  `b83c2c353cbc0666a62e1dc4d6faff086acc835edfd78480bdb284e9ebce6d6d`
  stopped on the worker's two-condition maximum before grind controls. Its
  three reader samples are all native100. The debugger detached, feature
  restored and five owned transactions were undone356→366. The corrected
  ephemeral worker permits exactly the reviewed three bounded wait types;
  the installed trusted worker is unchanged. Local preflight now executes
  its actual validator before any SSH or editor operation. The fixed schedule
  has a4.2s aggregate wait bound.23 inert validator/replay/geometry cases and
 23 detached-owner/contact/control cases pass; production tests were not rerun.
- `v2` receipt SHA
  `bbdc899cae3c33f96397bcf803614ff14ce29af5067cbb92c2748b8a1ae3a9ea`
  stopped before fixture/debugger/grind work on a missing exact initial console
  echo. Fresh capture confirmed Hub; reviewed Escape/grave recovery visibly
  opened the console and the open-console status reader confirmed gameplay at
  rev366. No blind retry or revision relaxation was used.
- `v3` receipt SHA
  `00533c0a9b83be72e0ce3bab2456a559ee18295d9345b6ea476001743661e767`
  used the corrected schedule with unit-create/native-move scale3 setup. Its
 270 samples never entered400–406; the first native-geometry wait timed out
  and withheld all grind-change controls. The18m native endpoints were never
  measured, so that setup is unqualified. Cleanup restored absence atrev386.
  The successfulv4 reverted to the already verified scale2/12m setup, with
  its original geometry guards. No production editor change was made.

The new offline `analyze_grind_slide_controls.py` verifies all four pinned
receipts, exact preparation count, third-phase delivery timing,47 admission
brackets,18 later Salad brackets, the mixed-time contact observations and
cleanup revisions. It has no process, controller or console access. The
source-first native402 control mechanism is now measured, but no new production
recipe or requested-name acknowledgment is justified yet. Production stays at
7de1f23 and its previously recorded22480 composed checks; the known baseline
contextual-air assertion remains. No DLL/CI build, install, merge or release.

Both peers confirmed the exclusive window and received explicit clean handback.
All four probe slots and both new approach-reset slots are consumed. The reset
`grind-native-approach-reset-slide-heading-c048914-v2/reset.py`, SHA
`88c69be762481ac6fb25470a09e9fb21597919c4a2022f397ab6a54c4d937c32`,
used the original ground-checked trainer reset386→390; its receipt is
`results/1793554960130688000-status.json`. The final probe cleanup ended at
BAM generation23/revision410, native100 speed0.000243253, position
144.29736328125,1.028672695159912,-42.73271179199219, heading338.749237.
Installed identity remains c048914/PID85401/launcher85410 and DLL
`7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28`.
ArcadeON, menus/editorclosed, pad destroyed, debugger exit0/feature restored,
owned edits undone, fixture configured but object absent. Latest metadata start
is144.71788024902344,1.028671145439148,-63.59046173095703; rail position
144.71788024902344,-4.76837158203125e-06,-55.59046173095703, identity rotation,
scale1. No Grind job or reservation remains. These are historical pins and must
be revalidated with both peers before another trial.

### Original402 return completes to5-0; implement the narrow repeated change

Positive receipt `grind-native-slide-third-c048914-v2/results/two-leg-lateral.json.gz`
has SHA-256`4dd8538d0c8b4feab2a56a16f3ce1c18f712d739d32e0ffb458cc0dd75bd1c2d`;
immutable runner SHA is
`1cb5a921fa725e55082ebef0a92498db261cd270d62bfad82ced7035490223d9`.
Its614 independent samples have both clocks30Hz. Exactly three original406
actors complete401→403,403→402,402→403, with actual native names
50-50→5-0→Bluntslide→5-0 on the same measured12m segment. This is ArcadeOFF
physics/control evidence, not production animation/HUD/balance/combo/Ollie acceptance.

After two original completions and actual402/31 contact, third controls are
Down200ms, native XInput RS(32767,-8192)600ms, then neutral; writer-relative times
3605.187/3805.281/4405.393ms. The third lease remains disabled until actual raw
GrindInputs show positive wrapped heading below35degrees, pitch between-.5 and
-.1745329201, near-zero roll, and native actor+766/output+419 admission1.
Original native admission, destination selection, initialization, lifetime and
completion remain authoritative. No identity/animation/contact/target/completion
field is assigned.

Reader eligibility occurs3846.534879/3860.917245ms with raw heading
0.19176197052001953,pitch-0.34948572516441345,roll0 and physical/published contact1.
Third406 first appears3872.221ms. Original402→403 reaches phase1; cleanup records
elapsed0.4999999701976776s. Physical403 appears4380.144ms with old name31, which
is not accepted. Actual24 appears4405.680162ms through4627.771902ms:19 outer
contact samples,18 stable admission brackets, all on segment`[4045859192871,0]`,
endpoints x144.9501953125,y0.5007421970367432,z-44.1923828125/-56.1923828125.
Stable destination actor4137964656; natural701 at4639.043ms. No fourth native
leg is claimed. Outer contact0 at3334.823785ms during earlier preparation has a
later stable contact1 bracket: preserve both sequential observations, without
claiming uninterrupted completed-frame contact or a completed-frame loss.

Production now admits ordered X→B from established402/31 on horizontal geometry1
at least12m, using the measured controls and fresh1500ms lease. Both original
admission bytes and raw controls are bracketed by existing owner/actor/contact
validation. Initial arm additionally brackets the feature pointer/vtable/byte26
and requires feature-off with valid402 raw controls. The existing control
reader's default remains403-only; only this new arm requests402 explicitly.
Ordinary and prior lateral source policies are unchanged. Original
402→403 completion, independent same-segment24 and independently matching changed
animation are required. Early acknowledgment cannot skip neutral withdrawal.
Owner/actor/contact/segment loss, stale frames, incompatible state, A, OFF/menu
or conflicting authored ownership cancels. No other402 identity is qualified.
After actual5-0 re-establishes, a new Y→Y enters the existing native return to
50-50. That further leg after this new return has not been measured live.

A composed negative exposed a tie-priority dependency: excluding current
Bluntslide made simultaneous BX skip its preferred entry and fall through to
ordered X→B5-0. The recognizer now consumes only that excluded same-timestamp
Bluntslide tie without enqueueing. Distinct timestamps, other exclusions,
first-grind/early recognition and lip tables remain unchanged. Original physical
timestamps and300ms window remain intact. No HUD/balance/combo, manual, flip,
ground Ollie, push, revert, prewind or camera implementation changed.

Strict C++20 ASan/UBSan composed tests pass23526 cases at30/60/120Hz, including
native source/admission policies, stale completion, independent identity/animation,
held/repeated polls, cancellation, tie priority and another fresh request after
the return.651 lifetime and1362 rail-pop checks pass;1736 combo cases pass with
the existing portable warning profile.7010 production HUD publication/pending/
draw checks pass with the original ImGui sources linked. The contextual-air press-age assertion
still fails on input_pattern_dispatch, as on baselinec9c0581f; never report full
green checks. Windows/backend compilation remains deferred by the build gate.
Integrated production acceptance, broader sources and arbitrary repeated changes
remain incomplete.

The preceding third-legv1 is a retained failure: receipt SHA
`aca75b3a2372e48d7936d180ee3e08a4c1ff22043eff154aaaca80adad9212d0`,
runner SHA`cb1c7b3c976c1e32b89d9c13f0ffa77316eb9c4e52185cecfab00714709b4bf6`.
Its296 samples verify only two completions. Phase7's state guard rejected before
third controls but omitted the offending selector value; adjacent reader/GDB
samples show402/31. Its exact cause is unknown. V2 adds a bounded read-only
diagnostic before the unchanged guard, with no ownership/state relaxation.
Seven readiness samples all show402/31, physical/published contact1, feature-off.
V1 partial evidence confirms pad neutralization, GDB exit0/restored feature and
owned undo414→434.68 inert worker/lease cases pass. The new offline pinned
`analyze_grind_slide_return.py` verifies both receipts without hardware access.

Both peers confirmed the exclusive window and received clean handback. All trial
and reset slots are consumed. Reset-slide-thirdv1 SHA
`cb9b10832cb86149d73b4f6e0a48d8cd3a254a9a2b8e2bafcce8199127bae19b`
used the original ground-checked reset410→414; v2 SHA
`e7e5157b88dec519c60cf23f56932acc26b81d5427c5f55373f896e4fac40d80`
reset434→438, receipt`results/1793558318035946000-status.json`. These are setup,
not rail-exit evidence. Final five owned undos end BAM23/rev458, ArcadeON,
menus/editorclosed, fixture configured/objectabsent, pad destroyed, GDB0/feature
restored. Installed source remainsc048914/PID85401/launcher85410, DLL
`7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28`.
Native100 speed0.000064281, position144.82432556152344,1.0286675691604614,
-41.084808349609375, heading356.55670166015625. Metadata start
144.95022583007812,1.0286712646484375,-63.19279479980469; rail
144.95022583007812,-4.887580871582031e-06,-55.19279479980469, identityrotation,
scale1. No Grind job/reservation remains. Recoordinate and revalidate all pins
before further input. No DLL/CI build, upload, install, merge or release.

### Compose Bluntslide to50-50 from the two measured return mechanisms

Source review at13efbdd found a missing sequencing path: Y→Y was unsupported
from402/31 although402/31→403/24 and403/24→401/20 have independent native
receipts above. The official/main search still exposes only the trainer's
generic grind-control entry, not an equivalent named continuation executor.
This change reuses `GrindChange::request/verify/advance`, original admission,
the existing two control recipes and their native completion readers.

Offline re-analysis of the pinned third-legv2 receipt additionally finds13
stable destination403/24 brackets from4469.840734 through4627.771902ms with
actor+766/output+419 admission1, same actor/segment, physical/published contact1,
and requested pitch/roll0. That establishes a native neutral re-admission
window after the measured Bluntslide return. It does not establish a fourth
native completion. The independent return2-v4 receipt establishes the existing
403/24→401/20 neutral leg; composing those legs is an implementation candidate,
not a new native acceptance receipt.

Y→Y from established402/31 on horizontal geometry1 at least12m now starts the
existing guarded5-0 preparation. The full Down200ms/diagonal600ms trajectory
remains intact even if actual5-0 publishes early. Preparation readiness cannot
be latched before800ms; then original402→403 completion and fresh same-segment24
are required. Its first1500ms deadline still applies. Within the existing150ms
handoff bound, the original lease must retire and the native403 control reader
must observe feature-off and neutral pitch/roll before arming the existing
ordinary403→401 return. A still-held physical RS prevents the neutral handoff
through the existing runtime control publication.

The second lease has a distinct serial/time origin and1500ms bound; original
physical Y press times, recognition serial and request age are retained. No
new input is submitted or re-dated. Preparation completion/identity cannot
acknowledge the final requested50-50. Final acknowledgment requires this
request's original403→401 completion, actual same-segment401/20 and matching
changed animation. Wrong/replaced intermediate actor, owner/contact/segment
loss, stale frame, arm failure, nonneutral controls or a live prior lease
prevent continuation. No other402 source or target is added. Earlier401
preparations, direct5-0 return, Smith controls, simultaneous priorities and
the protected HUD/balance/combo/flip/manual systems are unchanged.

25118 strict C++20 ASan/UBSan production-composed checks pass at30/60/120Hz.
They include the full pulse despite early identity, separately scoped original
completions, native-neutral and lease-retirement waits, preserved timestamps,
no held-input replay, final identity versus animation and invalid handoffs.
The pinned offline receipt analyzer also passes with the13 neutral-admission
brackets added. These checks are not a fourth native transition, integrated
production acceptance or a DLL build. Previously documented baseline test
limitations remain. No hardware call, reservation, build/CI, upload, install,
merge or release occurred in this continuation; historical world pins remain
rev458 and require new peer coordination and validation.

Host storage interrupted automatic approval review twice with `No space left
on device` while opening its thread-writer lock. This was a review-service
initialization failure, not a policy rejection. Removing only this worktree's
rebuildable old test executables allowed the same reviewed patches and focused
checks to proceed. Source, native receipts and logs were retained. The host
still reported only134MiB available on its926GiB volume; further host cleanup
is needed before sustained captures/platform builds. No unrelated files were
removed and no approval check was bypassed.

### Compose5-0 to Nosegrind/Crooked through independently verified50-50

At e992b99, established403/fine24 accepted50-50, Bluntslide, Tailslide and
Smith requests, but rejected Nosegrind and Crooked. Current source review
found no native mechanism missing for those two routes: the measured
403/24->406->401/20 neutral return and401/20->406->403/23 or32 sweeps already
have production executors and separate native receipts. The complete local
official/main search still has no equivalent named-transition implementation;
the pinned fork survey and original control contracts above remain applicable.
This is a missing sequence, not evidence that arbitrary403/402 identities can
use the same neutral return.

The new routes accept only403/fine24 on horizontal geometry1 at least12m.
They retain the existing first1500ms neutral return and require its original
403->401 completion plus actual401/fine20 on the original segment. They own
neutral until the original lease retires, within the existing150ms handoff
bound. The original arm then revalidates401/20 and starts a distinct1500ms
sweep lease: heldUp, followed after200ms byRX12000 for Nosegrind23 orRX24000
for Crooked32. The measured sweep owns input through1000ms. The403-only
control reader is not generalized or used to pretend401 has403 controls.

Preparation verification now compares against its configured destination
state rather than a hardcoded403; every previous preparation still configures
403. Original physical first/last press timestamps and request serial remain
unchanged, and the final leg has a distinct serial/time origin. The intermediate
50-50 completion/identity/animation cannot acknowledge the requested name.
Only the final original401->403 completion, same-segment23/32 and independently
matching changed animation can do so. Invalid intermediate identity, missing
completion, changed actor/segment/contact, stale frame, still-live lease and
native arm failure prevent continuation. Other unqualified source contexts
remain rejected; no identity, animation, contact, completion or score is written.

27651 strict C++20 ASan/UBSan production-composed checks pass at30/60/120Hz.
The added cases exercise both physical pairs, distinct native completions,
lease retirement, exact controls, original timestamps, held/repeated polls,
independent final animation and invalid handoffs. This composes individually
measured native legs; it is not a newly observed full sequence or integrated
production acceptance. Existing baseline test failures remain as documented.
No Windows/backend build, CI, upload, installation, merge or release occurred.
No hardware calls or reservation were made; historical Deck pins remainrev458
and require fresh coordination. Host storage remained low at128MiB before
the focused edits/checks; source and native receipts were preserved.

### Native Nosegrind neutral return and production Y->Y integration

Source-first review reused the existing original403 selector at0x48142a5,
its neutral heading/pitch branch0x48142f0..0x481432e, the original406
admission/initialization/update and the existing5-0 neutral-return executor.
No identity or animation setter is introduced. Re-analysis of the pinned
nose-lateralv2 receipt found seven stable403/23 same-segment contact brackets
at2658.839163..2735.068375ms with actor+766/output+419 both1, requestedpitch/
roll0 and heading0.3120996..0.3121155. Five preceding held-pitch brackets had
admission0. That justified a return experiment, but was not return acceptance:
that old receipt completed403->402. The new offline
`analyze_grind_nose_return_candidate.py` retains that distinction and checks
both separately pinned receipts.

Both Manual and Grab confirmed handback before any Deck access. Fresh capture
showed Steam idle artwork; the reviewed single-Shift wake restored gameplay.
Fresh status independently confirmed installedc048914, DLL7853ae1a2dc11de526bb8cb4f19f6ec7e57a8b4913554f868f4525dac768df28,
game85401/launcher85410, BAM23/rev458. The newly pinned original native approach
reset restored only its exact owned fixture, used the native ground-checked
reset, and undid that single placement458->460->462. Reset runner SHA
`b2a50d961d706d058512d5e2cbe1f7a6d6d7d7454a010a4ff5076fd82d3679de`, receipt
`grind-native-approach-reset-nose-return-c048914-v1/results/1793609762984800000-status.json`.
This reset is setup only, not rail-exit evidence.

The Nosegrind-returnv1 attempt stopped locally before SSH because its required
2MiB receipt slot had not been preallocated. It sent no Deck input. The fresh
v2 slot contains the identical immutable runner SHA
`9920b5dace1ce2c752c36b21c190718ebecac618126e05f93f172a035adc1b1b` and a reserved
receipt. Ten inert gate cases and the actual ephemeral worker phase validator
passed before execution. All these trial/reset slots are now consumed.

Positive receipt:
`grind-native-nose-return-c048914-v2/results/two-leg-lateral.json.gz`, SHA256
`df44e2faebdcb3991bd93f33eec18c96ea56bb9a263f83bf77ba0d9433a26853`.
504 independent reader samples, both native dt readbacks0.0333333313s.
Original401->403 completed, actualNosegrind23 appeared2576.935ms, and the
conditional neutral phase was delivered2658.692ms on the writer clock after
original completion/actual23/contact/segment verification. The second original
406 appeared2675.484ms on the reader clock; original403->401 reached phase1
with native elapsed0.9333329201s. First401/old23 at3608.306ms is not acceptance.
Actual401/20 from3643.259224 through4561.950531ms has77 outer and77 stable
bracketed destination samples, physical/published contact1, missing0 and the
original segment[4191888080935,0]. Endpointsx145.05859375,y0.5007421970,
z-43.9931640625/-55.9931640625 measure12m. Gate reason
`second_native_identity_verified`, exactly two original completions.

An outer physicalcontact0 at3537.977005ms during406 has a later stable bracket
reading1 on that same segment. Preserve both: these reads are not atomic and
do not prove uninterrupted physical contact or a completed-frame loss. No
production cancellation guard is relaxed. Later403/24 and402/22 near the rail
end are incidental, not requested transitions. Natural701 at4776.880ms is
not A-button Ollie acceptance. The trial was ArcadeOFF physics/control
qualification, not integrated production animation/HUD/balance/combo acceptance.

Production now admits Y->Y from403/fine23 as well as the already qualified
403/fine24 source, using the unchanged1500ms owned-neutral return and original
native lease. The existing horizontal12m restriction, owner/contact/segment
guards, original completion requirement and independent final401/20 animation
evidence remain. Crooked/Smith/other403 and402 sources are not generalized.
28158 strict C++20 ASan/UBSan composed cases pass30/60/120Hz, now exercising
both return sources, old identity versus final animation, held-input replay,
subsequent fresh requests and negative owner/A/OFF/expiry/geometry cases.
No native hooks, ordinary Ollie/flip, HUD/balance/combo or other protected
implementations changed. Existing baseline test limitations remain.

Cleanup completed normally: GDBexit0, nativefeature0, pad/session destroyed,
all five owned transactions undone to BAM23/rev482, ArcadeON, menus/editor
closed, fixture metadata configured/objectabsent. Finalnative100 speed
0.00008172 at151.1729736328125,1.0286728143692017,-42.391971588134766,
heading90.10433197021484. Metadata start145.05862426757812,1.0286712646484375,
-62.993980407714844; rail145.05862426757812,-0.00000476837158203125,
-54.993980407714844, identityrotation, scale1. Both peers received explicit
clean handback; no job/reservation remains. These are historical pins requiring
fresh revalidation. No DLL/CI, upload, installation, merge or release occurred.
