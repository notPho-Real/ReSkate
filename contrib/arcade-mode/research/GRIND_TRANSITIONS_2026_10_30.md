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
