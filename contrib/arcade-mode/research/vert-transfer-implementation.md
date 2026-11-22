# Experimental vert controller

Based on validated checkpoint a2599c54 (full integration run 44981789373),
from the current Arcade development line. The subsequently observed 7c266cd1
was still building when the isolated branch was created.

Source-first findings: launches::modify_launch classifies pop writers and offers
shape overrides but has no authenticated ramp or contextual transfer identity.
Trainer drive_transition recognizes a broad transition history and manages coping
assistance; its own comments distinguish source observations from gameplay tests.
The native trajectory settings already describe natural vert air and landing-fan
selection. No equivalent contextual transfer controller was located by searching
Arcade, the complete upstream tree, the native-method matrix and 140 retained fork
comparison snapshots. Relevant fork hits were trainer documentation/settings.
The defect is missing contextual behavior, not a reason to replace native gravity.

Implementation uses NativeInputFrame owner/flight/step generations, original board
contact snapshots, SurfaceGeometryNativeApi's validated callback and allocator
lifecycle, the existing timestamped trigger journal, and arcade_air_apply's
transactional articulated velocity writer. Every contributing curved-patch shape
retains its own static owner. Landing candidates are never actual landing events.
A native supported riding state and destination contact establish the landing;
no expected position, simulation endpoint or HUD label does so.

The request has a 150ms physical-edge lifetime. Query results have a 50ms maximum
execution interval and must agree with the original sampled trajectory. Unexpected
velocity or displacement, stale owners, bail, landing, menu/focus loss and OFF
withdraw authority. The writer reuses the existing native packet preparation and world-velocity
trajectory setter before its single uniform velocity delta. It then lets
native gravity/collision continue. Alignment changes only the native physical
target, with bounded tilt and the existing fingerprinted orientation error budget;
active native trick actors retain their supplied pitch/roll. No scoring is invented.

Level Out redirects vertical momentum on both ascent and descent within the
same kinetic and velocity-delta limits; it never reverses descent into ascent.
A low initial speed is not itself a refusal: destination approach speed and impact
still qualify transfers. At exact rest, Level Out permits native physical leveling
with zero added momentum. Its clearance footprint uses the owned travel direction
and measured gravity; absent gravity or a zero-length ray cannot prove clearance.
This limiting case is not evidence of horizontal travel or a successful landing.
The regression reproduces the previous falling/no-motion and low-speed refusal,
including both triggers at 30/60/120 Hz without changing their deadlines.

Remaining evidence requirements: exact-candidate Windows compilation and native
quarterpipe no-input return; LT and RT Level Out with physical trajectory and
orientation; representative spine, hip and lower-transition entry with actual
contact and controllable exit; flips/grabs/spins and ground LT/RT/A-to-X; native
combo banking and OFF restoration. Query ray bundles are not articulated capsule
sweeps. Static allocation stamps do not prove same-incarnation editor relocation.
Runtime candidate admission may conservatively decline narrow crests or interrupted
footprints. The native planner receives both requested and reference velocities
through the existing setter, rather than retaining its pre-transfer landing fan.
Its chosen candidate and physical collision still require native measurement; no
continuous velocity restoration hides a native trajectory takeover.

Hardware gate: use the existing exclusive reviewed runner, verify the selected
Mega Park map and exact source/DLL, and preserve rollback. Capture no more than
one bounded representative batch per integrated checkpoint. Stop on owner/map/
focus mismatch, unexpected collisions, missing telemetry or an unverified install.
An input receipt, offline simulated landing or rendered frame alone is not a pass.

## First native detector trial

Passing build 4a5245c5 was used for one no-trigger approach while the later
planner candidate compiled. It retained 445 physical observations, including
100 -> 200 -> 201 -> 100 and a real supported landing, but never entered the
qualified vert phase. Repeated unavailable command snapshots reset transition
history immediately before departure. The earlier log did not record why each
snapshot was unavailable, so lock contention is a hypothesis, not a proven sole
cause. The approach also carried substantial speed along the lip; the landing
outside that narrow ramp is not evidence that native vert return needs replacing.
No contextual trigger was sent. This was a failed recognition test, not acceptance.

The bounded fix retains recent same-owner curvature only for a busy input mutex.
It clears staged queries and requires a newer admitted frame before any request
or write. Route withdrawal, mismatched state/owner, OFF and long gaps still cancel.
Released alignment cancels across an unobserved step. Development diagnostics now
report unavailable reasons and measured support qualification. Regression tests
exercise stale query completion, original edge expiry and repeated missing frames.
A post-trial screenshot bookkeeping error left the physical trace intact; it did
not justify replaying input. The game was closed normally and the exact prior
installation, score pack and all rollback files were verified restored.

