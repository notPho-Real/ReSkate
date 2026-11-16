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
| LT | Nollie context |
| RT | Switch / revert context |
| LB+RB | Native dismount / remount, one edge per fresh chord |
| Up → Down | Manual request |
| Down → Up | Nose Manual request |

Native locomotion supplies automatic pushes when movement intent and observed
speed require them. The player does not need to repeatedly press a push button.
The physical right stick always belongs to the camera; any synthetic gesture used
inside a native adapter is an implementation detail, not another player control.

X and B execution requires independently observed airtime and a fresh press.
Ground X does not start a jump; ground B retains native braking. Neither command
requires A to remain held. Landing, bail, menus and Arcade OFF release synthetic
ownership.
In production, a neutral airborne X press immediately recognizes a basic
Kickflip request without waiting for release. A valid direction matched before
or with that press takes precedence; a direction arriving after recognition
cannot replace the request, even while X remains held. Release cannot repeat it.
A fresh neutral X after an admitted Ollie release can also be recognized during
the existing 160ms takeoff preload, before AIR is observed. This belongs to that
released pop; unrelated ground input cannot borrow it. Native readiness and
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
A qualified ordinary first flip can instead follow its exact admitted pop through
native setup; owner, pop withdrawal and landing checks still cancel it. These
windows add no fixed wait and do not establish native trick completion.

Manual pairs accept direct reversal or a neutral gap within220ms on the ground
or400ms in the air. Ground matching has an explicit55ms usability/sampling
allowance: physical250ms pairs can span267–272ms in30Hz sampled input.
Air lifetime and actuation do not receive that allowance.
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
The first grounded left-stick Down is reserved for the pair for up to275ms including that sampling allowance;
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
| Neutral release | Kickflip | — | — |
| Left | Kickflip | Double Kickflip | Triple Kickflip |
| Up | Impossible | Double Impossible | Triple Impossible |
| Right | Heelflip | Double Heelflip | Triple Heelflip |
| Down | Pop Shove-It | 360 Shove-It | 540 Shove-It |
| Up-Left | Hardflip | 360 Hardflip | — |
| Up-Right | Inward Heelflip | 360 Inward Heelflip | — |
| Down-Left | Varial Kickflip | 360 Flip | — |
| Down-Right | Varial Heelflip | 360 Heelflip | — |

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

## Manual, revert and balance

Up → Down requests Manual; Down → Up requests Nose Manual. Only verified native
observations establish the actual pose. Manual correction uses Up / Down; grind
correction uses Left / Right. Both meters display the actual inertial balance
model, including fatigue retained throughout the same unbanked combo.

RT requests the appropriate switch/revert action for the observed context. Native
revert identity must be observed. A landing revert creates one continuation
opportunity; repeated revert cannot keep extending grace or farming multiplier.

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
