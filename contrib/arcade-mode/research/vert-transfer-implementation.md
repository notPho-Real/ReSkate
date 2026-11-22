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
