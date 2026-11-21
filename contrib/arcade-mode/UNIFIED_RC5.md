# Unified RC5 — experimental integrated Arcade candidate

The authoritative source checkpoint is `unified-sources.json`. Each runtime and
Deck artifact carries `candidate.json`, generated from that checkpoint and the
same run's source SHA, DLL hash, gameplay fingerprint and compiled feature flags.
This branch does not publish to the accepted Arcade update channel.

## Integration decisions

- Start from RC3 `cd106244`; merge actual official upstream `95b0a436`, not fork main.
  The live upstream head still matched the requested pin. Existing Arcade native
  address headers and fingerprint checks remain pinned to game build 20260929.
- Preserve RC3's newer held-flip Prepare-window guard while deriving native IDs
  from Trickpedia. The canonical registry remains the only trick binding source.
- Upstream's hook and patch services are retained. Remove its obsolete unused
  single-observer and asynchronous world-ray interfaces. Keep Arcade's separate
  synchronous park-surface query and balance-bail request, with their existing
  owner/thread/lifetime checks. Retain the supported multi-listener physics API.
- Preserve RC3 snapshot/retry crash diagnostics alongside upstream's cached
  parent directory and native mod-list fixture. The fixture now expects its real
  native test mod; do not preserve the obsolete blanket zero-mod assertion.
- Cargo always checks its fingerprints and lockfile. Retain the optional-asset
  regeneration helper, plus upstream source dependency enumeration.
- Move Trickpedia and RC4 test targets into the optimized reusable CI graph;
  retain independent Windows preflight, native-menu lifetime, full upstream,
  installer/updater, provenance, security and release-eligibility gates.
- Integrate RC4 at `dce382823f0cefb05d1e60c6b3713e6991ce9412`. Candidate CI
  reads its compiled wallplant option from `unified-sources.json`; packaging
  rejects either enabled/disabled mismatch. The ordinary CMake default stays OFF.
  Preserve Casper's independent material capture, per-joint animation weights,
  controlled playback clock, display turn and native render restoration. Share
  the same player with wallplant holds and clear expired controlled state before
  a new held submission. Both enabled and dependency-free disabled runtime
  regressions remain required.
- Enable upstream development tools in this experimental CI build so existing
  observation and regression tooling is retained. Release acceptance is unchanged.

## Evidence and acceptance

RC3 source reports retain native observations of early flips, repeated-X double
rotations, Manual/Nose Manual entry and contact, native revert actors, grab and
fingerflip actors, and selected one-footer Into/Loop/Exit sequences. They do not
establish blanket acceptance of all contexts, scoring, timing or stance cases.
See `research/rc3-gameplay-status.md` and the canonical Trickpedia evidence.

Arcade guidance uses the original Skatepedia cards and their input sections;
it does not replace the native page. The canonical registry supplies mappings
and small status notes. Native navigation, Back and same-session OFF restoration
still require the final candidate walkthrough.

The pre-wallplant candidate `0b247367` passed required CI and visibly rendered
x10 in the original score circle during its Mega Park HIL session. Its held
Casper still displayed Manual. The subsequent row-retention correction passes
its focused old-source negative control, but custom-name rendering remains
unverified. Do not mistake a model containing the text for a visible label.

RC4's exact source has separate native straight/angled hold-and-release,
flip/Manual follow-up, repeated-input, unrequested-collision and Arcade OFF
evidence. This is historical evidence for that source, not acceptance of the
merged RC5 DLL. The combined candidate still requires exact-source compilation
and a coordinated exclusive Deck regression session before owner testing.

## Install and rollback

Use the single `Install-ReSkate-Arcade.desktop` from the same-run Deck artifact.
First verify its accompanying `candidate.json`, source commit and DLL SHA-256
against the Windows runtime artifact. This is an experimental test candidate.
Close both game and launcher. Preserve the current DLL, launcher settings and
installation receipt, then run the existing transactional installer. Preserve all
stock, RC2 and RC3 rollback directories. Follow its existing installer prompts.

To roll back, close both processes and restore the preserved DLL, settings and
receipt byte-for-byte. Verify the restored DLL's recorded hash before launching.
The public updater remains on its existing channel; do not promote this branch.

## Next deliberate checkpoint

Finish the integrated source checks before one consolidated Windows build.
Validate the resulting exact candidate on the Deck: custom names and integer
multipliers, bank/bail settlement, wallplant follow-ups, Casper transitions,
Manual/revert/rail flip and grab exits, camera follow, native Skatepedia and
Arcade OFF. Preserve failed evidence and all rollback artifacts. A build or
inherited device receipt does not establish the merged candidate's acceptance.

