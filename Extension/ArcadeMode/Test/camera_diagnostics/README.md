# Finite camera observation

`arcadecameracapture <milliseconds>` arms one unsaved observation for 1–3000ms;
zero requests its end. It requires the existing telemetry worker and refuses a
second active capture. It does not enable Arcade, take a camera lease, change
focus, read a controller again, renew native authority, or change camera tuning.
Capture stays observable across Arcade OFF. OFF and restoration never wait for
the observer. No capture starts automatically.

The existing 64-entry client queue and 16-entry worker batch are unchanged.
At most 192 outer tick receipts are queued per capture. Later admitted ticks are
counted as lost/truncated. Formatter failure charges client loss. There is no
synchronous logging on a camera callback, extra native hook, or additional
transform/FOV/mode write. The new evidence remains smaller than the existing
largest client variant. Input capture durations, raw-log caps and rotation stay
unchanged; use a separate camera capture rather than adding it to a near-full
input trace.

Optional copies, atomic accounting and guarded pose reads add observation cost.
Unchanged decisions/native calls do not imply identical callback timing; absence
of a snap in one instrumented interval cannot exonerate an intermittent cause.

## Coverage and wire interpretation

`ARCADE CAMERA: schema=1` has begin/tick/end kinds 0/1/2. `ms` and `end_ms` are
native process clock values: tick entry and observed tick completion, not a
transform's commit instant or a mapping to a video/host clock. Sequence numbers
identify admitted outer ticks. Nested ticks are not borrowed by the outer
observer and mark its receipt `nested`.

The atomic admission gate pins a capture's immutable start/end/identity while
begin, writer and mode observations are in flight. Retirement closes admission
first and defers the footer until those references and sampled frames drain; it
never spins waiting for them. A caller that reaches admission after closure is
unsampled even if it carries an older timestamp. Coverage is of **admitted
attempts**, not every possible engine callback. A delayed prior-epoch caller or
timestamp before the exact new start cannot join a rearmed capture.

`attempted` includes admitted begin-lock misses, separately counted as
`uninspected`. `lost` includes those misses, finish-lock misses, cap exhaustion,
invalid rows and refused queue writes. `queued` is enqueue success, not disk
delivery. End is emitted only with `pending=0`. Explicit stop can leave a pending
frame whose later receipt is marked `late`. Missing end, lost rows, changed
client-drop counters, late receipts, nesting, truncation, duplicate/regressing ordinals or
clock inconsistencies prevent a complete diagnostic claim. There is no implicit
rearm, fabricated footer or claim that missing receipts mean no camera effect.

The three input tuples are only the original three reached `read_input` calls:
`inspected,valid,owner,owner_generation,route_generation,slot,right_x,right_y`.
An unacquired mutex is uninspected, not an invalid owner. An acquired but invalid
snapshot does not expose its subcause. Short-circuit paths leave unreached
attempts absent. Zero fields are unknown and cannot establish owner withdrawal.

Operation tuples are:
`requested,transform,requester,owned,uncertain,stage,before_known,before_mode,`
`before_camera,after_known,after_mode,after_camera,returned,lease_identity,`
`observed_identity,phase_sample,phase_generation,phase_calls`.
Requester 0 is party, 1 Arcade. Lease/observed identities are capture-local
anonymous tokens for the complete existing camera identity tuple, copied only
inside the original busy scope after initial revalidation. They are distinct
from the selected active-camera tokens and cannot be joined across rearmed
captures. Raw pointers are never serialized.
The top-level phase fields are the last reached operation's existing observation;
each operation retains its own tuple. No phase observation is acquired again.

Stages mean an existing operation reached a point, not a new authority grant:
1 initial revalidation; 2 mode call returned and the existing revalidation
returned; 3 transform call returned and revalidation returned; 4 FOV path and
revalidation returned. The subsequent original predicate can still reject.
`returned=true` records only an observed successful tail after those original
predicates. False is unobserved success/unknown, **not** proof the function
returned false: early no-lease, other-requester, busy/phase and destroyed-world
paths may return without a tail receipt. `restored` is the caller's successful
adapter return and alone does not prove a mode transition.

Pose tuples are `valid,kind,camera,eye_xyz,back_xyz,fov`; kind 1 is native camera,
2 FreeCamera, 0 unknown/requested. `pose0` is the optional first transform
operation's guarded pre-operation read. Restore-only operations do not add that
read. Its selected-camera identity came from the preceding original revalidation;
the read is not an atomic selection snapshot or a collision-result receipt.
Its own failure is swallowed inside an isolated error-preserving scope and
cannot suppress the original adapter operation. `follow` is the constructed
follow request; `pose1` the orbit request; neither is applied-result evidence.
`pose2` reuses the existing selected-view publication after known tick writers.
No roll is retained. Invalid/missing poses remain unknown.

Writer bits/counters 0–3 identify returned transform calls at debug tick,
Arcade, party, and later first-person render handoff. Totals include admitted
calls outside sampled ticks and are bounded by the exact capture start/end.
Mode request totals record OFF/ON calls independently of input readiness.
These observations neither synchronize writers nor prove native setters wrote
the requested value. The native camera callback is the existing observed phase;
native collision and other engine writers remain an aggregate unknown boundary.
Published view precedes a possible render-handoff writer and is **not render
final**. The wire always says `final_presented_verified=false`.

## Predeclared causal decisions

1. Lock-miss lease churn is supported only by a reached uninspected input attempt,
   its corresponding restore path, a same-callback successful restore operation
   with qualified mode 1/FreeCamera → mode 0/different native camera selection,
   and a later successful Arcade reacquisition plus qualified pose discontinuity.
   A lock miss, reset request, failed/deferred restore, or video snap alone does
   not establish that cause.
2. Source/owner withdrawal needs inspected, valid neighboring owner/route evidence
   that changes consistently with the restoration. Invalid snapshots, busy
   exits, absent phase observations and zero fields cannot identify a subcause.
3. Competing known-writer evidence requires its returned-call receipt and
   correlated selected-view observations. A changed published pose with stable
   requests alone cannot identify which writer changed it; later render effects
   are explicitly outside that pose's boundary.
4. A native-view versus requested-follow difference can localize a discontinuity
   to the native-output/request boundary. It cannot by itself identify collision
   handling or an unobserved engine writer. No collision or final-presentation
   cause is inferred from proximity to a wall.

No sensitivity, smoothing, deadzone, lease retention, owner freshness or OFF
behavior change follows from these observations alone. A later correction needs
native evidence in a stationary scene, movement near collision geometry,
right-stick look and release, and OFF/ON restoration, with the same source owner
and explicit unknowns. Existing gameplay/controller acceptance is unaffected.

## Software checks

The portable test uses the actual recorder and client queue, including cap/loss,
stop/expiry with pending frames, contention, footer retirement, pre-start/rearm,
nested calls, malformed fields, anonymous identities and OFF/render observations.
The existing source-generated lifecycle fixture executes all three original
camera try-lock failure positions and confirms unchanged restore/reacquisition.
Windows preflight builds and runs this target alongside all existing controls.
These checks establish recorder behavior, not native camera acceptance.
