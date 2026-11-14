# RC4 wallplant prototype

## Preserve the measured board-to-entity origin offset

The `210026e7` Deck trial reached the collision worker and broad-wall query,
but rejected `anchor_adjustment` before entering a hold. A fresh airborne A
was recorded before that invocation. Native state subsequently became 504.
The game remained running; a stale observer readiness sample interrupted the
capture runner. This was a failed gameplay test, not a successful wallplant.

The anchor calculation had treated a board contact point as an entity-origin
destination. Those origins are distinct: `read_surface_board_support` reads
native board part0, while the placement pose supplies the entity transform.
The existing physical-target adapter already preserves their measured offset.
The clearance calculation now does the same: establish the board destination
at the corroborated wall plane plus the wall distance, then subtract the
measured board-to-entity offset to obtain the entity destination. Both origins
move by the same displacement. This is an observation/coordinate correction;
it does not enlarge the 0.70104 m adjustment bound or change rebound tuning.

The wall-outward ray ends at the board destination. Entity and measured body
translation paths remain mandatory, with the same obstacle, ownership and
lifetime checks. The visible foot target also uses the corroborated wall point;
the original native contact is retained for collision revalidation. No body
transform is teleported by the geometry query. The existing bounded native
spring target remains responsible for the planted phase.

The failed build omitted the rejected anchor operands. Its nearby asynchronous
body samples cannot establish the exact entity pose at that invocation. Bounded
diagnostics now retain the same-call entity origin, board origin, destination
and displacement on both rejection and successful clearance. Focused regressions
cover distinct origins, obstacles, unchanged displacement limits and runtime
routing of the measured wall/body observations. Actual hold, release, foot
alignment and combo continuity still require validation of this revised DLL.

## Qualify first contact before collision response

The user-reported immediate bail was reproduced on `971c16b0`. A genuine
airborne A preceded a vertical board contact by 91 ms. The collision producer
recorded cause index 2 before the ordinary physics listener; the listener then
started a new owner generation, discarded takeoff continuity and rejected the
contact. No planted hold or release ran. The nearby bench in that capture has
not been proved to satisfy the broad-wall and clearance requirements.

The existing path now has an earlier entry at that exact collision predicate
call site. The original predicate runs first. A false result, foreign caller,
missing local collision, independent pending bail, disabled mode, stale input
or changed owner preserves the native result. Geometry uses the existing
queries under the native/world lifetime locks and the active collision
invocation. The collision callback validates its current worker's native TLS,
client context, supported phase and query-world reader admission on entry and
at every query boundary. The client update thread's cached identity alone
never authorizes a worker. Ordinary client queries retain their thread gate.
Ground clearance, a broad static face and the checked support-ray anchor are
still required.

The supported query's implementation acquires native reader counters at
world +0x9d0/+0x9d4; its allocation scope uses the calling TLS block. This is
why the collision query can use a qualified physics worker and must complete
and pop its result on that same worker. A writer-owned world, inactive/foreign
TLS, changed context or unavailable lifetime lock rejects before geometry.
The admission reads do not replace the native query's synchronization.

The callback consumes the original input-journal A without polling a held
button again. It retains the original AIR observation and the collision's own
timestamp and serial. After all gates pass it calls the same entry brake,
native drive and bounded hold used by the chooser path. Only successful entry
replaces this one predicate result; it does not clear cause flags, change a
selected state, disable collision processing or suppress later bail checks.
The typed collision proof also corroborates a solver response already moving
slightly outward. The exit still comes from preimpact reflection and scalar
damping, and only the difference from measured body velocity is applied.

Focused regressions cover callback-time A consumption, stale/foreign/repeated
input, early contact without geometry, unchanged observation clocks, solver
response provenance and the existing hold/controller composition. Exact-source
Windows CI and native validation are required before this correction is ready
for user testing. Required HIL includes a valid broad wall's first-contact
hold/release, consumed up/out velocity, clean landing, the trick chain and
genuine no-A/repeated-A controls. The earlier `971c16b0` successes do not validate
this correction. Spring drift, sole alignment and native scoring remain
separate measurements; PR #1 stays experimental.

## Route the held board reference to the native drive

The `93399cb1` trial passed the release-boundary timing check: the last held
target was published at 166 ms and the single release followed at 167 ms.
The first consumed root velocity was `(-2.44748,12.31599,4.40557)` m/s. The
physical board still had a substantial outgoing heading error, and later
horizontal speed reached 8.018 m/s against the 5.377 m/s exit request. The
2.563-second airborne interval ended cleanly, but this is not gameplay-fidelity
acceptance. The full trick chain and negative controls remain pending.

