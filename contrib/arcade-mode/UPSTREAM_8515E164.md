# Official upstream integration: 8515e164

The integration branch is `arcade-stage-upstream-8515e164`. The regular merge
`3fa99ea3ad54285518f84aadbfe3ee1534fb9ec3` has parents
`7abb02fabed9c96e0157fbd5691df003b29e2b11` (Arcade RC5.1) and
`8515e164e5e0d7b3eda497f5a187fc5735170778` (official upstream).
The supported game remains build **20260929**. This is an experimental branch;
its CI packages do not update the accepted Arcade channel.

## Merge behavior

The five actual conflicted files were `Extension/Skater/no_bail.cpp`,
`skater_animation.cpp`, `skater_animation.h`, and
`Extension/UI/Overlay/input_capture.cpp` and `overlay_render.cpp`.
The resolutions retain Arcade wall-contact publication, animation tickets,
component/age/sequence observations, rendered playback receipts, HID completion
correlation and Arcade HUD rendering, alongside upstream pose subscriptions,
room restoration, board mounting, scripted PlayStation menu input and shaders.
The board bone index is named `board_mount_joint` to distinguish it from the new
public `board_mount()` function. Whole-pose and board-mount requests also wake the
animation listeners without requiring a separate clip or joint request.

Runtime, VFS, profile, Animation Studio, input and CMake changes were merged with
the existing Arcade integrations. Upstream corrected Eclipse vector decoding
and clip takeoff/landing marks; Arcade joint-weight mirroring remains present.
The native trick registry, input engine, physics guards and score/UI restoration
were retained. Source retention is not a native gameplay acceptance result.

Upstream inserted `noclip_hold_flag()` before `debug_stop_noclip()`. The ground
writer and Manual reservation test generators now stop at that new boundary,
so they still compile the original body writer, not unrelated noclip helpers.
The chooser wrapper fixture also models the new on-foot ragdoll boundary and
checks fresh, expired and foreign-owner requests without granting a body write.

Full integration found a DualSense exit-filter regression: upstream detects long
USB reads from report contents, while the Arcade writer still chose offsets by
length. The writer now uses the same upstream detector. The regression fixture
uses real zero padding for simple Bluetooth reports and covers long USB reports,
including preservation of unowned controls. The exit-filter test now also runs
in Windows preflight.

## Facilities and ownership

This assessment uses the pinned upstream source, the existing Arcade runtime,
and the retained fork survey in `research/upstream-method-survey.json`. The
historical fork survey is supporting context, not a claim of a new exhaustive
survey. No new native trick implementation was found in the inspected upstream
animation, skater, engine or plugin sources.

| Facility | Native implementation and useful behavior | Limits for Arcade |
| --- | --- | --- |
| `reskate.skater.read_pose()` | `plugin_api_world.cpp::skater_read_pose` calls `RuntimeGame::skater_local_pose/skater_drawn_pose`, backed by `skater_animation::local_pose/drawn_pose`. 395 parent-local joints; 40 bytes per joint: scale, quaternion, position. Animated samples precede our clips; drawn samples include native body-physics placement, before our render overlay. | First request may have no sample. These vector/buffer results do not expose Arcade's entity/generation, render sequence or a synchronized board sample. They are not substitutes for `NativePose`/`PlaybackFrame` acceptance receipts. |
| Enhanced `reskate.skater.pose()` | `pose_replay::apply`, `pose_place`, and the existing animation/render listeners support whole buffers, absolute scale, relative placement, child preservation, still poses, and separate local/remote presentation. Permission `skater.animation`. | A renewable 300 ms global pose lease, with plugin ownership in `RuntimeGame::skater_pose`; it does not acquire an Arcade playback ticket. Full-body overlays can compete with a clip. |
| `reskate.board.mount()` | `skater_animation::BoardMount`, `pending_board`, and `on_board_render` place the complete board rig under the deck mount, in world or skater space, with optional child-bone placement. Permission `board`. | A separate 300 ms lease. A clip with a board wins while its board weight is positive. It does not provide `replace_owned`, `seek_owned`, cancellation by ticket, or board-render acknowledgement. |
| Corrected `reskate.board.bones()` | `on_board_render` retains the unmodified board; `take_board_before_mount` hands it once to `board_pose::on_render` on that thread. Repeated mounts no longer feed their previous result back as native input. | Read-only pose data, not physical contact, collider orientation or a same-frame body/board ownership receipt. |
| Native animation, collision and physics | Existing `anim_studio::Clip`, `try_play_controlled`, `replace_owned`, `seek_owned`, `playback_frame`, native wall contact and verified motion writers remain the Arcade path. General plugin physics/collision services remain permission-gated. | Pose writes occur in render/animation buffers. They do not establish a new rigid-body shape, rail contact, physical balance state, successful trick, or scored occurrence. |

