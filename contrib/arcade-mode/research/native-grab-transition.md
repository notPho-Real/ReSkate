# Native grab transition investigation — Stage A source pilot

## Current implementation checkpoint

The source now includes a native request adapter for BS Grab, Melon and Method.
This supersedes the historical investigation below; **native gameplay acceptance
is still incomplete**. BS Grab and Melon now have native animation evidence at
measured 30 Hz; Method still fails gameplay after the repeated-input correction.
The user subsequently authorized coordinated Deck trials. Broad
catalog expansion and accepted chains remain gated on Stage A native tests.

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
