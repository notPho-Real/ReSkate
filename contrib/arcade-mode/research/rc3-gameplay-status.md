# RC3 implementation status — incomplete candidate

## RC3-14 exact-source Deck verification

Source `c1658974816104a83be1cd382e52582829781398` passed development run
42136769290, targeted run42136769209 and upstream integration run42136769265
attempt2. DLL `bd61437f16b39c2ce9466811e2af6fcb2f1259482a59db2fecb790584e564460`
was installed after normal game and launcher closure. The restarted process
mapped the same DLL inode/device/hash. RC2 and previous candidates are retained.
Tests below used `skate3_megapark`, with native and cached steps both1/30 second.

Capture `bbf81aae51f0e39e4cd490a55752fb88292f3edd4b1d50a1f647fa9101b8575d`
compared single X with X presses150ms apart. Single measured0.99150 rotations
and landed. Double measured1.99157 but bailed; original held intent remained
unallocated and the extra count stayed0. Extending the unadmitted input to its
existing200ms deadline did not solve this timing case. PrepareHeldFlip was true
at approximately696–769ms after A; the second X was at670ms. A later held-state
flag alone is not the original EAT intent or a successful double.

The90ms control, capture
`ffa985d6e3e9400211dacceb93fd59ca5220b18eab52360804eb27132c68a2d7`,
measured1.99364 rotations, reached one native extra rotation, exited held state
and landed at1.42588 seconds. Exact held-loop callback and scoring remain open.
The input window therefore remains unreliable; RC3-14 is not gameplay accepted.

First-flight Judo capture
`49b4e47de9edb9f85751d568497ac9807db1904232024a81f3544c98f6c948f8`
did not start a native grab. OneFooterType was initially unallocated. The same
input after ordinary grabs, capture
`fedfcafdef551587be715b8719896e066257c449707e14943c4a7a4fbc6bcf1e`,
initialized and advanced the exact Judo Into actor with OneFooterType4, withdrew
the SDK input in air, then still entered native footplant601 before returning
to ground. The first-use and footplant failures are separate.

Source review found another circular dependency in native_grab::request:
one-footer input required an already allocated OneFooterType state value.
The next correction requires its verified original asset, allowing the native
producer to create the state. Acknowledgment still requires the actual identity,
matching initialized/advancing actor and native grab ownership. No state page,
identity or actor is fabricated. Existing20 portable suites pass; they do not
execute this Windows adapter. Windows CI and a fresh-process first-footer
hardware regression are required before accepting this correction.

Default B and Up+B capture
`4743399db19ea08ffdb3e1f9ccd30271a88bc8fddba01444e1bb129933543c97`
initialized/advanced the original BS and Nosegrab actors, withdrew SDK input
in air and returned to ground. Both actor receipts occurred during descent;
prompt ascent entry, full native cleanup and scoring remain unaccepted.

## Rocket reselection timing

RC3-11 Mega Park capture
`07f579fd1a2ef387671e22aa15af37007a6a77d6a2eba1dc5e1558393a950e81`
pressed B at550 and640ms after A while holding Up. Requests13 and14 in
episode117 recorded88ms between physical B edges. The original
GrabIntoReChoose flag became true; native family6 changed to10, detail38
followed, and ground100 returned at1.327s. Neither expected Rocket Into nor
Loop actor callback was captured. Animation, native exit and scoring remain
unaccepted; this is not a new complete actor receipt.

The original Loop-to-Into transition25340 requires CanGrabDouble and excludes
active GrabNose. The early Into self-transition2343 instead requires the
original GrabIntoReChoose signal. This distinguishes the successful early
native identity change from the later repeated-B failures. A first B at480ms
preceded state200 and produced only one dispatched request in capture
`659910b80c679f462e1eace702f4ed33e996c9eb90ad5c7672f1482dc1c9bde6`;
it does not prove supported aerial repetition. The next observer revision
extends the existing bounded unknown-actor diagnostic to Rocket, retaining
the same local owner/view checks and no acknowledgment of unknown assets.

## Repeated-flip timing correction

RC3-11 comparison capture
`8020ed494b0ea45894543218c30003693c9b4654198b3c86c84eed82365d0438`
used original stick controls with Arcade OFF. The held Heelflip established
the original held intent, advanced its extra-rotation count, withdrew intent
and returned to ground. The single control did not enter held intent.

Earlier repeated-X capture
`e2829dd7808d488d0dee4101e1b462098cbfd8077f33c6ce353a823dde340912`
used presses at 520 and 610 ms after A on Mega Park. Native board axes measured
1.99135 rotations, original held intent became true, the extra-rotation count
reached one, and the skater returned to ground at 1.39448 seconds. Heading
changed only 1.53 degrees. The exact held-loop callback and scoring remain
unverified. This successful earlier input does not erase the later failure.