The reusable C++ mechanisms are general building blocks and can serve existing
Arcade animation code without going through Luau. New mode-specific behavior
belongs in a Luau mod under the SDK rule; the public permissions already cover
read/pose/mount prototypes. A new C++ API should only supply a general missing
building block, never a Primo/Casper-specific entry point.

## Beneficiaries, in order

1. **Casper / Casper Flip:** corrected native board reads and drawn body poses
   could improve the entry seed. Existing `make_casper_clips` already authors
   synchronized body and board tracks and solves feet against the board.
   Keep that single owned clip rather than overlaying an independent mount.
2. **Primo:** whole-board roll and joint placement make a visual prototype
   practical in a plugin. Rail-edge support and contact still need independent
   native evidence; a 90-degree drawn board is not a Primo collision body.
3. **Railstand / Truckstand:** board child-bone placement and parent-relative
   feet can simplify authoring. There is no demonstrated new physical support
   mode in these APIs.
4. **Wallplant:** relative joint placement can help author foot/board visuals.
   Preserve `Play::contact`, owned render receipts and the existing guarded
   contact/rebound path; a posed foot touching a wall is not contact evidence.
5. **Pogo / advanced flatland:** native pose capture can supply better seeds;
   authored clips can still share one owner. Repeated impulses or new balance
   rules are a separate physics task.
6. **Stalls / lip tricks / wallrides:** reuse native state/contact observations
   first. Visual pose facilities do not supply missing admission, surface
   constraints, or exits.

## Proof-of-concept decision

The preferred experiment is a Casper entry seeded from the post-physics drawn
body and the matching pre-mount board, then played through the existing owned
clip. Directly replacing the current seed with `read_pose("drawn")` is unsafe:
that result lacks the entity/generation and synchronized board identity checked
by `custom_flatland_runtime::State::accepts_seed`. Independently applying
`board.mount()` is also not a drop-in experiment: the body clip has precedence,
the mount can outlive Arcade's 150 ms ticket heartbeat, and `cancel_play(ticket)`
does not revoke that separate mount lease. Plugin `poser_`/`mounter_` ownership
also does not participate in the Arcade ticket arbitration.

The isolated development experiment now supplies native_render_pose(): a
read-only, owner-tagged body/board seed captured before the body render overlay.
It reads both native buffers twice, rejects changing layouts/owners and active
overlays, and expires after 150 ms. The board mount's prior before/after pair
prevents visual feedback. Plugin writes to the deck mount block the seed until
native animation replaces that lane, even after the plugin clears its request.
Repeated stable reads are not an atomic physics/GPU frame guarantee.

The arcaderenderseed on command selects this seed for existing Casper clip
authoring; off restores the original path. The unsaved switch defaults off,
cancels the owned trick on change and adds no playback or mount lease. The
command reports the last accepted seed and ticket; that is submission evidence,
not rendering. The release configuration cannot enable the experiment.

Focused tests compile the production observer with simulated memory endpoints
and cover read failures, changed buffers/layouts/owners/plans, stale samples,
overlay interference and mount feedback. Existing runtime and release tests
cover entry, cancellation and the unchanged default. These checks do not prove
a visible improvement; the exact Windows DLL still requires the bounded Deck
comparison described below.

## Native acceptance procedure

Use only the exact CI DLL and its recorded source/run/hash, the trusted exclusive
hardware slot, and `skate3_megapark` verified from native level state before input.
Serialize every input and console job. Preserve prior DLL/settings/receipt and
rollback artifacts. A prior session's revoked transport is not reusable.

Run auto-push/Ollie; immediate single/double flips; Manual and Nose Manual held
for two to three seconds; revert; LB/RB prewind; native grabs; grind and rail-pop
exits; Casper and Casper Flip; Wallplant contact/rebound; camera; Trickpedia;
native/custom HUD scoring; and Arcade OFF restoration. Keep source/compilation,
rendering, physics and operator observations separate. At rate-dependent trials,
verify clock cadence and the local skater cached step, not only ForceSimRate.

Use native state transitions, fresh owner-bound event/collision records and
render-copy receipts as appropriate. Input requests and lingering HUD labels do
not prove tricks. Compare Arcade OFF, idle/no-request and cancellation/owner-loss
controls; stop for owner/map mismatch, stale observations or loss of cleanup.
Capture rendered body/board placement as visual evidence alongside native
receipts. CI and offline checks never establish gameplay acceptance.
