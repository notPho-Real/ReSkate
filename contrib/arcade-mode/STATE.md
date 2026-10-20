## Controller packet retirement visibility — local follow-on, 2026-10-20

Independent review reproduced a counter collision after route revocation:
physical20 produced synthetic21 then22; OFF restored native22, hiding the
restored state behind the previous packet number. Route rejection had skipped
the existing cancellation mechanism, and authority resets could erase it.

PlaybackPacket now retains a bounded retirement identity across mode/authority
reset, without retaining input authority. A rejected source consumes only its
own cancellation state and otherwise returns the native packet unchanged. If
the restored packet equals the last synthetic packet it advances once; an
already-distinct native counter needs no adjustment. Repeated untouched packets
are not repeatedly bumped. Recovery starts from the current physical baseline
and also avoids a first-delivery collision with the retired sequence.

The existing source-generated routing regression now includes the exact
production rejected-source branch, mode-route reset, retirement helper and
active packet sampling statement. It passes314 packet checks across all four
slots, OFF, source loss/reassignment, stale/ambiguous/focus-loss routes, repeated
reads, untouched controllers, recovery and unsigned wrap. Five targeted
rollbacks fail at their intended assertions. Existing72 native-publication,
367 routing/diagnostic,65 playback and119 action-runtime checks also pass under
host ASan/UBSan. Pad fields, single logical actor, freshness/admission checks and
ambiguous-source denial remain unchanged.

No full build, CI, hardware operation or publication has run for this local
follow-on. These checks establish software packet visibility, not native game
impact or player acceptance. Sole-controller applicability remains unverified.

## Controller routing diagnostics — local follow-on, 2026-10-20

The existing client telemetry queue now carries an `ARCADE CONTROLLER ROUTE`
observation at info level. It reports the connected XInput bitmap already read
by the client, selected physical slot, logical actor, routing generation, mode,
SDK source availability and explicit routing reason. It performs no additional
controller read, native memory inspection, input operation or selection.

`bitmap_known=false` means the bitmap is unavailable or the source is unsupported;
the printed zero is then a placeholder, not evidence of zero connected devices.
Unsupported source values and device-generation tokens are never emitted.
Physical slot and logical actor4 mean unavailable. `ready` describes transport
routing only; native owner, clock, gameplay and action admission remain separate.
Denials distinguish OFF, unavailable source, zero/multiple/unsupported sources,
missing/future/stale physical samples, and missing/future/stale selection.

Reporting is bounded to one row per second even during transitions; unchanged
status refreshes every five seconds. Existing logging levels and telemetry drop
accounting apply. No logging settings or controller behavior are changed.

Offline checks cover zero/one/multiple/stale sources, sanitized telemetry,
reporting rate and OFF. The prior source's routing behavior also matches140000
independent generated comparisons despite added diagnostic reads. No full build,
CI, hardware operation or publication has run for this local follow-on.

## Controller routing native publication correction — local candidate, 2026-10-20

Independent review found that the initial routing candidate admitted physical
slots1–3 but left native push and charge/air/turn publication inside physical
slot-zero guards. The correction routes those publishers, charge/rotation
actors and ordinary-jump rotation metadata through the existing logical local
actor. Per-transport packet tracking and diagnostics retain the physical index.

The routing target now also compiles source-generated production publication
statements into nonactuating sinks. Its72 publication checks cover every physical
slot, charged release, yaw, ground-turn handoff, jump metadata and exclusions;
the235 routing checks remain. Four targeted rollbacks fail at the corresponding
publication assertions. CI build/test selection now includes this exact target
and test while retaining every previous selection. No CI has been run.

The initial candidate remains immutable. Sole-controller applicability is still
unverified on hardware; ambiguous connected sets remain unavailable. Source and
offline publication checks provide no native/player acceptance credit.

## Single physical controller routing — local candidate, 2026-10-20

The native input facade retains one logical local-player session. Its original
slot-zero transport assumption was introduced in `41d80e06` alongside native
owner, focus and freshness binding. A native routing adapter now admits a sole
connected XInput controller on any physical slot to that existing session.
It consumes the selected controller's original cached packet, never the
merged overlay buttons or axes. Physical input diagnostics retain the actual
transport slot. Multiple connected controllers, unsupported sources, stale
samples and unavailable focus remain unadmitted; activity does not elect an
owner from an ambiguous connected set.

Source changes invalidate existing command authority and baseline client edges.
A routing generation is checked again when native authority is published; OFF
clears the route and physical caches. The existing native owner/contact/action
proofs, 150ms freshness bounds and physical movement/Manual implementations
remain in force. Other physical slots cannot advance or cancel the selected
local-player session. Input recognizer and session sources are unchanged.

Focused offline ASan/UBSan validation covers all four slots, ambiguous controller
sets, source transitions, native owner/proof loss, freshness, and OFF restoration.
The new regression is registered as `arcade_controller_route`. This is a local
source correction awaiting independent review and later exact-candidate native
validation; no full build, CI, deployment or hardware trial has run for it.

## Offline score replay admission correction — local candidate, 2026-10-18

The score boundary now compares the complete authority proof as well as the
payload before accepting an exact transport duplicate. A changed owner,
generation, combo, canonical occurrence, attestation, meaning or action slot
invalidates unconfirmed monetary credit. Exact duplicates also advance the
observed context clock; a subsequent backwards health sample is stale. A
tainted stream cannot regain a harmless-duplicate classification via replay.
Already published undrained bank/loss receipts remain immutable.

All three defects reproduce against the prior source and pass with the narrow
boundary correction. The new deterministic synthetic corpus passes105 checks;
existing328 settlement,263 native-score and21 capture-health checks pass.
Full combined ASAN/UBSAN tests retain34489 model/movement checks and existing
input/component regressions. All57 recorded unowned captures still reject
monetary authority. The corpus is included in the existing combined target.
See research/score-replay-contracts.json for tested contracts and native gaps.

No production ScoreProof adapter is enabled. Local payload ownership, canonical
native occurrence/action mapping, raw-versus-aggregate value semantics and
terminal completeness still require independent native evidence. Synthetic
proofs exercise software contracts only; no new native acceptance is claimed.
This follow-on candidate is local-only pending review, with no CI or hardware
operation. Input recognizers, physics, scoring policy and native payload hooks
are unchanged.

## Complex combo trace contract audit — local tests only, 2026-10-18

An interleaved model/admission corpus passes1736 checks at17/34ms intervals,
including repeated Manual/Nose sections, multi-rail recontacts, duplicate
observations, score-activity dropout, bounded revert links, expiry/bail order
and OFF/context recovery. All13 existing A-H report-grader tests pass.
The corpus joins the existing combined local regression target.

No runtime continuity or fatigue-reset defect was reproduced. Model/session,
native admission, input recognition and runtime750/1100ms grace are unchanged.
The initial800ms exit fixture omitted the established100ms contact retention
before landing grace; observing1000ms corrected the test, not the runtime.
See research/combo-trace-contracts.json for exact coverage and limits.

These are synthetic contract checks, not full A-H native acceptance. Remaining
evidence must independently identify intended airborne actions and repeated
manual/rail sections through contiguous owner-matched native history/contact,
cadence and terminal/recovery records. Pose alone cannot identify multiple
midair actions. Existing native denominators retain their original scope.

## Manual input reliability correction — native trials verified, 2026-10-18

Installed candidate f771cd21e6d859400edd0214e68375a93bd42d85 includes
0ede9d3/a45e2fc/9dad4d8. CI39154146695 passes all five gates; separate installer
CI39154146656 passes. Raw-CI and immutable-release DLL bytes match:
b01f307c31189771537648cd3baedf265cba099e4e6dc328ecb826e1fec2c8d7.
Guarded deployment completed with gamefbce74d5/stock5512a15e unchanged,
updatesfalse/recoveryfalse. HIL70 manifest4383ae67 and maintenance71 059723d9
change only version/build pins; frozen workers/installers and guards are unchanged.
406de15 remains the previous native-verified rollback. The installed candidate
now has software verification and native manual trials below. Hands-on feel
validation remains with the player; overall handling equivalence is not claimed.

Source-first review reused manual_contact_pose, NativePushContact, the partial
native right-stick recipe and the existing bounded input telemetry. Official
source/fork survey and native methods remain in research/upstream-method-survey.json;
the established reference has ground220ms, air400ms and air request700ms.
No new hooks, fields, physics recipe or balance tuning.

Deterministic failures: a held non-neutral baseline never armed until perfect
neutral; sideways center travel erased the first vertical edge; fresh valid
two-wheel context was lost when score-display freshness lapsed. Ground250ms
pairs exceeded220ms even though sampling adds a frame. Regressions now cover
fresh reversals after a held baseline, diagonal/neutral passage, both input
methods,150/220/250ms acceptance versus300ms/long holds,30/60Hz samples,
native push recovery and missing-owner rejection.34489 ASAN/UBSAN model checks
and1332 telemetry checks pass. The final complete combined target passes37114 checks,
including1562 manual checks. Its obsolete strict220ms/sideways-cancel assertions
were corrected after CI rejected0ede9d3; the superseded a45e2fc run was cancelled.
The original rearming regression fails before the correction.
9b6f618 integration CI39153189356 passes all five gates; final55ms sourcef771cd2
passes separate CI39154146695. The earlier260ms artifact was not deployed.

The initial260ms allowance still missed physical250ms commands sampled at34ms:
phase0 gives observed edges102/374ms, a272ms gap. Final recognition keeps220ms
as its reference with explicit55ms usability/sampling slack (effective275ms);
all150/220/250ms physical events pass every17/34ms sampling phase for both
directions/devices.350/800ms held-steering negatives pass. Physical300ms can
quantize identically to250ms at30Hz; do not claim that ambiguous negative was
rejected in every phase. The direct300ms observed-edge test remains rejected.
The invalid300ms sampling-negative run9dad4d8 was cancelled after its failure.
Air400ms/request700ms/native actuation remain separate.
Controls reuse owner-checked physical contact during score-presentation gaps,
and the existing200ms native push provenance during one-wheel recovery.
Established balance retains precedence. Combo/HUD identity admission still
requires independent native history. Change-only ARCADE MANUAL INPUT rows
record physical axes/buttons, directions, first-edge timestamps, request,
eligibility/rejection, ownership and delivered Y through the existing queue.

The player's photo correctly showed the native Hub. Status1792349540804834000
then confirmed the game and candidate pins. Automated launcher mouse/Enter did
not start the game; the source-defined default PLAY controller-A action, using
the unchanged frozen gamepad class, did. Oversized old logs were not admitted.
A fresh session with warning-level logging during setup stayed under the existing
16MiB limit. Saved runway collision was restored through scale100→99 at current
generation8/revisions11→13→15. Both endpoint native ground rays224.2m, local
on-board100, no menu, native clock60/cached step0.016666666 confirmed before trials.

25/25 intentional commands entered the correct native identity:13 Manual1,
12 NoseManual2, across both input methods. Evidence joins a new comparable native
history addition, matching0x4000/0x8000 wheel contact stable at least700ms, delivered
partial stick, and clean release/exit with neutral synthetic ownership. No bail
occurred in those positive trials. The frozen generic named_trick criterion stays
UNVERIFIED; exact manual proof comes from the independent history/contact join,
not requested input or HUD. Physics state sequences pass; sparse airborne
heartbeat sequences remain explicitly UNVERIFIED where they miss short states.

- 1792350280888573000:4/4 natural diagonal/center-arc pilots,225/250ms pairs.
- 1792350475510691000:13 executed entries correct, including150ms direct pairs,
 250ms neutral gaps and4/4 air350ms landing preparations;3 rolling cases skipped
 their baseline. First rolling entry/release passed but the short scripted brake
 left1.4m/s coasting, so returned_to_idle failed. Do not call that trial fully PASS.
- 1792350658672650000:4/4 rolling cases with the established longer native brake
 hold enter correctly and return to idle; observed maximum speeds9.29–9.92m/s.
- 1792350901141314000:4/4 valid800ms held-direction negatives, no manual request,
 fresh manual/nose history addition or manual combo entry; clean idle afterward.
 An earlier negative1792350771771321000 left the pad laterally after cumulative
 steering. It stopped on incomplete evidence and is INVALID, not a negative PASS.
 The existing saved spawn/heading was restored before the four valid negatives.
- 1792351024837505000:4/4 natural-arc cases at separately confirmed native30Hz/
 cached step0.03333333. Correct identities/contact, release and idle.150ms balance
 taps retain normal controls; sampled correction includes analog−0.728 and pad−1.
 Earlier60Hz trials also record positive/negative corrections, including Nose.

Full native motion/Trainer CSV collected with unchanged installation/owner guards;
pilot CSV397e7a05, representative23d8effc, rolling623de663, valid negativee61426b1,
30Hz54eaef88. Offline join and raw traces retained in local artifacts. Native pose
recognition takes283–894ms after request (slowest Nose at30Hz); the proven native
recipe and balance dynamics were preserved. Brief taps can fall between250ms
balance telemetry samples; absence of a sampled correction alone is not a failure.
The remaining first-tap-at-renewed-eligibility hypothesis has no demonstrated
native miss in these trials; do not add machinery or widen windows speculatively.
Final native status: ArcadeON,100, rest, editor/menu closed, saved runway spawn;
clock60 and cached step0.016666666 restored. Trainer loggingOFF, session log level
warning to avoid idle log growth. No further gameplay code changed during HIL.

## Installed startup guard fix — native movement verified

406de15fb5e5eee0a4df51b287855faa87e6b384 / DLL
9c915dd882888abae2cc132f37ed207776cc8b9ea00e2adf8e93dae2b6c10ec7.
CI39085320986 passes all five gates; immutable/raw-CI DLL bytes match. Guarded
deployment preserves gamefbce74d5/stock5512a15e, updatesfalse/recoveryfalse.
HIL69 manifest78a510cc, maintenance70 785c5310; pins/version only. Fresh process
and saved-pad collision restored through existing scale100→99 transforms. Both
endpoint rays224.2m, resting100, no menu, matched cached60Hz verified.

1792287306600672000: continuous standing response1.895/4.202/8.422/11.225m/s
at0.1/0.25/0.5/0.75s, then11.225 at1/2/3/5s and the final held frame near10s
(first post-release sample11.15). Ten native40 pushes, first movement within one
native frame. CSV551a5dab/motiondddfe3db. The observed false-reversal stall is
fixed, not merely hidden by a final-speed claim.
1792287370124174000: crouched-from-rest2.791/7.118/13.205/15.255m/s at
0.1/0.25/0.5/0.75s;15.255 at1/2/3/5s. Release A gives native103/200/201/100,
clean landing, then B stops at rest; no bail/offboard. CSV1232ed14/motion91d2d732.
1792287445880790000: coast/brake/steer/resume stays100 and ends at rest;
CSVad5785d7/motion9044a065.
1792287551055700000: OFF independently confirmed false afterward; native held-X
4.991/7.009/8.459/9.498/9.498m/s at1/2/3/5/9.99s,600 consecutive native60Hz
valid100 samples during the hold, no wipeout, and final rest. Same-process
physical B/native source calibrates kernel X timing within input-poll precision.
CSV16368c61/motion768c64f3. Long-case frozen HIL tail_limit remains explicit;
complete guarded native motion and independent Trainer trajectories support
these movement results, without promoting the truncated HIL view to PASS.
1792287716594062000 current-build smoke: exact risingKickflip2 and near-apexHeelflip3,
each independently airborne after A release, exact selection then clean100/idle/
neutral. Descending B+Right adds fresh nativeBSGrab29 (mask268435456, comparable
history) while201, then clean100. All three physics sequences/idle/neutral PASS
with no gap or dropped pad/flip/combo/grind/stick evidence. CSV66c2b4ac,
motiondd852ce3. No A hold is present at X/B.
Late Heel remains a landing failure: matched60 trial1792281784080085000 selects3
after a press74ms beyond measured peak, with only334ms from recognition to
offboard504, versus~382ms selection duration in the passing near-peak case.
Existing Trainer flip_speed is shared-curve discovery/writes in trainer_classes,
not a validated local animation interface. Do not scan or change it, inflate
airtime, shorten proved35ms setup, or promote identity alone to success.
Rotation1792287836845726000: native yaw182.8/-194.1° on the180 inputs;
short360 inputs332.1/-345.1°. One bounded75ms-longer shoulder hold per direction
1792288033205693000 gives341.9/-357.1°, all clean100/idle/neutral with complete
physics evidence/no gaps or drops. Not exact full360 acceptance; no native
angular manipulation or further tuning loop. CSV05719de9/motion5fa0938d, then
CSVdf00753d/motion7702988f retained privately. Preserved manual preparations
and existing grind/revert/balance transitions remain unchanged, with their
previous independently verified results below.

Normal Arcade Rail Fixture restored after tests; raised pad removed from active
layout through existing saved-park load, reusable runway save retained. Final
native100/rest, reset_readytrue, menu/editor closed, heading359.9895 and
matched cached60Hz verified (2026-10-17 18:51). Read-only readiness trace
1792288315107820000 also confirms Arcade ON and the installed DLL. Trainer logging OFF.
No further gameplay source changes while shipping this iteration for feedback.

## Saved native movement runway — 2026-10-17

Installed08d08732 / DLL92a324c5293173955bb315ef561fc43bc46ebf40a719cff6e1672ed5b91f0644
passes all five gates39083180604 and immutable/raw-CI byte comparison. Guarded
deployment preserves gamefbce74d5/stock5512a15e, updatesfalse/recoveryfalse.
HIL68 manifest7a7116e7; maintenance69 c558db9f, pins/version only.

Stop route scouting. `research/movement-runway.park.json` is the exact exported
Park Editor save, with reset/ownership/collision instructions in
`research/movement-runway.json`. One existing large pad, native scale99 through
the existing move path, at(0,150,0); old rail/save retained. Console create saves
scale but its native create recipe omits it: apply the documented existing move
after a reload before accepting collision. Endpoint native rays at z±100 both
hit224.2m. Pilot1792285225982347000 stays100 over177.369m, no air/offboard; its
early brake release fails idle, so retain that caveat. Native Trainer tp
(0,225.5,-100,heading0), then mount, reproducibly returns resting100 around
(0.1167,225.2787,-99.7696), heading359.985. Trainer marker5 saved. Actual matched
cached60Hz/.016666666, editor/menu closed, native motion—not images—are required.

Standing1792285407118890000 completes ten seconds with ten native40 push rises
at0.514,1.563,2.530,3.515,4.496,5.464,6.448,7.430,8.399,9.382s. Native velocity
is10.772m/s at1s,11.225 at2/3/5/10s,60.007Hz, all100. CSV945fa7b4,
motionfdf6e685 retained privately. First bounded nudge at16ms stalls until the
first native push; do not equate first motion with sustained acceleration.
Coast/brake/resume1792285523703283000 stays100:11.225 at release,10.609 after
two seconds of coast, native B reaches rest, forward resumes11.225; steering
changes heading to347.1 and final B stops. CSV8c90272a/motion58a8b756.
Frozen HIL reports tail_limit for these long cases; complete guarded native
motion and Trainer CSV are separately preserved. No guard was relaxed.

Crouched-from-rest1792285630381985000 FAILS (peak0.739, stalls), but A release
still gives native1/200/201/100 and a clean landing. CSVca08083a/motion91d5991d.
The same native samples identify the cause: resting vz≈-0.000103 becomes a
legitimate forward0.28/0.48; the dot-product guard calls that a reversal and
latches assistance off.406de15 requires meaningful measured velocity on both
sides before calling a reversal.34456 sanitized checks pass; regression fails
on previous source; genuine reversal/impact/stall guards remain. CI/native
recheck was pending at this checkpoint; the current-build results above supersede
that pending status and independently verify crouched speed.

Rolling-crouch1792286026323319000 measures11.703→14.344→15.255m/s at
0/0.1/0.25s after A, then holds15.255. A+B does not
brake effectively while the authored crouch remains held. After late A release
the neutral recovery overruns the pad edge (z≈150), then falls: reject the whole
trial. Raw saved-position teleport recovery was attempted but not independently
confirmed; subsequent freshness check rejects17,714,679bytes at the unchanged
16MiB guard. Close this session and restart; do not claim recovery or route around
geometry. CSV58f6b739/motion90d6b228. The next bounded check releases A, confirms
landing, then brakes; no overlapping control/telemetry or relaxed guard.

OFF1792285763782867000 is independently confirmed false/resting100 afterward.
Native held-X velocity is4.992/7.010/8.459/9.498/9.498m/s at1/2/3/5/10s, all100,
with ten native40 rises. Same-process physical B/native source calibration
aligns kernel X timing; retain input-poll uncertainty. CSVfc436bd5/motion0cb9e01d.
This pad's native ceiling is about9.50; do not relabel it9.1 from another surface.

Previous installed795 matched60 manual trials1792284053194205000 verify ground
200ms moderate-diagonal Manual1/Nose2 and tap corrections. Short air350ms pilots
1792284395871822000 verify both identities after landing and full neutral/idle/
reset with no gaps/drops (CSVcd91628f/motione708ded6). Retain late Heel failure.
Rotation1792284555678213000 lands both directions with native yaw335.7/-345.1°;
these are not exact360 acceptance (CSVd5169a95/motiond6706c87).

## Reproducible heading for movement acceptance — source pending

The source-first route investigation finds update_position_teleport constructs
an identity-orientation transform for every position-only Trainer arrival.
Its established native teleport manager already accepts a full transform.
Optional `trainer tp x y z heading` now passes a bounded upright heading through
that same request/transition path. Legacy position-only arrival stays at zero;
no new hook, offset, velocity write or stock file change.34448 sanitized model
checks pass, including independently read heading, orthonormality/handedness,
position retention and invalid inputs. Build/native heading acceptance PENDING.

Road pilot1792282773980551000 stays on-board, reaches11.23m/s by0.75s and
remains near11.23 at3s, then brakes to rest. It crosses a curb via605 between
about1.05–1.42s: useful transition evidence, NOT an uninterrupted flat baseline.
CSV679ea6d5/motion47ad0755 are preserved privately. It ends on the promenade
near(666.4,200.226,1056.928). Subsequent partial-turn/ten-second route
1792283054337018000 fails300/504 at the waterfront edge; reject flat acceptance.
Its complete CSVcd39b300/motion3b10bcd1 remain below the16MiB collection guard.
Measured heading and eventual travel direction differ after that partial turn;
do not use visual facing or its lingering HUD as route proof. Traffic was read
as default before temporary0; restoration to-1 was confirmed as default in the next fresh session
(2026-10-17 17:36:29). No traffic override remains.

## Installed responsive ground and elapsed-air setup — 2026-10-17

Installed795ab49a7a2da5bf931c2d80d2da731e896d1e59 / DLL
92bb9ba1c6644ee49ffdb6a757d3aeadfdb594856c375732d59c1777f521c4a5.
CI39078971196 passes all five gates; immutable package/raw CI DLL byte match.
Guarded deployment preserves gamefbce74d5/stock5512a15e, updatesfalse,
recoveryfalse. HIL67 manifestba84d1c0; maintenance68 a1b618a8; only pins/version
changed. Original uncommitted trajectory work remains separate.

After restarting the contaminated earlier session, native OFF controls
1792279962473636000 at matched30 select2/3 but land offboard. Matched60 control
1792280375725136000 lands Heelflip3; early Kickflip is not recognized. Keep the
two rates distinct. Do not interpret the earlier menu-paused controls as native
limitations. Trainer's existing write_live tuning edits a shared process asset,
then refreshes the local cache; it is not a local-only movement interface.

Trial1792281209568208000 measures matched60 (native dt16.667ms) initial ground
response: first speed>0.1 about15ms after input,1.61m/s at0.1s,3.85 at0.25s,
8.07 at0.5s,11.23 at0.75s and1s. Clock alignment uses the preceding physical
A event/native observation and kernel timestamps; uncertainty includes one
input poll. Native push40 rises at0.513s. The reversed lane reaches a barrier
and offboard504 at1.398s: NOT ten-second flat acceptance. The old full-left
circle1792281448425001000 also fails at the higher speed. Do not reuse either
as a clear route or claim repeated native pushes/top-speed acceptance from them.
Private motion/CSV metadata are preserved as push-response-reversed60 and
push-response-circle60. Collision/offboard contexts stop the ground writer.

Trial1792281590098436000 verifies exact rising Kickflip2 and near-peak Heelflip3,
both100→103→200→201→100, idle/neutral/reset PASS and no dropped/gapped evidence.
The corrected35ms delivered airborne setup is now verified at matched60.
Trial1792281784080085000 retains the late Heelflip failure: X atA+1152ms,
independent vertical-velocity apex aroundA+1078ms, native3 atA+1212ms, offboard504
atA+1546ms. Identity alone is not success. Later candidates were not executed
after the runner's evidence stop. No artificial airtime/catch substitution.
Trial1792281955027489000 verifies face-before-direction33ms Kickflip2 with
clean landing, and rising/descending directional B requests with fresh native
BS Grab29 history additions and on-board landings; all idle/neutral/reset PASS,
no evidence gap or drops. Its final full live-log collection rejects17,149,183
bytes at the unchanged16MiB guard; only CSV archivalaa33a1cb is retained for
that collection. Restart before further full-log telemetry; do not relax bounds.
Source rotation100/50ms delays apply to directional inputs; shoulder inputs
bypass them. Do not add that delay to the shoulder adapter. The preceding
matched60 prewind1792281182966823000 lands on-board with native heading about
182.9° unwrapped. Complete both-direction180/360 acceptance remains pending.

## Native push recovery and standing response — source correction

Source-first review of the existing push reader, full guarded Trainer writer,
official client transform/physics APIs and preserved4dab47d motion identifies
the actual contact gap. In1792275499071053000, native push40 clears at153217319ms
while contact remains8000 until153217421ms. Both input publication and writer
required40 on that one wheel, so X and assistance were released during recovery.
The adapter then resumes its intended acceleration only after two-wheel contact.
The earlier source did not measure this transition before changing thresholds.

New shared pure policy retains only200ms from an actual native push, enough
for the102ms recovery plus sampled-frame margin. Both publisher and guarded
writer use it; owner/toggle/context/balance/invalid-contact resets revoke it.
Manual commands still preempt ownership, and one wheel alone never qualifies.

The standing adapter also no longer requires the native animation to create
the first0.5m/s. Only a validated local100/mode1/two-wheel context can start its
bounded per-step budget. Direction comes from the existing validated entity
transform already used by Trainer heading; no new memory offset or hook.
Native vertical and relative body motion remain untouched. Impact/stall/reversal
or teleport blocks restart until genuine native progress clears the obstruction
or input is released. Crouched starts retain their separately bounded budget.
34406 sanitized model checks pass, including30/60 startup, recovery boundaries,
no inherited/manual/air recovery and repeated blocked-start rejection.
Hardware and full acceleration/coast/brake acceptance remain PENDING.

## Installed authored-clock build; elapsed setup fix pending — 2026-10-17

Installed0d895838563caf8f36814d7d8f2a0faecaa50218 / DLL
b64b7b9c9a35977b7c9931e121686009b17294014fc154ebf7dc33f494319339.
CI39074691042 passes all five gates; raw CI DLL/package byte match and guarded
deployment preserve gamefbce74d5/stock5512a15e, updatesfalse/recoveryfalse.
HIL66 manifestd7fc29e4, maintenance67 d400b725; only verified pins/version changed.

Trial1792277405025210000 rejects all three original30Hz flip timings: exact2/3
but offboard504. Face-first33ms case delivers the authored path but no exact
flip; its reset also fails native-menu closure after a telemetry console call
overlapped final reset. Do not accept it. All input/console operations must be
serialized. Native OFF controls1792278070755974000 are inconclusive (no air or
motion); guarded retry1792278317882008000 stops before input with native menu
open. The retained native customization menu is being dismissed. These are not
proof of a native animation limitation. No automatic replay bypassed the guard.

The same failed-trial stick records expose a concrete timing defect: first
airborne setup is delivered for20–28ms despite its documented35ms requirement.
A normalized tick counter cannot guarantee a duration when output starts
between publications. Source fix retains native35ms from actual output delivery
in the existing authored scheduler; grounded recipes, arc points and neutral
tails remain unchanged. The focused regression fails before and passes after;
65 playback,34304 model and119 action checks pass sanitized. Hardware PENDING.

The earlier2fbfee1 trial1792276075837229000 independently verifies matched60:
native rate readback60/cached16.667ms,1000 direct physics rows at16.667ms and
59.9856 callbacks/s; native Ollie1/200/201/100 and neutral/idle PASS. CSVaca8f761,
native motion157513d0 retained privately. A later center-lane run is rejected
for605/200/504 terrain contact; only its historical CSV is archived
(18a694792c263002ce89304586d0e3eab691a2fb29f8bb94c21997801957329f).
The old session exceeded the frozen16MiB log limit; no limit was raised.
Ten-second clear-lane speed, crouching/coasting/braking, matched-rate air-action
landings and complete180/360 acceptance remain outstanding.

## Installed guarded rate build — 2026-10-17

Installed2fbfee106c0a9f5717f35252d9ebb62259e0407b / DLL
de032bb8e3dea172a16a17d80ba06aabbd6f3a3afa8d617d68a055860535070e.
CI39073048584 passes all five gates; raw CI DLL and immutable package are
byte-identical. Guarded stopped-game deployment preserves gamefbce74d5 and
stock5512a15e, updatesfalse/recoverypendingfalse. HIL65 manifestf268f13a,
maintenance66 digest013b80a1; frozen workers unchanged. Only two reviewed fixed
rate commands now invoke the guarded fixture rate operation. Actual rate
readback/cadence and movement acceptance remain PENDING the fresh session.

Previous4dab47d matched30 movement pilot1792275499071053000 reaches native
11.150m/s before course contact/offboard504; NOT sustained-speed acceptance.
Native2252 physics rows all report33.3ms and Trainer position corroborates
movement. CSV85d8bc859bf41bbd3e088b004a70a6a8082a631a0c7f4414335e03675d75e052;
motion21c93fe84219f168ca65df50348b17e8aa481edc83099bfce541190ae791e251.
The ten-second HIL tail reports truncation; complete bounded native-motion
collection is preserved separately. No collision result is accepted as cruising.

## Air event matching and authored clock — source correction pending

Fresh4dab47d trial1792275159684754000: rising Kickflip2 and near-peak/later
Heelflip3 all select the exact native identity but finish offboard504. All are
REJECTED as gameplay successes. Inputs, native air/state/identity and clean
neutral/reset are retained; no screenshot or selection alone establishes landing.

Source-first comparison of existing authored compilation, client publication,
Hall of Meat cached-step API and surveyed playback adapters exposes a timing
defect: authored offsets are compiled at60 units/s but runtime advances only
once per client callback. At matched30Hz that doubles setup/tail durations.
New source advances a monotonic rational authored clock from the existing
guarded cached step. Missing/invalid rate releases synthetic ownership; native
frame quantization and ordered arc delivery remain. No animation/physics changes.

Original event matching accepts face/direction in either order within500ms;
the existing adapter only handled direction first. A failing focused regression
reproduced that gap. New source retains unmatched airborne X without stick
ownership, consumes it once a direction matches, and discards it on landing,
competing input, invalid direction, owner/context loss or expiry. Native
readiness remains a separate200ms budget after matching. Takeoff separately
clears directional history older than160ms; held directions remain usable.
Both500ms airborne matching and160ms preload boundaries are source-tested.
Sanitized34304 model and119 runtime checks pass. Hardware acceptance PENDING;
these source changes are not part of installed4dab47d or the rate-only2fbfee1.

## Focused matched30 control pilots — 2026-10-17

Installed4dab47d / DLL22ac7739, HIL64, unchanged native/stock contract.
After reloading Stadium, both native callback cadence and cached33.3ms step
establish matched30Hz. The earlier nominal60 setup is invalidated below.
No monetary, grind, revert or supported-build changes were made.

* 1792273386159627000: ground200ms diagonal pairs yield fresh native Manual1
  and Nose Manual2, riding100, idle/neutral/reset PASS, no observation drops.
  Short70–100ms D-pad corrections exercise the existing manual balance path.
* 1792273815256714000: rolling moderate diagonal200ms pair yields Manual1;
  D-pad200ms pair over held forward yields Nose Manual2. Both clean100 with
  idle/neutral/reset PASS and no observation drops. This does not support the
  speculative claim that rolling entry requires a different native recipe.
