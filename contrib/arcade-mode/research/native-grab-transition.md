# Native grab transition investigation — Stage A source pilot

## Current implementation checkpoint

The source now includes a native request adapter for BS Grab, Melon and Method.
This supersedes the historical investigation below; **native gameplay acceptance
is still incomplete**. BS Grab, Melon and Method now have exact native animation
evidence at measured 30 Hz. Method's repeated-input correction passed its native
trial. The be22b1a follow-up below observes native release at 30 Hz; full
scoring, capture completeness, body rotation and higher-rate acceptance remain open.
The user subsequently authorized coordinated Deck trials. Broad
catalog expansion and accepted chains remain gated on Stage A native tests.

### Physical repeated-B interval correction

Source review found a separate recognition defect: `UnifiedInput::poll` saved
the first grab's dispatch time, and `recognize_grab` used that time to open the
250 ms repeated-B window. A native admission delay therefore extended the
physical interval that could select Method. The existing request already
retains `physical_pressed_at`, and the original event matcher retains the
second edge's `last_at`; no new input producer or native mechanism is needed.
The repeat window now compares those physical edges. The presentation-only
`grab_table_at` remains the dispatch time. Flip timing and native transitions
are unchanged. Source-first reference: the recorded upstream/fork method
survey and the existing physical-history/queued-request implementations in
`arcade_input_dispatch.h`, `arcade_input_events.h` and `arcade_air_selection.h`.

A production-composed regression at 30/60/120 Hz delays the first native
submission by approximately 100 ms. A second physical press approximately
200 ms after the first still selects Method; one approximately 300 ms after
the first selects a fresh Melon. Both retain their actual physical timestamps
and the same airborne episode. This regression failed before the correction
and passes afterward. The grab suite now passes 494 checks. Input session
(326 + 5645), protected neutral flip (9014), air episode (29), grab hold (33)
and native grab wait (72) checks pass. These are source/contract results, not
new gameplay acceptance.

The broader `arcade_input_pattern_dispatch` test fails its contextual-table
300 ms press-age assertion. Recompiling it against the unchanged HEAD
`f372ddf169eae423443a3c1291c62a10a1de029d` header reproduces the same failure;
the ordinary-grab correction did not introduce it. Do not report the entire
local suite as green or silently alter unrelated contextual behavior.

The earlier missed Method trace remains a separate unresolved first-press
problem. Its capture 2 records input events 7/8 at 232018288 ms, followed by
action generation 2896 to 2897 at 232018303 ms. There is no corresponding
grab recognition. Events 7/10 form the later single-Melon recognition at
232018487 ms. The independent native samples place the first kernel B edge
between observations of state 200 and 201. That capture has no current source7
controller rows, so it does not establish the precise admission gate or permit
equating Windows and Linux clocks. This timing correction is not claimed to
fix that separate symptom.

The subsequent deployment-only window is recorded in
[the storage-blocked receipt](native-grab-be22b1a-storage-blocked-window.json).
No gameplay trial ran in that window. The Deck was explicitly released;
its installed identity and world must be freshly qualified before reuse.

### be22b1a coordinated follow-up

Five fresh-qualified 30 Hz trials on the installed and mapped be22b1a DLL
reached the exact original Basic BS, Method and Melon actors. Each initialized
with duration305 and advanced by a positive native delta. The Basic100ms tap
released B before actor initialization and still reached its original actor.
All five subsequently observed `Bool.State.Is.GrabExit=true`, cleared native
right-hand and `IsGrabbing` flags, and returned to ground100. A brief state0
sample after Method's first ground contact is retained in the evidence.

Both held shoulder directions delivered native LS +/-31783 alongside RT255
and Basic Grab actor advancement. Releasing the shoulder restored LS while B
kept RT; releasing B then cleared RT. This establishes input/animation
coexistence, not independent angular body-motion acceptance.

The first Method attempt at750/950ms after A press acknowledged only Melon.
A subsequent900/1100ms schedule acknowledged Method in the same episode.
This is evidence for investigating takeoff eligibility, not a production
delay requirement or a complete explanation of the missed first press.

Observer corrections matter: angular-on is accepted only for mask39, which
excludes controller-poll source7; `127 on` is rejected. The corrected trials
used supported127 without angular-on and admitted only a new capture's
controller rows. Native sample/step cadence measured32--33.5ms, with both
physics and cached dt=0.03333333. No Windows/Linux clock equivalence was assumed.
These five captures drained every expected row without new drops, but their
footers remain incomplete: required manual sources4/5 did not occur. Existing
scope103 includes the relevant sources1/2/3/6/7 with bounded selected-poll
compaction and is the next observer configuration to qualify. No footer has
been rewritten as complete. Native scoring remains owner-unverified.

See [the full follow-up receipt](native-grab-be22b1a-followup.json), including
raw evidence digests, physical/returned packets, exact actor identities,
native release transitions, timing and limitations. The earlier failed
capture setup is retained in [the first-window receipt](native-grab-be22b1a-window.json).
The Deck was cleaned up and explicitly released to the manual task for its
next installation; no DLL/world/rate/layout/fixture change occurred in the
second window.

### Trigger-only grab ownership and held shoulder rotation

The native grab adapter sets `ActionDispatch::Output::owns_triggers`, which
contributes to `ActionInputOutput::owned`. The final controller composition
previously passed that aggregate flag to both `AirRotation::poll_qualified`
and the angular permission gate. Consequently a held native grab suppressed
LB/RB rotation even though its adapter reserved neither stick. This is a
source-confirmed routing defect, not evidence that native grabs prohibit spin.

