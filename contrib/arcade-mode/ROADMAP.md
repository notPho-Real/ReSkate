# Arcade gameplay roadmap

Current work: `arcade-stage-rc51-core-20261122`, based on integrated
`6d8dda8f9431aef21268c74455a5f1ec930df8d8` (official upstream `8515e164`).
Supported game: **20260929**. Experimental only; the accepted channel is unchanged.
The previous checkpoint `d7d83a6a` passed all nine required gates in
[CI 44968422979](https://github.com/notPho-Real/ReSkate/actions/runs/44968422979).
The current batch adds grind continuations, native trick mappings, camera lease
restoration and collision-shape seam support. Previous CI does not compile or
accept these subsequent gameplay changes.

Implementation, compilation, offline tests, native observation and owner acceptance
are separate states. Historical captures support only their recorded source and
conditions. No current feature has blanket owner acceptance from this checkpoint.

| Priority / milestone | Expected play and current implementation | Verification / remaining issue | Complexity; next action |
| --- | --- | --- | --- |
| 0 / RC5 integration | Existing controller ownership, animations, Casper, wallplants, scoring, Trickpedia, camera and OFF restoration retained with upstream pose/mount facilities. | Base compiled and CI passed. Exact integrated native walkthrough remains incomplete. | M; consolidated native regression after source corrections, with exclusive runner and rollback. |
| 1 / RC5.1 immediate/repeated flips | Immediate A→X recognizes the physical edge through the admitted pop. Ordinary flips now share the owner-validated transient-zero reread. Rapid doubles and directional triple Kickflip/Heelflip use the existing native actor. | Focused immediate, repeated, grab and rail-pop sanitizer regressions pass; exact current ascent and triple rotations remain native-unverified. | S; representative native ascent/rapid repeat trial with unchanged physical deadlines. |
| 1 / RC5.1 Manuals/reverts | Analog diagonals, reversals, neutral gaps and D-pad entry feed native Manual/Nose Manual. RT handoff and Ollie exit retain combo ownership. No new physical tuning in this checkpoint. | 13,560 Manual step checks pass, including six-second Manual/Nose episodes at30/60/120Hz without duplicate integration; native continuous hold and revert chain pending. | S; one representative physical hold/RT chain when native access is available. |
| 1 / RC5.1 general wallplants | Static-world corroboration queries all categories; coplanar wall/floor child shapes of one authenticated static body form patches. Every child is revalidated before entry; wall children remain checked throughout hold. | 25,910 geometry and 1,317 runtime checks pass, including translated/rotated floor and wall partitions, materials/categories, corners, holes, obstructions and post-publication invalidation. Different bodies remain rejected; no universal map claim. | M; representative native category-query/hold smoke at a later checkpoint; independent offline features continue first. |
| 2 / RC5.1 movement/rotation/camera | Auto-push, native steering, Ollie, prewind, aerial rotation and camera orbit/recenter already implemented. New camera leases reset follow heading immediately after OFF/ON or controller/native-owner changes. | 691,135 composed lifecycle checks and 2,915 camera checks pass; the new reset regression fails before correction. Native switch/revert follow and visual restoration pending. | S–M; retain continuous smoothing; native visual confirmation at a later checkpoint. |
| 2 / RC5.1 balance HUD/scoring | Native Manual episode controls HUD lifetime; native/custom trick rows and model combo calculation remain distinct. | Model/row/bank-boundary tests exist. Authoritative native banking is not established by model totals or lingering labels. | M; fix visible problems from retained evidence; native bank/bail confirmation at the combined checkpoint. |
| 3 / RC5.2 native flip catalog | R3+Down+X selects FS Pop Shove-It7/22; R3+Up+X now selects FS 360 Pop Shove-It13/25. Ordinary mappings remain; native14/27 is correctly displayed as Laserflip. | 13,560 flip checks and 140 canonical Trickpedia entries pass. Source identities establish routing, not performed gameplay. Hardflip remains unresolved. | S per proven mapping; verify FS actor/rotation/landing in regular, switch and nollie contexts before accepting it. |
| 3 / RC5.2 grabs/ground tricks | Directional B/B,B, extended grabs, one-footers, No Comply and Boneless reuse existing native systems. Extended grabs reserve R3 only for the captured B-edge modifier; a later unrelated R3 remains native. | All 57 supported grab profiles have reachable selectors. Focused modifier ownership regressions pass; historical actor evidence does not establish exact current hold/release/stance behavior. | S–M; native hold/release smoke for representative variants; unsupported names need exact native contracts. |
| 3 / RC5.2 grinds | Bluntslide X→Y now continues through native 5-0 into Tailslide; X→X continues into Smith. Both use existing measured controls, exact segment and neutral/retired-lease handoff. Y admission and pop-out remain. | 39,998 composed transition checks pass at30/60/120Hz; new route regression fails before implementation. Composed chains and Grind→Flip/Grab→Manual remain native-unverified. | M; preserve final identity/animation evidence separately from intermediate5-0; extend only supported routes. |
| 4 / RC5.3 Casper | Manual→Casper→Casper Flip uses one owned body/board clip; integrated drawn seed experiment defaults off. | Authoring/runtime cancellation tests; physical support, visible placement and score continuity still need exact candidate acceptance. | M; finish current entry/exit/friction before additional flatland poses. |
| 5 / RC5.3 flatland depth | Manual pivots, grind-to-grind depth, Railstand, Truckstand, Anti-Casper, Primo and Pogo follow Casper. | Upstream pose/mount facilities supply visuals, not physical support or contact. | M–L; reuse owned playback and permission-gated plugin APIs; establish physical admission first. |
| 6 / RC6 environment | Wallrides, wallies, wallpushes, stalls/lips, acid drops, spine/hip transfers and gaps follow core acceptance. | No new acceptance claimed. Generalized wall-contact infrastructure is the intended foundation. | M–L; implement a simple supported mechanic after RC5.1 wall routing is verified. |
| 7 / RC7+ | Specials, advanced flatland, skitching, larger combos and challenges. | Deferred behind responsive core and environment. | L; no current implementation action. |

Representative acceptance uses only the scenarios affected by a change; all eight
are consolidated at release-candidate checkpoints:

| Scenario | Required result |
| --- | --- |
| A: Immediate flip | Auto-push → Ollie → immediate X; native flip during ascent, then land. |
| B: Manual | Enter and physically maintain Manual for **at least 3 continuous seconds**, then Ollie out. |
| C: Revert chain | Manual → RT → Nose Manual → Ollie → Flip → land with combo ownership intact. |
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

Portable tests cover transformed curved ramps, independent shape owners, holes,
clearance failures, bounded momentum, trigger timing, ownership cancellation and
query-to-physics races. Native gameplay acceptance remains pending; see
[the implementation evidence](research/vert-transfer-implementation.md).
