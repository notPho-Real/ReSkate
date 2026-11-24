# Arcade gameplay roadmap

Current work: `arcade-stage-rc51-core-20261122`, based on integrated
`6d8dda8f9431aef21268c74455a5f1ec930df8d8` (official upstream `8515e164`).
Supported game: **20260929**. Experimental only; the accepted channel is unchanged.
Current completion work (November24): `dbc0ca44` verifies genuine vert takeoff,
natural four-wheel return and LT Level Out in native gameplay. Its complete
581-event capture loses no records;39 independent AIR observations match the
submitted native trajectory, followed by supported four-wheel landing.
The mandatory combo recipe withheld RT because its landing gate opened after
the retained descent witness expired. Offline replay corrects the recipe timing
without changing physical or freshness requirements. The same capture also
reveals a production cancellation: native200 advanced to201 between callbacks
while live201 entered idle mode. The source now retains this forward publication
under the unchanged owner, actor, episode, contact and original100ms limits.
A later correlated physical AIR observation is still required for authority.
The retained-trace regression fails before the correction;7,487 controller
checks pass afterward. Compiled `46f15083` verifies genuine takeoff and natural
four-wheel return. Its complete577-event LT trial exposed a separate scheduling
failure: five client callbacks finished the destination fan after the original
150ms input deadline, so no handoff occurred. Discovery now uses a bounded25ms
query-work budget instead of one native physics tick. Client geometry can run
before the current motion observation; final clearance yields to another
callback when discovery spent the old source's freshness allowance. Only the
miss cursor survives, with unchanged150ms input and50ms motion-age limits.
All25 destinations and72 clearance rays remain required. Both cost and callback
ordering regressions fail before their corrections. Offline coverage includes
30/60/120Hz, both callback orders, obstruction, cancellation, owner replacement,
competing native tricks and expiry. The initial `521b3713` build was cancelled
after the offline callback-order reproduction; no native trial used it.
Compiled `9971834a` verifies that scheduling correction in native gameplay:
the fan advances18/25, defers clearance, then submits native AIR Level Out.
Natural takeoff/return passes at1.842/3.403seconds with four-wheel support.
The Level Out trial retains585/585 input events and47 AIR-update observations,
but enters bail402 at3.374seconds with an upward-pitched board. Landing and the
mandatory combo remain blocked; another identical trial is not justified.
The Switch diagnostic now proves native queue admission at priority4 into an
empty action lane. No leading-foot change follows. Its neutral lease retires
before native pushing/support changes complete. Source now preserves neutral
ownership through an issued request's supported state100 preparation; unknown
support, airborne/bail, owner changes, competing input and the original one-second
ceiling still cancel. A retained-shape regression fails before the correction;
2,546 focused checks pass afterward. This reservation correction needs native
verification and does not establish physical Switch success.
**Mandatory owner-ready gate:** genuine Vert → physical landing Revert →
continuous physical Manual/Nose Manual → Ollie → ascending Kickflip → supported
landing and authoritative native bank of the entire line. This sequence has not
yet passed native acceptance. Preserve the verified Manual/Pivot chain below.
Native Switch and exact integrated acceptance also remain open.