The candidate correction gives every unadmitted repeat the existing 200 ms
input-readiness budget. Previously, seeing PrepareHeldFlip once allowed its
next false sample to withdraw input after only 88 ms, before admission. The
native predicate still gates the original action; this changes no animation,
counter, physics or admission window. An unadmitted input expires at 200 ms;
an admitted input still withdraws at one native extra rotation or native exit.
The observed boundary regression fails with the previous implementation and
passes with this correction. All 20 local targeted suites pass. Windows CI
and a repeated late-input Deck trial are required before accepting this fix.

## RC3-11 Mega Park tests

Source `01ecfd50aaf66e5e1bcccfbe750efec24f9ee5b0`, DLL
`ecdc25a2a3c07a0ec23548d740a20d7a70652c89dce3ce7ebe202adb18cba984`,
passed all its CI gates, was installed after normal game/launcher closure,
and was verified loaded by file hash and mapped inode/device. All trials used
`skate3_megapark`. The earlier supposed DayZ blocker was Steam artwork;
process inspection found no running DayZ game. No other game was interrupted.

Repeated-B capture
`05dc351bee2f5328e54dcad164a951a61d95da5f27fc69df6b8f016bd708b578`
initialized and advanced the correct Method and Stiffy actors and returned
to ground. Judo now reached OneFooterType4 and initialized/advanced its
original Into actor. After input withdrawal, however, it entered state601
footplant and a second200/201 flight before ground100. Its clean release and
landing are not accepted. Rocket's repeated-B request still selected native
Nosegrab21, not Rocket38. These results do not establish catalog completion.
Coverage now includes17 exact actor receipts, explicitly separating the Judo
Into receipt from unverified Loop and exit behavior.

Flip capture
`5975d948a8da18dc293e699df09d743361eed86c08e77a4160a55b64f3205441`
measured0.9911 actual board rotations for single X and1.9916 for X/X. Single
landed; double bailed. The double's original held intent remained unallocated,
its extra-rotation count stayed0, and no expected loop actor was acknowledged.
Its input was withdrawn when PrepareHeldFlip closed before held state appeared.
This is a failed admission/catch/landing case despite the visible two rotations;
do not extrapolate RC3-09's successful count/catch capture to every attempt.

Grounded Up/Down entered Manual without Ollie, retained native contact through
RT, and initialized/advanced original tail-fakie pose actors. Capture
`18c06ead380770cdb7277707cf8f9283fd5dd07f051eac6dde90c01d2509f8d2`
stopped on stale balance telemetry while the manual remained active. The
reader now distinguishes reused log inodes by record prefix. Retry
`299533bc472605776fef5ab0fc858854eb3697d2efa8872bc7c05a11064ae96c`
retained the manual through RT but reached an obstacle and bailed.

Down/Up also entered Nose Manual without Ollie and retained nose contact
through RT, with original nose-fakie pose actors advancing. Capture
`82b49d9554e8d0c19dec0a732cdda728c94f304b5759be448a2b3493b22e2b90`
used a shorter approach. The native fakie indication changed during the hold;
later balance corrections failed and the skater bailed. Neither this nor the
Manual retries establishes a fresh ten-second fakie hold or final rendered
blend acceptance. Numeric scoring remains unverified throughout these trials.

## RC3-10 native follow-up

Source `9a2d5ba63d0cb3c8c3ea85d22399c4efacfefbcc`, DLL
`2b176ac71f7161a7bd3fcc59b6feeb1f20432671726a6bee7424ccb31d480aa6`,
passed all required CI gates and was independently verified loaded. Tests used
`skate3_megapark` and ordinary physical controller input.

A rolling ground RT changed native stance from bit0 to bit0x20 and heading by
about175 degrees. Default BS Grab then initialized and advanced its original
actor in the changed stance. That trial subsequently collided with the park
wall and bailed; it does not establish a successful landing. After ordinary
on-foot recovery onto clear ground, Melon, FS/Indy, Japan and Tailgrab each
initialized and advanced the correct actor with stance bit0x20, withdrew
intent and returned to state100. The initial stationary RT attempt did not
complete a half-turn and is excluded from stance-change acceptance.

The rolling capture's uncompressed SHA-256 is
`abdbc5829fabd1d96696f520e8a125744118e56cc140b853a58449e41da2cc5b`.
The separate four-grab capture's uncompressed SHA-256 is
`228017e834ed36eaf40664050562d5ea235edfbc1346bc06e87007d619d92d49`.
These tests do not establish numeric scoring, every grab's changed-stance
behavior, prompt ascent entry, or the full native exit lifecycle.