## Current native Manual acceptance duration

The owner's November 20, 2026 engineering request restores the native Manual
acceptance hold to **ten seconds**. Measure at least ten seconds from qualified
physical Manual entry, with continuous bounded contact, balance, stance, motion
and score/HUD observations. Button-hold time, a queued request or a lingering
label does not establish this interval. Keep existing owner, freshness, bail,
landing, entry/exit and Arcade OFF checks unchanged.

This supersedes the November 16 two-second requirement for future acceptance
trials. Preserve historical results, failures, pending observations and portable
lifetime regressions as recorded. No duration change waives the approach
prerequisites or establishes native acceptance on a different DLL.

## Casper integration checkpoint

The owner authorized this integration on November 12, 2026. The existing RC5
worktree was clean at `b12327c3`, and the Casper worktree was clean at `2b566b87`;
both matched their refreshed remote heads. RC5 is an ancestor of Casper, so the
integration fast-forwards all 20 existing commits without conflict resolution or
reimplementation. No later RC4, Trickpedia or upstream changes are included.

This checkpoint retains the authored entry, held balance corrections, horizontal
flip rotation, direct and Ollie exits, contact pose, shoulder rotation and
surface-dependent resistance. It also retains the Animation Studio authoring,
interpolation, stance, native-clock playback and cancellation changes. Casper
remains a development-only, unsaved opt-in (`arcadeflatland on`); ordinary Manual
and Arcade OFF behavior remain the defaults. Wallplants stay compiled OFF.

The prior exact Casper DLL and the prior RC5 package remain rollback artifacts.
A new candidate must carry this source manifest and its own CI run, source SHA,
DLL hash and installer identity. It supersedes neither historical evidence nor
the accepted update channel. The published candidate remains unaccepted.

### Regression scope

- Run the focused authoring/export, animation core, material selection, input,
  Manual lifetime, combo ownership and release-disabled tests with sanitizers.
- Run the existing targeted suite, including Trickpedia lifetime and the disabled
  wallplant link test, then all required Windows and cross-platform CI gates.
- On the exact combined DLL, verify Mega Park, source identity and native cadence
  before inputs. Exercise both stances through entry, moving hold, flip, direct
  and Ollie exits; include rotation held across exits and surface selection.
- Check ordinary flips, grabs, Manual/Nose Manual, revert and combo continuation,
  plus Trickpedia controller search/Back and same-session Arcade OFF restoration.
  Use native actors, state and ownership evidence alongside actual rendering.
- Exercise cancellation by focus/controller withdrawal, bail and menu transition,
  and confirm disabled wallplants issue no ownership or rebound.

The 19 native cases on the prior `2b566b87` DLL are historical supporting
evidence, not automatic certification of a newly built DLL. New CI and native
results belong in the exact-source artifact receipts. No combined native pass
is claimed by this document.

Owner visual approval is still pending. Slopes/edges and another body/footwear
configuration remain unverified. The displayed inverted board and hooked foot
use native Manual rigid-body support; this is not physical inverted-tail support
or a guarantee of mesh clearance for every outfit and frame. No custom native
scoring or trick-ID acceptance is claimed.

## Official upstream refresh

The owner requested the current official main on November 12, 2026. The source
pin advances from 95b0a436 to 355bf71e. The existing RC5 checkpoint 6de06957 and
all Casper authoring, playback, surface resistance and rotation changes remain
in history. RC4 stays disabled at its previous integration pin.

Resolve the new HUB front page and Trickpedia section collision with distinct
shared constants. Carry the Arcade switch and Trickpedia link into upstream's
new SKATER options, retaining native Back and level cleanup. Preserve the
bounded HID completion ownership while applying upstream plugin button masking
inside the admitted report handler; keep Steam-front masking before recording
the returned controller packet. Retain both park surface observation and the
new editor ground-grid request.

Upstream render-time pose replay is retained alongside the existing custom
clip and board publication path. Pose placement, PlayStation report, native
menu, score-line and news tests join the Windows preflight for this checkpoint.
The previous exact-source native results remain historical. In particular,
the prior Trickpedia failure is not declared fixed by a successful merge or
test build: the new menu requires an exclusive native walkthrough.

### Live native page discovery

The upstream refresh passed the full Windows compile and 128 integration tests.
Native regression testing then exposed an existing Trickpedia discovery defect:
the live screen registers a PageViewModel root directly, without the resource
wrapper the interception previously expected. The replacement now selects the
focused page whose original Back command is SP_Back. It retains the existing
typed copies, Back action and restoration path. A focused map page is an explicit
negative control; the production-body regression fails with the former lookup.

