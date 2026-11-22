# Arcade gameplay roadmap

Current work: `arcade-stage-rc51-core-20261122`, based on integrated
`6d8dda8f9431aef21268c74455a5f1ec930df8d8` (official upstream `8515e164`).
Supported game: **20260929**. Experimental only; the accepted channel is unchanged.
Current compiled checkpoint `fbb09e16` passed all nine required gates in
[CI 45076961332](https://github.com/notPho-Real/ReSkate/actions/runs/45076961332),
including Windows preflight, full runtime integration and experimental Deck
packaging. Native observations below belong to the earlier `6f132763` batch,
after which the exact prior installation was restored; they do not verify `fbb09e16`.

The Mega Park trial observed a **3.301-second sampled physical Manual hold**:
97 of 100 observations qualified, with a maximum 107ms qualified gap, stable
rear-truck support and owner, and visible balance HUD. RT caused physical board
and body rotation and a fakie change, but Pivot proof rejected a 42-degree
board/body difference at the first successor observation. Later exact hips
convergence, Ollie-out, the full chain and native banking remain unverified.

Switch delivered both RT-triggered B double taps and retained neutral ownership,
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
Exact LT/A replay passes 788 checks: delivered LT selects16, omitted LT selects1.
Pressure remains unsupported without an exact native actor.

Local follow-up fixes preserve passive Nollie/Pressure setup across native
pushes (the pushing flag was incorrectly treated as a bail), and preserve final
neutral Switch ownership while a live leading-foot change awaits publication.
The setup correction passes 862 selection and 96 composed dispatch checks;
the stance correction passes 759 production-adapter checks plus its focused
ownership suites. Both include old-source failures and sanitizer coverage.
These fixes follow the published `fbb09e16` candidate and are not in its build;
neither establishes native Nollie or Switch acceptance.

General geometry self-hit handling preserves native position, query readiness,
owner and ordering checks while skipping an unused self normal. The compiled
correction passes 89,988 geometry/runtime and 78 contact-reader sanitizer checks.
No recorded map failure is attributed to it without native evidence.

Implementation, compilation, offline tests, native observation and owner acceptance
are separate states. Historical captures support only their recorded source and
conditions. No current feature has blanket owner acceptance from this checkpoint.

| Priority / milestone | Expected play and current implementation | Verification / remaining issue | Complexity; next action |
| --- | --- | --- | --- |
| 0 / RC5 integration | Existing controller ownership, animations, Casper, wallplants, scoring, Trickpedia, camera and OFF restoration retained with upstream pose/mount facilities. | Base compiled and CI passed. Exact integrated native walkthrough remains incomplete. | M; consolidated native regression after source corrections, with exclusive runner and rollback. |
| 1 / RC5.1 immediate/repeated flips | Immediate A→X recognizes the physical edge through the admitted pop. Ordinary flips share the owner-validated transient-zero reread. Neutral third X now extends the owned double into Triple Kickflip; directional triples remain. | 2,635 held-flip sanitizer checks pass with physical parent/deadline, catch and fourth-input negatives; old source fails neutral-third regression. Current ascent and triple rotations remain native-unverified. | S; native ascent/rapid repeat at a later checkpoint; continue independent offline work first. |
| 1 / RC5.1 fixed ground controls | LT setup and native Switch adapter use the shared journal. Original held-A intent survives native crouch retirement with matching selection, generations and wheel contacts; launch identity survives LT release. Switch authenticates exported instances and now retains neutral ownership through its bounded acknowledgment. No ground Revert fallback or heading/velocity writes. | 6f132763 compiled and delivered both double taps with neutral ownership, but no native stance change. Native push still ends observation; its cause is unresolved. Exact LT/A replay passes788 checks: delivered LT selects16; omitted LT selects1. Hardware LT disappears upstream of raw input capture. Ordinary Ollie1 observed once. Pressure actor unsupported; no stance/Nollie/Pressure acceptance. | M; scalar assistance now obeys the same reservation (244 production checks); resolve native push provenance offline. Resolve upstream LT delivery before another LT trial. |
| 1 / RC5.1 Manuals/Pivots | Established Manual RT retains its original150ms publication lease and requests physical Pivot; unconfirmed balance cannot borrow later authority. Physical board-reference/hips rotation plus native successor support independently determine evidence. | Composed lifecycle checks pass in release and DEV. Exact6f native hold meets sampled3s minimum; physical Pivot turns but fails board/body coherence. Six-second hold simulation passes; full chain pending. | M; retain actual hips basis through the existing Pivot observation window; distinguish delayed body settling from incorrect physical rotation. |
| 1 / RC5.1 general wallplants | All-category wall/floor patches span independently authenticated static bodies. Every floor center and child owner is revalidated before admission, including single-shape floors; existing holds depend on the wall. | 55,816 geometry and1,523 runtime checks pass across transformed partitions, categories, mixed owner forms, corners, obstructions and invalidation. Old source fails deleted-floor admission. Same-incarnation editor relocation has no transform stamp, including center; no universal map claim. | M; preserve static snapshot scope; native query/hold smoke at a later checkpoint, after independent offline work. |
| 2 / RC5.1 movement/rotation/camera | Auto-push, native steering, Ollie, prewind, aerial rotation and camera orbit/recenter already implemented. New camera leases reset follow heading immediately after OFF/ON or controller/native-owner changes. | 691,135 composed lifecycle checks and 2,915 camera checks pass; the new reset regression fails before correction. Native switch/revert follow and visual restoration pending. | S–M; retain continuous smoothing; native visual confirmation at a later checkpoint. |
| 2 / RC5.1 balance HUD/scoring | Native Manual episode controls HUD lifetime; native/custom trick rows and model combo calculation remain distinct. | Model/row/bank-boundary tests exist. Authoritative native banking is not established by model totals or lingering labels. | M; fix visible problems from retained evidence; native bank/bail confirmation at the combined checkpoint. |
| 3 / RC5.2 native flip catalog | R3+Down/Up+X selects frontside shoves. R3+Left+X now selects Front Foot Impossible37, or Nollie Back Foot Impossible39; native card names stay distinct. Ordinary mappings and Laserflip name remain. | 15,799 flip checks,141 canonical entries and native guide restoration pass. Missing exact gestures refuse without substitution. Source identities establish routing, not performed gameplay. Hardflip remains unresolved. | S per proven mapping; verify new actors/rotations/landing before acceptance; reuse exact native catalog for further source work. |
| 3 / RC5.2 grabs/ground tricks | Directional B/B,B, extended grabs, one-footers, No Comply and Boneless reuse existing native systems. Extended grabs reserve R3 only for the captured B-edge modifier; a later unrelated R3 remains native. | All 57 supported grab profiles have reachable selectors. Focused modifier ownership regressions pass; historical actor evidence does not establish exact current hold/release/stance behavior. | S–M; native hold/release smoke for representative variants; unsupported names need exact native contracts. |
| 3 / RC5.2 grinds | Bluntslide X→Y/X→X continues into Tailslide/Smith. Y→B/B→B continues into Nosegrind/Crooked. Established5-0 can now reach Noseslide/Nosebluntslide through independently completed50-50 and Nosegrind legs; one original request, fixed total deadline, exact segment and retired leases. | 48,100 composed sanitizer checks pass at30/60/120Hz; old source fails the new5-0 routes. Qualified horizontal segment≥12m does not guarantee enough remaining travel. Full native chains remain unverified. | M; preserve each completion and final actor evidence; extend only proven native routes. |
| 4 / RC5.3 Casper | Manual→Casper→Casper Flip uses one owned body/board clip; integrated drawn seed experiment defaults off. | Authoring/runtime cancellation tests; physical support, visible placement and score continuity still need exact candidate acceptance. | M; finish current entry/exit/friction before additional flatland poses. |
| 5 / RC5.3 flatland depth | Further grind depth, Railstand, Truckstand, Anti-Casper, Primo and Pogo follow Casper. Core Manual Pivot is now RC5.1. | Upstream pose/mount facilities supply visuals, not physical support or contact. | M–L; reuse owned playback and permission-gated plugin APIs; establish physical admission first. |
| 6 / RC6 environment | Wallrides, wallies, wallpushes, stalls/lips, acid drops, spine/hip transfers and gaps follow core acceptance. | No new acceptance claimed. Generalized wall-contact infrastructure is the intended foundation. | M–L; implement a simple supported mechanic after RC5.1 wall routing is verified. |
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

The `6f132763` no-input trial observed a physical ramp rise, flight and return,
but qualification still ended before AIR. Offline replay identifies a rising
support publication that clears native push0x40 while wheel contacts change;
the old history bridge rejected it. The source correction retains only
bounded history across that push-clear edge, requires a fresh independent
confirmation, and keeps push onset, leading-foot changes and actor changes
rejected. Mismatched frames grant no transfer or motion authority.
The recorded mode2→1/partial-contact landing boundary now also retains the
pre-contact descent witness for at most100ms. Transfer motion stops immediately;
only original supported riding contact can release a reserved landing Revert.
Original trigger deadlines remain unchanged. The combined correction passes
6,443 vert and89,999 geometry/runtime sanitizer checks; native query admission
and the106 contact-reader checks remain intact.
Native flight qualification, transfer and landing acceptance remain pending.

Portable tests cover transformed curved ramps, independent shape owners, holes,
clearance failures, bounded momentum, trigger timing, ownership cancellation and
query-to-physics races. Native gameplay acceptance remains pending; see
[the implementation evidence](research/vert-transfer-implementation.md).
