# RC4 wallplant prototype

This implements an experimental wallplant path. Windows compilation and live
wallplant acceptance are pending. No native score, combo credit, animation state
ID, landing state or bail flag is manufactured. The milestone contract remains
open until the original native evidence establishes its required sequences.

RC4 integrates RC3 through `134e715d41dfe30bc9a20015b1e3c45160e9eec0`
with a merge preserving the original RC4 specification commit. The latest mongo
push No Comply implementation and RC3 findings are retained unchanged.

## Source decisions

The entire current upstream tree and retained fork survey provide no complete
wallplant adapter. Existing wallride/wallie settings and the native ground
Footplant state are distinct mechanisms. Native one-footer input can commit a
ground footplant and additional flight even after release; it is not reused as
an airborne wallplant command.

The original build-20260929 `process_body_contacts` routine supplies solver
contacts before their per-bone summaries. Its verified two-argument callback
is observed, and the original runs exactly once. Records provide point, normal,
other collidable and a complete native shape handle. Existing SDK pool readers
resolve the shape to its rigid body and check both dynamic and keyframed flags.
All new build offsets and byte fingerprints live in
`Engine/Game/Build/20260929/arcade_wall_contact.h`. The 20 source contracts were
checked against the pinned executable. No original executable or extracted
asset is committed.

The public per-bone reader was previously observed only during bails. The new
raw solver observer must establish that usable contacts arrive while riding
and still airborne. Neither a ray hit nor an empty contact observation proves
that capability. Bounded raw-contact telemetry includes empty airborne steps
to distinguish this failure from a missing observer.

A real, recent local collision is mandatory. Five independent rays corroborate
a broad vertical patch of that exact shape, rejecting narrow rails, edges and
corners. They do not create the contact. Ground clearance, native takeoff,
same-owner flight, shape/body lifetime, inward approach, controller ownership
and a fresh journal A are checked separately. Missing evidence leaves A on its
existing path. No second input engine is introduced.

The revised policy reflects the full horizontal pre-impact velocity around the
horizontal wall normal, then subtracts 5.715 m/s from its magnitude with a
5.08 m/s minimum. The entire reflected vector is scaled; tangential momentum is
not separately retained. The near-zero fallback points opposite the normalized
incoming horizontal direction. Vertical exit is always 12.7 m/s. The current
solver velocity is the baseline for the one release delta, never a second
reflection input. Existing typed chooser/reconstruction provenance is still
required when native collision resolution has already produced outward motion.

Ordinary riding and launch profile caps do not clamp this exit. The existing
articulated writer rejects nonfinite body values or any velocity/angular-velocity
component with absolute value at least 1000; it does not silently scale a valid
exit. Relative velocities and spin across the body batch remain intact. Profile
launch speed is used only to size the independent downward clearance query.

The source execution sequence was reviewed at revision
`98b4e24921446ccd4b157453e25697f9574f0053` of the specified public reference,
including both wall-contact admission and the dedicated hold state's update.
Other retained parameters are the 20-degree approach, 0.6096 m minimum clearance,
1000 ms repetition exclusion, 0.70104 m wall offset, 160 ms hold and 200 ms grind
exclusion measured from entry. No reference code or assets are copied here.

## Physical hold and release

The admitted chooser/reconstruction call performs one uniform entry brake. An
immutable owner/epoch/contact receipt then owns the existing physical-rig target
for 160 ms. The requested target is upright and faces outgoing travel. The ground
adapter bounds the composed articulated reference against the actual board body
and the stricter live native orientation limit. It reserves one native step of
angular travel from the measured root and reference-body angular velocities;
the limit itself is never changed. Native collision and original physics callbacks continue; ordinary SDK
motion writers yield during this interval. At the first validated native step
after the deadline, one release delta establishes the saved outward/upward exit.
That release also submits one fresh native trajectory packet, constructed from
the current owned rig by the pinned native initializer and observation routine.
Only its requested velocity and native launch-candidate option are changed.
The world-space exit now survives the setter's pop-shaping stage. In the prior
native trial the requested horizontal speed was 5.4685 m/s, while the recorded
planner request fell to 1.1405 m/s. The body consumed the initial up/out release,
then lost outward travel and bailed on landing. Native release occurred with a
wall-facing projection vector, so a ground-pop shaping path is inappropriate
for this already resolved rebound. A synchronous, object-owned option in the
existing trajectory bridge runs original shaping once, preserves its native
classification/configuration effects, and restores only the supplied velocity
before genuine collision-candidate generation. Ordinary launches retain native
shaping. The option expires before selection; it does not override collision
results, pending flags, native states, later physics or body impulses. An
unobserved shaping callback refuses release rather than reporting success.
Both the shaped vector and the actual query input are logged. Selected results
can still differ, and sustained physical motion remains an acceptance gate.
The original departure can submit its own query before the final body callback.
An observed 169 ms hold ended with that query using the slowed hold velocity;
the later explicit release was refused as busy. The setter now offers a due,
locally owned hold one synchronous opportunity to supply its saved world exit
to that existing native submission. The receipt is consumed before the call.
The caller's packet options and subsequent selector remain native; no second
query is submitted. Afterward the body callback revalidates the same object and
hold before its single release write. Earlier, foreign, expired and repeated
offers are refused. A failed submission cannot be retried. If no native departure
was offered, the existing explicit release path remains available.
An unrelated outstanding query still rejects release. This does not select a gameplay state
or manufacture a collision result. The release ticket is consumed before either
operation, so a failure cannot cause a second request or impulse.

