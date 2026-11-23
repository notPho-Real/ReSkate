# Arcade Mode — Classic Controls

This is the literal target control contract. It does not claim that every native
adapter is implemented or verified. Native result observations and the method
matrix determine support; input intent alone never proves a trick. An unavailable
exact equivalent is **UNSUPPORTED**, never silently replaced by another trick.

## Core controls

| Input | Action |
| --- | --- |
| Left Stick | Move, steer and directional trick modifier |
| D-pad | Alternate directional trick input |
| Right Stick | Camera |
| A hold / release, on board | Crouch / ollie |
| A, off board | Native on-foot jump |
| X, airborne | Flip |
| B, airborne | Grab |
| Y | Contextual grind / lip |
| LB / RB | Spin left / right |
| LT tap / hold, on ordinary ground | Select Nollie / Pressure setup |
| RT, on ordinary ground | Toggle native Regular / Switch stance |
| RT, established Manual / Nose Manual | Physical Pivot / Nose Pivot |
| LT or RT, qualified vert landing | Landing Revert, owned by the vert controller |
| LB+RB | Native dismount / remount, one edge per fresh chord |
| Up → Down | Manual request |
| Down → Up | Nose Manual request |

Native locomotion supplies automatic pushes when movement intent and observed
speed require them. The player does not need to repeatedly press a push button.
The physical right stick always belongs to the camera; any synthetic gesture used
inside a native adapter is an implementation detail, not another player control.

This is the single fixed PS2 control scheme. LT alone never jumps. A remains
the crouch/release control: normal setup selects native Ollie1 and Nollie setup
selects native Nollie16. The physical journal distinguishes an LT tap from a
200ms hold without delaying A. That threshold is implementation tuning, not a
historical timing claim. A second short tap returns to normal setup; Pressure
lasts while LT is held and release restores the previous selection. An overlapping
quick LT/A release uses the prospective tap selection immediately. The admitted
launch freezes its identity through that flight, including after LT release.
Both trigger edges use the existing strict30/31 physical dead zone. Holding a
trigger through landing, owner replacement or OFF/ON never creates a new press.

Pressure selection is recognized, but no supported native Pressure actor has
been established. Its unavailable launch/flip requests emit no unrelated trick.
If A remains held while the previous native crouch retires, releasing LT keeps
that original A intent and resumes a supported crouch only after native idle.
An airborne transition, changed actor or lost ownership cancels that intent.
Native riding pushes preserve the remembered setup. The native pushing flag
is not a bail; removing that mistaken reset does not relax launch admission.
On `fdf00b79`, the representative physical LT tap → A charge/release executed
native Nollie16, rose and returned to full wheel support (714/714 capture rows,
zero drops). A separate A-only control executed Ollie1 with no LT/RT input
(693/693 rows, zero drops). Neither substitutes a displayed name for actor proof;
Pressure, other stances and full combo chains remain unverified.

Ground RT targets the identified native ChangeStance input processor with one B double-tap
within its native 300ms window. The final controller packet owns only B; each
gesture phase needs a delivered packet and a newer native step. After final B
release, neutral ownership withholds automatic pushes until native acknowledgment
or the original one-second observation limit. Repeated neutral packets cannot
renew that limit; completion or cancellation releases ownership immediately.
Scalar speed assistance obeys this same finalized reservation, including its
neutral phase; normal eligibility resumes when ownership ends.
If the live leading foot changes before the sampled frame publishes it, only
the final neutral reservation may wait within the original observation limit.
Tap delivery still requires exact agreement; push, owner and asset changes
still cancel the reservation. This prevents a lost observation, not a native
stance change by itself.
The adapter never writes
heading, momentum or a rendered stance. Native leading-foot observation is
separate from input delivery; regular/switch/fakie movement still requires native
acceptance of this candidate. Ground RT has no Revert fallback.
The adapter authenticates exported instance GUIDs at the live object prefix.
File GUIDs identify the containing asset and cannot authenticate that object;
using them prevented Switch admission in the `ec28f738` native trial.
The `6f132763` trial delivered both double taps and retained neutral ownership,
but observed no stance change. A native push flag still ended observation; in
one attempt it appeared before automatic X resumed. The later `fdf00b79` trial
delivered a complete B double-tap while coasting, with no pushing throughout
the gesture and observation window, but still observed no leading-foot change.
Its capture is complete (615/615 rows, zero drops); heading and momentum continued
without a ground Revert. Final XInput delivery does not prove native
`Processor_Pad_B_DoubleTap` acceptance or `Input_ChangeStance` graph entry.
That consumer acknowledgment is the remaining boundary; no supported alternate
stance setter is established. Physical Switch acceptance remains pending.
The input adapter and physical Pivot evidence are separate from native acceptance.
LT during Manual/grind cannot replace the active physical support or its native
launch family. See [the shared trigger contract](TRIGGER_CONTRACT.md) for vert
precedence and exclusive consumption.

