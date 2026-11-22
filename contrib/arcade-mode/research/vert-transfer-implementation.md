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
