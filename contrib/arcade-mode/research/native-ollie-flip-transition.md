# Native Ollie-to-flip transition

Investigation and bounded prototype: 2026-10-29. Base staging commit:
`f9e61dcfbdfa7587c849cb624e82ede8784b67bc`.
Executable SHA-256:
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
RVA references below are specific to that executable. Resource numbers refer
to the independently decoded original game EBX assets, not invented symbols.

## Root cause and native ownership

`InputSession::request` previously retired the mod's delivered Ollie gesture,
then sent a second right-stick gesture after takeoff. This did not retire the
game's native Ollie. Ground recognition (`GroundTricks_FlipTrick_CG_B4D5A63`,
resource 7664, recognizer RVA 8312f0) and airborne late recognition (late buffer
11776 and `AirStates_LateFlip_CG_AE999AD`, 13831) are separate paths. The latter
waits on native TimeToLand. Sending a new gesture in that state does not imply
a normal flip transition.

The existing mid-flip action group (12910) excludes Ollie/Nollie and requires
FlipChangeWindow/FlipTrickPhase. The original Ollie clip (18703) supplies
neither. Its native TrickLaunch owner also exits before a 500 ms follow-up,
via the EarlyInAir transition (19657, GUID 087485c3-ec18-ec23) or DoingTrick
completion (8385, GUID 28897ec3-86d0-2c51). Changing the selected trick ID after
that exit did not start the requested clip in the negative control.

The native normal transition does exist. In TrickLaunchDefault (5622), the
intent-change expression (24357) sets IsFlipTrickChanged; the self-transition
(16893, GUID 9f243175-560d-bb82) accepts IsChanged and AllowReselection, performs
native entry/exit cleanup and blending, and selects a replacement PTM clip.
Its selection condition is not the restrictive gesture-recognition whitelist.
StateFlow candidate evaluation is RVA 129f360; transition execution is 129e1b0.

## Why selecting that transition alone was insufficient

The destination requests Branch_In_FlipTrickChanged (34822), with its signal
tuner (32229). RVA 12a0860 asks the newly created CDB/PTM subject for tag
interface 317458A7. A hardware trace observed a null tag collection before
the new PTM selected a clip; StateFlow consequently fell back to phase zero.
The resulting normal Kickflip replayed setup and its pop tags.

Changing StateFlow's destination-phase argument did not fix this. Calling the
native generic phase setter (e777d0) also did not fix it: PTM's next evaluation
restored its motion database position. Both approaches were rejected.

PTM evaluation (106a2f0) obtains a native motion-selection result, then commits
it through ed6da0. The result contains its selected database, sequence/mirror
bits, sample index and fraction. Its selected sequence points to the original
actor/tag assets and motion samples. Native actor initialization at ef2f70
uses that sample's starting tick, initializes the actor, and owns reuse,
blending, reference counts and cleanup. The motion path then advances from
the same sample instead of overwriting an unrelated controller-phase write.

## Correction

The bridge is armed only by an admitted ordinary A-release launch and a
validated local owner. It retains the two native Ollie exit candidates while
rising, and while a submitted normal selection is pending. An owned A-to-X
request submits EnumGS.Intent.FlipTrick to that owner's normal native graph;
it does not emit a second airborne right-stick recipe.

During the resulting new local TrickLaunch PTM evaluation, the bridge reads
the selected sequence's original Pop and Jump tags. It passes the first
existing motion sample strictly beyond their inclusive end to native motion
commit. It preserves database/sequence/mirror identity and does not overwrite
CurrentFlipTrick, animation assets, controller phase, velocity or impulses.
Native initialization, blending and cleanup remain responsible for playback.
For the observed clips the inclusive pop interval ended at tick 14, so the
first safe existing sample was 16. This is derived from metadata in production,
not hardcoded per trick or expressed as a timeout.

Zero-delta prediction creates disposable controllers. Only an advancing
evaluation of the controller initialized by this lease retires its intent.
Owner loss, mode/route withdrawal, landing and expiry invalidate the lease.
All three hooks require the exact supported image and verified entry bytes.

## Native evidence and limits

The adjacent JSON records seven bounded debugger trials. Hardware breakpoints
observed PTM initialization and actual positive-delta evaluation, independently
of input output and selected ID. Kickflip (at takeoff, +100 ms and +280 ms),
Heelflip, Pop Shove-it, Hardflip and Impossible began at their post-pop sample.
At the first advancing evaluation the last independently sampled hips vertical
velocity was positive in every trial. Both native late flags stayed false,
exactly one Pop rising edge was sampled, and the skater returned to state 100.
The local cached physics step was 0.033333331 seconds. An original-control
ground Heelflip control established the normal pre-pop path separately.

These are prototype results, not proof that the compiled production DLL or
physical A-to-X mapping has passed end-to-end acceptance. The probe submits
native intent directly. Native samples were taken every 10 ms; absence of a
sampled second Pop is not alone proof of bit-identical body trajectories.
Portable regression tests additionally compare original launch trajectories
and reject new native impulses, and exercise actual input recognition at
A-release, +100 ms and +500 ms. Those tests are not animation evidence.

Remaining acceptance work: compiled Windows runtime, complete CI, end-to-end
physical X timing, native scoring/landing observations and visual blending.
The installed Deck DLL and rollback artifact remain untouched.

## Button-timed prototype follow-up

`physical-native-evidence.json` adds three trials with X scheduled 0, 100 and
500 ms after A release. The probe independently observed BTN_X from the
validated virtual controller through EVIOCGKEY; it did not infer X from a
requested gesture. All three reached a positive-delta normal Kickflip PTM
evaluation with positive hips vertical velocity, both Late flags clear, one
sampled Pop edge and final ground state 100. X-to-evaluation intervals were
348, 259 and 51 ms respectively: the first two include the original native
pop/setup before takeoff. These remain temporary debugger prototypes; the
installed old DLL still emits its legacy gesture, while the probe submits the
normal intent. They do not establish production-DLL end-to-end acceptance.

## Production metadata reader checked against live native data

A bounded read-only capture of the five observed clips was replayed through
the actual C++ `post_pop_entry` template. This exposed an array-alignment bug:
array payloads retain address bit 2, whereas tagged EBX object references do
not. After separating those pointer rules, the production reader returned
entry tick 16 and Pop/Jump end tick 14 for all five original clips. Regression
fixtures now exercise both four- and eight-byte-aligned arrays. This check
validates the production reader against native data, not runtime hook delivery.

## Native hook ABI correction from isolated DLL testing

The b6e918f candidate failed native off-board mount startup in the isolated copy.
A prior-DLL control using the same retained virtual controller reached native
riding state 100/mode 1 with Y; the candidate remained in state 504. This is a
failed acceptance result, not a passing flip trial. Inspection found that the
StateFlow candidate wrapper omitted its fifth and sixth arguments. At entry,
129f360 sets RBP = entry RSP - 1192; 129f6ae reads the flags at RBP+1232
(entry RSP+40), and 129f6bf reads a float limit at RBP+1240 (entry RSP+48).
Caller 129fc52..129fc6f explicitly supplies those stack arguments. They must be
forwarded even with no Arcade lease. The corrected signature is six arguments;
the production callback is compile-time checked against that build contract.

The other two signatures were audited: PTM evaluation 106a2f0 uses RCX/RDX,
XMM2 and R9, returning the output pointer in RAX at 106d2d2. Motion commit
ed6da0..ed6e1e uses RCX/RDX/R8/R9b, has no incoming stack arguments, and
returns AL. A new compiled-DLL native acceptance run is still required.