Footplant capture
`044613996a45b3c43040c35db4411665a8fe9de5c714f8681fa45092a67904e9`
observed Boneless establishing native left-hand/family1, original Boneless
intent, state602, then states200/201 and ground100. Board height reached0.868m.
This is native entry and launch evidence, not exact PTM animation or scoring
acceptance. An earlier attempt failed and remains recorded. No Comply failed
in both attempts: its original CanNoComply and Is.NoComply state handles were
unallocated in the observed owner, and no launch occurred. The adapter must
resolve the original prerequisite producer; tests are not a substitute for it.

RC3-09's exact grab actor receipts now cover16 distinct actors. The four added
receipts are FS Nosebone, FS Tailbone, Double Grab Left and Double Grab Right.
Their correct actors advanced and matching intent withdrawal was observed;
the coverage generator retains gameplay-pending status and scoring limits.

## Mega Park follow-up

The owner-selected `skate3_megapark` is the required gameplay test map. On the
installed RC3-04 DLL, a grounded Up/Down gesture entered native Manual, retained
tail contact through state101-to100, and observed native fakie after RT. The
neutral-stick trial then reached the Arcade balance endpoint and bailed; the
balance failure was logged explicitly. This is neither a ten-second hold nor
proof of the final native posture. Future sustained trials must provide ordinary
physical balance correction and record it.

Tracing the original Manual SuperLayers subjects found a gap in the pose
observer: each of four native balance/turn blendspaces references nine original
pose actors, separate from the four medium loop actors previously watched.
`native-manual-pose-observation.json` records the source links and exact actors.
Observation now includes these36 actors with the same local-owner, state-view,
initialization and positive-advancement requirements. Blend contribution remains
unverified; observing an actor is not proof it supplies the final rendered pose.
No native pose, contact, orientation, animation time or blend weight is written.

## RC3-04 live follow-up

Source `baf44ab2c908dd49fc1acd2676b149d5df1f5845` passed all required
Windows, Linux, macOS, integration, targeted and Deck packaging gates. Its
DLL `f0f1fefc4614d029cbd195007802d9bdc3f3cbbbfe8a9b1dff1139b6a4054e07`
was installed after normal UI shutdown and independently verified loaded.
All earlier rollback copies remain preserved.

At measured native/cached 1/30-second steps, a single X retained zero extra
rotations and landed. X/X entered original held intent/state, reached one extra
rotation, withdrew input and landed without an off-board state. The exact held
loop actor was not acknowledged, so this is not yet accepted Double Kickflip
animation or scoring evidence. The original Kickflip/Heelflip loop actor GUIDs
were rechecked against the original source records; they match the contracts.
Bounded initialization diagnostics now distinguish missing tracking, ownership,
state-view and duration proof for these exact actors, without admitting any
unproven animation or changing the native rotation counter.

Default B and Up+B again initialized/advanced their exact BS and Nosegrab actors,
with air release and return to ground. A moving manual trial crossed a curb and
lost contact before its revert; it is excluded from fakie posture acceptance.
The four original manual loop actors receive the same bounded initialization
diagnostics. Local native captures remain separate from software fixtures.

## RC3-03 native follow-up

Candidate `de837990737f202658540839e621b2e2179b1372` passed its exact-source
Windows, Linux, macOS, upstream integration and targeted CI gates. Its installed
and loaded DLL SHA-256 is
`2e65b3cd7fbe68ded45f8e707bd3fb0d643af80532a3cd25690dd75dd08100e2`.
Controlled trials at measured native/cached 1/30-second steps established:

- Default B and Up+B initialized and positively advanced their exact native
  Basic BS and Nosegrab actors, withdrew intent in air and returned to ground.
  Up+B now selects family6/detail21 rather than the previous BS actor. Scoring
  and the rest of the catalog are not accepted by those two trials.
- Ground Up-to-Down entered a native manual without Ollie. The actuator stayed
  confirmed through native101-to100 during RT. The neutral-balance trial ended
  off-board afterward; rendered fakie posture and ten-second balance acceptance
  remain open. The second Nose Manual trial correctly refused off-board entry.
- Single X remained native Kickflip and landed. X/X entered a held animation
  state but completed zero extra rotations and ended off-board. This remains a
  failed Double Kickflip, not a successful feature.