The existing native LS shoulder actuator is reused. A current-poll
`native_grab_triggers` receipt is set only when the qualified grab lease delivers
right-trigger input. `blocks_air_rotation` allows that trigger-only receipt to
coexist with one held shoulder in the existing held-spin mode. Other action
owners remain exclusive. Missing trigger ownership, either reserved stick,
reserved face buttons, or a different trigger pair cannot use the exception.
The AIR spin mode is sampled once for its ownership decision and actuator so a
concurrent setting change cannot turn this exception into a tap body-rate command.
The native airborne proof, freshness, spin prohibition, reference owner and
board-transition gates still apply. Classic `consume_rt` is an input mask, not
an action owner; the qualified grab output still supplies RT after that mask.

Prewind, ground/manual/grind controls, the flip transition bridge, native grab
selection/hold/release, animation assets and physics are unchanged. This does
not enable tap-spin or direction-only custom angular motion during a grab.
It adds no action owner, pop, animation or native state write.

Source-first comparison used the recorded full upstream/fork survey and the
pinned GoalTrains `src/translate.h` native LS/trigger separation already cited
by `arcade_rotation.h`. SessionSticks 0.6.1 likewise separates stick movement
from trigger grabs; neither source supplies native acceptance for this adapter.

The production-composed grab suite now passes 470 checks at 30/60/120 Hz.
It exercises both shoulders with a real recognized Melon request and the
existing native input/rotation classes, retaining RT while delivering native
LS, preserving the airborne episode and issuing no custom angular body rate.
It covers shoulder release, opposed shoulders, spin prohibition, stale AIR,
owner replacement, landing, OFF, reference ownership and incompatible channels.
The native callback/animation evidence in these tests is a contract fixture,
not a live observation. Existing input (326+5645), neutral flip (9014), flight
(2749), spin (323), dispatch (124) and control-state (67042) checks also pass.
The lab suite passes 458 tests with one skip.
Windows CI on the first correction found missing ownership-variable bindings in
the generated controller-publication fixture. Its CMake extraction now includes
the exact production ownership statements. The locally compiled composition
passes 216 native-publication checks plus its existing routing, packet and
binding checks; both shoulders and competing-owner negatives are included.

Before gameplay acceptance, capture each shoulder during the same original
BS/Melon/Method actor episode, independently observe actor advancement, body
yaw, uninterrupted flight and release/landing, and compare against the same
grab without a shoulder. Verify that releasing the shoulder stops LS delivery
while B keeps the grab active; test opposed shoulders and native ineligibility.
The broader Stage A release/scoring and rate gates remain open. No new Deck
trial was claimed at source-change time. The follow-up above adds bounded
input/animation coexistence evidence while preserving those acceptance limits.

### Method native acceptance on c69dd24

Commit `c69dd24af196e2a43eb3b290ad92232d132b45d2`, DLL
`6cf3320e335d499106eff9473a981748dd767b401d9fbbad8cc9400359cce8bc`,
passed Windows (97/97), Linux and macOS CI run 41164301643. The DLL's installed
and loaded mapping identities were verified before the coordinated trial.
Capture SHA-256: `e8bf5b2a557e53ac9071c17d1d420389a2a7c8c98cd6f9f9b8b1a32f459a431d`.

The identical physical double-B schedule now admits Method with
`native_owner=true`, despite unchanged failed flip proofs 80. Serial 2 stays
in airborne episode 1. Original Method actor
`12d1f15d-c040-6fd0-12d1-f15dc0406fd0` initializes with native duration 305,
then advances 0 to 1.8027897 ticks at positive delta 1.8027897. Native detailed
identity 2 agrees with IsGrabbing. Independent sampling reports 486 samples,
no read errors or log gaps, measured and cached step 0.033333331 seconds, and
100→103→200→201→100 without another pop. The earlier Melon request is superseded
before its actor initializes; it is not counted as an executed Melon in this trial.

B release withdraws returned RT and the request ends acknowledged while airborne.
However, native hand/grab state remains active through exit, and restoring the
tweak request to neutral precedes a brief native BS detail tag. The HUD records
Method followed by BS Grab and a Sketchy landing. This may be the still-active
native blendspace returning to neutral during exit; comparison with original
release semantics is required before changing it. Do not claim clean named-only
release or full scoring acceptance. Native score 58/landing factor 0.9 is
supporting evidence only: the existing score observer remains owner-unverified.

See `native-grab-c69dd24-acceptance.json` for sanitized request, actor, timing and
release evidence. The capture footer still lacks complete source coverage.
BS and Melon were subsequently rerun on this same DLL. Both original actors
initialized with duration 305 and advanced from 0 to 0.20186043 ticks, with
matching native detail and IsGrabbing. Basic has 475 independent samples and
Melon 474; both measured/cached steps are 30 Hz with no read errors or log gaps.
Basic returned 201→100; Melon briefly reported state 0 between 201 and 100.
That transient is retained in the evidence and is not treated as a clean scored
landing. See `native-grab-c69dd24-regressions.json`.

Two original-control comparisons (Arcade OFF, native stick/trigger inputs)
reached native Method detail, but both bailed. One retained native grabbing after
RT release while the tweak was held. The animation observer is inactive in those
controls, so a detail tag is not counted as exact actor proof. These comparisons
do not settle release causality or justify forcing native cleanup. No release
code was changed in response.

60/120 Hz, full hold/release, scoring and chain acceptance remain outstanding.
The Deck was returned neutral with capture stopped,
Arcade ON and automatic routing restored; the prior manual DLL was preserved.

### Native grab-owner continuity correction

The `c8f7971eb2f53fe06f8fa152446c5bc7dd75b1b2` diagnostic DLL
(`09942bc8c76ed4a9ed4145cc7735f15cd011954274ced9deb2c3742b7b2a5f6b`)
was verified loaded after CI run 41162131670 passed all three platforms,
including 97 Windows tests. Its Method capture SHA-256 is
`4afe121d80901bc3b198888d1d56d41dd54543bb5b92f2ca580b19142cbd4ca1`.
The second physical B selected semantic Method (29), direction 4, presses 2.
It was cancelled before the native adapter (`gate=0`, `disposition=3`).
`failed_proofs=80` identifies unavailable flip selection and its absent object.
The native observer explicitly reported `flip_known=false`; its fallback ID -1
was **not** an observed idle selection. Previous source fixtures incorrectly
supplied a valid -1 selection, explaining their passing result.

