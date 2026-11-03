# RC3 native grab controls (gameplay acceptance pending)

These bindings use the existing native grab lease, original hand-input producer,
and original blendspace coordinates. They do not replay an Ollie, select a fake
score ID, or install animation assets. The runtime acknowledges a request only
after its exact native actor initializes and advances with a positive delta while
the matching native detailed-grab tag is observed.

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
layer; no setting or persistent mode is needed. Direction and modifier are taken
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

Only BS, Melon and Method have prior controlled native actor captures, at measured
30 Hz; those captures do not establish current RC2 gameplay reliability or native
numeric scoring. New profiles have source and production-composition tests, not
hardware acceptance. No trick in this document is promoted to fully IMPLEMENTED.

See `rc3-grab-coverage.json` for every declaration and inventoried sequence,
including blocked one-foot, cross-grab, fingerflip and context-specific entries.