The ground motion path publishes the composed context reference, then copies
the current physical board pose into a separate board drive target. Updating
the motion triplet alone leaves that second target following the incoming
pose. The original drive consumes it separately from the skater/foot targets.

During the existing owned ground-contact hold, the drive now receives the
same bounded reference already published by native ground motion. Admission
requires the exact ground caller, current hold/intent/owner, checked anchor,
unchanged physical pose, matching local-root composition and the live native
orientation limit. Failed writes restore the prior target. The original drive
runs once with its return value preserved. Dynamic body transforms, collision
observations, native states, constraint parameters and the release impulse are
unchanged. This fixes target routing; it does not establish a hard freeze or
guarantee that the physical turn finishes within the hold.

Production-composed regressions reproduce the native overwrite and reject a
mutation that leaves the drive following the old pose. Actual board motion,
outgoing orientation, planted-foot appearance and release speed still require
exact-build hardware validation. PR #1 remains experimental.

## Contact motion ownership and anchor sampling

The `cd4b7e41` trial did not pass gameplay acceptance. It held for 188 ms,
released once, and first reached consumed root velocity
`(-2.23201,12.48428,5.21087)` m/s. A later consumed sample reached
`(-3.37574,15.67908,7.95185)`: the requested 12.7 m/s launch does not establish
the subsequent native trajectory. The video still showed folded legs, and
the actual outgoing physical orientation remained incomplete. Subsequent air
lasted 2.69924 seconds before native ground return without a bail. No combo
chain or negative-input acceptance is claimed for this build.

Two routing defects are corrected in the existing path. Ordinary motion now
retires its remembered ground basis when yielding, and the same owner-bound
contact proof used for combo continuity prevents transient native ground from
re-arming ordinary gravity and speed limits. This preserves native trajectory
ownership without another impulse. Genuine later ground restores normal Ollie
controls. The proof does not create or alter native states.

The render contact anchor previously came from the first partially blended
foot. That made its fixed position and twist depend on entry draw timing and
could constrain the held animation back toward the incoming crouch. It now
samples the held authored frame at full weight using current placement and
native bone lengths, then retains that one surface anchor for the ticket.
Entry/release blends, reachability refusal, board transforms and physical
bodies retain their existing behavior.

These defects have focused regressions. They are not established as the sole
causes of the captured acceleration or visual failure. Actual spring motion,
sole appearance, outgoing orientation and native combo/scoring still require
fresh exact-build verification. RC4 remains experimental and is not ready for
user testing.

## Held body registration

Exact source `0b09695a` passed Windows and integration CI. Its native trial
produced one 174 ms spring hold and release. The first stable pending-clear
root velocity was `(-2.26081,12.31398,4.40095)` m/s, versus the requested
`(-2.23592,12.7,4.80005)`. No later upward overshoot was observed in the retained
capture. Subsequent air lasted 2.56666 seconds before native ground return,
without a bail. The video still failed convincing planted-foot acceptance.

The held asset has an extended planting leg, but body-only playback inherits
the physical placement's bank and pitch. Composing the retained authored frame
with this trial's admission rotation tilts its hips-to-chest direction away
from vertical; precise ankle-plane alignment alone does not correct the torso.
This geometric replay uses retained rig geometry and the admission rotation,
not a complete reconstruction of every rendered frame.

The existing held contact path now optionally removes that inherited bank and
pitch about the unchanged hips, retaining actual placement heading. At full blend both feet
are solved together: the planting foot reaches the observed plane and the
other foot retains its prior endpoint and orientation. Unreachable geometry
refuses the entire body correction and retains the previous limb-only path.
Draw evidence reports `body_frame_applied` separately from endpoint reachability;
this identifies the applied correction, not full blend or visual acceptance.
Placement, board transforms, joint translations, bone lengths and physical
bodies remain unchanged. The original held ticket and release blend bound it.
This does not finish the native outgoing turn or remove the live orientation
safety limit. Fresh visual and native acceptance are still required; RC4 is
not ready for user testing.

## Authored contact frame correction

The exact `4f832eca` batch produced a 187 ms spring hold, one release, stable
consumed root velocity `(-1.76857,12.42918,4.82503)` m/s and 2.701 seconds of
air before a clean landing. The continuous video still showed a tucked pose;
ankle proximity alone was not convincing foot contact. The same build separately
showed a native Kickflip followed by physical Tail Manual for 1.44984 seconds.
That capture ended during Manual, so a completed return to both axles was not
observed. A genuine no-air-A wall contact produced no rebound. The repeated-A
trial did not produce its first rebound and is inconclusive. Native combo
ownership and scoring remain unverified.