The revised page lookup still requires an exact-source Windows build and native
ON/OFF, search, Back and page-reload checks before candidate acceptance. Initial
merged-build Casper checks cover goofy entry, flip, direct return, Ollie and a
legitimate high-balance bail; they do not establish the remaining regression
matrix or complete foot-contact clearance.

### Preserve the native screen during Trickpedia presentation

The direct page lookup renders in game, and controller search opens and closes.
Native Back exposed a second ownership defect: publishing the entire donor Page
overwrites the live screen record, so its focus and transition state can remain
visible over the parent menu. The interception now publishes only its body and
title, and restores those fields plus the temporarily held Back action. Native
screen state is never replaced or rewound during restoration.

The production-body test keeps a nonzero native focus through publication,
changes it to hidden while the pause menu remains open, and verifies restoration
does not reacquire that hidden page. Reopening and Arcade OFF retain the latest
native focus. Allocation and partial-publication failures still unwind.

The merged gameplay source completed 25 bounded Deck cases, including both
stances, exits, shoulder rotation, surface slowdown at native 30/60 Hz, and
cancellation controls. This presentation-only revision needs a new exact-source
build and native Back/reopen/ON/OFF verification; the prior build is not accepted
for owner testing. Full foot-contact clearance remains visually unverified.

The Back action now restores only the input reference, navigation command and active flag temporarily held for search, following the native Hub lifecycle. Native action state outside those fields remains owned by the game. A focused regression changes that state while search is open and checks both search close and Arcade OFF. Native validation of this follow-up is pending.


## Default flatland and grouped guide

The owner requested flatland as ordinary Arcade behavior. The existing runtime
now starts enabled in both developer and non-developer builds; its incoming
Arcade gameplay context still gates hook preparation, material capture, input,
animation and movement. Arcade OFF cancels the episode. The developer command
remains an unsaved diagnostic override. This supersedes the opt-in requirement
at the earlier Casper checkpoint without changing its clips or physics.

Casper and Casper Flip are canonical experimental Flatland entries. Their guide
uses the same button and timing descriptors as the existing balance recognizer.
The guide includes Manual entry, return, held rotation and Ollie exit controls.
All tricks are grouped by type and alphabetized within each section; search,
type/status filters and the existing native Back owner are retained.

Portable production tests cover command-free entry, OFF/ON lifetime and the
same behavior without developer tools, plus guide controls and grouped browsing.
Exact Windows compilation and native visual/default-entry checks remain separate.
The Deck is reserved for owner testing of the separate wallplant candidate.
No wallplant merge, installation or hardware action is part of this change.


## Released-pop follow-ups

A quick B tap after an admitted A release now survives native takeoff and can
hand off the completed stick flick to the existing native grab adapter. B without
a released-pop proof retains its cancellation behavior. The original launch
ticket remains eligible while that grab is queued; no new impulse or rail launch
ticket is created.

Manual and Nose Manual jumps retain their distinct native selection identities
through the same handoff. The native flip bridge already supports both families;
the input lease now agrees with it and still rejects a change between them.
Ordinary rail-to-flip routing is unchanged.

The input-session regression includes an acknowledged native revert, established
Manual/Nose context, A setup/release and early X/B taps, plus rail pop-out.
It covers same-poll release, both stance flags, three polling cadences, repeated
polls and owner/focus/competing-action/bail/landing/stale-proof withdrawal.
The supplied native observations and callback receipts establish software routing,
not visible execution in the game. This revision still needs native grind-to-flip
and revert-to-Manual-to-jump-to-flip/grab acceptance on its exact DLL.

### Native rail admission between client ticks

Exact build 5ef44b1c was exercised on the Deck in Mega Park at verified native
30 Hz. Ordinary Kickflip and Manual -> revert -> Ollie -> Kickflip/BS Grab
passed native actor advancement plus rendered execution and landing checks.
Grind exits did not pass. Two attempts reached a native rail; one lost contact
before A release, and the earlier-release attempt exposed a readiness gap.
Two other approaches failed their native rail prerequisite and are excluded
from gameplay acceptance. Nose Manual variants and the final regression matrix
remain pending. The exact prior installation and all 95 rollback sets were
preserved after removing the temporary fixture and normally stopping the game.

In the earlier-release trace, A reached native state 403 while the client
permission still named the previous contact/action generation. The old
may_exit_grind required that delayed match and discarded the edge. A later
client read of the next rail class also withdrew the session before the
original physics callback caught up.

