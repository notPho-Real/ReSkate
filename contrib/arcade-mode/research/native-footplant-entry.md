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
`egf_frontbackfoot=true`, `egf_invertxaonboard=true` and `nocomply=true`.
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
and profile/UI values remain original. No Comply allows two seconds for native
preparation; Boneless retains one second. Entry input remains bounded to300ms
after its prerequisite. Stale ownership, step
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

RC3-10 Mega Park capture
`044613996a45b3c43040c35db4411665a8fe9de5c714f8681fa45092a67904e9`
observed the original Boneless prerequisite, intent, state602, flight and
ground return. Exact PTM animation and scoring acceptance remain open.
No Comply did not enter, and both of its observed state handles remained
unallocated. End diagnostics now distinguish preparation output from an
issued entry input and report the successfully sampled settings prefix.
The prefix bits follow `egf_invertxaonboard`, `egf_frontbackfoot`, `nocomply`;
mask7 means all three were sampled with the requested values. Missing reads
do not become native prerequisite proof.

RC3-18 capture
`acb6c406bf456c58dfd17057860fcca96de1946bdee57d63b243aba9744c6bcc`
observed settings mask7 and preparation output, but no CanNoComply or native
entry. This rules out waiting for an unsampled option in that attempt.

The original prerequisite producer is now resolved in the mongo full-push
sequences: EBX7413, 30864, 31286, 29787, 29575, 12548 and 27735. For example,
7413's BoolGameStateTagAsset26 sets the original CanNoComply asset true over
the sequence's start0/end30 tag interval, with ResetOnLeaveMode1. The regular
full-push sequences in the same bounded survey do not carry that tag. The
next native check must establish the mongo push actor and its tag window;
delivering A or observing a push request alone is insufficient. Do not set
CanNoComply directly or treat a regular push as equivalent preparation.

RC3-18 push capture
`fb9ee5368bff908b13cf53d95eb01f3d82604a713fd79401a133c0b8df18b49f`
isolated a routing defect: the former false A/X override delivered left-foot
intent and regular push throughout preparation, with no mongo state or
CanNoComply allocation. An Arcade-OFF A-then-X control subsequently observed
A select right-foot intent, mongo push and a true CanNoComply window; X selected
regular push. The original invert-A/X asset38291 defaults true; gamepad
expression7573 uses that asset at constant offsets0xf0 and0x110 for both push
branches, and front/back asset39466 at0x108 selects the stance-aware routing.
No Comply now keeps the native swapped preset true for its A preparation.
Boneless retains its independently verified false/false/false settings.
This fixes the settings/output mismatch; native No Comply entry, launch,
animation and scoring still require acceptance on the resulting build.

The regular push caused by the Up prefix is a separate timing constraint.
Native control capture
`dad345855f127cd79f88b766e1a4503b1f5b5b8e9a26b7647963b8f6f9ee8647`
delivered front-foot intent from0.204s but released it before the regular
push finished, so no mongo prerequisite appeared. Capture
`a987f1ce50d6d12d549880c768809370c91bf018b93e0e14c0e566ab144610b1`
held that input through the original transition: regular push ended at1.385s
and CanNoComply was true from1.426s through1.844s. No entry input was issued
in either control. The former one-second preparation deadline could never
reach that window; the two-second bound accommodates it without changing
auto-push, writing a prerequisite, or issuing an Ollie fallback. Production
tests cover the delayed native prerequisite at30/60/120Hz, the unchanged
300ms entry deadline, and expiry when the native prerequisite never arrives.