The independent native sampler has 479 samples, no errors or log gaps, measured
and cached 30 Hz, and flight 100→103→200→201→100. At the sample 5.46 ms before
the second kernel B edge, native IsGrabbing, CanGrab, RightHandGrab and InAirState
were true; LeftHandGrab was false and EnumGS.GrabType was BS (2). DetailedGrabType
was not allocated yet. Kernel timestamps join this observation to the physical
edge; Wine input-log milliseconds are a separate clock and are not substituted.
Only Melon initialized and advanced; Method still has no native acknowledgment.

Grab repeats now use the active grab adapter's exact original request lineage,
local owner/state block and airborne episode, live input lease, and those native
BS-grab signals. The dispatch boundary rechecks the native grab state when the
lineage changed. This selects another original pose within the same BS blendspace;
it does not admit another family, write a grab identity, force a transition or
alter a clock. Flip selection/retirement remains diagnostic only for grabs.
The flip queue and native flip bridge are unchanged.

The existing IsGrabbing/hand assets are joined by read-only assets
`Animation/Dingo/EnumGS.GrabType` (`387fb01d-9950-ce5d-387f-b01d9950ce5d`)
and `Animation/Dingo/Bool.GP.CanGrab` (`aa379a76-0ada-6e20-aa37-9a760ada6e20`).
Missing assets/state, wrong family or hand, failed eligibility, another request,
owner or airborne episode cannot grant continuation. The capture row's new
`native_owner` field records this independent proof; `retired` and `failed_proofs`
still describe the old flip predicate for comparison and never grant admission.

The 305 source checks include the observed invalid-selection case at 30/60/120 Hz
and each native-proof veto. They are not native animation acceptance. Next is the
same bounded Method trial against the corrected DLL, followed by hold/release and
regression acceptance if its exact native animation advances.

### Repeat correction hardware result

Commit `dd3e8ebaac4a99102e26296b9c6c373744b28660`, DLL SHA-256
`390e1db09244a62ae8268f1d3f132655fb638a5a05967f28adfda8a2784f7918`,
passed Windows/Linux/macOS CI (run 41158925576) and was verified loaded before
the next Method trial. The capture SHA-256 is
`df8f53fddc2d82783358b09ba7f882bcd9f32e2ac5e07b661d513f1a0b29e333`.
It contains 474 independent samples at measured/cached 30 Hz, with no sampler
errors or reported log gaps. The overall correlation footer is incomplete;
do not treat this as complete coverage of every requested evidence source.

The two physical B presses were recognized. Only Melon reached the native
adapter: its exact actor initialized and advanced by 1.8027897 ticks in the
same airborne episode. No Method request or Method animation was acknowledged.
Native flight returned to state 100. Numeric scoring remains unverified.
The inherited action generation was a reproduced source defect, but correcting
it did not resolve the live failure. Its sufficiency is disproved by this trial.

An offline replay of the captured physical polls, native state/selection IDs,
physics cadence and action-readiness delay reaches Method in production input
code. Selection-object lifetime and mode-generation provenance were not in the
capture; the replay supplies those and cannot prove live equivalence. The next
build adds capture-only `ARCADE GRAB INPUT` records without changing admission:
requested semantic identity, request/origin/current action generations, repeat
qualification, queue disposition, and the adapter's competing-owner gate.
No native address is logged. Records use the existing bounded grab event queue.

`failed_proofs` bits describe the existing retirement predicate: 1 valid AIR,
2 same episode, 4 same owner, 8 exactly one action change, 16 valid selections,
32 Ollie-to-idle IDs, 64 same nonzero selection object, 128 state 201,
256 mode 2, 512 same stance, 1024 advancing physics step, 2048 same nonzero
mode generation. These are failed conditions, not execution permissions.
`gate` is 0 before the adapter, otherwise entered=1, unavailable=2,
authored owner=4, manual owner=8, auxiliary owner=16, rail owner=32,
extension form=64. `disposition` uses InputDisposition (waiting=0, started=1,
unsupported=2, cancelled=3, expired=4). Neither row acknowledges animation.

Source validation now has 248 grab checks at 30/60/120 Hz, including a replaced
selection object that must remain rejected and observable. Stage A is incomplete.

The missing intent and observation definitions were recovered from the original
`dingolevel_root` shared bundle. The revised adapter delivers the original right
trigger through the existing bounded input output and writes the two inspected
native tweak request signals. The original input producer owns left/right hand
intent and eligibility. It leaves CanGrab, native grab
selection, detailed identity, animation clocks, blending, flight and scoring to
the game. The normal entry edge is level EBX 19856; the earlier edge 27611 is a
separate held-flip entry and must not be used as the ordinary Ollie route.

Delivery is additive at the existing local-state-qualified candidate entry.
The original SceneOpMatrix places ControllerUpdate at index 7 and SkaterPhysics
at index 13. This establishes source ordering for the request to reach native
physics eligibility on the next evaluation; it is not measured gameplay timing.
The flip StateFlow/PTM transition code is unchanged apart from a grab-delivery
call and a veto while the native grab owner remains active.

The source pilot bindings are B = BS Grab, Left+B = Melon, and Left+double B =
Method. They retain physical press timestamps and the existing repeat window.
Only the inspected stance is admitted; switch and nollie remain unsupported by
this pilot. LB/RB remain available for airborne rotation. Other named grabs do
not alias these three animations.