The held asset's installed rig and channels identify an authored `Ground`
reference and a lower, extended foot; the other foot is tucked with the board.
The native on-board scalars remain set for both feet and cannot identify the
plant. The existing renderer previously projected an ankle onto the wall while
retaining its original orientation. It neither preserved authored ankle
clearance nor aligned the foot's contact frame with the wall.

The correction resolves `Ground` by the existing library's joint-name API and
selects the lower foot in that reference frame before comparing held-frame wall
reach on the observed body. It derives a foot-local support normal and unscaled
clearance from those actual transforms. The generic render-only limb solver
transfers that reference to the observed wall plane, keeps its first tangential
anchor and orientation throughout the bounded ticket, and rotates only the last
three limb joints. No assumed sole axis, fixed sole thickness, root translation,
bone stretching or native physics write is added. Missing or invalid reference
geometry is refused. An unreachable target remains unchanged and is reported
as unreachable, rather than counted as a successful contact.

Draw-correlated evidence now reports the resulting authored reference point,
normal alignment, target error and reachability. This is not extracted mesh sole
geometry, a physical collision or proof of visual acceptance. The new candidate
requires continuous video and native measurements. The prior spring drift
(0.115 m root, 0.156 m board), later velocity overshoot and orientation limit
remain limitations: that trial requested a 2.0569 radian turn, while the native
1.1 radian constraint permitted 1.0139 radians after the motion reserve. The
contact correction does not alter that safety constraint or claim full outgoing
alignment. RC4 remains experimental and is not ready for user testing.

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
until the first release callback due at 160 ms. The requested target is upright and faces outgoing travel. The ground
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
Release may wait at most one observed native physics interval for its callback
(about 33.3 ms at the pinned 30 Hz cadence). The anchored target remains owned
through that callback, then the single release consumes it. The pose uses the
same fixed dispatch expiry and matching release notification. This is a measured
scheduling difference, not a longer requested hold or a renewable lease.

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

The original stationary Judo footplant loop is loaded from the installed
game and mirrored through the existing animation library. Native/fakie context
still requires the same independent corroboration; both borrow the shared held
phase, rather than either variant's absorb/trick-out clip. The guarded local
pose facility borrows a bounded native pose during the physical plant. Placement
joints and physics are untouched by the pose player. Playback ownership tickets prevent cleanup
from cancelling a newer plugin clip. New trick input cancels the approximation.
The held sample maximizes authored foot reach toward the contacted wall in a
recently observed native body pose, then blends back to native animation
over 40 ms. The pose subscription reports its component, sequence and sample
age; a replaced component, missing sample or sample older than the existing
100 ms render-publication bound cannot substitute the requested outgoing facing.
Selection retains observed pitch and roll while the native body is still
turning. The stance is rechecked before submission and after borrowing playback.
Selection and playback share the body-only blend: native placement and bone
positions without authored translation tracks are preserved, while authored hip
translation and body rotations are applied. The snapshot precedes all pose-player
changes, so selection cannot feed back a previously displayed replacement.
A regression demonstrates that choosing from the clip rest rig can select the
opposite frame from playback on the live skeleton. At draw time,
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

The exact `42faaa55` trial failed visual acceptance despite one 182 ms spring
hold, one outward/upward release, 2.688 s of subsequent airborne gameplay and a
clean landing. Its selected 0.1167 s sample was frame 7 of the full clip. Native
asset records identify frames 0–15 of that clip as absorb and frames 15–40 as
trick-out; a separate 305-frame, 60 Hz, zero-distance loop supplies the held
sequence. The correction loads that held asset exclusively. Focused regressions
refuse absorb/release substitution, preserve the original and mirrored held
poses, and reject failed loads without publishing a partial stance set.
This routing correction does not establish sole alignment or physical freezing.
The existing spring hold, actual draw evidence and native release still require
fresh exact-source hardware acceptance before follow-up/combo tests.

The exact 9987484b hardware trial sustained outward/upward motion through a
clean landing after one 187 ms physical hold. Its two recovered screenshots
do not establish the pose during the hold: screenshot requests followed log
collection, their render times were unverified, and cleanup interrupted frame
collection. A future capture must retain completed partial frames and start
before waiting for the hold's rendered-foot log. These images cannot justify
inventing a sole offset or claiming that a new pose has passed visual acceptance.

