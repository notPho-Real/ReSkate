# Manual episode lifetime correction

Baseline: `65d34a0bc06034125b28779d10490e479df37fba`,
`arcade-stage-controller-ownership`. Isolated fix branch:
`arcade-stage-manual-episode-lifetime`. No integration or hardware deployment.

## Source investigation and reproduced failures

Searched Arcade Mode and the entire locally pinned `official/main` tree,
including native physics tuning, grind exit, No Bail and trainer observations.
Read `research/upstream-method-survey.json` and saved fork comparisons for
chasmlol `e1f4a456` (manual/pop observation) and gBGYo `a2a3ee5f`
(bail request versus native consumption). These do not provide a manual lifetime
owner. Reuse native owner/contact/state evidence and existing actuator, revert,
putdown and bail adapters; do not import launch or forced-state changes.

The defects are ownership, timing and observation:

* Grounded entry joins two-truck delivery to a later one-wheel native frame.
  History can publish before or after that frame. A missing dynamics record
  then reset GroundManualBalance and ManualObservation; ComboSession invalidated
  the combo, removing the HUD. A production-extracted regression reproduces
  this on the baseline immediately after the first visible grounded meter.
* ManualAcquisition kept its 1,200 ms clock after matching native contact.
  Missing admission from another observer could turn that clock into putdown.
  Physics continuity also demanded unchanged exact contact/action generations,
  while the identity observer allowed brief two-truck fluctuations.
* Revert protection stopped at acknowledgment before successor admission.
  The opposite actuator could receive the predecessor's still-published pose
  and cancel itself. Repeated client publication before the next original
  physics step exposes that ordering.
* Air preparation already joins owned delivery to an airborne episode and
  verified landing generations. Preserve that admission and the existing
  suppression of conflicting reverse input during prepared landings.

## Lifetime and evidence

`ManualEpisode` owns identity and lifetime under the existing input mutex.
Phases are inactive/acquiring, active, transitioning, airborne-suspended,
combo-airborne and ended. Independent observers admit it but cannot expire it.

Acquisition timeout never requests physical putdown. Established manual
identity survives missing history, score, dynamics and client publication.
Missing intervals suspend integration. No missed dt, input edges, trick labels,
acknowledgments or points are invented. Original request timestamps survive a
qualified short suspension/resumption. A long unobserved producer gap withdraws
old actuation authority until new attributable delivery/history admission;
it does not fabricate physical exit. Actual owner replacement revokes it.

Positive exit evidence is native 300 / observed wipeout, offboard 504, rail
family / 701, native push or an incompatible native owner/state/type without
revert provenance. A normal putdown requires at least 100 ms of consecutive
original state100/mode1/two-truck physics observations outside revert/launch;
a single row, an observation gap, or elapsed client time cannot establish it.
An integrated balance endpoint requests the existing putdown/bail adapter.
That request remains distinct from native completion. Arcade OFF clears all
manual authority and HUD ownership.

State103 is launch setup, not AIR. Actual200/201 suspends the manual; after
500 ms of observed flight its section ends while the combo continues airborne.
Short returns can continue the episode. Longer flights require qualified new
manual landing admission. Neither route banks or invents a trick multiplier.

Reverts preserve episode, combo and balance through the existing native actor,
independent acknowledgment, stance change, opposite RS delivery and later
matching native contact. Adopting already-present opposite contact additionally
requires changed contact generation and verified revert provenance. Balance,
fatigue and RNG transfer without a new setup penalty. The original ground
revert recipe is unchanged.

The linked successor takes RS as soon as the acknowledged native revert has
changed stance in state100/mode1. It does not wait for the remaining LS recipe
to finish. The old implementation kept the original manual's RS pressure
through this interval and could emit a neutral poll between actors. The LS
actor now retains its original trajectory, timing and trigger ownership while
the linked manual owns RS. The delivery observer accepts this specific shared
handoff; an unrelated action still excludes manual delivery. Neither an
acknowledgment alone nor a stance/contact change alone authorizes adoption.

The transition link retires only after both the LS actor has finished and the
successor actuator has independently consumed admitted matching balance. A
client's opposite-pose publication alone is too early: clearing the link before
actuator confirmation allowed a still-held RT to cancel the successor. RT
release is not a prerequisite for this existing verified transition.

Ordinary owned manual input also reserves native vertical locomotion through
acquisition and correction-publication gaps. The physical input still feeds
the balance model; it no longer simultaneously sends native reverse/slide Y.
An actual revert or rotation owner retains its own LS output.

`sample_episode()` integrates only original native rows with matching owner,
contact, input and dt, at or after the independently admitted section step.
Duplicate client reads do not integrate. The episode token owns manual HUD
visibility independently of sample presentation age. Original source timestamps
remain unchanged. Grind HUD retains its existing150 ms lease. HUD state grants
no actuator authority.

## Tests, diagnostics and remaining acceptance

The composition generator extracts actual native-observer, input/remap,
update_combo, mode-toggle and HUD bodies. The focused tests include ten-second
ground holds (both types; early/late history) at30/60/120 Hz, both revert
successors with preexisting/new contact, two chained reverts with ten-second
holds, duplicate consumers, missing records, native read recovery, actual manual
Ollie setup/release, short/long flight, qualified and rejected airborne entries,
real native boundaries, physical endpoints, owner replacement and OFF.
Actual ImGui vertices verify HUD axis/cursor continuity. Source mutations prove
regressions are detected. Old tests requiring observation loss to cause putdown
now require no fabricated putdown. Existing flip/launch/spin, ground control,
camera, controller routing, grind, score and endpoint regressions remain.

The revert vectors include acknowledgment before and after LS playback ends,
held RT through successor admission, repeated client/input polls before the
next native step, and both preexisting and newly acquired opposite contacts.
They require the opposite episode type, continuous RS through actor completion,
the same combo/episode, and every original LS waypoint. The final packet
composition checks both axes after all production writers. Removing shared
successor delivery must fail at the next-original-step admission assertion.
These fixtures supply native evidence at the observation boundary; they do not
prove that the game produces that evidence or that live reverts succeed.

`ARCADE BALANCE TERMINATION` now records episode, original physics step/source
time and first reason. `ManualEpisode::Exit` numbers: 1 disabled, 2 owner,
3 offboard, 4 grind, 5 bail, 6 confirmed putdown, 7 incompatible state/type,
8 modeled endpoint. Owner loss and modeled endpoint are not claimed as verified
native physical completion. Existing bounded capture schema remains compatible
(generic episode-exit code10).

Native flip hooks, StateFlow/PTM, gesture mappings, pop-to-flip handoff, Ollie
launch physics, animation timing and flip classification are unchanged. No
prewind was added. Native ground revert recipe is unchanged.

Offline tests are not live gameplay acceptance. Deck verification still needs
both grounded ten-second holds and entry HUD observation, both RT chains,
air-prepared landings without Powerslide, short/long Ollie transfers, actual
native endpoint outcomes, and protected flip/ground-control/grind/camera/board
regressions. Use existing native captures to report the first termination step
and verify actual physics cadence/cached dt. The installed working DLL must not
be overwritten by this isolated build.
