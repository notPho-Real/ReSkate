# RC3 native grab controls (gameplay acceptance pending)

The airborne bindings below use the existing native grab lease, original hand-input producer,
and original blendspace coordinates. They do not replay an Ollie, select a fake
score ID, or install animation assets. The runtime acknowledges a request only
after its exact native actor initializes and advances with a positive delta while
the matching native detailed-grab tag is observed.

Ordinary ground now has a separate original-input path: **R3+B** for Double
Ground Grab, **R3+Left+B** for BS Ground Grab, **R3+Right+B** for FS Ground Grab,
and **R3+Down+B** for Coffin. Hold B; release B to exit. Ground B without R3
still cannot brake. These four paths have software tests and exact native actor
acknowledgment, but their animation, exit and scoring captures are gameplay pending.
See [native-ground-grab-entry.md](native-ground-grab-entry.md) for the distinct
ground ownership and acceptance requirements. The airborne selector is unchanged.

| Native pose | Arcade binding | Native family / detailed pose | Tweak X,Y |
| --- | --- | --- | --- |
| BS Grab | B | 2 / 1 | 0,0 |
| Melon | Left + B | 2 / 4 | 0,1 |
| Method | Left + B, B within 250 ms | 2 / 2 | 1,0 |
| Japan | Up-left + B | 2 / 3 | 0,-1 |
| FS Grab | Right + B | 1 / 6 | 0,0 |
| Stiffy | Right + B, B within 250 ms | 1 / 10 | -1,0 |
| Crossbone | R3 + Up-left + B | 2 / 5 | -1,0 |
| FS Nosebone | R3 + Up-right + B | 1 / 7 | 0,1 |
| FS Tailbone | R3 + Down-left + B | 1 / 8 | 0,-1 |
| Tuck Knee | R3 + Down-right + B | 1 / 9 | 1,0 |
| Double Grab | R3 + B, neutral direction | 9 / 33 | 0,0 |
| Double Front | R3 + Up + B | 9 / 36 | 0,1 |
| Double Back | R3 + Down + B | 9 / 37 | 0,-1 |
| Double Left | R3 + Left + B | 9 / 34 | -1,0 |
| Double Right | R3 + Right + B | 9 / 35 | 1,0 |

R3 means clicking the right stick. Hold it when pressing B for the extended
layer. Under Skater > Cheats > Arcade Mode, **R3+B grab** selects the neutral
extended binding. Its default is Double Grab; the choice lasts until restart.
All 56 actor profiles are selectable. R3 plus a direction keeps the shortcuts
above, regardless of the neutral selection. Direction, selection and modifier are taken
from the physical B edge, including an edge buffered during state 200 to 201.
Hold B to hold the grab. Release B to request the original native exit. A short
tap stays pending until the matching animation is acknowledged or the bounded
lease expires. Landing, owner change and Arcade OFF cancel ownership.

The old Indy control slot now displays the actual native identity, FS Grab.
The source does not establish an independently selectable actor named Indy.
Other original table slots are retained but remain blocked until their separate
native entry path is established. They must not fall back to BS or another pose.

Native source: BS blendspace resource 27078, FS 22205, double 30219. BS uses the
original right-hand trigger producer, FS the left hand, and double both hands.
The adapter only writes the established TweakLeftRight/TweakFrontBack input slots;
native StateFlow owns animation, hand state, exit, landing and scoring.

## One-footers and two-foot releases

All rows below are also selectable through neutral R3+B. Hold B through native
entry to sustain the trick; release B for the original exit. The adapter first
establishes the listed native grab, then supplies the original foot input. It
does not request a second jump. Names are the native identities; Superdude uses
the game's `double_superman` one-footer identity.

| Trick | Additional shortcut | Required grab | Released foot | Native OneFooterType |
| --- | --- | --- | --- | --- |
| FS One Foot | Selected R3+B | FS | Left | 1 |
| Frigid Air | Selected R3+B | FS | Right | 2 |
| No Foot | Selected R3+B | FS | Both | 3 |
| Judo | Up-right+B,B | BS | Left | 4 |
| BS One Foot | Selected R3+B | BS | Right | 5 |
| Christ Air | Down-right+B,B | BS | Both | 6 |
| Benihana | Down-left+B | Tail | Left | 7 |
| One Foot Tailgrab | Down+B,B | Tail | Right | 8 |
| Tailwalk | Selected R3+B | Tail | Both | 9 |
| Dog Piss | Selected R3+B | Nose | Left | 10 |
| Nose Judo | Selected R3+B | Nose | Right | 11 |
| Airwalk | Down-right+B | Nose | Both | 12 |
| Superdude | Selected R3+B | Double | Both | 13 |