The d77a10c6 trial retained continuous video through contact and release. It
showed sustained outward/upward flight and clean landing, but the tucked pose
did not establish a convincing planted sole. The spring hold lasted 181 ms and
the measured physical root drifted 0.1174 m. The first stable pending-clear body
velocity was (-2.3821,12.4946,5.2438) m/s versus requested
(-2.4226,12.7,5.2031). Later native motion still requires comparison; this first
consumed sample does not establish exact trajectory fidelity. The new live-body
pose selection has not yet been validated on hardware.

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

The input snapshot is now read before the clock used to validate its age. The
controller arbitration lock has already been released at that point, so a new
publication can legitimately be newer than the arbitration timestamp. Selection,
reconstruction, hold and departure use the same ordering, retain the 150ms input
lease, and revalidate the original frame or hold deadline without renewing it.
Busy publication, disabled input, generation change, reversed clock, expired
input and multiplayer refusal have separate diagnostics. The native capture that
motivated this correction only reported their former combined refusal; it does
not establish which condition failed. A regression demonstrates the clock race,
but the corrected build still requires native hold and release validation.

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

## Release trajectory continuity

The ffa7 capture reached a 175ms spring hold and one release. Consumed native
body velocity was `[-2.080236,12.261449,3.956027]`, followed by a new native
trajectory request `[-1.067291,12.158988,-.331903]` during state200. The skater
returned toward the wall. No bail504 occurred, but this is not acceptance.
Physical orientation, sole alignment and combo scoring also remain unverified.

The pinned image shows state200's tick at `48347b0` invoking `482f380` at
`4834896` when it needs another curve; that routine calls the existing setter
at `482f646`. The existing release scoped world velocity to its own setter
call, leaving this later native query to ordinary pop shaping. The capture
does not identify the second request's exact call site; the new receipt logs
that site and must confirm the source-backed route on hardware.

A successful release from the already observed wall-contact100 now retains
one continuation receipt for that native100-to-200 query. It expires under the
existing150ms observation lease, cannot be renewed, and is consumed before the
original setter, even on refusal. The caller bytes, complete local owner,
controller generation, unchanged trick/stance/selection, exact next airborne
episode and original previous state are checked. Both body records must have
consumed their pending writes and still be moving upward and outward within
the original exit magnitude. The query uses current physical root velocity,
including losses since release; it never reapplies the stored launch velocity.
Native shaping side effects, collision queries and selection still run. This
continuation adds no query, body write, native state or collision result. It
does not extend the hold or renew the observation lease.

Focused production-composed regressions cover the captured velocities, original
caller propagation, single-use/refused receipts, observation boundaries and
unrelated owner/trick/state/pending-body negatives. Hardware must independently
show the retained outward trajectory, actual hold/release, orientation, clean
landing, follow-up controls and scoring before this branch is ready to test.

## Paired departure velocity

The 04a2 trial first consumed the release in native100: physical root upward
velocity was 12.4328m/s and body8 was 12.3744m/s. A subsequent native201 sample
reached 15.9534m/s on body8, 25.6% above the requested 12.7m/s. The first
consumed sample must not be confused with that later overshoot. The spring
hold lasted184ms; neither the full outgoing turn nor sole contact was accepted.

Private inspection of the pinned executable identified a concrete query-input
mismatch. Native departure construction at4839c5f duplicates its world velocity
into packet80 and90 before setting launch option10c. The setter copies these
to object1290 and12a0. Query preparation at475179c calculates the second
record's magnitude, and4751807 selects it for launch-enabled speed-dependent
candidate generation. RC4 previously replaced only packet80, leaving the
reference speed from the slow held departure. This is a routing defect, not a
reason to reduce the exit constants or alter selected collision candidates.

Explicit world exits now provide both three-component records before shaping
and preserve both before native candidate generation. Ordinary launches retain
their existing packet and shaping. The caller's packet, vector fourth lanes,
query options, genuine collision work and selector remain native. Both source
contracts are fingerprinted before the bridge becomes available. Regression
tests execute the production submission and bridge, verify both speeds and
unchanged packet fields, and reject a mutation that restores the old mismatch.

This fixes the packet inconsistency; it does not establish that the inconsistency
was the sole cause of the later acceleration. Target-history rebasing and curve
position edits were not introduced without that proof. The next exact-source
trial must record both native velocity records, consumed and subsequent body
motion, actual hold/release and orientation. All remaining visual, combo and
follow-up acceptance gates remain open; the draft is not ready for user testing.
# Contact frame consistency and release observation

