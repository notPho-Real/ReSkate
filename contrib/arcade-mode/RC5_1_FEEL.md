# RC5.1 feel checkpoint

This candidate is not hardware accepted. Development stays on
`arcade-stage-rc51-feel`; the accepted updater and historical builds are unchanged.

## Baseline and provenance

Refreshed both remotes before implementation. Observed heads: unified `dev`
`713249fa`, historical `arcade-mode` `a7548cf5`, `wallplants` `fcdde7ff`, alternate
wallplant `dce38282`, fork `main` `f4a0b7ec`, official upstream `360e62a8`.
The unified candidate still integrates official `355bf71e`; this feel pass does
not silently merge newer upstream gameplay. The previous worktree's local
`wallplants` commit `d699fd5e` remains preserved on its branch.

Baseline [CI run 43791688729](https://github.com/notPho-Real/ReSkate/actions/runs/43791688729)
failed the same settings-workflow source check on Linux and Windows: a single-line
YAML assumption no longer matched the multiline candidate configure command.
Windows passed the other 139 preflight tests. Reuse the concurrently completed
RC5 fixes `8d5a184a` and `1bbb02f0`, including fast-suite coverage; do not duplicate
their implementation. Baseline runtime compilation and packaging remain distinct
from those preflight results.

## Prioritized gap and effort matrix

Order considers player impact, existing implementation reuse, cost, regression
risk, native evidence and independent verification. Existing implementation is
not a request to repeat its discovery. Native evidence below is historical unless
explicitly tied to this candidate's exact DLL.

| Priority / feature | Current implementation and evidence | Defect / expected improvement | Effort | Dependencies / acceptance | State / next action |
| --- | --- | --- | --- | --- | --- |
| 0: CI admission | Required Windows/Linux source contract | YAML reader aborts before packaging; retain all assertions with block syntax | XS | Required preflight and packaging gates | Existing RC5 fixes reused; run full candidate CI |
| 1: Rotation/prewind | Existing native LS spin, flight owner and ground chord; RC3 native rotation observations | Short prewind suppresses initial AIR rotation until ground chord interval expires | S | Proved native flight; both shoulders/cadences, no ground yaw, release and switch landing | Immediate AIR handoff implemented; old-source regression fails; HIL pending |
| 2: Input responsiveness / grabs | Existing directional/repeated-B registry and native grab adapter | Second B before first grab admission loses repeated variant; retain physical prefix independently of native readiness | S | Same owner/action/flight, original edge window and deadlines; actual actor/exit/landing | Queued-prefix correction implemented; old-source regression fails; HIL pending |
| 3: Manuals/reverts | Analog reversal/diagonal/neutral recognizer, landing acquisition and native episode; historical 10-second contact/stance captures | Exact unified lifetime, immediate landing acquisition and chains not accepted | S verification; M only if native defect found | Sequences A/B/G, contact/stance/balance plus score, both stances | Existing model regressions; no speculative physical change |
| 4: Camera | Shortest yaw, rigid orbit, native pivot, elapsed duplicate sampling and restoration already fixed | Reverse restart remains inconclusive; exact candidate switch/revert follow pending | S verification | Sequence C and OFF, root/motion unchanged | Focused production suites pass; native walkthrough pending |
| 5: Ollie/movement | Native release, charge, launch descriptors and bounded push budget already implemented | No new source-proved tuning defect; short/held jumps and momentum need comparative evidence | S verification / M tuning | Native cadence, trajectory, catch/contact, collision/bail controls | Preserve coherent current parameters; measure before tuning |
| 6: HUD/scoring | Native-plus-custom combo model, integer ring and retained native stack entries | Prior candidate rendered x10 but not Casper names; latest row retention fix unobserved | S verification / M if binding defect remains | Sequence F, actual rendered names, banking/bail/OFF; RC5 chat owns HUD investigation | Preserve existing fix; coordinate exact-source acceptance |
| 7: Flips | Canonical directional/repeated-X selectors; native early/double flip observations | Complete catalog identity, triples and rail followups not accepted | S verified bindings / M unverified actors | Actual actor, board rotation, catch, land and score; sequences D/G | Existing early Ollie/Nollie/Quick Ollie routes retained; expand only from evidence |
| 8: Grinds | Existing native geometry admission, rail-pop lease and grind-change executor | Full grind-change and pop-to-Manual matrix incomplete | S verification / M transition fixes | Sequence D, rail prerequisite and exact identities; no snapping | Native short/held exits historically observed; exact candidate pending |
| 9: Flatland | Casper/Flip, native Manual support, authored playback and surface resistance | Final rendered names/contact clearance and full chain incomplete | S verification / M additional balance transitions | Sequence F, render acknowledgement plus native support and clean terminals | Stabilize existing mechanic before Railstand/Truckstand |
| 10: Wallplants | Alternate RC4 source integrated; independent historical straight/angled hold/rebound evidence | Merged DLL unaccepted; must not block independent feel fixes | S verification / M native failures | Sequence E/H with actual outward/upward motion and no second launch | Keep disabled build regression and historical pre-wallplant package |
| 11: Environmental mechanics | Native contact/query infrastructure available | Wallride/wallie/lip/stall implementation absent | M/L | Core gates, tangential/contact ownership and one playable slice | Deferred until Phase 0 and existing catalog reliable |

## Verification and native acceptance

The short-prewind regression fails against the original rotation header. The
fixed source passes 369 native spin composition checks and seven related
sanitizer suites, including Manual throw, balance entry and lifetime. The grab
regression likewise fails against the original dispatch header and passes with
the corrected queued-prefix recognition: 20,641 grab checks, 9,174 dispatch
checks, 6,517 session checks, 648 takeoff cases and 326 jump-idle checks pass with
address/undefined sanitizers and warnings as errors. Independent review added
clock-reset and native-retirement cases; only the existing native grab lease can
carry a queued repeat through the latter. These prove software behavior, not
native trick completion or feel. Gameplay commits are `78796a71` (prewind) and
`ce7f8af3` (queued grabs).

The grab correction begins once the first B has entered the bounded recognition
queue. Two presses both occurring before recognition becomes available are still
a separate outstanding timing case. Do not describe this as blanket support for
all pre-readiness double taps.

No additional camera correction was justified by the source review. Its
production tests cover reversal, pivot/orbit, cadence-normalized recentering,
duplicate callbacks, invalidation and restoration. Source review used current
official upstream, the existing pinned method survey and native observations.
The focused camera target passes 2,915 checks and the production lifecycle
composition passes 21/21 cases with sanitizers and warnings as errors.
The complete existing targeted graph passes 46/46 with address/undefined
sanitizers, including enabled and disabled wallplants and Casper playback.
An exploratory global Clang warning expansion hit existing unused helpers and
an aggregate initializer outside this change; the broad run uses its repository
warning policy. Windows compilation must still satisfy the required `/W4 /WX`.

Native testing requires the current exclusive hardware handoff, device and game
identity, `skate3_megapark`, exact source/DLL hash, installed receipt and rollback
inventory before installation. A returned/revoked historical receipt is not a
new hardware slot. No hardware result is claimed for this checkpoint.

Run sequences A–H on the same packaged DLL, retaining failed attempts. For this
delta specifically, compare short LB/RB prewinds across measured cadences, both
stances, release/switch landing and no ground spin; compare directional B→B
before/during native readiness against single B, held B, late B and owner/focus/
landing withdrawal. Require actual native actor and motion advancement, exit,
landing and score evidence. Requested actions and screenshots alone cannot pass.

## Installation and rollback

Install only the same-run experimental Deck artifact after required CI passes.
Verify `candidate.json`, `commit.txt` and DLL SHA-256 together. Preserve the prior
DLL, launcher settings and receipt; close game and launcher before transactional
installation. At session end stop both and restore the prior installation unless
the owner explicitly continues that session. Verify restored hashes and retain
all stock/RC2/RC3 rollback sets. Do not change the accepted update channel.

Exact candidate commit, CI results, artifact identity and any native results are
recorded at the next checkpoint; until then this is implemented source under
validation, not a playable or accepted milestone.