The same trace shows two retained riding-state samples without wheel contacts
before AIR, approximately 33ms. A qualified rising departure now survives that
handoff for at most 100ms from its last actual curved support. Position, upward
velocity, orientation, owner, mode and state must remain consistent. Unsupported
samples never renew the support deadline. This fixes a second source-level loss
of recognition; the external trace cannot prove query-footprint qualification.

A vertical upper transition is allowed as support geometry; it still requires
observed concave climbing to classify as a launch. A partial footprint at the lip
may retain the preceding qualified support during the same bounded departure.
A standalone vertical plane never supplies that missing transition history.

## Complete-scene search regression

A procedural back-to-back transition exposed a runtime search miss: seven
independently cleared, reachable spine trajectories existed, but the search
selected Level Out. Distance had been tied to flight time, excluding a close
landing after a long vert flight; one midpoint probe also missed a narrow crest.
The search now tests flight times independently at each distance/direction sample.
Three supported connection samples retain every shape owner and must rise toward
the origin lip. Missing support or an obstructed trajectory rejects the transfer.

The existing geometry test now links the actual vert runtime. Its compound scenes
cover spines, angled hips, lower transitions, missing crest connections and
obstructions under translation and rotation. These join the procedural curved,
split-shape, category and ownership tests. Native collision, posture and landing
remain acceptance requirements; a selected simulated trajectory is not a landing.

## Contextual landing input and original contact support

The shared trigger contract supersedes the earlier generic ground RT Revert
mapping. The primary input owner controls Nollie/Pressure, Switch and Manual
Pivot. The vert qualifier consumes the same timestamped trigger record through
`TriggerRouting`; it does not sample another edge stream or replace those handlers.
A descending geometry approach can reserve an edge within 180ms before contact.
Only independently observed original wheel support opens native Revert execution,
with a fixed 250ms post-contact window. Native ground publication can precede the
contact bridge by up to two steps; that gap reserves input without granting motion.
Transfer-owned edges cannot reclassify at touchdown. The existing native auxiliary
Revert actuator remains the execution path; neither qualification nor a request
awards score or proves stance completion. Shared dispatcher wiring is pending the
primary engineer's reviewed implementation checkpoint.

A second no-trigger native run on validated source 66bed252 recorded 443 physical
samples at 60Hz, including AIR200/201 and supported riding afterward. It again
failed vert recognition: every reported origin query patch was unavailable, despite
original wheel contacts on the curved transition. Diagnostics also established
both mutex contention and native-frame correlation failures. This does not prove
that either alone caused the absent query patches. The native detector remains
unaccepted. No trigger was sent; the ordinary Arcade air shaper was still active
because recognition failed, so this trace cannot isolate unmodified native vert
return. The exact prior installation and all 146 rollback folders were restored.

Origin recognition now accepts two spatially distinct original wheel contacts with
validated static shape/owner stamps, coherent normals and plausible board distance.
Observed concave climbing remains required; a steep wall alone, duplicate contact
rows or an ordinary Ollie cannot qualify. Complete query footprints remain required
for destinations, with native collision authoritative for landing. Development-only
query receipts identify the callback gate and exact ray/owner rejection without
relaxing them. The contact-source correction and landing qualifier have offline
coverage; their native integration and playable outcomes remain unverified.

The recorded contact sequence additionally retains only the rear wheel for about
109ms before AIR. Fresh contact with the same validated shape and owner can now
continue a previously proven curved transition. A lone contact cannot establish
curvature, and missing contacts still retain only the existing 100ms grace. An
external replay of the measured geometry reaches 17 qualified support and 12 AIR
observations without issuing a motion plan. That replay supplies fixture owner
stamps and input lineage; it is not proof of native admission or playable return.

Controller-state contention now waits for the bounded existing state read instead
of reporting ordinary ground or relinquishing native-flight preservation. Query
callbacks that acquire input ownership remain outside the controller mutex. The
regression pauses original owner validation and overlaps both LT and RT events in
AIR and the ground-before-contact publication gap. Each retains its original
transfer or landing owner and deadline; reservation alone never executes Revert.
The shared-journal session fixtures use the primary engineer's reviewed dispatcher
contract, not a parallel transfer sampler.

An expired transfer request also relinquishes only that request when the same
genuine native flight remains current. It preserves the independent landing
lineage, so a later fresh trigger can still qualify a real landing Revert. Native
flip ownership prevents a staged impulse throughout the original request lifetime.