The repeated-input route waited for PrepareHeldFlip before beginning the
original stick gesture. In the failed capture, its window lasted roughly74ms;
input was withdrawn before original held intent became available. Existing
stock-control captures establish the gesture before this window. The next
correction starts gesture preparation at the second physical X once the first
native animation identity is verified. Original EAT still admits the held
action; preparation without its window expires after200ms. Closing the window
without admission, owner/flight loss and the existing1200ms maximum still
withdraw input. No native intent, rotation count, actor time or physics is
written. A new regression fails against RC3-03 and passes with this correction.
Native repeat/catch acceptance of this timing correction is still pending.

This is an input-ordering correction using the existing source mechanism:
`native-held-flip-source.json` identifies original EAT condition23124,
action10792 and gesture query0x8312f0. The pinned upstream tree contains
held-flip scoring/Trainer reads but no repeated-X adapter; the existing fork
survey supplies no alternate verified adapter. No upstream revision changed.

RC2 rollback source is `2ffb006c73671466bfcdc15395ba96939de44f7d`.
Its preserved DLL SHA-256 is
`420cdfae31b76522ccb97f962a2ed288b787e854161c054ad6ddb71e8d144676`.
RC3 branches from that revision without updating upstream. RC2 remains unchanged.
The owner subsequently authorized Deck installation. RC3-02 source
`0747f57991a3d2dea2101c595e000256d4ebe087` was installed with DLL SHA-256
`89f9f391f8991008d048cbdeabf5d50f134fde549ede447528c050be289e61a3`.
The running process's mapped file identity and hash were independently verified
after installation. The separate RC2 rollback remains preserved.
This candidate does **not** satisfy the full RC3 mission and
must not be promoted as the accepted baseline.

## Owner gameplay rejection, 2026-11-04

The owner reports that repeated X does not produce Double Kickflip, added grabs
do not work, grounded opposite-stick gestures fail, and RT ends an established
manual. This is feedback on the verified current DLL, not a stale installation.
The mechanism and coverage descriptions below are not acceptance evidence.

The captured manual revert drops the existing actuator about150ms into native
state101, before returning to100. Two production defects were reproduced:

- The ordinary contact-loss timeout also ran during the separately admitted
  native revert lease. It now retains original partial RS pressure for that
  bounded transition, without renewing contact evidence or inventing a pose.
- Client cleanup used the persistent ended-episode state to cancel input every
  tick. New grounded first strokes were erased before the opposite stroke.
  Cleanup now consumes the terminal observation once. New physical gestures
  can open acquisition; new native wheel contact still admits the next episode.

Both new regressions fail before their respective fixes. The second executes
the extracted production client update, native observer and input facade at
30/60/120Hz. These source corrections have not yet been installed or accepted
in gameplay. Native fakie posture, double rotations and added-grab execution
remain unresolved acceptance requirements; no additional success is inferred
from the existing software tests.

### Controlled reproduction and source corrections

The owner authorized direct Deck testing. On the same installed RC3-02 DLL,
four controlled trials on clear pavement independently observed these results:

- Single X entered native animation Kickflip, retained zero extra rotations,
  and returned to ground.
- Two X edges also retained zero extra rotations. Native PrepareHeldFlip was
  true, but Arcade never started its held-input actuator. The native animation
  reported Kickflip while the physics selector correctly retained Ollie. The
  repeat path incorrectly demanded Kickflip from both. It now admits the
  verified animation identity while retaining the separate physics-owner and
  flight-continuity checks. The state-200/mode-0 takeoff corridor also no longer
  cancels this owned follow-up before state201.
- Default B initialized and advanced the exact Basic BS actor, released, and
  returned to ground. This is evidence for that tested default binding only.
- Up+B requested Nosegrab but initialized Basic BS. The original Nose selector
  became true one physics step after hand admission had already chosen BS.

An Arcade-OFF comparison varied only selector/trigger ordering. Simultaneous
input reproduced family2. Selector-first input observed the original Nose flag
before the hand, entered native family6, released, and returned to ground.
Both native physics and cached step were 1/30 second. The correction now sends
RS first and waits for the original family-selector acknowledgment before the
hand trigger. Nose, Tail, Crail, Seatbelt, Stale, Weddle, Rocket and dependent
one-footer profiles share this mechanism; an unknown selector cannot admit a
hand. Existing hand-only families retain their original immediate input.

The new regression tests fail against the previous implementation and pass with
the corrections. They include each selectable profile at 30/60/120Hz and the
animation/physics identity disagreement captured above. These corrections are
source changes awaiting a newly built DLL. The original-control comparison
does not prove the corrected Arcade Nosegrab actor or any new grab's scoring.
Actual repeated-X extra rotation, fakie posture, ground-manual reliability and
all new grab lifecycles remain gameplay acceptance requirements. Capture hashes
and limitations are in `rc3-gameplay-reproduction.json`.