The exact `24afd4f1` hardware trial is not gameplay acceptance. It completed
one 183 ms hold and one release, followed by a clean landing. The first consumed
root velocity was `[-2.32308,12.43731,5.15535]`; a later native201 observation
was `[-3.71507,15.88995,8.86619]`. The visible held pose remained tucked.
The outgoing physical orientation and the additional native acceleration are
still unresolved. No followup combo or negative-control success is attributed
to this build.

Contact selection and anchoring previously evaluated the authored pose under
the banked native placement. Drawing then levelled the torso and forced the
plant foot back to that earlier endpoint. The same preparation now supplies
all three stages: level about the existing hips, preserve the board foot, then
derive the planted endpoint. A failed levelled solve cannot fall back to a
different frame while reporting the prepared contact as reached. Native
placement, bone lengths, physical bodies and collision observations are not
changed by this render correction.

The original skeleton callback also records the physical body/reference and
spring target positions through four callbacks after release, within 160 ms.
This observation owns no actuator, query or state transition. A separate valid
flag distinguishes the derived target velocity from independently read body
velocity; nonfinite SIMD padding is not interpreted as geometry. The added
read addresses and drive fingerprint are pinned in the supported build header.
This is intended to identify the release discontinuity, not certify its cause
or conceal it with another velocity write. Native acceptance remains pending.
# Held contact solve and blend order

The next native trial of the consistent body frame still rejected all four
drawn contact solves. Its first consumed body velocity was approximately
(-2.4431, 12.3248, 4.8259) m/s; stable pending-clear observations did not show a
later upward overshoot. The 180 ms hold and clean landing do not establish
visual fidelity: the footage still shows a tucked pose rather than a planted
foot. Follow-up and negative trials were not run after this failure.

Two remaining source defects are independently reproducible. Directional pose
ranking did not test reach to the actual collision plane. Also, drawing solved
the held endpoint on an already blended incoming pose, whose hips and board
foot could differ from the fully authored pose used for anchoring. The contact
correction then blended those rotations again. Selection now rejects samples
that cannot reach the real plane. Drawing samples and solves the full held
pose against its fixed anchor before blending once from the current native
draw pose. An unreachable solve remains atomic and cannot stretch limbs or
move physical placement. This corrects the demonstrated source defects; it
does not establish that they exclusively caused the recorded native failure.

Focused regressions cover translated incoming hips, both feet, entry weights,
atomic refusal, and choosing a feasible sample over an unreachable sample with
greater directional reach. Native planted-foot alignment, complete outgoing
orientation, the full trick chain and native combo/scoring remain acceptance
gates. Keep this branch experimental until those gates pass.

# Commit the held endpoint after a successful solve

The next exact-source trial again refused all four observed contact draws.
It recorded a 186 ms spring hold, one release and a clean native landing.
The compositor subsequently crashed; final loaded-image and map checks are
missing, so the retained capture is explicitly incomplete. Neither the launch
receipt nor the landing accepts the visible plant or outgoing orientation.

The draw path retained its first projected endpoint before verifying that the
full authored pose could reach it. A refused endpoint could therefore lock
every subsequent draw to geometry from an earlier placement. The endpoint is
now committed only after a successful full solve. Before that success, each
draw can propose a target using its current placement; after success, the
endpoint remains fixed even when a later draw cannot reach it. Refusal remains
atomic. Measurements retain the attempted endpoint separately from the held
endpoint, including when the first solve fails.

Both-foot regressions cover an unreachable first placement, a reachable later
placement, and refusal to slide an
established anchor. This isolates a source defect, not the exclusive cause of
the captured failure. Hold duration, physical placement, launch velocity and
native orientation constraints are unchanged. Hardware fidelity and the full
trick/combo sequence remain unverified.

# Anchor the held foot to the collision point

Projecting the low authored ground foot onto the wall preserves its tangential
coordinates. That point can be beyond the leg's reach even when the observed
collision point is reachable. Held-body selection and drawing now use the
validated collision point plus the authored sole clearance along the unit
wall normal. They retain the same authored contact rotation, joint lengths,
board-foot constraint and atomic reach refusal. Other contact modes keep
their existing projection. The first successful held endpoint stays fixed.

Both-foot regressions reproduce a low foot whose wall projection is beyond
reach while the actual contact point is reachable. They also verify that
normal magnitude cannot change clearance and that the solve preserves bone
lengths and placement. This corrects the target construction without moving
physical bodies or manufacturing a collision.

