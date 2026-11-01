# Grind exit investigation and isolated routing candidate

Baseline: `65d34a0bc06034125b28779d10490e479df37fba`, the user-verified
`arcade-stage-controller-ownership` flip baseline. Work belongs only to
`arcade-stage-grind-transitions`. No installation, release, manual branch,
StateFlow/PTM flip implementation, balance model, or scoring changes.

**Status: incomplete gameplay feature.** The rail A routing defect is reproduced
and corrected offline. Neither a live pop on this revision nor grind-to-grind
execution is accepted. No named two-button combination is confirmed working.

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