Released-flight publisher contention: retained native captures contain short
`frame_lock_busy` observations. These now suspend an already delivered transfer
without erasing its independent landing lineage. No transfer execution, impulse or alignment
is admitted while suspended. Original landing edges may remain reserved with
unchanged deadlines, but cannot execute until fresh native contact. A newer original step must preserve owner, episode,
cadence, bounded elapsed time, predicted velocity and position before alignment
resumes. Collision, correlation failure, stale observations, bail and OFF still
cancel. Native contact alone establishes touchdown; no landing is predicted into
existence. The old controller fails the new regression. Current ASan/UBSan runs
pass 2,583 vert checks and 87,398 geometry/runtime checks, including transformed
spine, hip and acid scenes. This remains offline evidence, not native acceptance.


The integrated `ec28f738` candidate passed full CI (run 45017393061) and was
installed for one no-trigger native trial. Steep original ramp contacts reached
`rising_support`, but the flight did not reach `native_air`; this is a failed
playable checkpoint. The external evidence is
`vert-transfers-20261122/integrated-hil/vert-native-checkpoint.json` and its pinned
physical/capture recording. No transfer input or temporary park objects were
introduced. Exact prior installation, rollback inventory and score pack were
restored after the trial.

That recording shows the native rear wheel pair continuing on the lip with
contact mask `0x4000`, which the writer's former `0xc000` gate discarded. The
writer now inspects original contacts for either wheel pair while retaining all
state, thread, category, shape-incarnation and geometry checks. These bits alone
never prove a ramp or landing, and an admitted Manual remains a competing owner.

The optional existing native correlation diagnostic now reaches the vert writer;
normal frame admission is unchanged. Fresh same-owner contact/state publication
gaps retain history for at most 100ms from the last admitted observation. No plan,
transfer input or alignment is authorized in a gap. Only ordinary Arcade air
shaping is withheld, leaving native physics in charge. A new admitted frame is
required, and missing support cannot renew the launch deadline. Owner, stale,
rail, bail, stance and unrelated mode changes still cancel.

The recording also contains an idle `201` mode `1` to `2` transition with no
selected flip, no wheel contacts and an unchanged owner. Existing
`NativeInputState::defer_client_phase` already treats both as ordinary native AIR
while correlation remains invalid. This exact transition can retain no-input
flight history only: it cannot retain a pending or released transfer. The next
fully admitted frame must confirm the same airborne episode, idle selection
object, state `201`, mode `2` and no contacts within the same 100ms limit. Changed
actors, reverse mode transitions and combined mismatches reject. This correction
has offline regression evidence; native return, transfers and landing Revert
remain unaccepted until a compiled corrected candidate demonstrates them.


A separate existing-fixture regression reproduces a local-self query ordering
failure: the nearest hit has a verified local-component owner but unavailable,
inside-facing or zero normals. Those normals were rejected before the
existing native owner-exclusion query could reach the ramp. Only that already
authenticated local component now skips destination-normal validation; its actual
position is still repeatedly read and checked against the ray fraction. Shape,
owner, fraction, lifetime and native exclusion checks remain unchanged. Every
nonlocal hit still needs valid measured vectors. The old source fails this
regression; it does not establish that self hits caused the recorded generic
`motion_hit_vectors` failures.

## Push completion at the lip

Combined candidate 6f132763 (full CI 45031833140; feature tree retained by
2a36bce3) completed one guarded forward-only flight. The 463 physical observations
show ascent to board height 6.878, return to the originating transition with real
wheel contact, then continued supported travel. No LT/RT was sent. The controller
lost rising-support history before its first original AIR observation, so this
is not acceptance of full-flight preservation, transfer, Revert or scoring.

Original publications retain one owner generation while the native push bit
0x40 clears together with the c000-to-4000 contact transition. The old history-gap
policy treats that push completion as a stance change and clears ramp history
when the writer straddles publication. An artifact replay reproduces this loss;
observing the same original rear-contact frame directly preserves eligibility.
The precise failing writer mismatch was throttled out of the native log, so the
replay does not establish this as the exclusive cause of the observed failure.

The correction retains history only for falling 0x40 during proven rising support,
with unchanged leading-foot bit 0x20, owner generation, idle selected actor and
bounded contact/state timing. A subsequent original frame must confirm the
cleared push and actor before normal observation resumes. It does not admit a
mismatched input frame, renew the support deadline or consume a trigger. Tests
cover onset, stance, owner, actor, stale and conflicting continuation at 30/60/120Hz,
and establish original AIR separately. The previous policy fails the regression.