The latest native capture still does not establish the intended release:
root vertical velocity was 12.4882 m/s, briefly fell to 1.0936 m/s, then rose
to 13.4746 m/s despite a 12.7 m/s request. The selected native curve began
near 12.7960 m/s. The source of the later acceleration is not established.
The existing bounded observer now records before listeners, immediately
before the original skeleton response, and immediately after it, with a
shared callback sequence. Four release callbacks remain the maximum within
the existing 160 ms window. These are read-only observations, not additional
impulses, and do not certify motion from a pending-write receipt.

Offline captured-pose inspection is supporting evidence only. The external
capture clock and native millisecond clock are not aligned; matching their
numeric values does not identify the same draw or hold. Contact-state poses
support the target correction but do not replace a new exact-source native
trial. Planted-foot alignment, outgoing orientation, release motion, the
full trick chain and native combo/scoring remain unverified.
# Convert the outgoing reference into the native motion frame

The board-drive trial recorded a 168 ms hold, one release, first consumed root
upward velocity of 12.403459 m/s and a clean landing. Its physical heading still
missed the outgoing direction by 84.57 degrees. The four board-drive targets
were consumed, but that routing alone did not establish correct orientation.

The held orientation is specified as the desired published board reference.
Ground motion publishes the articulated local root composed with the local
target. Previously the desired reference was assigned as the world motion
target, preserving an additional articulated rotation in the result. The
correction now bounds the actual board against the desired reference, then
converts that bounded reference back through the original composed reference
into the native motion target. Recomposition verifies the native angular
constraint, including float rounding. The existing motion reserve and native
limit are unchanged; no physical pose, angular velocity or hold duration is
overwritten.

An independent quarter-turn fixture with distinct articulated, local and world
frames fails on the previous implementation. It checks both already-safe and
bounded outgoing headings. Mixed-axis regressions additionally verify the
published reference follows the intended shortest arc. These establish the
coordinate correction, not hardware fidelity or the exclusive cause of the
recorded failure. Physical orientation, foot alignment, the full trick chain
and native combo/scoring still require exact-source native verification.

# Attach the hold to the physical board through a temporary native joint

The reference-frame trial still missed the outgoing physical heading by
75.08 degrees. The published reference moved to the wall anchor while the
physical board remained roughly 0.8 metres away. Approach observations confirm
that the board's local Z axis is its forward axis. Routing a legacy target did
not establish a working physical hold.

The existing hold now uses an internal, general temporary-drive adapter. It
borrows the generated native handle corresponding to physical board body 8,
creates a shape-free keyframed helper body, and connects them with the native
joint factory. The helper starts at the measured board transform. Subsequent
targets carry the measured entity-to-board offset through the bounded outgoing
reference. The adapter uses the existing articulated controller's seven solver
profile values, verified against the pinned image; it introduces no launch,
duration or orientation-limit tuning.

Only the helper receives native pose updates. The borrowed board retains its
motion type, collision shapes and filters, and its articulated connections.
The joint is removed before the existing single release impulse, then its
helper is released. Cancellation uses the same ordered removal. Calls require
the originating thread, qualified client phase and active native TLS context,
current world and generation-checked handles. Uncertain native calls are not
retried. An observed world change quarantines the lease until process restart;
this does not claim an unimplemented world-teardown callback or cleanup of
unknown native allocations.

Focused tests cover resource ownership, ordered cancellation, failed and
uncertain acquisition/removal, stale worlds, recycled handles, measured-offset
conversion and snapshot validation before either target changes. Local runtime
syntax checks treat warnings as errors. These checks do not prove native joint
convergence, a motionless hold, planted-foot alignment, or safe integration of
the new adapter. Exact-source Windows CI and a controlled hardware batch remain
required. Measure the physical board and rig throughout the hold, removal,
release and landing; also test cancellation and Arcade Mode OFF restoration.
The full trick chain, genuine no-A contact, repeated-A rejection, and native
combo/scoring evidence remain outstanding. The branch remains experimental.

# Preserve the composed board origin and acknowledge deferred retirement

The first temporary-drive trial entered one real contact hold, sent one joint
target, then canceled after 81 ms because the angular motion budget was
exhausted. It never released, and the skater subsequently bailed. The recorded
joint target was 0.4535 m below the native composed board reference: the ground
route had already included the articulated offset, and the adapter added the
entity-to-board offset a second time. Ground and air now identify their target
frames explicitly. Ground uses the composed board reference unchanged; air
converts its entity anchor once. The native angular limit and motion reserve
remain unchanged. This corrects a demonstrated target error, without claiming
it was the only cause of the observed angular motion.

