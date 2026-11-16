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
| 0: CI admission | Required Windows/Linux source contract | YAML reader aborts before packaging; retain all assertions with block syntax | XS | Required preflight and packaging gates | Existing RC5 fixes reused; 32535ec4 full CI passes |
| 0: Windows compilation | Integrated contact contract included before platform headers | Four masks require undeclared Windows `LONG`; make their existing signed 32-bit type explicit | XS | Header-first compilation and contact-response regression | Reuse RC5 `e9872dc3` as `9345cc0a`; Windows build and package pass at 32535ec4 |
| 1: Rotation/prewind | Existing native LS spin, flight owner and ground chord; RC3 native rotation observations | Short prewind suppresses initial AIR rotation until ground chord interval expires | S | Proved native flight; both shoulders/cadences, no ground yaw, release and switch landing | Implemented; 32535ec4 short-window LB/RB native response observed; repeated switch acceptance pending |
| 2: Input responsiveness / grabs | Existing directional/repeated-B registry and native grab adapter | Second B before first grab admission loses repeated variant; retain physical prefix independently of native readiness | S | Same owner/action/flight, original edge window and deadlines; actual actor/exit/landing | Queued-prefix correction implemented; old-source regression fails; HIL pending |
| 2: Delayed grab recognition | Existing physical journal and native-air cutoffs | Two B edges before client readiness replay singles in reverse order; recognize the pair once | S | Both original edges, 250 ms repeat limit, second-edge +200 ms deadline, no stale replay | Implemented; 32535ec4 Method actor advancement/release/physical landing observed once |
| 2: Repeated-input expiry | Existing request policy and deadline gate | `39ef1c70` stores a repeat deadline but inherits an unbounded ordinary policy; enforce the new repeat's physical-edge window | S | Fresh native observations and unavailable admission through exact second-edge +200 ms | Negative regression reproduces the defect; new repeated paths bounded; ordinary first-request policy preserved |
| 2: Pre-AIR repeated X | The32535ec4 native pair records both edges during released-pop preparation | Two qualified edges become separate singles; retain the original flip prefix | S | Exact released pop and owner, existing160ms preload and second-edge+200ms expiry; actual held actor | Implemented and peer-reviewed; old-source failure and focused sanitizer regressions pass; native double pending |
| 2: Queued double flip | Existing held-flip adapter and queue eligibility | Second X before first admission becomes another single and can be discarded | S | Kickflip/Heelflip actor, native preparation window, same flight and original physical deadline | Queued-prefix correction implemented; triples remain deferred; exact-source HIL pending |
| 3: Manuals/reverts | Analog reversal/diagonal/neutral recognizer, landing acquisition and native episode; historical 10-second contact/stance captures | Exact unified lifetime, immediate landing acquisition and chains not accepted | S verification; M only if native defect found | Sequences A/B/G, contact/stance/balance plus score, both stances | Existing model regressions; no speculative physical change |
| 4: Camera | Shortest yaw, rigid orbit, native pivot, elapsed duplicate sampling and restoration already fixed | Reverse restart remains inconclusive; exact candidate switch/revert follow pending | S verification | Sequence C and OFF, root/motion unchanged | Focused production suites pass; native walkthrough pending |
| 5: Ollie/movement | Native release, charge, launch descriptors and bounded push budget already implemented | No new source-proved tuning defect; short/held jumps and momentum need comparative evidence | S verification / M tuning | Native cadence, trajectory, catch/contact, collision/bail controls | Preserve coherent current parameters; measure before tuning |
| 6: HUD/scoring | Native-plus-custom combo model, integer ring and retained native stack entries | 39ef1c70 rendered Casper/Casper Flip names and x10 in the separate RC5 HUD session | S verification / M if binding defect remains | Sequence F, actual rendered names, banking/bail/OFF; RC5 chat owns HUD investigation | Native rendered names and OFF cleanup observed on 39ef1c70; integrated 32535ec4 full chain pending |
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
the corrected queued-prefix and delayed-pair recognition: 20,730 grab checks, 9,174 dispatch
checks, 6,517 session checks, 648 takeoff cases and 326 jump-idle checks pass with
address/undefined sanitizers and warnings as errors. Independent review added
clock-reset and native-retirement cases; only the existing native grab lease can
carry a queued repeat through the latter. These prove software behavior, not
native trick completion or feel. Gameplay commits are `78796a71` (prewind) and
`ce7f8af3` (queued grabs).

The first grab correction covers an already queued physical prefix. A second
regression reproduces two B edges before client recognition becomes available:
the old matcher dispatched the newest single grab, then replayed the older one.
The existing journal now recognizes that pair as one requested native variant.
The physical interval stays at most 250 ms and the final edge retains its 200 ms
readiness deadline. Neither a queued first grab nor a matched pair counts as
native execution. A consumed sequence floor prevents stale replay through a
released-pop proof; the final B retains its own release or held state.

