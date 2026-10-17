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

X and B require independently observed airtime and a fresh press. Ground X
does not start a jump; ground B retains native braking. Neither command requires
A to remain held. Landing, bail, menus and Arcade OFF release synthetic ownership.

Manual pairs accept direct reversal or a neutral gap within500ms, including
moderate stick throws and diagonal thumb travel. D-pad
and left-stick sequences are independent; held forward cannot swallow D-pad
commands. Airborne preparation retains the proven native partial-stick method
through landing, and D-pad Up/Down can correct an established manual balance.
The first grounded left-stick Down is reserved for the pair for up to500ms;
a continued hold then passes through. B braking and horizontal steering stay native.
The entry direction does not immediately push the balance meter: release it or
tap the opposite direction to begin correction. Short taps change the meter's
drift velocity and retain their effect after release. Confirmed native manual
contact keeps the meter alive through score polling gaps. A failed manual keeps
its red edge meter until the native pose exits, including when a protected bail
cannot immediately end it; synthetic stick ownership is still released.

Holding forward keeps the native push button held through the complete native
acceleration sequence. There is no lower software cruise cap or resume threshold;
native pushing owns top speed. Releasing movement or entering an incompatible
state releases the push immediately.

## Flip table — X

Repeated presses select the exact listed variant only where a native equivalent
exists. A dash means this contract assigns no variant to that combination.

| Direction | X | X,X | X,X,X |
| --- | --- | --- | --- |
| Left | Kickflip | Double Kickflip | Triple Kickflip |
| Up | Impossible | Double Impossible | Triple Impossible |
| Right | Heelflip | Double Heelflip | Triple Heelflip |
| Down | Pop Shove-It | 360 Shove-It | 540 Shove-It |
| Up-Left | Hardflip | 360 Hardflip | — |
| Up-Right | Inward Heelflip | 360 Inward Heelflip | — |
| Down-Left | Varial Kickflip | 360 Flip | — |
| Down-Right | Varial Heelflip | 360 Heelflip | — |

The table describes the intended mapping. Current native Deck acceptance covers
Kickflip, Heelflip and Pop Shove-It. Hardflip is unresolved: the authored request
for native9 produced native7, and a separate radius control produced native4.
Neither result verifies Hardflip. Other catalog rows retain their authored
candidates and require independent native identity checks.

## Grab table — B

A single B press during native airtime requests the verified basic BS Grab in
the inspected stance. A held direction or movement stick keeps that basic action
available and preserves steering. This first playable grab control reports its
actual basic identity. The named directional rows below remain the target for
future verified modifiers; the explicit lab selector retains their candidates.
A basic BS Grab must never be reported as a successful Melon or other modifier.

| Direction | B | B,B |
| --- | --- | --- |
| Left | Melon | Method |
| Up | Nosegrab | Rocket Air |
| Right | Indy | Stiffy |
| Down | Tailgrab | One Foot Tailgrab |
| Up-Left | Japan | One Foot Japan |
| Up-Right | Madonna | Judo |
| Down-Left | Benihana | Sacktap |
| Down-Right | Airwalk | Christ Air |

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