Cleanup also incorrectly demanded immediate body-generation invalidation after
native release. That operation disables the body and queues its index for
retirement; the generation remains valid until the queue drains. Cleanup now
verifies the same generation, disabled bit and queued retirement before
relinquishing the owned helper. The joint still detaches first. Failed calls,
unreadable receipts and generation changes remain quarantined with no retry.

Focused regressions compose the production ground wrapper and actual native
cleanup methods. They cover the already-translated board reference, deferred
retirement with a still-valid generation, missing queue acknowledgement, an
active helper, changed generation and partial native-call failures. The prior
trial is rejected; hardware hold, foot alignment, outgoing orientation, release,
full trick chain, negatives and combo/scoring are still unverified.

# Establish the physical turn throughout the owned plant

The corrected-origin trial confirmed that the joint and published board targets
now agree. It still canceled at 79 ms with an exhausted angular motion budget,
zero releases and a later bail. Its first target requested a 1.066-radian turn
after 48 ms without a preceding physical turn target. Later asynchronous body
observations recorded approximately 43 and 32 radians/second of root and board
spin. Those observations do not establish an exclusive cause of the failure.

Admission now publishes the first observed native interval of the turn at the
actual board origin. Later targets sample one fixed shortest arc from the entry
board orientation to the outgoing orientation over the existing 160 ms plant.
At the measured 30 Hz cadence and 1.8605-radian requested turn, the first target
is about 0.3876 radians. Repeated callback times cannot accumulate additional
turns. The ordinary route retains the collision-checked position adjustment;
every target still passes the existing native angle limit and observed-motion
reserve. Solver parameters, collision detection, launch velocity and the hold
deadline are unchanged. Failure retains ordered joint removal without retries.

This is an explicit articulated-body adaptation: the reference changes heading
immediately, while this implementation requests that turn over the planted
phase. The target reaches the exact outgoing basis at 160 ms; that is not proof
that the physical body has reached it. Actual orientation, foot alignment,
hold drift, release and clean landing must be measured on the new DLL before
attempting the trick chain and negative controls. No prior trial is acceptance
evidence for this change.

Focused tests cover phase endpoints, native step scaling, duplicate callbacks,
invalid observations and mixed rotation frames. Substituting the old immediate
turn or an idle admission makes the regressions fail. Host runtime syntax checks
pass with warnings treated as errors. Windows and native validation remain
required; the branch is experimental.

# Carry the established orientation through release

The next trial completed a 166 ms plant and one release. The first consumed
root velocity was (-2.4053, 12.0824, 4.4859) m/s, but the native contact state
persisted and cause 3 preceded a bail. The board was nearly upright and within
17.85 degrees of the outgoing horizontal heading. The native reference had
returned to a rolled, wall-facing orientation. This trial is rejected; the
launch and pause alone do not establish gameplay acceptance.

Ground preparation rebuilds the motion target from the contact frame. Its
caller then publishes that reference before driving the articulated bodies.
Ending all target ownership at the release allowed this preparation to discard
the turn already established during the plant. The release now retains only
orientation ownership for the original contact episode, bounded by the existing
150 ms observation lease. Native preparation still runs once and supplies the
world target position. There is no remaining anchor, temporary joint, body
velocity write, additional query, or requested native state in this handoff.

Each target requires the successful release receipt, unchanged local owner,
controller generation, selected action and contact lineage, plus bounded
outward/upward root velocity. The preceding published orientation must still
match the owned reference within float composition rounding. Physical error
against that preceding reference and the new target must remain within the
original native limit, including the observed angular motion reserve. A new
reference, invalid physical pose, reversed motion, expired lease or authority
failure withdraws the handoff. The first native departure consumes the same
receipt; ordinary landing or another trick cannot reuse it.

Focused source-composed tests reject dropping the reference at release,
renewing its deadline, retaining the wall anchor, or accepting an invalid
physical pose. This is a bounded adaptation to the native contact-to-air
transition, not an extension of the planted phase. Exact-source Windows and
native validation remain required. Foot alignment, clean landing, the complete
trick chain, negative controls and native combo/scoring remain unverified.
# Native drive worker ownership correction

The next controlled trial entered the plant but cancelled at 53 ms, before
release. The entry callback ran on worker 772 and the next ground target ran
on worker 784. The native drive adapter required every operation to use the
thread that originally acquired the joint, so it refused that update before
calling the native target setter. No release or rebound occurred. The video
shows wall contact followed by descent; this is not gameplay acceptance.