## Manual recognition

Production uses a 450 ms opposite-stroke window on ground and in airborne
preparation. Each deliberate vertical excursion needs approximately 40% travel
(positive 12800 / negative 13057 raw axis units). The manual cone permits
horizontal magnitude up to 1.5 times vertical magnitude. Natural diagonals,
neutral crossings, unequal magnitudes and quick reversals retain the original
physical edge and consume it once. Native acquisition/contact deadlines and the
native Manual/Nose Manual actuator are unchanged.

The added regression samples 20 trajectories in each direction at 30/60/120 Hz,
both grounded and airborne: 240 gestures. It also checks startup-held input,
horizontal steering, unrelated X, stale gestures and duplicate polls. Ten-second
balance input does not retrigger recognition in the fixture. These are input and
composition tests; actual grounded entry, landing and ten-second native balance
acceptance remain outstanding.

## Native grabs

The airborne adapter now describes 57 exact actors: the original 44 in BS, FS,
double, Rocket, Nose, Tail, Crail, Seatbelt, Stale and Weddle blendspaces plus all
13 native one-footer identities. The blendspaces cover 37 detailed identities
and seven additional original actor variants sharing those identities.
`rc3-grab-bindings.md` lists all reachable controls. The extended layer is R3
held at B press. The physical edge retains that modifier and direction through
takeoff buffering. A menu selector exposes every actor through neutral R3+B;
its default retains Double Grab. Original default, directional and repeated-B
slots remain; Up+B and Down+B now enter genuine Nose and Tail.

The adapter delivers the correct original hand-trigger pair and the inspected
blendspace tweak coordinates. It requires qualified local owner/air episode,
original native admission, exact initialized actor, positive native advancement
and matching native detail before acknowledging. Original animation cleanup and
scoring remain authoritative. No jump/velocity writer or scoring writer was added.

`rc3-grab-coverage.json` accounts for all 171 declarations and all 300 sequences
in the existing inventory. The namespace counts overlap. There are 80 declarations
marked IMPLEMENTED — GAMEPLAY PENDING: 37 detailed identities, 13 one-footers,
four ground grabs and 26 fingerflips/varials. Another 77 declarations are
ALIAS/DEPENDENT, and 14 remain BLOCKED. These are declaration counts, not a sum
of unique tricks. Of the original 300 sequences, 83 have mapped entry/loop
mechanisms and 217 retain specific context or lifecycle blockers. Every
sequence has an explicit status, identity links and blocker. Unknown execution is
not classified as UNSUPPORTED. The original inventory itself does not prove that
every executable game context has been discovered.

BS, Melon and Method retain prior controlled initialization/advancement and exit
captures at measured 30 Hz, linked from the coverage file. Those captures do not
establish which two grabs the owner currently finds reliable in RC2. No new native
capture, numeric scoring evidence, or claim of current gameplay reliability is
made for RC3 here. Unit-test acknowledgments are injected observations, not game captures.
New stock RC2 controls independently entered all six additional families through
original RS and trigger inputs, observed native grab state/detail, original exit,
released hands and return to ground. Measured physics and cached step were 1/30.
`native-grab-family-rc2-controls.json` records that evidence and its limits.

Remaining mechanisms:

- Nose, tail, seatbelt, crail, stale and weddle now route original RS/trigger
  family entry into resources 23974, 3230, 25388, 31357, 23252 and 33124.
  Their 27 added actor profiles have production routing and lifecycle contract
  tests; RC3 exact actor, rendered pose, native scoring and flight comparisons
  remain pending. Other stance admission remains guarded.
- All 13 one-footer identities now establish their native grab and then deliver
  original left/right/both-foot input, with observed preference settings and
  exact original loop actor acknowledgment. All are available in the R3+B
  selector; existing repeated-B shortcuts remain. Gameplay acceptance is pending.
- The 26 non-Coffin fingerflip and varial identities use hold B plus X with the
  current direction. Original Input_TrickGesture and EAT LateFlipBuffer own entry;
  exact original actor and native destination observations govern the handoff.
  See `native-fingerflip-entry.md`. All four airborne Coffin fingerflip variants
  remain blocked on a verified Coffin handoff; they cannot borrow ordinary double grabs.
- Ground FS, BS, Double and Coffin use a separate native input owner and exact
  actor checks. Bindings are R3+Right+B, R3+Left+B, R3+B and R3+Down+B. Their
  native exit/scoring and ground-to-air handoff acceptance remain outstanding.
- The existing names Madonna, One Foot Japan and Sacktap have no established
  independent native identity in the inspected catalog. They remain blocked;
  another trick's animation has not been substituted.