A later exact-boundary regression found that `39ef1c70` recorded the repeated-B
deadline while retaining the ordinary policy's unbounded flag. Fresh native
observations could therefore keep a waiting repeat alive past that deadline.
Both new grab-repeat paths now opt into the existing bounded request policy,
with expiry anchored to the second physical edge. The original single request's
policy remains unchanged. The frozen `39ef1c70` build is restricted to the
independent HUD investigation; it is not input or gameplay accepted.

The same review reproduced a queued X-to-X defect: before the first flip's native
admission, the second X became another single and was lost on selector retirement.
Recognition now reuses the queue's existing eligibility predicate and the pending
Kickflip/Heelflip prefix. FIFO dispatch and the original held-flip adapter still
require the actual base actor, valid preparation window and independent rotation
evidence. The new repeat expires at its physical edge plus 200 ms. No triple-flip
execution or native success is inferred from recognition.

The final focused batch passes 1,504 held-flip checks, 20,740 native-grab checks,
9,174 dispatch checks and the existing composed session suites with sanitizers
and warnings as errors. Old-source overlays fail both the queued-X and fresh-state
grab-expiry regressions. Positive +199 ms and negative +200 ms cases cover both
new grab paths and queued doubles. Native observations in these tests are fixtures.
Correction commit `39a37647` also passes the full 46/46 targeted sanitizer graph.

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
new hardware slot. The exact 32535ec4 hardware observations below are narrower than full acceptance.

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

Exact compiled candidate and limited native observations are recorded below.
It is an installable experimental checkpoint, not accepted for owner gameplay.