X and B execution requires independently observed airtime and a fresh press.
Ground X does not start a jump; ground B retains native braking. Neither command
requires A to remain held. Landing, bail, menus and Arcade OFF release synthetic
ownership.
In production, a neutral airborne X press immediately recognizes a basic
Kickflip request without waiting for release. A valid direction matched before
or with that press takes precedence; a direction arriving after recognition
cannot replace the request, even while X remains held. Release cannot repeat it.
A fresh neutral X after an admitted Ollie release can also be recognized before
AIR is observed. The generic preload is160ms; independently verified native pop
setup retains each physical X edge through that same pop's existing lifetime,
strictly less than500ms from release. This belongs to that released pop;
unrelated ground input cannot borrow it. Native readiness and
exact result checks still gate execution. The separate compatibility policy
retains the delayed held/released-face directional query for500ms; production
does not delay its neutral default for that window.
Direction edges can match a later airborne X/B press for500ms, with held
directions remaining valid. Consumed released directions do not repeat. Extra
air trick face presses use300ms; named grab repeats use 250ms. Neutral B requests
BS Grab.
Repeated-X/B readiness handoffs expire 200ms after the second physical edge.
For a qualified queued pre-AIR double flip only, a matching first flip admitted
before that deadline may give its child one attempt on the immediately following
native physics frame. This consumes one opportunity, retains the original
deadline and cannot outlive the admitted frame's 150ms observation lease.
Changed owner, pop, stance, object, flight, skipped frames or stale observations
cancel that allowance; native actor admission still decides execution.
If first action readiness arrives after the repeat deadline, only the same
already-queued pre-AIR pair can use the original first-AIR observation lease.
Both edges must precede that first AIR, which must occur less than 500ms after
the admitted pop release. Parent admission and its one successor opportunity
must both remain strictly within first AIR +150ms. The timestamp never renews;
the resulting outer limit is less than 650ms after the original pop. Ordinary
airborne repeats retain their 200ms deadline. This adds no fixed input delay.
A qualified ordinary first flip can instead follow its exact admitted pop through
native setup; owner, pop withdrawal and landing checks still cancel it. These
windows add no fixed wait and do not establish native trick completion.

Production Manual pairs accept direct reversal or a neutral gap within450ms,
inclusive on the ground and strict in the air. Both timestamps belong to the
physical direction edges; steering across octants cannot renew the first throw.
The compatibility policy retains its220ms ground window plus55ms sampling
allowance and400ms air window. Air lifetime and actuation receive no sampling allowance.
Moderate stick throws and diagonal travel through center retain the first edge.
A held baseline is not an edge, but subsequent fresh reversals rearm without
requiring perfect neutral. D-pad
and left-stick sequences are independent; held forward cannot swallow D-pad
commands. A released airborne request remains eligible for700ms. Keeping the
final direction held retains its admitted request through the same fresh native
flight; releasing or losing that input cannot renew it. The first qualified
landing starts the separate1200ms native acquisition window. The held entry
direction is reserved until contact, then existing balance controls take over. D-pad Up/Down
can correct an established manual balance.
The first grounded left-stick Down is reserved for the production pair for up to450ms;
a continued hold then passes through. B braking and horizontal steering stay native.
The entry direction does not immediately push the balance meter: release it or
tap the opposite direction to begin correction. Short taps change the meter's
drift velocity and retain their effect after release. Confirmed native manual
contact keeps the meter alive through score polling gaps. A failed manual keeps
its red edge meter until the native pose exits, including when a protected bail
cannot immediately end it; synthetic stick ownership is still released.

Holding forward keeps the native push button held through the complete native
kick and its bounded wheel recovery. Arcade ground acceleration targets11.30m/s
standing or15.33m/s crouched, with a separate16.88/28.66m/s² acceleration budget.
Validated two-wheel input can start that budget before the native kick animation;
ordinary steering, braking, vertical motion and collisions stay native.
Releasing movement or entering an incompatible state releases the push immediately.

## Flip table — X

Repeated presses select the exact listed variant only where a native equivalent
exists. A dash means this contract assigns no variant to that combination.

