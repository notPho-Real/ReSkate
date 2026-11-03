# RC3 implementation status — incomplete candidate

RC2 rollback source is `2ffb006c73671466bfcdc15395ba96939de44f7d`.
Its preserved DLL SHA-256 is
`420cdfae31b76522ccb97f962a2ed288b787e854161c054ad6ddb71e8d144676`.
RC3 branches from that revision without updating upstream. RC2 and the installed
game are unchanged. This candidate does **not** satisfy the full RC3 mission and
must not be promoted as the accepted baseline.

## Manual recognition

Production uses a 450 ms opposite-stroke window on ground and in airborne
preparation. Each deliberate vertical excursion needs approximately 40% travel
(positive 12800 / negative 13057 raw axis units). The manual cone permits
horizontal magnitude up to 1.5 times vertical magnitude. Natural diagonals,
neutral crossings, unequal magnitudes and quick reversals retain the original
physical edge and consume it once. Native acquisition/contact deadlines and the
native Manual/Nose Manual actuator are unchanged.

The added regression samples 20 trajectories in each direction at 30/60/120 Hz,
both grounded and airborne: 240 gestures. It also checks startup-held input,
horizontal steering, unrelated X, stale gestures and duplicate polls. Ten-second
balance input does not retrigger recognition in the fixture. These are input and
composition tests; actual grounded entry, landing and ten-second native balance
acceptance remain outstanding.

## Native grabs

The original adapter now describes 15 exact actors in BS, FS and double-grab
blendspaces, adding 12 profiles to the three previously instrumented profiles.
`rc3-grab-bindings.md` lists all reachable controls. The extended layer is R3
held at B press. The physical edge retains that modifier and direction through
takeoff buffering. Original default, directional and repeated-B slots remain.

The adapter delivers the correct original hand-trigger pair and the inspected
blendspace tweak coordinates. It requires qualified local owner/air episode,
original native admission, exact initialized actor, positive native advancement
and matching native detail before acknowledging. Original animation cleanup and
scoring remain authoritative. No jump/velocity writer or scoring writer was added.

`rc3-grab-coverage.json` accounts for all 171 declarations and all 300 sequences
in the existing inventory. The namespace counts overlap. Fifteen detailed poses
are IMPLEMENTED — GAMEPLAY PENDING; 14 GrabType declarations refer to those same
poses and are ALIAS/DEPENDENT. The remaining 142 declarations are BLOCKED. Every
sequence has an explicit status, identity links and blocker. Unknown execution is
not classified as UNSUPPORTED. The original inventory itself does not prove that
every executable game context has been discovered.

BS, Melon and Method retain prior controlled initialization/advancement and exit
captures at measured 30 Hz, linked from the coverage file. Those captures do not
establish which two grabs the owner currently finds reliable in RC2. No new native
capture, numeric scoring evidence, or claim of current gameplay reliability is
made here. Unit-test acknowledgments are injected observations, not game captures.

Remaining mechanisms:

- Nose, tail, seatbelt, crail, stale and weddle blendspaces are resolved as original
  resources 23974, 3230, 25388, 31357, 23252 and 33124. Their family-entry input
  producer and hand/board state are not verified. Tweak coordinates alone cannot
  legitimately select another family's actor.
- One-footers require original foot release and their own Into/Loop/Exit path.
  Hand-only ownership does not establish those requirements.
- Fingerflips, grab varials and cross-grab transitions require established source
  grab, transition/catch and destination-grab ownership. Their enum declarations
  do not authorize arbitrary animation selection.
- Ground grabs and Coffin require separate grounded admission and lifetime.
- The existing names Madonna, One Foot Japan and Sacktap have no established
  independent native identity in the inspected catalog. They remain blocked;
  another trick's animation has not been substituted.
- Resource 15021's seatbelt BS-shifty sequence emits native detailed tag 24
  (Nose BS Shifty), rather than the expected 17. Preserve that source anomaly
  until its actual runtime selection/scoring is established.

## Fakie manual posture — unresolved

The native game has distinct `OnBoard.Manual.T.Fakie.Loop` (33850) and
`OnBoard.Manual.N.Fakie.Loop` (25379) nodes and native revert-to-fakie transitions.
Both depend on condition GUID `f9a95db4-e022-f765-f9a9-5db4e022f765` and the native
`Enum.Intent.Manual` value. Their entry condition is not equivalent to the RC2
leading-foot bit. The exact condition producer/actor ownership still needs to be
correlated with heading, travel and wheel contact in a native capture.

RC2 already transfers opposite pressure after observed 101-to-100 and requires
two consecutive opposite-wheel contacts. Repeating that inversion or changing a
HUD label would not repair native posture. No posture correction is claimed or
implemented in this candidate. The existing upstream `skater_frame()` reader in
`Extension/Plugins/plugin_game_runtime.cpp` corroborates cached fakie flags with
native context flags and is the source-first starting point for that correlation.

## Repeated flips — unresolved

The existing source audit resolves HeldFlip Into/Loop/Exit and 16 native base
identities, including ordinary Kickflip and Heelflip. HeldTrick condition resource
23124 and action 10792 own admission; the condition's compiled expression resource
is `0x1de580b00d02aaed`. Decoding the EBX metadata resolves imports but does not
resolve the compiled condition/input producer. Native tags own
`Float.Anim.TrickHeldFlipCount`; the actual loop advancement, catch boundary and
scoring amount must be established before a repeated-X adapter can safely use it.

No Double Kickflip or Double Heelflip execution is implemented here. No rotation
counter, PrepareHeldFlip boolean, actor time or scoring ID is forced. Existing
single-flip execution remains unchanged. FingerflipType.DoubleKickflip is not
ordinary Double Kickflip and cannot be used as a substitute.

## Focused owner acceptance

After a candidate DLL has passed its build gates, test on an owner-selected test
installation. This task does not install the candidate on Steam Deck.

1. At each measured 30/60/120 Hz, attempt 20 Up-to-Down Manuals and 20 Down-to-Up
   Nose Manuals. Include short neutral crossings, diagonals, moderate gestures,
   airborne preparation, landing and ten-second holds. Ordinary steering and
   sustained balance must not retrigger.
2. Exercise every row in `rc3-grab-bindings.md`: ascent press, 100 ms tap, held B,
   release before landing, held through landing and repeated attempts. Verify
   exact actor initialization/positive advancement, matching native detail,
   original GrabExit, cleared hands and native ground contact. Record native score
   and combo ownership separately. A displayed name is insufficient.
3. Verify Manual/RT and Nose Manual/RT, chained reverts and ten-second holds with
   body/board heading, travel, wheel contact and active native pose captured.
   The known fakie posture defect is still expected to require correction.
4. Verify single X Kickflip and directional Pop Shove-it during ascent, original
   Ollie trajectory, ground RT, grind entry/first HUD/Ollie-out, auto-push, camera,
   shoulder spin, board toggle, ground X/B suppression, controller ownership and
   Arcade OFF. Repeated-X double rotations remain an unimplemented acceptance item.

CI and DLL identity belong to the exact staged source revision and accompanying
artifact manifest. Green CI alone cannot close any native gameplay item above.