A released tap retains its request until the exact original actor has been
initialized, advanced with positive delta, and agreed with the game's detailed
grab tag and IsGrabbing state. Held B retains the intent after that acknowledgment;
release withdraws it. Missing evidence expires after a bounded 1500 ms ceiling;
this is not a minimum hold duration. Owner/episode loss, missing input heartbeat,
mode-off, landing or legitimate native exit withdraw the request. Restore writes
are conditional on unchanged owner, storage address and last owned value.

Two fingerprinted read-only observation detours retain the original native calls:
ActorController initialize at RVA 0x1277480 and evaluation at 0x1278770. Initialize
receives the state view directly; evaluation receives a context whose +8 field is
that view. The adapter never calls initialize itself or changes an animation ID.
Physical request/end records are queued for client-thread logging. Initialization
and advancement records include serial, episode and actual actor identity.

Local validation: 206 production-composed grab source/contract checks at
30/60/120 Hz; input-session, neutral-flip-release, air-motion-proof and action
dispatch suites; 58,213 existing extracted manual lifecycle checks. Those tests
use explicit native observation fixtures and do not prove native animation,
landing, scoring or legal chains. The full lab suite initially caught synchronous
input-path logging; the adapter now queues that evidence and the focused telemetry
contract passes. The full lab rerun passed 458 tests with one skip. Current CI
and DLL identity must be taken from the build receipt. Grab delivery is enabled
only when the existing candidate-entry hook has also installed successfully.
Windows also exposed an expired directional-extra request falling through to
the newly enabled neutral B binding. Neutral fallback now applies only without
an active named-grab direction; expired named requests retain their original
buffering rule. All 67,042 control-state checks pass locally after the fix, and
that existing suite is now included in Linux/macOS CI too. The 206 grab checks,
production input-session checks and neutral-flip checks also pass after the fix.

See `native-grab-source-pilot.json` for exact original identities and asset
receipts. Required gameplay acceptance remains A→B, A→Left+B, A→Left+double B,
tap/hold/release, flight continuity, native landing/scoring, and negative controls
at measured 30/60/120 Hz. Coordinated Deck installation/testing is now authorized;
live-channel publication remains prohibited.

## First hardware trial and input-path correction

The exact `fbcc0e9ef1be8620fab4e2b2fe20adf68a4ebc32` DLL was installed and
its loaded file mapping verified after an exclusive handoff from the manual task.
The previous build, receipt, settings and log were preserved separately; the
original rollback was unchanged. The tested DLL SHA-256 was
`fbe516a134440e81af03dd557d4e7b9ae46951223b40950a1ee1958183b45f2c`.

At measured 30 Hz, A then Left+B produced semantic Melon, expected detailed
identity 4, in the original airborne episode. Neither native actor initialization
nor advancement acknowledged the request. An independent read-only sampler then
captured 488 samples: RightHandGrab and TweakFrontBack=1 were present together in
61 samples, but valid native CanGrab stayed false in all 488. GrabType stayed 0.
Flight rose and fell through native states 200/201 and returned to state 100.
IsGrabbing and DetailedGrabType storage remained unallocated; absence was recorded
as unknown, not a false or zero observation.

An original-game comparison with Arcade OFF used a native stick Ollie and RT.
CanGrab and RightHandGrab became true in 54 samples, GrabType became 2, and
DetailedGrabType became 1. The HUD showed BS Grab. This qualifies the input route
for the correction, not an independent clip-advancement acceptance of our pilot.
Arcade was restored ON after every comparison. A bounded hardware watchpoint
observed the CanGrab write in the existing Bool setter at RVA 0x11dd7a0;
the verified return address was 0x16d16f0 in expression interpreter 0x16d1190.
Generic debugger stack unwinding was invalid and was not used as caller evidence.

The failure is therefore beyond semantic recognition: directly setting the hand
intent bypassed the original input path that establishes grab eligibility. The
follow-up supplies that native input under the existing owner/episode/heartbeat
lease. It has no fixed setup pulse: taps retain input until exact native actor
acknowledgment, and held input follows physical B until release or native exit.
Only tweak requests are written at candidate evaluation. No eligibility flag,
animation identity, animation clock, native physics or scoring is forced.

The correction passed 224 production-composed grab checks at 30/60/120 Hz and
the input-session, neutral-flip-release, air-motion-proof, action-dispatch and
control-state suites. The lab suite passed 458 tests with one skip. Its new DLL
still requires the three-pilot hardware acceptance; catalog expansion remains
gated. Machine-readable evidence is in `native-grab-hardware-pilot.json`.

## Corrected trigger-path hardware evidence and repeated-grab defect

Build `ec35a8ee929ce78a2e655fdac35a402c21efba8d`, DLL SHA-256
`8a8857f2d09ddf934df35c356a183c17b914756e37e578bb9b664aa2cc0c578e`,
passed Windows (97/97), Linux and macOS CI and the offline package job. The
installed and loaded DLL identities were independently checked. At measured
30 Hz, B produced detailed identity 1 and the exact BS actor initialized and
advanced. Left+B produced detailed identity 4 and the exact Melon actor
initialized and advanced. Both retained the same flight and returned to native
ground state 100. A 100 ms B tap still reached native initialization after
physical release, then withdrew intent after acknowledgment. Held B produced
the same native BS actor and exited cleanly. Independent native samples observed
CanGrab and IsGrabbing; no physical integration or native identity was forced.

These are animation and landing observations, not full scoring acceptance.
The numeric scoring owner remains unverified, and the capture completeness
flag remains false because some required correlation sources were absent.
Read-only native samples and exact actor evidence are recorded separately from
that incomplete correlation footer. See `native-grab-ec35a8e-acceptance.json`.