Readiness now admits the existing rail recipe from current owner-correlated
400..406 physics with mode <= 2. Same-owner transitions within those rail
classes defer destructive client publication without renewing the original
proof. Held rail departure, invalid observations, owner replacement, OFF/focus loss
and stale proof still withdraw authority. The existing native pop and follow-up
adapters remain responsible for execution; no geometry or launch force changes.

Source review reused the existing rail-pop contract, the full tracked official
source search, the pinned fork-method survey and the recorded native selector
evidence. A composed regression reproduces the lost edge on the preceding
headers and covers both stance flags and 30/60/120 Hz. The correction passes
1541 readiness checks under address/undefined sanitizers and all 33 focused
suites, including Casper. Its exact Windows build and native grind-exit
acceptance are still required; these local results do not establish a fix in
the game.

A second composed regression covers the client-ahead 701 exit after physical
A release. Only the existing released rail-pop lease may preserve the original
session across rail -> 701 -> 200/201 until the physics callback observes it.
This does not refresh that lease or admit a new jump. A held pop, unrelated
phase, landing, bail, rail recontact, unsafe mode and focus loss remain negative
controls. The regression fails without this continuation and passes with it;
exact-source native validation is still pending.
"
## Camera travel reversal

The existing shared follow camera normalized a linear blend of travel vectors.
Opposite vectors therefore stayed opposite: a 180-degree reversal could leave
the camera in front of the skater indefinitely. It now eases the shortest yaw
arc with the same response time. Position sampling retains elapsed time across
duplicate pose reads, so an adjacent camera callback cannot discard genuine
travel as an implausibly fast displacement. The eye still tracks the current
root directly; physical camera input and native camera restoration are unchanged.

The source review covered the integrated official source at 355bf71e, the
existing fork-method survey, follow_camera, CameraOrbit and controller/ground
motion routing. Forward input continues to request the existing native push;
this camera correction does not rotate physics bodies or redirect velocity.
Whether the reported backwards movement also needs a steering correction
remains a separate native acceptance question.

Focused tests cover reversals at 30/60/120 Hz, duplicate samples, closely spaced
callbacks, rest, teleport rejection, camera geometry, right-stick lookaround,
neutral return and input-owner withdrawal. Restoring the previous interpolation
makes the reversal regression fail. The camera suite passes 2,915 checks under
address/undefined-behavior sanitizers. The portable gameplay set passes 34 tests;
the lifecycle composition passes all 18 cases after removing the obsolete rail
admission local from its generated fixtures. Native visual acceptance is pending.

Before acceptance, the exact packaged DLL must pass Mega Park trials for
forward travel, reverse travel after a revert, 180-degree landings, right-stick
look/release and focus/OFF/controller withdrawal, correlated against native
position/velocity and the rendered camera. Grind-pop flip/grab and the remaining
Manual/Nose and stance regressions are still required on that same DLL.


## Native rail departure follow-up correction

The November 13 exact-source Deck run on fcf70d91 passed
revert -> Manual -> Ollie -> grab and clean landing. Continuing forward in the
same input session produced another 14.4 metres of ground travel with the
rendered camera following the skater. Earlier cases on that DLL passed the
corresponding flip, a native revert, left steering and right-stick look/recenter.
These bounded cases did not reproduce the reported backwards-control problem;
180-degree landing and the complete stance/cancellation matrix remain open.

The rail trial reproduced a remaining failure for both immediate X and B.
Native contact progressed through 401/403, exit 701, AIR 200/201 and ground 100.
The rendered result was a clean Quick Ollie with neither follow-up action. The
lossless input capture placed native departure 8ms before the physical A release;
the physics and animation selectors then chose Quick Ollie (31). The held writer
correctly cancelled at contact loss, but that also discarded the release proof.
The existing continuation and native flip bridge only recognized ordinary jumps.
The pinned native catalog independently identifies 31 as the Quick Ollie helper.

The correction retains a read-only observation of the original admitted setup.
It requires the same native owner and every forward physics transition, a
physical release within the existing 500ms input window after departure and
actual native Quick Ollie selection. It grants only follow-up recognition; the cancelled stick stays
neutral, and no new pop or ordinary velocity ticket is issued. Actual AIR,
ascent and existing native animation/actor admission still gate execution.
The native flip lease accepts Quick Ollie only when armed by this rail proof.

