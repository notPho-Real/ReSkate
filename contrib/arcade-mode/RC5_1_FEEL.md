# RC5.1 feel checkpoint

This candidate is not hardware accepted. Development is on
`arcade-stage-rc51-core-20261122`, based on integrated `6d8dda8f`.
The accepted updater and historical builds are unchanged. [ROADMAP.md](ROADMAP.md)
tracks current priorities; dated evidence below remains scoped to its recorded source.

## Fixed PS2 ground controls — current correction

The current contract supersedes the older ground RT/Revert and live LT modifier
descriptions below. LT tap selects Nollie; hold selects Pressure. A still owns
crouch and launch. Native Nollie16 and its follow-up identities survive LT release;
unsupported Pressure requests never substitute an ordinary trick. Ground RT
dispatches only native Switch, while established Manual RT requests physical
Pivot/Nose Pivot. Native Switch delivery remains under integration.

The shared [trigger contract](TRIGGER_CONTRACT.md) reserves vert flight and landing
inputs before ordinary ground commands. Native board-reference and hips motion
now provide independent Pivot evidence; label changes or fakie alone do not pass.
The current chain requires physical rotation, resulting stance and qualified
successor support. Manual acceptance remains at least three continuous seconds,
with no gameplay timeout. Current implementation and verification are recorded in
[ROADMAP.md](ROADMAP.md); older captures retain their original meaning.

## Offline gameplay expansion — November 22

Neutral third X now uses the existing owned Triple Kickflip path, superseding
the double-only neutral limitation in the November20 snapshot below. Its own
physical deadline, parent request, actor lifetime and catch rules remain intact.
The guide distinguishes implemented triples from native-accepted execution.
R3+Left+X adds native Front Foot Impossible37, or Nollie Back Foot Impossible39
with LT; ordinary directional mappings and exact-gesture refusal remain.

Wall and floor patches now span independently authenticated static pieces.
Each piece retains its own complete lifetime stamp and must present a native
normal agreeing with the measured plane. Obstructions, missing geometry,
stale ownership and unsuitable normals still reject. Allocation/owner stamps
are not transform stamps: same-incarnation editor relocation remains unproven,
including for the original center body. Bluntslide can also continue through
the measured5-0 and50-50 returns into Nosegrind/Crooked with separate native
completion gates. Exact-candidate native execution remains pending for all
these changes. No Deck operation was needed for this implementation batch.

## Manual RT publication correction — November 22

The composed Manual→RT fixture previously admitted a pending RT only because
input and client publication shared one millisecond. Advancing publication by
just1ms reproduces the lost input. The corrected route retains that edge for
the original native observation's150ms lease only when an independently
established Manual owns the stick. It preserves the exact Manual request,
direction, native generations and physical timestamp; a fresh observation
cannot renew expiry. Current balance proof is still mandatory before execution.
The earlier ground/unconfirmed RT1ms policy is superseded by the contextual
Switch/Pivot contract above. A new A immediately
cancels pending RT while preserving the ordinary Ollie setup and release.

The corrected production-composed lifecycle passes700,263 sanitizer checks,
including same-ms,1ms and one-frame gaps at30/60/120Hz. Session tests pass6,907
checks plus648 takeoff and326 jump-idle checks, including lease-age/expiry,
same-generation Manual replacement, owner loss and duplicate suppression.
Windows integration and native chain acceptance remain separate requirements.

## Manual acceptance checkpoint — November 22

Focused episode tests now explicitly check the three-second minimum, continued
six-second Manual/Nose balance at30/60/120Hz, and duplicate client callbacks that
must not integrate extra physics time. The existing native-interval buffer and
digital balance model are exercised without changing runtime tuning or native
stick routing. These are simulated contact observations, not proof of a physical
hold in the game. Exact-candidate native hold, revert-chain and exit acceptance
remain pending.

The active Manual recipe requires **at least three continuous physically qualified
seconds, followed by an Ollie exit**. Three seconds is a minimum test duration,
never a gameplay timeout. Balanced holds must continue beyond it. Earlier two-,
two-to-three- and ten-second recipes/evidence remain historical; do not reuse a
retained payload without updating its threshold and exit timing under the existing
runner review. No new hardware authority or test prerequisite is implied.

## Native triple follow-on — November 20

Directional third X now extends the already-owned Kickflip/Heelflip held loop to
two extra rotations. It reuses the original native actor and counter, never
writes a rotation or score, and never submits the base bridge twice. Exact
second-request lineage, third-edge deadline, owner/stance/object continuity and
the original200ms entry/1200ms lifetime remain required. Rejected or expired
thirds preserve the double catch; caught doubles cannot reopen and fourth input
cannot renew the hold. Neutral X remains double-only.

The production-correct one-shot bridge regression fails before the change.
Expanded sanitizer cases cover queued, admitted and active held thirds at
30/60/120Hz, both stances, both variants, native catch, cancellation and the
unchanged double dependency boundaries. Root and independent source review
found no blocker. Windows compilation and exact-DLL native triple rotation,
identity, release and landing remain required; retained native source controls
establish the mechanism, not acceptance of this candidate.

The separate Manual setup correction waits for settled non-pushing contact
before its full fifteen-second corridor test. It retains all speed, geometry,
owner, freshness, native brake and transport bounds;45 offline checks pass.
It is an inactive fresh-payload transform, not a new hardware authorization.
Its bounded video covers the complete setup/route/brake; five-second callback
capture remains acquisition-only. Receipts remain on the SSD under
`rc51-responsive-core-20261120/manual-setup`.

## Responsive core corrections — November 20

The recovered wall-clearance package at `aab5517d` passes all nine CI gates and
strict package verification. Two further production input defects are corrected:

- Rapid X presses after the generic160ms preload were not recognized until AIR,
  collapsing a pre-AIR pair into its latest edge. Recognition now follows the
  existing verified released-pop lifetime (strictly under500ms). Native AIR,
  actor admission, original repeat deadlines and physical double execution are
  unchanged. Regression cases span30/60/120Hz, both stances and basic flip variants.
- Manual recognition accepted analog diagonals, but held-flight and landing
  checks required cardinal input. Both now use the existing Manual projection,
  retaining held preparation and reserving vertical movement through acquisition.
  Native contact still admits balance; neutral, shallow steering, opposing input,
  exact D-pad policy, release/repress, owner loss and OFF retain their guards.

Both regressions fail against the preceding production code and pass with the
corrections under ASan/UBSan and warnings as errors. Candidate Windows compilation
and native acceptance remain required. The owner's current Manual acceptance is
at least three continuous physically qualified seconds followed by an Ollie exit,
superseding the historical two-second and ten-second requirements below. Three
seconds is a test minimum, not a gameplay timeout. The trusted status runner currently cannot resolve
`steamdeck.local`; no input, deployment or reuse of expired hardware authority
occurred. Checkpoint receipts and packaging reside in the SSD artifact directory
`rc51-responsive-core-20261120`.

Latest compiled checkpoint: `5ae5778627a7dbe0bf3153eb7b36b471bdd5aa1b`, including
the owner-reported Manual lifetime/endpoints, Casper support and static wall
ownership corrections. All nine required CI gates and strict package verification
pass; exact-candidate native acceptance is pending. Following the owner and
peer handoff, exact5ae is installed and its mapped DLL is verified in
San Vansterdam. Gameplay acceptance remains separate from startup evidence.
The preceding
exact `986fb8c0` DLL completed five scoped physical doubles at 30 Hz and one at
60 Hz, including native airborne release and supported landing. Its zero-phase
recovery path and durable native banking remain unverified. Earlier ae9 failures
are retained. Two exact-f002 corridor attempts stopped on speed and contact
prerequisites; no f002 Manual trial ran. All136 rollback sets are preserved and
the prior installation was restored for that completed session. The exact `36d615ce` corridor and native brake passed once; its
Manual trial was inconclusive because the observer stopped before recording
physical entry. The corrected observer passes 421 offline checks; these are
not native Manual evidence.

## Owner feedback correction — November 16

The owner requested and received exact `f0025f3e` for persistent playtesting.
Its installed DLL hash was verified; the previous `971c16b0` installation and
all136 existing rollback sets were preserved, with a137th owner-playtest backup.
The game was closed normally before installation. Automated hardware authority
was revoked afterward; the candidate remains installed for the owner.
Receipt: `artifacts/rc51-owner-playtest-f0025f3e/owner-handoff.json`.

Current feedback and source work:

| Feature | State | Evidence | Next action |
| --- | --- | --- | --- |
| Casper stance/support | Implemented and compiled; regular/Nose entry observed in San Van | Native admitted Manual/Nose support now matches its actual wheel contact; seed and active playback remain bound to owner, episode and stance | Exact-candidate native stance trials |
| Manual endpoints | Implemented and compiled; native pending | Regular backward lean bails; forward puts down; Nose reverses. Typed HUD label is independent of endpoint direction. Old regression fails, focused sanitizer suites pass | Verify actual native falling, not just a queued bail |
| Manual after plain landing | Implemented and compiled; native pending | Production-composed short-flight landing regression fails before correction; airborne retention no longer grants timeless old pressure on four-wheel landing | Owner replay with physical and HUD evidence |
| Manual without meter | Implemented and compiled; native pending | Fresh independent native history/contact can readmit after proved physical exit. Old production-composed cases fail; stale labels, same failed section, owner change and duplicate frames remain rejected | Replay trick-to-Manual landing with exact DLL |
| Main-map Casper/wallplants | Casper entry observed; wall routing compiled and native pending | No map-name allowlist found. Geometry1631/static-owner48 checks pass; independent static/allocation/entity proof and self exclusion retained | Verify San Vansterdam free skate with exact candidate |
| Manual acceptance duration | At least three continuous physically qualified seconds, then Ollie out; exact-candidate native pending | Historical two-second proposal has173 offline checks; it does not meet the current duration | Update the retained recipe before reuse, preserving entry/exit, freshness, owner/contact/stance, scoring and landing guards |

The November 16 proposal used **two qualified seconds**; this is historical and
is superseded by the current three-second minimum. Existing ten-second recordings and offline
endurance regressions remain historical evidence/tests, not a new hardware
requirement. That recipe retains the15-second overall observation for exit,
landing and banking. Proposal provenance:
`artifacts/rc51-manual-two-second-proposal/proposal-provenance.json`.

A read-only look at the current owner session log sent no gameplay input or
console commands and did not change the installation. It contained no relevant
wallplant/flatland failure diagnostics; main-map causality remains unverified.
The correction set now passes its exact-source Windows and package gates below.
Hardware acceptance remains pending.

Final readmission review also reproduced stale neutral-release suppression after an owned Manual ended. New independently proved native episodes now clear that old release step; the production-composed regression verifies both a visible bar and current actuation eligibility without inventing a new RS request.

Combined offline validation:46/46 shared targeted tests,21/21 production-composed lifecycle/HUD tests,12/12 native-input consumer tests,7,010 standalone rendered HUD checks and285 balance lifecycle checks pass. The clarified trick-to-Manual landing adds48 cases/792 checks covering both prior/current support types, immediate/delayed history and stale-history rejection. These checks preceded the final Windows checkpoint below. Test-only clarification was compiled separately after the21-test checkpoint.