* 1792273895088300000: both350ms airborne pairs follow independently observed
  Ollie1/air and produce the correct fresh manual on landing. Manual1 appears
 195ms after landing; Nose Manual2 appears696ms after landing. Both clean100,
  idle/neutral/reset PASS, no observation drops. Recognition,700ms air request
  and separate native landing-actuation lifetime remain distinct.
* 1792274222045567000: left prewind and later right airborne shoulder trials
  both land100 cleanly. Historical native Trainer headings unwrap to188.5 and
  -126.5 degrees respectively. This is rotation response evidence, not complete
 180/360-degree acceptance. Raw CSV is preserved with archival-only metadata,
  SHA2567bde227befa99f2314cf666d2bf34350a714f6ba37331f33779324a14f42439d.

The long session exceeded the frozen full-log16MiB collector limit; it was
stopped and its bounded2.44MB Trainer CSV archived separately. The limit was
not loosened. Per-trial bounded observations remain preserved; the archival
CSV must not be relabeled as a fresh live-session integrity check.
Fresh-session matched60 acceleration, sustained clear-lane speed, crouching,
braking/OFF and air-action timing acceptance remain outstanding.

## Native rate setup correction — pending guarded operation build

Installed4dab47d / DLL22ac7739 passed all five gates and raw/package match;
HIL64 manifest340dcefb, maintenance65 8dd4e39c. Originals remain unchanged.
Pilot1792272295465881000 produced native velocity11.225m/s and stopped at the
known course boundary, but is NOT nominal60Hz acceleration acceptance:
7870 physics observations run near60/s while their cached step is33.3ms.
The mixed-rate trajectory/motion/metadata are preserved privately under
artifacts/fidelity-rate-mismatch. Reload restores the map default30Hz.

Existing simulation_time.h documents core-construction caching; Hall of Meat
already owns a guarded step_length/set_step_length API. New explicit fixture
rate-30/rate-60 operations reuse that API and named ForceSimRate/MaxSimFps.
They require a fresh, stopped, local two-wheel owner and closed editor, reject
multiplayer, verify readback and restore owned values on failure. No new hook,
offset, native scan, velocity change or frozen-worker change. Future acceptance
must establish both callback cadence and cached step, not a clock command alone.

## Bounded ground drive — source pilot pending

Installed9826347 / DLLd12476b9 passed all five CI gates and raw/package byte
comparison; gamefbce74d5 and stock5512a15e are unchanged, updates off. HIL63
2bf5b6178b159ec111d3b749d1478f51f6d2ac25abdbb33e3f228d233e885252;
maintenance64 d7040eef3793a57a7b9af3d5580e5a07e2d94e138b8f151fb7a5c7b1e1ca4797.
Workers are unchanged. Native calibration1792269481315070000 lands100 with
Ollie1, no observation gaps. Ten-second forward1792269503023144000 confirms
repeated native pushing and rising speed, then collides around x-52/z40.8.
It is not sustained-speed acceptance. Existing Trainer CSV is preserved under
artifacts/feel-baseline-collision; its position-derived curve approaches7m/s
at2s and9m/s at3s. The alternate x-71/z52 teleport had no floor and was rejected;
validated fixture reset restored grounded504 at[-39.406,46.950,52.616].

The new adapter reuses Trainer push-speed ownership, native timestep and body
writer. It budgets16.88m/s2 toward11.30m/s standing,28.66 toward15.33 crouched,
counts native acceleration, and adds a uniform horizontal velocity increment.
Native vertical/relative body motion is preserved. Fresh movement intent and
validated local ground/contact are required; braking, editor, teleport, stalls,
impact-sized losses, hitches, invalid ownership and multiplayer suppress it.
There is no target-field write, velocity multiplier, transform write or new
hook/offset. OFF releases immediately, with150ms stale-publication expiry.
This is source-tested behavior; actual movement curve acceptance is PENDING.

## Control timing fidelity — source update pending hardware

Ground directional pairs now use220ms, airborne pairs400ms, and airborne
requests expire700ms after recognition. Landing actuation retains its separate
proven1200ms limit and bounded wheel-contact settling. Moderate/diagonal stick
travel and hysteresis remain; native partial-stick recipes are unchanged.

Airborne direction events match released-direction X/B for500ms, extra face
presses300ms. Readiness buffering is separately bounded to200ms within one
observed jump, canceled by landing or competing intent. Basic B preserves the
verified native identity; no named grab is substituted. Existing authored
playback and native readiness remain authoritative.

Trainer logging now also samples direct, guarded board velocity from the
existing physics-step listener, retaining the actual timestep and native
requests/contact/state. Existing client-position CSV is separate corroboration.
No new hook or offset. Focused sanitized checks pass; hosted build and hardware
acceptance remain pending. No movement adapter has been applied yet.

## Movement and manual feel correction — 2026-10-17

User play feedback supersedes the earlier basic control acceptance: cruising
was too slow, manual entry demanded unusually precise/fast input, and a Nose
Manual once outlasted its meter. That exact live pose had already ended.

Source-first review searched all1455 files in official upstream ba1245a5,
existing Arcade controls/model/observation/HUD and surveyed input adapters.
Native held pushing already provides the remaining acceleration steps. Remove
the lower6m/s cap and4m/s resume latch instead of writing velocity. Manual
actuation stays the verified partial RS20000 recipe; input now accepts500ms
pairs, moderate deflection and diagonal thumb travel with threshold hysteresis.
Both directions still require actual edges; long movement holds and expired
pairs do not count. First Down's reverse reservation expires after500ms.

Manual-only correction authority increases3x and damping decreases to15% of
the existing value, allowing short taps to change persistent drift. The entry
direction is ignored until release or a fresh opposite correction. Grind
dynamics and correction are unchanged. Model failure retains a bounded edge
display until native manual exit, without restarting the failed section or
retaining synthetic stick ownership.

A fresh matching native history label still admits each manual once. Existing
same-owner native100/mode1 wheel contact maintains that identity through score
activity gaps; two-wheel gaps get100ms continuity. Owner/menu/bail/invalidity,
opposite contact and real exits invalidate it. Loss of an established physical
manual cancels its synthetic hold. This addresses source-reproduced routes to
a missing meter, without claiming the user's vanished live case was reproduced.

Focused strict ASAN/UBSAN checks pass33765 model,524 manual input, and111 action
runtime checks; identity checks include contact continuity and negative controls.
Hosted build and real Deck acceptance are PENDING. Installed build remains
bedee9d / a521fb05 until a verified replacement is deployed. No executable or
stock contract changes, new native hooks, memory scans or physics writes.

## Installed directional basic B build — 2026-10-17

Installed bedee9d3430f496deba23f9ad7b5e26681fd6a09 / DLL
a521fb052701ad1988118af217aff55024b42d0c51b0d8c4b0e3d82dd8103aaa.
CI39017769634 passed all five gates, raw/package byte match, guarded deployment
preserves gamefbce74d5/stock5512a15e, updatesfalse/recoverypendingfalse.
HIL62 manifestb0bbef3f555a2577167a31b091ccf10a8152544f1256a073a735eb21e1cdf2fa;
maintenance63 aa55d5bbf08a079dbdbeb0f198545e33aace28fe676f5f7c67f33766eb473717.
Workers are byte-identical to61; only runner version and verified target pins
changed. The following acceptance uses this exact installed DLL at native60Hz:

* HIL62 trial1792228905221427000: ten seconds of forward-only input, no stick
  release/repress, produces native0x40 push edges106620389/106621490 and speed
  steps4.986/7.002m/s. Native100 throughout; release/brake returns to rest,
  neutralization/idle pass, no observation drops. Cruise settles lower later in
  the path; this is independent native push/speed evidence, not input delivery.
* 1792229186994077000: B+D-pad Left on ascent and B+held forward near the peak
  each produce fresh native Grab29 (added mask268435456) while201, then clean
  riding100, idle/neutral/reset. Native identity is the basic grab, not a named
  directional modifier. Secondary Deck capture shows the corresponding pose.
* 1792229243272985000: all SIX Kickflip/Heelflip rising/near-peak/later cases
  pass exact native2/3 after native Ollie1/air, clean100 landings, idle/neutral/
  reset; no pad/flip/combo/grind/stick drops. Existing native Trainer CSV
  SHA25639e10da9cda9350a2eac9fff7597e167dab8d199c314627ee2e80b38c0a2fcfc
  has22474 rows and8 air intervals. State-edge clock consistency0.8–5.6ms;
  observation precision remains about one native frame. Early flip presses are
  268–306ms before measured peak; near-peak presses29ms before; later Kickflip
  is65ms after peak with native vertical-0.477m/s. Later Heelflip/B are within
  one frame of peak; the later pilots below resolve their descent timing.
* 1792229398399098000: standing/moving on-foot A have fresh air edges127ms after
  physical A, pelvisVy peaks6.843/3.822m/s, Y rise0.951/0.888m, descent and grounded
  landings977/929ms after A.168 samples,166 valid, no ragdoll/wipeout/gaps;
  native flip stays-1. Both140ms direct stick manual pairs and both70ms
  airborne-prepared pairs produce distinct fresh Manual1/Nose Manual2 and
  riding100; idle/neutral/reset pass with no observation drops.
* 1792229568459038000: B+forward54.5ms after measured peak, native vertical
  -0.303m/s, establishes fresh Grab29 while201 then lands100/idle/neutral/reset.
  A Heelflip47.5ms after peak, native vertical-0.310m/s, recognizes exact3 but
  lands off board504: FAIL landing, despite successful reset. Do not count it
  as full success. Native standing-air time is finite; the earlier six-case
  acceptance does not imply an unrestricted late window. Existing Trainer CSV
  SHA256518a4c12f8e265d3cb11706d09c8162b4174ac865f7ea1b8dfd94b5d8734c6c4,
  5753 rows, edge-clock consistency0.2/1.5ms. Private evidence is preserved as
  artifacts/ding-bat-bed-later-air (.csv, -metadata.json, -correlation.json).

These trials reuse the existing source-derived gestures, guarded observers and
Trainer CSV. Private evidence is retained under artifacts/ding-bat-bed-native-air
(.csv, -metadata.json and -correlation.json). The older sections below retain
their checkpoint-time pending status; the current acceptance above supersedes it.
Hardflip remains an exact-identity failure documented below; no wrong native
trick is accepted as Hardflip. No physics tuning or artificial airtime was added.

Focused Hardflip follow-up1792230696374614000 reuses the existing bounded arc
catalog (index34/native9/ground), its original midpoint magnitudes and the
previously verified reflected path after a native500ms-load/68ms-pop Ollie.
Fresh status1792230649893175000 confirms Arcade OFF, local riding100, closed
native menu, stationary baseline. Native1 precedes200/201, then native7 rather
than9, clean100/idle/neutral and no drops: FAIL exact identity. Restoring original
magnitudes alone is falsified; the compiler was not changed. The current native
guide agrees with the authored small scoop/flick recipe. Earlier grounded9
successes in research/reflection-control-results.json remain valid; this is an
unresolved airborne adapter failure, not evidence that native Hardflip is absent.
The existing catalog already exports bounded arc data; no new export or memory
scan was needed. Underflip patterns remain distinct and are not substituted.

Final native status1792231013107190000: Arcade ON, local riding100, fixture
reset_ready=true, editor/menu closed, controller neutral; native60Hz restored,
Trainer logging OFF. Installation status1792230890840751000 confirms bedee9d /
a521fb05, stock5512a15e unchanged, updatesfalse/recoverypendingfalse. Reset briefly
left the actor grounded off board; native Y remounted successfully before this
final independently observed riding baseline. No new binary followed acceptance.

8fec865 native trial1792226717773020000 adds real-motion timing evidence using
the EXISTING Trainer CSV (native map keydingolevel_sdm_int_001, not CLIstadium_1).
CSV SHA256f330465f433efc18356bbda4d3b817b2a1abf1f1227839335b576655d7865e3c,
12637 rows,6 native air intervals. State-edge clock consistency1.3ms in the
Heelflip case: native peak1071ms after A; physical X1096ms after A, with native
Y falling and vertical-0.137m/s; exact native3 and safe100 landing PASS. Thus the
previously named apex trial actually includes a descending-air request, not an
A-playback inference. Plain-Ollie peak1089ms after A. Early B atA+768ms and mid B
atA+900ms both establish fresh Grab29 in201 then land100/idle safely, no drops.
A during a real one-wheel push also passes: preceding native100/1, contact0x8000,
request0x40; then native Ollie1/air, rail contact/release and safe100/idle/reset.
All five trials neutralize/reset, no observation drops. No new physics reader,
logger or tuning was introduced; the existing Trainer native CSV is reused.

The first CSV attempt had not established logging; explicit console focus now
verifies native log=1/log=0 replies and actual file creation. Logging is OFF.
The long native log reached19MB beyond the frozen16MB reader limit; restarting
the same verified DLL renewed the session without changing that guard. Old
trial records are preserved. Current native apex data is retained privately in
artifacts/ding-bat-source8-native-air.csv and its metadata/correlation JSON.

## Directional basic B source repair — 2026-10-17

Runtime remains verified8fec865 / DLL47ba309f. Normal single B plus any valid
movement/direction now explicitly requests basic_bs_grab in source, reusing the
same proved RT500ms plan, native-air gate, ownership, landing cancellation and
fresh-press rearm. Previously the named-table selection made ordinary B plus
held movement unavailable. Actual request diagnostics remain the basic identity;
no Melon/Indy/Nose identity is claimed. Named catalog candidates and explicit lab
selector are retained separately. Ground/on-foot B remains native, invalid opposing
directions remain rejected, and no wait or stale queue is added.111 focused
runtime checks pass strict ASAN/UBSAN, including all nine valid D-pad modifiers,
forward/backward movement and existing safe cancellation. New source hardware
acceptance is PENDING its next verified DLL. Native Trainer CSV collection is
being repaired through existing command routing; no new physics hook or reader.

## Installed8fec865 core regressions — 2026-10-17

HIL61 trial1792224271386351000 PASSES early native Kickflip2, apex-timed
Heelflip3, basic neutral B (fresh Grab29 in native201), standing/moving on-foot A,
and10s held-forward push; all native state/idle/neutral/reset pass, no drops.
Foot A edges102046173/102050174 have fresh native air after125/125ms, pelvisVy
peaks6.843/5.101m/s, Y rises0.951/0.888m, descent and grounded504 landings976/925ms
after A. No ragdoll/wipeout/gaps; position age<=7ms; nativeflip stays-1.
Push speed4.920m/s at1s,6.925 at2s,6.755 at10s, native0x40 edges688/1789ms,
state100 throughout, stop0 by12s. This is the installed47ba309f DLL, not an older
binary. Native opposite-hand control1792224137714688000 has no fresh grab:
its short native air window is not a positive FS Grab result. Full native rail
pop in the same control also supplies no longer air; no runtime recipe changed.
Native Trainer CSV apex measurement1792224768461031000 records a plain Ollie
and exact Heelflip with safe landings; CSV retrieval is not yet verified, so
physical apex/later classifications remain unproved. Existing native CSV is
being reused; no new physics observer or tuning changes were introduced.

## Separate native30Hz manual acceptance — 2026-10-17

8fec865 / DLL47ba309f, HIL61 trial1792223841285352000 PASSES both140ms direct
stick pairs at explicitly selected native30Hz: fresh Manual1/Nose Manual2,
riding/one-wheel contacts, idle/neutral/reset, no observation drops. Native console
readback1792223728249647000 confirms30; readback1792224019832417000 confirms60
restored afterward. Frozen workers remain unchanged. A reviewed fixed30 command
was added to separately pinned maintenance60 (digest
1cc529903f42f5b7e49731e5863b557e788176b27398e67bd03c3ec5d9c430b8);
removing only that command exactly reproduces maintenance59, including guards.
Hardflip radius hypothesis1792223560073617000 is FALSIFIED: native4, not9,
with clean landing/idle/no drops. Compiler was deliberately not changed.

## Native direct-stick manual repair accepted at60Hz — 2026-10-17

Installed8fec865 / DLL47ba309f, HIL61.1792223321181273000 PASSES both rapid
140ms direct stick pairs. Native100/mode1, fresh distinct Manual1/Nose Manual2
history without duplicates/relabels, corresponding one-wheel0x4000/0x8000,
proper balance activation, clean native idle/neutral/reset and no observation
drops. The previous direct-stick Nose failure is fixed. A held first Down
negative control restores native101 after the350ms reservation, with no partial
RS manual request, no manual history or balance activation; idle/neutral/reset
also PASS. Native reverse remains available when the player continues holding.
1792223434141577000 PASSES rapid70ms stick commands prepared in real Ollie1/air:
native100 landing then fresh Manual1/Nose Manual2, clean idle/neutral/reset,
no drops. Both airborne preparations work on the deployed source.
Separate native30Hz checks remain pending; native60Hz is still active.

## Installed manual-routing build — 2026-10-17

Installed8fec86579d19a690155fb8181b28352b2c4f9431 / DLL
47ba309f13af64a8edc54b68ce1aa8a1369fc0c6bf9994bc909d67482fd74c6d.
CI39013028970 passed all five gates; raw CI and immutable package are byte-identical.
Guarded deployment verified gamefbce74d5/stock5512a15e unchanged, updatesfalse,
recoverypendingfalse. HIL61 manifeste3e4aba48896f74562604568719e4acd9f98ea6bea4e23c86595dcbe2dce6bc9,
maintenance59 aabd469696bda4806a0dc0def4798a9a01c0446167591a72e73e6f8705965ccc.
HIL workers are byte-identical to60; only runner version and target pins changed.
Manual routing acceptance is pending startup on this build.

Additional20b native trials1792222690419044000: directional Pop Shove-It4 PASS,
grounded-X negative control has no takeoff at X and no stale flip after later
Ollie1; both clean native100/idle/neutral/reset, no drops. Requested Hardflip9
instead produces native7: FAIL exact identity despite safe landing. Preserve this
catalog failure and inspect its authored pattern/mirroring before further changes;
never report native7 as Hardflip. No extra catalog rebuild or physics change yet.

## Fast-air/basic-grab native pilots — 2026-10-17

Installed20b1902 / DLLad732353 remains verified on HIL60, Stadium60Hz.
1792221943045155000: early Kickflip2 and apex Heelflip3 both PASS exact native
selection after independent Ollie1/air, clean native100 landing, idle/neutral/
reset; no pad/flip/combo/grind/stick drops.1792222050645768000 also PASSES apex
Kickflip2 and early Heelflip3 with the same independent evidence. Thus both
identities now land at early and apex timings; no A hold is required in air.
Normal neutral-B in1792221943045155000 produces fresh native base-name Grab
(label29, added mask268435456),404ms after B while independently201, followed
by native100 landing90ms later; valid/comparable history, no malformed/drop/gap.
This proves the basic grab action, not a side or a named modifier. Its rendered
BS label is secondary. Persistent history never counts as a fresh action.

Preserve late failures:1792222237518222000 X3800 Kickflip has no native2;
X3750 Heelflip3 recognizes but lands504, so neither passes late acceptance.
1792222050645768000 natural-ramp late commands fail exact identity; the measured
remaining air before native402 contact is180/139ms, inadequate for the proved
native animations. Do not call these late successes or inject extra airtime.
Native-OFF guide controls1792222180487567000 also fail named identity: Nose
RSdown50ms then RT600ms yields no fresh grab label; RT400ms then RSup350ms
produces only native Grab29, not Melon. Both land100/idle safely with no evidence
drops. This falsifies simply extending those two candidate holds as a named-grab
fix. Named recipes remain unavailable; basic neutral B remains enabled.
Source8fec865 manual Down reservation is still awaiting its verified DLL and
hardware test. Existing direct-stick Nose failure is not waived.

1792222577286819000 current-build push regression: ten seconds uninterrupted
forward/full-left on flat ground, native100/mode1 throughout, native0x40 rises
657/1757ms after capture baseline. Speed4.912m/s at1s,6.922 at2s,6.753 at10s;
release ends automatic push, B brings speed0 by12s. Native state/idle/neutral/
reset PASS, no pad/combo drops. Separate stop/resume trial stops by5s then has
three new native0x40 edges after resuming; final idle FAILS at0.15m/s residual
speed, although state100/neutral/reset pass. Preserve that failed idle result.
Native grind-to-pop control1792222324725394000 preserves independent401→200/201→
100 and exact Ollie1, clean idle/neutral/reset, no observation drops. It does not
supply a longer native air window. Native Nose release-before-trigger control
1792222500508908000 yields only fresh Grab29, not Nose Grab6, with safe landing;
the extra neutral gap does not establish that named action either.

## Direct-stick Nose Manual routing repair source — 2026-10-17

Installed20b1902dbbafe5b163c76f75d49b40559c4367ce / DLL
ad732353eb55ec86c85640642192e20ef2c129004432e7d197a248aa174fa862.
CI39010889427 passed all five gates; raw/package match; guarded deployment preserves
gamefbce74d5/stock5512a15e, updatesfalse/recoverypendingfalse. HIL60 manifest
437c1c19aacba77378ec753fc60c4c2740e653bb75e3ddac4e7971c840a768bb;
maintenance58 digestb6cc521593703a7298d9fd9e1e02186fcd153e3b0a551acfca897e095d7268d8.
Fast-air/basic-grab native acceptance on this build remains pending startup.

HIL59 direct-stick1792220511391760000: Up/Down confirms native Manual, but
Down/Up fails Nose Manual. Native LS Down enters101/1 only36ms after its edge;
the existing correct safety gate then cancels the pair. ManualGesture source is
identical in3e295fb/20b1902. Source-first reuse: native101/revert classification
already exists; SessionSticks classic deliberately owns native LS Y. Reserve only
the grounded first Down within the existing350ms recognizer, preserving all RS
manual actuation and rejecting actuation during101. A hold beyond350ms passes
through; B, horizontal steering, menus/OFF/on-foot and established corrections
are untouched. A distinct packet-mask bit makes restoration visible even with
unchanged physical input and camera ownership. Hardware proof pending next DLL.
The existing native push bit0x40 also keeps a one-wheel kick in riding input
context until independent balance is established, so pushing cannot consume a
fresh manual pair. This reuses the supported push observation; no new state read.
33755 model/513 manual/62 playback/99 runtime checks pass strict ASAN/UBSAN.

Manual correction1792220275819179000 PASSES both Manual/Nose: fresh matching
native history and pose, D-pad correction+1/-1 with near-zero physical LY, actual
balance x/velocity response, no extra manual request or failure; native state/
idle/neutral/reset PASS, no gap. No new observation infrastructure was needed.
Riding/air D-pad manual acceptances above remain valid. Keep failed direct-stick
Nose result; do not replace it with the passing D-pad result.

## Manual acceptance; faster native air path source — 2026-10-17

Runtime remains3e295fb / DLL00e91c07 on HIL59. Ground trials
1792218467126868000 PASS rapid70ms D-pad pairs over held forward for BOTH Manual
and Nose Manual: fresh distinct matching native history, riding/one-wheel contact,
native idle/neutral/reset PASS, no gap. Air preparation1792218281960351000 also
PASSES BOTH: independent Ollie1/air, partial native stick before landing, correct
one-wheel landing and fresh matching Manual/Nose Manual identity; all state/idle/
neutral/reset PASS. Earlier1792217949057994000 preserved: Manual identity succeeds
but brake too short; Nose cut off before recognition. Focused longer control
1792218168224600000 proves Nose identity808ms after native actuation with clean
stop. Native manual recipe,350ms command window and accepted balance preserved.

Air source3e295fb:1792217646730504000 early Kickflip2 lands cleanly, apex Heelflip3
recognizes but steps off.1792217815919279000 early Heelflip3 lands cleanly, apex
Kickflip2 recognizes but steps off; both late requests fail exact identity. Retain
all failures. Native OFF controls1792218594467142000 and1792218776707718000 land
exact Kickflip2/Heelflip3 near apex with35ms setup/full-range fast flick, full
native Ollie, no artificial airtime/tuning. Native recognition is~80ms earlier.
Source now prepares that path: original authored24ms flick spacing and allowed
maximum magnitudes,35ms setup, conditional30ms neutral reset. Only delivered zero
RS across distinct game ticks for>=30ms can omit reset; nonzero outputs and OFF/
menu/offboard invalidate that proof. Ground full-pop uses native allowed range;
from-grind pop and ordinary grounded flip recipes remain unchanged. Hardware
acceptance of this new source is PENDING the next verified DLL.
Native OFF late control1792219198041405000 also FAILS exact Heelflip3 with clean
idle/no gap. This falsifies the hypothesis that all late rejection is Arcade
playback latency; a longer natural-air control remains required. No jump-height,
game-time or native velocity changes are authorized as a substitute.

Normal neutral-B pilot1792218085190513000 has native Ollie/air, safe native landing,
fresh visible BS Grab Sketchy and a new native history row. Basic grab identity
was missing from the existing exact-label allowlist, so do not accept from image
alone. Source reuses that same guarded history reader with exact BS Grab/FS Grab/
Grab labels and bounded added/removed label masks. Persistent history/baselines
never become additions; no amounts, awards, banking or monetary authority changed.
Fresh native history must corroborate independent air/pose and actual B before
acceptance. Unproved named directional grabs remain unavailable.
33752 model,62 playback,499 manual,99 runtime,150 history and1328 telemetry checks
pass strict ASAN/UBSAN;414 lab tests pass. New source is not yet deployed.

## Installed source-first build; native A jump proof — 2026-10-17

Installed3e295fb8b5e7fd4b1c9a0e88b1f9d97b5a61e8a9 / DLL
00e91c0784d70616b9be5a9de5a4d68cf3f2947d89626fe78d4bdb4660726640.
CI39007119326 passed all five gates; raw CI DLL and immutable package match.
Guarded deployment verified original gamefbce74d5/stock5512a15e unchanged,
updatesfalse/recoverypendingfalse. HIL59 manifest
b7e13ca38a989b2b62b52c2a7fe06e11f3e62998a884aaff34ade7210a14777b;
maintenance57 digest86be6abc24180e133977d9d5a6f4ad6a1ca3ab46234f76559db521a68e308b1f.
Only numeric foot parser and pins added to existing frozen HIL; guards unchanged.
Stadium saved fixture generation8/revision9 loaded/adopted, native60Hz restored.

HIL59 trial1792217442592376000 establishes standing A and repeated A while moving
laterally across the flat apron. Independent source-A events95162830/95166830;
valid local grounded504 baselines; fresh native air edges129ms after each A.
PelvisVy peaks6.843/5.101m/s, native Y rises0.951/0.886m, then negativeVy and
native landing at980/930ms after A. No ragdoll, wipeout, malformed rows, sequence
gaps or truncation; position age<=7ms, one owner epoch. Native flip stays-1,
with no synthetic on-board Ollie. Native state/idle, neutral and fixture reset
all PASS. Quiet pre-A grounded interval is the negative control. Arcade OFF
comparison1792217547962775000 also PASS: native client observed physical A#5/#6,
two fresh offboard air transitions, positive pelvisVy, native Y rise, descent
and landing, no ragdoll/wipeout/gaps, native state/idle and neutral PASS. OFF
source-pad diagnostics are deliberately disabled; client A edges and native
motion remain independent. Native Y held1s remounts successfully; the earlier
200ms OFF remount failure remains recorded. Airborne flips, basic B and new
manuals still pending. Arcade ON restored after the OFF control.

Preserve1792217312524657000: standing jump independently passes, but straight
running path ragdolled just after the second landing near the rail. REJECT that
moving landing; the clean lateral rerun above supersedes only its acceptance,
not its recorded failure. No velocity, airtime, tuning or original files changed.

## Native motion evidence and late-stroke correction — 2026-10-16

Source audit pinned official upstream4c07f2493bccf6b7f42c4cff17f9bd7e6efebeb8
and searched its complete1427-file tree. Reused HallOfMeat::read_step in the
existing NoBail physics listener: offboard air/ragdoll and pelvis velocity.
Trainer::telemetry supplies separate position timestamp/entity provenance;
unreadable/stale/changed owners invalidate evidence. Numeric20Hz snapshots retain
flag edges through the existing bounded queue; no new hooks, scans or physics writes.
Physical A routing remains native. Standing/moving/repeated/OFF jump trials require
valid grounded504, physical A, fresh native air edge, pelvisVy>2.2m/s, Y rise>0.1m,
descent and landing, no ragdoll/wipeout/gaps; no-A baseline is a negative control.

Flip timing reuses SessionSticks0.6.1 GPL source core.c build_stroke(rest_first)
(survey archive b6d7c9dfec2cbdec45128f172ab99344857f3bd30fd94580fbe411c8433c78c8):
30ms native neutral reset,10ms load,35ms setup/travel and60ms end hold, quantized
to game ticks with interpolated authored paths. Replaces unproved one-frame air
setup, retaining ground Ollie, native IDs and delivery ownership. This is a timing
repair, not a new trick catalog. Early/apex/late Kickflip2 and Heelflip3 acceptance
requires independent Ollie1/air followed by fresh exact native selection; requests
alone never pass. Basic B and both manual identities still require hardware proof.
33746 model,499 manual,99 runtime and52 playback checks pass ASAN/UBSAN;
1328 telemetry checks and3 bounded-parser tests pass. New runtime is NOT deployed.

## Mandatory source-first correction — 2026-10-16

WITHDRAW the on-foot jump hardware acceptance claim below. Jump HUD labels and
screenshots are secondary evidence; they do not prove a fresh takeoff or its
input cause. Existing native source already provides the required offboard flags,
pelvis velocity and takeoff detector. Earlier visual trials remain preserved,
but standing/repeated/moving/OFF jump acceptance is PENDING guarded native proof:
grounded504 without ragdoll, physical A event, fresh native airborne edge,
positive pelvis Y velocity and displacement, descent and landing, plus negative
controls. Reuse HallOfMeat::read_step, NoBailSkater ownership/physics listeners,
and Trainer motion provenance; no memory scan or artificial physics.

Installed9bbff13 / DLLb92b633e remains known. Source d9ac4a6 is building in
CI39005469689; no newer DLL deployed. Last native OFF comparison
1792214149269363000 completed and neutralized; it is not jump acceptance.
Source-first audit now precedes further mechanic design or implementation across
all four core controls, including existing official upstream and surveyed forks.
No scoring/combo work resumed.

## On-foot A hardware acceptance — 2026-10-16

HIL58 peak-aligned trial1792213720094790000 on installed9bbff13/b92b633e
visibly establishes native Jump from standing, then a second native Jump while
moving after landing. Correct capture triggers4s/8s follow A3650/7650ms;
artifacts/ding-bat-foot-peak-1.png and-2.png show feet clear of the ground and
two native Jump entries. No authored playback or syntheticX, native504 foot
context, ordinary chord remount succeeds, all state/idle criteria and neutral/
fixture-reset PASS, no evidence gap. Native guide independently confirms A.
The mistaken A→X adapter from4f5fbe5 is fully removed in installed9bbff13.
Native OFF restoration comparison remains to be completed explicitly.

Early-X observer-readiness source and locally saved manual/basic-grab changes
pass combined strict ASAN/UBSAN suites:33746 model,499 manual,99 runtime plus
all existing control regressions. Next source build is not yet deployed; early,
apex, late Kickflip/Heelflip and normal B acceptance remain required.

## Native air Kickflip established; early-X handoff source — 2026-10-16

Installed9bbff13c11e299272c1d1ee5ac3cd43849fdfdc8 / DLL
b92b633eb7a2518a5f15c2e325888da1828fa0928781a6ac5e249c4b290b1d49.
CI39001777689 passed all five gates; raw and immutable package DLL match.
Guarded stopped-state deployment retains gamefbce74d5/stock5512a15e,
updatesfalse/recoverypendingfalse. HIL58 manifest
8ae6b988b1ee2c1e78dbc926fb31e3ea5c489005364d24a2175193672691a226;
maintenance56 digestd267c44ea48b75326ff9b1842bb9cccc8d61b41614e604aa3fb1f8eddb89bc1c.
Pin-only copies; frozen workers unchanged. Launcher Ready25414733 checked;
Stadium generation8/revision8 confirmed1792212194792394000, saved fixture loaded
and adopted, native60Hz restored.

