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

### Native handoff follow-up

The live leading-foot candidate exposed two separate boundaries. In a qualified
Manual RT trial, original native101 returned to100 with the leading-foot bit
unchanged. The opposite RS request was delivered, but changing directly from
-20000 to+20000 was followed by native103,200,201 and an Ollie. The successor
manual was not acquired. This is failed live acceptance, despite the composed
successor-observation fixtures passing. The native pop/flip implementation must
not be changed to conceal this failure.

Bounded native-input controls subsequently isolated this behavior with Arcade
OFF and original-step/returned-packet capture at an observed30Hz. Abrupt
-20000 to+20000 reproduced103,200,201. A200ms physical RS crossing stayed100
and reached the opposite wheel in both directions; the two-truck transfer
interval was approximately234ms and201ms respectively. Neither result proves
the complete Arcade RT chain or ten-second successor hold. The first two
diagnostic attempts only held Manual (capture/parser issues prevented any
transfer), and receive no transfer-success credit.

The handoff now starts from the original pressure and crosses over200ms.
Consecutive original native observations advance the input trajectory; duplicate
client polls and skipped native intervals cannot accelerate it. The complete
opposite pressure must actually be delivered before the existing contact join
can admit the successor. An expired or invalid unfinished transfer withdraws
its actuator instead of falling through to full opposite pressure. This is
input actuation, not invented balance integration or a native acknowledgment.
The existing RT LS recipe, native launch physics and flip paths are unchanged.

In the inverse trial the LS recipe ran, but native101 and a completed revert
were not observed. Original nose contact returned after a brief two-truck
fluctuation. At recipe completion the canceled original actuator had no owner
to resume its RS hold; pressure became zero, followed by a genuine putdown.
This is not evidence that the native revert recipe needs modification.

The original manual actuator now remains the RS owner during its admitted RT
transition. The existing finite revert actor retains its LS trajectory. Only
verified native return transfers RS to the successor request. If no return is
verified, completion of the LS actor leaves the original held manual intact;
its request timestamp, balance episode and combo remain unchanged. Native
owner loss, incompatible gameplay transitions and observation freshness still
bound this authority. The pending transition does not become an acknowledgment.

A production-composed regression replays the inverse trial's missing native
transition and short contact fluctuation, then holds the original manual for
ten seconds at30/60/120Hz in both directions. It checks RS ownership, unchanged
request timestamp, absence of a false successor, continuous episode/HUD/combo,
and subsequent real putdown. Restoring cancellation at RT start fails this
regression. Additional production-composed cases require monotonic native-step
pressure, no advancement across missing delivery intervals, owner-loss cleanup,
and preservation through the final native packet writers. These tests establish
software composition, not live acceptance of the complete revert chain.

### Contact-publication race and immediate chained RT

Live c45f883 trials at measured30Hz established both single RT directions and
held the successor for more than13 seconds, with opposite original wheel
contact and a continuous combo. The quick second RT failed: source step7206
changed Nose contact to both trucks before the client balance publication.
The returned RS became zero at205601182ms, before the second native101 at
205601326ms. No balance endpoint caused that loss. This is failed chain
acceptance, not a successful native termination. The first trial's later real
balance endpoint after feedback stopped is recorded separately.

The input facade was replacing the retained episode pose with riding whenever
exact contact-generation admission lagged. RT then cancelled the original
manual and ran as an unrelated ground revert. Retained pose now uses the
existing fresh, same-owner episode publication during contact-only changes;
correction and new action admission still require their exact current proof.
An established actuator retains RS under trigger-only input while the actual
RT edge waits in the existing request queue for fresh admission. No RT edge,
contact, elapsed physics interval or acknowledgment is synthesized.

A second production regression exercises two acknowledged links without an
intervening native step that observes the pending flag clear. A boolean cannot
distinguish that new RT from the completed link. The admitted request timestamp
now identifies each transition throughout producer and client observation, so
completion of one link cannot suppress the next link in the same episode.

Both regressions fail on their prior production behavior. The publication-gap
case covers both directions at30/60/120Hz through a ten-second successor hold,
plus owner loss, offboard, bail, true putdown and stale-evidence rejection.
These corrections still require a new verified DLL and live chain retesting;
the installed c45 build does not contain them.

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