This addresses a distinct native routing problem: a recorded body batch consumed
the upward release, but the following native air motion reversed it. The native
air path follows a stored trajectory; a body write alone does not refresh that
trajectory. The old curve was not captured in that trial, so this remains a
source-supported explanation rather than proof of exclusive causation. Native
query submission can be asynchronous and its selected candidate can differ from
the request. Requested and selected curve values are logged separately; sustained
post-release body motion must establish acceptance.

Late, interrupted or invalid phases cancel rather than retrying an impulse.
The target stops at 160 ms; release may wait at most one observed native physics
interval for its callback (about 33.3 ms at the pinned 30 Hz cadence). This is a
measured scheduling difference, not a longer requested hold.

A target exactly at the native limit is insufficient: a recorded hold set an
error of 1.0999995 radians against a 1.1-radian limit, then the next native
callback observed 1.1261044 radians and a wipeout. That plant canceled after
62 ms without a release. The motion reserve addresses this measured movement
between publication and consumption. It extrapolates current rates and cannot
guarantee a bound on a later collision impulse or articulated acceleration.
Exhausted budgets cancel; original collision and wipeout checks remain active.
Actual outgoing alignment within the hold still requires hardware evidence.

The target is the real collision point plus the full collision normal times
0.70104 m. Existing client-thread native queries check the outward path, root
path and measured board deck-part support path before admission. The adjustment
is bounded by that same distance; an obstructed or unknown query rejects the
plant. These are finite rays, not a complete articulated volume sweep. Native
collision remains responsible for articulated-body contact.

This native target is a spring drive, so it is not yet proof of complete movement
suspension. The implementation records target calls and entity target error
separately from the owned physical root and board transforms, facing, velocities
and pending-write flags at entry, target callbacks and release. It must not be accepted on the strength
of target writes alone. Unlike the reference's suspended displacement with stored
velocity, this adaptation brakes once and stores the exit until release.

An authenticated plant retains the SDK's existing airborne combo through its
single native100 reconstruction and at most one native step after release. A
later real landing or bail still settles normally. This does not award points,
modify native score state, or establish native combo ownership; those require
separate hardware evidence.

## Animation approximation

The original regular/fakie Judo footplant clips are loaded from the installed
game and mirrored through the existing animation library. The guarded local
pose facility borrows a bounded native pose during the physical plant. Placement
joints and physics are untouched by the pose player. Playback ownership tickets prevent cleanup
from cancelling a newer plugin clip. New trick input cancels the approximation.
The held sample maximizes authored foot reach toward the contacted wall in the
outgoing upright basis, then blends back to native animation over 40 ms. This
selection assumes an upright basis and the clip's native bind pose. At draw time,
the existing limb solver bends the selected leg toward the nearest point on the
observed wall plane from its first drawn placement. The body collision point
defines the plane, not the foot's height. That endpoint stays fixed for the
owned ticket, rather than sliding with later draws. It changes only three render-pose
rotations, preserves bone lengths and the authored foot orientation, and leaves
unreachable targets unchanged. The existing draw restoration restores the native
pose afterward. The same ownership ticket, hold deadline and release blend bound
this contact constraint; it cannot change the root, board or physical bodies.
Endpoint reach is not sole alignment, and the authored foot orientation is not
replaced with a wall-normal alignment. Actual appearance still requires testing.

This is a native asset displayed through the existing pose player, not an
authentic Footplant StateFlow/ActorController transition. The 160ms plant
requires visual testing; clip-clock logs do not prove rendered foot
contact or original actor initialization. Ticket-correlated rendered foot joint
positions are measured against the actual wall plane, but joint origins alone
are not sole-contact proof. A rebound with this approximation
does not close full wallplant acceptance.

The reference's temporary ceiling-collision exception is not reproduced by
disabling collision detection. Genuine native ceiling and articulated contacts
continue throughout the hold and release; resulting deviations require native
evidence rather than fabricated collision or state observations.

## Independent tests and required hardware evidence

Portable tests cover rebound geometry, bounds, body-batch atomicity, contact
and owner lifetimes, separation, cooldown, fresh input, held-A rejection,
ground A, and rejection of unavailable geometry. Query tests use fixture
storage, including narrow rails, seams, changing owners and invalid normals.
Synthetic tests are not native gameplay acceptance.
Production-extracted tests also distinguish physical body transforms from entity
targets and reject busy, changed-owner, stale or invalid trajectory preparation.
They verify that current native packet fields survive unchanged except for the
requested exit and launch option. Before preparing a release, the original native
selector may consume an already completed pending query once. A query still
pending afterward is refused; the helper never clears the flag or polls, and
ownership is revalidated after selection. Tests execute this production helper
against idle, completed, unfinished, renewed and invalid-code boundaries.
This does not prove that a particular hardware query was ready or that the new
exit curve was adopted. Both-foot geometry regressions cover rotated
placement, unchanged bone lengths and unrelated joints, and unreachable targets.

Hardware preflight observed the existing RC3-24 DLL on `skate3_megapark`, local
state100 and native/cached step 1/30, without input or state changes. The
897-sample capture SHA-256 is
`745b30af3e1d4f164b10d3c788ec5efae6cb248246ccb61c3f9f64c6688cda61`.
This is baseline evidence only, not an RC4 wallplant trial.

Before acceptance, verify the exact built and loaded RC4 DLL, actual riding
wall contact, outward/upward native velocity, continued air, original animation
advancement, native scoring/combo ownership and clean landing. Test head-on,
angled and both stances; no-A, floor, slope, ceiling, rail, stale/repeated
contact and Arcade OFF negatives. Then run every sequence in
`WALLPLANT_MILESTONE.md`, including flip/plant/flip/manual continuity. Preserve
RC2 and all RC3 rollback artifacts. Do not promote the accepted branch.