Repeated B shortcuts use the existing 250 ms physical repetition window.
These rows are **IMPLEMENTED — GAMEPLAY PENDING**. Exact actor initialization,
positive advancement and the matching native OneFooterType are required by the
runtime acknowledgment. The initial hand-grab tag cannot acknowledge a footer.
Native hold, landing and numeric scoring still require hardware acceptance.
Footplant-blend endpoint poses and fingerflip transitions remain separately
accounted for in the coverage report.

Source and control ownership details: [native one-footer entry](native-one-footer-entry.md).

The new six families also use the original held right-stick input producer to
establish their native family. These are the qualified inspected-stance inputs
observed with Arcade OFF, converted from Linux Y to XInput Y:

| Native family | Original input supplied while the lease owns the grab |
| --- | --- |
| Tail | LT + RS Y +28000 |
| Nose | RT + RS Y -28000 |
| Seatbelt | RT + RS Y +28000 |
| Crail | LT + RS Y -28000 |
| Stale | LT + RS X +28000 |
| Weddle | RT + RS X +28000 |
| Rocket Air | LT + RT + RS Y -28000 |

These inputs are family admission, not a second flip or Ollie recipe. The
existing native tweak mechanism selects the exact original pose. The qualified
stance gate remains; opposite/switch-stance admission has not been broadened
without native evidence. Trigger and selector release together. Held LB/RB
continues using the original native LS spin channel independently.

Up+B now reaches Nosegrab and Down+B reaches Tailgrab. Every pose below is also
reachable by choosing its displayed name under **R3+B grab**, then centering LS,
holding R3 and pressing B:

| Family | Selectable native poses |
| --- | --- |
| Tail | Tailgrab, Tail NoseBone, Tail FS Shifty, Tail BS Shifty |
| Nose | Nosegrab, Nose Tailbone, Nose FS Shifty, Nose BS Shifty, Nosegrab (alternate native pose), Crail FS Shifty (from Nose) |
| Seatbelt | Seatbelt, Seatbelt FS Shifty |
| Crail | Crail, Crail FS Shifty, Crail BS Shifty, Crail BS Shifty (alternate native pose) |
| Stale | Stale, Stale Tailbone, Stale Nosebone, Stale TuckKnee, Stale Tuck Knee (right pose), Stale Tuck Knee (middle pose) |
| Rocket | Rocket Air (also Up+B,B within 250 ms) |
| Weddle | Weddle, Weddle Tail, Weddle Nose, Weddle Japan, Weddle (alternate native pose) |

Alternate pose descriptions identify different original actors within the same
native trick identity; they do not invent additional scoring identities. The
machine-readable coverage file gives each actor, source resource, family,
coordinates and exact binding. The Seatbelt BS asset's inconsistent native tag
is still blocked rather than silently relabeled.

The original six-family input/exit observations are recorded in
`native-grab-family-rc2-controls.json`. They prove stock native admission and
return to ground, not RC3 animation initialization or scoring acceptance.

Only BS, Melon and Method have prior controlled native actor captures, at measured
30 Hz; those captures do not establish current RC2 gameplay reliability or native
numeric scoring. New profiles have source and production-composition tests, not
hardware acceptance. No trick in this document is promoted to fully IMPLEMENTED.

See `rc3-grab-coverage.json` for every declaration and inventoried sequence,
including blocked one-foot, cross-grab, fingerflip and context-specific entries.

Rocket Air uses chooser resource 32512 entries 10/11: both original hands
and GrabNose select native family 10 from idle or Nose. Original Skatepedia
entry 4206 specifies that same RS and both-trigger input. Loop resource 2183
owns detailed tag 38 and actor ecc31571-ae62-708b-ecc3-1571ae62708b.
This source join and the production binding are implemented; Rocket was not
one of the eight stock control trials and still needs native runtime acceptance.