Method did not pass: the second B was physically captured and recognized, but
its request inherited the first grab's action generation. Native grab entry
retired the Ollie selection from 1 to -1 while remaining in 201/mode2. The queue
therefore canceled the repeat before it reached the native adapter. Two timing
comparisons also failed; no Melon acknowledgment is counted as Method success.

A production-composed regression reproduced that cancellation at 30/60/120 Hz.
The narrow correction stores the first grab's native provenance and uses the
existing `jump_selection_retired` proof for a new physical repeat. Only the
qualified grab adapter enables this rule. The repeat inherits its direction
across exactly one observed Ollie-to-idle action change in the same owner,
airborne episode, state, mode, stance and animation object. Its physical press
timestamp remains unchanged. Other action changes, owner replacement and
landing still reject inheritance. No queued input is renewed and no flip
transition or native animation mechanism is changed. The revised build still
requires Method hardware acceptance before Stage A can close.

## Historical investigation checkpoint

Source-only checkpoint, 2026-10-30. Baseline:
`65d34a0bc06034125b28779d10490e479df37fba`.
Game executable SHA-256:
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.

**No new grab was implemented or accepted at the historical checkpoint.** The user selected
source/build work only. Native animation acceptance requires a separately
authorized non-Deck test installation. Stages B/C/D remain gated on Stage A;
the recovered enum inventory is not an executable trick catalog.

## Root cause in production

`InputSession::request_auxiliary` explicitly rejects classic named grabs other
than `basic_bs_grab`, including nollie requests. Removing that guard would expose
unverified recipes, not establish native animation initialization.

`grab_plan` describes controller candidates from the inspected native guide.
It supplies no candidate for Indy, Madonna, Sacktap or One Foot Japan.
`NativeActionPlayback::start` permits sustained hold only for BS Grab; named
candidates use fixed 50/300 ms steps and a 34 ms release. Its elapsed-time
sampling can skip setup steps at sparse polls. Neither the recipes nor
`DispatchResult::started` acknowledge the native animation.

A released basic-grab request waits instead of starting in
`request_auxiliary`. The code explicitly lacks a native minimum-initialization
and exit contract. An arbitrary longer trigger pulse would conceal that gap.
`authored_.busy`, the auxiliary owner and the airborne episode guards also
control admission. They cannot be removed merely because a semantic B request
has been recognized. Nonordinary forms route through the extension adapter;
having a repeated-B row in the semantic table does not implement it.

## Source-first search

Searched the current Arcade sources, the entire locally pinned official tree
`official/main` (`e88853add8a78fb79ec4dd5c7fe50ee61e7b9598`), the saved
fork comparisons and the existing upstream method survey. The official tree
contains native trigger guards in `grind_exit_control`, but the search did not
locate a named-grab transition adapter. The previous survey explicitly records
the absence of exact grab adapters and incomplete held-trick semantics. This is
a pinned source search, not a claim about every newer external commit.

Read original installed archive spans without process-memory access, controller
input, console operations or writes to the installation. Every compressed span
was checked against its saved manifest SHA-1, and the executable hash was
checked on each read batch. Decoded metadata was inspected with the baseline's
`ebx_document` and `binary_io` parser. Private game bytes are not committed.
The adjacent JSON preserves resource names, root GUIDs, digests and findings.

The private offline decoder was oozextract revision
`8acaeb2bc375dd56c413a83aae4e87156136fdac`, with local Rust 1.76 compatibility
changes to lockfile syntax, the build-script directive and an equivalent
checked slice conversion. It is not a runtime dependency or DLL component.
Source: <https://github.com/lvlvllvlvllvlvl/oozextract>.

## Recovered native mechanism

Resource indices below are local to the indicated original bundle; they are
not trick IDs or executable addresses.

1. **Candidate intent graph, not a qualified gameplay route:**
   `Includes_UpdateGrabIntents_AG_9AED893` (level EBX 31339)
   contains two float and two bool intent actions with native lifetime and
   reset-on-end behavior. Independent original assets include
   `Bool.Intent.LeftHandGrab`, `Bool.Intent.RightHandGrab`,
   `Float.Intent.OB.LeftGrab`, `Float.Intent.OB.RightGrab`, and the signed
   `Input_GrabTweakLR/UD` inputs. The exact compiled expression operand-to-state
   bindings and timing relative to Arcade dispatch were initially unresolved.
   Follow-up decoding resolves a significant limitation: its inspected float
   update expressions reference `Input_ArmsUp`, not a verified left/right
   trigger input. See the follow-up below before treating this graph as the
   production grab producer.
2. **Owner:** `SF_OnBoard_L_Grab_InAir` (9734) is a dedicated StateFlow
   controller, with `MaxActiveNodes=3` and `EndOnSubjectEnd=false`.
   `Grab.Into.Node` (1268) requires `Bool.GP.CanGrab`. Its subject is
   `Superlayer_OnBoard_InAir_Grab_Into` (11144), whose base is
   `CDBChooser_OnBoard_Grab_Into_(PBRate)` (29785), then
   `CDBChooser_OnBoard_Grab_Into` (26534), then `CDB_OnBoard_Grab_Into`
   (32512). The entry database has 16 entries, not 91 separate entry clips.
3. **Entry and selection:** `Default.Grab.Node` (18812) has an original
   transition to `Grab.Into.Node` (27611), with a VelocityBlendAsset and
   native expression tag. The entry chooser has `ChooseEveryUpdate=false`,
   `ChooseOnReset=false`, and `AllowRechooseOnBlendOut=false`.
   An enum write alone therefore does not demonstrate a new selection.
4. **Reselection:** `Transition_Grab.Into.Node_to_Grab.Into.Node` (2343)
   requires `Bool.Anim.GrabIntoReChoose`, and supplies a DiffBlendAsset.
   The owning node also requires CanGrab for the flattened self-transition.
   `Bool.Anim.AllowGrabIntoReChoose` is a distinct database input. These are
   grab-specific states, not `EnumGS.Intent.FlipTrick` or the flip PTM lease.