HIL58 early-X1792212478734742000 FAILS exact Kickflip: native Ollie1 only.
Native air precedes X, but selection1 is still active. Apex-X1792212607514078000
PASSES exact Kickflip2 after independent Ollie1/air; all native/idle/neutral/reset
criteria pass, no gap. Source now uses the existing owner-validated physics
observer (no new native read) to wait for fresh native air with no active flip
selection(-1). A released early X survives ONLY that same air window; landing,
menu/OFF, lost context or a newer grab/grind intent discards it. No fixed wait,
no stale future-jump queue and no physics writes. Hardware early/late/Heel trials
remain required after the next build. Manual/basic-grab commit35ba9f1 is saved
locally and not yet pushed/built/deployed.

The native Off-board Jump guide1792213558729192000 explicitly shows A; screenshot
artifacts/ding-bat-offboard-jump-guide.png. A-only trials1792212273497226000 and
1792212389990254000 produce actual Jump HUD without authored playback or nativeX.
Their screenshots do NOT establish jump peaks: trusted periodic capture triggers
at0/one-third/two-thirds/end, while elapsed_ms is recorded AFTER encoding. Earlier
frames were taken before the scheduled press; lack of a visible jump in them is
not evidence that standing A failed. Single X/B control1792212667509698000 and
A+X1792212859733654000 retained for comparison; no unsupported mapping promoted.
Peak-aligned standing/moving A trial is running. Full repeat/visible/OFF acceptance
is still pending. All trials neutral/reset; intentional foot/remount idle failures
remain recorded. No scoring work resumed.

## Manual responsiveness and basic-grab source ready — 2026-10-16

Runtime remains4f5fbe5 / DLL0e185a8a; stopped-state1792211207948586000 confirms
both game and launcher absent, original game/stock unchanged, updatesfalse,
recoverypendingfalse. Source9bbff13 CI39001777689 is still building Windows;
Linux/macOS gates pass. No new runtime is deployed yet.

HIL57 manual baseline1792210643848860000 reproduces ignored rapid D-pad Up/Down
over long-held forward: no synthetic manual stick, no native manual label, no
evidence gap; all native/idle criteria, neutral/reset pass. New focused test
fails against HEAD recognizer and passes after separate stick/D-pad channels.
350ms window and native partial-stick recipe remain unchanged.499 manual checks
include30/60Hz rapid pairs with/without neutral, held-forward nonactivation,
no cross-device pair completion, and bounded apex-to-landing preparation. D-pad
vertical correction now uses the established manual balance path. Ground/air
Manual AND Nose Manual hardware acceptance remains required.

Native OFF controls1792210929031828000 establish actual BS Grab HUD with RT
alone. RT followed by up after150ms still produces BS Grab, not Melon; earlier
50ms candidate also failed exact identity. Both controls have native100→201→100,
idle and neutral PASS, no gap. Source adds distinct neutral-B basic_bs_grab
without renumbering old actions; normal classic mode gates unproved named grab
recipes, while explicit lab mode retains them. No false named support claim.
Combined sanitizer checks pass:33742 model,499 manual,99 action runtime plus
existing regression suites. Manual/basic-grab source is not yet built/deployed.
Camera input retains its existing Arcade owner on foot. Air flip pilots remain
pending installation of the verified9bbff13 build. No physics writes added.

## Core gameplay pilots and airborne source — 2026-10-16

Installed runtime remains4f5fbe5 / DLL0e185a8a. Stadium saved fixture and60Hz
restored. HIL57 trial1792209954009004000 holds forward continuously10s with
native full-left steering on the flat apron. Independent push-request rising
edges87673859/87674959 accompany speed4.90m/s at1s,6.85 at2s,6.08 at9s,
5.98 at10s; native state100/mode1 throughout, brake reaches0 by12s. Screenshot
artifacts/ding-bat-flat-tight-2.png verifies flat course, no gap, all state/idle
criteria and neutral/reset pass. Earlier gentle circles1792209301698374000 and
1792209525733163000 hit geometry and are not flat-ground acceptance. Broader
release/resume and incompatible-state acceptance remain required.

CORRECTION: native on-foot jump is A, not X. Native OFF comparison
1792210065933189000 changes native request words after A alone; Jump HUD persists
through the later X, which caused the earlier mistaken attribution. Installed
A-to-X pilot1792209898156278000 has no jump request and is a failed repair.
Source removes that translation and preserves native on-foot A outside all
Ollie/flip/manual handlers, including recovery. Source right-stick input is also
preserved on foot. Neither the corrected on-foot source nor new independent air
X/B runtime is yet deployed. No hardware jump acceptance is claimed.

Air source accepts fresh X only in independently observed air, starts the existing
exact authored flick after one delivered setup tick, cancels on landing/bail,
and releases completed neutral tails before another owner. B tracks neutral
releases during Ollie ownership and dispatches immediately in classic mode;
ground B keeps native braking, stale requests are never queued. Named grab plans
remain candidates; previous Melon control yielded BS Grab and is not exact
identity acceptance. Kickflip/Heelflip and basic grab pilots are required next.
Manual command responsiveness remains outstanding. No scoring work resumed.

## Held-push/on-foot runtime installed — 2026-10-16

Installed4f5fbe566b3a95e8d61efd3a448f61a064db0330 / DLL
0e185a8a92c74d08c53c78cf5c7b9f260257e3b31a4cfed0a5bb03ff27d3a663.
CI38998301227 passed all five gates; raw and immutable packaged DLL match.
Stopped-state1792208662734365000 precedes guarded deployment. Original game
fbce74d5/stock5512a15e unchanged, updatesfalse, recoverypendingfalse. New pin-only
HIL57 manifest2fda19e36816e0c530e2649390ba9ef5f58023f786dbae94b7ad2c5937efbaa7;
maintenance55 digest483da060565f52b6322455327f2435f46054957ae340ed75e1ddacee0bcacf7f.
Frozen workers and older tools unchanged. Launcher Ready25414733 visually checked.
Native Stadium/60Hz restoration and full push/on-foot acceptance still pending.

Bounded grab pilots on the previous da1 runtime:1792207357264612000 reproduces
first airborne B discarded because auxiliary rearm was reset by Ollie ownership.
Method control1792207680102233000 dispatches, but native HUD shows BS Grab,
NOT requested Melon. Nose controls1792207849901664000 and1792208303353491000
do not establish named Nose Grab. All reset/neutral and state criteria pass,
no gaps. No named grab is promoted. Source air X/B handoff/rearm work is in
progress and not built/deployed; no artificial airtime or physics changes.

## Native held-push refinement and on-foot source ready — 2026-10-16

Installed runtime remains da1e762 / DLL278ee129, Stadium saved fixture restored,
native60Hz readback captured in artifacts/ding-bat-60-readback.png. Trial
1792206629078066000 gives multiple independently observed native push requests
(request_words[0]&0x40, already used by supported trainer code) and acceleration
4.96→6.82m/s. It still hits course geometry, so ten-second flat acceptance is NOT
passed. Neutral/reset pass; returned-to-idle criterion fails before reset.
1792206736881376000 shows braking/resumption and exact native Ollie, then a bail;
neutral/reset pass, full scenario fails. Preserve both limitations.

One ordinary native OFF held-X control1792206917876989000 reaches4.816 then6.709
m/s in about2s and brakes to zero, all three state criteria/idle/neutral pass,
no gap. Source client_noclip.cpp documents the native held-push speed steps;
short repeated taps are not equivalent. New source therefore holds native X
until cruise and permits its own one-wheel kick ONLY with independent native
push request0x40, while established balance, missing contact, unsafe physics,
manual input ownership, bail, menu and OFF remain exclusions. No physics writes.
Combined strict sanitizer suites pass, including33680 model checks. This held
refinement and the separate on-foot A→nativeX source are not yet built/deployed.
Current Deck is native OFF, stationary/riding after the native hold control;
fixture is preserved but pose is ordinarily steered away from its reset start.

## Push repair deployed; native control verification continuing — 2026-10-16

Installed da1e762f7a809ce03a7cf4a8bcb63b89e29f51a0 / DLL
278ee129113221acb074ab920f2e8438bb6368a4b63b400a62a41e7cc733c09d.
CI38996213467 passed all five gates; raw CI and immutable packaged DLL bytes
match. Stopped-state1792206230857679000 confirmed both processes absent before
guarded deployment. Supported game fbce74d5 and stock backup5512a15e unchanged,
updatesfalse, recoverypendingfalse. HIL56 is a pin-only copy of frozen55,
manifest1338cbd5164e489a37edab413880cc0ea24345913ba83040d4bcc018055d6b5e;
maintenance54 similarly changes only runtime pins. Existing tools unchanged.
Launcher visibly Ready/build25414733 after the existing guarded Steam Escape.
Game is starting; fixture/60Hz restoration and hardware repair acceptance pending.

Complete baseline1792205304985308000 independently reproduces the push defect:
first native acceleration reaches4.954m/s with a one-wheel contact exclusion,
then sustained forward coasts above4m/s for about6s with no further synthetic
push edge. No evidence gap; neutral/reset pass. Later obstacle/contact losses
are retained separately. Repair preserves intent/cadence across those exclusions.

Native OFF on-foot comparison1792205600560120000 shows native Jump HUD after X,
no Jump result after A; follow-up1792205669283663000 keeps the native mapping
pilot. Both neutralize and return to riding; truncated numeric evidence remains
UNVERIFIED and is not promoted. Source-only on-foot A→nativeX adapter now bypasses
on-board playback, preserves native recovery and cancels on unsafe state/menu/OFF.
33662 model checks plus existing combined control suites pass under sanitizers.
No on-foot repair DLL is built/deployed yet. Air X/B and manual responsiveness
remain outstanding; do not resume advanced score work.

## Core controls repair in progress — 2026-10-16

Automatic push regression reproduced in the pure controller: after an eligibility
gap at4.5m/s, sustained forward never restarts because stop() clears acceleration
intent above the4m/s resume threshold. The fix releases synthetic X immediately
on exclusion while retaining intent and cadence until movement release, menu,
toggle or cruise. Pulse180ms/cadence850ms and4/6m/s thresholds unchanged.
33648 model checks pass with strict warnings and address/undefined sanitizers.
Hardware acceptance is PENDING; this is not a verified gameplay claim.

Deck status1792204887724192000 confirms8079c14/DLL788cdba0 with supported game
and original stock hashes unchanged. Profile/Map menus were visibly dismissed
using fixed Back actions. Trial1792204933146075000 refused fixture reset and sent
no gameplay input. Trial1792205069473215000 delivered10s forward, neutralized and
reset; screenshots show movement followed by the far barrier. Bounded evidence
reports tail_limit/grind_evidence_incomplete amid noisy idle physical-stick logs;
it does not establish a sustained speed curve or repeated native push acceptance.
Next: isolated push trial with existing actions-off selector, then CI/deploy/test
the repair. Off-board A, independently airborne X/B and rapid manuals remain
required next priorities. Preserve all historical evidence below. Existing dirty
trajectory/fixture/parser/method-matrix work is retained outside this change.

## Rail-clearance repeatability and bounded trajectory collection — 2026-10-16

Source093ad76a4c7d6763df377c66835b07aead9bf593; installed runtime remains
8079c14363f7d66e76b186d73fe1281e650d6ca9 / DLL788cdba0c2d47c8dc007575fd4c20e50ee6bb3e820583079fb466db0cd1f7b19.
Original Stadium fixture and supported25414733 game unchanged, measured60Hz.
Trajectory batch1792175961255279000 attempted4/6 before a genuine collection
limit stopped it: original timing missed2/2; +200ms had1 full-chainPASS and1
UNVERIFIED. The fifth record is a stop receipt, not another attempted trial.
All4 neutral/reset; planned +350ms trials were not attempted. Native-invalid
trajectory terminal in the third attempt is a separate preserved limitation.

Without optional trajectory,1792176257349805000 gives +200ms1/2 and +350ms2/2
full-chainPASS, exact native2→2→2, one grind, continuous fatigue and valid grace.
All4 neutral/reset. Eighteen +350ms repetitions1792176487738322000 finish14/18
full-chainPASS: compact6/8, standard8/10. With earlier2/2 this is16/20, not20/20.
All14 actual grind entries compose successfully; four no-grind failures remain
in the denominator, including one native wipeout.18/18 finally neutral/reset.
Three misses have pre-pop speed~3.96m/s versus~4.54m/s in passes; fourth bails
at normal approach speed. Fixture position/heading stable; source-side bounded
minimum-speed wait is being added to measure this discriminator. No physics
changes. Money remains UNVERIFIED; misses/incomplete evidence retained.

Reviewed source15221c9 limits explicitly armed trajectory collection to512KiB
(measured maximum301681bytes); ordinary reads stay256KiB. Native-invalid,
prefix mismatch, truncation and overflow checks remain fail-closed.399Python,
11focused tests and3digest-rejection checks pass. Separate HIL54 installed,
manifest1b799753f44f4b92e27d1cc9a273407515289ed4e1094838dfe27d14e1f6e738;
same8079/788 runtime pins, frozen53 unchanged. FirstHIL54 trajectory trial
1792176912351967000 full-chainPASS, neutral/reset, no transportgap and prefix
MATCHED. Native trajectory still stops INVALID atseq76 while grinding; its
path is incomplete and not promoted. Existing-read validation diagnostics are
being investigated independently; collection fix did not hide nativeinvalidity.

Sourcebe2dcea adds bounded static binder-record diagnostics without new native
reads or ownership authority; CI38949416941 pending Windows. Source093ad76
adds exact unique local-spawn declaration lookup with both GUIDs required and
existing asset/read limits;152sanitizer checks and independent review pass,
CI38950775268 pending. Neither change is deployed. CI38947777388 at0d10ca8
passed all five gates. Next: finish repeatability batch, one bounded trajectory
capture, then verified runtime diagnostics/inspector to resolve score ownership.
Root retains sole hardware ownership; no second rail has been placed.

## Repeated Manual20/20 and post-grind landing refinement — 2026-10-16

Current source0d10ca850fc41ae39a839068343a529030f47039; runtime still8079/788.
HIL53 result1792175262921339000 adds17/17 passing repeated Manual chains to
the3/3 pilot:20/20 total,tenstandard/tencompact commands,exact2→1→2→1,
continuousfatigue,749–766msgrace,20/20neutral/reset. Native monetary banking
remains UNVERIFIED; this acceptance is controlled60Hz, not30Hz or repeatedNose.
The existing native-method matrix now records strict current admission evidence.

Post-grind native-riding probes1792175720346321000:compact1/1 fullchainPASS
with exact2→2→2 and750msgrace;standard1/1 MISSED_CONDITION after real rail
recontact (twoentries), no falsemanual promotion. Both finallyneutral/reset.
Six trajectory-equipped original/+200/+350ms flip-out comparisons are running,
twoeach; only shift existing command timing to measure rail clearance. No new
rail, physics changes, synthetic state forcing or repeatability claim yet.

CI38945032179 at2e10278 passed all five gates. Exact metadata inspector
f9caa1fcf962992f9ab534b6da5b7ea422d31ed99c1a62a9a9a47f6627721254
ran through the reviewed bounded Proton workflow; output
artifacts/score-assets-2e10278-1792175858618737000.ndjson,INSPECTOR_RETURNED,
gameprocesspresenttrue. Six prioritized declarations now captured within the
existing32asset bound; owner/producer remainUNVERIFIED. Runtime unchanged.
CI38947777388 validates0d10ca8 (menu+reasondiagnostics); not deployed. Further
static mapping diagnostic is source-only work: existing binder guard metadata/
alignment/frame assumptions are not generic binder proofs. Never relax them or
claim owner authority from the diagnostic. Root retains sole hardware ownership.

## Controlled baseline recovery and HIL53 — 2026-10-16

Runtime remains verified8079c14 / DLL788cdba0; game and original Stadium rail
unchanged. HIL52 batch1792174032767624000 stopped before its first trial:
fixture remained504 because a native RipCard/Profile/Map menu stack was open.
Three observed fixed Back actions restored gameplay without a restart. Source
5c6c870 adds current native-menu diagnostics via the existing validated UI
reader;391Python tests and independent review pass, not yet deployed.

HIL53 parser-only bundle installed separately, manifest
fbba5b7d42e41a4000384612192299fde19ecb95f8134a1dad3ad730aa799bc0;
runtime pins still8079/788, older tools unchanged. Batch1792174536628978000
completed7/7 neutral/reset, but all5 chain criteria failed. Delivered action
tick/wallclock slopes measured29.96–30.00Hz in six cases; clean-land sparse
estimate31.53Hz. This was NOT the intended60Hz comparison. First manual admits,
but subsequent air inputs/links fail; repeated cases do not establish the
history fix. Preserve these failed uncontrolled-rate results. Console command
requires an open console in maintenance52: invoking simulation-60 from gameplay
did not set the rate. Explicit console open/set/read verified native override60
in simulation-readback-8079.png; console closed and focused7trial rerun running.

Fresh bounded HIL53 binder snapshots are COMPLETE with zero parser loss.
Final cumulative route1unverified7,route2layout_rejected5,route4layout_rejected19;
relationship_matches0. Native binder is exercised; do not infer cached-binding
bypass from earlier truncated zero-counter captures. Exact rejected predicate
still unknown; next bounded reason counters preserve all existing checks.
Monetary score ownership/banking remains UNVERIFIED. Sourcec0f868a adds only
four bounded rejection reason counters, no new native reads/relaxed predicates;
independent review,250native-score/21health sanitizer checks,393Python pass.
CI38945032179 at2e10278:Linux/macOS and installer pass; Windows build pending.

Controlled60Hz rerun1792175001437752000 passes all5 chain cases:1 firstmanual,
3 repeatedmanual,1 regularrevert→manual. ExactairIDs/fatigue/grace pass;7/7
neutral/reset and negativecontrols grant no falsemanual. Three repeatedmanual
successes are DISCOVERED only;17more across7standard/10compact inputs running.
Trajectory1792175194208240000 has239valid relative samples,prefixMATCHED,
zero parser/sequence loss; timed window complete but terminal capture remains
UNVERIFIED because reset cancels after collection. Real grind336ms/oneentry/
oneexit,neutral/reset PASS. FullchainFAIL:no native manual history addition
afterflipout; brief native landing/air rebound occurs during manualhold. Do not
promote prior permissive one-wheel admission as this chain's proof. Next compare
event-driven postlanding manual requests before extending the fixture.
Original saved fixture preserved; no second rail placed.

Maintenance53 installed separately, SHA
6c0c3d35cdb6748aec40e67cbfb8c0b0f3f58ae0b382f4f2c66970f106189c5b.
Its four simulation/time operations now use the existing self-contained console
path: invoke from gameplay with console CLOSED, never pre-open. Other operations
and8079runtime pins unchanged. Structural/compile/eightmockedbranch checks pass.

## Reviewed manual/trajectory build installed — 2026-10-16

Installed8079c14363f7d66e76b186d73fe1281e650d6ca9, DLL
788cdba0c2d47c8dc007575fd4c20e50ee6bb3e820583079fb466db0cd1f7b19.
CI38940712773 passed all five gates; raw/package DLL bytes match. Controlled
stop1792173451856385000 confirmed both processes absent before guarded deploy.
Supported25414733 game/stock hashes unchanged,updatesfalse,recoverypendingfalse.
HIL52 manifestacbdee54a0c84d4e5e6ccbd96ddedab2f64411f75fdfc6a6657d169fec9ed6a6;
maintenance52 pinb2870bdcce566a8605f7169058fac081ea1da1d4e63e3965411e0df81661b9e4.
Older frozen tools unchanged. LauncherReady25414733 verified after remotely
dismissing Steam idle overlay, no authentication. Stadium8/8 observed before
saved fixture load/adopt/editorclose;60Hz restored. Seven-case manual/history/
revert regression is running onHIL52; next1 trajectory chain. Startup score
binderprepared/attachedtrue, counterszero. Parser9b82b05 corrects signed health
phase(-1406ms) interpretation, never grants authority. Offline inspector14f0740
prioritizes six exact captured producer/local-spawn assets within existing32/read
bounds.383Python+118metadata checks pass; independentreview/CI pending.
Root solely owns hardware. Monetary authority remains disabled/unverified.

## Repeat-manual fix and trajectory preparation — 2026-10-16

Installed Deck remains verified5f6f233 / DLL a49ea593d0b50a69472c7381084be5d2b13b0a17835bd895846182791234aefe,
Stadium fixture restored, editor closed,60Hz. No hardware owner besides root.
Source67ef0cc CI38936894869 passed all gates, including publication, but MUST
NOT be deployed: review found binder diagnostic preparation could indirectly
satisfy the existing progression-enable path outside its startup prerequisite.
The diagnostic has never run on the Deck. Source457f203 separates diagnostic
readiness from progression authority and preserves the primary player-card
observer; independent review found no blocker. Sanitized242native-score/21health
checks,33641combined model checks and378Python tests pass. All57 recorded
unowned captures still reject monetary authority. Combined CI/deployment pending.
Source8079c14363f7d66e76b186d73fe1281e650d6ca9 combines the reviewed binder
fix with trajectory prefix correction. Independent reviews clear both fixes;
381Python and38trajectory checks pass. CI38940712773 has Linux/macOS green,
Windows still building. No8079 runtime deployed yet. Original named rail layout
is unchanged; no second rail placed. HIL52/deployment preparation scripts are
local only and require exact successfulCI/raw-package identity before installation.

HIL51 Nose comparison1792170912645805000:6/6neutral/reset,0/6fullchains.
All lead-ins remained effectively stationary, so this is NOT a valid moving
versus stationary comparison. Nose labels were late and admitted only15–66ms
before the next native action. Corrected controls1792171445674275000 finish4/4neutral/reset. Moving trials
reach4.0–4.58m/s. Regular Manual confirms450ms before Ollie; movingNose0/2
and stationaryNose0/1 have no native Nose history/confirmed pose before Ollie
despite delivered+20000RS. Later rail contacts and one bail are separate outcomes.
Ground entry differs from the previously successful moving/prelanding context;
this does not establish an unsupported native mechanic. No magnitude/physics change.

Follow-up1792172092638657000 holds the same ground request1500ms longer:
Nose2/2 admits after798/830ms and remains confirmed1400/1366ms; regularManual
control admits306ms and remains1950ms.3/3neutral/reset. Thus prior650ms Nose
window was too short, not an unsupported native recipe. These are ground-entry
controls, not proof of landing-link timing or repeated Nose chains. No physics
or gesture magnitude changed.

Landing-to-Nose long controls1792172746045959000:2/2 native Nose recognition
but0/2 continuous chains; first combo expires750ms after landing before the
Nose history edge. Both neutral/reset. Flip stick ownership ends near1092ms,
prepared1150/1230ms controls1792172861592926000 pass2/2 continuity, fatigue
and750ms grace with native Nose admission717ms after landing. Both neutral/reset.
Exact requested air-ID grading fails2/2:observed2→16 rather than requested2→1.
ID16 is native Nollie; do not silently substitute the requested Ollie. This is a narrow33ms landing-window margin,
not repeatability acceptance or monetary verification.

## Manual identity runtime deployed — 2026-10-16

Source5f6f23363be75e54ff1b0a9e64dcae9ee262ab1c (CI38930044138) combines
dd8078e manual admission/nonblocking telemetry/type diagnostics with the tested
28c8f07 geometry parser and a diagnostic-cache shadowing rename for strict MSVC.
All360 Python tests pass; strict shadow-warning sanitizer runs pass43 history,
470 admission,1323 telemetry and192 native-score checks. Combined model33641
and recorded57capture rejection remain unchanged.
Original dd8078e CI38928009142 failed solely on MSVC C4459 for the renamed
diagnostic variable. Geometry-only28c8f07 retains the same earlier source.
CI38930044138 passed every gate, including Windows and publication. Raw and
packaged DLL match:a49ea593d0b50a69472c7381084be5d2b13b0a17835bd895846182791234aefe.
Guarded deployment completed after stopped-state1792168689316404000;
game and stock hashes unchanged,updates=false,recovery_pending=false.
HIL51 and maintenance51 installed separately; older tools unchanged.
Restart encountered Steam idle overlay, dismissed with the existing guarded
Steam Escape action. Launcher visibly Ready/build25414733, no login required.
Saved Stadium fixture restored after observed8/8 guard; fresh status
1792169409019016000 confirms configured/present,editorclosed,revision9 and
original rail/start coordinates. Current12-trial batch establishes native riding
baseline and covers3 clean-land controls,3 first-manual chains,3 repeated-manual
diagnostics and3 missed-rail controls. Then bounded rail geometry.
Read-only schema1792169169543265000 is complete:8line/24sequence fields,zero
parser loss. It establishes neither local owner nor occurrence identity.
Context GUID still lacks a supported metadata lookup in captured declarations.

HIL51 result1792169444622320000:12/12neutral/reset. First-manual chains3/3
PASS; repeated-manual chains0/3 because label-union admission misses the second
identical label. All3 second entries have a distinct Manual presenter addition
while local native100/mode1/0x4000 persists; history diagnostics have no parser
loss. Clean landing/missed-rail controls grant no manual balance. Root repair
uses fresh distinct additions only to corroborate native contact; monetary
authority remains disabled. Local144history and470admission sanitizer checks
pass; all365Python tests pass. New hardware repeatability remains required.
History pruning/removal deliberately invalidates admission; capped histories
remain a conservative failure envelope. Binder diagnostic623e941 adds only
bounded mapping counters via the existing guarded hook; activation needs new
CI/native validation and never enables progression or monetary authority.
Read-only geometry1792169821067657000 completes129/129rays with current8/9
context:height0.4997m,axisZ sampled[-2,+1.75],0.25m uncertainty. Original saved
fixture unchanged; next longer-chain layout must use these measured dimensions.

Hot8ab Nose context comparison while CI ran:1792167499330441000 gives0/3
stationary repeated-nose chains, first nose contact only50ms and no sustained
second entry, despite delivered+20000RS. All3neutral/reset. Existing moving
recipe1792167699522151000 gives1/3full chains:one missedrail but sustained
nose482ms,onepre-commandbail,andonepropergrind/nose533ms. The moving candidates
had prelanding input; this is context evidence, not renewed grind acceptance.
Prelanding-only comparison1792167871961533000 still0/3full repeated chains:
twofirstnose sections316–317ms, onebrief34ms, no sustainedsecondmanual. All3
neutral/reset. Timing alone has not resolved stationary Nose composition; no
additional recipe tuning before new identity diagnostics.

## Native composition and software checkpoint — 2026-10-16

Current source7ccce3c644b8374bf2d851480b997a6e2df3e1d5 includes opt-in,
bounded fixture cross-section measurement and explicit offline chain timing
policies.333 Python tests and854 geometry sanitizer checks pass. CI38923734572 passed Windows, Linux, macOS, package and publication, including
new geometry and Park Editor transaction regressions. No new runtime deployed.

Installed source8ab84cd1209a49c477042ef5f1248bcd7d8ab098, DLL
042a6ad2f17c4dccc90751766d2ef2a05263a454ed95bc081d1196d631ebb094.
Supported25414733 game, stock backup and recovery launcher remain unchanged.
The game is running in the standalone installation, Stadium single rail restored,
editor closed, Arcade ON/default mode5,60Hz. No authentication intervention.
HIL50 manifest0d5ed482b6134927d03efef1806e625d2e36e9323117aed55c6eb66208d9e6e9;
all older frozen tools remain unchanged.

Native revert/manual composition: result1792163073714564000 adds17/17 to the
three350ms Ollie-out controls in1792162893360510000:20/20 across ten early/ten
standard commands, single grind entry, retained fatigue, native air IDs, final
750ms grace, neutral/reset. Controlled regular-context mechanic REPEATABLE;
monetary and broader stance acceptance remain open. Previous220ms18/20 and
its two rail recontacts remain recorded; no physics/grace/input recipe change.
The same comparison also passed3/3 Kickflip-out revert/manual links.

Mixed result1792163515127404000 completes12 cases with12 neutral/reset-ready
recoveries and no hard stop. Five graded native chains pass; tripleRT permits
one opportunity and expires exactly1100ms later. One intended grind-failure
case misses the rail entirely, so it earns no grind-failure credit. Fixed-rail
brake comparison1792164223478905000 isolates the approach:300ms bails before
grind;400ms and500ms enter native grinds at2475/1831mm/s for2432/2449ms, then
physical balance wipes out. All3 reset safely. The original600ms stopped short.
These controls preserve historical20/20 physical grind-failure acceptance while
measuring the current approach failure envelope. Rotation/board smoke cases do
not replace independent angle or full stress acceptance.

Chain-C pilot1792163828985339000 produces native2→1→2 in3/3 but finaltakeoff
983–1000ms after landing exceeds750ms grace:0/3 continuous chains. Earlier
command1792164333661204000 passes1/3; event-wait latency plus400ms crouch still
leaves two boundary failures, including one late first manual. All6 neutral/reset.
Shorter manual spacing and300ms final crouch in1792164460296133000 pass3/3;1792164620483477000 adds17/17, for20/20 across ten standard/ten compact timings. All retain native2→1→2, intended manual, fatigue,750–752ms grace and neutral/reset. Monetary authority and midair late flips remain unverified. This is three
native hops with intermediate landings, not evidence of a midair late flip.
Repeated-manual diagnostic1792165157533252000 completes3/3 native2→1→2→1
sequences, two sustained contact candidates and clean resets, but only the first
manual produces a label edge. History rows4→5 at second entry merit bounded
identity diagnostics, not proof of native occurrence. One-tick one-wheel
landing and missed-rail candidates lack matching manual labels. A tested
strict admission boundary now separates control preparation from combo/HUD
identity; it is not accepted until repeated-manual integration is resolved.

Prepared source adds bounded nonblocking input/client telemetry with explicit
loss, manual-history count diagnostics (no handles), and stable native scoring
type metadata snapshots. Local sanitizer checks:1323 telemetry,470 manual
admission,43 history,192 native score;353 Python tests pass with the new
history/schema parsers. Combined model regression passes33641 checks, including
328 settlement checks; all57 recorded unowned captures remain rejected. None establishes new hardware or monetary acceptance.

Scoring windows1792162733637269000 complete two exact native Kickflips and two
physical manual bails, all neutral/reset ready. Baselines bind in1909–2841ms;
historical route2 unbound count4 remains unchanged. Clean line ends occur10051ms
after start; bail captures include both failed and success-valued subsequent
events. Ownership, canonical occurrence, raw-point semantics and end-boundary
health remain unverified.21 separate trials/57 captures replay with zero
authoritative monetary operations;17 focused replay tests pass. The delayed
old-generation baseline diagnostic1792162265048983000 and its correction are
preserved.32-level capture-walk exhaustion now records loss;167 native-score,
21 health and328 settlement checks pass. Native score authority remains disabled.

CI4780c89,1e02a0f and7ccce3c passed every gate. Earlier1a45a35/eeba163
publication403 failures remain recorded; no permissions changed. The corrected
eeba163 read-only inspector5453381e8c0f96766733214d1108f9d34a49bacc4b1b70fd59a65ff126983000
ran at idle:result1792165018770361000,32 assets/164532bytes, bounded INCOMPLETE.
Serialized imports identify actual scoring types but Context remains unresolved;
manager cString IDs and UI presentation do not prove local event ownership.
Game stayed present. Next: CI the coherent runtime, deploy only verified
artifact, capture native schema and repeated-manual diagnostics, then targeted
manual regression and fixed-rail geometry for the longer fatigue chain.
Do not infer bank/loss from unowned score captures.

## Default RT runtime update prepared — 2026-10-16

CI8ab84cd passed Windows/Linux/macOS/Deck/publish. Its packaged DLL exactly matches
the raw CI artifact:042a6ad2f17c4dccc90751766d2ef2a05263a454ed95bc081d1196d631ebb094.
HIL48 is separately reviewed/frozen (manifest5e8b7d5b66a70f18e8add9acfb2ca306c0371e431f3fb45cc4fd3de74aec4276),
with score-health parsing, mode5 and the optional neutral scoring baseline gate.
Maintenance49 uses these verified target pins; all earlier tools remain unchanged.
The keyboard close was ignored; a reviewed exact-window native close stopped both
game and launcher. Read-only stopped status1792160934853292000 preserved all old
pins. Guarded deployment is in progress; new native acceptance has not begun.

