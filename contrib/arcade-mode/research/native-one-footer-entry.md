# Native one-footer entry in RC3

## Independent identity correction

RC3-11 Mega Park capture
`608b1dd6d21f10144d331962bc9f37afcbc327df4d33b8aedc62e0fad750a6e4`
pressed Up-Right+B at 550 ms and again at 690 ms after A, releasing B at
960 ms. Original OneFooterType4 appeared at 928 ms. Judo's exact Into actor
initialized, followed by its Loop actor. DetailedGrabType remained unallocated
throughout. No actor acknowledgment was issued: the runtime incorrectly
required that unrelated detail read before examining any footer actor.
Consequently the released input stayed active until state601 invalidated the
air owner at 1.297 seconds; the native footplant produced another flight.

The correction reads only OneFooterType for footer profiles and only
DetailedGrabType for ordinary profiles. IsGrabbing, exact actor identity,
initialization, positive advancement and native ownership remain required.
Missing identity reads still fail closed. The shared observation path is
tested with the unrelated field unavailable for every profile, including B
release before acknowledgment. This fixes an observed acknowledgment gate;
clean Judo release and landing still require a new installed-build trial.

Comparison capture
`0d215f938a3bb101f63dedde9205c5e0a28eb2b0ba251a52fbda7ba41aeca85a`
first performed an ordinary BS Grab. With DetailedGrabType now allocated,
the same Judo request acknowledged its exact Into actor and withdrew the SDK
input while airborne, but still entered a footplant at landing. Expanded
capture `1ee5a000e80039d05f4f177d6712bb2e9b0d1ead05bf0aa60eef9fb4bd0cf33d`
showed original CanOneFoot, LeftFoot and hand intent persisting after neutral
pad output until state601. SDK request withdrawal is not native exit proof.

Original-controller capture
`69f7101cd0673c00472c81d2dc44c70e0a7c824ba5edd1ad67ea1eb571e6db60`
with Arcade OFF reproduced the same Judo footplant and second flight after
physical release. The original AirStates Footplant condition (EBX4307,
RES4771) consumes Bool.State.Is.Footplant; it does not produce that state.
The remaining native airtime/context and footplant commitment requirements
must be resolved without suppressing original physics or fabricating an exit.

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
Ollie-button input and the allowed native GrabType. Their separate input adapter is described in
[native-footplant-entry.md](native-footplant-entry.md). Ground/coffin and fingerflip/varial transitions are
also separate owners and remain blocked in catalog accounting.

## Entry actor follow-up

The RC3-03 Benihana trial reached native OneFooterType7 during its Into
sequence, but never reached the previously tracked loop before the landing
ended in a bail. Entry bodies are distinct original actors: all13 exact
Into actors are now observed as well as their loops. A matching original
entry actor must initialize and advance with positive delta in the owned
local state view while the corresponding OneFooterType and IsGrabbing
are present. Only then can a released physical B request withdraw the native
input without waiting for a loop. No transition or animation clock is forced.
The actor receipt distinguishes Into from Loop; neither substitutes for
held-loop, release/landing or scoring acceptance. The bail cause remains open.

## Repeated entry after Ollie retirement

The physical Up-Right+B,B binding has an unsupported first name (Madonna)
and a supported second native action (Judo). The first press deliberately
submits no substitute actor. A portable reproduction found that retirement
of the original Ollie selector between these presses rejected the second
request's old action generation. The dispatcher now accepts the existing
independent `jump_selection_retired` proof for this unsupported prefix only.
Established grabs still require their own native owner proof. Owner or
selector replacement, landing, another flip and an expired repetition remain
rejected. The reproduction failed before the change and passes at 30/60/120 Hz.
This fixes request routing; Judo actor and landing acceptance remain pending.

The RC3-09 Mega Park Benihana capture reached native OneFooterType7, but the
temporary controller binding was revoked before landing and the skater bailed.
It provided no exact actor acknowledgment. Its uncompressed SHA-256 is
`05bf35eb440c4dbd64aa2f26bb940708a98535a351b93be5333659ca44041cad`.
Do not count the native identity alone as successful one-footer acceptance.

RC3-10 repeated that failure on Mega Park in the changed leading-foot stance:
OneFooterType7 appeared at1.000s, returned to0 at1.226s, and the original
physics entered504 at1.425s. The controller route was revoked near the end
of flight. This ordering does not prove revocation caused the bail: the
original wipeout observer can also invalidate the owner before state504.
Capture SHA-256:
`c67bfe41916dec4eaf79f3408a8ab43f7421a53864b946c9c2a5dbfffe0f6b1f`.
The loaded Benihana entry asset was inspected read-only and contains the
same ActorControllerAsset GUID as the source contract. Missing asset identity
is therefore excluded for that process.

Follow-up diagnostics preserve all admission rules. Controller binding
failure codes1–14 distinguish deadline, source, physical sample, selection,
native proof, owner, owner generation, physics step, native clock, contact
generation, action generation, disconnect, capture and explicit clear.
Footer initialization records at most8 other actor identities per lease and
32 per process. A known actor advancing without initialization in the current
lease is reported separately and never acknowledged. These diagnostics must
identify the missing native transition/ownership evidence before another fix.
