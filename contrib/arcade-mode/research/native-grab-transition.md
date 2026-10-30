# Native grab transition investigation — Stage A source pilot

## Current implementation checkpoint

The source now includes a native request adapter for BS Grab, Melon and Method.
This supersedes the historical investigation below; **native gameplay acceptance
is still unverified**. Source/build work remains the authorized scope. Broad
catalog expansion and accepted chains remain gated on Stage A native tests.

The missing intent and observation definitions were recovered from the original
`dingolevel_root` shared bundle. The adapter writes only the native left/right
hand intents and the two native tweak signals. It leaves CanGrab, native grab
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

See `native-grab-source-pilot.json` for exact original identities and asset
receipts. Required gameplay acceptance remains A→B, A→Left+B, A→Left+double B,
tap/hold/release, flight continuity, native landing/scoring, and negative controls
at measured 30/60/120 Hz. No installation or live-channel publication is authorized.

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
