# Native footplant entry

Pinned game: build 20260929. These controls use the original gamepad input
producer and native action graphs. No animation, trick identity, physics state,
score, trajectory or rotation value is written.

## Controls

- Up then A: No Comply. The Up press must precede A by less than 300 ms.
- Up, release, Up then A: Boneless. The first Up must be within 500 ms of A.
- Plain A retains the existing Ollie path.

The second Up selects Boneless before the single-Up rule. Both stick and D-pad
feed the existing physical event journal. Startup-held directions, intervening
trick/trigger input, air, manuals, rails and occupied controls cannot start this
ground command. One accepted command consumes its direction prefix and A edge;
holding or releasing that A cannot start an additional Ollie.

## Original entry mechanisms

The gamepad update graph is EBX 34108, compiled resource 7573. NoComply input
uses the original raw right-stick magnitude greater than 0.6, angular interval
130–230 degrees, and `nocomply` boolean option. The angle function calls the
original `atan2f(x,y)` import; the interval comparison converts degrees to
radians. GroundTricks_NoComply condition 14074 / expression 10960 requires
`CanNoComply && Input_NoComply`, or an already active `Is.NoComply`. Merely
presenting the stick input is insufficient.

The adapter temporarily selects original front/back push controls with
`egf_frontbackfoot=true`, `egf_invertxaonboard=false` and `nocomply=true`.
It presents A for the native front-foot push, then waits for native
`Bool.GP.CanNoComply` and a later physics step before presenting RS down at
28000. It withdraws when original `Bool.Anim.Is.NoComply` is observed.
EAT action 13663 owns intent, direction, completion and animation selection.
CDB root 5249 retains the game's choice between the original clip path and
sequence path; the adapter does not force the experimental flag. Sequence CDB
10810 contains regular/fakie and straight/frontside/backside variants. The
existing launch observer distinguishes the No Comply trajectory call from an
ordinary Ollie using its call site and preceding physics state.

Boneless condition 29324 / expression 8134 requires XOR of hand-grab inputs,
a fresh PlantLeftFoot, PlantRightFoot or OllieButton input, and native GrabType
1 or 2. The constant-true predicate is not an additional capability gate.
The adapter selects original absolute-foot controls with
`egf_frontbackfoot=false`, `egf_invertxaonboard=false`, and `nocomply=false`.
It supplies LT, waits for original left-hand intent, no right-hand intent and
GrabType 1 on a later physics step, then presents a fresh A plant input.
Native Boneless intent together with state 602 withdraws the inputs. Original
PTM_Trick_Boneless asset 13104, mirror wrapper 16460 and transition 18690 own
execution and launch; they are not airborne one-footer ActorControllers.

All option changes use the temporary gameplay-option owner. Each value must
have been returned by the gameplay getter after the request before any
preparation control is supplied. Another option owner cannot be displaced,
and profile/UI values remain original. Entry is bounded to one second, with
at most 300 ms of entry input after its prerequisite. Stale ownership, step
rollback, unexpected state, lost settings, conflicting controls, focus loss,
disconnection and Arcade OFF withdraw ownership. The 300 ms settings expiry
covers a missing heartbeat.

## Validation and limits

Production-header tests cover recognition at 30/60/120 Hz, physical edge
consumption, prerequisite/physics-step ordering, settings waiting, incorrect
hands/grab family, timeout, release and cancellation. The InputSession test
confirms this path never starts its authored Ollie or automatic-push actuator.

In-game acceptance is outstanding. The native-entry diagnostic explicitly
reports animation, launch and scoring acceptance as unverified. Positive
controls must observe the original action, actor/pose, one native launch,
original scoring and landing. Negative controls are plain A, ground X, ground
B, unavailable entry, held A after completion, manuals, rails, air and Arcade
OFF. Test both stances and both input presets, then normal Ollie-to-flip and
held double flips. Input delivery and native intent alone are not animation
or scoring evidence.

One-footers remain a separate implementation. Fingerflip/varial transitions
and ground-specific grab poses still require their original owners; this
change does not count them as complete.
