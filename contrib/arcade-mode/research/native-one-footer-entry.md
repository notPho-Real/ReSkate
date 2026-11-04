# Native one-footer entry in RC3

Scope: pinned build 20260929, existing airborne grab owner. No extracted game
assets or decompiled code are distributed with this implementation.

The original CDB_OnBoard_OneFooters_Into (resource 514) selects 13 identities
using GrabType and the original LeftFoot, RightFoot and BothFeet intent flags.
The loop database (1796) resolves through wrappers and blendspaces to the body
actors recorded in `arcade_grab_transition.h`. Original StateFlow nodes 15071,
12656 and 12271 own Into, Loop and Exit. CanOneFoot remains a native result;
the adapter does not write it or use it as a prerequisite for sending the foot
input that may produce it.

The pinned gamepad update expression (EBX 34108, RES 7573) publishes
Input_LeftFootGrab, Input_RightFootGrab and Input_NoFootGrab. Their producers
are at expression PCs 0x2bec, 0x2cfc and 0x2d28 respectively. The first two use
the original X/A inputs, with swap and front/back-foot preferences; NoFoot uses
the native slide input. X+A is the separate HippyJump combination and must not
be substituted for BothFeet.

The adapter temporarily holds the existing `egf_invertxaonboard` and
`egf_frontbackfoot` preferences false while a footer lease owns the input. In
that original configuration, X selects the left foot and A the right foot;
B selects both. These are output controls, not extra physical button presses
fed back to the Arcade recognizer. Profile values and the Settings UI retain
the player's preferences. Another option owner is never displaced. Input waits
for both values to be observed from the gameplay getter after the request;
requesting a hold alone is not proof it has reached the native producer.
Cancellation, landing and Arcade OFF release the holds, with the existing
300 ms expiry as a lost-heartbeat fallback.

Each profile first supplies its original hand/RS family selector. Only a
qualified native IsGrabbing, CanGrab, matching hand pair and matching GrabType
allow the foot input to start. Once started, the original graph owns its
CanOneFoot, animation and cleanup lifecycle. The normal bounded lease keeps a
short physical B tap pending until exact native acknowledgment or failure.
No one-footer identity, scoring field, animation clock or physical velocity is
written. A missing optional OneFooterType asset cannot disable ordinary grabs.

## Acceptance state

Production code and portable input/lease tests cover all 13 identities at
30/60/120 Hz, delayed native admission, settings consumption, wrong family or
hand pair, wrong actor or identity, B release, landing, owner loss and native
shoulder composition. These are injected observations, not in-game evidence.

Hardware acceptance remains outstanding for every newly mapped footer: exact
actor initialization and advancement, visible pose, sustained hold, native
exit, landing and numeric scoring. The previously staged DLL at source
7126426 does not contain this follow-up implementation.

No Comply and Boneless use separate native entry mechanisms. The pinned input
producer gates NoComply with the `nocomply` preference (default false) and a
right-stick region. Boneless requires one hand-grab input with a foot-plant or
Ollie-button input and the allowed native GrabType. They are not implemented
by this one-footer change. Ground/coffin and fingerflip/varial transitions are
also separate owners and remain blocked in catalog accounting.
