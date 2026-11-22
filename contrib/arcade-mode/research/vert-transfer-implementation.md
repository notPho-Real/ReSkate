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
