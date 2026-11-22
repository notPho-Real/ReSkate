# Arcade gameplay roadmap

Current work: `arcade-stage-rc51-core-20261122`, based on integrated
`6d8dda8f9431aef21268c74455a5f1ec930df8d8` (official upstream `8515e164`).
Supported game: **20260929**. Experimental only; the accepted channel is unchanged.
The previous checkpoint `2e1f806e` passed all nine required gates in
[CI 44989104909](https://github.com/notPho-Real/ReSkate/actions/runs/44989104909),
with 141 Windows preflight and 136 integration tests and a verified experimental
installer. The current batch corrects the fixed PS2 trigger controls, preserves
Nollie launch identity, measures physical Pivot evidence and integrates the
separate vert controller. Previous CI does not compile or accept these changes.

Implementation, compilation, offline tests, native observation and owner acceptance
are separate states. Historical captures support only their recorded source and
conditions. No current feature has blanket owner acceptance from this checkpoint.

| Priority / milestone | Expected play and current implementation | Verification / remaining issue | Complexity; next action |
| --- | --- | --- | --- |
| 0 / RC5 integration | Existing controller ownership, animations, Casper, wallplants, scoring, Trickpedia, camera and OFF restoration retained with upstream pose/mount facilities. | Base compiled and CI passed. Exact integrated native walkthrough remains incomplete. | M; consolidated native regression after source corrections, with exclusive runner and rollback. |
| 1 / RC5.1 immediate/repeated flips | Immediate A→X recognizes the physical edge through the admitted pop. Ordinary flips share the owner-validated transient-zero reread. Neutral third X now extends the owned double into Triple Kickflip; directional triples remain. | 2,635 held-flip sanitizer checks pass with physical parent/deadline, catch and fourth-input negatives; old source fails neutral-third regression. Current ascent and triple rotations remain native-unverified. | S; native ascent/rapid repeat at a later checkpoint; continue independent offline work first. |
| 1 / RC5.1 fixed ground controls | LT tap selects native Nollie16; hold selects Pressure intent without an unrelated actor. A retains its original timing through setup changes; launch identity survives LT release. RT ground dispatch is exclusively Switch, never Revert. | 370 setup and 133 session sanitizer checks pass. Exact native Switch processor is B double-tap; adapter integration is pending. Pressure actor is unsupported. No native acceptance claimed. | M; finish verified native Switch adapter, then combined stance/launch smoke. |
| 1 / RC5.1 Manuals/Pivots | Established Manual RT retains its original150ms publication lease and requests physical Pivot; unconfirmed balance cannot borrow later authority. Physical board-reference/hips rotation plus native successor support independently determine evidence. | Composed lifecycle checks pass in release and DEV, including true180 fixtures and label/fakie-only negatives. Bounded DEV telemetry records independent physical evidence. Six-second hold simulation remains; native3s hold, Pivot stance/support and full chain pending. | M; complete integrated runtime build, then representative physical chain acceptance. |
| 1 / RC5.1 general wallplants | All-category wall/floor patches span independently authenticated static bodies. Each piece retains its own owner/shape lifetime and facing native normal; center contact remains exact. | 55,816 geometry and1,451 runtime checks pass across transformed partitions, categories, mixed owner forms, corners, obstructions and invalidation. Same-incarnation editor relocation has no transform stamp, including center; no universal map claim. | M; preserve static snapshot scope; native query/hold smoke at a later checkpoint, after independent offline work. |
| 2 / RC5.1 movement/rotation/camera | Auto-push, native steering, Ollie, prewind, aerial rotation and camera orbit/recenter already implemented. New camera leases reset follow heading immediately after OFF/ON or controller/native-owner changes. | 691,135 composed lifecycle checks and 2,915 camera checks pass; the new reset regression fails before correction. Native switch/revert follow and visual restoration pending. | S–M; retain continuous smoothing; native visual confirmation at a later checkpoint. |
| 2 / RC5.1 balance HUD/scoring | Native Manual episode controls HUD lifetime; native/custom trick rows and model combo calculation remain distinct. | Model/row/bank-boundary tests exist. Authoritative native banking is not established by model totals or lingering labels. | M; fix visible problems from retained evidence; native bank/bail confirmation at the combined checkpoint. |
| 3 / RC5.2 native flip catalog | R3+Down/Up+X selects frontside shoves. R3+Left+X now selects Front Foot Impossible37, or Nollie Back Foot Impossible39; native card names stay distinct. Ordinary mappings and Laserflip name remain. | 15,799 flip checks,141 canonical entries and native guide restoration pass. Missing exact gestures refuse without substitution. Source identities establish routing, not performed gameplay. Hardflip remains unresolved. | S per proven mapping; verify new actors/rotations/landing before acceptance; reuse exact native catalog for further source work. |
| 3 / RC5.2 grabs/ground tricks | Directional B/B,B, extended grabs, one-footers, No Comply and Boneless reuse existing native systems. Extended grabs reserve R3 only for the captured B-edge modifier; a later unrelated R3 remains native. | All 57 supported grab profiles have reachable selectors. Focused modifier ownership regressions pass; historical actor evidence does not establish exact current hold/release/stance behavior. | S–M; native hold/release smoke for representative variants; unsupported names need exact native contracts. |
| 3 / RC5.2 grinds | Bluntslide X→Y/X→X continues into Tailslide/Smith. Y→B/B→B now continues into Nosegrind/Crooked through separately completed5-0 and50-50 returns, exact segment and retired leases. | 43,922 composed checks pass at30/60/120Hz; old source fails new routes. Qualified horizontal segment≥12m does not guarantee enough remaining travel. Full native chains remain unverified. | M; preserve each completion and final actor evidence; extend only proven native routes. |
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

Portable tests cover transformed curved ramps, independent shape owners, holes,
clearance failures, bounded momentum, trigger timing, ownership cancellation and
query-to-physics races. Native gameplay acceptance remains pending; see
[the implementation evidence](research/vert-transfer-implementation.md).