| Direction | X | X,X | X,X,X |
| --- | --- | --- | --- |
| Neutral X | Kickflip | Double Kickflip | Triple Kickflip |
| Left | Kickflip | Double Kickflip | Triple Kickflip |
| Up | Impossible | Double Impossible | Triple Impossible |
| Right | Heelflip | Double Heelflip | Triple Heelflip |
| Down | Pop Shove-It | 360 Shove-It | 540 Shove-It |
| Up-Left | Hardflip | 360 Hardflip | — |
| Up-Right | Inward Heelflip | 360 Inward Heelflip | — |
| Down-Left | Varial Kickflip | 360 Flip | — |
| Down-Right | Varial Heelflip | Laserflip | — |

Hold R3 and Down when pressing X to request **FS Pop Shove-It**. This uses
the original frontside native identity (7, or 22 in nollie context); ordinary
Down+X retains Pop Shove-It. The modifier and direction are captured on the X
edge, and no repeated frontside variant is assigned. The mapping is implemented;
exact-candidate native actor, rotation, catch and landing verification is pending.

Hold R3 and Up when pressing X for **FS 360 Pop Shove-It**, using the original
native identity 13 (25 in nollie context). Ordinary Up+X remains Impossible.
This is one native 360 shove request through the same captured-edge binding,
not an inferred repeat of the 180 shove. Native execution and landing acceptance
remain pending.

Hold R3 and Left when pressing X for **Front Foot Impossible** (native37).
With Nollie setup selected, the same chord selects **Nollie Back Foot Impossible**
(native39). Ordinary Left+X remains Kickflip and Up+X remains Impossible.
These use their exact native gestures; a missing gesture never substitutes
another trick. Native execution and landing acceptance remain pending.

Neutral or directional Triple Kickflip and directional Triple Heelflip extend the existing held double
only when a fresh third X reaches that same owned flight before its catch. The
third edge keeps its own200ms deadline; it cannot borrow the double's successor
allowance or restart the native base actor. The original held-entry and lifetime
limits remain unchanged. A fourth X cannot extend the triple or delay its catch. These routes
are implemented but require exact-candidate native triple acceptance.

After releasing an ordinary Ollie, a prompt neutral X tap can wait through
takeoff for native AIR readiness. This requires the admitted Ollie release and
its observed native jump; an unrelated ground tap is not carried into a fall.
The physical tap retains the existing500ms query window. The actor still waits
for the native jump selection to become ready before issuing Kickflip.

These prompt-tap and held-air changes implement missing intended controls.
Earlier scripted native positives did not establish reliable physical-controller
use of either sequence; that acceptance remains separate.

The table describes the intended mapping. Current native Deck acceptance covers
Kickflip, Heelflip and Pop Shove-It. Hardflip is unresolved: the authored request
for native9 produced native7, and a separate radius control produced native4.
Neither result verifies Hardflip. Other catalog rows retain their authored
candidates and require independent native identity checks.

## Grab table — B

A neutral B press during native airtime requests BS Grab. A matched directional
input selects the named grab below. Release and press B again within 250ms to
request the second variant, retaining the first grab's direction. Holding B is
not a second press. Native readiness and same-flight ownership still govern
admission; neither a request nor its label proves execution.

These bindings are implemented except the explicitly unsupported entries. The
Indy control slot uses the native FS Grab identity and displays that name.

| Direction | B | B,B |
| --- | --- | --- |
| Left | Melon | Method |
| Up | Nosegrab | Rocket Air |
| Right | FS Grab | Stiffy |
| Down | Tailgrab | One Foot Tailgrab |
| Up-Left | Japan | Unsupported: One Foot Japan |
| Up-Right | Unsupported: Madonna | Judo |
| Down-Left | Benihana | Unsupported: Sacktap |
| Down-Right | Airwalk | Christ Air |

Holding R3 with a direction and pressing B uses the existing extended bindings:

| Direction | R3 + B |
| --- | --- |
| Left / Right | Double Grab Left / Right |
| Up / Down | Double Grab Front / Back |
| Up-Left / Up-Right | Crossbone / FS Nosebone |
| Down-Left / Down-Right | FS Tailbone / Tuck Knee |

Neutral R3 + B retains the diagnostic selector's chosen profile. The selector
also exposes additional native profiles without assigning them new common bindings.

The [retained native actor pass](research/rc3-native-grab-catalog-pass.md) observed
initialization and advancement for 44 ordinary profiles on its exact historical
build. It does not establish current-build acceptance of every control, extended
hold, stance, exact Exit actor or numerical scoring. One-footers have separate,
incomplete lifecycle evidence. Preserve those limits when reporting a grab as
observed or accepted; never substitute a basic grab for an unavailable named one.

## Grind / lip table — Y

