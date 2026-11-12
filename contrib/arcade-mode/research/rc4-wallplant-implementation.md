# RC4 wallplant prototype

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