- Resource 15021's seatbelt BS-shifty sequence emits native detailed tag 24
  (Nose BS Shifty), rather than the expected 17. Its original Seatbelt blendspace
  places that exact actor at X=+1, Y=0. The selector now exposes this native pose
  through R3+B without manufacturing tag17 or replacing the actor with Nose.
  Runtime animation, exit and scoring acceptance remain pending.

## Fakie manual posture — correction awaiting native acceptance

The RC3-03 follow-up exposed a rejected native observation: adjacent frame flags
were both1 while riding and both0 after another native stance change, with the
exports agreeing. The inherited upstream exclusivity test rejected both cases.
The orientation reader now validates their Boolean range and export agreement
without assuming they are exclusive. Fakie still requires the separate frame
flag, physics bit and export to agree. The revised regression fails before this
correction and passes afterward. No native stance value is written.

A moving Manual-to-RT capture then observed original RidingFakie become true,
the corroborating physics/frame flags change, and Manual contact0x4000 persist
through state101-to100. A slower trial nearly stopped and did not set native
fakie; this is not interchangeable with the moving case. Exact pose acceptance
now also has a read-only observer for the four original medium-speed manual
body actors, using the existing fingerprinted ActorController callbacks and
local-player state-view ownership. It records initialization and positive
advancement alongside contact and orientation; it never selects a pose.

The native game has distinct `OnBoard.Manual.T.Fakie.Loop` (33850) and
`OnBoard.Manual.N.Fakie.Loop` (25379) nodes. The formerly unresolved condition
`f9a95db4-e022-f765-f9a9-5db4e022f765` is `Bool.Physics.RidingFakie`.
`Enum.Intent.Manual` selects Tail=1 or Nose=2 independently of that condition.
The original manual intent conditions read `Input_BoardAdjust`; their Tail/Nose
identity is not exchanged merely because native travel becomes fakie.

RC2 explicitly inverted RS pressure following native 101-to-100. RC3 removes
that inversion and its 200 ms crossing. The original Manual or Nose Manual
pressure continues while the original native graph selects its fakie pose.
No animation, stance Boolean, physical heading, contact bit or HUD label is
forced. This corrects the input routing mismatch; rendered posture acceptance
is still outstanding.

The handoff now requires the original and returning corroborated native fakie
observations to differ. The general SDK reader uses the same native frame,
physics-context and exported stance agreement checks as upstream
`plugin_game_runtime.cpp::skater_frame`. Leading foot alone is insufficient.
An unknown or unchanged fakie state cannot acknowledge the handoff. A return
without orientation proof expires after 1100 ms, rather than keeping a pending
manual transition indefinitely. Consecutive original physics contacts still
establish completion independently of input and score-label delivery.

The portable orientation reader has negative controls for missing sources,
contradictory native/export flags, invalid pointers and offboard flags. Session
tests cover both initial orientations and both manual types, unchanged native
fakie despite changed leading foot, unknown stance, missing 101, owner change and
observation gaps. The composed lifecycle tests cover chained reversions, native
LS trajectory preservation, contact bounce, delivery gaps and ten-second holds
at 30/60/120 Hz. These are synthetic native observations, not rendered evidence.

## Repeated flips — production input path awaiting runtime acceptance

The source audit resolves HeldFlip Into/Loop/Exit and 16 native base identities,
including ordinary Kickflip and Heelflip. HeldTrick condition resource 23124 and
action 10792 own admission. The compiled condition (`0x1de580b00d02aaed`) has now
been decoded and its native call registrations inspected. It requires a matching
original gameplay-input gesture with its held flag, PrepareHeldFlip, enabled held
flips, and the original catch/time-to-land/hand/dark-state checks. The ordinary
Arcade bridge delivers flip intent without producing that matching native gesture.
The push-button alternative applies to IDs 33/34, not ordinary Kickflip/Heelflip.

Live ordinary StateFlow entries are 17680/14839. Earlier examples 33022/7770
require Bool.Debug.False true and cannot justify normal runtime entry. Original
Kickflip/Heelflip hold sectors are resolved from resources 38264/37949. The native
EAT owns HeldFlip lifetime; original animation tags own TrickHeldFlipCount. The
stock-control captures below establish its units; RC3 exit/catch timing still
requires runtime correlation.

The native-control setup is now repaired. With Arcade temporarily OFF, native Y
first recovered the fallen skater, then mounted the board. Separate stock single
and held comparisons reached native airborne states 200/201 and landed in 100.
Both native physics delta and cached step were 1/30 second. The native held action
and held state became true only in the held trials; the original counter advanced.
`native-held-flip-rc2-controls.json` records the changes and private capture hashes.