5. **Hold:** `Transition_Grab.Into.Node_to_Grab.Loop.Node` (6620) uses a
   native branch window and InertializationBlendAsset. The loop database
   (17741) contains 10 entries. Its BS loop reference resolves to
   `BlendSpace_InAir_Grab_BS_Proto_(Loop)` (31564), a LoopingControllerAsset
   with original `LoopStartTime=55` and `LoopEndTime=305`. It wraps
   `BlendSpace_InAir_Grab_BS_Proto` (27078), a SignalDrivenBlendSpaceController.
   These are native animation ticks, not an Arcade input-hold timeout.
6. **Named poses:** The BS blendspace directly references original sequence
   assets at `(0,0)` for BS Grab (11089), `(0,1)` for Melon (28506), and
   `(1,0)` for Method (18768). Each sequence contains native actor tracks and
   references the corresponding original body ClipControllerAsset. The
   blendspace owns interpolation, has original signal rate/acceleration
   limits, and updates tags on the most heavily weighted controller.
   Its SignalX/SignalY GUIDs are recorded in JSON; their mapping to physical
   stick inputs has not been independently resolved. Coordinates here are
   native blendspace coordinates, not verified Arcade input recipes.
7. **Exit/change:** `Grab.Loop.Exit` (12330) has its own native subject and
   transitions. Loop-to-entry (25340) has 13 conditions, including exclusions
   using `EnumGS.GrabType`. Entry-to-one-foot and fingerflip transitions also
   exist. Asset edges alone do not establish legal physical chaining,
   cleanup, landing, scoring or combo continuity in the current episode.

The working flip bridge operates on a native TrickLaunch PTM replacement and
selects a sample beyond Pop/Jump tags. The grab source path recovered here is
a CDB chooser plus layered, signal-driven blendspace and looping controllers.
**Do not transplant the flip post-pop sample patch, force flip intent, write
GrabType directly, or set a body animation ID to simulate grab entry.**

## Identity and pilot evidence

The original `Enum.GrabType` (gameconfig EBX 3447) has 91 values, including
`None`, ground states and compound transitions. `EnumGS.GrabType` is a separate
state asset. Native enum spellings are preserved in JSON, including
`Benihanna`; they are not silently renamed to semantic-table spellings.

| Pilot | Native enum | Original sequence | Existing semantic binding | Acceptance |
| --- | --- | --- | --- | --- |
| BS Grab | Backside = 2 | seq_c_proto_onb_inair_bs_grab_loop | B | New build unverified |
| Melon | Melon = 11 | seq_c_proto_onb_inair_bs_grab_melon_loop | Left + B | Disabled; unverified |
| Method | Method = 13 | seq_c_proto_onb_inair_bs_grab_method_loop | Left + two B presses | Disabled; unverified |

The asset references and blendspace membership are verified source facts.
The enum values identify declared native states. No claim is made that Arcade
causes those enum values or that a native event occurs from a requested ID.
An absent matching spelling in this enum is not proof that a game trick is
unsupported: aliases, one-foot states and native contextual naming require
separate evidence. Full flip discovery and extended bindings have not begun.

## Missing evidence before implementation and Stage A acceptance

* Resolve native input-expression ports, ordering, eligibility and intent
  consumption against the exact executable; identify a narrowly scoped,
  local-owner-qualified delivery point. Resolve both blendspace signals.
* Trace native chooser/actor initialization, positive-delta advancement,
  release and cleanup. Qualify exact functions and ABI before considering a
  hook. A StateFlow graph declaration does not qualify an executable hook.
* Establish the legitimate tap lifetime from native initialization/exit
  acknowledgments. Do not choose an arbitrary minimum B duration.
* In an authorized test installation, independently correlate physical B,
  semantic request, native identity, active sequence/actor, initialization,
  positive-delta advancement, episode, flight velocity, release, landing and
  scoring for all three pilots at measured 30/60/120 Hz.
* Reject mismatched pose, missing acknowledgment, stale ownership, landing,
  Arcade OFF, ground B brake/X push, and held-input retriggers. Preserve
  existing flips/reverts and prove supported family chains before enabling.

This checkpoint leaves runtime gameplay unchanged. It does not enable named
grabs, substitute animations, change physics, alter manual/grind work, or
claim that model tests prove native playback. Broad catalog implementation
must wait for the missing native mechanism and Stage A acceptance.

## Follow-up for the next gameplay build

Source-only follow-up to `911819950ef977cef414763855e8001b447c189f`.
The existing checkpoint DLL is not a new grab implementation and must not be
installed as one. No installation, process-memory capture, controller input or
gameplay trial was performed in this follow-up. Static archive reads and local
PE disassembly do not establish that a recovered path is active in gameplay.
Machine-readable findings and selected resource receipts are in
[`native-grab-input-followup.json`](native-grab-input-followup.json).

### Expression dependencies

The private inspection utility previously omitted `ResourceReference` values.
Printing those values allows an expression EBX's `GraphData.Resource` to be
matched to its compiled RES entry. This was an inspection limitation; changing
the utility does not change game behavior.

* The float OnUpdate expressions at level EBX 32442 and 10366 use compiled
  RES 5892 and 5893. Their constant-reference patches resolve to the file GUID
  `a9ed1f33-71e6-40f9-a325-61cf67d30888`, whose original asset is
  `Gameplay/Input/Inputs/Input_ArmsUp` (9546). The compiled boolean input getter
  has call hash `adef306d`, resolved to native RVA `0x8314c0`. This establishes
  what the inspected expressions read, not that normal grabs execute them.
* The separate `Input_GrabIntents_AG_B01C97F` (25045) has five actions. Its
  inspected float update (31101) also patches that ArmsUp reference. At least
  one of its bool actions is disabled. Neither graph qualifies a named-grab
  actuator merely by having “GrabIntents” in its name.