The next metadata inspector exports strict declared parameter names/qualifiers,
a bounded allowed-prefix manifest-name inventory and three exact imported type
identity lookups.85 sanitizer checks pass. No extra payload scan, guessed owner,
native offsets or game-build contract change. This source awaits CI before use.

## Hot native score windows and declaration inspection — 2026-10-16

Source966aa6b remains the latest pushed checkpoint; runtime919/ac6bc714 and HIL47
remain installed. Hot result1792159939246683000 runs two separate clean Kickflips
without another fixture console reset. Both capture native sequence score13,
line start16/17ms later, and line end13/reason1 after10,035ms in unchanged epoch259.
Both neutralize and return idle with no reported capture loss. This supports
reset-time rebinding as the earlier missing-record cause; it does not prove owner,
native occurrence identity or raw-point semantics. The replay corpus now retains
17 separate real trials/45 captures, all rejected as monetary authority.320 Python
checks pass;16 focused replay tests preserve exact timing without stitching trials.

CI22193dc is fully green. Default-RT candidate8ab84cd and combined checkpoint966aa6b
await Windows completion; no unverified runtime has been deployed. HIL48 is staged
for review, not installed, including mode5 parsing and the optional bound baseline.

CI-verified inspector d689ef2 ran while the game stayed hot: artifact SHA-256
f0bcec065b709248499f125de66cd2dbaafd3a7948c88ab74a22d9bacbd13e69,
result1792160075371998000,32 assets/151238 declared payload bytes, INCOMPLETE.
It found two of four graph targets and shared line/sequence declaration types.
The line-start/end graphs share one parameter import; its role remains unknown.
Next bounded lookup: parameter declarations and three exact imported type GUIDs,
with missing references explicit. Static type links are not local-owner proof.
Game, stock backup, launcher, saved fixture and all frozen tools remain unchanged.

## Bound scoring baseline prepared — 2026-10-16

Optional score_capture_ready fixture trials now wait neutrally for a fresh
same-time four-route active/bound observer snapshot, with unchanged validated
local grind owner and riding evidence. Reset-time skipped counts become the
baseline; they do not masquerade as trial losses. Original native timestamps
and observation boundary are preserved. Timeout/evidence loss stops before
gesture input; interrupted trials retain the baseline receipt. Defaults are
unchanged.85 focused batch checks pass; no new frozen runner or Deck acceptance
for this gate yet. Source8ab84cd carries the default RT binding; its CI is pending.
Current Deck remains runtime919/HIL47, with two bounded scoring windows running
without another fixture console reset to isolate rebinding-related capture loss.

## Default RT binding prepared — 2026-10-16

New source appends action mode classic=5 and selects it at startup, reusing the
recorded regular-context native revert recipe. Existing selectors0–4 stay intact.
LT+RT is rejected in classic mode; LT appearing during playback cancels ownership.
LT authored nollie A/X, Arcade OFF restoration, the120ms board chord, release to
rearm and input-owner priority remain unchanged.81 sanitized action-runtime checks
and104 focused parser/integration checks pass. The new default path is NOT yet
installed or Deck accepted; current hardware remains91906e5/ac6bc714 with mode3.
Nollie/switch, stationary contexts and exact side identity remain separately
unverified. No claim of monetary score banking. Next runtime deployment will
combine this reviewed control change with score capture-gap diagnostics after CI.

## Revert/manual repeatability and score capture gaps — 2026-10-16

HIL47 result1792158793300151000 completes14/14 planned trials without readiness
stops, lost evidence or stuck input;13/14 native chains pass. Combined with the
six completed HIL46 trials:19/20 full grind→Ollie→native revert→manual chains
pass continuity, native air IDs, fatigue and750ms grace. Both command timings
were exercised: early9/10, standard10/10. The separate earlier hard-stop trial
is not erased or included as a complete gameplay result. All20 complete trials
neutralized and reset.

The one failed chain is a measured deadline boundary, not failed native revert
recognition: landing36535497, RT36535767, native10136536153, combo expiry36536247,
independent revert label36536264 (17ms late), continuation rejected. No grace
refresh or manufactured link; manual preparation then correctly remained gated.
The existing250ms heartbeat condition can delay automated RT admission after
landing. Keep this failure envelope; do not change gameplay grace merely to make
this test pass. Runtime919 and HIL47 stay installed/hot at the Stadium fixture.

New source-only scoring health diagnostics count skipped known-graph callbacks
while unbound/expired or after an epoch race, and emit bounded timestamped worker
snapshots. Existing offsets, schema guards, ownership=false and event decoding
remain unchanged. Sampled health cannot prove completeness; cumulative losses
before the trial are distinguished from increases between its samples. New
parser/replay tests reject malformed/lost detail and health evidence, duplicate
or orphan capture details and unbound intervals. Fifteen separate original
trials/39 native captures replay with zero authoritative monetary operations.
New runtime diagnostics are NOT deployed or native accepted yet. Next: complete
CI and inspect fixed scoring declarations, then measure an explicitly bound
clean native line and physical bail before enabling monetary settlement.

## Conditional HIL parsing and scoring evidence — 2026-10-16

Runtime91906e5 / DLLac6bc714 remains installed; supported game, stock backup,
recovery launcher, fixture and60Hz configuration are unchanged. CIe2608f2 passed
all gates including publishing. Inspector source d689ef2 is in CI; not deployed.

HIL46 smoke1792157520917909000: early-manual chains1/2 PASS; the other recontacted
the rail and never obtained a native revert. The successful trial preserves Up
at native101 then delivers manual only after100; native manual, persistent grind
fatigue, grace and neutral/reset pass. Two triple-RT windows each accept exactly
one opportunity and expire after1116/1100ms without refresh. Monetary authority
remains UNVERIFIED.

HIL46 repeat1792157728067445000:6 complete early-manual chains PASS; trial7 stops
on readiness age101ms with correct context/owner. No remaining trials ran. Old
error reporting omitted trial7 partial input evidence, so it stays interrupted,
not a successful or cleanly unattempted trial. Read-only measurement reproduced
Python parsing contention: full parser age126ms versus idle46ms. Narrowing only
condition-wait parsing to existing native state/contact/owner evidence gives a
measured64ms maximum versus full106ms in the same new measurement. Full final
telemetry remains retained; the100ms safety threshold is unchanged.

293 software checks pass. New frozen HIL47 adds this parser change and bounded
partial interruption evidence, including completed neutral-write receipt and a
single read-only post-abort observation; it never resets/retries after hard stop.
Manifest7e5794f88f7b26161ade5b20f85c92348cdd8b92d0120c3c57a4f381673f018e.
All older frozen tools and919 target pins remain unchanged. Remaining4 early
and10 standard manual-chain trials are running, with the earlier interruption
kept separate.

Score window1792158343465402000:2 native Kickflips land, but capture only native
line-end, over9.3s after Arcade expiry. Two physical manual failures reach observed
wipeout17/16ms after request and reset model fatigue; later native success-valued
sequences have kind3/category10/base0, unlike failed kind1/category5/score5.
They cannot rehabilitate the failed combo or prove banking.4/4 neutral/reset;
actual bails do not return to riding before reset. Missing start/sequence and
unbound capture intervals remain explicit limitations even with zero drops.

First verified inspector85f25c9 ran with existing Proton runinprefix while game
stayed hot;32 assets/129316 declared bytes, metadata INCOMPLETE. Prefix ordering
exhausted the cap before target graphs. d689ef2 prioritizes the exact four graphs
and global line/sequence declarations across both fixed bundles, then exports
only bounded declared field names/hashes.56 sanitized metadata checks pass.
No raw payload export, offset guessing or ownership inference. Next: inspect
that CI-verified artifact after the active HIL batch; resolve score attribution
and finish repeatable native chain acceptance before monetary integration.

## Manual recovery runtime installed — 2026-10-16

Installed source91906e5a5183429d16f787d0f8f6bb5f2b99da1b, DLL SHA-256
ac6bc7141ee9ef40ba8a3042e572137a3ba41ded0623228eb140d6f9becb29a6.
CI38903969020 passed build/Linux/macOS/Deck gates; publishing alone failed403.
Used those exact CI artifacts through the existing verified-package parser and
guarded installer, independently matching the raw DLL artifact and commit.
No release, account permissions or workflow security changed. Both processes
were confirmed stopped before installation. Stock5512a15e, gamefbce74d5 and
recovery launcher997aeeef remain unchanged; old DLL7a8b254e remains preserved.
Launcher again reached Ready/build25414733 without authentication. Stadium
restoration and targeted native acceptance are in progress; not yet accepted.

HIL45 result1792156456487293000: first conditioned grind/Ollie/revert/manual chain
passes native identity, continuity/fatigue/grace, neutral/reset. Native riding
wait399.976ms moved RT until state100 with both-wheel contact. Second trial
stopped on stale startup readiness age109ms, context/owner still correct; remaining
trials did not run. Do not count this as4 attempted gameplay trials. The startup
fix now waits neutral up to250ms for genuinely fresh evidence, preserving the
100ms age limit and check-start timestamps.285 Python checks pass.

New frozen HIL46 adds only that readiness fix and verified919 target pins to45.
Manifest22778bba17ac8e71b1d402985062c23e2d394ec8c83570140a5760630f2299a8.
Maintenance48 uses those target pins and the previously verified focused native
close. SHA-256ffec26ada0d86d059e3809a756a81fba30132da535d8563b2b890dacf4906f38.
All older frozen bundles remain unchanged. Next: conditioned early-manual smoke,
then20-trial repeatability and fresh bounded score diagnostics. Monetary ownership
and bank/bail amount remain UNVERIFIED; no authoritative score adapter enabled.

## HIL transport and clean-checkout repair — 2026-10-16

HIL44 result1792155961380022000 is empty: a read-only status reproduction
confirmed SSH's `Argument list too long` error before worker execution. The
124,330-byte reviewed payload became a131,774-byte shell argument. HIL45 keeps
the exact44 workers/target pins and compresses only verified source, reducing
the command to43,282 bytes. Request JSON remains stdin; all existing checks,
timeouts and no-replay behavior remain. Frozen43/44 are unchanged. HIL45 manifest:
1717b4b80a36d9a75b4873647a3659adeae7181e74365484534d31d1709c95b6.
The failed batch is infrastructure evidence, not attempted gameplay acceptance.

CI38905135152 failed because a new staging test assumed the ignored artifacts
directory existed. The test now creates that directory on clean checkout.
281 local Python checks pass, including compressed-source round-trip, stdin
separation and transport bounds. Installed runtime remains2bdf062c; candidate
91906e5 still awaits its Windows build gate before any deployment.

## Native continuation convergence — 2026-10-16

Source c9e255b52585e25043db9056c1afe8166c0d25ae preserves the first manual
command direction during fresh state101/mode1 only while an independently
accepted revert opportunity remains active. Synthetic manual input still requires
the existing state100/air eligibility; unsafe completion cancels, and the350ms
command deadline, ownership conflicts, bail/menu/toggle cancellation remain.
Local119 manual checks and33,636 model checks pass. CI38903225629 is running;
this source is NOT deployed or hardware accepted. Installed source2bdf062c and
all recovered game/DLL/stock/launcher pins remain unchanged.

HIL43 is a separately reviewed copy of42 with only bounded correlated input
retention128→512 and its bundle version changed. Manifest SHA-256:
239810553b304d3258f8713565ee881159ab9a0c4d7288ad443000ddcc213407.
All old bundle hashes were checked; target pins and frozen42 remain unchanged.
Staged parsers match the tested source functions;36 focused parser/trace tests
pass. No scoring parser or runtime changes were imported into this bundle.

Result1792154379890949000: six native flip/grind/nose-manual chains completed,
all neutralized/reset with no loss.220ms and350ms grind-exit holds each3/3 pass
continuity/native IDs/fatigue. The saved plan still expected350–650ms grace and
therefore reports FAIL; actual748–750ms matches the installed750ms contract.
Preserve the original result and this expectation mismatch. This flip-based
comparison does not establish the cause of plain-Ollie rail recontact.

Result1792154660225524000: all8 attempts retained complete input/native evidence,
neutralized and reset. Manual command after transition2/5 passed full native
link/IDs/fatigue/grace, both at850ms from accepted revert to manual. Other3/5
recontacted the rail and rejected RT while airborne. The prior six early-command
failures are localized: first Up atRT+900ms was discarded in state101; return100
at+925–941ms made Down a new first edge. No synthetic manual hold was delivered;
auxiliary ownership had already ended. This is command admission, not a failed
native manual recipe.

The three repeated-RT trials retained all input rows, including140 rows in one
trial. Two accepted one native-confirmed continuation, then expired exactly1100ms
later despite three started RT requests; one recontacted the rail and never
accepted a continuation. No refresh occurred. Monetary multiplier/banking remains
UNVERIFIED; do not use the continuation grader (which requires a follow-up trick)
as an expiry grader. Result1792154925602997000 completes20 repeated-RT trials:17/20 accept exactly one opportunity and expire at1100ms despite3 started requests;3 reject the first RT during rail recontact.20/20 neutral/reset, no dropped evidence. Next:
finish bounded event-driven native landing readiness in HIL, validate the manual
fix on a CI-tested runtime, and measure native score payload semantics without
promoting unowned events. Game remains hot at the saved Stadium fixture.

Source91906e5 adds coherent manual pose/eligibility publication under the existing
input mutex and explicitly cancels pending directions across owner/menu changes.
33,641 model and119 manual checks pass. It also adds bounded read-only native
SequenceKind/ActionScore/BaseScore/Landing/current-category diagnostics;113 native
score and328 settlement checks pass with sanitizers. Authority stays false.
CI38903969020 is pending Windows completion; Linux/macOS and Deck software pass.
No runtime deployment yet.

New scoring window1792155462686766000:2/2 native Kickflips/clean landing; native
line-end route2/reason1 observed in both. Complete second trial has line start to
end10,034ms, over9s after Arcade grace expiry. First lacks sequence/start records
and remains incomplete. Two physical manual failures have real bail pose6 and
failed sequence receipts followed by later success-valued native sequences; those
later events cannot be treated as a clean Arcade bank.4/4 neutralized/reset.
Existing replay corpus now contains11 original trials/29 captures, including all
four new windows, with source/result hashes. Eight Python replay tests pass;
all29 C++ captures remain rejected as monetary authority. No fabricated zero
points, score identity, ownership, or banking proof.

Repository HIL now supports at most two bounded native-riding phase waits with
fresh same-owner heartbeat/contact/physics evidence; subsequent phases shift
coherently. A missed condition records a neutralized timeout and continues;
integrity loss still stops.279 full Python tests pass, including host persistence,
zero-frame recording, missing-condition and stale/changed-owner cases. Frozen43
is unchanged; a separately reviewed44 is next. The fixed timeline remains intact.

A portable read-only scoring metadata inspector reuses existing GameData/EBX APIs
for only fixed core bundles and scoring manager prefixes.37 sanitized metadata
checks and3 CLI checks pass locally. Windows CI builds/tests a separate inspector
artifact for existing Proton/Oodle support; workflow permissions unchanged. It
exports bounded declarations/references only, never raw payload bytes or values,
and cannot itself prove local ownership. Actual pinned assets still uninspected.

## Recovery launcher installed and hardware resumed — 2026-10-16

Reconnected to the paired Deck. Installed the separately tested launcher from
source e8e71a42ed1ed1753f3a5eb955020a3a78ae3a48, SHA-256
997aeeef5aeb9133c983a7827ac726199ad9ea63cf793af843d48255afae4329.
Old launcher5b11e30a is preserved as
~/Games/ReSkate/ReSkateLauncher.pre-recovery-fix-20261016.exe. The first close
request was ignored; the stopped-process guard refused installation without
writing files. A focused native close succeeded, then the guarded installation
verified runtime, stock backup and settings unchanged. No frozen tool changed.

Updated launcher reaches Ready to skate with build25414733, updates disabled.
Existing shortcut and Proton prefix remain selected. Play launched the game,
which reached the hub and then Stadium1 without QR, download or recovery.
Arcade source2bdf062c, DLL7a8b254e, stock5512a15e, gamefbce74d5 and
steam_api88b55dc3 remain exact. Evidence: artifacts/installed-recovery-launcher.json
and artifacts/separate-recovery-status.json. Normal startup is verified; future
remembered Steam authentication remains unverified because this launch required
no authentication. Do not deliberately redownload to test it.

Fresh power readback confirms Mains online1, Gaming Mode IdleSuspendACSeconds0
and Desktop AutoSuspendAction0; battery sleep remains900. Evidence:
artifacts/recovered-power-status.json. Previous connection loss is not proven to
have been caused by sleep. Both paired hostname and address work this session.

Restored the saved Stadium rail after fresh editor generation8/revision8,
adopted it and closed the editor. Native status confirms present=true,
configured=true, failed=false, editor_open=false, revision9 and the original
rail/start coordinates. Restored60Hz and auxiliary actions OFF; Arcade ON.
HIL42 preflight1792134636446255000 records the fixture. Prepared seven-trial
native-link integration batch resumed through the unchanged trusted runner.
No accepted grind/HUD suite was repeated.

Initial batch1792134665419948000:7/7 neutralized and reset, no dropped evidence.
The two composed grind/manual and kickflip/grind/flip-out cases each passed
continuity, native air IDs, persistent fatigue and750ms model grace (765/766ms
observed). Native score banking remains UNVERIFIED. The RT cases ran with action
mode OFF and must not count as native revert tests. Follow-up1792134949410116000
also lacked action events: the requested mode change was rejected because Arcade
was ON. Preserve those four results as invalid setup, not recipe failures.

Required action setup: open console, Arcade OFF, set actions-revert-fs, Arcade ON,
verify the native "Actions: RT frontside candidate" confirmation, close console.
Do not infer configuration success from the maintenance helper's requested result.
This preserves the existing mode-change guard. The native actuator remains an
explicit experimental selection, not an accepted default RT binding.

Verified enabled control1792135247578671000: one started RT request,43 owned
samples, fresh native label16777216/sequence14, uncertainty0. No active combo,
so no continuation accepted; input neutralized and reset. Enabled chain batch
1792135305249152000:2/2 revert-to-manual links PASS, independent labels16/19,
one1100ms opportunity consumed after984/950ms, continuous native poses/air IDs,
fatigue and model grace PASS. Still discovery, not repeatability or score-bank
acceptance. The repeated-RT case accepted one opportunity and expired exactly at
its deadline, but lost10 rows in each pad/stick/action stream: full anti-farming
acceptance remains UNVERIFIED. All three reset and neutralized. Evidence retained
in research/grind-exit-results.json with original result hashes and setup limits.

Repeatability result1792135457196907000:20/20 valid attempts with complete
evidence, neutralization and reset. Original timing9/10 passed full native
revert/manual continuity, IDs, fatigue and grace; the100ms-earlier manual timing
passed0/10. Total9/20, NOT REPEATABLE. Successful revert-to-manual intervals
933–967ms. The original-timing miss re-contacted the rail and RT was rejected
as unsafe context. Earlier-timing cases split between rail re-contact/unsafe RT
and independently recognized reverts with no subsequent manual. Preserve this
failure envelope; do not promote RT to default accepted controls.

Bounded-tail expiry follow-up1792135968903873000 also accepted exactly one
continuation and expired at its1100ms deadline, despite three started RT
requests. It again lost10 pad/stick/action rows. Source inspection identifies the
observer's128-row tails, not evidence of a runtime queue failure: the three
requests exceed that retention limit. Source probe.py now retains512 correlated
input rows (pad/source/stick/playback/action), still reports overflow and native
gap counts, and preserves validation/allowlists. Focused99 lab tests pass,
including138-row correlation,512-row bounds, overflow and explicit-gap coverage.
Full software self-test passes258 tests; naming and diff checks pass.
This is an offline observer fix; frozen HIL42 is unchanged and prior losses are
NOT erased. Anti-farming and score ownership/banking remain UNVERIFIED.

Hardware is left running at the native-reset Stadium fixture, Arcade ON,
revert-fs explicitly selected,60Hz, editor closed and input neutral. Next:
prepare a separately reviewed observer bundle before repeating expiry; use the
already-prepared grind-pop-hold-six comparison to isolate rail re-contact, then
measure manual delivery against native revert recovery. No arbitrary native
offset, game physics or stick-recipe changes were made.

## Supported installation restored — 2026-10-15

Source e8e71a42ed1ed1753f3a5eb955020a3a78ae3a48. The supported launcher
download succeeded after authorization. Separate installation ~/Games/ReSkate
now contains build25414733: executable fbce74d5 and steam_api64.dll88b55dc3.
Full read-only verification against the downloaded native Steam manifest
4621099302092747785 passed135/135 files,14,133,684,076 bytes. The cache SHA-256
is b98ac1e3bbd1a49a24b143335bad550c463bcd75f5083398b17b6aae3d09d552;
SteamKit's native manifest reader validated its cache checksum and manifest ID.
Evidence: artifacts/recovered-game-full-verification.json and
artifacts/recovered-manifest/inventory.json. These are game-file checks, not new
gameplay acceptance. The launcher visibly reports Ready to skate.

Installed Arcade source2bdf062c, DLL7a8b254e and stock5512a15e are unchanged;
updates=false, recovery clear, game stopped. Existing shortcut and Proton prefix
still select the separate installation. Old Steam-managed installation retained.
A second full supported recovery copy is NOT available:74 native files differ,
requiring13,189,462,119 replacement bytes, versus about6.13GB free. No original
files were deleted to make room. Steam's normal update target is separate from
the selected game; no Steam app-manifest workaround or version bypass was used.

Commit e8e71a4 retains the CI-tested launcher as a separate artifact, avoiding
replacement of the hardware-tested runtime. Launcher-only deployment is prepared
with source/hash checks, stopped-process guards and preservation of the old
launcher. CI38850122409 passed all Windows/Linux/macOS, Deck software and publish
gates. Artifact source e8e71a4 is locally verified: launcher SHA-256
997aeeef5aeb9133c983a7827ac726199ad9ea63cf793af843d48255afae4329,
5,313,024 bytes, retained at artifacts/tested-recovery-launcher. The old
launcher5b11e30a remains installed. The paired Deck connection
then timed out before any deployment. One existing configured wake was sent;
the subsequent strict-key address check still timed out and the original paired
hostname did not resolve. Local neighbor cache still maps the address to the
paired MAC; final post-CI check returned No route to host. No deployment was
attempted while disconnected. Reconnection and actual launcher installation
remain pending. Current
account-name preference is empty; persistent Steam authentication is UNVERIFIED.
Do not promise that Steam will never require renewed authorization.

Next: finish verified launcher-only installation and confirm Ready to skate;
restore the existing Stadium fixture and run the prepared seven integration
trials through HIL42/maintenance47. Do not repeat accepted grind/HUD tests.
Prepared local guarded actions: artifacts/deploy-recovery-launcher.py and
artifacts/recovered-maintenance.py; both require isolated Python. Deployment
requires successful CI38850122409 metadata plus its source/hash-matched launcher
artifact, stops if either game or launcher is running, and preserves the prior
launcher. Frozen HIL tools are unchanged.

## Supported recovery retry and launcher login fix — 2026-10-15

Started from3c44036. Paired Deck is reachable at the previously verified address
with its unchanged pinned host key. Dismissed a covering EA launch-error dialog;
the separate ReSkate launcher remains available. One supported QR retry reached
the Steam challenge, then failed with InitializeSteam failed/exit1. No successful
authorization or manifest download was observed. Post-retry hashes remain game
91552264 (unsupported), launcher5b11e30a, DLL7a8b254e and stock5512a15e.
Game stopped, launcher running, updates=false, about6.30GB free. No HIL trials,
new binary deployment, credential extraction or version-guard changes.

Source fix ba6f19434f64bdcbba74a872aa1619e6826b5c81: QR now honors the existing
remember preference. A successful,
noncancelled download may retain the account name from DepotDownloader's explicit
QR success hint for its existing remembered-account route. Missing, malformed or
conflicting hints do not select an account; no token/password is read or stored
by the launcher addition. Specific authentication errors now survive the generic
InitializeSteam failure summary. Focused portable C++ regressions and naming/
diff checks pass. Windows CI now builds the launcher and runs its focused output/
login tests alongside existing Arcade tests. Development CI38846120298 and Deck
software CI38846120302 passed, including Windows launcher compilation and focused
login/output tests, Arcade regressions, Linux/macOS lab and packaging gates.
Prior source75645f3 CI38838076079 completed successfully, all jobs.

Reference behavior checked against SteamRE/DepotDownloader tag
DepotDownloader_3.4.0, Program.cs and Steam3Session.cs. Its QR account hint is
conditional on returned guard data: absent hints must remain unresolved. This
source improvement is not proof of persistent authentication on our Deck and
does not remove Steam's initial or renewed account-holder approval requirement.
Installed launcher is unchanged. Next hardware action remains supported-build
recovery/full validation, identity checks and the prepared seven integration
trials; accepted grind/HUD tests and frozen HIL42 tools remain untouched.

## Separate installation and AC sleep — 2026-10-15

User authorized separate installation, launch/HIL path migration, and disabling
automatic sleep on external power. Copied308 regular files/15,218,691,855 bytes
from the Steam-managed installation to /home/deck/Games/ReSkate. Every copied
file was SHA-256 verified, then every original rechecked unchanged. No hard links
or symlinks; original installation retained. Only the copied receipt's log_path
changed to the new logs/ReSkate.log. The remote migration inventory is
~/Games/.ReSkate-migration-20261015.json. About6.39GB free remains.

Updated the existing Steam shortcut Target and Start In through Steam UI.
AppID3763278562 / launchID16163158349561462784 remain unchanged. Read-only
process inspection verifies the running launcher comes from the separate folder
and retains the existing Proton prefix. Launcher5b11e30a opens normally and
offers supported-build25414733 recovery. Actual game remains stopped, executable
91552264 remains unsupported, DLL7a8b254e and stock5512a15e remain exact.
No offset, native fingerprint, DLL or supported-game pin changed. No new gameplay
acceptance. Steam authentication/download/full validation still required.

Steam Power UI: When Plugged In and Idle / Sleep after = Disabled, persisted
IdleSuspendACSeconds=0. Battery sleep remains900 seconds; AC dim900/battery
dim300 unchanged. This is Gaming Mode idle-sleep prevention, not disabling the
manual power button or changing Wi-Fi/network/security settings. Desktop Mode
also has an explicit ~/.config/powerdevilrc [AC][SuspendAndShutdown] preference
AutoSuspendAction=0, verified with kreadconfig6 against the installed6.4.3 schema.
Only that AC key was created; battery and manual-button defaults remain intact.
It applies on the next Desktop session; no mode switch or long idle test was run.
Full-size Gaming Mode UI evidence saved locally.

Standard ~/.local/share/reskate-arcade/config.json now selects the separate copy
(no selector config existed before). Source discovery supports the standard
~/Games/ReSkate location and explicit existing selector; malformed/missing chosen
targets never silently fall back to Steam. Source75645f32e9a955095a1fe321a910f9f636d37fac
pushed.257 Python tests and naming guard pass. Installer CI38838076003 passed;
development CI38838076079 Linux/macOS passed, Windows built and remains in its
test step at this checkpoint. No new runtime from that run was installed.
Separately reviewed HIL42 preserves all HIL41 worker behavior except installation
selection; same source/DLL/stock/game pins and prepared trial plans. Old frozen
tools remain unchanged. New external tool identities:
- trusted-v42/trust.json:2d8d7180b5ce1c9936088edcdff1840b539b6c03e629c5d53e6bbc46c59b7ec0
- maintenance-v47/maintenance.py:6c2254d2c23e5c11dda75884fc6d8c30e9a686efa4fad1545dcbaecc0874a133
- recovery-inspect-v3/inspect.py:669b5eba63282dc9ce8916429340758f7de31c2f4819e435876488f9ac488e1b

Hostname resolution failed during this session. One configured wake request did
not restore mDNS. The single cached IP matching the paired MAC was reachable;
per-action recovery used that address with HostKeyAlias=steamdeck.local and the
unchanged strict pinned key. No network scan or authentication weakening.
Next: authenticate in the separate launcher when the user is available, finish
supported recovery/full validation, verify hashes, restore saved Stadium fixture,
then the unchanged seven integration trials through HIL42/maintenance47. Do not
use old HIL41/maintenance46 for the new path or repeat accepted grind/HUD tests.

## Connected recovery checkpoint — 2026-10-15

Source HEAD81c14b92e72155034aa52188c78afb8715ba80c8. Deck connectivity is
restored. Fresh frozen recovery-inspect-v2 confirms the installed2bdf runtime
and DLL7a8b254e, stock backup5512a15e, updates=false, recovery clear. Game
executable remains unsupported91552264; no gameplay or HIL41 trials run.
The existing pinned launcher5b11e30a was opened through its established Steam
shortcut using a separately reviewed, launcher-only recovery action. Neither
frozen HIL tools nor game-build guards were changed. Launcher2.0.7 explicitly
offers build25414733 and reached its supported Steam QR authentication flow.

The first QR attempt ended with the exact bounded launcher error:
"The Steam download did not finish (exit code 1). Error: InitializeSteam failed".
No authentication completion was observed. Fresh post-failure inspection
confirms all installed hashes above unchanged. The generic launcher failure UI
shows a green build indicator even though the executable is unsupported; that
indicator is not identity evidence. Manifest availability is not yet proven.

WAITING FOR USER STEAM AUTHENTICATION: Retry returned to supported Download;
the sign-in panel was reopened. Select Scan a QR code with the Steam app, scan
using Steam Guard and approve directly. No password requested, read or supplied.
No account/QR values belong in project evidence. The game remains stopped.
Next: finish download, use launcher Verify game files for full validation,
recheck supported executable/steam_api and preserved DLL/stock identities,
restore saved Stadium fixture, then unchanged HIL41 integration seven.
Frozen HIL41 and maintenance46 hashes still match the pins recorded below.
No offline scoring or isolated upstream candidate was deployed.

## Offline engineering checkpoint — 2026-10-15

Scoring implementation:829d8429e980050ea5635da76c92a1c4f8b489bc, promoted
to arcade-mode with this checkpoint after software gates; initial HEADc8d90059.
The temporary scoring branch preserves the same commits without a rebase.
Final software CI38807974657 passed all gates. No Deck connection, wake, recovery,
deployment or new native acceptance occurred. Installed identities below remain
LAST VERIFIED. Frozen HIL41, maintenance46 and prepared plans are unchanged.

Official upstream e88853add8a78fb79ec4dd5c7fe50ee61e7b9598 was39 commits
ahead of Arcade's merge base (Arcade102 ahead); fork main was110 behind.
Normal merge preserved in arcade-stage-offline-integration, isolated worktree
artifacts/upstream-offline; combined checkpoint b1d1ad620d4faca903b44b0433890db82b85ab7d.
Only conflict:overlay_render.cpp; both Arcade balance and upstream drone HUD
paths retained. Initial upstream CI38802299936 passed. Expanded combined
CI38808153342 passed, building the Windows runtime and launcher and running
11 selected tests, including Trainer, park transactions and controller tests. No history rewritten or other worktree overwritten.

Keep upstream isolated: its launcher now rejects Steam-managed directories,
which conflicts with current recovery/HIL paths. Its new native patches and
additional data-build expectations require supported-build verification.
Incoming tracked content and all39 incoming commit messages pass naming checks.
All12 existing executable/depot/cache identity constants remain identical;
no Arcade offsets, fingerprint contract or deployment pins were changed.

Implemented fail-closed ScoreBoundary/ScoreSettlement around the existing model:
independent owner/occurrence/semantic proofs, action-slot deduplication, generation,
freshness and loss checks; complete action coverage; deferred bank; explicit bail
loss; immutable receipts consumed once; fatigue reset through existing model.
Banking also requires fresh complete evidence after model completion. Native
worker diagnostics now mark stale/invalid captures without granting ownership.
The community source does not establish local payload ownership and changes
native multipliers for its own scoring policy; that policy was not copied.
Production monetary admission remains dormant, not fabricated or zero-scored.