The counter measures **additional** rotations: ordinary Kickflip remained zero,
the short hold reached one and the native HUD displayed Double Kickflip, and the
long hold reached two and displayed Triple Kickflip. Native HUD scores were 15,
33 and 33 respectively, within one ongoing line; these are observations, not a
general score formula or an independent scoring-owner capture. Exact actor and
board rotation trace acceptance remains outstanding. All trials neutralized and
closed the virtual controller, restored Arcade ON and preserved the RC2 DLL hash.

Prepared gestures now retain a validated native Kickflip/Heelflip held endpoint.
Playback can hold that endpoint through the ordinary neutral deadline, then
withdraw it for native catch. It preserves the existing airborne reset, flick,
mirror mapping and absolute safety bound. Missing/mismatched hold sectors fail
closed. Tests cover 30/60/120 Hz input playback; they do not prove native rotations.

The production second-X route is now connected for Kickflip and Heelflip. A fresh
second press within 300 ms of the original physical press retains that flight and
base trick. The request waits up to the existing 200 ms readiness deadline for
native PrepareHeldFlip. It submits the validated original held gesture, reusing
the existing delivered-neutral proof when available, and withdraws RS at one
additional native rotation. It never calls the single-flip bridge a second time.
Missing/stale state, another owner, landing, a different base trick, competing
input or the bounded failure deadline withdraws input. Each flight admits at most
one such repeat. Single-X mappings and the original Ollie bridge are preserved.

Read-only native observation resolves the original state assets and watches the
exact Kickflip/Heelflip loop actors through the existing fingerprinted shared
ActorController hooks. A counter alone cannot acknowledge animation execution:
initialization and positive native advancement are also required. Native input
withdrawal, actor acknowledgment and landing/scoring acceptance have separate
diagnostics. No rotation counter, PrepareHeldFlip boolean, actor time or scoring
ID is written. FingerflipType.DoubleKickflip is not used.

The composed input tests exercise physical A/X/X, neutral/directional Kickflip,
Heelflip, admission delay, native count withdrawal and landing at 30/60/120 Hz,
plus stale/foreign/unknown/overshot negative controls. Their native observations
are fixtures. The new runtime module still requires Windows compilation and an
RC3 native capture before this is classified as gameplay accepted. The successful
stock-control captures establish the native mechanism, not RC3 X/X acceptance.

## Focused owner acceptance

After the exact candidate passes its build gates, the owner has authorized its
Deck installation while preserving RC2 rollback. Installation is separate from
gameplay acceptance and never promotes the accepted development baseline.

1. At each measured 30/60/120 Hz, attempt 20 Up-to-Down Manuals and 20 Down-to-Up
   Nose Manuals. Include short neutral crossings, diagonals, moderate gestures,
   airborne preparation, landing and ten-second holds. Ordinary steering and
   sustained balance must not retrigger.
2. Exercise every row in `rc3-grab-bindings.md`: ascent press, 100 ms tap, held B,
   release before landing, held through landing and repeated attempts. Verify
   exact actor initialization/positive advancement, matching native detail,
   original GrabExit, cleared hands and native ground contact. Record native score
   and combo ownership separately. A displayed name is insufficient.
3. Verify Manual/RT and Nose Manual/RT, chained reverts and ten-second holds with
   body/board heading, travel, wheel contact and active native pose captured.
   The pressure routing correction still needs rendered native pose acceptance.
4. Verify single X Kickflip and directional Pop Shove-it during ascent, original
   Ollie trajectory, ground RT, grind entry/first HUD/Ollie-out, auto-push, camera,
   shoulder spin, board toggle, ground X/B suppression, controller ownership and
   Arcade OFF. Verify X/X and Right+X/X against the exact native loop actor, two
   board rotations, original catch, landing and scoring; the RC3 path is pending acceptance.
5. Test all 13 footer selections, all four ground grabs and each cell of the
   fingerflip/varial mapping. For transitions hold B through catch, then release
   and land; verify native destination, exit and score. Early X, held X, Rocket
   Air and footers must not fall back to an ordinary flip.
6. Test Up→A No Comply and Up→release→Up→A Boneless against original native
   push/grab admission, animation and one launch. Plain A must remain Ollie.

## No Comply and Boneless

Up→A within 300 ms requests No Comply. Two Up strokes, separated by release,
then A within 500 ms of the first Up request Boneless. No Comply supplies the
original front-foot push and waits for native CanNoComply in a later physics
step before the original flick input. Boneless establishes the original front
hand grab before issuing its fresh native A input. The settings getter must
observe the required original foot-control preferences first. Native intent or
entry state withdraws preparation; the game owns launch and cleanup. No second
Ollie impulse is added. See `native-footplant-entry.md`; gameplay acceptance is pending.