* `Includes_GrabTweakCalculation_AG_A353937` (28360) is more specific. Its
  update (19055, compiled RES 13815) references `Input_GrabTweakLR` (771),
  `Input_GrabTweakUD` (26706), the native tweak timer (280), the X/Y delta
  variables (26080/28631), and `EATVariable_Vec2_CurrentGrabTweak` (20515).
  The native float getter resolves to RVA `0x831250`; the expression also
  calls the native spring function at `0xfea340`. The connection from that
  Vec2 to the BS blendspace's SignalX/SignalY is still missing. It would be
  incorrect to equate those names or write the blendspace coordinates directly.

### Native scene update and physics configuration

`SkaterInput_SceneOp` (gameconfig EBX 1149) identifies a native scene operation.
Its update at RVA `0x10681a0` loads the callback at scene context `+0x10c0`,
converts the update interval and calls callback vtable slot zero. Setter
`0x106a220` stores that callback. Registration helper `0xfccdd0` is called
from `0xf8789b` after the registry constructor `0xfaa890`; the registry vtable
is `0x6176088`, with update `0xfb85d0`. That update iterates registered
callbacks and invokes their first virtual method.

This identifies a dispatcher, **not the per-skater grab producer**. Its local
owner binding, the relevant registered callback, and the ordering relative to
the actual grab state transitions remain unqualified. No hook was added at
any of these locations, and none is declared a supported command API.

The original `SkaterCorePhysicsInputConfig` (21229) includes `WantsToGrab`,
`IsGrabbing`, and `IsGrabbingWorld` references. They describe animation inputs
to physics; their names do not establish physical-button commands or action
acknowledgments. Their asset GUIDs are preserved in the follow-up JSON.
The physics output configuration (16420) was also decoded. No physics field
was written or reinterpreted as a grab actuator.

### Unresolved acceptance gates

The following are the exact outstanding dependencies for the three pilots:

1. Locate the definitions and producers of BS blendspace signals
   `806e09fc-e770-8456-806e-09fce7708456` and
   `5e9afe38-769a-3d80-5e9a-fe38769a3d80`. They were not resolved in the
   inspected float/input/small-signal assets. This bounded search is not proof
   that the definitions are absent from the game.
2. Identify the active local-skater grab intent consumer and its eligibility
   and selection ordering. The recovered generic callback registry does not
   supply that ownership proof.
3. Qualify native chooser/actor initialization and advancement independently
   of requested enum, controller packets and StateFlow candidate evaluation.
4. Establish the tap lifetime, held-pose renewal, native release/cleanup, and
   allowed family transitions. No arbitrary pulse duration is accepted.
5. Obtain actual animation, continuous-flight, landing and scoring evidence
   in a subsequently authorized gameplay test. Source-only tests cannot pass
   this gate.

Stage A remains incomplete; zero new grabs are enabled. Runtime source and
the verified native flip bridge are unchanged. There is no new gameplay DLL,
new DLL hash or new CI success to report from this follow-up. The previous
CI result applies only to the previous source checkpoint. Do not start the
broad catalog or ship repeated/compound bindings on the strength of this
static investigation.
# ab50787 hardware follow-up (2026-10-31 Pacific)

The exact CI-built DLL (`9ea528fb0c0896888c9aac614c8cbfb01dce1e32b82534ea7c622afb2d667755`)
was installed and independently verified as mapped by PID 74662. The preceding
be22 DLL, receipt, settings and log were backed up before replacement. See
`native-grab-ab50787-window.json` for measured results and raw receipt hashes.

At native 30 Hz, the 100 ms Basic tap initialized and advanced the original BS
actor, then cleared grab flags through native GrabExit and returned to ground.
Two directional B presses 304 ms apart correctly remained single Melon requests;
the original Melon actor initialized and advanced. Scope 103 at 3500 ms produced
complete, zero-drop captures for these controls. The original 5000 ms capture
overflowed; its incomplete status is retained.

The fast directional pair **was recognized as Method** (semantic 29, presses 2),
but did not initialize an actor. The first request's action generation changed
5679 → 5680 → 5681; the repeat reported `native_owner=false`, failed proofs 88,
and cancellation. Its prior lease ended unacknowledged with `airborne=false`
while independent native samples remained in state 201. That log field evaluates
`same_owner() && airborne()`, so it does not prove a landing. The evidence does
not distinguish owner-resolution failure, identity mismatch or an intervening
state read. Preserve the guards until the exact failed predicate is established.

The earlier 750/950 ms pair still recognized only one Melon. This remains a
separate takeoff/watermark investigation. No runtime catalog expansion, native
scoring acceptance, 60/120 Hz hardware acceptance or chaining acceptance follows
from this window. The Deck was cleaned up and handed to the Manual task.

The subsequent observation-only change makes each `ARCADE NATIVE GRAB END`
identify its retirement call site, the fresh owner proof, whether the native
state was read, and the last failed validity check with its timestamp. Owner
failure codes distinguish no request (1), the existing owner resolver failing
(2), an identity mismatch (3), and a different animation state block (4).
Validity failure codes distinguish gameplay permission (1), assets (2), owner
proof (3), and native air state (4). A false `state_read` means unknown, even
when the numeric state field is zero. The last failed check may precede the
retirement sample; its timestamp is part of the evidence.

The checks retain their original order and short-circuit behavior, and no owner
or episode guard is relaxed. Logging remains in the bounded existing event
queue and client-tick flush. This is diagnostic preparation, not a Method fix
or new native acceptance. Local production-composed regressions passed 494 grab
and 9014 protected flip checks across 30/60/120 Hz.

### dbaa45a diagnostic hardware window