The device sequence failed the portable regression before this correction.
It now passes through both InputSession and the real facade. Negative cases
cover lost owner, focus, conflicting actor, late or absent release, another
selection, bail, stance change, trigger input and unowned Quick Ollie. A
quarter-second physical hold also retains continuously observed native takeoff;
holding A alone cannot request a follow-up. Existing held-setup contact-loss
checks remain unchanged. This correction has not yet
been installed or verified on the Deck; it is not an RC5 acceptance result.


### Vertical camera pivot

Native testing of the camera follow correction also exposed an upward-look
failure. The captured skater root is already near hip height. Adding another
metre placed the follow camera below the orbit's lower elevation limit, so
physical upward look could publish an unchanged frame. Orbit now uses that root
directly, preserving the supplied neutral view and the existing input, ownership,
angle limits and recentering behavior.

A production-body regression uses the observed root and camera heights, checks
both vertical directions and neutral recovery, and verifies that camera look
does not change the root. Restoring the former offset must fail at the specific
upward-look assertion. All 19 lifecycle composition tests pass locally.
This correction still requires its exact Windows build and native retest.

The preceding exact build passed native and rendered short-release grind exits
to flip/grab, revert-to-Manual exits to flip/grab, and a 250 ms held grind exit
to Kickflip. Casper direct and Ollie exits, horizontal camera orbit/recenter,
ground turns and a switch-landing continuation also passed their bounded cases.
The long-held grab attempt missed rail contact and is inconclusive. Stop/restart
attempts did not satisfy their native prerequisites; they do not establish that
the reported backwards restart is fixed. No blanket gameplay acceptance is claimed.

## Native Nollie rail-exit follow-ups

The continuous Impossible -> grind -> pop -> Heelflip trial selected native
Nollie 16 on the rail before states 200/201. Physical Heelflip recognition was
admitted, but the released-pop route accepted only Ollie 1 and Quick Ollie 31.
The existing native flip bridge already accepts Nollie; no new animation,
physics impulse or selector write is needed.

The rail episode now retains that observed selection for its released follow-up.
A different jump at or after departure is still rejected, as are owner, stance,
clock, bail and conflicting-input boundaries. Ground takeoffs retain their
original requested Ollie/Nollie match. Tests reproduce the recorded rail path,
including release before and after native departure, and preserve the existing
preloaded flip/grab negative controls.

Source comparison used official upstream 355bf71e, its launch contracts and
trick-gesture loader, plus the existing pinned fork-method survey. The upstream
loader skips from-grind recipes; the existing RC5 ownership route remains the
integration point. Device acceptance requires actual native Heelflip identity,
visible rotation and a landed continuation; recognition alone is insufficient.

The flatland test also reached a legitimate balance putdown before requesting
Casper. Retesting must apply ordinary balance controls and establish the second
Manual before X/Y. Preserve Fakie Manual after the revert, native balance
endpoints and the existing custom animation owner. No new native Casper scoring
or trick ID is claimed. Exact-source Windows and Deck validation are pending.


### Authored trick scoring and presentation

The custom flatland execution path now emits registry identities `casper` and
`casper_flip`, with distinct occurrence serials. A queued gesture is not an
award: the held entry must have reached the renderer, and the flip must have
reached its final pose while its native Manual episode and playback lease still
match. Repeated steps, stale ownership, missing render acknowledgement and
cancelled flips cannot mint another occurrence.

The existing combo model retains an Arcade bonus subtotal: 250 for entry,
100 per second of acknowledged hold on the original physics clock, and 500
for a completed flip. These are new mode tuning values. Each completed action
adds one combo multiplier without ending or reseeding Manual balance. The
subtotal follows the existing combo through its links, banks on its clean
landing terminal, and is lost on a bail or invalidation. It never supplies
native score attestations, progression, XP or engine trick enum values.

The existing owned score-line widget is prepared while the native root is idle.
During a scored custom combo it shows native history rows plus the custom
occurrences and adds the Arcade bonus to the native presentation. The native
amounts remain display values, not inferred score events. The original
visibility fields are leased through the existing reversible HUD hook; plugin
hiding, activities, menu transitions, expiry and Arcade OFF restore the native
presentation. No native score, trick row or collision contact is overwritten.

Source review reused the upstream owned HUD, native score reader and animation
renderer callbacks. The missing behavior was the connection from acknowledged
custom execution to the existing combo and presentation, not a native flip
selector. Focused regressions cover duplicate/invalid occurrences, rate-independent
hold points, flip cancellation, retained balance, native-score isolation,
bank/bail behavior and native/custom history ordering. Exact-source Windows
compilation and device acceptance of this combined revision are still pending.
