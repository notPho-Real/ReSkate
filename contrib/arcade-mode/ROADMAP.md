# Arcade gameplay roadmap

Current work: `arcade-stage-rc51-core-20261122`, based on integrated
`6d8dda8f9431aef21268c74455a5f1ec930df8d8` (official upstream `8515e164`).
Supported game: **20260929**. Experimental only; the accepted channel is unchanged.
Current compiled checkpoint **7febbc63** passed all nine required gates in
[CI 45096437488](https://github.com/notPho-Real/ReSkate/actions/runs/45096437488):
148 Windows preflight tests, 136 full integration tests and experimental Deck
packaging. Runtime SHA256:
`b7e814bb5de19ad3ffcccfd6525afaadf3f3ed6b2f8c4260c707364748f74a45`.
The earlier test-only C4244 failure was corrected and separately validated before
this build; its failed-run evidence remains historical.

The 7feb native no-input trial recognized real AIR and retained direct-touchdown
history through original supported riding. Natural return still lands behind the
ramp. Its fresh-LT Level Out trial submitted a bounded plan but produced a large
next-step velocity spike and cancelled; physical Level Out is **not accepted**.
Pinned executable inspection identified the missing AIR201 curve-change
notification when using the pop setter. The integrated follow-up uses the native
AIR updater and leaves selection/rebasing to its ordinary tick; state200 and
contact launches remain unchanged. Its 28 contract checks pass with an old-route
failure; Windows compilation and exact-candidate native motion remain pending.

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
| 1 / RC5.1 immediate/repeated flips | Immediate A→X recognizes the physical edge through the admitted pop. Ordinary flips share the owner-validated transient-zero reread. Neutral third X now extends the owned double into Triple Kickflip; directional triples remain. | 2,635 held-flip sanitizer checks pass with physical parent/deadline, catch and fourth-input negatives; old source fails neutral-third regression. Current ascent and triple rotations remain native-unverified. | S; native ascent/rapid repeat at a later checkpoint; continue independent offline work first. |
| 1 / RC5.1 fixed ground controls | LT setup and native Switch adapter use the shared journal. Original held-A intent survives native crouch retirement; setup now survives native pushing. Switch authenticates exported instances and retains neutral ownership through bounded publication. No ground Revert fallback or heading/velocity writes. | fdf native LT→A executes Nollie16; A-only executes Ollie1, both with physical flight/support and complete captures. RT still produces no leading-foot change after a complete B gesture without pushing. Pressure actor unsupported. | M; establish native double-tap consumer/ChangeStance acknowledgment or a supported native stance route; preserve verified launch behavior and continue independent gameplay work. |
| 1 / RC5.1 Manuals/Pivots | Established Manual RT retains its original150ms publication lease and requests physical Pivot; unconfirmed balance cannot borrow later authority. Physical board-reference/hips rotation plus native successor support independently determine evidence. | Composed lifecycle checks pass in release and DEV. Exact6f native hold meets sampled3s minimum; physical Pivot turns but fails board/body coherence. Six-second hold simulation passes; full chain pending. | M; use the compiled bounded hips/board trace when a native Pivot check is justified; distinguish delayed body settling from incorrect physical rotation. |
| 1 / RC5.1 general wallplants | All-category wall/floor patches span independently authenticated static bodies. Every floor center and child owner is revalidated before admission, including single-shape floors; existing holds depend on the wall. | 55,816 geometry and1,523 runtime checks pass across transformed partitions, categories, mixed owner forms, corners, obstructions and invalidation. Old source fails deleted-floor admission. Same-incarnation editor relocation has no transform stamp, including center; no universal map claim. | M; preserve static snapshot scope; native query/hold smoke at a later checkpoint, after independent offline work. |
| 2 / RC5.1 movement/rotation/camera | Auto-push, native steering, Ollie, prewind, aerial rotation and camera orbit/recenter already implemented. New camera leases reset follow heading immediately after OFF/ON or controller/native-owner changes. | 691,135 composed lifecycle checks and 2,915 camera checks pass; the new reset regression fails before correction. Native switch/revert follow and visual restoration pending. | S–M; retain continuous smoothing; native visual confirmation at a later checkpoint. |
| 2 / RC5.1 balance HUD/scoring | Native Manual episode controls HUD lifetime; native/custom trick rows and model combo calculation remain distinct. | Model/row/bank-boundary tests exist. Authoritative native banking is not established by model totals or lingering labels. | M; fix visible problems from retained evidence; native bank/bail confirmation at the combined checkpoint. |
| 3 / RC5.2 native flip catalog | R3+Down/Up+X selects frontside shoves. R3+Left+X now selects Front Foot Impossible37, or Nollie Back Foot Impossible39; native card names stay distinct. Ordinary mappings and Laserflip name remain. | 15,799 flip checks,141 canonical entries and native guide restoration pass. Missing exact gestures refuse without substitution. Source identities establish routing, not performed gameplay. Hardflip remains unresolved. | S per proven mapping; verify new actors/rotations/landing before acceptance; reuse exact native catalog for further source work. |
| 3 / RC5.2 grabs/ground tricks | Directional B/B,B, extended grabs, one-footers, No Comply and Boneless reuse existing native systems. Extended grabs reserve R3 only for the captured B-edge modifier; a later unrelated R3 remains native. | All 57 supported grab profiles have reachable selectors. Focused modifier ownership regressions pass; historical actor evidence does not establish exact current hold/release/stance behavior. | S–M; native hold/release smoke for representative variants; unsupported names need exact native contracts. |
| 3 / RC5.2 grinds | Bluntslide X→Y/X→X continues into Tailslide/Smith. Y→B/B→B continues into Nosegrind/Crooked. Established5-0 can now reach Noseslide/Nosebluntslide through independently completed50-50 and Nosegrind legs; one original request, fixed total deadline, exact segment and retired leases. | 48,100 composed sanitizer checks pass at30/60/120Hz; old source fails the new5-0 routes. Qualified horizontal segment≥12m does not guarantee enough remaining travel. Full native chains remain unverified. | M; preserve each completion and final actor evidence; extend only proven native routes. |
| 4 / RC5.3 Casper | Manual→Casper→Casper Flip uses one owned body/board clip; integrated drawn seed experiment defaults off. | Authoring/runtime cancellation tests; physical support, visible placement and score continuity still need exact candidate acceptance. | M; finish current entry/exit/friction before additional flatland poses. |
| 5 / RC5.3 flatland depth | Further grind depth, Railstand, Truckstand, Anti-Casper, Primo and Pogo follow Casper. Core Manual Pivot is now RC5.1. | Upstream pose/mount facilities supply visuals, not physical support or contact. | M–L; reuse owned playback and permission-gated plugin APIs; establish physical admission first. |
| 6 / RC6 environment | Experimental vert recognition, Level Out and shared acid-drop/spine/hip solver reuse native ownership and generalized geometry. AIR201 replacements now use the native curve updater rather than a new pop. | 7feb recognizes AIR and retains direct touchdown; natural ramp return fails. Its Level Out produced a velocity spike. The updater correction passes 28 contract checks plus unchanged vert/runtime suites; physical transfers remain unaccepted. | M–L; compile the AIR update correction once, then verify replacement timing and physical motion; natural return remains separate. |
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