Native rail/board geometry decides the appropriate member of a semantic pair.
Left/right does not hardcode an identity that depends on physical approach,
orientation or which end of the board meets the rail.

| Input / relationship | Semantic action |
| --- | --- |
| Y, parallel | 50-50 |
| Y, sufficiently perpendicular | Appropriate Boardslide / Lipslide |
| Up + Y | Nosegrind |
| Down + Y | 5-0 |
| Left or Right + Y | Appropriate Noseslide / Tailslide |
| Up diagonal + Y | Appropriate Crooked / Overcrook |
| Down diagonal + Y | Appropriate Smith / Feeble |
| Up,Up + Y | Nosebluntslide |
| Down,Down + Y | Bluntslide |

| Direction | Y,Y, where an exact equivalent exists |
| --- | --- |
| Up | Nosegrind to Pivot |
| Down | 5-0 Overturn |
| Left | Hurricane |
| Right | Salad |
| Up-Left | Hang Ten Nosegrind |
| Up-Right | Crail Slide |
| Down-Left | Double Blunt Slide |
| Down-Right | Darkslide |

On an established Bluntslide, X→Y now requests Tailslide and X→X requests
Smith through the existing native 5-0 return. The same rail segment, completed
return, retired transition and observed neutral controls are required before
the final leg. Y→B requests Nosegrind and B→B requests Crooked through that
5-0 and a separately completed 50-50 return before the final sweep. Each
intermediate keeps its own original native admission and completion checks;
the original physical button timestamps remain unchanged. These routes require
the existing qualified horizontal rail segment of at least12m; available travel
still determines whether the entire chain can finish. They are offline-tested; exact-candidate native
execution, animation and combo acceptance remain pending.

From an established 5-0, Y→X requests Noseslide and B→Y requests Nosebluntslide.
Both use the completed 50-50 return, then a separately completed Nosegrind
preparation, then the existing native lateral transition. Neither intermediate
counts as the requested slide. The original request has a fixed maximum of
4.5 seconds; this is an expiry ceiling, not an added input delay. Loss of rail,
owner or transition authority cancels the chain. These extensions pass the same
offline controls and completion tests; native execution remains unverified.

## Manual, Pivot and balance

Up → Down requests Manual; Down → Up requests Nose Manual. Only verified native
observations establish the actual pose. Manual correction uses Up / Down; grind
correction uses Left / Right. Both meters display the actual inertial balance
model, including fatigue retained throughout the same unbanked combo.

Ordinary ground RT toggles native stance without a Revert trajectory or automatic
180-degree rotation. Established Manual/Nose Manual RT requests a physical
Pivot/Nose Pivot, retaining the balance owner and combo. Success requires measured
board and body rotation, native stance transition and qualified successor support;
changing a label or fakie flag alone does not pass. A vert landing Revert creates one continuation
opportunity; repeated revert cannot keep extending grace or farming multiplier.
During an established Manual, a physical RT press survives a delayed client
balance publication only until the original native observation's150ms expiry.
It runs immediately when current Manual proof arrives; later polls do not renew
it. Losing or replacing that Manual, changing native generations or pressing A
cancels the pending Pivot. Unconfirmed Manual acquisition cannot borrow Pivot
authority later from a held RT. The original landing qualifier exclusively owns
vert Reverts; neither transfer nor landing inputs may become delayed ground commands.

The active Manual acceptance recipe is entry → at least three continuous seconds
of independently qualified physical balance → Ollie out. Begin timing at qualified
entry, not the direction pair. Check actual wheel support and pose, stable balance
HUD, owner/stance continuity, and the native exit and landing. Then check
Manual → RT → physical Pivot → valid successor Manual/Nose Manual → Ollie → Flip
continuity separately, including resulting stance and support. Three seconds is
a test minimum, never a gameplay cutoff; balanced holds must continue beyond it.
Retained two-second or ten-second payloads are historical recipes and must be
updated before reuse, with their existing physical, freshness and cleanup guards.

## Live mode boundary

Pause → Mod Options → Player → Arcade Mode changes ON/OFF without a restart.
OFF restores native behavior and cancels all Arcade synthetic input, latches,
balance, HUD and combo state. ON requires release/rearm so an already held input
does not become a fresh press. The preference persists normally.

## Acceptance order

Security is complete. Preserve the accepted toggle, automatic skating, board chord
and rotation work. Basic native Y grinding, exits and balance HUD now have fixed-rail
Deck evidence; finish their remaining integration before deferred grabs and exotic
named variants. This table remains the target contract. STATE.md and the existing
native-method matrix distinguish verified behavior from outstanding acceptance.