Local sanitizer regressions pass:328 new score checks,33636 core-model checks,
existing control/manual/revert/HUD/playback checks,67 native-score checks,
40061 grind-observer checks and251 Python tests. Seven provenance-preserved
hardware trial excerpts replay19 actual score rows; all monetary outcomes stay
UNVERIFIED. Original mechanic PASS/FAIL/UNVERIFIED grades remain separate.
35 explicitly synthetic loss/duplicate/truncation mutations grant no authority.
Details:research/grind-exit-results.json, offline_scoring_sprint_2026_10_15.

Next hardware action remains supported25414733 recovery/full identity validation,
saved Stadium fixture and prepared HIL41 seven integration trials. Do not repeat
completed grind/HUD tests. Then measure the smallest scoring proof gap: local
receiver/payload ownership, canonical occurrence across listeners, raw-point
semantics with native multipliers unchanged, and occurrence-to-action mapping.

## Prior native grind and combo checkpoint — 2026-10-15

Recovery authorized; connection preflight currently BLOCKED before Steam login.
Frozen recovery-inspect-v2 (333e6f1e96b7c2ec53257f4df738409aeef31bd3c3a39c8d4e52412938a826ac)
returns SSH255: steamdeck.local cannot resolve. Existing configured three-packet
wake sent via frozen recovery-wake-v1; subsequent checks still cannot resolve,
and no matching cached neighbor was found. No Deck files/processes changed.
The installed identities below are LAST VERIFIED, not a fresh connected check.
The manifest has NOT been attempted or shown unavailable; no login was requested.

Supported recovery confirmed in Launcher/gui_launcher.cpp and updater.cpp:
app3354750/depot3354751/manifest4621099302092747785, differing-file download,
optional full validation, then both game and steam_api fingerprints. Keep updates
off to preserve verified DLL. README supports an empty folder outside Steam's
managed installation; launcher resolves game paths beside its executable.
However current HIL discovers the Steam library and the existing shortcut targets
that directory. Separate installation is supported by the launcher, but Deck
shortcut/prefix/HIL continuity must be checked before moving anything. No move
or guessed alternate launch path has been made.

Safe offline work: native_revert_link_result now grades HIL41 recorded evidence
against fresh independent labels, sequence identity, one accepted continuation,
observed deadline and continuous active/fatigue state. It rejects input-only,
stale/coalesced/dropped evidence and expired/restarted links.244 Python tests pass
(53 focused batch tests). Replay of the three recorded08d revert/manual trials
correctly gives no continuation credit; no new Deck acceptance is claimed.
Existing checkpoint recorder retains revert_links; summarize-native-links.py
uses the new offline grade. Frozen HIL41 and runtime are unchanged. Resume with
recovery-inspect-v2, supported launcher download/validation, full content and
DLL/stock verification, saved Stadium fixture, then prepared seven integration
trials. Do not rerun completed grind/HUD acceptance.

Installed source:2bdf062c7e392350aa160653b25fbc6317d9409f.
DLL:7a8b254e61cbab32ec113c3d4bf1aef86d5c9fc6b130fcc6bb2da9cbcd73f00e.
All five CI38727430627 gates passed. Frozen native-label-link-deploy-v5
receipt verified unchanged stock/game hashes at installation. Steam then
updated the game to25617022; fresh executable SHA
915522645e3b151a641743f52b6c7437e75b6d1381d8420edaa83493b025c4b2
differs from pinned25414733/fbce74d5. Launch guard rejected relaunch.
BLOCKED — GAME BUILD CHANGED. Both game and launcher are stopped, input
neutral, updates=false (ReSkate setting), recovery clear, stock backup unchanged.
No native gameplay on2bdf has run. Do not repin or launch against25617022.
Read-only frozen game-build-inspect-v1 confirms Steam app3354750 build25617022,
LastUpdated1792065018, UpdateResult0. Evidence:grind-exit-results.json
game_update_blocker_2026_10_15. Upstream main6df4f9a7 still has only20260929.
Existing supported recovery is ReSkateLauncher’s Steam download for build
25414733 (depot3354751, manifest4621099302092747785); user Steam sign-in
may be needed. After restoring the pinned game, recheck identity, relaunch via
maintenance46 and restore saved Stadium rail before any HIL input.
Prepared HIL41 fixture-native-link-integration-7.json, then targeted hold
comparison and fixture-core-mixed-20.json; none executed on the new DLL.
Frozen HIL41 manifest:
cd6585465a4750cf14304162d807b4100b31e7a2e1b192e9191a6599417ebfd2.
maintenance46 SHA350a6258b54e3106d43c74929a0ea47bf87e6bd47d7d7d4612045b35aa077856.
Native gates/labels,750ms landing/1100ms single revert window and once-per-
generation grind binding publication are now installed; Deck acceptance pending.
Earlier7c7/2c3 acceptance below retains its original source scope.

GRABS DEFERRED — RETURN AFTER ROTATION, GRINDING, HUD AND BASIC COMBO INTEGRATION.
Preserve grab implementation/tests/guide and synthetic-input ownership fix.
Limited airtime is unresolved gameplay context, not proof recipes are wrong.

Verified milestones (full sanitized results/hashes in research/grind-exit-results.json):
- Required fixture matrix:50/50 native grinds/exits, complete/neutral/reset,
  no bails.10 native controls,20 Y variations,10 approach variations,10 best
  repeats. Results1792041692611729000/1792042807619960000/
  1792043198772064000/1792043536440628000; research/grind-fixture-results.json.
  Raw enum values available50/50 in that matrix, deliberately unnamed.
- Both actual native HUD orientations captured at1280x800 near all five
  requested positions; visible track/center/limits/danger. Manual1792046565110110000;
  grind1792046847730988000 plus negative-edge retry1792047037488038000.
  Strong-left first attempt exited the rail:failed capture preserved.
  Four correction trials1792044354029696000 move the real model as commanded.
- Native physical failure:20/20 manual-mode and20/20 grind-mode, one-shot native
  impact selector acknowledgement plus independent wipeout/pose6; actual
  Slam/ragdoll, HUD/fatigue clear, neutral/reset.13/13 fresh recovery chains.
  Pilots1792048572454649000/1792048743400184000 and batches
  1792048927704982000/1792050652355691000/1792051238136143000.
  Brief native300 may be missed by slow sampling; its UNVERIFIED grade remains.
- Installed landing repair:20/20 Grind→Ollie Out→Manual→Ollie chains, exact IDs,
  continuous lifecycle/fatigue,449–464ms final grace, all neutral/reset.
  Pilot1792052795903153000 and repeat1792052976503465000. Two trials reproduce
  the initial17ms manual contact/150ms rebound and recover without broken link.
-20/20 Kickflip→Grind→Kickflip Out→Land, exact IDs before/after rail, retained
  fatigue,448–466ms grace, all complete/neutral/reset. Same pilot plus
  repeat1792053383541871000. Basic native exits are REPEATABLE in this fixture.
- Longer Kickflip→Grind→Kickflip Out→Manual→Kickflip pilot1792053729215656000:
  3/3 complete native chains, exact IDs, retained fatigue,450ms grace/reset.
  Repeat1792054829820135000:16/17 regular chains passed;first native landing bail
  also lost16 host flip rows at128-row cap. Aggregate19 successes/20 attempts,
  one UNVERIFIED incomplete chain. Nose variants0/3:one broken link after brief
  opposite-wheel contact, two continuous chains produce native16 instead of2.
  All20 neutral/reset. Do not promote these variants or hide the failed attempt.
  HIL39 RT pilot1792055621478241000:4/4 delivered101/score5/reset, no label edges.
  Single correlation1792055790699018000 independently shows FS Revert Clean5
  at1280x800 (artifacts/rt-label-current.png). Only1 exact side confirmation;
  telemetry label reader remains unresolved. Grabs remain OFF.
  Backside control1792058148684380000 independently shows BS Revert Clean5
  at1280x800 (artifacts/rt-bs-label-current.png), native100/101, sequence5,
  zero lost streams, neutral/reset. Exact FS and BS each DISCOVERED1/1.
  Nose LT rail pilot1792056218140808000:0/3 complete chains;two miss rail and
  one loses manual. Isolated Nose Manual pop1792056450177578000:LT native17
  chains2/2 pass;standard context2/2 gives16 instead of requested2.
  HIL39 repeat1792056683965862000:18/18 longer-hold LT exits produce native17,
  continuous chain/fatigue,448–452ms grace, complete/neutral/reset. Together with
  the pilot,20/20 LT exits across two hold timings. Both longer standard-context
  controls still produce16 rather than2; four isolated standard controls failed
  exact identity overall. No rail engagement was requested in these controls.

Failure envelope preserved:old2c3 build manual chain9/10; failed contact17ms then
150ms rebound canceled hold. Later command timing0/5 missed grace. Installed7c7
holds only the bounded initial landing settle (200ms rebound within250ms),
retaining intentional-trick/menu/bail cancellation. Prior Kickflip repeat6 lost
one native queue row and stopped safely:INCOMPLETE, no success credit.7c7 removes
the consumer-lock discard path. New host overflow above is separate:fixed2048
flip rows cover12s+10s at60Hz; loss still invalidates. HIL39 also stops after safe
reset on required flip/combo evidence loss. HIL38 incorrectly continued after
the first lost flip trace; later trial baselines/results are retained independently. Old source9a96/956 native exit data remain separately recorded;
Ollie repeat6 on956 lacks post-entry ID and is not credited an exact exit.

Outstanding limitations:scored native payloads are captured but owner_verified=false;
no monetary bank/loss claim and no points consumed by ComboSession. Current
runtime starts zero-point lifecycle on native takeoff. Incidental short one-wheel
landings still appear as manual poses; do not infer named tricks or score events
from them. Full multi-rail fatigue chainF, RT revert abuse, scoring and mixed stress
remain open. Existing rotation/manual/chord evidence below stays valid in scope.
Earlier7c7 chain captures lack raw enums;08d now retains both enum bindings
after discovery across native resets. Base50-50 Grind history text is observed
independently; exact enum-to-name and frontside/backside meanings remain unknown.

Installed observer/landing repair:
- Global grind-asset scan progress survives fixture resets with fresh binding
  validation; numeric discovery diagnostics identify scan progress.
- Bounded initial wrong-wheel preparation (50ms wrong contact,250ms to desired
  pose, no false confirmation) targets observed Nose Manual17ms/200ms rebound.
- Numeric label reader source/row/stage/component diagnostics distinguish
  missing history, fallback and split labels without logging arbitrary text.
- Balance samples retain250ms rate bound without lifetime4096-event cutoff.
90 manual,33616 model and40061 grind sanitizer checks;242 Python tests passed
before CI. f836 Windows failure C4459 was fixed in08d2a3a; no failed artifact
was installed. Older981/f836 frozen deployment helpers are superseded.
08d2a3a pilot1792059876270789000:all6 complete/neutral/reset. Three of four
rail attempts enter native grind; Kickflip/grind/Kickflip and one Nose Manual
chain pass. Ollie entry misses; other Nose chain recontacts rail and loses link.
The passing Nose chain includes34ms opposite-wheel contact then118ms air before
desired pose515ms, confirming bounded recovery once; not repeatability acceptance.
Grind discovery progresses0→1→3 over6 passes, then retains both bindings across
resets. Raw enums recover; mappings remain unknown. Native history exposes exact
base strings "Revert" and "50-50 Grind" (numeric diagnostic bits1/8); displayed
FS/BS is separate, explaining missing full-label matches.
Entry controls1792060345476559000:900ms setup2/3 grinds; previously successful
1100ms setup3/3. All complete/neutral/reset, raw enums on5/5 actual grinds.
Observed entries near2.2–2.4s; the prior2250ms exit command can precede contact
or pop too early, leading to rail recontact. Do not change geometry or physics.
Later-approach repeat1792060804904761000:15 complete native grinds/exits,
then trial16 observed a grind but the rejection counter increased1→2.
No stream overflow; neutral/reset passed. HIL correctly stopped with
grind_evidence_incomplete. The final planned trial did not execute; the stop
record is not an extra trial. With the earlier3 controls:18 complete successes
from19 executed attempts, one incomplete, not20/20.
A follow-up source change publishes immutable grind bindings once per generation
instead of taking the foreground guard every25ms indefinitely. It retains all
rejection/drop detection; effect on actual contention still needs Deck evidence.
40061 grind observer sanitizer checks pass.
Later controls1792061323418617000:4/4 complete native chains (two Ollie/manual,
two Kickflip/Nose Manual/LT Nollie Kickflip), one rail entry/exit each.
Repeat1792061493757663000:18/18 complete native grinds/exits,16/18 complete
Nose chains. With two pilot controls:18/20 full Nose chains, not acceptance.
Trial1 links through Nose Manual then bails on final landing;trial5 recontacts
rail, obtains only33ms Nose Manual, and expires before final air. All18
neutral/reset; the independent combo pose6 records the bail even though the
coarser grind rubric missed it. No missing evidence or stream loss.
Established1100ms setup/2650ms exit remains narrower at roughly3.95m/s
than4.52m/s. Geometry and physics are unchanged. Hardware batch complete.

Follow-up source NOT installed:allow exact base labels25(Revert)/26(50-50 Grind),
without inferring side; native101 preserves existing grounded grace without
itself confirming a revert. One fresh unambiguous native revert-label edge can
consume the existing model's single continuation opportunity on observed riding;
repeated/stale/coalesced labels cannot refresh it. Numeric link telemetry added.
32 score-edge sanitizer checks,33623 model checks and243 Python tests pass.
Source follow-up also accepts the fresh native revert-label edge during101:
measured labels in1792059876270789000 precede the return to100. The prior
100-only gate would discard those edges. Owner, score freshness, bail, riding
and label-integrity guards remain tested. The binding-publication change
must be included in the next verified combined install. Actual landing→RT→manual/abuse trials remain required.

Measured native revert follow-up (still installed08d2a3a):
-1792062277478865000:three Ollie controls; early setup cancels acceleration,
  speed remains0. Early RT rejected; later delivered RT has no native101/label.
  Do not claim stationary delivery is a revert. All complete/neutral/reset.
-1792062425409350000:three moving grind exits, all native rail engagement and
  complete/neutral/reset. Control lands then expires450ms later. RT3800 trial
  recontacts rail and cannot request revert. RT3950 trial lands at3781ms,
  accepts RT at3960ms and shows native Revert at4414ms:633ms after landing,
 454ms after input. Actual101 starts before the label, after old grace expired.
Runtime follow-up therefore sets existing model tuning to750ms, allowing the
measured native acknowledgement plus a bounded reaction margin. No event comes
from input intent. Further native controls1792062678538132000 produce3/3 actual
Revert then Manual, all complete/neutral/reset; native manual begins933–984ms
after the label because101 must finish and manual recognition takes time.
The same existing model now has separate bounded revert-window tuning:runtime
landing750ms, one confirmed revert continuation1100ms. Repeated reverts cannot
refresh either window. Both default pure-model windows remain450ms.
33636 model sanitizer checks pass; native full-link acceptance remains pending.
Host grader also recognizes the independent native wipeout edge when brief300
is missed, with fresh baseline/complete-stream guards;243 Python tests pass.
The next combined verified deployment must include this window and transition
gate and separate continuation timing;58d6955/5dd2728 and prior frozen
native-label-link helpers are superseded targets. No intermediate DLL installed.

Native moving RT repeat1792062890271733000:18/18 state101 plus exact base
Revert diagnostic, complete evidence/no stream loss, neutral/reset. Latency
452–500ms after RT. With two matching controls1792059876270789000:20/20
in this moving clear-bit fixture context. This establishes generic native
revert repeatability, not FS/BS side or combo continuation acceptance.
Backside-selector batch1792063351680907000:20/20 native101 plus base Revert,
complete evidence, all neutral/reset. Exact BS display was independently
confirmed earlier; base labels remain side-agnostic. Current bounded hardware
comparison1792063747821039000 stopped safely:three220ms controls yielded
2/3 full chains;3.950m/s control recontacts rail and loses link. First350ms
attempt records native grind/exit but observer integrity rejects evidence.
Four actual attempts, one additional stop record; remaining two unexecuted.
All actual trials neutral/reset. No longer-hold success credit. Resume only
after the already-pending binding-publication fix is installed and revalidated.
Final combined source2bdf062c7e392350aa160653b25fbc6317d9409f,
CI38727430627 pending; frozen native-label-link-deploy-v5 helper
SHA0b630797654d91f9106baa9fa95e59155b4e8596bbb16ce15b2c84a08dd16f3a.
2bdf only renames a shadowed test variable after6a1c1f4; runtime unchanged.
Combined model/playback/action/manual/label tests pass locally with-Wshadow
and address/undefined sanitizers;33636 model checks plus all companion checks.
Final2bdf installed after all five gates passed; older08d evidence retains its source scope.

Fixture recovery:existing Park Editor layout "Arcade Rail Fixture", one native
own_bkrails_generic_flatrailround_00002, scale1, rail position
(-39.4061546,46.00006485,60.6163254), identity quaternion. Native saved start
(-39.4061546,47.00006485,52.6163254); verified remount position
(-39.28947,47.028736,52.84672), heading359.9895, stopped100; spread<0.2mm.
No skater transform/velocity/collision/state writes. See research/grind-fixture.json.
After a necessary verified DLL restart:close hub, open map overlay, then
map-destination dropdown → stadium-one → map-travel (in that order). Confirm
Stadium arrival and CLOSE MAP OVERLAY. Inspect fixture status; load only with
freshly observed editor8/revision8,
adopt, close editor, restore60/Arcade ON/actions OFF. Do not reuse stale8/8 guards.
Best approach:forward300ms,A900,releaseY1300,neutral3000,B3200,neutral5600.
Slow native B1300–1900,A1900–2300 approach gives1.391m/s/2250ms grind and stalls
with heldY; use for range/failure checks. Normal approach enters near5m/s.
Use capture0 for native detection, capture1 for end HUD; capture4 has previously
failed100ms readiness. Do not weaken freshness guards. HIL uses one controller per
batch, native resets,512 bounded pad rows and2s pre-fixture idle; no routine restart.


## Rotation results and verified grind installation — 2026-10-14

Current source/installed commit5ed2d873e3b2434c9999557ec656ad74aa7c43ac.
All five CI38667662491 gates passed; DLL
92bc4e858098b6070c0ba0f68d7831cdc9af0996fd2f37c165ad514b190eba41.
External grind-deploy-v1/deployment-result.json verifies unchanged stock/game
hashes, updates off and recovery clear. Frozen trusted-v26/maintenance-v30.
Game launch requested after the necessary verified DLL restart. Root owns Deck.

Rotation f297 batch1792030218835535000:20/20 executed, neutralized and idle
resets; no evidence gaps. Full-size native screenshots independently show:
180ms shoulder holds4/4 partial BS/FS Ollies;300/600ms holds8/8 BS/FS180;
prewind8/8 BS/FS360. Four right180 landings Clean; other16 Sketchy.
Delivered owned spans36–52ms,164–168ms,416–420ms and864–892ms respectively.
All ended unowned with neutral LS. This establishes functional shoulder rotation
in the stationary Stadium context, not moving/ramp/540 acceptance. Chord/OFF,
menu and bail regression remain outstanding. First capture4 attempt completed
one partial BS Ollie, then stopped before trial2 with input_readiness_lost;
end-only capture retry completed20. Capture load is not a proven root cause.

Next: restore Stadium1 at60Hz, native mount with Arcade OFF, enable Arcade,
targeted board-chord/OFF regression, then actual Y rail contact and A/X exits.
Y board holds/from-grind catalog selection are installed but still unverified
in native rail gameplay. Physical balance failure and combo acceptance remain
incomplete. Grabs remain deferred; retain their source and all prior evidence.

5ed targeted chord regression1792031590223483000:3/3 native100-504-100
round trips,2/2 single-shoulder exclusions,5 neutral/reset passes, no gaps or
rotation ownership. Native Y mount also worked with Arcade OFF before enabling.
Two native approach/brake trials1792031754413996000 and1792032569411147000
passed neutral/reset but remained outside the course behind crowd barriers;
no grind attempt or grind success claimed. Trainer where independently read
(-47.37,47.03,39.00),heading63,speed0. Reviewed maintenance-v31 adds only
fixed native ground reset(-40,55), a nearby candidate inside the course;
visual verification pending. No jump-height, velocity or grind-state forcing.

## Earlier rotation installation checkpoint — 2026-10-14

Source f29721f8ad0b56a4f1dffa1255d0c91fe607c976 installed after all five
CI38665858009 gates passed. DLL
199f9bdd38d080f473d7ee22b460c3451533f3631fd02bba3d503defcb5f4271.
Stock/game hashes unchanged, updates off, recovery clear. Receipt external
rotation-deploy-v1/deployment-result.json. One necessary DLL restart; no stock
restore. Frozen trusted-v25 and maintenance-v29 pin this exact installation.
Game launch requested; restore the Stadium practice baseline and simulation60
before shoulder-rotation-f297.json. Root owns hardware; no active grab work.

Latest pushed source5ed2d873e3b2434c9999557ec656ad74aa7c43ac adds basic Y
and authored rail exits; CI38667662491 pending. This newer grind source is NOT
installed. Rotation native acceptance is first. The native forward/brake pilot
1792028815212335000 reached Stadium center barriers, then stopped cleanly using
2 seconds B: max8.46m/s, final0, neutral/reset PASS. One earlier attempt was
skipped off-board after map travel; no input sent. Use an actual low rail/ramp
for grind acceptance, not a crowd barrier or fabricated engagement.

## Active priorities and hot Deck checkpoint — 2026-10-14

Rotation source f29721f is pushed; CI38665858009 Windows build pending,
Linux/macOS and hosted installer tests pass. All220 Python tests pass locally.
This checkpoint prepares the next milestone: basic Y neutral/nose/tail board
hold across native air-to-rail contact, actual from-grind A/X catalog selection,
and physical-button exclusion for the board chord. No geometry/physics forcing
or named-grind success claims. These local changes are not installed. Finish
rotation hardware acceptance before promoting grinding.33,579 model and48
playback sanitizer checks pass. No new native grind acceptance is claimed.

GRABS DEFERRED — RETURN AFTER ROTATION, GRINDING, HUD AND BASIC COMBO INTEGRATION.

Preserve all grab source, tests, mappings and the a970167 synthetic ownership fix.
Insufficient practical airtime remains an unresolved gameplay-context limitation;
it does not establish that native grab recipes are incorrect. No further grab
batches or physics changes are planned in this milestone.

Source parent f29721f8ad0b56a4f1dffa1255d0c91fe607c976; installed source
f1ce40c22a0a908480a891661b3f1d0bbe19427f, DLL
3bb57725475257d26e1d1912ef7d821b3ea1fbf3bc292446bb6437c56a3b726c.
Game remains hot in Stadium1, Arcade ON, auxiliary actions OFF, simulation60.
Root owns hardware; trusted-v24 and maintenance-v28 pin this installed build.
The 20-trial manual balance batch finished:20 executed,20 neutralized,20 idle
resets, no evidence gaps. All10 Manual and10 Nose Manual trials showed the
matching native pose and directionally correct model correction. This remains
one stationary context, not moving-chain acceptance. Result1792027038994591000.json.
Earlier four sustained manual/nose-manual cases confirmed native poses and live
vertical HUD correction in both directions (result1792026779741841000.json).
Full limit-position and physical balance-failure acceptance remain outstanding.

Next: install the verified rotation build and test continuous LB/RB with the
existing120ms board chord; then basic Y rail engagement,
actual grind/manual HUD and physical failure, then basic combo composition.
The local LB/RB actuator now supplies native LS rotation and crouched prewind,
preserving the120ms board chord and release-to-rearm.33,560 model checks pass.
Raw native method result1792027415513923000.json:6/6 spins independently visible,
BS/FS180 for200/600ms airborne holds and BS/FS360 with prewind;6 neutral/reset
passes, no evidence gaps. Every landing was Sketchy. These are native method
discoveries; shoulder mapping, short-press control and larger spins remain
unverified. No unverified source was installed. Game remains running.
Earlier CI38663884831 passed all five gates; its source is not installed.
All older next-step notes below are historical and superseded by this priority.

## Grab ownership and HIL readiness fixes — 2026-10-14

Latest pushed source1d90357; installed f1 remains hot in Stadium1, Arcade ON,
actions=grabs, simulation60 requested. Hardware root only. Diagnostic fix19776cf
passed all CI gates38661373108; combined preparation fix38662070079 still builds.
Six stationary Classic B trials completed (Melon/Nosegrab/Tailgrab twice),
0 independently verified named grabs,6 neutral/reset passes. Generic infrastructure
stop occurred before a seventh result; game/launcher and installed hashes were
confirmed intact afterward. Preserve trusted-v23/results/1792025749679033000.json.
The runtime retains authored ownership for32 neutral settling ticks, preventing
airborne B handoff. Local fix yields only an already-delivered neutral tail to
B in observed air with experimental grabs enabled; unchanged for ordinary flips.
33,510 model and48 playback sanitizer checks pass. Native retest still required.
HIL now has optional matching-heartbeat speed bounds for setup AND reset, plus
allowlisted stop reasons/trial identity. These local changes are not yet frozen
into the trusted runner. Keep pending changes separate from installed claims.
Next: complete CI for the combined fixes, verified deployment, then RT/grab and
manual-chain retests from the clear Stadium baseline. No city moving acceptance.

# Arcade Mode persistent project state

## Ground-preparation combo regression found — 2026-10-14

Installed f1 remains running, CI for19776cf diagnostic fix is building.
Manual command retry:3/4 executed,1 skipped; one independently visible Manual
and two exact native Ollie1 results,2 reset passes,1 traffic/bail reset failure.
All executed inputs neutralized. No continuous-chain or Nose Manual acceptance.
Evidence and earlier4 skipped cases are in manual-revert-controls.json.
Both Ollies showed native100/mode0 then103/mode0 before takeoff200/mode0;
existing pose mapping invalidated the combo at that legitimate preparation.
The targeted local fix treats those ground preparations as riding while retaining
strict action-start gates.33,504 model sanitizer checks pass; Deck retest pending.
City sidewalk baseline is unsuitable for moving batches facing traffic. Move to
the existing Stadium practice map and establish a clear lane before further trials.

## Verified native runtime installed — 2026-10-14

Source and installed commit `f1ce40c22a0a908480a891661b3f1d0bbe19427f`;
DLL `3bb57725475257d26e1d1912ef7d821b3ea1fbf3bc292446bb6437c56a3b726c`.
Development CI38657376204 passed all five gates; hosted Deck simulations38657376177
also passed. Existing game and stock hashes below remain unchanged, updates off,
recovery clear. Receipt: external native-actions-deploy-v1/deployment-result.json.
Trusted-v23 and maintenance-v28 now pin this installed identity. One necessary
restart for the verified update; no stock restore. Root owns hardware.

Native community method has6 independently visible revert results in8 executed
moving trials: FS, BS, Switch FS and Switch BS labels. Two regular attempts at an
obstacle produced no verified revert. Nollie-clockwise2/2 produced FS and Switch BS;
two counterclockwise cases were skipped when riding context was lost. These are
source-method discovery, not20-trial repeatability or Classic RT acceptance.
Eight subsequent raw grab cases were all skipped: no gameplay claim from them.

Short native B braking does not stop coasting (observed2.38m/s after reset), so a
riding heartbeat must not be called stationary readiness. Walking near stadium
props did not establish a useful baseline. Two rejected Trainer position candidates
were tried; game returned to the clear native stadium entrance. Do not reuse those
coordinates as an accepted practice reset. Full external artifacts are preserved.

Pending local collector changes expose bounded native scoring events/status and
HUD amounts, with owner/event identity explicitly unverified;216 Python tests pass.
First Classic RT block:1/2 new native FS Revert labels (score5);2/2 neutralized
and returned to riding. Second trial retained the prior label, not a second success.
The native score observer bound and captured sequence amount5 plus line start;
local ownership remains unverified. Evidence trusted-v23/results/1792024652125144000.json.
Idle action cadence on two controller slots crowded out input evidence in both
trials. The local fix retains changed values and cadence during actual ownership.
Switch BS Revert exact label alias added from the earlier native screenshot;
21 score-edge sanitizer checks and216 Python tests pass.
Next: manual-to-Ollie trials on installed f1; build the diagnostic fix before
repeatable RT/grab delivery analysis. Keep game hot
between trials and preserve all current result denominators.


## Native revert discovery and HIL timing repair — 2026-10-14

Source checkpoint continues `2ea4337710ea7fb7fa6281f3dcbe4229e6345be8`.
Installed c1e2a79 DLL/hash below remains unchanged; game remains hot in Stadium1,
Arcade Mode OFF, mounted, controller neutral. Native Classic preset independently
confirmed; no native gameplay settings changed. No Comply is disabled; other
recorded settings and evidence are in community-revert-source.json.

The community235ms recipe now produces actual FS Revert and Switch FS Revert
labels on the Deck:2/2 moving trials, both neutralized/reset. This is DISCOVERED,
not20-trial repeatability or RT integration acceptance. Stationary requests did
not establish native reverts. Four native Ollie positive controls passed exactID1.

Measured a runner defect before judging the recipe: full process-list scans on
every5ms angular step stretched235ms to339.427ms. Moved those same read-only
checks to a background observer; stale100ms evidence, changed capture owner,
failed capture or session deadline stops input. Existing identity checks,
watchdogs and neutralization remain. Real Deck retest delivered234.985ms with
max0.258ms write lateness; native Ollie still passed.215 Python tests pass.
Frozen external trusted-v22 is current; maintenance-v23 uses reliable console
key timing. Native game polling is a distinct boundary from uinput write timing.

CI restored by the user after account-level blocking. The development run
38656982739 for2ea4337 is active; Linux/macOS gates passed, Windows build pending.
No unverified DLL deployed. Next: stance/direction revert trials, then install the
new runtime only after complete build/package gates and test the actual RT path.


## Live Deck resumption and score observer — 2026-10-14

