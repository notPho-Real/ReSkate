# Native grab fingerflips and varials

RC3 supplies the original right-stick gesture while maintaining an established
native grab. It never calls the ordinary Ollie-to-flip bridge, writes a trick
identity or rotation count, or creates an animation. This implementation is
**GAMEPLAY PENDING**, including native catch, landing and scoring acceptance.

## Source and ownership

- `AirStates_Fingerflips_CG_B32A9E8` (EBX 23264, expression 7633) requires
  `egf_enablefingerflips`, a positive original `EATVariable_Onboard_LateFlipBuffer`,
  appropriate hand intent, no DangerZone, InAirState and sufficient native
  TimeToLand. Double-hand admission excludes Rocket Air. Airborne Coffin has a
  separate entry path; its four fingerflip identities remain blocked.
  The original intent tree (13508) places Coffin (607) under GroundTricks (274),
  GroundStates (388), OnGround (469). Fingerflips (432) belongs to AirStates (202)
  under InAir (242). Original airborne Coffin Into (15254) requires RideCoffin;
  finding that actor alone does not establish a safe handoff from the ground action.
- `Includes_UpdateBufferedLateFlipLogic` (AG 388, update 11394 / expression 11776)
  consumes original `Input_TrickGesture` (39367). The original gesture producer
  and EAT own the buffer, its trick enum and speed. Arcade never writes these.
- Fingerflips AG 2186 sets `Enum.GS.Intent.FliptrickTypeForFingerflip` in original
  OnBegin 20033 / 5360. Update 15727 / 13347 consumes the buffer once native
  `Bool.State.Is.FingerFlip` is established.
- `CDB_OnBoard_Grab_FingerFlip_Into` (6479) has 26 non-Coffin entry rows. Source
  grab and original gesture determine FingerFlipType, GrabSwitch and destination.
  [Machine-readable joins](native-fingerflip-transitions.json) record every row,
  original sequence and original body actor. FS Varial uses sequence 33917;
  filtering filenames for `fingerflip` alone misses it.

The adapter accepts a fresh physical X press from either the acknowledged
native grab pose or a freshly initialized and advancing original family Into
actor, with matching hand intent, CanGrab and family. The latter proves only
the family required by the original fingerflip CDB, not a detailed tweak pose.
The [original family joins](native-grab-family-entries.json) account for all16
Grab_Into rows and their12 distinct body actors. Eleven non-Rocket actors cover
the nine fingerflip source families, including FS/BS-to-double transitions.
The runtime checks the same local owner/view, exact actor asset, positive
advancement and150ms freshness; a new grab request clears earlier Into receipts.
It rejects footers, Rocket
Air, nollie, ground, another stick owner and conflicting controls. It delivers
the existing source-authored regular Kickflip, Heelflip, BS Pop Shuvit or FS Pop
Shuvit gesture. A temporary settings owner waits for the original getter to
observe EnableFingerflips first. Saved preferences and the settings UI are unchanged.

First use requires the verified original state and identity assets, not already
allocated state pages. RC3-15 read-only capture
`11af1d8978bcfc6e7fdac2f4616f77feffa89072a6b9ff90b11f74ed93a205d6`
found both handles0 before any fingerflip. Requiring their values before sending
the original gesture made entry circular. The adapter now records state and
identity availability independently from the required source-grab observations.
Unknown values permit preparation only: native entry must be observed, and
acknowledgment requires known active state, exact identity and actor advancement.
Losing the state after entry withdraws input rather than treating it as a catch.
The original native entry predicates and failure deadlines remain unchanged.
The first-use allocation correction passed Windows and RC3-18 hardware
admission. Capture
`7e4b9df898e3ab2952a7d9578d2a9442b6a25073efb2e3a5e1ade54200384f70`
accepted BS Fingerflip identity11 input on the first attempt after launch, but
never entered the native fingerflip actor. Its BS hold loop was acknowledged
at1.576s, X arrived at1.60s, and native TimeToLand fell from0.31 at1.611s to0.24
at1.679s while the authored gesture was still being delivered. The native
time-to-land predicate remains unchanged. Earlier family-Into admission removes
the artificial wait for the hold loop; its Windows build and native execution
remain pending. Request admission is not successful trick evidence.

During entry the old grab's tweak writes are restored and suspended. The native
fingerflip can leave IsGrabbing without ending its input owner. Acknowledgment
requires the exact initialized body actor, positive animation advancement and
matching native FingerFlipType. The adapter follows the native destination
family, supplies its hand input and holds RS neutral during the catch. Return
requires FingerFlip to end, the expected destination, IsGrabbing and CanGrab
before resuming the ordinary grab lease. Neither input nor an enum alone is success.

B release, landing, owner loss, Arcade OFF and stale input withdraw controls;
original StateFlow performs cleanup. Bounds are 800 ms for unacknowledged entry,
2500 ms total and a 150 ms input heartbeat. These are failure bounds, not
animation durations or fabricated completion times.

## Controls

Establish the source grab and keep B held. Press X with the **current** direction:
neutral or Left = native kick gesture, Right = heel, Down = BS shuv, Up = FS shuv.
Centering the stick removes the direction that originally selected the grab.
Release X before another transition. A rejected X is consumed without fallback.

| Held grab | Neutral / Left + X | Right + X | Down + X | Up + X |
| --- | --- | --- | --- | --- |
| BS | BS Fingerflip | BS Finger Heelflip | BS Varial | BS to Weddle |
| FS | FS Fingerflip | FS Finger Heelflip | FS Varial | FS to Stale |
| Nose | Fingerflip | Finger Heelflip | Nose to Seatbelt | Nose to Seatbelt |
| Tail | Tail Flip | Tail Heelflip | Tail to Crail | Tail to Crail |
| Stale | Stale Flip | Stale Heelflip | Stale to FS | Stale to FS |
| Weddle | Weddle Flip | Weddle Heelflip | Weddle to BS | Weddle to BS |
| Seatbelt | Seatbelt to Nose | Seatbelt to Nose | Seatbelt to Nose | Seatbelt to Nose |
| Crail | Crail to Tail | Crail to Tail | Crail to Tail | Crail to Tail |
| Double | Double Grab Fingerflip | Double Grab Heelflip | Double Grab BS Varial | Double Grab FS Varial |

The last row uses the native double-**grab** family. FingerFlipType names
`DoubleKickflip` and `DoubleHeelflip` do not demonstrate two ordinary rotations.
Ordinary X-X has a separate held-flip owner.

## Acceptance

Production-header tests cover all 26 identities at 30/60/120 Hz, delayed
settings, gesture delivery, wrong/absent/nonadvancing actors, native destination
return, clock/owner withdrawal, startup-held and grounded input, single physical
dispatch and rejection without ordinary-flip fallback. Injected observations
are not gameplay evidence.

For each table cell capture the request, exact native actor advancement,
FingerFlipType and catch/destination. Hold B through return, try a second
transition, release B and land. Repeat with B released during entry and near
landing. Verify native combo/score and original vertical trajectory. Held X,
X before grab acknowledgment, footers, Rocket Air and ground B+X must not start
a substitute flip or another Ollie. No successful RC3 fingerflip animation or
scoring capture is yet claimed.
