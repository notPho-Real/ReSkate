# Unified Arcade release candidate 1

This is the first owner-test integration candidate. CI and composed tests establish build and source readiness, not gameplay acceptance. Do not install automatically, synchronize original upstream, advance arcade-mode, or open an upstream PR. Phase 2 begins only after explicit owner acceptance of the staged RC1 source and DLL identity.

## Exact inputs

| Role | Branch | Source SHA |
| --- | --- | --- |
| Gameplay-verified baseline | arcade-stage-controller-ownership | 65d34a0bc06034125b28779d10490e479df37fba |
| Manuals | arcade-stage-manual-episode-lifetime | 566020aeade0c2e6013522dafff1539ebaf06ffd |
| Flips and grabs | arcade-stage-flips-grabs | 5bd2593613424a52a440c16d1f454ab06c9c0947 |
| Grinds | arcade-stage-grind-transitions | 5d1b9ef72e0845be8db3283faf82d5f936919fb9 |

All three feature tips are merged with their history. The grab tip includes the diagnostic report after production 5387765; grind production is 69d1b62 with subsequent research and diagnostic work retained. Feature branches and the verified baseline are not reset or rewritten. The earlier upstream-integration branch is reference material only and is not merged into this candidate.

## Manual readiness

Keep the existing ManualEpisode architecture. The final demonstrated source blocker was a 1,500 ms revert request deadline applied after original native 101 entry. A live inverse revert lasted 1,698 ms and lost the manual before the observed return to 100. Commit 566020a retires initial expiry only after original native entry; current-owner/freshness checks remain, and only a consecutive original return starts successor acquisition. A missing observation cannot fabricate acknowledgment or physics integration.

The candidate also includes grounded/airborne acquisition, independent contact and HUD continuity, true-exit handling, analog cone-entry qualification and the measured opposite-pressure handoff. Composed tests cover Manual and Nose Manual ten-second holds, both revert directions and repeated chains at 30/60/120 Hz, including long native transitions and observation interruptions. The final timeout correction remains live-unverified. Failed native admission, an unobserved return and a real balance endpoint remain distinct outcomes; unsupported handoffs fail conservatively rather than inventing success.

## Integration decisions

- Preserve manual-owned RS during an admitted ground revert and retain native-grab trigger delivery only under its separate airborne admission. Neither is converted into the other's action owner.
- Keep ordinary flip admission and native animation logic. The grab branch adds its existing before-transition callback and exclusive active-grab guard to the shared bridge; the native flip recipe, PTM implementation and Ollie velocity writer are unchanged.
- Keep the grind branch's separate rail release lease and original from-grind authored input. It never receives an ordinary ground launch velocity ticket. Preserve the manual episode identifier-returning API rather than restoring the grind branch's obsolete boolean manual wrapper.
- Publish original native evidence through the rail-aware helper, then run the existing manual transition observer. Retain all manual reset/exit state beside rail cancellation.
- Keep both native-grab and native-grind startup/tick/cancellation paths, and all build source files. Arcade OFF retains the actual production controller/session reset path.
- Keep existing first-grind HUD and combo logic. Optional physics/animation identity discovery remains independent of balance admission.
- Union the Windows test lists and portable targets, including native grabs, rail pop, grind lifetime and grind changes. Adapt generated fixtures to the combined facade signature and exact production publication helper; no assertions or rollback controls are removed.

No feature commits, research reports or tracked diagnostics are excluded. Only the obsolete manual facade declarations are superseded by the newer manual branch implementation. Ignored local hardware runners, proprietary binary assets and raw captures are not source deliverables.

## Supported scope and limits

Grabs retain the three narrow native pilots: Basic BS, Melon and Method, physical repeated-press timing, delayed native-air recognition and held-shoulder trigger coexistence. Their original actors have measured 30 Hz evidence. Native scoring ownership, the intermittent owner failure, sustained hold, exact early state200 timing and broad higher-rate/chaining acceptance remain incomplete. The catalog audit enables no additional grabs, underflips or held-flip variants.

Grinds retain rail A admission, native early-flip routing, independent physics/animation observations and the narrow qualified control recipes. Named destinations include 50-50, Nosegrind, Crooked, 5-0, Bluntslide, Noseslide, Nosebluntslide, Tailslide and Smith only from the documented native source/geometry contexts. Repeated preparations require independently observed intermediate completion/contact and a fresh lease. Unsupported sources, Smith returns, unqualified slide returns and arbitrary repeated combinations remain rejected. Constituent native physics evidence does not establish integrated animation, HUD, combo or production A-to-flip gameplay acceptance.

No prewind development or catalog expansion is included. Protected controls to verify in owner testing: ascent Kickflip/Pop Shove-it animation and upward trajectory; ground X/B masks; left-stick auto-push; ordinary ground RT; camera/board ownership; first-grind balance HUD; Arcade OFF restoration.

## Verification and staging

The initial merged macOS source checks passed 557 native-grab, 9,014 protected flip-release, 1,362 rail-pop, 651 native-grind lifetime, 35,671 grind-change, 40,190 grind-observer, 2,981,393 input-contract, 19,783 handoff and 5,000 ground-control checks. Controller-publication tests run the actual extracted statements. All 18 manual composition and negative-control programs pass, including 677,559 episode, 686,760 final-consumer and 736,359 HUD checks.

Windows runtime, required Windows tests, Linux/macOS lab checks and Deck packaging must complete for the exact final integration revision before staging. Record their URLs, exact source SHA, DLL hash and artifact verification in the accompanying staged build manifest. Report skipped and pre-existing failing broader ad hoc suites separately; never count them as passes.

Rollback: source 65d34a0bc06034125b28779d10490e479df37fba, gameplay-verified native-flip DLL SHA-256 0949252c47746e881ef98af78164674aeda5fb96f122cd596bdc63ef568f42a1. Retain that artifact separately. No installed Deck identity is assumed or changed by this integration.

After staging, stop at owner acceptance gate 1. Feedback fixes remain on arcade-stage-unified-rc1 with numbered candidates. Original upstream is fetched and merged only after that explicit acceptance; RC2 then starts from the approved RC1 checkpoint.