Deck reconnected. Installed the newest fully gate-passed artifact, source
`c1e2a79c51d92f0575232462767d3f299be0492a` (CI `38636563486`), DLL
`7ce21465ecbf83234ecdd2b191d715d20a58a5eae09c77c666092f1324cf2834`.
Stock backup remains
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`;
game remains build25414733, executable
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
Updates remain disabled, recovery is clear. Game is running; root owns hardware.
Frozen external `trusted-v18` uses one controller per batch, exact current pins,
and the locally validated208-test persistent runner. `maintenance-v20` supplies
fixed reviewed console operations. Deployment receipt is in external
`plumbing-deploy-v1/deployment-result.json`.

Console independently reported forced simulation30 and time scale1. Changed
forced simulation to60 to match upstream recorder, with console confirmation.
Current controlled baseline is mounted, stationary on the Blue Light Beach spawn
sidewalk, physical LS/triggers neutral. Both eight-trial A/B blocks completed: delivery5/8 exact IDs; reference5/8
exact IDs, including one inconclusive final off-board/reset failure with tail
overflow. First15 trials have complete timing and no dropped rows; matching
relative tick/XY trajectories exclude schedule stretching in this baseline.
No playback default promoted. See research/playback-parity.json.

Loaded Stadium1 for clear follow-up. The map transition reset forced simulation
from60 to30, independently confirmed in console; reapply60 before comparable
trials. External trusted-v19 adds the reviewed optional end-only screenshot and
256KiB bounded evidence window (211 offline Python tests pass). Identity checks,
neutralization and timeouts remain unchanged. Preserve the running game/DLL.

The pinned GPL-3 native line/sequence observer is now implemented locally with
exact graph/schema validation, bounded resumable worker discovery, nonblocking
fixed queue and background sanitized logging. All66 sanitizer checks pass,
including40,000 concurrent publications. Capture serials are not native event
IDs; two line-end routes can duplicate events, sequence category and ownership
remain unverified. No score writes or combo decisions are added.

Stadium reference follow-up:6/8 exact IDs, both Impossibles selected4; all8
neutralized/reset with complete timing. End-only screenshots reduced artifact
size to25MB. The24-trial reference repeat block completed:Kickflip8/8,Hardflip7/8,
Varial Kickflip5/8 exact; all24 reset with complete timing. Combined Stadium
reference denominators are10/10,9/10,7/10 respectively, and Impossible0/2. Upstream recorder accepts onlyIDs1..32, so its27/27 evidence cannot
cover Impossible36. Read-only grind asset/page plumbing also passes45 sanitizer
checks; runtime discovery/wiring and named enum mappings remain pending.

Newer runtime code, including grab/revert/manual plumbing and the score observer,
is not installed. Actions remains externally disabled, so fresh Windows/package
verification is blocked; no settings were changed and no unverified DLL deployed.

## Full unfinished-requirement source review — 2026-10-14

Continues source `2f941376a3ec9f3972e2095fb5d8171022ee0a3a`. Expanded the existing
PR/fork survey to standalone repositories, upstream issue attachments and official
native documentation. Every63 action row now has a source/fallback review; the same
native-method matrix covers40 broader requirements, including all eight chains.
This checkpoint changes documentation and source classification, not runtime code.

Strongest new reuse path: GPL-3 native line/sequence event capture from
`SmokeyDawg58/reskate-custom-mods@de837826417b55e2a5756c303f4805ae6a39ae89`.
Its validation records actual native22-point bank, rejected840-point bail and11
sequence amounts totaling18,518. This is external Windows evidence on the pinned
game build, not our Deck acceptance. Extract the read-only expression observer;
omit multiplier writes and its different bail policy. Local attribution, event
deduplication and Arcade lifecycle integration remain required.

GPL-3 `GoalTrains/reskatedualstick@1181aa819bea2eb49042b3f67e6ff5c7c9c4e348`
adds concrete fallback recipes for an LS-only revert, spin wind-up and rail input
continuity. The existing235ms revert remains first to test. Official native
Darkslide input and held-flip/composite-record data reduce the need for new research.
No reviewed source supplies the complete exact named Y table or four missing grabs.
The additional source attachment has mostly offline evidence; an apparent second
translator port is empty. All distinctions and license limits are in the existing
survey. Missing methods remain unresolved, not falsely declared unsupported.

Fresh development workflow dispatch for2f94137 was rejected by GitHub because
Actions is disabled for the repository. No settings were changed. The earlier
Windows test-warning fix has local verification only; a fresh Windows/package gate
is still required before deployment. Deck reachability and recorded installation
remain unchanged: source6a8db43, DLL
`8bcf84f8affa3961d9422e4958babd93ae88e600e91b01aca956a154dad353b5`.
No controller session, deployment, restart or restore occurred. Current live game
state is unknown; the last installation receipt reported game/launcher stopped.

Next hardware task is still controlled flip A/B after fresh identity/build checks.
Next source integration is the bounded read-only native score-event path, followed
by native grind observation/bail requests and the pinned control recipes. HUD,
physical balance failure, real combo/fatigue chains and mixed stress are unaccepted.
No full gameplay completion is claimed.

## Current source reuse checkpoint — 2026-10-14

Continues `55a6303f062c2832cdc93409970e2d37d99b29a9`. Surveyed 20 open PRs,
103 closed PRs and the 180 returned public forks; inventoried 362 branches across
179 external forks, deduplicated 264 heads and screened their differences. Relevant
implementations and licenses are pinned in `research/upstream-method-survey.json`.
This is targeted source review, not line-by-line review of every fork.

PR #217 supplied native line-score/multiplier mappings. The existing observer now
reads them with independent validity, strict full-consumption parsing and bounded
sanitized telemetry; no raw UI strings or fabricated scores. These are presentation
snapshots, not bank/trick events, and local ownership remains unverified. Forty new
C++ parser checks pass with sanitizers; all 208 Python tests pass.

Other concrete reuse paths: Trainer state-page readers for native grind enums;
bounded native bail requests and actual ragdoll-entry observation; existing authored
from-grind playback plus the already-integrated grind exit hook. Exact named Y
selection, enum mapping and native acceptance remain open. PR #232's actual current
diff only affects PlayStation HID; it does not solve Deck grind selection. PR #230
is unchanged, so prepared flip A/B remains the first hardware task.

Baseline Windows run `38639254535` failed on a narrowing warning in a manual test;
the test now uses an explicitly typed button array and passes strict local checks.
A fresh Windows/package gate is required for this change. No new DLL was deployed,
no Deck trials occurred, and the recorded installation/network blocker below is
unchanged. No frozen runner, security settings or native gameplay tuning changed.

## Current runtime and HIL integration checkpoint — 2026-10-14

Runtime source is `53b13bb519858e07087d70f5f4c3c1e4d9a57ca8`, continuing
`c1e2a79c51d92f0575232462767d3f299be0492a`. The formerly preserved
manual implementation is now reviewed and integrated, including fixes preventing
balance corrections from becoming fresh manual commands and cancelling on native
takeoff/bail/grind/opposite manual or conflicting controls.

The unsaved `arcadeactions off|observe|grabs|revert-fs|revert-bs` selector
requires Arcade Mode OFF to change and defaults to `off`. Candidate B grabs and
RT reverts reach the actual remap through explicit channel ownership and packet
progression; A/X/manual/board owners take precedence. Only the inspected clear-bit
stance is admitted. A separate native-state freshness gate preserves revert
playback through crouch state 103. Native recognition is not inferred from requests.
Delivered action axes, triggers, packets and request/result fields enter the
existing bounded evidence queue and sanitized lab collector.

The score observer tracks fresh allowlisted label appearances continuously,
separate from its capped schema diagnostics. Baselines, gaps, coalesced changes
and persistent text cannot create repeated trick events. These are collected as
label evidence only; combo continuation remains disconnected until HUD slot roles
and real native revert outcomes are verified. No score amounts are invented.

The repository's HIL batch path now reuses one virtual controller, kernel/SDL
observers and capture preflight. Trials retain identity checks, fresh idle
baselines, independent evidence, neutralization and bounded reset checks. Ordinary
recognition/reset failures remain recorded results and do not abort the batch;
integrity/transport failures close the session. Each replay retains its 25-second
guard, with at most 24 trials and an absolute 1,100-second device lifetime under
the existing 1,200-second transport timeout. Single-run behavior stays bounded
to 25 seconds. The frozen external runner has not been changed: a reviewed bundle
refresh and live verification are still required before using this improvement.

The trusted read-only Deck status retry still failed. A paired no-op SSH check
confirmed `steamdeck.local` still cannot resolve. No input, deployment,
restart or restore occurred. Recorded installed source/DLL remain those below.
This runtime source's hosted simulated Deck suite passed; its Windows development
build was still running when inspected. Flip A/B remains the next hardware task,
followed by native action candidates and manual transition checks. External runner
updates have not been installed. Persistent stream transport, native grind/spin
integration, physical balance failure and final chain/stress acceptance remain open.

Local validation: 41 runtime action checks, 75 manual checks and 19 label-occurrence
checks pass with sanitizers, alongside the prior C++ suites and 33,500 model checks.
All 206 Python tests pass, including action/score-edge parsing, serialization,
persistent-session cleanup and the actual trusted batch entry point. Delivered
request evidence cannot satisfy native trick criteria. The naming guard passes.

## Current plumbing checkpoint — 2026-10-14 evening

Continues source `3a3baebeae7a3f97105f2f1ace32ee0e492b2239`; the recorded
installed DLL and hardware blocker below are unchanged. No Deck input or
deployment was performed. Flip A/B remains the first hardware experiment.

At the user's request, offline implementation now also covers:

- Exact B/Y command selection and repeated direction handling, with missing
  geometry/methods reported explicitly.
- Twelve bounded grab candidates from the existing native guide and the
  independently written community revert candidate. A default-disabled dispatcher
  reserves input channels, excludes other owners, and cancels on stale context,
  incompatible native state, conflicts or reversed time. These are not live B/Y/RT
  bindings and cannot prove native success. Grab timings remain experimental.
- Independent native-event/request correlation, with epoch/sequence freshness,
  one-shot observed revert handoff, and reset/bail invalidation. A native occurrence
  adapter is still needed; polling a persistent selected ID is not a new event.
- A larger model-driven HUD: horizontal grind and vertical manual tracks span
  30% of the respective display dimension, with warning/severe zones and stronger
  cursor, center and boundary marks. Gamescope screenshots remain unverified.
- An offline A–H chain evidence grader requiring independent native events,
  coherent combo/fatigue/banking state, recovery and repeated contexts. Its live
  semantic telemetry adapter is pending; synthetic grader fixtures are not Deck
  trials. Missing evidence cannot pass. See `lab/chain_report.py`.

The existing manual runtime changes remain in the working tree, preserved from
recovery. They pass the combined local model tests but are not promoted to Deck
acceptance. Existing combo dynamics, trusted runner, security configuration and
playback default are unchanged. Native grind actuation, spins, physical balance
failure consequences, score values and persistent stream transport remain open.

Validation: 124 dispatcher/command checks, 192 revert recipe checks, 41 event
handoff checks, 122 HUD geometry checks, 40 playback checks and 33,500 working-tree
model checks pass with address/undefined-behavior sanitizers. HUD syntax passes
against the bundled ImGui. All 191 Python tests pass, including ten chain-grader
contract tests. All new C++ suites are included in the existing hosted
Arcade test target. Native repeatability denominators for the new candidates are
zero; no milestone is marked DISCOVERED or ACCEPTED from these local tests.
The staged source without the preserved manual edits also passes all new suites
and 33,480 existing model checks with sanitizers.

## Current resumption checkpoint — 2026-10-14 afternoon

This section supersedes the historical status below. Security work is complete
and is not an active gameplay milestone. Local and remote `arcade-mode` were
both `6a8db432d3d90f4179f55a50d7d3413eabd59cf8` at recovery. The other existing
worktree remains detached at `42102ae`; no files there were changed. Earlier
uncommitted manual adapter, score-label and grab-guide work is preserved separately.

The latest **recorded installation**, newer than the old handoff, is source
`6a8db432d3d90f4179f55a50d7d3413eabd59cf8`, DLL SHA-256
`8bcf84f8affa3961d9422e4958babd93ae88e600e91b01aca956a154dad353b5`.
The private `arc-observer-deploy-v1/deployment-result.json` reports the game and
launcher stopped at installation, unchanged game/stock hashes, updates off,
active receipt and no recovery pending. This is recorded evidence, not a fresh
claim that the game is running. The external v17 and v3 runner pins still target
`d0c835c`; they must be reconciled against a fresh installation read before input.

**External blocker:** paired SSH cannot resolve `steamdeck.local`. The trusted
status request failed, and two explicit paired connectivity checks failed with
the same resolution error, including after the configured three-packet wake
request. No controller input, game close/restart, deployment, or stock restore
was performed in this continuation. Wake/network reconnection is requested.

The earliest incomplete requirement remains flip repeatability. The latest
recorded d0 regression is 16/24 exact native IDs, including eight failures; all
24 neutralized and returned to idle. No new Deck trial has been run here.
Authored compilation already matches the pinned Style Editor reference. The
demonstrated difference is delivery-relative rescheduling versus an absolute
game-tick timeline. The reference also increments packets every poll and replaces
the complete scripted pad. This does not yet prove which difference causes the
native misrecognitions.

The current change adds unsaved `arcadeplayback delivery|timeline|reference|off`
diagnostics, selectable only while Arcade Mode is OFF. Ordinary startup behavior
is unchanged. Timeline isolates absolute scheduling; reference also uses per-poll
packet progression and full-pad replacement during authored flips. Freshness,
menu cancellation and bounded expiry remain in force. Per-tick/changed-output
numeric evidence includes cumulative poll count and delivered packets; the
existing bounded collector now parses it. The external runner was not updated.
See `research/playback-parity.json` for the exact comparison and safety differences.

Local validation: 40 focused playback/packet checks and 33,500 working-tree model
checks passed with sanitizers; the isolated commit without the preserved manual
work also passed all 33,480 model checks. All 166 Python tests passed. These are not native
acceptance. Controlled Deck A/B remains required.

Playback implementation `6ec5e58ac99d436b421b2b56a65e60b264cc552f` is pushed.
Hosted run `38630732455` passed all five gates: Windows build/model tests,
Linux, macOS, packaging tests and publication. No new DLL was deployed.
The A/B artifact is cached privately at `artifacts/playback-parity-6ec5e58/`.
Its DLL SHA-256 is
`fafea7d19e8c1d7f8b3829dafaa514f5653fdbaa30306f81fecb8886531383a5`.
The immutable package and raw CI artifact contain byte-identical DLLs; their
source and checksums were verified locally without executing downloaded code.
The offline `lab/playback_report.py` follow-up joins the
sampled cadence/stick evidence and checks packet increments against cumulative
poll counts. Fifteen focused analyzer tests pass, and the full Python suite now
passes 181 tests. Native IDs, timing completeness,
neutralization and reported reset are kept separate; no requested action becomes
a success through this analyzer. Existing results without cadence evidence cannot
pass the new A/B evidence check.

The exact community revert repository and revision are now recovered. The
verified source recipe is 235 ms, including 33 circle samples rather than 32.
There is no compatible source-copying grant, so use the behavioral recipe for an
independent implementation. The native-method matrix now classifies RT as
COMMUNITY METHOD AVAILABLE, without a Deck-success claim. See
`research/community-revert-source.json`; its source uses human verification,
not native observations.

**Resume:** reconnect Deck; read fresh status/trace; reconcile the reviewed runner
pins with the installed artifact; complete the hosted build gate before any new
DLL deployment; restore a safe stationary scene in the same game session where
possible; run paired reference/delivery trials for Kickflip, Hardflip, Varial
Kickflip and Impossible under measured 60 Hz simulation and identical stance.
Use timeline mode only to isolate any reference improvement. Preserve every native
ID and reset outcome. Do not promote a playback fix without real trials. Then
evaluate the verified revert recipe, continue the preserved manual/grab work,
and proceed to grind, HIL throughput, HUD and combo/stress acceptance.

## Security-first sequence — 2026-10-14

Active work follows SECURITY.md, then live Arcade Mode toggle, automatic skating,
board chord, exact controls and native-method integration, revert, persistent
HIL, readable balance HUD, and finally combo acceptance. Earlier combo experiments
below are evidence, not permission to skip these gates.

The real game remains running with the development DLL; no stock restore.
Use the reviewed external lab runner, never persistent approval of checkout code.

Security checkpoint 451a5ce passed all hosted jobs in run 38527591836, including
the isolated publisher. GitHub rulesets are active and the owner push path works.
SSH signatures verify locally; GitHub reports unknown_key until public signing-key
registration. Do not enforce signatures yet. See SECURITY.md for the two account
actions and the access audit.

Current installed source is d0c835c18af48cee21554453a12c41998706dee8, all five
gates passed in run 38590114296. DLL SHA-256:
7ae0c5c0828ca4da34b32d3dbb4699d15e916404d223137439241fc56127149d.
The game executable and stock backup hashes remain unchanged, automatic updates
are off and recovery is clear. Reviewed runner v16 and lifecycle helper v16 use
this exact target. The late-poll correction passed its local regression but the
24-trial Deck matrix still found intermittent diagonal recognition failures and
four Impossible requests that produced shove-its. Every trial neutralized and
returned to idle with no gaps or dropped flip rows. The fix is not sufficient
for repeatable exact recognition. See research/playback-regression-results.json.
Bounded authored-arc and delivered-stick diagnostics are the next investigation.

The preceding 53593c4 build also passed all gates. Its Impossible mapping failed
four native trials: regular requests produced ID 4, nollie requests ID 19;
Kickflip/Heelflip controls passed and all six trials neutralized/returned to idle.
The current-game Skatepedia confirms a full right-stick circle from downward
setup toward the heel side, with no extra button modifier shown. This supports
re-testing the missed-arc correction before changing the native recipe.
See research/impossible-control-results.json. The native guide is accessible by
pausing twice, selecting its third side-menu row, then choosing Flip Tricks;
Impossible is the sixteenth entry. The visible side-menu Get Unstuck action may
provide a native reset later, but has not been tested.

Earlier installed source was f47e6cfd0a8bc5e2f118e09d3cc7f8704910432d, all five
gates passed in run 38579226003. DLL SHA-256:
3dec1862d95b44218435374537a8c8e087aa682f1e3c26b127993be65b056178.
The game and stock backup hashes are unchanged, updates are off, and no recovery
is pending. Reviewed runner v16 and lifecycle helper v13 match this target.
The game remains running in Stadium1 after the necessary DLL restart.

The reflected authored adapter passed all seven mapped primary flips with exact
native IDs: Kickflip 2, Heelflip 3, Pop Shove-It 4, Hardflip 9, Inward Heelflip 6,
Varial Kickflip 5 and Varial Heelflip 8. Left/right D-pad also produced IDs 2/3
without opening the native menu. A hold/release passed 20 controlled trials:
seven 200 ms, seven 500 ms, six 1500 ms holds, all native Ollie ID 1.
Every trial neutralized and returned to idle, with no evidence gaps or dropped
flip rows. This establishes A repeatability in the stationary test context;
flips remain discovered until repetition and practical context variation pass.
See research/reflection-control-results.json. The nollie/repeated-input matrix
produced 17 exact results from 19 mapped requests. Two inward-nollie requests
produced plain Nollie ID 16, but both later succeeded on retry. Five unmapped
exact double/triple variants produced no substitute native flip.

Repetition exposed intermittent diagonal failures: 17/21 primary requests passed;
four produced simpler tricks. D-pad timing variation passed nine initial trials,
then Hardflip produced ID 9 but collided with spawn barriers and bailed (504).
Fourteen subsequent cases skipped without input because the riding baseline was
lost. A map reset is recovering those cases without restarting the process.

A local playback regression explains one possible cause: absolute catch-up could
skip a one-tick authored arc when input polling missed a game tick. The correction
delivers each arc on a distinct observed tick, preserves release spacing and the
absolute expiry bound, and passes 33,480 sanitizer checks plus strict warnings.
The defect is reproduced locally; its role in the Deck failures is not yet proven.
The correction still requires hosted build and Deck deployment. Do not promote flip repeatability.
53593c4672c565c84e459eeae38555be51ecb93b (Impossible and score-label observer)
is pushed and in CI; f47 remains the installed target until all gates pass.

The catalog contains 92 valid patterns. Four rejected assets have an arc minimum
above its maximum (stage 7, field 13, error 2), so the catalog remains partial.
Fixed numeric authored-label tokens establish Impossible IDs 36 (regular) and
38 (nollie); 37/39 identify different foot variants and are not substituted.
The exact Impossible mapping is added locally but has not been deployed or HIL
verified. See research/authored-flip-catalog.json and native-method-matrix.json.
Null matrix classifications mean source review remains incomplete, not that an
exact equivalent is absent or that new reverse engineering is warranted.

On the preceding d799 build, A hold/release worked twice but lateral flips had
opposite native IDs. The f47 correction reflects authored X for the measured
clear-bit stance using the existing guarded stance flag. Other stance contexts
remain unverified. The older 3fa timed adapter produced twelve Kickflips and
twelve Heelflips; those counts do not establish repeatability of the new adapter.
The d799 failures remain in research/authored-control-results.json.

Physical right stick controls the camera when the authored catalog is active,
using the existing guarded native camera lease. On d799, yaw worked both ways;
stronger two-second pitch holds after controller activation moved up/down.
Pause restored the native camera. Movement, collision and repeated restoration
remain open. See research/camera-results.json. A live OFF check on d799 restored
native A and right-stick behavior without restarting; ON was then restored.
The f47 A repeat batch followed a DLL restart, so it is not counted as the same
live-toggle cycle. See research/live-toggle-authored-results.json.

The next change also adds an exact 20-label diagnostic allowlist for native
manual/revert/grab text, with a separate numeric label event. It does not infer
grab success from input or generic numeric score fields. Unknown text is excluded.
Local validation: 162 Python tests and 33,475 C++ sanitizer checks pass.

Previous installed source d93dcef1822d1a0eccb660bb7b9508e86d666059 passed all five hosted
jobs in run 38554517047. DLL SHA-256:
1dda9303d02e018edf67932dc32b5e36e95bff19315fb6bba571fb0938f0df1b.
Game and stock backup hashes remain unchanged; updates are off, no recovery pending.
This adds bounded game-facing pad edge evidence to diagnose the trigger boundary.
External trusted-v5 added numeric pad observation parsing with independently
reviewed installation and exact target pins. The necessary game restart for the
new DLL completed and Stadium1 was loaded, with no stock restore. The pad matrix
showed that requested LT/RT levels never changed the game-facing trigger bytes;
the failed chord arrived as LB+RB with both triggers zero. Treat prior
trigger-dependent HIL checks as unverified. Runner v6's standard Linux controller
version identity did not fix this. Reviewed v8 now observes only the kernel event
device created by its own live uinput descriptor, after hotplug permissions settle.
Numeric trigger samples and bounded failure codes distinguish device transport
from game-facing translation. Runner v10 additionally confirmed correct full-range
trigger values through the system SDL controller API. The standard device name
and v11 wider-range experiment did not fix game-facing translation; the wider
range was removed. No shortcut-specific binding override was found in the bounded
inspection. The next DLL adds numeric source edges before existing ReSkate filters
to locate the remaining boundary. Chord semantics remain unchanged.

Source 4081eccfe0a7c0a347262c49617a88983cb2473f passed all five hosted gates
in run 38562178690. It is not installed; d93dcef remains hot. A single bumper
press/release before later trigger ramps made both game-facing trigger bytes
reach 59/127/255 and return to zero. A shorter lead failed again, so this is not
yet a repeatable readiness guarantee. A later full LT ramp produced native
off-board state before the chord; the adapter emitted no synthetic board pulse.
Clear Stadium1 follow-up passed two partial-trigger smoke cases and all 20 repeated
cases (10 LT, 10 RT): the game received 59, the chord emitted no synthetic Y,
releasing the trigger while holding both bumpers did not trigger a delayed toggle,
and native riding/neutral input remained stable with no evidence gaps or drops.
The fixed bumper activation and 800 ms trigger lead are part of this tested setup;
general hotplug readiness and full-trigger native behavior remain separate open
cases. See activation_followup in board-toggle-results.

Source 81c7f910074ba4b2ec32c03c4e9173fa536cb776 adds the existing upstream
authored flip parser and guarded native FlipTrickType observation, with asynchronous game-data loading and
bounded numeric catalog/identifier telemetry. This prepares exact native mappings;
it does not yet change the timed input adapter or claim full trick support.

Windows CI 38565068892 rejected a shadowed loop variable under warnings-as-errors.
The parser variable names are corrected. No failed artifact was deployed.
The follow-up also prepares bounded authored flick compilation, hold/release tick
playback and exact native-ID/context selection, with 32,487 sanitizer checks and
156 Python checks passing. Playback is not yet connected to the live adapter.
Reviewed external runner v13 adds only bounded numeric catalog/native-ID parsing;
it is installed with the existing d93 target pins pending a verified new artifact.

The real Deck live toggle gate PASSED in one game process: ON A produced native
100 → 103 → 200/201 → 100, OFF A remained 100 and rolled normally, and ON A
again produced the air/landing sequence. All three trials returned to idle with
neutralized input and no dropped physics samples or evidence gaps. The Player
menu shows the setting; controller A toggles it without triggering a trick in
the menu. See research/toggle-live-results.json. This is one acceptance sequence,
not final control/combo acceptance. No restart
occurred between the three settings. Private full-size captures remain ignored
under artifacts/toggle-acceptance. The tested A gesture still uses its existing
timed recipe; hold/release Classic Controls comes at the control-table milestone.

An additional controlled flat batch passed 20/20 A air/landing trials with neutral
input and idle restoration. A city batch first passed once, then hit traffic and
skipped 18 offboard cases; neither failure nor missing context was counted as a
success. See research/toggle-repeat-results.json. The saved ON preference survived
the natural restart for the automatic-skating DLL.

The automatic-skating adapter uses guarded
native horizontal speed, ordinary riding/two-wheel contact eligibility, lower/upper
speed thresholds and bounded native push-button cadence. It never writes velocity.
Pure tests cover cadence, hysteresis, menu/toggle rearm, conflicts and stale state;
runtime speed parsing is numeric and bounded. Basic Deck acceptance passed: forward
intent accelerated from rest to 5.49 m/s, the identical OFF input stayed below
0.00004 m/s, and ON brake/resume accelerated, stopped, accelerated again and stopped.
All three controlled cases neutralized and returned to native riding, with no
gaps/drops. Initial mixed-scene cases encountered obstacles and remain recorded
honestly in research/automatic-skating-results.json. Downhill behavior and varied
context repeatability remain open; these are not final integrated skating chains.

Follow-up manual/nose observations confirmed push exclusion. A short movement
batch using backward-stick stops remained neutralized but did not establish
repeatable acceleration: screenshots show native powerslides changed heading,
eventually pointing into a wall. Native B stops preserved heading in three
subsequent calibration trials, each accelerating then stopping. Use that stop
in bounded HIL while B remains native in this build. Do not count the earlier
20 short trials as 20 acceleration successes. See
research/automatic-skating-followup-results.json.

A later obstructed batch remained neutralized but was excluded from repeatability.
After a native scene reset and full-size baseline capture, 16 consecutive trials
across 300/450/600ms movement holds accelerated near 4 m/s and stopped near zero.
Further travel reached course obstacles: one collision transition, one blocked
case, then a native lip stall (400) and one context skip. These remain separate
from clean flat trials. Native B is only the current lab stop, not the final B
control mapping. The same hot game was used for these batches. Five more clean
600ms repetitions on d93dcef brought the clean flat total to 21: each reached
3.998–4.026 m/s, stopped near zero, remained native riding and neutralized without
evidence gaps/drops. The final full-size capture is near a barrier; reset the scene
before further movement batches. Downhill and full-chain integration remain open.

External trusted-v4 was independently reviewed and installed with the current
verified target pins; it adds only numeric push observation parsing. The private deployment helper
uses encoded transport because plain inline installer text falsely matched its
own process-name check. Actual stopped-process and hash checks remain intact.

The board chord is now the active milestone. The local adapter requests one native
Y pulse for LB+RB within 120 ms, preserves individual/late bumper input, requires
both releases to rearm, and suppresses menus, stale state, air, ragdoll and trick
conflicts. It uses existing guarded offboard flags rather than input intent to
decide eligibility. Hosted gates passed and the first real cycle plus a 20-cycle
batch passed: each showed exactly 100 → 504 → 100 in both heartbeat and physics
observations, with neutralized input and no gaps/drops. Long holds did not
oscillate. The batch varied simultaneous, left-leading and right-leading presses.
Five additional quick-tap cycles passed. Individual bumpers, a late pair, the menu
boundary and two OFF cases passed. However, a partial trigger/chord and a full
trigger held before the chord both unexpectedly dismounted. These are recorded
failures, not successful suppression. The skater was remounted with the fixed
native setup action. Bounded numeric game-facing pad edge evidence is being added
to distinguish requested virtual input from actual received input before the
board-control checkpoint closes. See research/board-toggle-results.json.

The literal target table is in CLASSIC_CONTROLS.md and arcade_trick_table.h.
Unassigned repeats have no fallback, and geometry-dependent grinds retain pair
identity. These definitions are not native support claims. The existing timed
gesture adapter remains installed; authoritative native methods are the next
integration pass after the board boundary checks.

## Combo/balance implementation in progress — 2026-10-13

Keep baseline captures at the normal display size. cat pouch

08ab first runtime batch completed6 candidates: neutral no events, manual/nose
pose4/5 detected, long neutral and LY12000 each failed Arcade combo after6.833s,
wrong-way LY-32768 failed1.634s. Native manual continued until input release;
physical failure remains NOT integrated. HUD render-idle omission fixed locally;
added capped4Hz numeric cursor/velocity/raw-input/correction diagnostics to resolve
ineffective modest input.132 Python tests and30,520 C++ sanitizer checks PASS.
Next commit/CI these fixes before deployment. A two-trial existing-Y dismount/remount
experiment is running against verified08ab; no failure actuation wired yet.
Collector-only parser additions are local tested; input runner unchanged from
CI08ab. Do not claim those new parser additions CI verified until next gate.

FIRST RUNTIME HARDWARE BUG:08ab batch artifacts/trick-batch-20261013T232900-54160
(session9788) logs real manual pose4 and accumulated fatigue, but private trial001/1.png
shows no meter. Found overlay_render.cpp idle guard omitted Arcade HUD pending
state, so draw function was skipped without another overlay notice. Local fix adds
freshness-bounded balance_hud_pending to that guard. Must pass CI before update;
leave08ab installed while remaining bounded correction trials finish.

LATEST REAL INSTALL:08ab7dc5b0a91c3fcb65ae1e1084fefa427c62a5 passed ALL CI
38519111179 gates; deployed atomically after verified game/launcher shutdown.
Destination SHA4eba60e862ce52de28b165d2229b50f5aa4df4103118caf9f9118cab51c3ac9e,
stock backup unchanged, updatesfalse/recoveryfalse. Steam shortcut relaunched.
This is FIRST runtime combo/HUD build; not yet hardware accepted. Next launcher
PLAY, leave hub, ensure on board on safe sidewalk, run balance-runtime-08ab7dc.json.
Includes brief manual/nose and long neutral/correcting/wrong-way holds. Native
physical failure and revert remain unverified; no stock restore.

LATEST: HEAD08ab7dc5b0a91c3fcb65ae1e1084fefa427c62a5 pushed; runtime/HUD CI
38519111179 building, separate Deck38519111183 PASS. DO NOT deploy08ab until full
gate. Installed7e3f4ea5ccc4a4848fc767e938d11c230a36004f after ALL CI38517265406
passed; destination SHA4f484cbea989854543d4351b259dfdbb92f5dc937ab977f088bf7dcef03f6056.
Stock5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039 unchanged,
updatesfalse/recoveryfalse. Existing Steam shortcut launched; next verify launcher
ready, PLAY, leave hub/mount, read scalar probe and reset ground away from traffic.
No stock restore. Runtime balance is committed but unverified on hardware.

Runtime wiring checkpoint (local, NOT deployed): combined native activity+contact
classifier,50ms score read, ComboSession, raw LX/LY correction and original
horizontal grind/vertical manual HUD.131 Python tests and30,520 sanitizer model
checks PASS. Revert NOT wired; native balance failure NOT actuated (combo loss only),
so not integrated/complete. Twenty observer repetitions:19 normal correlations,
last interrupted by truck collision (private trial019/1.png); off-board504 clears
adapter. Results research/manual-score-repeat-results.json. Current real install
still823c3ad. Next: CI runtime build, safe ground reset away from traffic, deploy
only full-green build, validate actual manual HUD/correction/failure/reentry.

Current HEAD13d2f334485c7a3abf0efc4cb287ea4f41818755 pushed; Linux/macOS
PASS, Windows still pending. Installed823c3ad fully CI verified, restarted and
healthy. Six native scoring trials completed at artifacts/trick-batch-20261013T222500-48096;
turn/ollie controls at artifacts/trick-batch-20261013T222836-48224. Score bool7aab675c
and int7f3e74e7 are GENERAL trick activity (also ollie), not manual identity.
158-node schema and sanitized values saved in research. Next small diagnostic
adds read-only kind15/size4 scalar values in existing bounded schema walk; needs
CI before deployment. Runtime combo/HUD remains NOT integrated. Keep dev installed.

823c3ad CI38513539193 ALL FOUR gates PASS. Twenty-trial manual-pop batch
completed candidates_exhausted:20/20 native HUD Manual→Ollie confirmed in reviewed
frame2,11 Clean/7 Sketchy/2 unqualified. Same near-stationary flat sidewalk only;
not full integration. Findings saved research/manual-pop-findings.json. Latest
pure tests30,512 PASS and Python130 PASS. Graceful shutdown completed; read-only
status confirms Skate and launcher both stopped, stock hash unchanged. Verified
823 immutable DLL SHA e3d3b895c824fae315a88ad3f3156607096d97e5770552f4bce9e6352ef12b92.
Atomic deployment completed: destination hash verified, stock backup unchanged,
updatesfalse/recoveryfalse. Existing Steam shortcut relaunch requested. No stock restore. Next collect nested score signals during manual and
negative controls; actual event identity remains unverified.

Current continuation: 130 Python tests PASS, 0 failures after correcting the
batch evaluator: either heartbeat or complete per-trial physics sequence may
prove a transition; a missing sample is UNVERIFIED, never an invented trick
failure. New evaluator remains local pending CI; the active batch uses frozen
823c3ad runner code against verified installed276a396. Twenty-trial manual-pop
repeat batch is artifacts/trick-batch-20261013T220120-45043 (session35745).
First11 completed trials show air→riding and reset PASS; remaining trials running.
Reviewed trial010/2.png explicitly shows Manual→Ollie Clean. Do not equate state
sequence alone with named-trick confirmation. Stock remains untouched; dev stays
installed. Next: complete repetitions, review evidence, wait823 full CI, deploy
nested score observer, correlate actual manual state before runtime/HUD wiring.

Active source823c3adb6255b3879381edb1027f0e39fe68144f pushed; CI38513539193
Linux/macOS PASS, Windows pending; separate Deck38513539191 PASS. Do not deploy
until full gate. Installed276a396 is healthy. During build, manual-pop timing
batch artifacts/trick-batch-20261013T215347-43251 completed3 trials (100/250/400ms
full-down setup after manual,300ms up flick). Physics traces show103→200→201
for ALL THREE; heartbeat missed200 in two and reports FAIL. This is sampling loss,
not evidence the trick failed. Screenshot trial000/2.png explicitly shows Manual
then Boned Ollie Sketchy in native history. Discovery only; no repeatability claim.
Important integration constraint: current A/X profile waits1000ms then crouches
600ms, exceeding450ms link grace. Do not hide that by inventing input-intent links.
Bounded follow-up manual-pop-flick-276a396.json fixes setup100ms and compares
75/150ms flick, twice each; currently running. Existing A mapping unchanged.

276a396 launched successfully; fresh sanitized trace contains28 actual
ScoringHUDViewModel fields (research/score-schema-276a396.json). This proves the
read-only UI path works. Observed boolean kind10/size1, integers kind8/size4;
text-bearing fields are nested structures, plus two arrays. Flat probe therefore
does not yet identify native manuals/reverts. Follow-up locally adds a bounded
depth3/256-node walk of this ONE schema, arrays<=8, never object/pointer traversal,
only booleans/int32 and exact four text labels; max4096 value changes per process.
129 Python tests PASS; awaiting next CI before deployment. No feature integration
claim. Current276a396 stays installed. Plan manual-score-276a396 is prepared,
not run yet. Next diagnostic must correlate native scoring values with real
manuals and negative turn controls before the model/HUD is activated.

LATEST DEPLOYMENT VERIFIED:276a39664f7805c1693c7f28f6f314c21bc6d469 passed
ALL CI38510708319 gates and immutable release validation. Graceful Skate exit
confirmed both game/launcher stopped. Existing installer updated development DLL
atomically and verified SHA3683ab2c2e7031331b481e46f205ef865dadf5dd6252f6531c11a479d35c571a.
Stock5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039 unchanged;
updatesfalse, recoveryfalse. Steam shortcut relaunch initiated. Keep this build
installed. Next: launcher PLAY, collect score schema via controller trace, then
known manual holds on this new pinned source/hash. Older35d933e plans must be
repinned explicitly; their integrity refusal against this install is correct.

Current in-flight checkpoint276a39664f7805c1693c7f28f6f314c21bc6d469:
CI38510708319 Linux/macOS PASS, Windows building (started04:18UTC);
Deck installer38510708174 PASS. Must wait full build/release before DLL update.
Moving revert batch: first candidate hit a car; three remaining candidates
correctly skipped off-board state. Not a revert failure conclusion. Existing Y
recipe recovered upright. Used existing ReSkate trainer console (grave key) to
read position, then validated `trainer ground 630 1069` placed skater on sidewalk.
This is a reproducible native ground-surface reset, no marker/settings overwritten.
Live Skate X11 window56623105 title `skate.` on DISPLAY=:1 (requery on relaunch).
Console input at400,640; grave closes; console filter currently `heading`.
No game DLL changed yet;35d933e remains installed. Next after CI: graceful close,
verified276a396 deploy, relaunch, native score schema/label trace during manuals.
Contact-control durations: false manual-like pattern up to433ms during turns;
500ms debounce would exceed450ms link grace, so it is not an acceptable shortcut.

LATEST: febdb40 CI38508894785 ALL FOUR gates PASS.61cdf4d runner Linux/macOS
and separate Deck installer PASS; Windows pending last check. Continued real
testing completed16 additional trials across three batches (all bounded and reset
PASS), summarized in research/manual-revert-controls.json. Crucial negative
result: LEFT-stick arcs also produce0x4000 contact flags during ordinary turns;
do NOT wire contact bits alone as manual events. Both actual manual types remain
confirmed by native HUD labels, not negated by this observer limitation.
Next implementation checkpoint adds a strictly read-only ScoringHUDViewModel
probe using existing validated native UI APIs/model lock: bounded field metadata,
boolean values and four exact allowed labels only, no pointers/arbitrary strings.
No gameplay writes. It needs Windows CI then verified deployment before use.
Pure ComboSession adapter added/tested: semantic transition edges, real landing
grace/revert consumption, failure latch preventing per-frame combo restart.
30,504 C++ sanitizer checks and128 Python tests PASS. Runtime/HUD integration
still incomplete; next real experiment must identify actual scoring/manual event,
then wire that verified observation. Current Deck still35d933e; no restore.

ACTIVE follow-up: febdb40 pushed; CI38508894785 Linux/macOS PASS, Windows
building; separate Deck38508894846 PASS. New real six-trial batch completed:
artifacts/trick-batch-20261013T210055-33354 (two neutral, two tail, two nose).
All six neutralized/reset PASS; unchanged35d933e DLL and preserved stock verified
per trial. Reviewed trial003/1.png native "Manual" and trial005/2.png native
"Nose Manual" labels. Request word1 mask0xc000: riding0xc000, sustained tail
0x4000, nose0x8000; air200/201 has0. Candidate wheel-contact interpretation,
NOT yet authoritative event mapping. Transient crouch103 also has0x4000, so
mask alone is unsafe. Next: negative low-stick controls and repeated holds,
then validate a state-gated, time-qualified observer before runtime activation.
Quiet baseline revealed missing timestamp watermark after long unchanged state;
local runner fix uses log byte boundaries and marks truncation. Also freeze
remote source at process startup so edits cannot change later batch candidates.
126 offline Python tests PASS. These follow-up changes await CI before real use.
No deployment, restore, or game-file mutation in this follow-up.

Capture correction checkpoint: per-trial batch evidence now includes bounded
numeric physics samples, delimited by a pre-input timestamp watermark. This
does not label manuals from input or selector100.124 offline Python tests pass
(two new capture regressions); diff check passes. Saved sanitized discoveries in
research/manual-observations-35d933e.json. New runner changes must pass CI before
real use. Next: repeat known +/-20000 holds with per-trial native samples and
correlate actual manual/nose-manual evidence; both already happened on Deck.

LATEST HARDWARE CHECKPOINT: CI38504144171 passed ALL four gates for35d933e
(Windows/model, Linux, macOS, tested release); separate Deck CI38504144182 passed.
Explicit standing-authorized development update completed to
35d933e919c405132e8fa948f30de2eadb2511d9, installed DLL SHA256
9a79deffb6c6104c131f20e15666475a29def3b32d6ba27ef2905b688a3bb17a.
Stock backup remains5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039;
updatesfalse, recoveryfalse. Game and launcher were confirmed stopped before
installer deployment; destination verified afterward. Relauch through existing
Steam shortcut initiated. DO NOT restore stock between experiments.
Next: enter Skate, verify fresh heartbeat/probe; run six manual candidates from
COMBO_EVENTS.md and inspect numeric traces plus private screenshots. Runtime
model/HUD/native failure are still NOT integrated. Local uncommitted450ms float
boundary fix passes30,496 sanitizer checks but is not part of deployed35d933e.
Diagnostic launch succeeded; fresh source-bound physics traces received, zero
queue drops. On-foot504/mode0 matched screenshot; existing Y recipe mounted the
board. First six-candidate manual matrix completed with neutralization and idle
return PASS for all six, but every heartbeat remained100/mode1. No manual native
identity established. This does NOT mean no manual occurred: user confirmed
trial3 tail manual (+20000 Linux RY) and trial6 nose manual (-20000 RY).
Trial002/1.png independently shows Skate's native "Manual" label. Both are
DISCOVERED on real hardware, not yet repeatability/integration validated.
The coarse100/mode1 detector missed the distinction; fix observation rather
than reject working gestures. Private batch artifacts/trick-batch-20261013T203914-30124;
tracked plan research/manual-discovery-35d933e.json; sanitized trace private
artifacts/combo-manual-neutral-trace.json. A positive control subsequently
produced native "Ollie Sketchy" and100/103/200/201 transitions. Next correlate
the known-working +/-20000 holds with a minimal actual manual-state observable.
Do not require a new selector ID or infer actual state from stick intent.
No rolling-speed variant ran. Keep35d933e installed and stock backup untouched.

Starting HEAD 0979224 on arcade-mode, origin notPho-Real/ReSkate; clean
worktree before this task. Preserve A/X gestures and the active development
installation. No stock restore between experiments. New scope is combo links,
native revert/manual observation, persistent grind/manual fatigue, inertial
balance and its HUD. Pure model is being extended first; runtime integration
and hardware acceptance are NOT complete. Existing model's informational grace
and linear cursor drift do not meet this task's specification.
Phase1 local result: 483 C++ checks passed with strict warnings,119 Python
tests passed, git diff --check passed. Implemented automatic grace banking,
single-use revert window, persistent normalized mode/global fatigue, seeded
240Hz inertial dynamics and tunable difficulty/edges. Runtime still does not
drive the model. Added bounded read-only physics-step transition instrumentation
to identify manual/revert; details in research/COMBO_EVENTS.md. Windows compile
and release CI are the next gate before deploying this observer.
Real Deck preflight succeeded; installed f669926/hash c67bb9dd... and preserved
stock5512a15e... match prior evidence. Game/launcher running, updatesfalse.
Private screenshot artifacts/combo-start shows Hub menu. Existing tested
exit-hub robot recipe ran and neutralized; resulting context not yet reviewed.
No deployment or stock restore occurred.
Checkpoint8cd48cc pushed; CI38501933776 Windows building, Linux/macOS green;
separate Deck CI38501933781 green. Exit-hub screenshot verified on-board skating.
Follow-up local work: balance entry now adds link fatigue once (continuous entry
still idempotent);27 deterministic envelope runs saved,483 C++ checks and122
Python tests pass (three new strict numeric combo-trace sanitizer tests).
Python compileall, all shell syntax and diff checks passed. Manual/revert mapping,
HUD/runtime driving and native physical failure remain unverified/incomplete.
Second pushed checkpoint97f071f: CI38502612665 pending Windows; Deck installer
CI38502612733 passed. Additional local tests now pass30,493 checks with address
and undefined-behavior sanitizers, including10,000 seeded varied event sequences.
Prepared raw controller axes on the existing overlay-bypass sample and pure
balance correction/meter geometry (horizontal grind, vertical up-positive manual).
These are not connected to gameplay or a rendered HUD yet. No input filter slot
changed. Waiting for first diagnostic CI before native manual candidate batch;
do not deploy an unverified local DLL or call partial geometry an integrated HUD.
Checkpoint31dedcc preserves raw axes/helper plus30,493 sanitizer checks.
First Windows run38501933776 FAILED: MSVC C4456 /WX, observer sample count
shadowed existing button count. Renamed to sample_count. Neither8cd48cc nor
97f071f/31dedcc is deployable until a fixed source passes every CI gate.
No diagnostic DLL reached the Deck; it still runs f669926. Latest real evidence
is successful exit-hub recipe and on-board screenshot, not combo acceptance.
Verified repository observations: air 200–299, wipeout300, upstream measured
grind400–499 (especially401). Manual and revert identities are unverified;
button intent must not substitute for those events. Exact next action: implement
and test deterministic lifecycle/dynamics, then inspect minimal native event
instrumentation. No real Deck changes in this task so far.

## Wake helper — 2026-10-13

User explicitly approved narrow administrator changes to enable Wi-Fi wake.
sudo -n confirmed a password is required. Created lab/enable_wifi_wake.py and
Enable WiFi Wake.command, syntax-checked and opened in Mac Terminal for direct
remote sudo password entry (never captured/stored). The script requires the
inspected rtw88_8822ce baseline (profile0x1, driver disabled, device disabled),
sets active profile magic0x8, enables iw magic-packet, enables device wake,
verifies all three and attempts complete rollback on failure. No automatic
suspend/reconnect, daemon, read-only-mode change or game-file mutation.
User completed terminal authentication. Read-only verification CONFIRMED:
iw magic-packet enabled, device power/wakeup enabled, profile wake-on-wlan
0x8. Configuration is enabled; actual wake and reboot persistence UNVERIFIED.
Baseline /sys/power/suspend_stats/success=7. login1 CanSuspend returns challenge,
so user was asked to press power once and confirm Asleep for the packet test.
Do not equate enabled settings with successful wake. No game files changed.

REAL HARDWARE WAKE TEST PASSED once: user confirmed Asleep; wake.py sent
three magic packets; subsequent strictly host-key-verified SSH returned, and
suspend_stats/success advanced from 7 to 8. iw still reports magic-packet
enabled and device power/wakeup still enabled after resume. This verifies one
controlled suspend/wake cycle, not reliability across repeated cycles or
reboot persistence. No game files or SteamOS read-only setting changed.

Follow-up real Deck read-only inspection after user woke it: phy0/wlan0,
rtw88_8822ce driver, kernel 6.18.50-valve2-1-neptune-618-gc7289a96b14d.
`iw phy` explicitly advertises magic-packet WoWLAN support. `iw phy phy0
wowlan show` reports disabled; device power/wakeup also disabled.
NetworkManager reports modifying connection settings requires authentication.
Capability advertisement is not proof of successful suspend/wake. No wake
setting has been changed. Enabling device wake/driver magic-packet handling
crosses the user's no-root/system-change boundary: obtain explicit narrow
authorization before doing so; never store password or disable read-only mode.

User requested a Wake-on-LAN sender while X hardware work was connectivity
blocked. Added lab/wake.py and Wake Steam Deck.command: three 102-byte magic
packets to local subnet UDP port 9; no SSH, root, deployment or Deck settings
changes. MAC/subnet from the user's earlier photo are kept only in ignored
tools/arcade-deck/wake.json (0600), not committed. CLI --mac/--broadcast can
also be supplied; never infer successful wake from sendto success.
119 Python tests passed including five packet/validation/bounds/cleanup tests;
shell syntax passed. Three actual packets sent; subsequent strict-key SSH to
the last known IP timed out. Wake UNVERIFIED, Deck connectivity still blocked.
No new DLL reached Deck. X build run 38489414985 remains Windows-in-progress,
Linux/macOS success as last checked. Resume that work after connectivity and
green CI, without restoring stock. Repo action works with its private config;
if copied elsewhere, supply --mac and --broadcast explicitly.

## X tricks continuation — 2026-10-13

Starting HEAD 5f479ba. Deck connection verified read-only: active f669926,
game/launcher running, expected dev hash and original stock hash intact,
updates=false. Temporary mDNS failure resolved after wake; IP was checked
against the existing paired hostname key, with no new key acceptance.
Upstream fetched read-only at 28118efc2735a71a0c38bda2e7955d35cb2e7d24:
71 commits behind, including relevant grind-exit gamepad filtering. Integrated
on arcade-mode before new trick work, preserving both upstream filter and our
outer-game-read remap. Local 114 Python tests and C++ model pass after merge;
Windows integration still needs CI. No deployment or gameplay input yet.
Merge checkpoint: 25507d6. Shared bounded A/X gesture engine now implemented
locally: X neutral/left = discovered kickflip, X right = discovered heelflip,
direction selected by D-pad before left stick. Preserves measured A timing,
physical right-stick cancellation, no queued/repeated hold, menu release gate.
Only grounded state100/mode1 starts; midair ollie->flip remains unsupported and
must be characterized rather than counted as integrated. C++ tests include 20
repetitions per flip profile in simulation plus exact A boundary equivalence.
X implementation commit 7eb6b3b. Its Windows CI configure failed before compile:
upstream Runtime.cmake globbed an unfetched windows_x86_64_msvc Cargo import
library. Linux/macOS passed. Workflow now fetches locked Windows dependencies
before CMake (preserving upstream features). Next: rerun CI, then deploy/test
and record actual hardware outcomes. Second run 38431924270 passed configure
and built dependencies but MSVC /WX rejected an int-to-uint16 button mask in
arcade_gesture.h. Explicitly narrowed the bounded mask; rerun required. No
development DLL update has occurred. Private game guide captures confirm
diagonal flip directions and describe scoop paths; see research/TRICK_FINDINGS.md.
Other
directional trick names remain selection-only
until native gestures are measured. Do not call the flips repeatable/integrated.

Current X source checkpoint: adbf6ee24918648a97673fceb228b5b20f403191.
Strict local -Wall -Wextra -Wconversion -Werror model build/run passes.
CI 38489414985: Linux/macOS passed, Windows pending at this checkpoint.
Deck later became unreachable at both paired hostname and known IP (bounded
strict-key read-only checks timed out). User wake request is pending. Attempted
graceful close did NOT reach the Deck, and no DLL deployment occurred. Last
observed screen was Hub's menu after leaving Skatepedia; do not send a trick
until on-board skating is freshly verified. No restore is requested or needed.
Resume: wait for this CI, verify immutable release, regain SSH, gracefully close
game/launcher, update active development DLL through controller, relaunch,
run an A/X neutral/X-left/X-right batch before repetitions and chain boundaries.

## Autonomy implementation — 2026-10-12

Starting HEAD for this task: `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`.
Repository remains notPho-Real/ReSkate, branch arcade-mode. Preserve the older
flip selector milestone and uncommitted research recipes; no upstream edits.

Added `.codex/config.toml` (workspace-write, network off, automatic escalation
review, high reasoning) and absolute-path command rules. They require a trusted
project reload and cannot override this session's managed read-only policy.
No global configuration is modified. `inspect_project.py` exposes fixed git,
fork Actions/release and robot/controller read-only commands without forwarded
host/options. `trick_batch.py` accepts only bounded JSON controller data and
pins the target, source hash, DLL hash and original stock hash. See
`lab/TRICK_BATCH.md` and `.codex/README.md` for exact interfaces and limitations.

The batch runner reuses single-run robot cleanup/locking/capture, checks every
candidate, continues ordinary criterion failures, checkpoints private summaries,
and never deploys/restores/publishes. New read-only native heartbeat every 250 ms
provides fresh numeric state/mode for conservative on-board context gating.
No heartbeat means SKIPPED_CONTEXT. Named trick remains UNVERIFIED; sampled
state sequence is not proof of trick identity or integration. Recovery currently
waits boundedly for existing game/HIL readiness; no automatic launcher clicking.

Local verification: 114 Python tests passed (23 new batch tests), C++ pure model
executable passed. Includes fake Deck exact stock roundtrip, existing 21 robot
tests and new failed-candidate continuation/integrity/interrupt/evidence bounds.
Current CLI accepts auto_review/workspace-write and parses the rules; positive
batch rule matched. Python compileall, shell syntax and git diff checks passed;
arbitrary python, ssh and a same-named script outside this checkout matched no
project allow rule. Both workflow YAML files parsed. 15 Python files and 11
shell files passed syntax validation.
No new batch was executed on the real Deck during this task.
Last known installed build remains f669926 and stock backup remains preserved.

Implementation commits: `5c1be16b78bbb856c6d831a7d9d99bf52b2eb32f` and
`e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60`, pushed only to origin arcade-mode,
along with the preserved earlier 8ff309f milestone. Final code CI
https://github.com/notPho-Real/ReSkate/actions/runs/38356744624 passed all four
jobs: Windows build/model, Linux tests, macOS lab and gated release. Separate
installer run 38356744628 passed too. Local suite: 114 passed, zero failures;
pure C++ executable passed. Hosted jobs include the same batch tests.

Immutable release `arcade-build-e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60` and
rolling channel were downloaded and verified with the existing pinned-origin,
commit, archive hash, DLL hash and x64 PE validators. DLL SHA-256:
`c44fd019365bed73ab97e1bb0f9ee4912e743d5a1e5f75bf27c18bcdf0223f45`.
Example JSON now pins this build. This final handoff/data-pin commit does not
change executable code and deliberately skips another identical CI build.

Exact next action: reload this trusted project so Codex can apply local policy
(managed host policy still wins), use the established standing-authorized
deployment procedure to update the active development installation to the
verified e63dbe1 build, launch Skate into on-board skating, then run the command
in lab/TRICK_BATCH.md. No new runner operation or SSH access occurred during
this autonomy implementation; hardware behavior of heartbeat/batching remains
UNVERIFIED. Last known installed DLL remains f669926; do not mistake release
publication for deployment. Keep dev installed. Resume controlled repeatability
and chain tests, not claims of completed flips. Recovery is bounded readiness
waiting, not automatic launcher navigation. Skipped-context trials and named
trick UNVERIFIED must not be promoted to passes.

## Live Deck research session (update immediately after each milestone)

Latest user instructions establish **ACTIVE HARDWARE-IN-THE-LOOP DEVELOPMENT**.
Leave the current tested Arcade development DLL installed across iterations;
the preserved stock backup is emergency/final rollback, not the default between
experiments. The user grants standing authorization for bounded HIL candidates,
repo edits/tests/CI, and atomically updating an active dev installation with
a newly CI-verified development DLL. Restore only on explicit user request,
declared end of development, inability to start the game, failed integrity
requiring recovery, or a concrete safety invariant. Never restore because a
trick probe fails or a handoff is being written. Do not claim any trick
implemented from one HUD observation.

Trick maturity: DISCOVERED (>=1 confirmed), REPEATABLE (>=20 controlled trials
plus varied speed/respawn), INTEGRATED (composes with ollie/landing/grind/manual
contexts, boundaries and physical-input precedence characterized). Current A
ollie is hardware demonstrated twice, Kickflip once, Heelflip at least once;
NONE meets the full new integrated standard. Immediate chain baseline is
ollie -> land; kickflip -> land; heelflip -> land; ollie -> kickflip -> land;
ollie -> heelflip -> land. Grind/manual chains follow once controllable.
Use data-driven bounded candidate tables and machine result files under
`contrib/arcade-mode/research/`. Keep game/dev build hot, neutralize temporary
virtual input after each candidate, and do not publish raw frames/logs.

The user explicitly approved redeploying the previously tested f669926 DLL
for native flip research, then restoring stock. Read-only preflight passed:
both processes stopped, tested release source f669926 and expected DLL hash
`c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7`.
`controller.py deploy` then succeeded and verified this exact installed hash,
preserved original backup hash
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`,
active=true, updates=false. The later standing authorization above supersedes
the original restore-after-research instruction. Keep development installed.
The new X selector code remains only in
local commit `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`, not on the Deck.