The first combined Windows checkpoint, source `727ef18c664f1b42adccdd08825f185a7f6aa8eb`,
failed [CI43927214085](https://github.com/notPho-Real/ReSkate/actions/runs/43927214085)
while generating the Manual consumer rollback fixtures. The consumer generator
still matched the old fixed idle-flip declaration after the landing fixture
gained an explicit flip selector. The earlier local12-test consumer result
predated that fixture edit and cannot validate the final combination. Preserve
this failure; correct and regenerate the consumer suite before the next build.
No package from this run is accepted. Full failed preflight log SHA-256:
`220a6f8f9e0af62feb735c76b8beefa9edf91705fa4e9ffc6b47c44fc0f6075c`.

The consumer adapter now preserves the supplied flip selector outside its
explicit air-mode override. Fresh generation, compilation and all12 consumer
tests pass, including five rollback controls and six source-correspondence
checks. A24-case matrix exercises both selector paths and their native
selection/crouch fields. CMake also tracks the transitive episode and score
fixture inputs. Gameplay source and required assertions are unchanged by this
CI correction; replacement Windows compilation remains required.

Replacement source `d0cbb854b751594fdf234fb5b9950eb5554c9f0f` compiled the full
Windows preflight and passed139/140 tests in
[CI43930516537](https://github.com/notPho-Real/ReSkate/actions/runs/43930516537).
The remaining `arcade_mode` regression assumed neutral drift toward the regular
Manual's positive endpoint must bail; that endpoint is now the owner's requested
safe putdown direction. Its pending-meter test must explicitly drive the failing
direction and retain the physical-exit assertions. Failed preflight log SHA-256:
`19501726b57a8b7bd51ef00a908274865f2ddbba77c7341fb1e740988ceaa2c0`.

The same run independently failed its required early crash-report capture test:
direct capture, source0/write, `0x8007012b` partial-copy error, with no minidump.
This batch does not change crash-report code. Existing bounded retries still
reported failure; a transient cause is not established. Diagnostic archive
SHA-256 `1e7f7016bf00ef64809fa9d617c9730cc4bb4e1f9bdb4b7605caeb4329abc284`.
Neither failure is waived, and this run supplies no accepted package.

The model regression correction explicitly covers Manual/Nose Manual, both
endpoint directions and30/60/120Hz while retaining pending-meter and physical
exit checks. It also corrects a second old positive-lean bail assumption.
The exact13-source combined model suite passes45,793 checks with strict warnings
and ASan/UBSan; the changed model alone passes34,226 checks with additional
conversion warnings. Both original endpoint failures are retained. No gameplay
code or required CI assertion is weakened by this correction.

The earlier727 runtime job completed clean Windows compilation under the upstream
warning policy and132/132 full integration tests. Its early crash-capture test
also passed. Runtime log SHA-256:
`0fb11aa21db622deb5936deb19fbb098a3a08b7b01eeb0b7363d617bc31c068f`.
This supplies compilation evidence for the gameplay changes, but cannot override
that run's failed preflight or verify a later candidate package.


## Verified owner-feedback package: 5ae57786

Exact source `5ae5778627a7dbe0bf3153eb7b36b471bdd5aa1b` passes
[CI 43934608600](https://github.com/notPho-Real/ReSkate/actions/runs/43934608600):
all nine required gates, 140 Windows preflight tests, clean Windows compilation,
and 132 full integration tests. The repeated early crash-capture gate also
passes. Previous failures remain preserved above; their cause or outcome is not
rewritten by this later success. Accepted-channel publication was skipped.

[Experimental Deck installer](https://github.com/notPho-Real/ReSkate/actions/runs/43934608600/artifacts/14262183361),
[runtime](https://github.com/notPho-Real/ReSkate/actions/runs/43934608600/artifacts/14261363726),
[integration results](https://github.com/notPho-Real/ReSkate/actions/runs/43934608600/artifacts/14262353210).
Strict archive, PE, source provenance, test-result and embedded-installer checks
pass. DLL size: 45,612,544 bytes. DLL SHA-256:
`af4ca57f79139b2bb520d01222293ba3baf626e99e050141c87e7fcd44435063`.
Installer SHA-256:
`680178756e3eaf0dab6fea8185dca514288bb4a80844cc02864cb09088127847`.
Verification receipt SHA-256:
`d4cff5287038ecc7730002a001731d86189e10525c7314bbb0b79384e7212d73`.
Local verified package: `artifacts/rc51-owner-feedback-ready-5ae57786`.

This is implemented, compiled and installed for owner testing. The owner
explicitly granted Deck access, and the other RC5 chat confirmed exclusive
handoff. The update procedure passed73 offline checks. Its initial installation
transport timed out; no automatic reinstall was attempted. Independent read-only
verification confirmed exact5ae, no pending transaction, all137 prior rollback
sets unchanged, and a138th backup containing the exact prior f002 DLL, settings,
receipt and four-file HUD pack. Current settings and HUD files were preserved.
Evidence: `artifacts/rc51-owner-playtest-5ae57786/verify_install_outcome.json`.

A subsequent launch verified the actual mapped DLL hash before and after a
passive observation, the pinned game executable, and stable San Vansterdam
native root/sublevel identity. Five owner-validated physical samples were
collected; the character was on foot with the map open, so they prove neither
Manual nor Casper execution. Evidence: `native_startup.json` in the same folder.
The owner clarified that the main-map failure occurs everywhere in free skate,
not at a specific landmark. Gameplay repeatability and acceptance remain pending.

After an authorized update, rollback requires both game and launcher stopped,
then restoring the exact prior DLL/settings/receipt from the newly verified
`.arcade-rc51-rollback-f0025f3e-before-5ae57786-owner-playtest` backup. Confirm the
recorded hashes and preserve all older rollback sets and the stock backup.
The new backup exists and its prior-file hashes and HUD copy were independently
verified. Preserve all138 rollback sets. Never restore while either process runs.

At this November 16 checkpoint, remaining acceptance included two-second Manual
lifetime (superseded by the current three-second minimum) and
trick-to-Manual meter acquisition, directional native bails/putdowns, all
supported Casper stance/support combinations, and physical wallplant rebound
in San Vansterdam free skate and Mega Park. Main-map causality and durable
native banking remain unverified. The other RC5.1 acceptance gaps below remain
separate from this correction set.

## Scoped exact-candidate city observations — November 16

All receipts below are in `artifacts/rc51-owner-playtest-5ae57786`, using the
exact5ae DLL and stable native San Vansterdam BAM identity at Blue Light Beach.
The owner reported failures everywhere; this spot is one scoped test location.

* `bound_manual_scoped.json`: a correctly routed natural Up→Down gesture
  produced native Manual intent1, selected rear support0x4000, real contacts,
  board pitch and a visible balance bar. Independent review found71 coherent
  rows spanning2.01147009 seconds, maxgap36.98ms, with stable owner/stance.
  Speed was0.00088–0.417m/s: slow/stationary evidence, not moving-hold acceptance.
  `bound_manual_scoped-independent-analysis.json` preserves qualification.
  The regular positive endpoint requested putdown, followed by native intent
  release and four-wheel contact. Sparse frame timing does not prove the exact
  meter disappearance boundary or continuous HUD rendering.
* `casper_city_corrected_guard.json`: after independently established Manual,
  X→Y produced a renderer-acknowledged Casper execution credit, visible Casper
  label/x2 and authored pose. For the first2.046 seconds after trial start,
  72 samples retained native100, Manual intent1, rear support and the same owner.
  `casper_city_corrected_guard-support-review.json` records independent review.
* `casper_city_nose.json`: native Nose Manual intent2/support0x8000 preceded
  Casper execution credit and visible authored pose/label/x2. The positive Nose
  endpoint subsequently produced native balance-bail admission, native100→504,
  sampled bail causes and a rendered physical fall. This is one positive Nose
  entry and one scoped bail observation, not repeatability or all-stance acceptance.

The native physical and cached step were both approximately1/30s. These external
samples do not prove callback continuity or exact collision-board/foot geometry.
Custom execution credit is bounded by actual controlled playback acknowledgement;
model settlement remains explicitly `native_bank_verified=false`. No durable
native bank, Casper Flip, switch/fakie coverage, wallplant rebound, trick-to-Manual
landing bar or moving-hold acceptance is claimed.

Retained setup failures: the startup log had rotated before an initial remount
guard; an unbound virtual controller caused `multiple_connected`; a later
binding observer stopped at log rotation before the gesture. The existing
explicit controller lease resolved routing. A diagnostic-only Casper guard
initially checked the push bit in contact word1 rather than production word0;
retained rows reproduce that false refusal, and four offline positive/negative
checks validate the corrected mapping. No gameplay code or admission requirement
was weakened for these trials. Native Y held1s with Arcade OFF established riding
for setup; Arcade ON and automatic routing were restored afterward.

Final handback: normal close stopped both game and launcher. The independent
`final_install_verify.json` reconfirmed the installed5ae DLL, all138 rollback
sets and unchanged settings/HUD files. `owner-handoff.json` links each receipt
by hash. The candidate remains installed at the owner's request; automated
authority is revoked in `handoff-authority.json` and `hardware-returned.json`.
The other RC5 chat was notified to leave the Deck for owner testing until a
fresh exclusive handoff. These records do not promote the accepted channel.

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

## Historical gap and effort matrix

This records earlier checkpoint decisions. [ROADMAP.md](ROADMAP.md) supersedes its
implementation priorities, including the former deferral of directional triples.

Order considers player impact, existing implementation reuse, cost, regression
risk, native evidence and independent verification. Existing implementation is
not a request to repeat its discovery. Native evidence below is historical unless
explicitly tied to this candidate's exact DLL.

| Priority / feature | Current implementation and evidence | Defect / expected improvement | Effort | Dependencies / acceptance | State / next action |
| --- | --- | --- | --- | --- | --- |
| 0: CI admission | Required Windows/Linux source contract | YAML reader aborts before packaging; retain all assertions with block syntax | XS | Required preflight and packaging gates | Existing RC5 fixes reused; exact ae9c827e passes 140 preflight and 132 upstream integration tests, compilation and package verification |
| 0: Windows compilation | Integrated contact contract included before platform headers | Four masks require undeclared Windows `LONG`; make their existing signed 32-bit type explicit | XS | Header-first compilation and contact-response regression | Reuse RC5 `e9872dc3` as `9345cc0a`; Windows build and package pass at ae9c827e; phase-read source compiled in986fb8c0 |
| 1: Rotation/prewind | Existing native LS spin, flight owner and ground chord; RC3 native rotation observations | Short prewind suppresses initial AIR rotation until ground chord interval expires | S | Proved native flight; both shoulders/cadences, no ground yaw, release and switch landing | Implemented; 32535ec4 short-window LB/RB native response observed; repeated switch acceptance pending |
| 2: Input responsiveness / grabs | Existing directional/repeated-B registry and native grab adapter | Second B before first grab admission loses repeated variant; retain physical prefix independently of native readiness | S | Same owner/action/flight, original edge window and deadlines; actual actor/exit/landing | Queued-prefix correction implemented; old-source regression fails; HIL pending |
| 2: Delayed grab recognition | Existing physical journal and native-air cutoffs | Two B edges before client readiness replay singles in reverse order; recognize the pair once | S | Both original edges, 250 ms repeat limit, second-edge +200 ms deadline, no stale replay | Implemented; 32535ec4 Method actor advancement/release/physical landing observed once |
| 2: Queued grab after pop retirement | Existing released-pop queue proof already preserves the first B | Auxiliary admission rejected its retired action stamp; use the already-qualified current lineage and retain B2 recognition | S | Valid timely AIR, exact owner/pop/stance, native adapter admission and second-edge +200 ms | c038d602 implemented; old-header regression and 46/46 sanitizer suites pass; exact36d615ce full Windows CI/package verified; native tests pending |
| 2: Grab after held-Y rail pop | Existing rail setup retires its initiating Y; exact released-pop and native grab occurrence proofs exist | Raw Y still blocked both native grab permission gates, losing the next B | S implementation; M native harness integration | Original owner/flight, unchanged 2,500 ms bound, fresh Y/repress negatives; repeated native actor/exit/landing | aff7a33b implemented; old-source regression, 46/46 sanitizer and Windows suites, full f002 CI/package pass. Current native guard must bind owned rail segment/contact before held-Y trials |
| 2: Repeated-input expiry | Existing request policy and deadline gate | `39ef1c70` stores a repeat deadline but inherits an unbounded ordinary policy; enforce the new repeat's physical-edge window | S | Fresh native observations and unavailable admission through exact second-edge +200 ms | Negative regression reproduces the defect; new repeated paths bounded; ordinary first-request policy preserved |
| 2: Pre-AIR repeated X | Existing released-pop journal and bounded dependency queue | Preserve physical repeats through late first-action readiness | S | Same owner/pop, immutable first-AIR bounds, actual native actor and board rotation | Exact ae9c827e preserves all three immediate-pair requests beyond 200 ms; physical doubles 2/3 at 30 Hz, with a retained post-request execution failure |
| 2: Queued double flip | Existing held-flip adapter and queue eligibility | Matching native observation can disappear before held actor entry | S | Actual actor/preparation/rotation, current owner and phase; no cached phase or renewed deadline | Source18de7d3a compiled in986fb8c0; scoped doubles5/5 at30Hz and1/1 at60Hz with native release/landing; zero-reread path unobserved, causal effect and full acceptance unproven |
| 3: Manuals/reverts | Analog reversal/diagonal/neutral recognizer, landing acquisition and native episode; historical 10-second contact/stance captures | Exact unified lifetime, immediate landing acquisition and chains not accepted | S observer correction; M only if native defect found | Sequences A/B/G, contact/stance/balance plus score, both stances | Exact36 corridor/native braking passes; shorter-approach Manual is inconclusive after startup backlog/truncation guard rejection. Lossless startup draining passes421 offline checks; current three-second physical hold remains pending |
| 4: Camera | Shortest yaw, rigid orbit, native pivot, elapsed duplicate sampling and restoration already fixed | Reverse restart remains inconclusive; exact candidate switch/revert follow pending | S verification | Sequence C and OFF, root/motion unchanged | Exact 2427231d right/up orbit and neutral recenter observed with complete camera captures and stable body heading; switch/revert follow and OFF restoration remain pending |
| 5: Ollie/movement | Native release, charge, launch descriptors and bounded push budget already implemented | No new source-proved tuning defect; short/held jumps and momentum need comparative evidence | S verification / M tuning | Native cadence, trajectory, catch/contact, collision/bail controls | Preserve coherent current parameters; measure before tuning |
| 6: HUD/scoring | Native-plus-custom combo model, integer ring and retained native stack entries | 39ef1c70 rendered Casper/Casper Flip names and x10 in the separate RC5 HUD session | S verification / M if binding defect remains | Sequence F, actual rendered names, banking/bail/OFF; RC5 chat owns HUD investigation | Native rendered names and OFF cleanup observed on 39ef1c70; integrated 32535ec4 full chain pending |
| 7: Flips | Canonical directional/repeated-X selectors; native early/double flip observations | Complete catalog identity, triples and rail followups not accepted | S verified bindings / M unverified actors | Actual actor, board rotation, catch, land and score; sequences D/G | Existing early Ollie/Nollie/Quick Ollie routes retained; expand only from evidence |
| 7: Third rapid X | Triple mappings exist; native executor and extra-count2 lifecycle are unsupported | Recognition cleanup alone cannot deliver a native triple | M | Existing one-shot native flip lease; queued upgrade and actual extra rotation | 36 production-correct fixture cases preserve the working double; no proven XS/S gameplay regression. Retain audit and defer native expansion |
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
The subsequent 2427231d runtime build below tests the corrected grammar;
earlier DLLs retain the known recognition defect.


## Compiled and native-observed checkpoint: 2427231d

Exact source `2427231d4b562711e8b6aa5cc22df911bdf85401` includes the qualified
ground-preparation repeat correction above. It passed
[CI 43805397400](https://github.com/notPho-Real/ReSkate/actions/runs/43805397400):
140/140 Windows preflight tests, 132/132 upstream integration tests, Windows
runtime compilation, packaging and provenance verification. The same-run
[experimental Deck package](https://github.com/notPho-Real/ReSkate/actions/runs/43805397400/artifacts/14199965915)
and [runtime artifact](https://github.com/notPho-Real/ReSkate/actions/runs/43805397400/artifacts/14199881665)
contain DLL SHA-256
`e97489337e2a6a1195932f2614ceb1b9d8d7dc53f96bf944c9ae2c8df742bd50`
(45,599,232 bytes). Package verification is retained at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-preair-ready-2427231d/verification.json`.
The artifact is experimental; release eligibility remains false and the accepted
update channel is unchanged.

**Session status: restored and returned.** Exact-DLL trials used the pinned game
build and native-verified `skate3_megapark`. Both game and launcher were normally
stopped. The exact prior `971c16b00ff807725ef6fffe64718394da2742ef` DLL, settings
and install receipt were restored; all 130 rollback sets were preserved. Only
the four exact-hash generated score-pack files absent before this session were
removed. Transport authority is revoked with no pending hardware actions.
`hardware-returned.json` SHA-256 is
`a92c5af06f10cb304e4fa8f0b787ec7e2c5a7e0c22d5c602113c5e7834b2687b`;
`restore_prior.json` SHA-256 is
`11dedd0d66bfa9f9f241dd7fa2c3ceb92cf44f8e56d3f5fe9cc8ee780519a8d6`.
Evidence is retained at
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-preair-hil-2427231d`.

| Trial | Physical/native evidence | Classification and limitation |
| --- | --- | --- |
| `flip60_pair1` cold trial | Controller binding revoked with failure 13 (`captured`) before replay; sampled state stayed 100 and no Ollie or flip actor executed | Retained failed harness trial, not a failed native held-flip admission. Reopening Console after binding caused the captured-route withdrawal; see `flip60_pair1-route-failure.json` |
| `flip60_pair2` after helper correction | Approximately −1.994812 physical board turns; matching native base 2 and held 2 actor initialization/positive advancement, extra count 1, release and ground100 return | Physical double observed; one sampled owner, no sampled bail, four selected contact flags at landing. Global capture incomplete: expected 723, drained 600, dropped 123 |
| `flip60_pair3` independent repeat | Approximately −1.989403 physical board turns with the same matching actor, extra-count, release and landing evidence | Second physical double observed under the tested conditions; one sampled owner, no sampled bail, four selected contact flags. Global capture incomplete: expected 717, drained 596, dropped 121 |

The local helper correction removes the early controller arm and combines the
controller rearm with capture in the final Console interval immediately before
replay, after warmup. Its SHA-256 is
`263ff80c1a145c7477ae6fc736b8e3bbea8f614f44c3af3627844cba99484904`;
20 focused offline generator checks passed. This is an external HIL helper hash,
not a source commit or gameplay change. The original failed trial is preserved.

Both corrected trials retain the original 200ms second-edge admission boundary:
the native held-input request followed the second Windows input edge by 168ms
and 172ms, respectively. Independent physical-axis sampling and actual actor
advancement support the two observed doubles. The source-7 capture reached its
224-row cap in each trial; gaps occur after input admission but before held-loop
and landing evidence. Neither receipt is complete, and successful repeated
execution does not establish full timing, stance, scoring or A–H acceptance.
Selected contact reads are not atomic physics snapshots. Native sequence scores
and HUD totals are retained in the evaluations, but authoritative banking remains
unverified. See `flip60_pair2-evaluation.json` and `flip60_pair3-evaluation.json`;
input requests, labels or scalar scores alone do not pass these criteria.

The paired-trial summary is `flip60-pairs2-3-native-summary.json`. A separate
`flip30_pair1-native-summary.json` records 29.99294Hz native cadence, complete
380/380 input capture and held-input admission 196ms after the second edge.
Matching held actor initialization/advancement and extra count 1 were observed.
Its physical board-axis unwrap remains inconclusive: a 74.195ms sampling gap can
alias a complete turn. Do not classify that result as a proven single, invent
the missing turn, or claim a physically verified 30Hz double.

A separate repeat, `flip30_light1`, confirms a physical single at 29.99294Hz:
441 additional bounded body-axis reads include 94 flight samples, approximately
−0.991508 turns and a maximum flight gap of 10.861ms. The original full observer
agrees. The base actor advanced, but no held actor ran and the extra count stayed
zero. Input capture is complete (378/378). Both physical X edges were recognized;
the next native dispatch opportunities straddled the unchanged second-edge plus
200ms deadline (190ms then 224ms). Readiness/FIFO expiry is a source-backed
explanation, not an explicit native cancellation receipt. This failure remains
part of the candidate record. Extra observer scheduling cost was not independently
measured. See `flip30_light1-native-summary.json`; no missing turn is inferred.

The opt-in lightweight observer uses existing validated body pointers, checks
owner/body identity before and after each read, runs for at most four seconds
and caps at 600 rows. Its helper SHA-256 is
`a356a45feee8930d96ca8bba94eba2f904b73b3f3ee182b2e9ddf9cf8d2dd30c`.
Seventeen offline generation/guard checks passed, recorded in
`light-basis-generator-test-results.json`; default payload output is unchanged.

`grab60_pair1` records one history-pair Method request 124ms after the second B
edge. The exact native Method actor initialized and advanced; native airborne
release was acknowledged, followed by ground100 and four selected contacts with
one sampled owner and no sampled bail. Measured input cadence was 59.316Hz.
Its 730-record capture drained 601 and dropped 129 after admission; full capture,
Into/Exit identity and authoritative banking remain unverified. Native previous
sequence 30 and HUD 60/x2 are retained as distinct score observations in
`grab60_pair1-native-summary.json`, not treated as bank proof.

Exact-242 camera trials `camera242_right1.json` and `camera242_up1.json` each
retain complete 178/178 camera captures, with no loss, uninspected receipts,
nesting, truncation, late rows or changed client-drop counters. Right-stick
horizontal input produced about −126.850 degrees of yaw relative to follow,
then returned monotonically to −0.081 degrees after neutral. Up input produced
about −21.559 degrees of pitch, then returned to −0.043 degrees. Each trial
retained one observed input owner/route; all 169/167 qualified Arcade transform
operations returned and the requested orbit matched the published selected
camera pose. No other known writer was recorded during those captures.

Independent body samples show one owner and ground100 throughout both camera
trials, roughly 24.43m/24.12m of travel, and maximum heading change only
0.0122/0.0103 degrees. This supports orbit without appreciable body yaw in these
trials; no matched neutral-input comparison establishes every motion component
unchanged. Three extracted video frames per trial show the orbit/pitch and return
while the skater remains visible. They are not continuous playback review or an
atomic native-pose/render join. Both captures end before cleanup OFF/ON, so they
provide no OFF-restoration acceptance. Detailed evidence is retained in
`camera242_right1-native-analysis.json` and `camera242_up1-native-analysis.json`.

The accompanying [controls documentation](CLASSIC_CONTROLS.md) now describes
immediate neutral-X press recognition, existing directional/repeated-B and R3
bindings, and their native-admission limits. Unsupported named grabs are explicit.
This corrects stale documentation; it adds no actor or control binding and does
not promote historical grab evidence to current lifecycle acceptance.

The next source priority is the measured 30Hz dependent-flip deadline boundary,
with a failing regression before any bounded continuation change. Remaining
independent work includes complete capture coverage and control trials,
additional native cadence/stance coverage,
Manual/revert and rail chains, current grab lifecycle/scoring, camera and OFF
restoration, wallplant continuation and full flatland acceptance. The verified
package makes this correction playable; the broader RC5.1 acceptance gate remains
open. The ten-second Manual trial was withheld because the current −Z corridor
stops/reverses near Z −136 after approximately 36m; historical successful Manual
routes run elsewhere. Qualify a clear corridor before that acceptance test.
Follow the installation/rollback procedure above; this session's exact restore
and returned-slot evidence is preserved.

### Follow-up: one native successor for the queued first-flip dependency

The retained 30Hz failure exposes a dependency between a timely first flip and
its already-recognized child: first-flip admission at second-edge +190ms can
leave the next native dispatch at +224ms, beyond the original +200ms deadline.
The follow-up keeps that deadline unchanged and adds a queue-local, one-shot
allowance only for the exact pre-AIR child already queued before its matching
basic parent was admitted before expiry. Native admission is still required;
the allowance grants no animation, completion or score evidence.

The allowance belongs to the parent's accepted native observation and its
existing 150ms freshness lease. Only its immediate successor physics step can
use it; duplicate polls cannot renew it, skipped steps cancel it, and one failed
successor attempt consumes it. Owner, exact physical parent, pop, object,
corroborated stance, flight and monotonic observation identity must remain valid.
Reentrant queue changes cannot manufacture a preexisting child. Ordinary paths
retain their original timing and do not acquire the new orientation prerequisite.

The old header compiles successfully with the composed regression and then
fails the intended 190→224ms behavior assertion. The correction passes 1,860
held-flip checks, 6,517 session checks, 648 takeoff and 326 jump-idle cases,
9,174 dispatch and 20,740 grab checks with address/undefined sanitizers and
warnings as errors. Independent review found no remaining blocker. Exact
commands, hashes and the failing-old result are retained in
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-held-dependency-checks/regression-results.json`.
Windows compilation and exact-DLL native validation remain pending. The prior
capture does not serialize the precise producer orientation at the dependency
boundary, so that new allowance's live orientation gate remains unverified.

The rebuilt shared-consumer sanitizer graph also passes 46/46 suites in 57.45s,
including both wallplant configurations and existing Casper consumers. Receipt:
`/Volumes/ReSkateSSD/ReSkateWork/builds/worktrees-rc4-work/rc51-targeted/rc51-dependency-targeted.xml`.

### Bounded diagnostic capacity

The incomplete 60Hz source-7 captures reached the intentional 224-record output
cap; they were not evidence of controller input loss. The separate localized
adjustment raises that finite cap to 384. A regression reproduces the measured
347 selected observations across 2,990ms, while the 385th record still makes a
capture incomplete. Existing outstanding-ticket, queue-loss, hook invariance,
compaction and footer checks remain intact; no old receipt is relabeled complete.

The producer retains the same four previous-poll records and existing queues,
with no new hooks or native reads. The change adds zero static storage and at
most 160 formatted records (about 362KB at the largest observed line length).
Extra serialization/logging cost and native timing remain unverified. Evidence
is retained at `/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-capture-cap384`;
the diagnostic improvement is separate from native gameplay acceptance.
The final fixture keeps each native proof unchanged across duplicate polls while
binding publication time advances. It passes 2,656 controller-capture checks,
4,222 telemetry checks and 188 schema-4 checks with warnings-as-errors and
address/undefined sanitizers. An isolated old-cap header compiles and fails the
347-record retention assertion.

Windows targeted compilation in
[CI 43816063989](https://github.com/notPho-Real/ReSkate/actions/runs/43816063989)
rejected checkpoint `576fa623` with C4459: a dependency-loop local named
`pending` hides a wallplant translation-unit variable during template
instantiation. The local is renamed `queued_child`; warning and test policies
remain unchanged. Cancellation of the remaining obsolete jobs was requested after
the failure was preserved. The two directly affected local sanitizer suites pass
again. Full-log review also identified C4244 in the new negative-test fixture:
an integer conditional was passed to a float count parameter. The fixture now
uses explicit float literals, preserving its exact 0/1 cases. Both findings are
corrected before another push. This run supplies no installable or
native-accepted checkpoint.

## Compiled and native-observed checkpoint: 7b23ea2f

Exact source `7b23ea2fa5f972e0060d21b2cb662e79bccff399` passes
[CI 43817475375](https://github.com/notPho-Real/ReSkate/actions/runs/43817475375):
all required jobs, 140 Windows preflight tests, 132 upstream integration tests,
runtime compilation, packaging and provenance. Publication was skipped.
The [experimental Deck artifact](https://github.com/notPho-Real/ReSkate/actions/runs/43817475375/artifacts/14204627935)
and [runtime artifact](https://github.com/notPho-Real/ReSkate/actions/runs/43817475375/artifacts/14204602392)
contain the same 45,605,888-byte DLL, SHA-256
`00a7e69cc494dfbbee12d2c340e9fec50f3a833286f4c2df64570b45975ca6c9`.
The installer embedding, source scripts, archive hashes, PE and test XML were
independently checked. Local package:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-successor-ready-7b23ea2f`;
`verification.json` SHA-256
`d8afaf1b23f39ab9726ecd8d66b2df13aeeb87f872f5abc1a460d4e998a99dd2`.

The exact loaded DLL and native MegaPark descriptor were verified during a
bounded exclusive Deck session. Original receipts and independent analyses are
retained in `/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-successor-hil-7b23ea2f`.

| Trial | Actual cadence / native and physical result | Capture and limitation |
| --- | --- | --- |
| `flip30_pair1` | 30.0107Hz; matching base/held actors, extra count1, approximately1.992 dense physical turns, grounded contact | Complete379/379; held input196ms after second X |
| `flip30_pair2` | 30.0141Hz; matching base/held actors, extra count1, approximately1.991 dense physical turns, grounded contact | Complete377/377; held input188ms after second X |
| `flip30_pair3` | 29.9718Hz; base actor only, approximately0.992 dense physical turns, no held input/actor/count | Complete377/377; intended double failed |
| `flip60_pair1` | 59.3220Hz; matching actors, extra count1, approximately1.991 dense physical turns, grounded contact | Complete720/720;346 selected-poll rows; held input160ms after second X |
| `flip60_pair2` | 59.2829Hz; matching actors, extra count1, approximately1.995 dense physical turns, grounded contact | Complete717/717;343 selected-poll rows; held input172ms after second X |

Each input trial retained one sampled owner, native/cache steps matching its
configured cadence, no observer error, and no sampled bail/offboard phase.
Dense physical sampling had maximum gaps of about10.6–14.2ms; its independent
turn counts agree with the original observer. External observations remain
non-atomic. These results do not independently prove native catch, durable
banking, other stances, or full sequence acceptance.

**30Hz reliability remains failed: two observed doubles and one actual single.**
All successful held admissions occurred before the original200ms deadline.
None establishes native use of the new successor-frame allowance.
For the failed trial, second X was at1558355075 and its deadline at5275.
Complete predeadline records lacked action readiness; the first ready records
appeared at5286/5287. The current allowance requires a timely admitted parent,
so this evidence supports a late-first-readiness gap. Exact parent adapter
submission was not directly logged; do not substitute an inferred timestamp.
The next correction needs one explicit, nonrenewable same-pop contract and
combined timely/late regressions, not a renewed general repeat deadline.

The finite384 cap now has repeated native completeness evidence: the two60Hz
captures retain119–122 selected-poll rows beyond the old224 cap with no recorded
loss. This verifies retention for these trials, not every logging-overhead case
or gameplay acceptance. Original incomplete242 captures remain incomplete.

### Manual-path evidence and test-harness limitations

The first baseline attempt was rejected locally before SSH because its generated
loader exceeded65,536 bytes. No controller input was sent. A separately named
attempt used AST-equivalent source compaction, with the transport limit unchanged;
44 offline checks preserve existing eligible payloads and identical executable
syntax. The original failed payload/start receipt is retained.

`manual90_baseline2` recorded11.09s of ordinary forward movement from the
historical+X start, one owner and stable stance, with native/cache steps1/60.
It stopped at an ambiguous contact boundary: row state100, all four selected
contacts present, but contact state-after0. The observer classified that unknown
phase as contrary support. This is not proof of collision or physical contact
loss. The planned braking segment did not execute; later video shows approach
toward the perimeter wall. The15-second route and stopping room remain unverified.
The10-second Manual was not attempted. Its acquisition capture also remains
incomplete (3060/3101,41 dropped), independently of the sampled motion evidence.

A separate, unapplied observer correction treats unknown0 as unavailable evidence,
never qualified contact. Real contrary phases and the150ms qualification limit
remain enforced;29 offline checks pass. A fresh complete baseline is still
required before the Manual hold. No success is retroactively assigned.

### Restoration and remaining work

Both game and launcher normally closed. The session removed only four exact-hash
generated score-pack files absent before installation, restored prior
`971c16b00ff807725ef6fffe64718394da2742ef` DLL/settings/receipt exactly, and
preserved the stock/game hashes and all131 rollback sets. Transport is revoked
with no pending hardware action.
Returned receipt SHA-256:
`016728671f3d1f324f5e22780dc0f18a39747053afa0fef26ed42e763d8cdf97`.
Restoration receipt SHA-256:
`6cc5bb5b362d4ad72e6cc9d8291a21bf622c30b4f28fae09faee2a9ca30fff53`.

The installable package remains experimental and not accepted for owner gameplay.
Use the same-run installer only with the existing stopped-state preflight and
rollback procedure; the accepted updater is unchanged. The preserved prior
installation is the rollback target, not a speculative replacement from another
branch.

Immediate work is the source-proven late-first-readiness gap, conservative
observer correction, and distinct model-terminal diagnostics. CurrentLine is
sequence history, not bank authority; native-only model terminal events are
missing from the custom-only log. Exposing those events must not change scoring
or claim durable native banking. Remaining independent acceptance includes
Manual/revert lifetime and chains, immediate single grabs and rail followups,
full grab lifecycle/scoring, camera switch/revert/OFF restoration, wallplant
continuation and full flatland sequences. Broader environmental mechanics remain
behind those foundational gates.

## Follow-up: bounded first-AIR readiness and model terminals

Gameplay correction `1d017e95e111b7d386591ff2d501f0ada4fdcfe0` addresses the
retained7b23 late-first case within the existing queue. It copies the producer's
already-established first-AIR timestamp into the input context. Only the exact
pre-AIR physical pair, still queued behind its matching parent on the same pop,
can qualify when the child has expired but has not been cancelled. Both physical
edges must precede first AIR. First AIR must fall inside the existing500ms
released-pop corridor; parent admission and the one immediate successor
opportunity must remain inside the original first-AIR150ms lease. The endpoint
is never anchored to a later poll or acceptance, and the original second-X+200ms
deadline is not rewritten. This late path is strictly shorter than650ms after
the original release. The previously tested timely-parent path is unchanged.

Actual parent `started`, the pre-adapter child snapshot, all native ownership,
orientation, episode, actor and cancellation gates remain mandatory. Duplicate
polls do not renew the allowance; a skipped frame, stale proof, changed first-AIR
timestamp or failed successor retires it. Ordinary airborne repetition keeps its
original timing. This is one bounded dependency contract with timely and late
qualification, not a retry-until-success mechanism.

The old source compiles and fails the composed late-readiness regression.
The corrected code passes2,035 held-flip checks,6,517 session checks,
648 takeoff cases,326 jump-idle cases,9,174 dispatch checks and20,740 grab checks
with ASan/UBSan and warnings as errors. Combined regressions retain the earlier
190→224ms boundary and the new first-AIR1324/base-ready1381/successor1415 case,
with the physical child deadline1370 unchanged. Half-open149/150ms and499/500ms
boundaries, changed timestamps, missing or reentrant parents, context/owner
withdrawal and native-adapter negatives are covered. Independent diff review
found no blocking issue. Receipts:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-first-air-dependency-checks`.
Native success of this correction remains unverified until its exact DLL runs.

Diagnostic correction `20ce768944aa63a62f1037112774d2c790bf9427` reports actual
model terminal sequence, kind, total points, action count and custom subtotal,
including native-only terminals with zero custom points. It reuses the current
one-shot sequence guard and bounded worker queue. Existing custom-score rows and
scoring behavior are unchanged. Every formatted row explicitly states
`model_only=true native_bank_verified=false`; observation time does not claim a
native execution step or durable banking authority.

The exact old production block fails a native-only terminal regression; the new
block passes11 bank/bail, one-shot, mixed-score, formatter and reset checks.
Typed telemetry tests exercise real model bank/bail outcomes, invalid records and
zero-action terminals. Focused ASan/UBSan checks pass4,240 telemetry and2,656
controller checks plus the existing custom-score and camera/storage suites.
The local ABI measurement shows unchanged1,016-byte client variants and the
same64-row queue; no portable ABI guarantee is inferred from that measurement.
Receipt:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-model-settlement-checks/receipt.json`.
These changes are batched for one necessary runtime checkpoint; they do not
retroactively pass the7b23 failed gameplay or Manual baseline.

The rebuilt shared sanitizer graph passes 46/46 suites in 17.57s, including
enabled/disabled wallplants and Casper. JUnit: `rc51-targeted/rc51-first-air-targeted.xml`
under the SSD build directory. The final provenance edit also passes both existing
candidate-manifest tests. Build necessity and exact regression receipts are
retained with the first-AIR task; Windows and native results remain pending.

## Exact ae9c827e compiled checkpoint and uncertain native trial

[CI run 43827670185](https://github.com/notPho-Real/ReSkate/actions/runs/43827670185)
passes all nine required jobs, including Windows compilation, 140 preflight
tests, 132 integration tests, Deck packaging and release eligibility. Publication
was skipped; the experimental candidate remains unaccepted.

The verified DLL is 45,607,936 bytes, SHA-256
`d927dac32067973b96802ec8975394f45d72249590a532292f04d5066fc2ca82`.
Runtime artifact: `14209790623`; Deck installer artifact: `14209242144`.
The local installable package and complete verification receipt are under
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-first-air-ready-ae9c827e`.
Verification receipt SHA-256:
`2e2102c9076a58747dfd9ba8e9ab3eebd13a2456d223336e730d7868515b0da5`.
Archive hashes, PE identity, source provenance, embedded installer DLL and source
scripts all pass. A build pass does not establish native gameplay behavior.

The exclusive Deck session verified the exact mapped DLL and native MegaPark
descriptor before input. Its first original queued-X pair timed out at the
unchanged 80-second transport bound. No finalized trial receipt, full physical
samples or dense board basis returned. The attempt was retained as uncertain
and was not automatically replayed.

A separate read-only recovery acquired the exclusive lock and recovered native
logs. These contain all 383 contiguous capture rows, a complete 383/383 footer
with zero drops, measured 30.0035 Hz native cadence, base and held actor
advancement, and a held request 192 ms after the second X. That timing does not
exercise the new post-deadline allowance. Physical double rotation, catch and
landing cannot be reconstructed from the missing samples. The logs also contain
one model bank terminal: 224 points, two actions, zero custom points, explicitly
`model_only=true native_bank_verified=false`. This observes the new diagnostic;
it does not prove durable native banking.

The native logs span about 26 seconds from controller binding to final ON.
Their wall clock is not independently aligned with the local start marker.
The cause of the transport timeout is unresolved. The
local transport discarded available partial output on timeout; preserving that
output and reducing receipt transfer size are independent harness corrections.
Neither changes gameplay, capture coverage or test deadlines.

Both game and launcher subsequently closed normally. An initial close refused a
transient gamescope focus mismatch before Alt-F4; fresh observation preceded the
successful `close_window` operation. Four exact candidate-generated score-pack
files were removed, and the prior `971c16b0` DLL, settings and install receipt
were restored exactly. All 132 rollback sets, stock DLL and pinned game remain
preserved. Authority is revoked with no pending hardware action.

Session evidence:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-first-air-hil-ae9c827e`.
Returned receipt SHA-256:
`d767aa31f07d11334d10afce2bb743c14e76e184adea333b22936532754d8b97`.
Restoration receipt SHA-256:
`c7d6650867ff0f7509324488c3c65185de0d1939b0c4a4ef6354647bdf2cc171`.
Rollback remains the exact preserved prior installation; use the existing
stopped-state restoration procedure and verify these identities. The accepted
updater and historical builds are unchanged.

The Manual-only observer correction now passes 111 focused checks, plus 76
scaffold, 23 corridor-generator and 29 contact-phase checks. Independent review
confirmed separate Linux arrival/native counter clocks, authenticated native log
rotation, unchanged 450 ms freshness and 700 ms gates, and AST-equivalent loader
compaction below the existing limit. This establishes harness readiness only.
The new west corridor and 10-second physical Manual remain untested. Next work
is reliable receipt delivery, repeated exact-DLL input trials including the
immediate-release pair, then the independent corridor and Manual acceptance.

## ae9c827e repeated native trials with complete physical receipts

The local receipt repair preserves available timeout bytes and uses an opt-in
lossless envelope with strict size, hash and decoding checks. It changes only
terminal serialization, with unchanged gameplay/sampling AST and 80-second
transport bound. All 107 focused checks pass, including retained flip/video
roundtrips and the existing maximum video size. Root and independent review
passed before a fresh exclusive session. This repair does not establish the
cause of the earlier timeout.

The same exact DLL ran the existing immediate A-release/X pair, retaining its
68 ms X spacing. All three new receipts returned successfully:

| Trial | Measured native cadence | Complete capture | X2 to held request | Physical turns, dense / full | Result |
| --- | --- | --- | --- | --- | --- |
| 30 Hz immediate 1 | 30.014124 Hz | 379/379, zero drops | 232 ms | -1.991519 / -1.993671 | Double observed |
| 30 Hz immediate 2 | 30.014124 Hz | 382/382, zero drops | 228 ms | -1.991519 / -1.993393 | Double observed |
| 30 Hz immediate 3 | 29.971791 Hz | 382/382, zero drops | 224 ms | -0.990663 / -0.990663 | Single; reliability failure |

Native and cached steps were 1/30. The two doubles had matching base/held actor
advancement, count/release evidence and corroborated four-contact landings.
Dense sampling found no ambiguity under its recorded bounds. Their readiness
captures are consistent with late first-action readiness, but do not expose the
exact adapter admission instant. A request beyond 200 ms alone cannot identify
which dependency allowance ran. Catch and durable native banking remain separate
unverified claims.

The third trial preserved the physical repeat and issued a held request, then
withdrew it 128 ms later with no usable matching native flip observation. No
held actor initialized or advanced; physical motion was a single. This is an
execution/lifecycle failure after request preservation, not a missed second X.
The exact failing native read is not present in this build's telemetry.
The prepared 60 Hz comparison was not executed after that failure.

The independent west corridor baseline also stopped without passing. At
9.5046 seconds, its request block disagreed with separate reads of the same
basis/orientation fields. The row retained four stable support contacts, valid
pose, flat board and continuous motion; adjacent request masks were normal.
This supports a mixed observation boundary, not a demonstrated collision.
The 15-second route and planned native brake did not complete, and the later
OFF sample still moved at 5.6186 m/s. The 10-second Manual was withheld.
Its acquisition capture was separately incomplete: 3092/3101 rows, nine drops.
A separate unapplied observer proposal has 35 regressions for bounded unknown
evidence versus coherent contrary state; it does not retroactively pass this run.

Evidence is under
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-first-air-hil-ae9c827e-compressed`.
The three `*-independent-summary.json` files preserve both physical analyses,
native timing and limitations. `manual90_west_baseline-local-analysis.json`
preserves the failed baseline. Both processes normally stopped, four owned
generated files were removed, and exact prior DLL/settings/receipt restoration
retained all 132 rollback sets. Authority is revoked.
Returned receipt SHA-256:
`62353084d892d8541420eb86ce733af72285fdebc861341f34748ed0cd1eaabf`.
Restoration receipt SHA-256:
`e068caa6d303774e496a851197d674cd61857f0cbf2239cf7522f4f8170122d7`.

## Follow-up: consistent current native phase reads

Source correction: `18de7d3a0faa405ef4f197aa9386409c6162e10e`.

Source review found a concrete asymmetry: native grabs already use
`read_current_physics_state`, while held-flip observation and actor ownership
checks cancelled on a single zero phase read. Both now reuse that existing
helper. Only zero receives one fresh reread, bracketed by exact skater and state
block identity checks. No cached phase, waiting window, deadline extension or
native actor substitution is introduced. Persistent zero, unreadable data,
changed owners and nonzero ground/bail/rail phases still reject. Existing 150 ms
observation freshness and actor/id/view/count requirements remain unchanged.

The old single-read behavior, extracted into the same production test seam,
fails the composed early-withdrawal regression. This is explicitly a
behavior-preserving legacy seam, not an unmodified old checkout. Corrected
ASan/UBSan and warnings-as-errors tests pass 2068 held-flip, 6517 session,
648 takeoff, 326 jump-idle and 20740 grab checks. The regression covers both
owner-check boundaries, failed reads, persistent zero and zero-to-ground/bail.
Bounded observation-loss and reread diagnostics distinguish block, phase and
flip-field failures; they do not claim acceptance or exhaust every actor-callback
refusal. Root and independent source review passed.

Receipts and exact source hashes:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-held-phase-checks`.
This corrects a source-proven read-consistency gap. The third trial's exact
failure cause, Windows compilation and new-DLL native acceptance remain pending.

The integrated shared sanitizer graph passes 46/46 suites in 57.69 seconds,
including enabled/disabled wallplants and Casper. JUnit:
`rc51-targeted/rc51-held-phase-targeted.xml` in the SSD build directory.

## Related ground-input phase consistency

Source correction `f2ce72dbc724402df97b4e26fb48379830f5b108` applies the same existing
current-phase helper to Footplant and GroundGrab observation and ground actor
ownership. Their old successful zero read reached active-lease cancellation.
Both observers now require a known current phase; only zero gets one fresh read
bracketed by exact skater/state-block identity. Existing Footplant subtype phase
sets, GroundGrab state100, exact actor view, settings and deadlines remain intact.
No cache, new buffer or native actor identity is introduced.

Two behavior-preserving legacy-seam active-lease regressions fail before the
correction and pass afterward. Footplant/GroundGrab, session, takeoff, jump-idle
and held-flip sanitizer/Werror checks pass, with independent review. Evidence:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-ground-phase-checks`.
This is a source-proven sibling cancellation gap; no ground-mechanic native
failure or new-DLL success is claimed. It is batched with the held-phase fix
before a single Windows compilation checkpoint.

The final combined held/ground batch passes 46/46 shared sanitizer suites in
56.91 seconds. JUnit: `rc51-targeted/rc51-native-phase-targeted.xml`.
The final manifest edit separately passes both candidate-contract tests.

## ae9c827e independent Manual prerequisite attempt

The reviewed mixed-request observer correction passed 76 preparation and 109
focused checks before a new exclusive slot. The exact previously compiled ae9
DLL was reused; these observations do not validate later source corrections.
Fresh stopped-state installation checks, mapped DLL hash and native Mega Park
identity passed. Both native and cached steps measured 1/60 after setup.

The corridor trial stopped at 11.835830679 seconds on an unknown top-level
phase (`0`). The owner remained identical. Separate wall-motion, contact and
bail reads reported state100, with four stable active contacts, coherent request
words, valid pose, unchanged stance and continuous motion. Adjacent state100
rows bracketed the unknown within 59.63 ms. This is an inconsistent external
observation; it does not establish a physical collision, owner change or bail.

The baseline remains failed: neither the full 15-second movement interval nor
the separate native brake ran. Its later OFF sample still moved at 6.05082 m/s.
No Manual trial was attempted. The retained acquisition capture was incomplete
(3095/3099 rows, four drops); it cannot establish complete input history. A
separate offline audit covers unavailable phase handling across existing
physical, arrival and feedback gates before another hardware attempt.

Evidence directory:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-manual-hil-ae9c827e`.
Independent analysis SHA-256:
`412177474ca1e6ad9ad59317392110d87e6ffe81b97f82955293ac47838445c9`.
Both processes closed normally. Four exact session-generated HUD files were
removed, and prior971c DLL/settings/receipt hashes were restored. All 132
rollback sets remain intact; transport authority is revoked with no pending
hardware action. Returned receipt SHA-256:
`94daa555f8e630e0dc1cd0b1851f908834af59c76bc24d75fee119915e426a18`.
Restoration receipt SHA-256:
`7a28afe065c8ba234ee267b3be3021e44884939545dc599d0a40b1d570dc8b89`.

## Queued grabs through native pop retirement

Source `c038d602f2f38ab4449a9a221c3bdd93896ae43c` fixes a source-proven
inconsistency between existing queue eligibility and auxiliary admission. A
waiting first B could retain the exact released-pop proof but reach the native
grab adapter with its retired action stamp. The queue now passes the already
qualified current lineage to that unchanged adapter. Repeated-B recognition
uses the same eligibility gate, preserving the first direction and the second
physical edge. No native actor is inferred from either request.

The old production header compiles and fails the composed regression with AIR
reached inside the existing 500 ms takeoff window. An earlier exploratory
fixture exceeded that window and was insufficient evidence; it is retained as
a negative test. The correction covers single and repeated B at 30/60/120 Hz,
both stances, neutral grabs, the 199/200 ms repeat boundary and owner, object,
stance, stale observation, landing, wrong selector, focus and new-A cancellation.
Original timestamps, ordinary first-request policy and native admission remain
unchanged. Both B presses already queued before a later selector retirement
remain a gameplay limitation: a child cannot fabricate native grab ownership.

Focused ASan/UBSan and warnings-as-errors checks pass: 20,796 native-grab,
6,517 session, 648 takeoff, 326 jump-idle, 2,068 held-flip and 9,174 dispatch.
The shared graph passes 46/46 in 57.49 seconds, including disabled wallplants
and Casper. Root and independent source review passed. Receipts:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-grab-retirement-checks`.
JUnit: `rc51-targeted/rc51-grab-retirement-targeted.xml`.
This source correction is not covered by the older ae9 or 986 packages; Windows
compilation and repeated exact-DLL native acceptance remain required.

## Compiled checkpoint: 986fb8c0

Exact source `986fb8c014e6d4afc5c4f7034df56130ec104d49` passed
[CI 43845046801](https://github.com/notPho-Real/ReSkate/actions/runs/43845046801).
All nine required jobs succeeded, including 140 Windows preflight and 132
upstream integration tests, Windows runtime compilation, packaging and release
eligibility. Actual release publication was skipped. The held and ground phase
read corrections are compiled; the newer queued-grab correction is excluded.

Installable experimental artifacts:
[Deck installer](https://github.com/notPho-Real/ReSkate/actions/runs/43845046801/artifacts/14217675637),
[runtime](https://github.com/notPho-Real/ReSkate/actions/runs/43845046801/artifacts/14216269863),
[integration results](https://github.com/notPho-Real/ReSkate/actions/runs/43845046801/artifacts/14216833635).
DLL size: 45,610,496 bytes. DLL SHA-256:
`65e59c3bde3abe98935e87298c0a7912e08f56956bfa850451b395592307f215`.
Strict archive, PE, provenance and embedded DLL/installer/updater checks pass.
Local package: `/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-native-phase-ready-986fb8c0`.
Verification receipt SHA-256:
`14bd109b15de97d3df8fb37f48d256da1d76a46f09d976f2662f2fd0163f61a9`.

No native test of this exact DLL is claimed. Use the existing stopped-state
installation and rollback procedure above, preserving prior DLL/settings/receipt
and stock assets. The accepted updater remains unchanged. Next native work is
repeated early-flip/held-actor validation and ground-input cancellation controls;
the independent Manual observations continue to belong to ae9 only.

## ae9 corridor qualification and brief physical Manual

The next independently reviewed observer version passed 186 focused and 105
clock/rotation checks, then 76 preparation and 112 inactive-scaffold checks.
Unavailable top-level phase cannot qualify or renew physical/balance evidence;
unknown or stale samples neutralize corrective input. Existing entry, stop,
owner, contrary-state and time bounds remain intact. Both previous failures
remain retained; neither is retroactively accepted.

On the exact ae9 DLL, the new corridor completed 15.0218 seconds of movement.
One owner and stance persisted; the largest independently qualified sample gap
was 97.984 ms, within the original 150 ms bound. Nineteen unavailable samples
did not renew evidence. Native braking after Arcade OFF ran for 2.2155 seconds,
stopping over 5.8176 metres to 0.0000664 m/s, with a later stationary sample.
This qualifies one bounded follow-up on the same reset, not arbitrary geometry
or continuous native callback coverage. The acquisition capture was incomplete
(3090/3101 rows); video ended before braking, whose proof is physical telemetry
and the post-stop frame. Independent analysis SHA-256:
`e81eaabed8408e0862a3f26c0d658fb000dc3965aeb7109ddb69b5b8d46a4122`.

After a fresh identical reset, the natural stick throw produced a brief physical
Manual: 24 samples over 0.865 seconds show approximately 2–11 degrees of board
pitch, two upward-normal support points, valid pose, stable owner and stance.
Native current-sequence points advanced 0 to37, with rendered Manual/balance/x1
feedback. This is stronger than intent or HUD alone, but is not lifetime or bank
acceptance. Every entry-window sample exceeded the unchanged 8 m/s test margin
(8.4196–8.8846 m/s). The 700 ms entry gate timed out; no adaptive correction,
10-second hold record or Ollie exit was executed. Video shows a later bail after
physical collection stopped, so its precise cause is not established.

A second failure occurred during final log collection after the collector and
video had stopped: the bounded log planner rejected backlog/truncation. It did
not cause the earlier entry timeout. Next offline work uses ordinary input to
meet the existing speed margin and separates final diagnostic draining from
live balance freshness; no production speed tuning is justified by this test.
Manual analysis SHA-256:
`8efb6e5371f87b24f2d0fef6fec0b9e02921507b0f0efface91797bd4a2e87cc`.

Evidence directory:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-manual-phase-hil-ae9c827e`.
Game and launcher closed normally, four exact generated HUD files were removed,
and prior971c DLL/settings/receipt were restored with all132 rollback sets.
Authority is revoked, with no pending hardware actions. Returned receipt:
`1123126c478c5a75fb49cfeb8b94c5ada8caca7626a5a6ec6e62c459ed67092a`.
Restoration receipt:
`8af305cd31bd80526c7154b5d1011998b47166b043972f35aaefa56cf621a8fa`.

## Exact 986fb8c0 native repeated-flip comparison

Exact source `986fb8c014e6d4afc5c4f7034df56130ec104d49` and DLL SHA-256 `65e59c3bde3abe98935e87298c0a7912e08f56956bfa850451b395592307f215` ran on the pinned game build in `skate3_megapark`. This is the packaged checkpoint from CI run `43845046801`; the exclusive hardware operator was root. Independent local analysis reused the existing full and dense board-motion evaluators.

The finite immediate repeated-X recipe produced five scoped doubles in five executed 30 Hz repeats and one in one executed 60 Hz comparison. Each had a fresh reset, complete source-103 capture, matching source/owner/board body, native base actor ID2 and admitted held actor ID2 with actual advancement, sampled extra count1, approximately two physical board rotations, airborne native held-state exit and a supported ground landing. Endframes show **Double Kickflip Clean**, **LINE224**.

| Trial | Measured input callback Hz | Complete capture, zero drops | Turns: dense / full | Dense maximum gap (ms) | Native held exit before supported landing (ms) |
| --- | ---: | ---: | ---: | ---: | ---: |
| flip30_immediate1 | 30.003530 | 377/377 | -1.991518 / -1.991492 | 13.063 | 265.744 |
| flip30_immediate2 | 30.014124 | 382/382 | -1.991519 / -1.991514 | 12.326 | 210.520 |
| flip30_immediate3 | 29.971791 | 380/380 | -1.991520 / -1.990643 | 14.927 | 252.643 |
| flip30_immediate4 | 30.003530 | 380/380 | -1.991519 / -1.995557 | 15.408 | 227.635 |
| flip30_immediate5 | 30.045953 | 377/377 | -1.991518 / -1.994730 | 11.978 | 186.324 |
| flip60_immediate1 | 59.282925 | 717/717 | -1.994810 / -1.990552 | 11.130 | 214.204 |

Native physics and cache steps were 1/30 for the five 30 Hz trials and 1/60 for the 60 Hz trial. Held actor advancement was 2.0011625 ticks at 30 Hz and 1.0005813 at 60 Hz. Actual release is established by native held state becoming false in AIR201, with held intent/preparation false and count1 still present, before four stable sampled ground contacts in state100. The release command acknowledgment alone was not used as proof: trials2 and4 logged no loop acknowledgment at withdrawal but later directly sampled count1 and airborne native exit. A dedicated catch actor was not independently traced.

All six logged only known phase100 held-observation loss at landing. None logged STATE RECHECK, so the transient-zero phase recovery path remains unobserved. These results do not prove that the new read correction caused the improved six-trial outcome, and bounded diagnostics do not enumerate every callback refusal. External physical samples remain non-atomic; agreeing dense/full unwraps reduce sampling concern without establishing continuous observation.

Each terminal ModelSettlement is kind1, points224, actions2, custom_points0, explicitly model_only=true and native_bank_verified=false. Rendered clean labels and the model total do not establish durable native bank authority.

The earlier ae9c827e comparison remains retained: two physical doubles out of three 30 Hz trials, with the third preserving the second input/request but producing only one physical turn and no held actor advancement. Its cause was not visible in the older telemetry. The newer scoped success does not erase that failure or establish exhaustive reliability.

The second 60 Hz trial was prepared but **not executed**: the exclusive slot lacked enough remaining time for a fresh reset and bounded trial. No additional cadence or control outcome is inferred. Normal shutdown stopped game and launcher. Exact prior971c DLL/settings/receipt were restored, only four session-generated HUD files removed, all133 rollback sets preserved and authority revoked. Return receipt SHA-256: `d121e8ec2f4cb69412ab706b79b21610907c11ccfaddfaf7ecca631673add144`; restoration: `6765d66e057dc7e42116ff628cb371acb308c77ac00dc50caed40cd72051056e`.

Independent evidence: `artifacts/rc51-native-phase-hil-986fb8c0/flip-six-trial-independent-ledger.json`, SHA-256 `40d40c97b793c637c7b4e7762b308660cba347cf7f504ee000a671c244a5ac58`. Per-trial `*-independent-summary.json` files retain original receipt and evaluator hashes, physical release rows, diagnostics and rendered-frame references. The sixth summary hash is `eca3e5e490cfa8f41e3f9622d60b7f90e18c5f018747f5370e8277153060e1a4`.

## Compiled queued-grab checkpoint36d615ce

Exact source `36d615ceeec306aa8cfb400a15a436925914a599` passes [full CI43854205335](https://github.com/notPho-Real/ReSkate/actions/runs/43854205335): all9 required gates,140 preflight and132 integration tests. DLL SHA-256 `60e97c42fd070f71bbb0aa06af902ed1086fb194144c17d24a63ed610a96129b`,45,610,496 bytes. Strict archive, PE, source provenance, embedded DLL and installer checks pass; verification SHA-256 `1c886857f34b255354850ace5a9a3f5ec67d53baa119558efa7d7c3091b672a5`.

[Experimental Deck package](https://github.com/notPho-Real/ReSkate/actions/runs/43854205335/artifacts/14223285849), runtime artifact14221623307 and integration artifact14221952710 bind the same source. Installer SHA-256 `6fd0227e6b76b2c3d6b5c19893667e01fad48a1ad6a0b99ab422386268097981`. Local verified package: `artifacts/rc51-grab-retirement-ready-36d615ce`. Native acceptance is pending;986 observations do not verify this later DLL. Preserve prior files and use the existing exact-hash transactional rollback procedure above.

Initial reduced push run43853451468 was cancelled before packaging; unchanged source was dispatched with full integration as43854205335. The reduced run is retained as evidence and was not accepted by the strict verifier. No test or packaging requirement was relaxed.

### Retired rail input preserves the next native grab — aff7a33b

Source: `aff7a33bff6d94f68f7f59a36fb2080193a78725`.

An admitted rail pop already retired its initiating Y command, allowing continued AIR actions while that physical Y remained held. Native grab permission construction and the session admission gate still rejected raw Y, so a fresh B could lose its intended native grab. The composed regression reproduces this with a qualified released rail lease; releasing Y later does not recover the consumed B.

Both gates now use one read-only proof of that exact initiating Y hold. Initial admission requires the existing released rail-pop lease. After grab entry retires the pop selector, continuation requires the existing native grab occurrence callback with matching owner, flight and request lineage. Request success grants no native completion or score. The original2,500ms pop bound remains fixed; polling or grab admission cannot extend it.

Physical Y release clears the exception. Ordinary Y+B, a fresh Y after setup, release/repress, second A, owner changes, stale observations, focus/OFF, landing/bail, missing native grab continuity and expiry remain negative. Fresh native observations test the inclusive2,500ms boundary and rejection at2,501ms.

Validation: the old-header behavioral regression compiles and fails as expected (the compatibility seam models the original top-level raw-Y gate). Focused ASan/UBSan checks pass with warnings as errors:1,456rail,20,796grab,6,517session plus648takeoff and326jump-idle, and2,068held-flip checks. The established sanitizer graph rebuilt and passed46/46; JUnit is retained. Root and independent camera review found no blocker.

Status: implemented, reviewed and committed; Windows compilation and exact-DLL native gameplay remain pending. Native occurrence observations in portable tests are explicit independent fixtures, not inferred from request success. Earlier986 hardware results cannot verify this later change. The next batched Windows checkpoint must preserve exact source/DLL provenance and validate repeated held-Y rail-pop→B execution, native actor advancement, release and physical landing before this is called gameplay-verified. No accepted release channel change is authorized by this note.

Evidence directory: `artifacts/rc51-retired-rail-y-checks/`; see `build-necessity.json`, `red-results.json`, `green-results.json` and `targeted-junit.xml`.

## Rail regression stack correction

[CI43860751678](https://github.com/notPho-Real/ReSkate/actions/runs/43860751678) compiled the targeted Windows tests but crashed in the new rail-pop test; the other45 passed. The remaining run was cancelled, preserving the failure log. Each full controller fixture occupies208,416 bytes; the new helper had a measured1,051,280-byte unoptimized frame. A private1MiB thread reproduces a crash with the old helper. This supports stack exhaustion, although the original Windows exception code was not available.

Only the five new fixture owners move to scoped heap storage. Inputs, assertions, production code and stack limits are unchanged. The helper frame falls to9,264 bytes; the same constrained-stack probe and all1,456 rail checks pass with sanitizers and optimized compilation. Root and independent review pass; corrected Windows CI remains required. Evidence: `artifacts/rc51-rail-pop-stack-checks/`.

A separate third-X audit retains36 production-correct cases across30/60/120Hz, both stances, Kickflip/Heelflip and three readiness phases. All preserve the working double under the real one-shot bridge contract;2,212 sanitizer checks pass. An earlier permissive callback fixture could accept a second submission, but production native lease rules reject it. That fixture is retained with its limitation, not treated as a second native actor. Triple execution remains an M lifecycle gap; see `artifacts/rc51-third-x-audit/audit.md`.

## Verified package: f0025f3e

Exact source `f0025f3ed25d2d4912ab90b66c36826af1a1490b` passes
[CI 43862368489](https://github.com/notPho-Real/ReSkate/actions/runs/43862368489):
all nine required gates, 46 targeted Windows tests, 140 preflight tests and 132
upstream integration tests. The corrected rail test passes on Windows. Runtime
compilation and strict archive, PE, provenance and embedded installer checks pass;
release publication was skipped. The preceding failed/cancelled run remains
retained and supplies no accepted package.

[Experimental Deck installer](https://github.com/notPho-Real/ReSkate/actions/runs/43862368489/artifacts/14225183664),
[runtime](https://github.com/notPho-Real/ReSkate/actions/runs/43862368489/artifacts/14225453738),
[test results](https://github.com/notPho-Real/ReSkate/actions/runs/43862368489/artifacts/14225388256).
DLL size: 45,611,520 bytes. DLL SHA-256:
`5e4d3b6e98b7d15f4fc13e3956dd6a226d6cd151ef044ef1d4a5e8e130199ddb`.
Installer SHA-256 (`Install-ReSkate-Arcade.desktop`):
`87857520d7842fa0f9e4abe4291a0a501f77501e34f123f90a5dd8829ed5de5d`.
Verification receipt SHA-256:
`ca06a6539a296224be4dfc076a75a5233f2bb24c843503826d4d7a9e752a1023`.
Local package: `artifacts/rc51-rail-grab-ready-f0025f3e`.

This is compiled and installable, not native accepted. Use the stopped-state
transactional installation/rollback procedure above. Preserve the prior DLL,
settings, receipt and every rollback set. Neither this package nor its installer
changes the accepted release channel.

The exact DLL was subsequently loaded in native-verified Mega Park. Its first
corridor prerequisite failed the unchanged upper speed bound; no Manual trial
ran. See the retained failure and restoration below. Loading the package does
not qualify its rail-grab correction or the full gameplay sequences.

## Exact36 Manual prerequisite and observer failure

The exact36 DLL completed a 15.042929-second west-corridor baseline at measured
59.6302 Hz, with native/cache steps of 1/60. One owner and stance persisted;
the maximum independently qualified sample gap was 101.662 ms, inside the
unchanged 150 ms bound. Native braking after Arcade OFF stopped in 2.224810
seconds over 5.702026 metres to 0.000465 m/s; a later sample remained stopped.
The acquisition capture has 27 dropped rows and is not complete callback proof.
Physical telemetry and a post-stop frame establish braking; video ends earlier.

The subsequent shorter-push Manual trial is **inconclusive**. The first log
collection rejected its backlog/truncation guard, which checks offsets and the
existing 1 MiB per-file unread bound.
The only retained physical row precedes trial start. A later Manual label and
score sample cannot establish physical entry, delivery, a qualified lifetime
gate or a ten-second hold. Final diagnostic collection also rejected rename
continuity. No retry was performed; the failure remains retained.

Evidence: `artifacts/rc51-manual-approach-hil-36d615ce`. Independent baseline
analysis SHA-256: `cdd34c693bf85cb1b80738f4220c5755328ea925489315d05f6831eacb3d2e1a`;
Manual analysis: `897ca44d2551ad5df87d1dcec8d2c78a2194367535f20ef37345e8cd389d7843`.
Normal shutdown stopped game and launcher. Exact prior971c DLL/settings/receipt
were restored, four exact generated HUD files removed and all134 rollback sets
preserved. Authority was revoked. Return receipt:
`7d62156a6b463533fe78d1ad8d57c85a25dd3a4a587bca400884af751735d499`;
restoration: `69233863dd2d13eb5afa1f9fecc343a9cdc7ce47e4d2d5091e129cc99f921f48`.

The offline correction drains the original unread log baseline during serialized
console setup, before collector creation, and starts the collector before the
existing warmup. Pretrial reads cannot qualify or renew live balance evidence.
No bytes are discarded and all loss, rotation, owner, speed and time bounds
remain intact. Lossless punctuation compaction keeps the generated Manual loader
below its unchanged limit, with full AST equality and compilation assertions.
All421 offline checks pass. Retained-identity loaders are 64,140/64,131/64,127
bytes; fresh device identities must be regenerated and checked before transport.
The executed36 scaffold is unchanged. Frozen proposal manifest:
`artifacts/rc51-manual-startup-drain-proposal/FINAL-combined-provenance.json`,
SHA-256 `809a585e22a1407194e450717621ca44b2a94dd0dd23809608fcb3b39ff618d0`.
Exact failing offsets were not retained, so the startup-backlog cause remains
source-supported inference; this correction supplies no native Manual result.

## Wallplant clearance ownership correction

The exact installed `5ae5778627a7dbe0bf3153eb7b36b471bdd5aa1b` was tested in
San Vansterdam free skate on November 16. The bounded attempt delivered a fresh
airborne A, passed two native broad-wall queries, then failed clearance admission
and fell without a hold or rebound. Native callbacks measured about 29.72 Hz;
physics and cached steps were both 1/30. The captured location includes a raised
bench surface, so it is not an accepted unobstructed-wall trial. Capture-wide
event completeness is unproven. Evidence is retained in
`artifacts/rc51-wallplant-verify-5ae57786/city_wall_1.json` and its independent
analysis; no wallplant acceptance follows from this attempt.

Source review independently found that ordinary ground samples can lack the
static-owner stamp required by wallplant geometry. Both wallplant consumers now
reuse ground only after revalidation under their own owner policy; otherwise
they query fresh clearance through the existing native mechanism. All final
geometry, ownership, height, timing and collision gates remain in place.
Regressions fail against the old runtime in both paths and pass with the fix:
335 lifecycle checks under AddressSanitizer/UndefinedBehaviorSanitizer and an
optimized build. Windows compilation and exact-candidate native verification
remain pending. The installed DLL is still 5ae57786, without this correction.

## Remaining independent acceptance work

Held-Y rail-grab testing needs the existing owned fixture, segment, physical
contact and current-owner guards integrated into its wait. Historical rail-grab
success released Y before the pop and cannot verify this change. The isolated
hold-preserving wait proposal passes95 offline checks and fails closed without
that native guard; it is not a ready hardware recipe. This is an M integration
dependency, documented in `artifacts/rc51-held-y-rail-hil-audit/readiness.md`.

Native banking remains a separate M evidence gap. The score buffer deliberately
leaves owner verification false. Existing settlement contracts require verified
payload ownership, canonical occurrence identities, award semantics, complete
event health and matching native terminal meaning. Rendered totals, model
settlement and successful receipt publication do not prove durable native credit.
Reuse `research/score-replay-contracts.json`; do not add a competing score model.

Next independent work remains exact-candidate Manual lifetime and transitions,
switch/revert camera and restoration, native grab lifecycle/rail handoffs, and
the existing Casper/wallplant chains. Triple flips and new environment mechanics
remain deferred behind these foundational acceptance gaps.

## Exact f002 corridor speed failure and restored hardware

The independently reviewed inactive scaffold passed651 offline checks and bound
the corrected observer to f002, a fresh exclusive slot and the prior134 rollback
inventory. Its manifest SHA-256 is
`9894d865d39c92a4e89fb1d375198664b4efde4bc04ea595d67a55e38037518e`.
Fresh stopped-state preflight passed; the exact loaded DLL and native
`skate3_megapark` descriptor were verified. Actual-identity loaders measured
64,003 bytes for the baseline and64,127 for each prepared Manual trial.

The first baseline failed at3.513706119 seconds: forward speed8.0183258 m/s
exceeded the unchanged8 m/s test bound. The preceding accepted sample was
7.9939847 m/s. Retained post-start samples show native state100, one owner and
regular stance;120 qualified and7 unavailable samples have a maximum qualified
gap of114.085491 ms. No coherent wrong contact mask or sampled bail establishes
an obstruction at the failure. These are bounded external samples, not atomic
or continuous physics proof.

The ordinary push recipe matched the preceding36 baseline: stick forward at0.1
seconds, release at0.8. Actual input duration was697.526 ms versus700.030 ms;
measured native callback cadence was59.5625 Hz versus59.6302 Hz, both with native
and cached1/60 steps. Nevertheless, speed was about1 m/s higher early in this
trial. The retained evidence does not establish the cause or a runtime regression.
No production tuning or test-limit widening follows from this observation.

Replay aborted and neutralized input. No native brake segment,15-second route
completion or Manual attempt occurred; both prepared m1/m2 trials remained
unexecuted. Acquisition evidence has35 known ordinal holes and no terminal
record, so complete capture and total loss are unknown. The startup observer
correction was not exercised by a Manual attempt.

Evidence directory: `artifacts/rc51-manual-startup-hil-f0025f3e`. Independent
analysis SHA-256:
`28e7c0e06209df0cbf28fdec082a27ffeca68d6179481daeddb828210156d8ec`.
Normal shutdown stopped game and launcher; four exact generated HUD files were
removed. Exact prior971c DLL/settings/receipt were restored and all135 rollback
sets preserved. Authority is revoked, with no queued hardware actions.
Return receipt SHA-256:
`b3d02e21c6eb2d14ae5b4fddc444356829dc32761ec94dafcea2223e0adb197a`;
restoration: `4a7fdfde7241c717371f6d548b14e6338e05d391dd0e430563196c79aa8c3bf7`.

Next independent setup work aligns the baseline's initial ordinary push with
the already shorter Manual approach. Preserve the original failed attempt,
all physical/time/speed bounds, native braking and fresh-reset requirements.
This is test setup work; no new runtime build is needed for it.

## Exact f002 shortened-push contact failure

The separate setup proposal changed only the west-baseline ordinary push release
from0.8 to0.45 seconds, matching the existing Manual approach. Its122 offline
checks plus2 loader checks verify scheduling, whole-generated-AST identity after
that one event change, unrelated-recipe byte identity and unchanged rejection
of the earlier8.0183 m/s sample. No physical speed prediction follows from those
checks. Proposal manifest:
`b62a7f55068b8bf44bc86382ce2f0c9c5b8fc9a332d5847c96bc1cee1b963de9`.
The new inactive scaffold passed230 focused checks and retained the existing
Manual evidence; manifest:
`d27059f81b9d0a84a9fcf2ba7c791735a32983fef561218166165796f003d44d`.

Fresh device/source/DLL/native-map verification passed on the same compiled f002
package. The baseline then failed at0.703230218 seconds on two-truck selection.
Matching request words, native state100, one owner and regular stance accompany
mask0x8000, two repeat-stable selected and raw contacts, and board pitch−0.358826
degrees. Speed was5.6774 m/s. Four contacts returned in the next sample at
0.753161165 seconds. This is not the earlier demonstrably mixed-word read;
nevertheless current-native-step contact freshness and axle mapping remain
unproven. It does not establish a runtime regression or justify relaxing guards.

The trace contains21 post-start physical rows through0.812117 seconds and a
76.298 ms maximum sample gap. The longest native source6 group measures58.7156Hz;
native/cache steps are1/60. The acquisition capture has1124 consecutive ordinals
but no terminal and ends212.919ms before the failed physical sample, so it is
incomplete. Video shows pushing/coasting; it cannot replace the missing15-second
proof. Native braking and both Manual attempts did not run.

Evidence: `artifacts/rc51-manual-short-baseline-hil-f0025f3e`; independent analysis
SHA-256: `d1e8b88ceb42a8c4f5bd47c7919d526dc611c70b0495924f03ec4109bcdc6761`.
Normal close, exact four-file generated-pack cleanup and prior971c restoration
passed. Both processes are stopped, all136 rollback sets remain, and authority
is revoked with no queued hardware work. Return:
`3a43cfe5bb2a86c284bf71118c33eace8475af545cf44ccc792bdf3287cba27c`;
restore: `2b9456f21a5f34d076557092ac8ded6a9ea8098aa2c362203a3d61d122dc9902`.

Manual lifetime remains unaccepted. Further work must resolve the relationship
between native pushing, physical contact and the corridor prerequisite from
retained evidence before another hardware attempt; neither guard widening nor
an automatic retry is justified. Independent held-Y rail guard integration and
the other acceptance gaps remain separate work.

## Recovered interrupted wall-clearance checkpoint — November 20

The interrupted “Polish and Expand Arcade Mode” thread continued beyond the
previous notes. Its artifacts are under `/Volumes/ReSkateSSD/ReSkateWork/artifacts`,
not this worktree's `artifacts` symlink. The earlier bounded selection note missed
this unfinished package verification; it is superseded by this recovery.

Source `aab5517dec915c05f1abcc7eef5ee8edf62fa5b2` completed CI43972957491:
all nine required gates, 140 Windows preflight tests, 132 integration tests and
Windows runtime compilation passed. On November 20 the existing strict verifier
successfully checked archive hashes, PE, source provenance, test results and
byte-identical embedded installer DLL. No CI was dispatched again.
DLL SHA-256: `089946ee7049a997a5ea9172d7f729840f682e17fcb73fe0501889a5bbcc2b37`
(45,613,056 bytes). Verified installer:
`/Volumes/ReSkateSSD/ReSkateWork/artifacts/rc51-wall-clearance-ready-aab5517d/deck/Install-ReSkate-Arcade.desktop`.

The earlier exact5ae Mega Park capture was retrieved before interruption. Its
retained independent physical review records one 164 ms hold, 11 strictly
post-release upward/outward consumed-velocity samples and 59 subsequent grounded
samples, with unchanged owner. Compressed and raw capture hashes were rechecked
on November 20. The capture footer reports one dropped row and incomplete event
health; external samples are non-atomic. This supports one scoped physical
hold/rebound/return to riding, not repeatability, banking or exact-aab acceptance.
The repeat reset failed focus before sending input. No new hardware call occurred.

The expired session has no final shutdown/handback receipt in the recovered
artifact directory; current device/process/installation state is unknown.
Do not reuse its expired authority or assume its old process identity is current.
Next: obtain a fresh exclusive hardware handoff, inspect current device state,
and validate the packaged clearance correction against the retained city failure
using the established prerequisites. No new source defect is inferred from that
pending acceptance. Allowance consumption is unavailable.