The first feature run, [43794704338](https://github.com/notPho-Real/ReSkate/actions/runs/43794704338),
was cancelled after the baseline compiler failure became known, avoiding another
known-failing runtime build. Its passing Linux checks do not qualify a package.
Baseline run 43791688729 passed 132 upstream integration tests but produced no
valid DLL or installer. The corrected full checkpoint includes the header fix
and both repeated-grab timing corrections.

Checkpoint `39ef1c70` runs in
[43795662044](https://github.com/notPho-Real/ReSkate/actions/runs/43795662044).
Its Windows preflight passed 140/140 tests; runtime compilation and packaging
are separate gates. The subsequent bounded-repeat correction supersedes it for
input acceptance. The RC5 chat may use its verified binary for the independent
HUD question, then must stop, restore the exact prior installation and return
the exclusive slot before this thread's corrected-candidate gameplay trials.


## Compiled and native-observed checkpoint: 32535ec4

Exact source `32535ec46d175fd7b67158048f0870b12db57d00` passed
[CI 43797965962](https://github.com/notPho-Real/ReSkate/actions/runs/43797965962):
140 Windows preflight tests, 132 upstream integration tests, runtime compilation,
package and provenance gates. Release publication was skipped. The same-run
[experimental Deck artifact](https://github.com/notPho-Real/ReSkate/actions/runs/43797965962/artifacts/14196327909)
and [runtime artifact](https://github.com/notPho-Real/ReSkate/actions/runs/43797965962/artifacts/14195809799)
contain DLL SHA-256
`f513b29bf468bfc05c4362f806fe9a04db2ed133bc03de52c23e34960c236378`.
The installer archive hash is
`ddef21a484d92e8f2692858c88a7f8297a5dec80e4eb33879ff922ec6b15e01e`.
Local verified packages and receipts remain outside tracked source at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-bounded-ready-32535ec4`.

The bounded exclusive session used the exact loaded DLL, pinned game build and
native-verified `skate3_megapark`. Captures and original failures remain at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-bounded-hil-32535ec4`.

| Trial | Physical/native evidence | Classification and limitation |
| --- | --- | --- |
| Neutral Ollie | Native 100→103→200→201→100; about 1.114 m rise, stable heading and four wheel contacts on return | Observed once at 59.34 Hz; score authority not independently verified |
| Original LB prewind | AIR rotation followed shoulder input; 226 ms shoulder lead | Outside the corrected short window; retain as control, not regression proof |
| Short-window LB | 57 ms shoulder lead; native rotation input 3 ms after AIR, no ground rotation input | Physical turn/release/landing observed once at 59.83 Hz |
| Short-window RB | 54 ms shoulder lead; native rotation input 2 ms after AIR, no ground rotation input | Opposite physical turn/release/landing observed once at 59.57 Hz |
| Rapid X pair | Both physical edges recognized 68 ms apart in GROUND; one ordinary Kickflip actor and about one physical board turn; landed with four contacts | Double did not execute: both edges during released-pop preparation became singles. Source correction required; no double acceptance |
| Rapid B pair | One history-pair Method request 161 ms after second edge, inside its 200 ms deadline; exact native Method loop initialized and advanced; acknowledged airborne release and physical landing | Observed once; Into/Exit actor identities and authoritative bank total remain unverified |

All trials retained one local owner, native/cached step near 1/60 and no sampled
bail. Global capture receipts are incomplete (queue drops); the short-window
shoulder-to-landing intervals separately have contiguous ordinals and consecutive
native callbacks. Do not relabel those global receipts complete. The grab trial
dropped one capture record and the flip trial seven. Single attempts are native
observations, not repeatability acceptance. HUD points and native success callbacks
are recorded, but score observer ownership was not authoritative.

Both short-window spin trials initially landed fakie, then the native fakie flag
cleared about 1.3 seconds later while travel remained backward at roughly 5 m/s.
This preceded Arcade OFF. A second native log corroborated the flag change;
leading-foot/basis flags changed shortly afterward. Source review found no
Arcade stance writer explaining it. Native stance transition remains an open
question, not a demonstrated steering defect or justification to force stance.

The separate RC5 session on `39ef1c70` rendered Casper, Casper Flip and integer x10
with native custom events and OFF row cleanup. Its receipts are retained at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc5-native-ui-hil-39ef1c70`.
This supports HUD rendering on that DLL; it does not certify full flatland or
integrated 32535ec4 acceptance.

Session cleanup normally stopped both game and launcher, removed only the four
exact-hash generated score-pack files absent before the session, restored prior
source `971c16b00ff807725ef6fffe64718394da2742ef` and exact settings/receipt,
and preserved stock/game files and all 129 rollback sets. Returned receipt
`hardware-returned.json` SHA-256 is
`06a34473837065e3d700f133cbcfb790e1ea41da79e416ebd28c198387c8bcc3`;
restoration receipt SHA-256 is
`c2be3b8f9a1d0a7545c7d60bde6ea3004c4ca75fd954a4d22fe750b7fdcc6d2e`.
Transport authority is revoked, with no pending native operations. The accepted
updater and historical baseline branches remain unchanged.

Next independent work: correct qualified pre-AIR repeated-X recognition with
negative deadline/ownership controls; establish native double timing without
manufacturing completion; repeat short-window/grab trials; finish Manual/revert,
rail, wallplant and full flatland A–H acceptance. The stance transition needs a
native normal-control comparison before any movement correction. Environmental
mechanics remain behind these foundational gates.


### Qualified ground-preparation repeat correction

The same qualified released-pop prefix now recognizes a second physical X while
native takeoff is still preparing on the ground. This is recognition only;
execution still requires native AIR, first-flip admission, the matching held
actor and preparation window. The existing 160 ms preload and second-edge plus
200 ms deadline remain unchanged. The ordinary first-flip ground exception
cannot make the repeated child unbounded. Focus, owner, stance, selection,
landing, bail and new-pop cancellation remain in the existing input engine.

The retained native timing uses a single monotonic clock: second kernel X edge
`1547599259891192` ns to first sampled native base flip
`1547599446208901` ns is about 186.318 ms. This is a sampled upper bound,
not an exact submission timestamp or double execution. Scheduled offsets and
Windows game milliseconds have different origins and cannot be subtracted to
establish a missed deadline. No timing expansion is included.

The new composed regression covers two edges during native ground100 or
preparation103 at30/60/120Hz, both stances and three directional cases. Positive
+199ms, negative +200/+222ms and cancellation controls preserve the boundary.
This follow-up source requires its own compiled candidate and exact-DLL native
repeat trial;32535ec4 remains the earlier packaged checkpoint.

The follow-up focused sanitizer and warnings-as-errors checks pass: 1,685 held
flip, 20,740 grab, 9,174 dispatch and 6,517 session checks, plus 648 takeoff and
326 jump-idle cases. The old source fails the new two-pre-AIR-edge regression.
Independent review found no concrete defect. Receipts are retained at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-preair-input-checks`.

Correction commit `c0093d513bf7949e1482dfd05c24df71cedc43ab` also passes
46/46 rebuilt targeted sanitizer suites (57.69 seconds), including shared input,
Manual, spin, rail, wallplant enabled/disabled and Casper consumers. JUnit receipt:
`/Volumes/ReSkateSSD/ReSkateWork/builds/worktrees-rc4-work/rc51-targeted/rc51-preair-targeted.xml`.
The next runtime build is necessary to test native held-flip execution through
the corrected ground-preparation grammar; existing DLLs contain the known defect.
