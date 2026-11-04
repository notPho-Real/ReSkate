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

## Fakie manual posture — correction awaiting native acceptance

The native game has distinct `OnBoard.Manual.T.Fakie.Loop` (33850) and
`OnBoard.Manual.N.Fakie.Loop` (25379) nodes. The formerly unresolved condition
`f9a95db4-e022-f765-f9a9-5db4e022f765` is `Bool.Physics.RidingFakie`.
`Enum.Intent.Manual` selects Tail=1 or Nose=2 independently of that condition.
The original manual intent conditions read `Input_BoardAdjust`; their Tail/Nose
identity is not exchanged merely because native travel becomes fakie.

RC2 explicitly inverted RS pressure following native 101-to-100. RC3 removes
that inversion and its 200 ms crossing. The original Manual or Nose Manual
pressure continues while the original native graph selects its fakie pose.
No animation, stance Boolean, physical heading, contact bit or HUD label is
forced. This corrects the input routing mismatch; rendered posture acceptance
is still outstanding.

The handoff now requires the original and returning corroborated native fakie
observations to differ. The general SDK reader uses the same native frame,
physics-context and exported stance agreement checks as upstream
`plugin_game_runtime.cpp::skater_frame`. Leading foot alone is insufficient.
An unknown or unchanged fakie state cannot acknowledge the handoff. A return
without orientation proof expires after 1100 ms, rather than keeping a pending
manual transition indefinitely. Consecutive original physics contacts still
establish completion independently of input and score-label delivery.

The portable orientation reader has negative controls for missing sources,
contradictory native/export flags, invalid pointers and offboard flags. Session
tests cover both initial orientations and both manual types, unchanged native
fakie despite changed leading foot, unknown stance, missing 101, owner change and
observation gaps. The composed lifecycle tests cover chained reversions, native
LS trajectory preservation, contact bounce, delivery gaps and ten-second holds
at 30/60/120 Hz. These are synthetic native observations, not rendered evidence.

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
   The pressure routing correction still needs rendered native pose acceptance.
4. Verify single X Kickflip and directional Pop Shove-it during ascent, original
   Ollie trajectory, ground RT, grind entry/first HUD/Ollie-out, auto-push, camera,
   shoulder spin, board toggle, ground X/B suppression, controller ownership and
   Arcade OFF. Repeated-X double rotations remain an unimplemented acceptance item.

CI and DLL identity belong to the exact staged source revision and accompanying
artifact manifest. Green CI alone cannot close any native gameplay item above.

## Local validation and build gate

On macOS with AppleClang 15, the 14 targeted tests pass with warnings as errors.
The production-composed manual lifecycle suite passes all 18 tests, including
rollback controls, HUD ownership, landing, chained reverts and simulated
ten-second holds. The controller route test and generated production publication
checks pass. Existing input handoff, action dispatch, control state, manual
lifecycle, manual observation, manual history and acquisition-boundary suites
also pass. The native grab suite passes 1013 source/composition checks; the new
manual suite covers 240 trajectories and 17544 assertions.

The wider input-events suite retains a pre-existing failure at
`face-first unmatched flip owns no adapter`, reproduced against unchanged RC2.
That expectation conflicts with RC2's immediate neutral-X behavior. RC3 updates
only its manual-window expectations; it does not revert RC2's working flips to
make that historical assertion pass.

Windows runtime compilation, hosted Windows/Linux/macOS and upstream integration
CI have not run for RC3. No RC3 DLL or DLL checksum is available yet. The installed
publication guard only permits an existing development-branch fast-forward and
requires a separate reviewed procedure for new branches. Its full-tree inventory
also refuses the unchanged 27,951,721-byte `assets/builtin/throwdown_menu.zip`, above
its 16 MiB per-blob limit. That blob is identical in RC2 and RC3:
`5c0629f2e24604b1078b135e18de172cd050f0ff`. The guard has not been disabled or
changed, and RC3 has not been published through an alternate writer.