CI and DLL identity belong to the exact staged source revision and accompanying
artifact manifest. Green CI alone cannot close any native gameplay item above.

## Local validation and build gate

On macOS with AppleClang 15, all 20 targeted tests pass with warnings as errors.
The production-composed manual lifecycle suite passes all 18 tests, including
rollback controls, HUD ownership, landing, chained reverts and simulated
ten-second holds. The controller route test and generated production publication
checks pass. Existing input handoff, action dispatch, control state, manual
lifecycle, manual observation, manual history and acquisition-boundary suites
also pass. The native grab suite passes 1013 source/composition checks; the new
manual suite covers 240 trajectories and 17544 assertions.
The 12 manual reservation checks also pass. Ground-grab and fingerflip tests
pass with address and undefined-behavior sanitizers. The 459 Python tests pass
with one optional CMake integration test skipped in that invocation.

The wider input-events suite retains a pre-existing failure at
`face-first unmatched flip owns no adapter`, reproduced against unchanged RC2.
That expectation conflicts with RC2's immediate neutral-X behavior. RC3 updates
only its manual-window expectations; it does not revert RC2's working flips to
make that historical assertion pass.

Hosted Windows, Linux, macOS and upstream integration CI run on the isolated RC3
branch. The earlier installed candidate has its own artifact receipt. Newer
candidate builds remain subject to their exact revision's gates. The source
through `5c6bf1444894157b4682a3625a4618c6832278f8` includes the Windows test-target
list correction and the strict MSVC fixture type correction. Consult the staged
artifact manifest for final source, run links and DLL SHA-256; do not reuse the
earlier installed DLL's identity for these additions. No candidate is gameplay
accepted solely because CI passes. Fakie posture and physical X-X rotations
remain mandatory owner acceptance items.

## Follow-up from controlled RC3-03 trials

The pinned installed RC3-03 DLL exposed a routing gap in Up-right+B,B: the
unsupported first Madonna slot discarded repetition provenance, so the supported
Judo binding never reached its native adapter. The recognizer now retains that
physical prefix only when its native first profile is absent and its second
profile exists. It does not execute a substitute first grab. The unchanged
action-generation, owner, airborne-episode and 250ms physical-edge guards still
apply. A production-composed regression failed before the fix and passes after
at30/60/120Hz, including owner, landing and timeout rejection.

All four ground-grab trials were also refused before native input: the original
Coffin state asset had handle0 before its first use. The ground observer had
required an allocated Coffin slot even for FS/BS grabs. Only that verified
unallocated handle now permits initial observation; nonzero invalid handles,
missing pages and failed reads still reject. Ground actor verification also
now checks the original actor asset type at the asset pointer, matching the
working airborne/held-flip observer, rather than comparing the runtime actor
object to the asset type. Native entry/lifecycle acceptance remains pending.

On RC3-03, exact native actors initialized/advanced for Melon, FS Grab, Japan,
Tailgrab, Crossbone, Tuck Knee, Double Front, Double Back, Method and Stiffy.
Their scoring remains unverified. Rocket repetition delivered both hands but
remained in Nose family6/detail21; it did not initialize the Rocket actor.
The independent Melon trace entered IsGrabbing while ascending approximately
70ms after B; the named hold actor followed the original entry transition.
Do not mistake the later loop acknowledgment for proof that entry waited for
descent. Local raw captures and hashes are retained in the RC3 native-followup
artifact receipt. These findings do not close full catalog acceptance.

## Mega Park map-change failure

The installed RC3-04 failed both single X and X/X after loading the owner's
selected `skate3_megapark`: first X at 520ms or 720ms after A press remained in
the original Ollie animation. A bounded read-only comparison found that the
previous CurrentFlipTrick asset address now held a different resource GUID;
the live registry supplied a new address with the correct GUID and owner slot.
See `native-asset-reload.json` for identities and capture hashes.

The flip and held-flip bridges had permanently latched their initial asset
readiness. Grabs rechecked only optional assets, and ground inputs also latched
readiness. Per-read GUID checks correctly refused stale resources, but required
assets were never rediscovered. All four caches now revalidate their required
GUIDs before the existing bounded lookup. Lost identity withdraws the affected
lease or observation; optional grab entries remain independently discoverable.
No native handle, animation, rotation count, impulse or scoring value is changed.

The relocated-resource regression and five affected portable suites pass.
Compiled CI and native map-reload acceptance remain pending. Earlier actor
evidence from a different loaded-map lifetime does not establish recovery here.