The adapter now binds worker identity to each synchronous operation. An atomic
guard rejects overlapping or reentrant operations. Each operation still checks
the active native TLS context, supported client phase, exact client/world/store,
body and joint generations, and the existing lease. A changed world remains
quarantined; a partially failed target cannot be retried. Cleanup can run on a
different qualified physics worker. The caller's existing source transaction
lock continues to serialize gameplay ownership and target composition.

Production-extracted regressions reproduce the observed worker change and
exercise invalid TLS, client phase, context, world replacement, reentrancy,
failed-target quarantine and ordered cleanup. Mutations restoring lifetime
worker affinity or removing TLS/serialization checks fail these regressions.
This correction changes no hold duration, target pose, native spring profile,
collision rule, launch velocity or release lease. Exact Windows compilation and
a new controlled native trial remain required before acceptance.

# Retained contact query validation

The next controlled trial reached a real airborne wall contact with fresh A,
but never entered the hold. The initial geometry frame read succeeded; a later
current-context check then failed with `frame_lock_busy` before any surface
query or request publication. The retained-contact route only selected its
synchronized validator when the initial frame read had already contended.

A current retained contact now selects that existing validator for the entire
query, including when the first read succeeds. The original frame is reused
without changing its timestamp or step, and the retained contact is revalidated
before admitting the route. Queries without a retained contact remain
nonblocking. Contact queries do not publish ordinary cached surface geometry.
The existing owner, TLS, world, generation, episode and freshness checks still
apply at every native query boundary; no lock order or gameplay values change.

Focused regressions cover a successful first read followed by contention,
changed retained contact, ordinary nonblocking behavior, and the existing
expired, future, mode-off, authority and native-transition boundaries. This
fix addresses admission only. It does not establish planted-foot alignment,
release motion, landing, combo continuity or scoring acceptance.

# Retain the published drive reference across release

The admission correction reached a 166 ms hold and a single release. The first
observed consumed root velocity was (-2.9373, 12.1786, 3.4601) m/s, compared with
the requested (-2.2898, 12.7, 4.9940). The next ground target withdrew orientation
ownership on `release_orientation_reference_changed`; the following native
observation reported an orientation error of 1.1734 radians against its original
1.1-radian limit, then state 504. The video confirms a bail. This is rejected
hardware evidence, not acceptance of the hold or launch.

The pinned ground publisher copies the same composed reference both to the
live context and to the rig's published motion reference before driving the
bodies. The handoff now checks the retained rig copy. Admission requires both
copies to agree; subsequent targets require that retained publication to match
the release ticket, including revalidation at the write boundary. A different
native publication still cancels ownership. The live context is measured
separately so its reuse is not mistaken for replacement of the owned drive.
This changes observation identity, not the reference tolerance, native pose
limit, impulse, target position, contact authority or lease duration.

Focused regressions distinguish a reused live context from a replaced drive
publication and reject replacement during authority validation. Native evidence
must still confirm that the retained copy survives the original update and that
the corrected handoff permits an ordinary airborne departure and clean landing.
The spring-held plant is not a verified freeze; sole contact, subsequent tricks,
negative controls and native combo/scoring remain acceptance requirements.

# Carry publication ownership through the planted phase

The next exact-source hardware trial entered a plant but cancelled at 182 ms on
`ground_target_orientation`, before any release. Six physical targets were
submitted and the final native state was 504. The release-reference correction
was not exercised. This remains rejected evidence, not a successful wallplant.

Ground preparation also runs before a due release's body callback. The held
route previously compared the current physical pose against that newly prepared
reference, even after our preceding bounded target had been published and
driven. It now retains ownership only after the board-drive preparation verifies
the context and retained rig publication agree and its target write succeeds.
The next held target requires that exact retained basis and checks the actual
pose against it. Changed publication, changed ownership, stale authority or a
physically invalid pose still refuses the target. The first target has no such
ownership and keeps the original native-reference check.

This reuses the existing publication fields and guards; it changes no hold
duration, native orientation limit, angular-motion reserve, launch velocity,
contact requirement or body pose. Extracted-production regressions cover a
ground callback preceding release at 182 ms and reject reference replacement,
write-boundary changes and genuine physical misalignment. Restoring the old
held-reference route makes the positive regression fail. Same-call prepared
and preceding orientation errors are now recorded for the next native trial.
The prior capture did not contain those exact operands, so it does not establish
that this mismatch was the sole failing predicate. Native validation remains
required before user testing.