Live update: ReSkate launcher READY with updates off; Play launched Skate.
The tested virtual controller exited Hub with B and mounted the board with Y.
On flat ground, right stick down (XInput Y=-32768) for 600 ms then upper-left
(-28000,+28000) for 250 ms produced the in-game HUD **Heelflip Sketchy**.
The mirror upper-right (+28000,+28000) produced **Kickflip Sketchy**. Four
private Gamescope frames per run; the named HUD is visible in frame 2 of each
`artifacts/flip-candidate-left-01` and `...right-01`. These are REAL Deck
observations, but each is one attempt and "Sketchy" means the current timing
may need refinement. The Classic Controls mapping is left X=Kickflip/right
X=Heelflip, so the native right-stick **direction is opposite** the Arcade
direction for these two names. No X runtime build has been deployed.

## Active X flip-trick milestone (2026-10-12 Pacific)

Starting HEAD `970b2d93048d46c41506ed35a8a5df05076aefaf`, branch
`arcade-mode`, clean at start. User requested exact Arcade flip tricks on
X plus left stick or D-pad. The Arcade control table
specifies Xbox X + left/right/up/down = Kickflip/Heelflip/Impossible/Pop
Shove-It, and top-left/top-right/bottom-left/bottom-right = Hardflip/Inward
Heelflip/Varial Kickflip/Varial Heelflip. This is a verified classic input
mapping; the native Skate right-stick sequences for those eight moves are
**not yet verified**. The real Deck was confirmed reachable and STOCK by a
read-only `controller.py status`; game and launcher stopped. Do not deploy a
new DLL without explicit approval for the tested source. Immediate work:
derive bounded native gesture candidates and test with the stock game where
possible, then implement pure direction selection and verified native X
remapping. Preserve A behavior and rerun local/CI gates.

Progress: `arcade_flip.h` now has a pure eight-direction selector for both
XInput D-pad and left stick, with D-pad precedence, a stick deadzone, and
invalid opposing D-pad rejection. C++ model tests pass locally. `lab/robot.py`
has two bounded, explicitly experimental upper-diagonal right-stick recipes,
with fake-device tests and instructions in `lab/ROBOT.md`. Python Deck/Mac
suite passes 91/91 with permitted temporary filesystem access. The initial
read-only sandbox run failed 71 tests because the sandbox disallowed *all*
temporary directories; the exact suite passed unsandboxed. No X runtime hook
or native trick claim yet; no new Deck game mutation. Await stock Skate being
launched before attempting screen-based gesture identification.
Read-only remote robot preflight found the Deck connected with Gamescope and
writable uinput, but `skate_running=false`, so no gesture was sent and no
native trick identity was asserted. The existing ReSkate Steam shortcut was
identified by read-only lookup; shortcuts.vdf was not edited. Python syntax
passes for changed files. These changes are research and pure selection only,
not an X gameplay implementation; do not call this milestone complete.

Stock-game attempt: launched the existing ReSkate Steam shortcut solely for
gesture research; its launcher displayed **"A ReSkate update is ready"** and
offered UPDATE, so no button was selected. Launched Skate's own installed Steam
app separately; private Gamescope evidence showed startup error E111000B:
"Wine, Proton, and Steam Deck are not supported by this application." Skate
never reached gameplay. The launcher window opened by this attempt was closed
via its verified X window ID, without pressing update. Final read-only status
showed game=false, launcher=false, active Arcade=false, updates=true, current
DLL and original backup still exactly
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No candidate flip gesture was sent. Private images are ignored under
`artifacts/flip-*` and must never be published.

Current external blocker for **native trick identification**: stock Skate
cannot start directly under Proton, while the ReSkate launcher currently
requires a pending stock update to expose Play. Do not click UPDATE or deploy
a development DLL implicitly. Next action is to resolve that launcher state
through an explicitly authorized safe operation (or obtain native gesture
evidence through another primary route), then run the candidate recipes on
the actual game, identify HUD results, add only confirmed gesture profiles to
the guarded runtime X remapper, and pass C++/Python/Windows/Linux/macOS gates.

Updated 2026-10-13 UTC (2026-10-12 Pacific). This first section is authoritative.
The historical sections below are retained for provenance and are superseded.

## Current handoff

Repository: notPho-Real/ReSkate, branch `arcade-mode` ONLY. Never change
main/master/upstream, replace Skate.exe, use Deck root/sudo, disable SteamOS
read-only mode, save a Deck password, or edit shortcuts.vdf. Origin is
https://github.com/notPho-Real/ReSkate.git. Starting HEAD for the most recent
A-ollie implementation was 2a410720c61aecc2895f10bbc213bfff8ade16bb.
Feature commit f669926ab790d1328bc234a9828c290fb46ee276 was pushed only
to arcade-mode; a documentation-only final handoff commit may follow. No PR.

The REAL Steam Deck is currently STOCK: ReSkate.dll and preserved first-stock
backup both hash SHA-256
5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Skate and ReSkateLauncher were both stopped after the test. Arcade receipt
active=false, recovery_pending=false, and the original ReSkate updater
preference was restored to true. User explicitly approved deploying and testing
feature f669926, then restoring; the approved session is finished. Another
development DLL deployment requires an explicit user operation/authorization.

## Implemented architecture