The diagnostic DLL passed all CI (97 Windows tests, Linux, macOS, targeted
tests, runtime build and Deck packaging) and was verified on disk and mapped
in PID 78791. DLL SHA-256:
`4a54bf245ce4fe00b58602c98b9f0770382d7c09b68e561608f9a6bfe66a9356`.
The preceding manual build was backed up before installation. The full measured
results and raw hashes are in `native-grab-dbaa45a-window.json`.

Two Method trials initialized the original Method actor and advanced it with
positive delta; a 100 ms Basic tap did the same for the BS actor. The native
observer measured 30 Hz and return to ground 100 with grab flags cleared.
Retirement owner proofs passed, and no failed validity check was observed.
The intermittent failure therefore remains unreproduced and unresolved; this
observation-only build has not established a gameplay correction.

The first Method retained every capture row (435/435), and Basic retained
423/423. The second Method's SDK footer also reported complete and zero drops,
but independent ordinal auditing found only 426/433 rows in local storage,
missing 278–284. Its second request log is also absent. Native actor execution
is evidenced, but that trial's complete input/recognition chain is **unverified**.
Future acceptance must check retained ordinals as well as the SDK footer.
The first Method's GrabExit value remained unknown; Basic and the second Method
observed native GrabExit. Native scoring ownership, higher-rate hardware tests,
early-press reliability and chains remain outstanding. The Deck was cleaned up
and released with the diagnostic build identity explicitly reported.

### Early native-air B recognition

The ab50787 early trial independently sampled the first physical Left+B at
241038487 while the native actor was already in state 200/mode 0. State
201/mode 2 followed at 241038499, before client action readiness caught up at
241038527. The second B at 241038687 was consequently treated as a single Melon.
This is a recognition-history timing defect, separate from the intermittent
native owner failure.

`ArcadeInput::poll` already reconciles the pose from the physics observation.
However, `UnifiedInput::poll` advances the shared face-input cutoff on every
native action-generation change. That discards the first B during 200 -> 201
even though neither the physical owner nor the airborne episode changed.
The released-Ollie marker used by flips cannot substitute here: B deliberately
retires that marker in `InputSession::poll`.

The fix adds a separate history cutoff used only when recognizing an ordinary
grab through the native grab adapter. It reuses the existing
`InputNativeContext::released_jump_continues` observation predicate, restricted
to two already-airborne samples. This checks the same owner, native selection
object, stance, forward state/selector progression and generation increments.
It grants recognition continuity only; the native grab callback, current action
readiness, request correlation, animation ownership and lease checks still
decide execution. Ground-to-air cutoffs, contextual tables and all flip cutoffs
are unchanged. No animation, engine write or native flip transition changed.

Source review covered the pinned official tree
`6c45e9666dcdea6577248f6ab7f3111de48b6039`, the surveyed fork implementations in
`upstream-method-survey.json`, and the existing event journal, action queue,
native readiness and released-jump observation code. No surveyed implementation
supplied a better equivalent for this specific delayed-action-publication gap.
The shared observation predicate is reused instead of adding an engine hook.

The production-composed regression fails against the preceding source and
passes with this change. At 30/60/120 Hz it covers held and released early B,
original timestamps/episode, a following Method press, no second pop, no held
repeat, and rejection after ground input, owner/selector replacement, landing,
other native action, Arcade OFF, stale observation or expired physical input.
557 grab checks, 9014 protected flip checks, 72 grab-wait checks and 124 action
dispatch checks passed. These are source checks, not native animation acceptance.
The next hardware trial must repeat the original early pair with complete
retained capture ordinals and independent actor initialization/advancement,
release, flight, landing and scoring evidence. Broad catalog expansion remains
gated on Stage A; the intermittent owner failure remains unresolved.

### 5387765 native controls and boundary classification

The isolated Windows DLL from successful CI run 41268074406 was installed and
independently matched to three live process mappings. Windows completed 97/97
tests; Linux and macOS CI also succeeded. The DLL SHA-256 is
`0e0b256ca26cc60b0694765bbad4799b80034d4d6ea4824dae66b0e3f03cd8ad`.
No release was published. The prior manual build and known-good flip rollback
were preserved.

`native-grab-5387765-window.json` records five finite trials with raw receipt
hashes, physical B edges, requests, exact native actor assets, advancement,
flight and native state transitions. Actual physics and cached local-skater
steps were 30 Hz. Complete captures establish native actor initialization and
positive advancement for Basic BS Grab (100 ms tap), Melon and Method. Native
ground100 and cleared grabbing flags followed all three; Basic and Melon also
observed GrabExit, while that value remained unknown in the Method control.
These observations do not establish native scoring ownership or landing score.

The Method control retained 428/428 rows, Melon 427/427, and the repeated Basic
capture 423/423. The first Basic capture failed the local log collector and is
explicitly incomplete. A revised read-only collector retained file descriptors
for rotated segments to prevent inode reuse and added exact assertion reasons;
its subsequent boundary and Basic captures were complete. The cause of the
earlier unlabelled assertion is not independently proven.

Neither Method timing recipe qualifies the specific state200 input-history
fix. The first recipe's initial B arrived in state201. The earlier recipe's
initial B arrived in grounded state103 and was correctly excluded; its second
B became a single Melon, with 431/431 rows retained. Scheduling an input near
takeoff does not prove its actual native context. A future bounded trial must
qualify the first physical edge in state200 from independent observations.
Do not relax the grounded-input cutoff to make this negative control pass.

Stage A remains incomplete: sustained hold, native scoring, the intermittent
owner failure, and higher-rate hardware acceptance remain outstanding. Broad
catalog activation and midair-chain acceptance remain gated. The session ended
with capture stopped, pads destroyed, automatic controller routing restored,
Arcade enabled, native100/mode1 and the console closed. Both collaborating tasks
received the fresh build/process identity and explicit Deck handback.
