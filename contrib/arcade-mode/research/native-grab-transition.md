# Native grab transition investigation — Stage A incomplete

Source-only checkpoint, 2026-10-30. Baseline:
`65d34a0bc06034125b28779d10490e479df37fba`.
Game executable SHA-256:
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.

**No new grab is implemented or accepted by this checkpoint.** The user selected
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

1. **Intent:** `Includes_UpdateGrabIntents_AG_9AED893` (level EBX 31339)
   contains two float and two bool intent actions with native lifetime and
   reset-on-end behavior. Independent original assets include
   `Bool.Intent.LeftHandGrab`, `Bool.Intent.RightHandGrab`,
   `Float.Intent.OB.LeftGrab`, `Float.Intent.OB.RightGrab`, and the signed
   `Input_GrabTweakLR/UD` inputs. The exact compiled expression operand-to-state
   bindings and timing relative to Arcade dispatch are still unresolved.
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