Arcade Mode is native in ReSkate.dll because stock ReSkate does not load
arbitrary Plugins/*.dll and custom Lua scripts are currently disabled. Its
startup and client-tick hooks already exist; the overlay controller reader
provides A/B/X/Y edges and the runtime observer. A=ollie, X=flip/trick family,
B=grab, Y=grind/context are intended controls; only A ollie has gameplay
implementation. The existing pure `arcade_model.h` handles combo scoring,
banking, fatigue, balance/grace, and bail reset without affecting game state.
Future multiplayer knockdowns should be host-authoritative events that invoke
native bail, not player rigid-body collisions.

Feature f669926 adds pure `arcade_ollie.h` and hooks the already-present
XInput game-read paths, including Steam's front hook. A triggers a native
right-stick gesture only when guarded client-tick reads show a fresh local
on-board idle state (state 100, rig mode 1), game focus and no interactive
overlay. It suppresses the triggering game A, waits 1 s, holds the right stick
down 600 ms, releases up 150 ms, then returns to neutral. Holding A starts once;
physical right-stick movement or menu capture cancels it. The binary signatures,
owner checks and five-second bounded numeric diagnostic trace remain in
`arcade_native_probe.h` and `arcade_mode.cpp`. This path does not write game
memory or guess a native trick function. The major technical risk remains
commanding Skate's native trick state machine cleanly across contexts.

## Build, release and tests

Feature source f669926 passed local 91/91 Python Deck/Mac simulations, pure C++
model tests, and diff check. GitHub run 38338735186 passed Windows x64 DLL build
and C++ tests, Linux simulations/shell syntax, hosted macOS lab tests/Python
syntax, and gated Deck packaging. Installer workflow 38338735189 passed.
The public immutable `arcade-build-f669926ab790d1328bc234a9828c290fb46ee276`
and rolling tested `arcade-deck-latest` point to source f669926 and DLL SHA-256
c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7
(13049344 bytes). The channel was verified with commit/hash/PE/origin checks.
There is no expiring artifact or Deck GitHub-auth requirement. CI advances the
rolling channel only after Windows, Linux and macOS gates; older late workflows
cannot roll it backward.

Mac lab controller is `contrib/arcade-mode/lab/controller.py`, Deck installer
and updater are `deck_installer.py` and `deck_updater.py`, and autonomous
virtual pad/screenshot runner is `lab/robot.py`. The controller offers
self-test, pair, read-only preflight, explicit deploy, status, collect, restore,
and numeric trace. Pairing uses a dedicated ed25519 key and known_hosts under
the user's HOME; password goes only to the SSH terminal. Host-key checking
stays strict. Preflight performs only reads. Deploy validates pinned release
metadata, refuses running processes, preserves the first stock DLL and previous
updater preference, atomically installs and verifies source/hash. Restore
requires stopped processes and recovers exact stock. Bootstrap runs its tests
before installing the HOME controller and never deploys automatically.
`LAB_PROTOCOL.md` and `lab/ROBOT.md` specify the interfaces and safety rules.

## Real hardware evidence

Diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 established a native
right-stick ollie baseline on this Deck and was restored to stock. That result
is at
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/bbd1def2eaf42945d43f4e0708574852eee3b817.json .
A private Gamescope frame showed Skate's “Ollie”; complete numeric capture
transitioned state/mode 100/1 -> 100/0 -> 103/0 -> 200/2 -> 201/2 -> 100/1.
A prior A-only board baseline was contaminated by an in-game traffic collision,
not an application crash; do not use it as a clean control.

For feature f669926, a fresh read-only preflight passed with the game closed,
stock and backup matching, and sufficient space. After explicit authorization,
`Lab.deploy` installed and verified the feature DLL, retained the backup and
disabled updates. ReSkate launcher showed READY and updates-off; Skate started,
`arcade_mode_initialized` was true, “Arcade Mode ready” logged, and B exited
Hub. Y mounted the skater on the board on a sidewalk. The bounded virtual-pad
`idle` recipe sent ONLY a 100 ms A tap with NO right-stick input. Four private
Gamescope frames showed crouch, Skate's native “Ollie” HUD, and board landing.
Complete native capture id 1 had 21 samples and the same state/mode sequence as
the ordinary right-stick ollie. A continuous 3000 ms A hold generated exactly
one additional complete capture id 2 (21 samples), with the same native
sequence, and no repeated rising edge. Skate did not unexpectedly crash.
A/B/Y observer events registered; a short 120 ms X tap was missed after virtual
pad hotplug, while a later one-second X hold registered. Do NOT claim that short
X tap passed.

Live evidence was saved before graceful game close. After both game processes
stopped, restore verified the exact stock SHA above, active=false and
updates=true. Final sanitized observer report has 13/13 PASS (holds,
native-controls and no-crash include operator observation):
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276.json .
Separate sanitized machine-readable A-specific result, with no full logs,
screenshots, credentials, IPs or personal paths:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276-ollie.json .
Private frames remain ignored under `artifacts/robot-feature-*`; Mac HOME
contains sanitized results at `~/.local/share/reskate-arcade-lab/results`.
No Mac controller bootstrap/pair/config was installed or changed in this run.
The Deck game DLL/updater were modified only for the expressly approved test
and restored; transient virtual input devices and Gamescope temporary frames
were created by the test.

## Exact next action and unknowns

A -> native-quality Skate ollie is VERIFIED for two bounded Deck inputs in this
single session. Next gameplay milestone: X -> one fixed native flip, followed
by grounded/air/grind/manual/landing/bail observation. First review this state
and the published result, then design the fixed X action against native input
evidence; run pure/CI gates before any new explicit deployment. The A mapping
still needs testing with physical controllers, varied terrain, repeated sessions
and online contexts. Short X taps near virtual-controller hotplug are
unreliable in this setup; use a settled pad or longer hold when testing.
No current external blocker. Never claim broader hardware compatibility from
this one session.

## Historical robot implementation progression (superseded by current handoff)
LATEST WORK (2026-10-12): user requested autonomous virtual controller input over
SSH with periodic screenshots. Start HEAD 80ec088f0906482dcff926798825deec5b9bef1d,
clean arcade-mode, origin pinned to notPho-Real/ReSkate. Strict read-only SSH
capability attempts failed: hostname resolution, then last known IP timed out.
No remote command ran. Deck input/capture permissions are UNKNOWN, not denied.
Implementing lab/robot.py: stdlib temporary Linux x86_64 uinput Xbox-style device,
fixed bounded experiments, neutralization/destruction on failure, same-owner
Gamescope SIGUSR2 screenshots via pidfd, private local frames, no auto publication
or deployment. No sudo, permission changes, system installation or game writes.
Upstream Linux uinput headers/docs and Valve Gamescope main.cpp confirm interfaces;
installed SteamOS version/support and Steam player assignment remain UNVERIFIED.
Tests are in progress. Exact next action: finish offline tests, then retry read-only
robot preflight; run a bounded test only when game is intentionally open/visible.
The diagnostic DLL remains undeployed pending the prior explicit deploy question.
UPDATE: after user woke Deck, real read-only SSH succeeded. Linux x86_64,
/dev/uinput character-device write access and Python pidfd support are VERIFIED.
Skate was closed. Display is same-user gamescope-wl, argv0 gamescope, SIGUSR2
handler installed. /proc/PID/exe returns EACCES; do not change permissions to read
it. Identity check uses comm+argv0+UID+caught handler and pidfd for signalling.
Input creation, game delivery and screenshots not yet verified. User asked to
open Skate at a skating scene. Offline 87/87 PASS, 12 Python syntax/11 shell syntax
PASS, C++ model/capture suite PASS; no CI run yet for this work.
REAL HARDWARE UPDATE: temporary neutral virtual gamepad successfully created using
the real Gamepad implementation. /sys/devices/virtual/input matching name was
absent before, present during, absent after (exit 0). No non-neutral input sent,
no game files changed. This proves rootless device creation/cleanup, not Steam
player routing or gameplay. New explicit `robot.py capture --output <new-dir>`
can verify display capture while Skate is closed; it never creates a controller.
REAL SCREENSHOT VERIFIED: this SteamOS emits /tmp/gamescope_<timestamp>.avif,
not PNG. First capture attempts correctly returned no image rather than using
stale files; adapted collector to PNG/AVIF with freshness, ownership, no-symlink,
size, complete-box and stable-metadata checks. Actual AVIF downloaded, hash-checked
and saved under ignored artifacts/robot-screen-01; converted locally with macOS
sips and visually inspected: Steam ReSkateLauncher.exe page, Play button visible.
No game running, so non-neutral input/Steam player routing and periodic in-game
capture still UNVERIFIED. Deck changes: transient neutral input device (removed)
and Gamescope-owned temporary screenshots only. No DLL/config/settings changed.
No Mac HOME installation changed; preview/evidence files stay inside repository
ignored artifacts. Exact next action: once Skate is at a skating scene, run robot
idle experiment, inspect four frames and actual response before any further input.
Latest local validation: 89/89 Python tests PASS (70 existing + 19 robot tests),
12 Python compile checks PASS, 11 shell syntax checks PASS, one C++ model/capture
executable PASS, git diff --check clean. Existing Linux/macOS CI discover the new
tests automatically; bootstrap now includes test_robot.py. CI/channel status will
be recorded after pushing; no workflow definition or runtime DLL code changed.
Implementation commit 367f67da09c686bc5755efdf5644aa552c539c33 pushed only to
origin arcade-mode. Hosted Linux/macOS and installer passed; Windows was building
at last check (development run 38334453685, installer 38334453702). Follow-up
review found screenshot-worker joining could delay neutralization after a replay
error. Fixed by neutralizing before joining and cancelling screenshot waits; two
regression tests added. Latest local suite 91/91 PASS, 12 compile checks PASS.
This supersedes the earlier 89-test count. User has not yet confirmed Skate open;
last real preflight still reports game closed. No non-neutral input has been sent.
SUBSEQUENT AUTONOMOUS STARTUP: under the user's request to perform tests remotely,
captured and inspected a fresh Steam screen showing existing ReSkateLauncher.exe
Play entry selected. Sent exactly one virtual A press (120 ms) through tested
Gamepad API after rechecking no Skate process, then neutralized/destroyed device.
The following screenshot shows a black launch transition. Launcher/game arrival
is not yet verified. No DLL deployment performed. This supersedes the preceding
"no non-neutral input" statement; no gameplay acceptance is inferred from it.
Launcher arrival now visually verified: ReSkate 2.0.7 window with UPDATE button,
not Play. Did not activate update. User EXPLICITLY APPROVED deployment of tested
diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 and continued automated tests,
including closing launcher and relaunching. This supersedes pending approval.
Next: verify stock/hash, close only launcher, pin immutable bbd1def release,
rerun installer preflight/deploy, relaunch, run bounded inputs/screenshots/traces.
DEPLOYMENT VERIFIED: immutable bbd1def manifest/hash/PE validated; two exact
same-user ReSkateLauncher processes asked to close via SIGTERM/pidfd. Fresh
preflight passed with both processes stopped. Shared Lab.deploy installed DLL
082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468, receipt active,
updates=false. Stock backup remains 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Deck now has DIAGNOSTIC development DLL, not stock. Prior stock/updater preference
preserved. Next: inspect Steam screen, relaunch existing shortcut, enter game,
verify runtime and virtual input delivery with bounded captures. No ollie feature
implemented; still observational native-request instrumentation.

NEW ACTIVE TASK: user authorized proceeding with A -> native-quality ollie.
LATEST VERIFIED BUILD: bbd1def2eaf42945d43f4e0708574852eee3b817. CI run
38313781791 PASS for Windows build/model, Linux, macOS, and gated Deck packaging;
installer run 38313781774 PASS. Earlier runtime-only run 38313325123 also PASS.
Public tested release was downloaded/validated in memory: channel commit bbd1def,
DLL SHA-256 082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468,
13045760 bytes. No deployment. Local Python tests 70/70 PASS, C++ model/capture
suite PASS normally and under ASan/UBSan, 10 Python syntax checks PASS, diff check
clean. No workflow definitions changed. Only local ignored research artifacts
were added outside tracked code; real Deck operations in this task were reads.
The user has been asked for explicit diagnostic deployment under the original
deploy-only safety rule. Pending that answer, Deck remains stock. After authorized
deployment: idle five-second A-armed capture, then A-armed ordinary stick ollie,
repeat, `controller.py trace`, and interpret numeric evidence. This is a hardware
input dependency, not completion of A-to-ollie. The unknown native request API
remains the primary blocker; never present this diagnostic as the gameplay feature.

Runtime probe commit: 270a532c01ad39ddfbcfbbe8515f5ed484b032a4, pushed only to
origin arcade-mode. CI 38313325123: Linux/macOS passed; Windows still running
at last check. Installer workflow 38313325113 passed. Follow-up adds read-only
`controller.py trace` collection with numeric allowlist and source/hash/freshness
binding; local suite now 70/70 PASS, including serialized fake-Deck trace and
wrong-DLL refusal. These supersede the earlier 65-test count for this new work.
The first implementation is a focused READ-ONLY native request probe, not an
ollie trigger. See NATIVE_OLLIE.md for exact static evidence and hardware protocol.
Two 32-byte contracts (rig crouch predicate RVA 0x476f130 and producer 0x1009e29)
match the actual binary; runtime disables the new probe on mismatch. A starts a
five-second capture limited to 128 changed samples, no game memory writes or
new hooks. Pure tests cover inactive capture, deduplication, ownership loss,
recovery, time expiry/backwards clock, cap and rearming. Local C++ model/probe
suite PASS with -Wall -Wextra -pedantic; existing Python simulations 65/65 PASS.
Next: Windows/Linux/macOS gates for this build, then explicit diagnostic deployment
and user-performed normal stick ollie to correlate native requests. No safe native
ollie command is established yet. Deck remains stock; never attribute old hardware
acceptance to this new probe build. No new real-target writes in this task.

Active task authorization context: user authorized proceeding after
observer acceptance. Work starts from f814aec2cbdbacfaee2582a5175cc546f8bcd0de,
branch arcade-mode, clean worktree. Native-path investigation is now in scope.
Read-only SSH copied installed Skate.exe to ignored repository artifact
artifacts/arcade-native-research/Skate.exe (144567168 bytes); SHA-256
fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9 matches the
existing 20260929 build contracts exactly. Never commit/publish this binary.
No new DLL deployed; Deck still stock. Existing trainer_jump.cpp changes no-comply
and boneless launch velocities, not an ollie trigger. Existing selector at RVA
0x4745a70 and context request flags are analysis anchors, not an established
ollie command. Next: trace native crouch/pop request producers and consumers in
this verified binary; do not force state IDs or invent a function signature.

Final sanitized report timestamp: 2026-10-12T06:31:01Z; source build
f09872f179434cd3d474c9952b8325363424c308, installation ID
0c790805de1b44d69785aaacac95493e. Report:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
All 13 criteria PASS, observer_acceptance=PASS. Runtime process/initialization/
ready/fresh log and A/B/X/Y events came from actual SSH evidence; normal controls
and no crashes during the session were user-reported. Holds were user-executed
and verified by exact +1 counter deltas on all four buttons. This is bounded
session evidence, not a guarantee against future crashes.

After user closed the game, restore verified both processes stopped, no pending
recovery, receipt inactive, and installed DLL AND preserved backup both equal
original SHA-256 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Updater preference restored to true. The final collector correctly retains prior
runtime evidence for this same installation ID; reskate_process_seen=true in
the combined report describes the tested session, not a currently running game.
No game is running at restoration; the Deck currently has STOCK ReSkate, not
the development DLL. Do not redeploy implicitly. Tested release remains f09872f;
documentation changes do not advance the rolling channel. Report publication
changed arcade-lab-results only; STATE.md handoff belongs on arcade-mode.

No current infrastructure/hardware blocker. Exact next development milestone is
A -> native-quality Skate ollie, but native trick reverse engineering is outside
this completed task and requires a new task instruction. Following milestones:
X -> one fixed native flip, then grounded/air/grind/manual/landing/bail observation.
Historical intermediate notes below preserve provenance and are superseded by
this current handoff wherever they describe incomplete pairing/testing.

## Historical deployment and acceptance progression (completed; see current handoff)
All controlled button holds VERIFIED: after user's A/B/Y test the actual counters
are A=16/B=9/X=8/Y=6, exactly +1 each for A/B/Y with X unchanged. Combined with
the prior controlled X test, all four holds generated one rising edge and no
repeats. User confirms normal controls work and previously no crashes so far.
Published report 2026-10-12T06:29:45Z has 11/13 criteria PASS; only stock_restore
and updater_restore remain UNVERIFIED. Overall acceptance correctly stays
UNVERIFIED until restoration is checked. Same source/installation ID and result
URL as below. Holds are marked operator-reported in schema and additionally
supported by the controlled before/after counts documented here.
Exact next action: user exits Skate AND launcher; verify both stopped, run the
existing restore operation, verify original stock hash and updates=true, then
collect/publish the combined report for this same installation. Do not stop the
user's game remotely or replace any DLL while either process is running.

Controlled X hold VERIFIED on real hardware: user released all buttons, held X
once for three seconds, released, and reported done. Read-only log comparison:
X event count/latest counter rose from 7 to 8; A=15, B=8, Y=5 unchanged. Exactly
one X rising-edge event occurred; no repeated events during this controlled hold.
Overall holds remains UNVERIFIED until equivalent A/B/Y holds are checked.
Next action: from current A=15/B=8/Y=5 baseline, user separately holds each once
for three seconds with full releases; expected counters A=16/B=9/Y=6, X=8.

Latest report 2026-10-12T06:26:53Z: user reports no crashes so far; collector
published no_unexpected_crash=PASS as OPERATOR-REPORTED, bringing total to 9/13
PASS, 4 UNVERIFIED. User says holding three seconds produced no new notification;
this is ambiguous between expected absence of repeats and absence of initial edge,
so holds remains UNVERIFIED. Read-only current-session event counts: A=15, B=8,
X=7, Y=5, each latest counter equal to event count. Counts alone cannot establish
which events correspond to holds. Code emits only rising edges, not at three
seconds. Next controlled check: release all buttons, hold X once for three seconds,
release, and compare X counter against baseline 7 (avoid other X presses).
Native controls and actual stock/updater restore remain unverified.

REAL RUNTIME EVIDENCE obtained 2026-10-12T06:24:45Z after user launched Skate.
Actual SSH collection verifies running Skate, fresh session log, Arcade Mode
initialized active, ready marker, and ALL A/B/X/Y observer events: 8/13 criteria
PASS. Remaining 5 criteria UNVERIFIED: holds without repeated rising edges,
native controls, no unexpected crash over test session, exact stock restore,
updater setting restore. Overall observer acceptance remains UNVERIFIED.
Sanitized report successfully published through the tested collector to
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
Publication changed only arcade-lab-results, without force; no raw logs published.
Local report is under ~/.local/share/reskate-arcade-lab/results/ with the same SHA
filename. Current next action: verify each three-second hold produces one edge,
obtain operator confirmation of normal controls/no crash, then close game and
perform explicitly authorized stock restoration to verify the last two criteria.
Do not redeploy or restore while the game is running. Historical pending-launch
statements below are superseded by this actual runtime result.

USER-AUTHORIZED REAL DEPLOYMENT COMPLETED after the preflight below. The user
explicitly requested updating the Deck to our build and testing it. Actual
`controller.py deploy` exited 0 with `DEPLOY VERIFIED`: installed commit
f09872f179434cd3d474c9952b8325363424c308 and destination DLL hash
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01` match
the verified release. First stock backup hash matches the pre-deploy DLL exactly:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
Receipt exists and active=true, updates=false (previous setting true), no recovery
pending. Both game and launcher stopped at deployment verification. Game files
HAVE NOW BEEN CHANGED by this explicit deployment; prior no-change statements
below describe earlier preflight only. No game launch or runtime acceptance yet.
Exact next action: user launches Skate normally and exercises A/B/X/Y taps and
holds; collect actual fresh runtime evidence over SSH. Do not deploy again just
to collect. Restore and updater restoration remain unverified on hardware.

After the user completed interactive pairing, the actual controller `preflight`
exited 0 and printed `PREFLIGHT PASS — NO GAME FILES CHANGED`. This verifies
noninteractive dedicated-key SSH with strict host checking on the real Deck.
Authenticated user/host: deck/steamdeck. Discovered Steam library:
`/home/deck/.local/share/Steam`; game:
`/home/deck/.local/share/Steam/steamapps/common/Skate`.
Skate.exe, ReSkateLauncher.exe and ReSkate.dll all present; safe paths pass.
Neither Skate nor launcher running; process check complete. Current DLL SHA-256:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No stock backup, Arcade receipt or config exists; no recovery pending; Arcade
inactive and installed commit unknown. Launcher settings readable; updater true.
Free bytes 24475693056, required bytes 220581888. Verified latest tested release
is f09872f179434cd3d474c9952b8325363424c308, expected development DLL SHA-256
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01`.
Pairing necessarily created local SSH identity/trust and authorized the public key
on Deck; this preflight performed only reads. No game files were changed and no
development DLL deployed. Runtime observer, native controls and restore acceptance
remain UNVERIFIED on hardware. Exact next action: obtain an explicit deployment
operation from the user before any game mutation, then intentional launch and
observer validation. Do not repeat pairing or diagnose old reachability failures.

Current tested implementation/release: f09872f179434cd3d474c9952b8325363424c308.
Final HEAD is a subsequent STATE.md-only handoff commit marked [skip ci]; use
`git rev-parse HEAD` for its exact SHA. It does not change the tested code or advance
the release. No essential continuation state depends on the conversation.

## Boundaries and starting state
Only `notPho-Real/ReSkate`, branch `arcade-mode`. Never main/master, upstream,
upstream PRs, root on Deck, SteamOS read-only changes, Skate.exe replacement,
manual shortcuts.vdf edits, password storage, or implicit deployment.
Starting HEAD: f46994894addece6b6c111bae29e474ae9e929f2.
The clean worktree initially used arcade-stage-02-state-probe. arcade-mode was
fast-forwarded from d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc to preserve both newer
commits (b62fe7c isolated stage CI; f469948 read-only native-state correlation).
Current HEAD: use git rev-parse HEAD; this document cannot contain its own commit SHA.
Last known tested release SHA before this run's push:
d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc; Windows/Linux Actions runs
38176087200 and 38176087218 succeeded. Public channel/archive/commit/PE validation
was executed in memory: DLL SHA-256
5a3d6a79fe8e8c2662e570dd5fad68c41f8d2fa551ec9e647d88ed89e1ed05ca,
13,039,616 bytes. This baseline predates the new macOS gate. No hardware evidence.

## Architecture and verified repository facts
Arcade Mode lives natively in ReSkate.dll: stock ReSkate has no arbitrary
Plugins/*.dll loading contract; custom Lua is explicitly disabled by
Extension/Scripting/custom_script_loader.cpp custom_scripts_enabled=false.
Runtime/runtime_init.cpp calls start(base), records arcade_mode_initialized;
Runtime/client_tick.cpp invokes on_client_tick before the 500 ms model throttle.
CMake includes runtime sources and optional standalone arcade_mode model tests.
The observer uses DingoSDKOverlayReadControllerInput, bits A/B/X/Y =
0x1000/0x2000/0x4000/0x8000. Rising edges log and notify individually; holds do
not repeat. Input unavailable clears held mask (refocus can produce a new edge).
Input requires focus and a closed interactive overlay. No suppression or trick
injection. Existing watched_physics_state diagnostics are sampled, not a
transactional gameplay event stream; preserve them without extending research.

Pure arcade_model.h owns combo, saturating points/multiplier, bank/bail,
balance, correction/drift and fatigue persistent throughout an unbanked combo.
Grace is informational pending actual landing events. None drives Skate yet.
Intended controls: A=ollie, X=flip/trick family, B=grab, Y=grind/context.
Future knockdown: host-authoritative ATTACK/KNOCKDOWN validation and native bail;
reject stale/duplicate/unauthorized requests, not rigid-body player collision.
Major risk: commanding Skate's native trick state machine cleanly.
Next gameplay milestone AFTER observer hardware acceptance: A -> native-quality
Skate ollie, then X -> one fixed native flip, then observe grounded/air/grind/
manual/landing/bail. No trick reverse engineering belongs in this lab task.

## Existing build and deployment
Windows CI builds Release x64 ReSkate.dll and pure model tests. Linux tests
package the offline installer. Immutable arcade-build-<40-char-SHA> releases
feed arcade-deck-latest/channel.json; pinned fork, archive SHA, DLL SHA, commit
checks. Serialized publishing plus branch HEAD check prevents late rollback.
Existing deck_installer.py preserves first stock backup and updater preference;
deck_updater.py fetches public releases without Deck GitHub authentication.
Fixed: old deploy_to_deck.py --pair now returns after pairing, never deploys.

## Evidence and acceptance
Actual log code: Engine/Core/Log/paths.cpp chooses game/logs/ReSkate.log, with
LocalAppData/ReSkate/logs fallback; RESKATE_LOG_DIRECTORY can override. Logger
truncates at session start in the default path. Crash marker comes from
Engine/Core/Log/crash.cpp: ReSkate unhandled exception. Do not invent a Proton
prefix path or publish full logs. Runtime/runtime_state.cpp routes record() to
logging::event. Important review correction: Engine/Core/Log/events.cpp formats
events as text (`Arcade mode initialized: active=true`); raw JSON is trace-only.
The original record() default is debug and hidden at the launcher's normal info
level. Runtime/runtime_init.cpp now emits this one event at info, and the collector
matches its actual text (also supports trace JSON). No gameplay behavior changed.
Hardware acceptance requires actual startup, initialized active, ready, each
A/B/X/Y tap observed, no repeat on holds, normal native controls, exact stock
restore and previous updater preference restored. All UNVERIFIED on hardware.
Absence of a crash log alone cannot prove no unexpected crash.

## Lab implementation
lab/controller.py provides self-test, pair, preflight, deploy, status, collect,
restore and test-first bootstrap. Seven .command actions expose setup/operations.
HOME destination is ~/.local/share/reskate-arcade-lab; dedicated SSH key and
known_hosts live there. No keys were created during development. SSH host policy
is ask only during pairing, strict yes thereafter. Password goes only to SSH tty.
lab/probe.py is a separate read-only payload sent into Python -B memory over SSH;
tests inspect the generated code and run it against fake HOME/library fixtures.
Deploy validates pinned release/archive/commit/DLL hashes and x64 PE, preflights
twice and uses the local trusted shared installer, not downloaded Python code.
Shared installer now uses nonblocking flock, exclusive immutable backup, fsynced
rollback journal, atomic writes, destination verification and interrupted recovery.
Legacy shell helpers are migrated to this same lock/transaction through manual_install.py.
Results allowlist booleans/enums/source SHA/DLL hash/time and unique installation ID.
Log baseline/freshness and latest-ready scoping prevent old/combined-session evidence.
Holds/native behavior/no-crash require labeled operator observations, never inference
from absence. Post-restore collection can join only the same installation's evidence.
GitHub Git Data API publishes only sanitized results to orphan arcade-lab-results;
updates are fast-forward only, no force push, no development checkout mutation.
Publication failure leaves local JSON and installation untouched.
LAB_PROTOCOL.md documents all interfaces, operation semantics and recovery details.
arcade-dev.yml now gates deck packaging/publication on build, linux-tests, macos-lab.

## Tests and remaining milestones
Final offline suite after actual log-format correction: 65 tests passed,
0 failures (macOS execution, fake targets).
This includes actual generated SSH payload execution, exact stock restore, AST
preflight non-mutation, failed writes, flock contention, bootstrap failure gating,
stale log refusal, mocked GitHub publication, an actual child-process exit during
transaction, explicit fallback-log baselining, and legacy shell round-trip.
Python compileall: 10 files passed. Shell bash -n: 11 files passed.
Standalone C++20 model/observation executable: 1 passed, 0 failed.
Both workflow YAML files parsed; explicit needs/build/Linux/macOS runner assertions
passed. Git diff whitespace check passed. Report/channel/manifest schema rejection
and allowlist tests passed within the 65-test suite.
Implementation committed and pushed only to origin arcade-mode:
b4fb40f37acc52f99d61da562b8e43b8a63fdf48, "Add tested Arcade Lab controller, safe
deployment and hardware evidence protocol" (22 files). Credential/key/log audit passed.
Hosted run 38199528643 (separate Linux Deck workflow) passed. Run 38199528667
(Windows + Linux + macOS release-gated workflow) passed all four jobs.
Logging correction committed/pushed as e39227940ef9f536b61fd425bea0cd2823bc31a2.
Its run 38199870622 passed, including Windows and the release job.
Separate Deck run 38199870610 passed. Final self-test presentation safeguard
suppresses fixture success messages (so fake LAB READY cannot confuse users) and
refuses incomplete test installations. Committed/pushed as
f09872f179434cd3d474c9952b8325363424c308 (final implementation source).
Final local rerun: 65/65 passed, compileall and diff check passed.
Its gated workflow is [38200147202](https://github.com/notPho-Real/ReSkate/actions/runs/38200147202):
all four jobs passed. Actual job logs confirm Windows x64 build + CTest 1 passed,
0 failed; linux-tests 65 passed, 0 failed; macos-lab 65 passed, 0 failed;
deck packaging/release tests 65 passed, 0 failed; publication step succeeded.
Separate [Linux Deck run 38200147179](https://github.com/notPho-Real/ReSkate/actions/runs/38200147179)
passed another 65 tests, 0 failed. No hardware or user credentials used by CI.
Earlier run 38199528667 completed all four jobs successfully. Its publication log
explicitly said "Newer arcade-mode commit exists; leaving the update channel
unchanged." Public channel remained d83e1e6, verifying late-workflow protection
in a real hosted run (not a simulated Deck result).
b4fb40f/e392279 are superseded implementation checkpoints.

## Verified release and channel
Public channel was downloaded and validated after final CI success:
`arcade-deck-latest/channel.json` points to source
f09872f179434cd3d474c9952b8325363424c308 and immutable release
`arcade-build-f09872f179434cd3d474c9952b8325363424c308`.
Installer archive SHA-256:
9ff9be15724f977ebc7b3e9bd26c09f33e1657c2df516a307296317e4c5103c4.
DLL SHA-256:
d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01.
DLL size: 13,041,152 bytes. Public archive hash, pinned origin/URL, embedded source
commit, DLL hash and x64 PE validation all passed using verified_release().
Bytes were verified in memory only; no DLL was deployed to the user's Mac or Deck.

## SIMULATED/CI VERIFIED versus REAL HARDWARE VERIFIED
SIMULATED/CI VERIFIED: all local/hosted tests above, fake Deck exact stock-byte
round trip, preferences, interruption recovery, non-mutating preflight, fake SSH,
pairing idempotency, release rejection, sanitized collection and GitHub API mocks.
Real GitHub CI/release integrity and late-workflow protection were also verified.
REAL HARDWARE VERIFIED: NONE. No real Deck SSH connection, preflight, pairing,
installation, game launch, controller event collection or restore occurred.
No persistent Mac lab installation or key was created. Outside the repository,
only temporary test fixtures/tool temporary files were used; fake HOME trees were
cleaned up. Repository-local ignored tools contain the standalone model executable.
No keys, credentials or full runtime logs are tracked.

## Historical setup blockers and actions (resolved; see current handoff)
The following records earlier states, not current blockers or instructions to repeat.
Follow-up after the user reported the Deck powered on: attempted read-only TCP/SSH
to deck@steamdeck.local. Both the socket lookup and actual OpenSSH attempt failed
to resolve steamdeck.local (SSH exit 255). No authenticated connection or Deck-side
command occurred, and no Deck files changed. The default Mac lab directory
~/.local/share/reskate-arcade-lab does not exist, so its dedicated SSH key and
known_hosts are also absent. No pairing or bootstrap was run during this attempt.
Immediate next action: obtain the Deck's current Wi-Fi IP and confirm Mac/Deck are
on the same network; retry read-only connectivity to that specific address. Pairing
will still be required before authenticated preflight unless a valid project key
is subsequently provisioned. Do not bypass host-key verification or deploy.
Subsequent user network-settings photo supplied a concrete private IPv4 address.
Targeted TCP and SSH attempts to that address returned "No route to host";
SSH exited 255. The Mac has an address in the same /24, a direct route via en0,
and an ARP neighbor entry whose MAC matches the photographed Deck. Two targeted
pings received no replies; a second SSH-port check also failed. This establishes
an addressing/neighbor match, NOT authenticated SSH or hardware acceptance.
Possible blockers remain Mac local-network permission, network filtering/isolation,
or Deck availability. Do not assert any one cause as proven. No network settings,
host keys, pairing state, or Deck files were changed. Current private IP/MAC are
omitted from tracked state; obtain the latest address from the user if needed.
Router comparison subsequently performed at the user's request: read the actual
default gateway from the Mac route table, then tested router and Deck in parallel.
Router: 3/3 ICMP replies, 0% loss, TCP port 80 reachable. Deck: 0/3 replies,
TCP port 22 still reports "No route to host"; neighbor MAC still matches photo.
Therefore this process DOES have working local-network access to the router;
a blanket Mac local-network permission block is not supported by these results.
The Deck-specific reachability failure remains unexplained; no authenticated SSH
has occurred. Next diagnosis must distinguish Deck availability/current address
from host-specific filtering or Wi-Fi client isolation, without changing settings
or assuming the router's cached neighbor entry proves the Deck is currently awake.
Important subsequent correction: user enabled Codex's Mac Local Network permission
and supplied a photo showing it on. Immediate retry then received 2/2 Deck ping
replies and successfully connected to TCP port 22. Router reachability had NOT
ruled out this permission issue; do not repeat that inference. Network reachability
and an open SSH port are now verified on the real Deck, but not authentication or
game/runtime behavior. Strict SSH stopped with an unknown ED25519 host key; the
dedicated lab private key is still absent. No host key was accepted or stored,
no public key added, and no remote command/game operation ran. Next prerequisite
is explicit first-time pairing with user host-key verification and the Deck password
entered directly into SSH's terminal prompt, followed by read-only preflight.
Latest pairing follow-up: user reported a launcher permission error. The repository
Pair Steam Deck.command is owned by the current user, mode 0755, with no quarantine
attribute; the source of that reported error is not established. Opened this exact
launcher with macOS `open -a Terminal`, which returned success. A subsequent
metadata-only check confirms the dedicated HOME lab directory and private-key file
now exist, so local pairing setup has started. Key contents were not read. Terminal
UI inspection was refused by the computer-use tool; do not bypass that restriction.
Host trust, successful authentication and authorized_keys changes are NOT verified.
Next action: user completes the visible SSH prompt in Terminal (never sends the
password in chat), then run read-only preflight. No game deployment was performed.
Follow-up connection check after the user's key-creation question: actual SSH with
the dedicated key, BatchMode=yes and StrictHostKeyChecking=yes again reached host
verification and stopped because steamdeck.local has no trusted ED25519 host key
in the dedicated known_hosts. Authentication was not attempted; key acceptance
by the Deck remains unverified. Local key creation is complete, first host trust
is still incomplete. Do not conflate these steps or regenerate the existing key.
Further local cryptographic verification: ssh-keygen successfully parsed the
existing private key; its derived public key exactly matches the saved public key.
Private-key permissions are 0600. No key material was printed. The dedicated
known_hosts file does not exist. A strict, noninteractive SSH test to the user's
supplied numeric Deck address also stopped at unknown ED25519 host identity
(exit 255), before authentication. Thus the key pair is verified valid locally,
but Deck authorization remains untested until first host trust is completed.
Old tools/arcade-deck is empty; no existing project SSH credentials were available
there for a read-only preflight. Deck reachability, SSH enablement, correct game
directory, first backup being genuine stock, actual Proton log path, focus/controller
delivery, native behavior and absence of crashes remain unverified assumptions.
Publication transport is tested with mocks; no live hardware result was invented
or published, and arcade-lab-results will be created only by an actual collection.
Mac gh authentication/publication permission will be checked when collecting.

After resolving reachability, the setup action is to double-click
`contrib/arcade-mode/Install Arcade Lab.command`. It reruns all 65 offline tests,
installs under HOME, optionally pairs via SSH's password prompt, then performs ONLY
read-only preflight. It must print LAB READY before proceeding. Pairing is optional
at setup but necessary before strict SSH preflight if no key was authorized.
The user then chooses Deploy Latest Arcade Mode explicitly, launches Skate, tests
A/B/X/Y taps/holds/native controls, collects, closes the game, restores stock, and
collects again. See LAB_PROTOCOL.md for action locations and precise semantics.
Do not begin A/ollie native reverse engineering until observer acceptance is backed
by real evidence. Following gameplay milestone remains A -> native-quality ollie,
then X -> one fixed native flip, then complete grounded/air/grind/manual/landing/bail
observation. Primary technical risk remains commanding Skate's native trick state machine.