This flight also recorded three self_hit_position query refusals. Admission stays
closed. Development receipts now copy the existing point-reader failure stage,
finite native point and squared ray residual, without additional native reads;
phase-changing vert invalidations retain their original/live correlation fields
instead of being lost to periodic logging. Native point semantics still require
measurement before any change to that query contract. Focused sanitizer checks:
5,063 vert, 89,993 geometry/runtime and 106 body-reader; release-mode boundary
compilation also passes. These are offline results, not native transfer acceptance.

Evidence is in the external artifact workspace under
vert-transfers-20261122/stance-hil: vert-native-checkpoint.json,
push-contact-replay-evidence.json and before-push-faults-result.json. After the
independent ground checks, the game and launcher closed normally; exact prior
5ae57786 files, all 149 rollback sets and the score pack were verified preserved.

## Contact before riding publication

The same 6f trace explicitly records mismatch 192 at original step 17794: idle 201
mode 2 becomes mode 1 while contacts change from zero to 8000, with unchanged owner.
The previous policy rejected this boundary and reset the landing window. A focused
replay demonstrates the lost Revert lineage versus direct original support.

A bounded 100 ms bridge now withdraws transfer velocity/tilt requests immediately,
retains the existing landing lineage and preserves native contact response. The
next original observation must confirm owner, flight, stance, idle actor, mode
and contacts. Only original supported 100/103 establishes landing; Revert still
requires the existing 100/c000/mode<=1 gate. No mismatch executes input, and neither
partial contact nor repeated observations renew the bridge or trigger deadline.

The bridge retains the last pre-contact descent witness. Retained native samples
remain descending; arrested/reversed post-contact velocity is an explicit synthetic
counterexample, not a claimed native observation. Removing contact or changing
owner, actor, stance, mode, direction before contact, or timing cancels safely.
Focused strict sanitizer results: 6,443 vert and 89,999 geometry/runtime checks;
release-mode compilation passes. The runtime test verifies native-preservation
without motion authority through partial 201 contact and original support before
both LT/RT Revert readiness. Native transfer/Revert acceptance remains outstanding.

## Combined candidate native push boundary

The approved fbb09e16 source passed full CI run45076961332 and produced DLL
c31c00d9099fd036d3c8e9dc38f771388172cd1659158704d24574b534181fc4.
One no-trigger Mega Park trial recorded464 physical samples and real
100 -> 200 -> 201 -> 100 transitions, but did not qualify vert. The unthrottled
loss diagnostic isolated step9650: established curved support was cancelled
when the native push bit started (0 -> 0x40), with state100, mode1, contacts
0xc000 and owner generation unchanged. The next original frame confirmed the
push, and original AIR followed at9656. Idle201 then cleared the push as it
entered mode2. The trial sent neither contextual trigger and submitted no plan.
This is failed native recognition, not playable transfer or no-input acceptance.

The focused replay retains measured positions, normals, velocities and original
publication times. Shape and selection identities are explicit fixture tokens;
the capture does not provide an atomic replay of those addresses. The old policy
fails at the captured push onset. History now survives either push edge only on
already established curved support, with unchanged leading foot and known idle
actor. The original confirmation, owner/generation, bounded gap, deadline and
geometry requirements remain. Idle AIR can finish its push during the existing
mode1 -> mode2 handoff, again with no input or motion authority during the gap.
Flatground and standalone vertical support cannot acquire vert from a push edge.

The same trial diagnosed stable local self-hit points away from the ray origin
at fraction0 (squared residuals .00303023 and .0453456). An independently
reciprocal local skater/board component supplies an exclusion owner, not a
landing surface. Zero-fraction self exclusions now require stable borrowed
position storage without treating that contact point as ray geometry. Nonzero
fractions and every nonlocal measured surface retain the2cm ray agreement;
normal, category, world, shape, owner generation, exclusion and cleanup fences
are unchanged. The self point never supplies destination position or orientation.
This is an observation-contract correction, not a wider surface tolerance.

Local strict-warning ASan/UBSan checks:6812 vert,90117 geometry/runtime,106 body
contact reader. Both defects have failing old-policy regressions. The changed
source still needs Windows CI and a new representative native no-trigger run
before Level Out input or transfer acceptance. Original trial and package proof
are retained in the external vert-transfers-20261122/contact-hil and
contact-package artifact directories. Core ground control dispatch is unchanged.