Last fully green compiled checkpoint **9971834a2079609de8d95ccd0e012fac128e2851** passes
all required gates in [CI 45355199647](https://github.com/notPho-Real/ReSkate/actions/runs/45355199647),
including148 Windows preflight and137 integration tests.
Runtime SHA256: `7f83bb5e3870a363d6f5f9f98213aea2cb4e3437f073b1ae3245ac47afd52287`.
Switch and the full Revert/Manual line remain unaccepted.
Preserved `cb368987` camera evidence now establishes actual orbit both directions
and neutral recentering to4.96degrees:175 complete ON ticks,161 returned Arcade
camera writes,180 complete OFF ticks with zero Arcade writes, and native camera
restoration on console focus withdrawal. Neither capture lost records. Console
focus released the camera before OFF, so direct OFF of an active lease and
Switch-follow remain unverified. The game is closed; all165 rollback snapshots
and the score pack are verified, and hardware is released.

Preserved native Manual/Pivot evidence: **a235c2782027b63539ea63e506e4cb735831ab92** passes
all nine required gates in [CI 45241154647](https://github.com/notPho-Real/ReSkate/actions/runs/45241154647),
including 148 Windows preflight and 137 integration tests. Runtime SHA256:
`de9dde688560c167f2e80999616ef57ff4d09b6e63fd996808a1c8c3ca807241`.
The exact loaded DLL was verified in Mega Park at native/cached 60Hz. The latest
trial sustained a sampled 3.313-second physical Manual, completed Pivot at 701ms,
continued supported fakie Manual, then executed Ollie and an ascending Kickflip
(first observed upward velocity 3.632m/s) and landed on all four wheel contacts.
Pivot board/hips rotation was 206.93/179.95 degrees, within the unchanged
180±30-degree rotation and 30-degree coherence limits. The same native owner
persisted throughout. The native current line reached 203 points and moved to
the previous line at 19.508s, before OFF at 32.464s. Video corroborates the
balance HUD, Pivot, flight and continued riding. This establishes the compound
chain and native bank, not the displayed multiplier.
A separate immediate A-release/X trial executes native Kickflip during ascent
(first observed upward velocity 4.574m/s), lands on both trucks, and moves its
13-point native sequence into the completed line before OFF. This verifies one
native bank, not multiplied scoring or a compound chain. OFF capture retained
2799/2799 events with no drops; native A passed through without an Arcade jump.
The corrected camera capture records 105 returned Arcade writes, successful
FreeCamera mode1→native mode0 restoration on console focus withdrawal, and
178 subsequent OFF ticks with no Arcade writes. Both captures have zero loss.
It does not establish right-stick orbit or direct OFF of an actively owned camera;
the console already released that lease. Earlier missing-lease/focus failures
remain historical. Switch remains blocked at native ChangeStance admission:
the Regular transition targets ChangeStance but its inspected predicate has no
Input_ChangeStance reference; the Fakie transition explicitly consumes that input.
No verified alternate native command owner/type/phase has been established.
Another identical B timing trial is not justified. Preserve
historical Nollie and vert evidence; no new native acceptance is inferred for
unchanged mechanics. **Not ready for owner testing:** the earlier handoff was
premature. Resolve native Switch and the remaining camera/native acceptance
gaps before owner testing; preserve the verified compound chain and do not
expand the catalog. The first qualified
post-Pivot log batch now primes the test gate instead of being discarded; the
second advancing batch and all existing physical/freshness limits remain required.
The old-failing/new-passing fixture and negative cases pass 22 offline checks;
the corrected gate admitted the complete native chain above. The game is closed,
all 158 rollback sets and the score pack are preserved, and hardware authority
is revoked. Source/DLL identities are unchanged; no duplicate CI was run.

Historical compiled checkpoint **922f303d** passed all nine required gates in
[CI 45130208910](https://github.com/notPho-Real/ReSkate/actions/runs/45130208910):
148 Windows preflight tests, 137 full integration tests and experimental Deck
packaging. Runtime SHA256:
`81dff98584ffec0167364faf369e6372362af4c46e369bda20181efc5fb620e0`.
The earlier test-only C4244 failure was corrected and separately validated before
this build; its failed-run evidence remains historical.

Resource-constrained RC5.1 scope: preserve the integrated catalog, wallplant,
Casper and vert work; no expansion. The 922 native trial sustained a sampled
physical Manual for 3.315s, performed native `bsmanualrevert`, continued supported
fakie `tailmanual`, then executed `ollie` and landed. All 67 consecutive original
Pivot motion rows are retained: board/hips meet the unchanged 180±30-degree and
30-degree coherence criteria at 697ms, within the existing 1100ms window. The
old diagnostic rejects prematurely at 608ms; that does not cancel physical balance.
The source correction now continues observing supported hips convergence within
the original 1100ms window. It changes no motion or tolerance: 218,271 lifecycle
checks pass under ASan/UBSan, and replay of all 67 retained native rows completes
at 697ms. The previous implementation fails the new delayed-convergence case.
The original scoring producer advances 103→118 and then moves 118 from current
to previous line. The displayed ×5/590 is not proof of a multiplied native bank.

The separate ascending-Flip trial withheld A/X when a readiness observation aged
101ms; the same game process remained alive. The test harness now reports
readiness loss accurately and may await a fresh observation only while already
neutral inside the original prerequisite deadline. Active input still requires
the unchanged 100ms freshness. 158 focused runner tests pass; no runtime rebuild
is needed for this test-only fix. The subsequent 922 trial delivered A release
and fresh X together: native Kickflip appeared at 1.638s with upward velocity
4.834m/s, before the sampled 1.821s apex. Independent board-basis samples unwrap
361.65 degrees of physical flip, followed by same-owner supported riding.
The native action record is `kickflip` and awards 75 points. Its settlement was
first sampled after OFF, so normal pre-OFF banking is not inferred.
The subsequent compound trial sustains a sampled 3.290s physical Manual,
executes `bsmanualrevert`, continues supported `tailmanual`, then launches and
executes native `kickflip` before supported landing. Its 67 consecutive original
Pivot rows meet the unchanged rotation/coherence criteria at 705ms; final board
and hips rotation are 194.25 and 202.44 degrees. The native producer settles
333 points from current to previous line at 19.347s, before OFF at 31.661s.
This verifies representative native chain banking, not the displayed multiplier.
The retained video shows balance HUD, physical Pivot, flight and continued riding.
The earlier session restored exact prior 5ae files with 156 rollback sets and
the score pack preserved. Switch remains
blocked at the unverified native ChangeStance consumer owner/value-slot/phase;
another identical B-gesture trial is not justified. Separate Nose Manual,
Grind-chain, multiplied native banking/bail and full OFF control/camera acceptance
remain open. Exact-candidate Pivot observation evidence is recorded above.

The 922 vert follow-up recognizes genuine takeoff and natural curved-ramp
return. Fresh LT submits Level Out, reaches native-owned AIR, and returns to
two-truck support at 3.800s with continued riding through 8.549s. The retained
landing publication gap preserves lineage until original support; landing
Revert was not tested. This session restored exact prior 5ae files, preserving
157 rollback sets and the score pack, and revoked hardware authority.

Candidate 12644089 compiled the Windows preflight, but 13 tests shared an outdated
DEV fixture assertion expecting immediate rejection of incomplete Pivot motion.
The test-only correction expects bounded tracking, exercises delayed supported
hips convergence through production callbacks, and preserves rejection after a
real motion gap. It does not change gameplay source or require another DLL build.

The 7feb native no-input trial recognized real AIR and retained direct-touchdown
history through original supported riding. Natural return still lands behind the
ramp. Its fresh-LT Level Out trial submitted a bounded plan but produced a large
next-step velocity spike and cancelled; physical Level Out is **not accepted**.
Pinned executable inspection identified the missing AIR201 curve-change
notification when using the pop setter. The integrated follow-up uses the native
AIR updater and leaves selection/rebasing to its ordinary tick. Its packet uses
the current AIR-specific tuning pair through the verified native accessor;
state200 and contact launches remain unchanged. Its 33 contract checks pass with
old-route and generic-settings failures. The later checkpoint below supplies
Windows compilation and representative native motion evidence.

The later **5f9eea19** neutral-before-takeoff trial establishes representative
normal return onto the curved ramp and continued riding. Fresh LT physically
leveled the board and reached platform support without the previous velocity
spike, but the controller falsely cancelled on native curve adoption. Full
Level Out and landing Revert acceptance remain withheld. The integrated
follow-up hands future Level Out motion back to native AIR only after successful
submission and transactional body writing. It cannot emit another transfer or
alignment; actual support and a fresh trigger still govern landing/Revert.
Checks pass: 6,956 controller, 90,466 geometry/runtime and 33 trajectory contracts.
That session restored the prior installation with 153 rollback sets preserved.

Candidate `10954ec7` failed Windows preflight because the ground-writer source
fixture included the new vert handoff block. The fixture now extracts its angular
write and completion separately; 2,741 checks pass in both release and DEV with
ASan/UBSan. A further captured-packet correction measures landing speed along the
actual support plane: the steep ramp had 9.05m/s tangential speed but only
0.99m/s horizontal speed. The 1m/s minimum and all contact/ownership requirements
remain; 6,983 controller checks pass, including pure-impact and flat-ground
negatives. The green `096bbf03` build includes both corrections.

Native `096bbf03` passes representative no-trigger curved takeoff, supported
return and continued riding. The separate LT trial rejects a state-publication
chain before any plan or motion write; RT was skipped. All original callbacks
are present, but the retained original/live pair and actor/episode fields were
not captured. Conditional ground-to-state200 and idle-AIR successor corrections
pass 7,059 controller and 90,466 runtime/geometry checks with unchanged
freshness/step limits. Each original must confirm the preceding live state before
a correlated physical AIR frame can establish eligibility. Those conditional
corrections compile in418, but do not establish native acceptance.

The `418aa996` no-trigger trial captures the previously missing original/live
pair: a valid two-step takeoff confirmation bypassed code that ran only for
longer gaps. No transfer plan or LT/RT trial followed. The integrated correction
uses the same exact proof across that boundary and retains only confirmed200/201
successors; 7,195 controller and90,466 runtime/geometry checks pass. Windows
compilation passes in922; native vert acceptance remains pending. Exact prior files are
restored, all155 rollback sets preserved, and hardware authority revoked.

The earlier `6f132763` Mega Park trial observed a **3.301-second sampled physical Manual hold**:
97 of 100 observations qualified, with a maximum 107ms qualified gap, stable
rear-truck support and owner, and visible balance HUD. RT caused physical board
and body rotation and a fakie change, but Pivot proof rejected a 42-degree
board/body difference at the first successor observation. Later exact hips
convergence, Ollie-out, the full chain and native banking remain unverified.

That `6f132763` Switch trial delivered both RT-triggered B double taps and retained neutral ownership,
but neither changed the native leading foot. Native push still ended observation;
the second push flag appeared before automatic X resumed. The cause remains
unresolved. The neutral hold passes 2,371 sanitizer and 203 production publication
checks; those checks establish input ownership, not native stance execution.

The compiled `fbb09e16` batch also withholds scalar speed assistance throughout that
same stance reservation (244 production-path checks). Existing DEV Pivot rows
now retain current board/hips bases through the original 1100ms observation
window, including after rejection. Owner/episode loss, release and OFF close
the trace; this changes neither the physical mechanic nor its acceptance.
Strict telemetry and composed lifecycle tests pass in release and DEV builds.

Earlier `ec28f738` evidence observed ordinary Ollie1 rise and landing. Its LT
reached kernel/SDL but never captured game input; that trial cannot test Nollie.
The newer `fbb09e16` moving-route trial delivered LT through kernel255/SDL32767
to nine raw game-input polls, with an 80ms press/release interval and stable
owner/route. Returned LT0 matches existing Arcade trigger consumption. A was
withheld: all 12 second-gate physical samples exceeded the recipe's 8m/s limit
(9.57–9.92m/s), with stale log arrival later. No Nollie was attempted. The
capture drained574 rows without drops but ended incomplete; the earlier LT loss
layer remains unisolated, rather than an ongoing demonstrated delivery failure.
Exact LT/A replay passes 788 checks: delivered LT selects16, omitted LT selects1.
Pressure remains unsupported without an exact native actor.

The separate `fbb09e16` coast trial timed out before RT. Captured automatic X
stopped with raw stick neutral; native push began176ms later and cleared400ms
after neutral while returned buttons stayed zero. Late log arrival and physical
push prevented the coast prerequisite. No RT/B or Switch result was obtained;
this does not establish the cause of native pushing.

The corrected coasting recipes ran once each on **fdf00b79**. Physical LT tap
followed by A charge/release executed **native Nollie16**, with measured rise and
full wheel support on return, stable owner and complete714/714 capture. A separate
A-only control executed **Ollie1**, with no LT/RT input, physical rise and supported
return, and complete693/693 capture. Both had zero drops. This establishes those
representative native launch identities, not Pressure or every stance/combo.

Ground RT delivered a complete native B double-tap but **did not change leading
foot**. All615/615 capture rows were retained without drops; no pushing occurred
during the gesture or observation window. Heading and continued travel were
preserved, without a ground Revert. Native double-tap processor acceptance or
ChangeStance graph entry is still unobserved. Final XInput packets prove delivery,
not that consumer boundary; no justified retiming or alternate setter is known.
Exact receipts and independent analyses are retained externally under
`rc51-ps2-controls-native-fdf00b79-package-bound`.

Published follow-up fixes preserve passive Nollie/Pressure setup across native
pushes (the pushing flag was incorrectly treated as a bail), and preserve final
neutral Switch ownership while a live leading-foot change awaits publication.
The setup correction passes 862 selection and 96 composed dispatch checks;
the stance correction passes 759 production-adapter checks plus its focused
ownership suites. Both include old-source failures and sanitizer coverage.
These fixes are included in the green `fdf00b79` build. Its separate native
launch comparisons establish Nollie16 versus Ollie1; Switch remains blocked.

The newer source also preserves bounded ramp history across native push onset
and clearing, and excludes authenticated zero-fraction local self hits without
using their borrowed contact point as destination geometry. Focused sanitizer
checks pass: 6,812 vert, 90,117 geometry/runtime and 106 contact-reader. Native
qualification and transfer remain unverified; details are below.

Implementation, compilation, offline tests, native observation and owner acceptance
are separate states. Historical captures support only their recorded source and
conditions. No current feature has blanket owner acceptance from this checkpoint.

| Priority / milestone | Expected play and current implementation | Verification / remaining issue | Complexity; next action |
| --- | --- | --- | --- |
| 0 / RC5 integration | Existing controller ownership, animations, Casper, wallplants, scoring, Trickpedia, camera and OFF restoration retained with upstream pose/mount facilities. | Base compiled and CI passed. Exact integrated native walkthrough remains incomplete. | M; consolidated native regression after source corrections, with exclusive runner and rollback. |
| 1 / RC5.1 immediate/repeated flips | Immediate A→X recognizes the physical edge through the admitted pop. Ordinary flips share the owner-validated transient-zero reread. Neutral third X extends the owned double into Triple Kickflip. | 2,635 held-flip sanitizer checks pass. Exact a235 native immediate Kickflip starts during ascent and lands; post-Pivot Kickflip also ascends and banks in a compound line. Triple physical rotations remain unverified. | S; preserve passing ascent evidence; verify repeated execution only when needed for the integrated checkpoint. |
| 1 / RC5.1 fixed ground controls | LT setup and native Switch adapter use the shared journal. Original held-A intent survives native crouch retirement; setup now survives native pushing. Switch authenticates exported instances and retains neutral ownership through bounded publication. No ground Revert fallback or heading/velocity writes. | fdf native LT→A executes Nollie16; A-only executes Ollie1, both with physical flight/support and complete captures. RT still produces no leading-foot change after a complete B gesture without pushing. Pressure actor unsupported. | M; establish native double-tap consumer/ChangeStance acknowledgment or a supported native stance route; preserve verified launch behavior and continue independent gameplay work. |
| 1 / RC5.1 Manuals/Pivots | Established Manual RT retains its original150ms publication lease and requests physical Pivot; unconfirmed balance cannot borrow later authority. Physical board/hips rotation and successor support determine acceptance. | Exact a235 native sampled3.313s hold, physical Pivot at701ms within unchanged thresholds, supported fakie Manual, Ollie, ascending Kickflip and landing; native compound line banks203 before OFF. Separate Nose Manual remains unverified. | M; preserve completed chain evidence; verify Nose Manual support in the consolidated acceptance. |
| 1 / RC5.1 general wallplants | All-category wall/floor patches span independently authenticated static bodies. Every floor center and child owner is revalidated before admission, including single-shape floors; existing holds depend on the wall. | 55,816 geometry and1,523 runtime checks pass across transformed partitions, categories, mixed owner forms, corners, obstructions and invalidation. Old source fails deleted-floor admission. Same-incarnation editor relocation has no transform stamp, including center; no universal map claim. | M; preserve static snapshot scope; native query/hold smoke at a later checkpoint, after independent offline work. |
| 2 / RC5.1 movement/rotation/camera | Auto-push, native steering, Ollie, prewind, aerial rotation and camera orbit/recenter already implemented. New camera leases reset follow heading immediately after OFF/ON or controller/native-owner changes. | 691,135 composed lifecycle checks and 2,915 camera checks pass; the new reset regression fails before correction. Native switch/revert follow and visual restoration pending. | S–M; retain continuous smoothing; native visual confirmation at a later checkpoint. |
| 2 / RC5.1 balance HUD/scoring | Native Manual episode controls HUD lifetime; native/custom trick rows and model combo calculation remain distinct. | Model/row/bank-boundary tests exist. 922 original native current-line118 moves to previous-line118. Multiplied×5/590 banking and bail remain unverified; displayed totals are separate. | M; fix visible problems from retained evidence; native bank/bail confirmation at the combined checkpoint. |
| 3 / RC5.2 native flip catalog | Existing R3 shoves/Impossible remain. R3+diagonal+singleX directly selects360 Hardflip,360 Inward Heelflip,360 Flip or Laserflip, including exact Nollie variants, before a base flip starts. Ordinary and double-X mappings remain. | 16,238 composed flip checks and141-entry Trickpedia suite pass; old registry fails direct-route regressions. Exact gestures refuse substitution. New direct bindings compile in922; native execution and ordinary Hardflip remain unresolved. | S per proven mapping; preserve this compiled catalog while RC5.1 acceptance is completed; no expansion. |
| 3 / RC5.2 grabs/ground tricks | Directional B/B,B, extended grabs, one-footers, No Comply and Boneless reuse existing native systems. Extended grabs reserve R3 only for the captured B-edge modifier; a later unrelated R3 remains native. | All 57 supported grab profiles have reachable selectors. Focused modifier ownership regressions pass; historical actor evidence does not establish exact current hold/release/stance behavior. | S–M; native hold/release smoke for representative variants; unsupported names need exact native contracts. |
| 3 / RC5.2 grinds | Bluntslide X→Y/X→X continues into Tailslide/Smith. Y→B/B→B continues into Nosegrind/Crooked. Established5-0 can now reach Noseslide/Nosebluntslide through independently completed50-50 and Nosegrind legs; one original request, fixed total deadline, exact segment and retired leases. | 48,100 composed sanitizer checks pass at30/60/120Hz; old source fails the new5-0 routes. Qualified horizontal segment≥12m does not guarantee enough remaining travel. Full native chains remain unverified. | M; preserve each completion and final actor evidence; extend only proven native routes. |
| 4 / RC5.3 Casper | Manual→Casper→Casper Flip uses one owned body/board clip; integrated drawn seed experiment defaults off. | Authoring/runtime cancellation tests; physical support, visible placement and score continuity still need exact candidate acceptance. | M; finish current entry/exit/friction before additional flatland poses. |
| 5 / RC5.3 flatland depth | Further grind depth, Railstand, Truckstand, Anti-Casper, Primo and Pogo follow Casper. Core Manual Pivot is now RC5.1. | Upstream pose/mount facilities supply visuals, not physical support or contact. | M–L; reuse owned playback and permission-gated plugin APIs; establish physical admission first. |
| 6 / RC6 environment | Shared native ownership/geometry, Level Out handoff and support-plane landing speed are retained. Exact takeoff publication confirmation now applies to short and longer gaps. | Green418 captures a two-step confirmation bypass before any plan; corrected source passes7,195 controller checks. Actual failure pair is captured; later live-pair sequencing remains conditional. Physical transfers remain unaccepted. | M–L; compile with the RC5.2 mappings, then establish native admission/handoff/support before independent landing Revert. |
| 7 / RC7+ | Specials, advanced flatland, skitching, larger combos and challenges. | Deferred behind responsive core and environment. | L; no current implementation action. |

Representative acceptance uses only the scenarios affected by a change; all eight
are consolidated at release-candidate checkpoints:

| Scenario | Required result |
| --- | --- |
| A: Immediate flip | Auto-push → Ollie → immediate X; native flip during ascent, then land. |
| B: Manual | Enter and physically maintain Manual for **at least 3 continuous seconds**, then Ollie out. |
| C: Pivot chain | Manual → RT → measured physical Pivot → valid successor balance → Ollie → Flip → land; verify stance, support, HUD and combo ownership. |
| D: Repeated flips | Rapid valid presses produce the exact double/triple actor and physical rotations. |
| E: Grind combo | Ollie → Grind → Pop-out → Flip/Grab → Manual → land. |
| F: Wallplant | Contact → planted hold → outward/upward rebound → follow-up → land; varied local geometry, category, ownership and obstruction fixtures pass offline. |
| G: Casper | Manual → Casper → Casper Flip → Manual → Ollie → land with body/board and score evidence. |
| H: Camera/rotation | Prewind → Ollie → rotation → switch landing; correct steering, camera and OFF restoration. |

Three seconds is a minimum test interval measured from qualified physical contact,
not an input-hold duration or a gameplay timeout. A balanced Manual can continue
longer. Historical two/ten-second recipes and captures remain historical; neither
defines current acceptance. Offline checks do not prove native animation or play feel.

## Experimental vert controller

The isolated vert-transfer workstream adds concave-transition takeoff recognition,
unchanged no-input flight, contextual LT/RT Level Out, and a shared destination
solver for Spine Transfer, Hip Transfer and Acid Drop. It reuses native body
ownership, the timestamped input journal, static-owner geometry, collision and
physical orientation limits. No new map IDs, native state fabrication or scoring
awards are introduced.

The `6f132763` no-input trial observed physical ramp rise, flight and return,
but qualification ended before AIR at a push-clear/contact publication gap.
The later `fbb09e16` trial isolated another rejected boundary: established
curved support at step9650, state100/mode1/contacts0xc000 and unchanged owner,
with push0→0x40. Original AIR followed; idle201 later cleared push while entering
mode2. Neither trial submitted a transfer trigger or proved native qualification.
The current source retains only bounded history across either push edge on
proven curved support, plus that exact idle-air handoff. Fresh original actor,
owner and push confirmation remains mandatory; leading-foot changes and unrelated
actors are rejected. Mismatched frames grant no input or motion authority and
cannot renew curvature, takeoff or trigger deadlines.
The recorded mode2→1/partial-contact landing boundary now also retains the
pre-contact descent witness for at most100ms. Transfer motion stops immediately;
only original supported riding contact can release a reserved landing Revert.
Original trigger deadlines remain unchanged. Stable zero-fraction local self
points may differ from ray origin; only an independently authenticated local
owner may supply an exclusion. Nonzero and nonlocal surface measurements retain
strict ray-position/normal validation. Focused sanitizer checks pass:6,812 vert,
90,117 geometry/runtime and106 contact-reader, with failing old-source regressions.
The fdf no-input trial subsequently established native flight qualification;
natural ramp return, transfer and landing Revert remain unaccepted. The integrated
follow-up retains direct AIR→riding touchdown and consecutive original/live
contact handoffs only with confirmed owner, actor and unchanged100ms history
deadlines. It grants no input or motion until correlated original observations.
Focused ASan/UBSan checks pass:6,868 vert and90,448 geometry/runtime. New native
causation and transfer behavior still require exact-candidate evidence.

Portable tests cover transformed curved ramps, independent shape owners, holes,
clearance failures, bounded momentum, trigger timing, ownership cancellation and
query-to-physics races. Native gameplay acceptance remains pending; see
[the implementation evidence](research/vert-transfer-implementation.md).
