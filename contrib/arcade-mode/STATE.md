## Native grind fixture priority — 2026-10-14

Stop ad hoc approaches. Next milestone is a saved straight Park Editor rail,
measured native reset/heading/speed, native positive controls, then50 controlled
trials(10 native,20 Y variations,10 approaches,10 best-candidate repeats).
Primary detector is native400–499 physics, not screenshots or Y intent.
Observer runtime now wires the existing guarded enum reader to physics steps,
generation/owner checks, independent delivered-pad samples and bounded nonblocking
queues drained by a worker. It also observes Arcade OFF positive controls.
Raw enums remain UNKNOWN. Missing enum bindings do not erase valid physics.
56 observer/capture sanitizer checks and 225 Python tests pass locally. Observer
CI38675120210 passed all five gates; combined fixture CI38676253606 is building.
Installed runtime verification remains pending. No grind success claimed.

Reject maintenance-v31 ground(-40,55) as a fixture reset: the native topmost
surface path placed the skater on Stadium roof supports, not the skating floor.
Capture1792033069939264000 verifies the failure. Native map recovery returned
the skater to the Stadium entrance on foot, confirmed by capture1792035562147649000.
Game is alive and neutral, no test batch active. Do not reuse the roof candidate
or count it as a valid approach. Entrance terrain still needs fixture validation.
Installed source remains5ed2d873 and DLL92bc4e85 below; no new source deployed.

Fixture source0caed0e9fb7270ee220a33a29e60ad2a9dc6da0b is pushed. New fixture command
uses only the existing Park Editor transaction/save APIs and native Trainer
teleport. Exact installed catalog key own_bkrails_flat_00001 must be available;
no guessed substitute. Start lane uses short near-ground collision probes,
rail8m ahead, scale1, identity or quarter-turn orientation. Placement, spawned
object, reset heading/speed and actual native grinding still require Deck tests.
Saved fixture can be adopted after loading its existing park mod. HIL now accepts
zero screenshots while retaining input readiness and automatic neutralization.
HIL source77b3522 adds fixed native reset/status operations, fresh observer
readiness, before/after reset measurements and zero-screenshot trials. Missing
delivered input, dropped events, generation changes or unsafe reset stop a fixture
batch; ordinary misses and recoverable bails remain recorded results. Capture
the rail transform once and retain measured entry speed. Local tests pass;
external frozen HIL27 is staged, not installed. No50-trial matrix executed yet.

## Rotation results and verified grind installation — 2026-10-14

Current source/installed commit5ed2d873e3b2434c9999557ec656ad74aa7c43ac.
All five CI38667662491 gates passed; DLL
92bc4e858098b6070c0ba0f68d7831cdc9af0996fd2f37c165ad514b190eba41.
External grind-deploy-v1/deployment-result.json verifies unchanged stock/game
hashes, updates off and recovery clear. Frozen trusted-v26/maintenance-v30.
Game launch requested after the necessary verified DLL restart. Root owns Deck.

Rotation f297 batch1792030218835535000:20/20 executed, neutralized and idle
resets; no evidence gaps. Full-size native screenshots independently show:
180ms shoulder holds4/4 partial BS/FS Ollies;300/600ms holds8/8 BS/FS180;
prewind8/8 BS/FS360. Four right180 landings Clean; other16 Sketchy.
Delivered owned spans36–52ms,164–168ms,416–420ms and864–892ms respectively.
All ended unowned with neutral LS. This establishes functional shoulder rotation
in the stationary Stadium context, not moving/ramp/540 acceptance. Chord/OFF,
menu and bail regression remain outstanding. First capture4 attempt completed
one partial BS Ollie, then stopped before trial2 with input_readiness_lost;
end-only capture retry completed20. Capture load is not a proven root cause.

Next: restore Stadium1 at60Hz, native mount with Arcade OFF, enable Arcade,
targeted board-chord/OFF regression, then actual Y rail contact and A/X exits.
Y board holds/from-grind catalog selection are installed but still unverified
in native rail gameplay. Physical balance failure and combo acceptance remain
incomplete. Grabs remain deferred; retain their source and all prior evidence.

5ed targeted chord regression1792031590223483000:3/3 native100-504-100
round trips,2/2 single-shoulder exclusions,5 neutral/reset passes, no gaps or
rotation ownership. Native Y mount also worked with Arcade OFF before enabling.
Two native approach/brake trials1792031754413996000 and1792032569411147000
passed neutral/reset but remained outside the course behind crowd barriers;
no grind attempt or grind success claimed. Trainer where independently read
(-47.37,47.03,39.00),heading63,speed0. Reviewed maintenance-v31 adds only
fixed native ground reset(-40,55), a nearby candidate inside the course;
visual verification pending. No jump-height, velocity or grind-state forcing.

## Earlier rotation installation checkpoint — 2026-10-14

Source f29721f8ad0b56a4f1dffa1255d0c91fe607c976 installed after all five
CI38665858009 gates passed. DLL
199f9bdd38d080f473d7ee22b460c3451533f3631fd02bba3d503defcb5f4271.
Stock/game hashes unchanged, updates off, recovery clear. Receipt external
rotation-deploy-v1/deployment-result.json. One necessary DLL restart; no stock
restore. Frozen trusted-v25 and maintenance-v29 pin this exact installation.
Game launch requested; restore the Stadium practice baseline and simulation60
before shoulder-rotation-f297.json. Root owns hardware; no active grab work.

Latest pushed source5ed2d873e3b2434c9999557ec656ad74aa7c43ac adds basic Y
and authored rail exits; CI38667662491 pending. This newer grind source is NOT
installed. Rotation native acceptance is first. The native forward/brake pilot
1792028815212335000 reached Stadium center barriers, then stopped cleanly using
2 seconds B: max8.46m/s, final0, neutral/reset PASS. One earlier attempt was
skipped off-board after map travel; no input sent. Use an actual low rail/ramp
for grind acceptance, not a crowd barrier or fabricated engagement.

## Active priorities and hot Deck checkpoint — 2026-10-14

Rotation source f29721f is pushed; CI38665858009 Windows build pending,
Linux/macOS and hosted installer tests pass. All220 Python tests pass locally.
This checkpoint prepares the next milestone: basic Y neutral/nose/tail board
hold across native air-to-rail contact, actual from-grind A/X catalog selection,
and physical-button exclusion for the board chord. No geometry/physics forcing
or named-grind success claims. These local changes are not installed. Finish
rotation hardware acceptance before promoting grinding.33,579 model and48
playback sanitizer checks pass. No new native grind acceptance is claimed.

GRABS DEFERRED — RETURN AFTER ROTATION, GRINDING, HUD AND BASIC COMBO INTEGRATION.

Preserve all grab source, tests, mappings and the a970167 synthetic ownership fix.
Insufficient practical airtime remains an unresolved gameplay-context limitation;
it does not establish that native grab recipes are incorrect. No further grab
batches or physics changes are planned in this milestone.

Source parent f29721f8ad0b56a4f1dffa1255d0c91fe607c976; installed source
f1ce40c22a0a908480a891661b3f1d0bbe19427f, DLL
3bb57725475257d26e1d1912ef7d821b3ea1fbf3bc292446bb6437c56a3b726c.
Game remains hot in Stadium1, Arcade ON, auxiliary actions OFF, simulation60.
Root owns hardware; trusted-v24 and maintenance-v28 pin this installed build.
The 20-trial manual balance batch finished:20 executed,20 neutralized,20 idle
resets, no evidence gaps. All10 Manual and10 Nose Manual trials showed the
matching native pose and directionally correct model correction. This remains
one stationary context, not moving-chain acceptance. Result1792027038994591000.json.
Earlier four sustained manual/nose-manual cases confirmed native poses and live
vertical HUD correction in both directions (result1792026779741841000.json).
Full limit-position and physical balance-failure acceptance remain outstanding.

Next: install the verified rotation build and test continuous LB/RB with the
existing120ms board chord; then basic Y rail engagement,
actual grind/manual HUD and physical failure, then basic combo composition.
The local LB/RB actuator now supplies native LS rotation and crouched prewind,
preserving the120ms board chord and release-to-rearm.33,560 model checks pass.
Raw native method result1792027415513923000.json:6/6 spins independently visible,
BS/FS180 for200/600ms airborne holds and BS/FS360 with prewind;6 neutral/reset
passes, no evidence gaps. Every landing was Sketchy. These are native method
discoveries; shoulder mapping, short-press control and larger spins remain
unverified. No unverified source was installed. Game remains running.
Earlier CI38663884831 passed all five gates; its source is not installed.
All older next-step notes below are historical and superseded by this priority.

## Grab ownership and HIL readiness fixes — 2026-10-14

Latest pushed source1d90357; installed f1 remains hot in Stadium1, Arcade ON,
actions=grabs, simulation60 requested. Hardware root only. Diagnostic fix19776cf
passed all CI gates38661373108; combined preparation fix38662070079 still builds.
Six stationary Classic B trials completed (Melon/Nosegrab/Tailgrab twice),
0 independently verified named grabs,6 neutral/reset passes. Generic infrastructure
stop occurred before a seventh result; game/launcher and installed hashes were
confirmed intact afterward. Preserve trusted-v23/results/1792025749679033000.json.
The runtime retains authored ownership for32 neutral settling ticks, preventing
airborne B handoff. Local fix yields only an already-delivered neutral tail to
B in observed air with experimental grabs enabled; unchanged for ordinary flips.
33,510 model and48 playback sanitizer checks pass. Native retest still required.
HIL now has optional matching-heartbeat speed bounds for setup AND reset, plus
allowlisted stop reasons/trial identity. These local changes are not yet frozen
into the trusted runner. Keep pending changes separate from installed claims.
Next: complete CI for the combined fixes, verified deployment, then RT/grab and
manual-chain retests from the clear Stadium baseline. No city moving acceptance.

# Arcade Mode persistent project state

## Ground-preparation combo regression found — 2026-10-14

Installed f1 remains running, CI for19776cf diagnostic fix is building.
Manual command retry:3/4 executed,1 skipped; one independently visible Manual
and two exact native Ollie1 results,2 reset passes,1 traffic/bail reset failure.
All executed inputs neutralized. No continuous-chain or Nose Manual acceptance.
Evidence and earlier4 skipped cases are in manual-revert-controls.json.
Both Ollies showed native100/mode0 then103/mode0 before takeoff200/mode0;
existing pose mapping invalidated the combo at that legitimate preparation.
The targeted local fix treats those ground preparations as riding while retaining
strict action-start gates.33,504 model sanitizer checks pass; Deck retest pending.
City sidewalk baseline is unsuitable for moving batches facing traffic. Move to
the existing Stadium practice map and establish a clear lane before further trials.

## Verified native runtime installed — 2026-10-14

Source and installed commit `f1ce40c22a0a908480a891661b3f1d0bbe19427f`;
DLL `3bb57725475257d26e1d1912ef7d821b3ea1fbf3bc292446bb6437c56a3b726c`.
Development CI38657376204 passed all five gates; hosted Deck simulations38657376177
also passed. Existing game and stock hashes below remain unchanged, updates off,
recovery clear. Receipt: external native-actions-deploy-v1/deployment-result.json.
Trusted-v23 and maintenance-v28 now pin this installed identity. One necessary
restart for the verified update; no stock restore. Root owns hardware.

Native community method has6 independently visible revert results in8 executed
moving trials: FS, BS, Switch FS and Switch BS labels. Two regular attempts at an
obstacle produced no verified revert. Nollie-clockwise2/2 produced FS and Switch BS;
two counterclockwise cases were skipped when riding context was lost. These are
source-method discovery, not20-trial repeatability or Classic RT acceptance.
Eight subsequent raw grab cases were all skipped: no gameplay claim from them.

Short native B braking does not stop coasting (observed2.38m/s after reset), so a
riding heartbeat must not be called stationary readiness. Walking near stadium
props did not establish a useful baseline. Two rejected Trainer position candidates
were tried; game returned to the clear native stadium entrance. Do not reuse those
coordinates as an accepted practice reset. Full external artifacts are preserved.

Pending local collector changes expose bounded native scoring events/status and
HUD amounts, with owner/event identity explicitly unverified;216 Python tests pass.
First Classic RT block:1/2 new native FS Revert labels (score5);2/2 neutralized
and returned to riding. Second trial retained the prior label, not a second success.
The native score observer bound and captured sequence amount5 plus line start;
local ownership remains unverified. Evidence trusted-v23/results/1792024652125144000.json.
Idle action cadence on two controller slots crowded out input evidence in both
trials. The local fix retains changed values and cadence during actual ownership.
Switch BS Revert exact label alias added from the earlier native screenshot;
21 score-edge sanitizer checks and216 Python tests pass.
Next: manual-to-Ollie trials on installed f1; build the diagnostic fix before
repeatable RT/grab delivery analysis. Keep game hot
between trials and preserve all current result denominators.


## Native revert discovery and HIL timing repair — 2026-10-14

Source checkpoint continues `2ea4337710ea7fb7fa6281f3dcbe4229e6345be8`.
Installed c1e2a79 DLL/hash below remains unchanged; game remains hot in Stadium1,
Arcade Mode OFF, mounted, controller neutral. Native Classic preset independently
confirmed; no native gameplay settings changed. No Comply is disabled; other
recorded settings and evidence are in community-revert-source.json.

The community235ms recipe now produces actual FS Revert and Switch FS Revert
labels on the Deck:2/2 moving trials, both neutralized/reset. This is DISCOVERED,
not20-trial repeatability or RT integration acceptance. Stationary requests did
not establish native reverts. Four native Ollie positive controls passed exactID1.

Measured a runner defect before judging the recipe: full process-list scans on
every5ms angular step stretched235ms to339.427ms. Moved those same read-only
checks to a background observer; stale100ms evidence, changed capture owner,
failed capture or session deadline stops input. Existing identity checks,
watchdogs and neutralization remain. Real Deck retest delivered234.985ms with
max0.258ms write lateness; native Ollie still passed.215 Python tests pass.
Frozen external trusted-v22 is current; maintenance-v23 uses reliable console
key timing. Native game polling is a distinct boundary from uinput write timing.

CI restored by the user after account-level blocking. The development run
38656982739 for2ea4337 is active; Linux/macOS gates passed, Windows build pending.
No unverified DLL deployed. Next: stance/direction revert trials, then install the
new runtime only after complete build/package gates and test the actual RT path.


## Live Deck resumption and score observer — 2026-10-14

Deck reconnected. Installed the newest fully gate-passed artifact, source
`c1e2a79c51d92f0575232462767d3f299be0492a` (CI `38636563486`), DLL
`7ce21465ecbf83234ecdd2b191d715d20a58a5eae09c77c666092f1324cf2834`.
Stock backup remains
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`;
game remains build25414733, executable
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
Updates remain disabled, recovery is clear. Game is running; root owns hardware.
Frozen external `trusted-v18` uses one controller per batch, exact current pins,
and the locally validated208-test persistent runner. `maintenance-v20` supplies
fixed reviewed console operations. Deployment receipt is in external
`plumbing-deploy-v1/deployment-result.json`.

Console independently reported forced simulation30 and time scale1. Changed
forced simulation to60 to match upstream recorder, with console confirmation.
Current controlled baseline is mounted, stationary on the Blue Light Beach spawn
sidewalk, physical LS/triggers neutral. Both eight-trial A/B blocks completed: delivery5/8 exact IDs; reference5/8
exact IDs, including one inconclusive final off-board/reset failure with tail
overflow. First15 trials have complete timing and no dropped rows; matching
relative tick/XY trajectories exclude schedule stretching in this baseline.
No playback default promoted. See research/playback-parity.json.

Loaded Stadium1 for clear follow-up. The map transition reset forced simulation
from60 to30, independently confirmed in console; reapply60 before comparable
trials. External trusted-v19 adds the reviewed optional end-only screenshot and
256KiB bounded evidence window (211 offline Python tests pass). Identity checks,
neutralization and timeouts remain unchanged. Preserve the running game/DLL.

The pinned GPL-3 native line/sequence observer is now implemented locally with
exact graph/schema validation, bounded resumable worker discovery, nonblocking
fixed queue and background sanitized logging. All66 sanitizer checks pass,
including40,000 concurrent publications. Capture serials are not native event
IDs; two line-end routes can duplicate events, sequence category and ownership
remain unverified. No score writes or combo decisions are added.

Stadium reference follow-up:6/8 exact IDs, both Impossibles selected4; all8
neutralized/reset with complete timing. End-only screenshots reduced artifact
size to25MB. The24-trial reference repeat block completed:Kickflip8/8,Hardflip7/8,
Varial Kickflip5/8 exact; all24 reset with complete timing. Combined Stadium
reference denominators are10/10,9/10,7/10 respectively, and Impossible0/2. Upstream recorder accepts onlyIDs1..32, so its27/27 evidence cannot
cover Impossible36. Read-only grind asset/page plumbing also passes45 sanitizer
checks; runtime discovery/wiring and named enum mappings remain pending.

Newer runtime code, including grab/revert/manual plumbing and the score observer,
is not installed. Actions remains externally disabled, so fresh Windows/package
verification is blocked; no settings were changed and no unverified DLL deployed.

## Full unfinished-requirement source review — 2026-10-14

Continues source `2f941376a3ec9f3972e2095fb5d8171022ee0a3a`. Expanded the existing
PR/fork survey to standalone repositories, upstream issue attachments and official
native documentation. Every63 action row now has a source/fallback review; the same
native-method matrix covers40 broader requirements, including all eight chains.
This checkpoint changes documentation and source classification, not runtime code.

Strongest new reuse path: GPL-3 native line/sequence event capture from
`SmokeyDawg58/reskate-custom-mods@de837826417b55e2a5756c303f4805ae6a39ae89`.
Its validation records actual native22-point bank, rejected840-point bail and11
sequence amounts totaling18,518. This is external Windows evidence on the pinned
game build, not our Deck acceptance. Extract the read-only expression observer;
omit multiplier writes and its different bail policy. Local attribution, event
deduplication and Arcade lifecycle integration remain required.

GPL-3 `GoalTrains/reskatedualstick@1181aa819bea2eb49042b3f67e6ff5c7c9c4e348`
adds concrete fallback recipes for an LS-only revert, spin wind-up and rail input
continuity. The existing235ms revert remains first to test. Official native
Darkslide input and held-flip/composite-record data reduce the need for new research.
No reviewed source supplies the complete exact named Y table or four missing grabs.
The additional source attachment has mostly offline evidence; an apparent second
translator port is empty. All distinctions and license limits are in the existing
survey. Missing methods remain unresolved, not falsely declared unsupported.

Fresh development workflow dispatch for2f94137 was rejected by GitHub because
Actions is disabled for the repository. No settings were changed. The earlier
Windows test-warning fix has local verification only; a fresh Windows/package gate
is still required before deployment. Deck reachability and recorded installation
remain unchanged: source6a8db43, DLL
`8bcf84f8affa3961d9422e4958babd93ae88e600e91b01aca956a154dad353b5`.
No controller session, deployment, restart or restore occurred. Current live game
state is unknown; the last installation receipt reported game/launcher stopped.

Next hardware task is still controlled flip A/B after fresh identity/build checks.
Next source integration is the bounded read-only native score-event path, followed
by native grind observation/bail requests and the pinned control recipes. HUD,
physical balance failure, real combo/fatigue chains and mixed stress are unaccepted.
No full gameplay completion is claimed.

## Current source reuse checkpoint — 2026-10-14

Continues `55a6303f062c2832cdc93409970e2d37d99b29a9`. Surveyed 20 open PRs,
103 closed PRs and the 180 returned public forks; inventoried 362 branches across
179 external forks, deduplicated 264 heads and screened their differences. Relevant
implementations and licenses are pinned in `research/upstream-method-survey.json`.
This is targeted source review, not line-by-line review of every fork.

PR #217 supplied native line-score/multiplier mappings. The existing observer now
reads them with independent validity, strict full-consumption parsing and bounded
sanitized telemetry; no raw UI strings or fabricated scores. These are presentation
snapshots, not bank/trick events, and local ownership remains unverified. Forty new
C++ parser checks pass with sanitizers; all 208 Python tests pass.

Other concrete reuse paths: Trainer state-page readers for native grind enums;
bounded native bail requests and actual ragdoll-entry observation; existing authored
from-grind playback plus the already-integrated grind exit hook. Exact named Y
selection, enum mapping and native acceptance remain open. PR #232's actual current
diff only affects PlayStation HID; it does not solve Deck grind selection. PR #230
is unchanged, so prepared flip A/B remains the first hardware task.

Baseline Windows run `38639254535` failed on a narrowing warning in a manual test;
the test now uses an explicitly typed button array and passes strict local checks.
A fresh Windows/package gate is required for this change. No new DLL was deployed,
no Deck trials occurred, and the recorded installation/network blocker below is
unchanged. No frozen runner, security settings or native gameplay tuning changed.

## Current runtime and HIL integration checkpoint — 2026-10-14

Runtime source is `53b13bb519858e07087d70f5f4c3c1e4d9a57ca8`, continuing
`c1e2a79c51d92f0575232462767d3f299be0492a`. The formerly preserved
manual implementation is now reviewed and integrated, including fixes preventing
balance corrections from becoming fresh manual commands and cancelling on native
takeoff/bail/grind/opposite manual or conflicting controls.

The unsaved `arcadeactions off|observe|grabs|revert-fs|revert-bs` selector
requires Arcade Mode OFF to change and defaults to `off`. Candidate B grabs and
RT reverts reach the actual remap through explicit channel ownership and packet
progression; A/X/manual/board owners take precedence. Only the inspected clear-bit
stance is admitted. A separate native-state freshness gate preserves revert
playback through crouch state 103. Native recognition is not inferred from requests.
Delivered action axes, triggers, packets and request/result fields enter the
existing bounded evidence queue and sanitized lab collector.

The score observer tracks fresh allowlisted label appearances continuously,
separate from its capped schema diagnostics. Baselines, gaps, coalesced changes
and persistent text cannot create repeated trick events. These are collected as
label evidence only; combo continuation remains disconnected until HUD slot roles
and real native revert outcomes are verified. No score amounts are invented.

The repository's HIL batch path now reuses one virtual controller, kernel/SDL
observers and capture preflight. Trials retain identity checks, fresh idle
baselines, independent evidence, neutralization and bounded reset checks. Ordinary
recognition/reset failures remain recorded results and do not abort the batch;
integrity/transport failures close the session. Each replay retains its 25-second
guard, with at most 24 trials and an absolute 1,100-second device lifetime under
the existing 1,200-second transport timeout. Single-run behavior stays bounded
to 25 seconds. The frozen external runner has not been changed: a reviewed bundle
refresh and live verification are still required before using this improvement.

The trusted read-only Deck status retry still failed. A paired no-op SSH check
confirmed `steamdeck.local` still cannot resolve. No input, deployment,
restart or restore occurred. Recorded installed source/DLL remain those below.
This runtime source's hosted simulated Deck suite passed; its Windows development
build was still running when inspected. Flip A/B remains the next hardware task,
followed by native action candidates and manual transition checks. External runner
updates have not been installed. Persistent stream transport, native grind/spin
integration, physical balance failure and final chain/stress acceptance remain open.

Local validation: 41 runtime action checks, 75 manual checks and 19 label-occurrence
checks pass with sanitizers, alongside the prior C++ suites and 33,500 model checks.
All 206 Python tests pass, including action/score-edge parsing, serialization,
persistent-session cleanup and the actual trusted batch entry point. Delivered
request evidence cannot satisfy native trick criteria. The naming guard passes.

## Current plumbing checkpoint — 2026-10-14 evening

Continues source `3a3baebeae7a3f97105f2f1ace32ee0e492b2239`; the recorded
installed DLL and hardware blocker below are unchanged. No Deck input or
deployment was performed. Flip A/B remains the first hardware experiment.

At the user's request, offline implementation now also covers:

- Exact B/Y command selection and repeated direction handling, with missing
  geometry/methods reported explicitly.
- Twelve bounded grab candidates from the existing native guide and the
  independently written community revert candidate. A default-disabled dispatcher
  reserves input channels, excludes other owners, and cancels on stale context,
  incompatible native state, conflicts or reversed time. These are not live B/Y/RT
  bindings and cannot prove native success. Grab timings remain experimental.
- Independent native-event/request correlation, with epoch/sequence freshness,
  one-shot observed revert handoff, and reset/bail invalidation. A native occurrence
  adapter is still needed; polling a persistent selected ID is not a new event.
- A larger model-driven HUD: horizontal grind and vertical manual tracks span
  30% of the respective display dimension, with warning/severe zones and stronger
  cursor, center and boundary marks. Gamescope screenshots remain unverified.
- An offline A–H chain evidence grader requiring independent native events,
  coherent combo/fatigue/banking state, recovery and repeated contexts. Its live
  semantic telemetry adapter is pending; synthetic grader fixtures are not Deck
  trials. Missing evidence cannot pass. See `lab/chain_report.py`.

The existing manual runtime changes remain in the working tree, preserved from
recovery. They pass the combined local model tests but are not promoted to Deck
acceptance. Existing combo dynamics, trusted runner, security configuration and
playback default are unchanged. Native grind actuation, spins, physical balance
failure consequences, score values and persistent stream transport remain open.

Validation: 124 dispatcher/command checks, 192 revert recipe checks, 41 event
handoff checks, 122 HUD geometry checks, 40 playback checks and 33,500 working-tree
model checks pass with address/undefined-behavior sanitizers. HUD syntax passes
against the bundled ImGui. All 191 Python tests pass, including ten chain-grader
contract tests. All new C++ suites are included in the existing hosted
Arcade test target. Native repeatability denominators for the new candidates are
zero; no milestone is marked DISCOVERED or ACCEPTED from these local tests.
The staged source without the preserved manual edits also passes all new suites
and 33,480 existing model checks with sanitizers.

## Current resumption checkpoint — 2026-10-14 afternoon

This section supersedes the historical status below. Security work is complete
and is not an active gameplay milestone. Local and remote `arcade-mode` were
both `6a8db432d3d90f4179f55a50d7d3413eabd59cf8` at recovery. The other existing
worktree remains detached at `42102ae`; no files there were changed. Earlier
uncommitted manual adapter, score-label and grab-guide work is preserved separately.

The latest **recorded installation**, newer than the old handoff, is source
`6a8db432d3d90f4179f55a50d7d3413eabd59cf8`, DLL SHA-256
`8bcf84f8affa3961d9422e4958babd93ae88e600e91b01aca956a154dad353b5`.
The private `arc-observer-deploy-v1/deployment-result.json` reports the game and
launcher stopped at installation, unchanged game/stock hashes, updates off,
active receipt and no recovery pending. This is recorded evidence, not a fresh
claim that the game is running. The external v17 and v3 runner pins still target
`d0c835c`; they must be reconciled against a fresh installation read before input.

**External blocker:** paired SSH cannot resolve `steamdeck.local`. The trusted
status request failed, and two explicit paired connectivity checks failed with
the same resolution error, including after the configured three-packet wake
request. No controller input, game close/restart, deployment, or stock restore
was performed in this continuation. Wake/network reconnection is requested.

The earliest incomplete requirement remains flip repeatability. The latest
recorded d0 regression is 16/24 exact native IDs, including eight failures; all
24 neutralized and returned to idle. No new Deck trial has been run here.
Authored compilation already matches the pinned Style Editor reference. The
demonstrated difference is delivery-relative rescheduling versus an absolute
game-tick timeline. The reference also increments packets every poll and replaces
the complete scripted pad. This does not yet prove which difference causes the
native misrecognitions.

The current change adds unsaved `arcadeplayback delivery|timeline|reference|off`
diagnostics, selectable only while Arcade Mode is OFF. Ordinary startup behavior
is unchanged. Timeline isolates absolute scheduling; reference also uses per-poll
packet progression and full-pad replacement during authored flips. Freshness,
menu cancellation and bounded expiry remain in force. Per-tick/changed-output
numeric evidence includes cumulative poll count and delivered packets; the
existing bounded collector now parses it. The external runner was not updated.
See `research/playback-parity.json` for the exact comparison and safety differences.

Local validation: 40 focused playback/packet checks and 33,500 working-tree model
checks passed with sanitizers; the isolated commit without the preserved manual
work also passed all 33,480 model checks. All 166 Python tests passed. These are not native
acceptance. Controlled Deck A/B remains required.

Playback implementation `6ec5e58ac99d436b421b2b56a65e60b264cc552f` is pushed.
Hosted run `38630732455` passed all five gates: Windows build/model tests,
Linux, macOS, packaging tests and publication. No new DLL was deployed.
The A/B artifact is cached privately at `artifacts/playback-parity-6ec5e58/`.
Its DLL SHA-256 is
`fafea7d19e8c1d7f8b3829dafaa514f5653fdbaa30306f81fecb8886531383a5`.
The immutable package and raw CI artifact contain byte-identical DLLs; their
source and checksums were verified locally without executing downloaded code.
The offline `lab/playback_report.py` follow-up joins the
sampled cadence/stick evidence and checks packet increments against cumulative
poll counts. Fifteen focused analyzer tests pass, and the full Python suite now
passes 181 tests. Native IDs, timing completeness,
neutralization and reported reset are kept separate; no requested action becomes
a success through this analyzer. Existing results without cadence evidence cannot
pass the new A/B evidence check.

The exact community revert repository and revision are now recovered. The
verified source recipe is 235 ms, including 33 circle samples rather than 32.
There is no compatible source-copying grant, so use the behavioral recipe for an
independent implementation. The native-method matrix now classifies RT as
COMMUNITY METHOD AVAILABLE, without a Deck-success claim. See
`research/community-revert-source.json`; its source uses human verification,
not native observations.

**Resume:** reconnect Deck; read fresh status/trace; reconcile the reviewed runner
pins with the installed artifact; complete the hosted build gate before any new
DLL deployment; restore a safe stationary scene in the same game session where
possible; run paired reference/delivery trials for Kickflip, Hardflip, Varial
Kickflip and Impossible under measured 60 Hz simulation and identical stance.
Use timeline mode only to isolate any reference improvement. Preserve every native
ID and reset outcome. Do not promote a playback fix without real trials. Then
evaluate the verified revert recipe, continue the preserved manual/grab work,
and proceed to grind, HIL throughput, HUD and combo/stress acceptance.

## Security-first sequence — 2026-10-14

Active work follows SECURITY.md, then live Arcade Mode toggle, automatic skating,
board chord, exact controls and native-method integration, revert, persistent
HIL, readable balance HUD, and finally combo acceptance. Earlier combo experiments
below are evidence, not permission to skip these gates.

The real game remains running with the development DLL; no stock restore.
Use the reviewed external lab runner, never persistent approval of checkout code.

Security checkpoint 451a5ce passed all hosted jobs in run 38527591836, including
the isolated publisher. GitHub rulesets are active and the owner push path works.
SSH signatures verify locally; GitHub reports unknown_key until public signing-key
registration. Do not enforce signatures yet. See SECURITY.md for the two account
actions and the access audit.

Current installed source is d0c835c18af48cee21554453a12c41998706dee8, all five
gates passed in run 38590114296. DLL SHA-256:
7ae0c5c0828ca4da34b32d3dbb4699d15e916404d223137439241fc56127149d.
The game executable and stock backup hashes remain unchanged, automatic updates
are off and recovery is clear. Reviewed runner v16 and lifecycle helper v16 use
this exact target. The late-poll correction passed its local regression but the
24-trial Deck matrix still found intermittent diagonal recognition failures and
four Impossible requests that produced shove-its. Every trial neutralized and
returned to idle with no gaps or dropped flip rows. The fix is not sufficient
for repeatable exact recognition. See research/playback-regression-results.json.
Bounded authored-arc and delivered-stick diagnostics are the next investigation.

The preceding 53593c4 build also passed all gates. Its Impossible mapping failed
four native trials: regular requests produced ID 4, nollie requests ID 19;
Kickflip/Heelflip controls passed and all six trials neutralized/returned to idle.
The current-game Skatepedia confirms a full right-stick circle from downward
setup toward the heel side, with no extra button modifier shown. This supports
re-testing the missed-arc correction before changing the native recipe.
See research/impossible-control-results.json. The native guide is accessible by
pausing twice, selecting its third side-menu row, then choosing Flip Tricks;
Impossible is the sixteenth entry. The visible side-menu Get Unstuck action may
provide a native reset later, but has not been tested.

Earlier installed source was f47e6cfd0a8bc5e2f118e09d3cc7f8704910432d, all five
gates passed in run 38579226003. DLL SHA-256:
3dec1862d95b44218435374537a8c8e087aa682f1e3c26b127993be65b056178.
The game and stock backup hashes are unchanged, updates are off, and no recovery
is pending. Reviewed runner v16 and lifecycle helper v13 match this target.
The game remains running in Stadium1 after the necessary DLL restart.

The reflected authored adapter passed all seven mapped primary flips with exact
native IDs: Kickflip 2, Heelflip 3, Pop Shove-It 4, Hardflip 9, Inward Heelflip 6,
Varial Kickflip 5 and Varial Heelflip 8. Left/right D-pad also produced IDs 2/3
without opening the native menu. A hold/release passed 20 controlled trials:
seven 200 ms, seven 500 ms, six 1500 ms holds, all native Ollie ID 1.
Every trial neutralized and returned to idle, with no evidence gaps or dropped
flip rows. This establishes A repeatability in the stationary test context;
flips remain discovered until repetition and practical context variation pass.
See research/reflection-control-results.json. The nollie/repeated-input matrix
produced 17 exact results from 19 mapped requests. Two inward-nollie requests
produced plain Nollie ID 16, but both later succeeded on retry. Five unmapped
exact double/triple variants produced no substitute native flip.

Repetition exposed intermittent diagonal failures: 17/21 primary requests passed;
four produced simpler tricks. D-pad timing variation passed nine initial trials,
then Hardflip produced ID 9 but collided with spawn barriers and bailed (504).
Fourteen subsequent cases skipped without input because the riding baseline was
lost. A map reset is recovering those cases without restarting the process.

A local playback regression explains one possible cause: absolute catch-up could
skip a one-tick authored arc when input polling missed a game tick. The correction
delivers each arc on a distinct observed tick, preserves release spacing and the
absolute expiry bound, and passes 33,480 sanitizer checks plus strict warnings.
The defect is reproduced locally; its role in the Deck failures is not yet proven.
The correction still requires hosted build and Deck deployment. Do not promote flip repeatability.
53593c4672c565c84e459eeae38555be51ecb93b (Impossible and score-label observer)
is pushed and in CI; f47 remains the installed target until all gates pass.

The catalog contains 92 valid patterns. Four rejected assets have an arc minimum
above its maximum (stage 7, field 13, error 2), so the catalog remains partial.
Fixed numeric authored-label tokens establish Impossible IDs 36 (regular) and
38 (nollie); 37/39 identify different foot variants and are not substituted.
The exact Impossible mapping is added locally but has not been deployed or HIL
verified. See research/authored-flip-catalog.json and native-method-matrix.json.
Null matrix classifications mean source review remains incomplete, not that an
exact equivalent is absent or that new reverse engineering is warranted.

On the preceding d799 build, A hold/release worked twice but lateral flips had
opposite native IDs. The f47 correction reflects authored X for the measured
clear-bit stance using the existing guarded stance flag. Other stance contexts
remain unverified. The older 3fa timed adapter produced twelve Kickflips and
twelve Heelflips; those counts do not establish repeatability of the new adapter.
The d799 failures remain in research/authored-control-results.json.

Physical right stick controls the camera when the authored catalog is active,
using the existing guarded native camera lease. On d799, yaw worked both ways;
stronger two-second pitch holds after controller activation moved up/down.
Pause restored the native camera. Movement, collision and repeated restoration
remain open. See research/camera-results.json. A live OFF check on d799 restored
native A and right-stick behavior without restarting; ON was then restored.
The f47 A repeat batch followed a DLL restart, so it is not counted as the same
live-toggle cycle. See research/live-toggle-authored-results.json.

The next change also adds an exact 20-label diagnostic allowlist for native
manual/revert/grab text, with a separate numeric label event. It does not infer
grab success from input or generic numeric score fields. Unknown text is excluded.
Local validation: 162 Python tests and 33,475 C++ sanitizer checks pass.

Previous installed source d93dcef1822d1a0eccb660bb7b9508e86d666059 passed all five hosted
jobs in run 38554517047. DLL SHA-256:
1dda9303d02e018edf67932dc32b5e36e95bff19315fb6bba571fb0938f0df1b.
Game and stock backup hashes remain unchanged; updates are off, no recovery pending.
This adds bounded game-facing pad edge evidence to diagnose the trigger boundary.
External trusted-v5 added numeric pad observation parsing with independently
reviewed installation and exact target pins. The necessary game restart for the
new DLL completed and Stadium1 was loaded, with no stock restore. The pad matrix
showed that requested LT/RT levels never changed the game-facing trigger bytes;
the failed chord arrived as LB+RB with both triggers zero. Treat prior
trigger-dependent HIL checks as unverified. Runner v6's standard Linux controller
version identity did not fix this. Reviewed v8 now observes only the kernel event
device created by its own live uinput descriptor, after hotplug permissions settle.
Numeric trigger samples and bounded failure codes distinguish device transport
from game-facing translation. Runner v10 additionally confirmed correct full-range
trigger values through the system SDL controller API. The standard device name
and v11 wider-range experiment did not fix game-facing translation; the wider
range was removed. No shortcut-specific binding override was found in the bounded
inspection. The next DLL adds numeric source edges before existing ReSkate filters
to locate the remaining boundary. Chord semantics remain unchanged.

Source 4081eccfe0a7c0a347262c49617a88983cb2473f passed all five hosted gates
in run 38562178690. It is not installed; d93dcef remains hot. A single bumper
press/release before later trigger ramps made both game-facing trigger bytes
reach 59/127/255 and return to zero. A shorter lead failed again, so this is not
yet a repeatable readiness guarantee. A later full LT ramp produced native
off-board state before the chord; the adapter emitted no synthetic board pulse.
Clear Stadium1 follow-up passed two partial-trigger smoke cases and all 20 repeated
cases (10 LT, 10 RT): the game received 59, the chord emitted no synthetic Y,
releasing the trigger while holding both bumpers did not trigger a delayed toggle,
and native riding/neutral input remained stable with no evidence gaps or drops.
The fixed bumper activation and 800 ms trigger lead are part of this tested setup;
general hotplug readiness and full-trigger native behavior remain separate open
cases. See activation_followup in board-toggle-results.

Source 81c7f910074ba4b2ec32c03c4e9173fa536cb776 adds the existing upstream
authored flip parser and guarded native FlipTrickType observation, with asynchronous game-data loading and
bounded numeric catalog/identifier telemetry. This prepares exact native mappings;
it does not yet change the timed input adapter or claim full trick support.

Windows CI 38565068892 rejected a shadowed loop variable under warnings-as-errors.
The parser variable names are corrected. No failed artifact was deployed.
The follow-up also prepares bounded authored flick compilation, hold/release tick
playback and exact native-ID/context selection, with 32,487 sanitizer checks and
156 Python checks passing. Playback is not yet connected to the live adapter.
Reviewed external runner v13 adds only bounded numeric catalog/native-ID parsing;
it is installed with the existing d93 target pins pending a verified new artifact.

The real Deck live toggle gate PASSED in one game process: ON A produced native
100 → 103 → 200/201 → 100, OFF A remained 100 and rolled normally, and ON A
again produced the air/landing sequence. All three trials returned to idle with
neutralized input and no dropped physics samples or evidence gaps. The Player
menu shows the setting; controller A toggles it without triggering a trick in
the menu. See research/toggle-live-results.json. This is one acceptance sequence,
not final control/combo acceptance. No restart
occurred between the three settings. Private full-size captures remain ignored
under artifacts/toggle-acceptance. The tested A gesture still uses its existing
timed recipe; hold/release Classic Controls comes at the control-table milestone.

An additional controlled flat batch passed 20/20 A air/landing trials with neutral
input and idle restoration. A city batch first passed once, then hit traffic and
skipped 18 offboard cases; neither failure nor missing context was counted as a
success. See research/toggle-repeat-results.json. The saved ON preference survived
the natural restart for the automatic-skating DLL.

The automatic-skating adapter uses guarded
native horizontal speed, ordinary riding/two-wheel contact eligibility, lower/upper
speed thresholds and bounded native push-button cadence. It never writes velocity.
Pure tests cover cadence, hysteresis, menu/toggle rearm, conflicts and stale state;
runtime speed parsing is numeric and bounded. Basic Deck acceptance passed: forward
intent accelerated from rest to 5.49 m/s, the identical OFF input stayed below
0.00004 m/s, and ON brake/resume accelerated, stopped, accelerated again and stopped.
All three controlled cases neutralized and returned to native riding, with no
gaps/drops. Initial mixed-scene cases encountered obstacles and remain recorded
honestly in research/automatic-skating-results.json. Downhill behavior and varied
context repeatability remain open; these are not final integrated skating chains.

Follow-up manual/nose observations confirmed push exclusion. A short movement
batch using backward-stick stops remained neutralized but did not establish
repeatable acceleration: screenshots show native powerslides changed heading,
eventually pointing into a wall. Native B stops preserved heading in three
subsequent calibration trials, each accelerating then stopping. Use that stop
in bounded HIL while B remains native in this build. Do not count the earlier
20 short trials as 20 acceleration successes. See
research/automatic-skating-followup-results.json.

A later obstructed batch remained neutralized but was excluded from repeatability.
After a native scene reset and full-size baseline capture, 16 consecutive trials
across 300/450/600ms movement holds accelerated near 4 m/s and stopped near zero.
Further travel reached course obstacles: one collision transition, one blocked
case, then a native lip stall (400) and one context skip. These remain separate
from clean flat trials. Native B is only the current lab stop, not the final B
control mapping. The same hot game was used for these batches. Five more clean
600ms repetitions on d93dcef brought the clean flat total to 21: each reached
3.998–4.026 m/s, stopped near zero, remained native riding and neutralized without
evidence gaps/drops. The final full-size capture is near a barrier; reset the scene
before further movement batches. Downhill and full-chain integration remain open.

External trusted-v4 was independently reviewed and installed with the current
verified target pins; it adds only numeric push observation parsing. The private deployment helper
uses encoded transport because plain inline installer text falsely matched its
own process-name check. Actual stopped-process and hash checks remain intact.

The board chord is now the active milestone. The local adapter requests one native
Y pulse for LB+RB within 120 ms, preserves individual/late bumper input, requires
both releases to rearm, and suppresses menus, stale state, air, ragdoll and trick
conflicts. It uses existing guarded offboard flags rather than input intent to
decide eligibility. Hosted gates passed and the first real cycle plus a 20-cycle
batch passed: each showed exactly 100 → 504 → 100 in both heartbeat and physics
observations, with neutralized input and no gaps/drops. Long holds did not
oscillate. The batch varied simultaneous, left-leading and right-leading presses.
Five additional quick-tap cycles passed. Individual bumpers, a late pair, the menu
boundary and two OFF cases passed. However, a partial trigger/chord and a full
trigger held before the chord both unexpectedly dismounted. These are recorded
failures, not successful suppression. The skater was remounted with the fixed
native setup action. Bounded numeric game-facing pad edge evidence is being added
to distinguish requested virtual input from actual received input before the
board-control checkpoint closes. See research/board-toggle-results.json.

The literal target table is in CLASSIC_CONTROLS.md and arcade_trick_table.h.
Unassigned repeats have no fallback, and geometry-dependent grinds retain pair
identity. These definitions are not native support claims. The existing timed
gesture adapter remains installed; authoritative native methods are the next
integration pass after the board boundary checks.

## Combo/balance implementation in progress — 2026-10-13

Keep baseline captures at the normal display size. cat pouch

08ab first runtime batch completed6 candidates: neutral no events, manual/nose
pose4/5 detected, long neutral and LY12000 each failed Arcade combo after6.833s,
wrong-way LY-32768 failed1.634s. Native manual continued until input release;
physical failure remains NOT integrated. HUD render-idle omission fixed locally;
added capped4Hz numeric cursor/velocity/raw-input/correction diagnostics to resolve
ineffective modest input.132 Python tests and30,520 C++ sanitizer checks PASS.
Next commit/CI these fixes before deployment. A two-trial existing-Y dismount/remount
experiment is running against verified08ab; no failure actuation wired yet.
Collector-only parser additions are local tested; input runner unchanged from
CI08ab. Do not claim those new parser additions CI verified until next gate.

FIRST RUNTIME HARDWARE BUG:08ab batch artifacts/trick-batch-20261013T232900-54160
(session9788) logs real manual pose4 and accumulated fatigue, but private trial001/1.png
shows no meter. Found overlay_render.cpp idle guard omitted Arcade HUD pending
state, so draw function was skipped without another overlay notice. Local fix adds
freshness-bounded balance_hud_pending to that guard. Must pass CI before update;
leave08ab installed while remaining bounded correction trials finish.

LATEST REAL INSTALL:08ab7dc5b0a91c3fcb65ae1e1084fefa427c62a5 passed ALL CI
38519111179 gates; deployed atomically after verified game/launcher shutdown.
Destination SHA4eba60e862ce52de28b165d2229b50f5aa4df4103118caf9f9118cab51c3ac9e,
stock backup unchanged, updatesfalse/recoveryfalse. Steam shortcut relaunched.
This is FIRST runtime combo/HUD build; not yet hardware accepted. Next launcher
PLAY, leave hub, ensure on board on safe sidewalk, run balance-runtime-08ab7dc.json.
Includes brief manual/nose and long neutral/correcting/wrong-way holds. Native
physical failure and revert remain unverified; no stock restore.

LATEST: HEAD08ab7dc5b0a91c3fcb65ae1e1084fefa427c62a5 pushed; runtime/HUD CI
38519111179 building, separate Deck38519111183 PASS. DO NOT deploy08ab until full
gate. Installed7e3f4ea5ccc4a4848fc767e938d11c230a36004f after ALL CI38517265406
passed; destination SHA4f484cbea989854543d4351b259dfdbb92f5dc937ab977f088bf7dcef03f6056.
Stock5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039 unchanged,
updatesfalse/recoveryfalse. Existing Steam shortcut launched; next verify launcher
ready, PLAY, leave hub/mount, read scalar probe and reset ground away from traffic.
No stock restore. Runtime balance is committed but unverified on hardware.

Runtime wiring checkpoint (local, NOT deployed): combined native activity+contact
classifier,50ms score read, ComboSession, raw LX/LY correction and original
horizontal grind/vertical manual HUD.131 Python tests and30,520 sanitizer model
checks PASS. Revert NOT wired; native balance failure NOT actuated (combo loss only),
so not integrated/complete. Twenty observer repetitions:19 normal correlations,
last interrupted by truck collision (private trial019/1.png); off-board504 clears
adapter. Results research/manual-score-repeat-results.json. Current real install
still823c3ad. Next: CI runtime build, safe ground reset away from traffic, deploy
only full-green build, validate actual manual HUD/correction/failure/reentry.

Current HEAD13d2f334485c7a3abf0efc4cb287ea4f41818755 pushed; Linux/macOS
PASS, Windows still pending. Installed823c3ad fully CI verified, restarted and
healthy. Six native scoring trials completed at artifacts/trick-batch-20261013T222500-48096;
turn/ollie controls at artifacts/trick-batch-20261013T222836-48224. Score bool7aab675c
and int7f3e74e7 are GENERAL trick activity (also ollie), not manual identity.
158-node schema and sanitized values saved in research. Next small diagnostic
adds read-only kind15/size4 scalar values in existing bounded schema walk; needs
CI before deployment. Runtime combo/HUD remains NOT integrated. Keep dev installed.

823c3ad CI38513539193 ALL FOUR gates PASS. Twenty-trial manual-pop batch
completed candidates_exhausted:20/20 native HUD Manual→Ollie confirmed in reviewed
frame2,11 Clean/7 Sketchy/2 unqualified. Same near-stationary flat sidewalk only;
not full integration. Findings saved research/manual-pop-findings.json. Latest
pure tests30,512 PASS and Python130 PASS. Graceful shutdown completed; read-only
status confirms Skate and launcher both stopped, stock hash unchanged. Verified
823 immutable DLL SHA e3d3b895c824fae315a88ad3f3156607096d97e5770552f4bce9e6352ef12b92.
Atomic deployment completed: destination hash verified, stock backup unchanged,
updatesfalse/recoveryfalse. Existing Steam shortcut relaunch requested. No stock restore. Next collect nested score signals during manual and
negative controls; actual event identity remains unverified.

Current continuation: 130 Python tests PASS, 0 failures after correcting the
batch evaluator: either heartbeat or complete per-trial physics sequence may
prove a transition; a missing sample is UNVERIFIED, never an invented trick
failure. New evaluator remains local pending CI; the active batch uses frozen
823c3ad runner code against verified installed276a396. Twenty-trial manual-pop
repeat batch is artifacts/trick-batch-20261013T220120-45043 (session35745).
First11 completed trials show air→riding and reset PASS; remaining trials running.
Reviewed trial010/2.png explicitly shows Manual→Ollie Clean. Do not equate state
sequence alone with named-trick confirmation. Stock remains untouched; dev stays
installed. Next: complete repetitions, review evidence, wait823 full CI, deploy
nested score observer, correlate actual manual state before runtime/HUD wiring.

Active source823c3adb6255b3879381edb1027f0e39fe68144f pushed; CI38513539193
Linux/macOS PASS, Windows pending; separate Deck38513539191 PASS. Do not deploy
until full gate. Installed276a396 is healthy. During build, manual-pop timing
batch artifacts/trick-batch-20261013T215347-43251 completed3 trials (100/250/400ms
full-down setup after manual,300ms up flick). Physics traces show103→200→201
for ALL THREE; heartbeat missed200 in two and reports FAIL. This is sampling loss,
not evidence the trick failed. Screenshot trial000/2.png explicitly shows Manual
then Boned Ollie Sketchy in native history. Discovery only; no repeatability claim.
Important integration constraint: current A/X profile waits1000ms then crouches
600ms, exceeding450ms link grace. Do not hide that by inventing input-intent links.
Bounded follow-up manual-pop-flick-276a396.json fixes setup100ms and compares
75/150ms flick, twice each; currently running. Existing A mapping unchanged.

276a396 launched successfully; fresh sanitized trace contains28 actual
ScoringHUDViewModel fields (research/score-schema-276a396.json). This proves the
read-only UI path works. Observed boolean kind10/size1, integers kind8/size4;
text-bearing fields are nested structures, plus two arrays. Flat probe therefore
does not yet identify native manuals/reverts. Follow-up locally adds a bounded
depth3/256-node walk of this ONE schema, arrays<=8, never object/pointer traversal,
only booleans/int32 and exact four text labels; max4096 value changes per process.
129 Python tests PASS; awaiting next CI before deployment. No feature integration
claim. Current276a396 stays installed. Plan manual-score-276a396 is prepared,
not run yet. Next diagnostic must correlate native scoring values with real
manuals and negative turn controls before the model/HUD is activated.

LATEST DEPLOYMENT VERIFIED:276a39664f7805c1693c7f28f6f314c21bc6d469 passed
ALL CI38510708319 gates and immutable release validation. Graceful Skate exit
confirmed both game/launcher stopped. Existing installer updated development DLL
atomically and verified SHA3683ab2c2e7031331b481e46f205ef865dadf5dd6252f6531c11a479d35c571a.
Stock5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039 unchanged;
updatesfalse, recoveryfalse. Steam shortcut relaunch initiated. Keep this build
installed. Next: launcher PLAY, collect score schema via controller trace, then
known manual holds on this new pinned source/hash. Older35d933e plans must be
repinned explicitly; their integrity refusal against this install is correct.

Current in-flight checkpoint276a39664f7805c1693c7f28f6f314c21bc6d469:
CI38510708319 Linux/macOS PASS, Windows building (started04:18UTC);
Deck installer38510708174 PASS. Must wait full build/release before DLL update.
Moving revert batch: first candidate hit a car; three remaining candidates
correctly skipped off-board state. Not a revert failure conclusion. Existing Y
recipe recovered upright. Used existing ReSkate trainer console (grave key) to
read position, then validated `trainer ground 630 1069` placed skater on sidewalk.
This is a reproducible native ground-surface reset, no marker/settings overwritten.
Live Skate X11 window56623105 title `skate.` on DISPLAY=:1 (requery on relaunch).
Console input at400,640; grave closes; console filter currently `heading`.
No game DLL changed yet;35d933e remains installed. Next after CI: graceful close,
verified276a396 deploy, relaunch, native score schema/label trace during manuals.
Contact-control durations: false manual-like pattern up to433ms during turns;
500ms debounce would exceed450ms link grace, so it is not an acceptable shortcut.

LATEST: febdb40 CI38508894785 ALL FOUR gates PASS.61cdf4d runner Linux/macOS
and separate Deck installer PASS; Windows pending last check. Continued real
testing completed16 additional trials across three batches (all bounded and reset
PASS), summarized in research/manual-revert-controls.json. Crucial negative
result: LEFT-stick arcs also produce0x4000 contact flags during ordinary turns;
do NOT wire contact bits alone as manual events. Both actual manual types remain
confirmed by native HUD labels, not negated by this observer limitation.
Next implementation checkpoint adds a strictly read-only ScoringHUDViewModel
probe using existing validated native UI APIs/model lock: bounded field metadata,
boolean values and four exact allowed labels only, no pointers/arbitrary strings.
No gameplay writes. It needs Windows CI then verified deployment before use.
Pure ComboSession adapter added/tested: semantic transition edges, real landing
grace/revert consumption, failure latch preventing per-frame combo restart.
30,504 C++ sanitizer checks and128 Python tests PASS. Runtime/HUD integration
still incomplete; next real experiment must identify actual scoring/manual event,
then wire that verified observation. Current Deck still35d933e; no restore.

ACTIVE follow-up: febdb40 pushed; CI38508894785 Linux/macOS PASS, Windows
building; separate Deck38508894846 PASS. New real six-trial batch completed:
artifacts/trick-batch-20261013T210055-33354 (two neutral, two tail, two nose).
All six neutralized/reset PASS; unchanged35d933e DLL and preserved stock verified
per trial. Reviewed trial003/1.png native "Manual" and trial005/2.png native
"Nose Manual" labels. Request word1 mask0xc000: riding0xc000, sustained tail
0x4000, nose0x8000; air200/201 has0. Candidate wheel-contact interpretation,
NOT yet authoritative event mapping. Transient crouch103 also has0x4000, so
mask alone is unsafe. Next: negative low-stick controls and repeated holds,
then validate a state-gated, time-qualified observer before runtime activation.
Quiet baseline revealed missing timestamp watermark after long unchanged state;
local runner fix uses log byte boundaries and marks truncation. Also freeze
remote source at process startup so edits cannot change later batch candidates.
126 offline Python tests PASS. These follow-up changes await CI before real use.
No deployment, restore, or game-file mutation in this follow-up.

Capture correction checkpoint: per-trial batch evidence now includes bounded
numeric physics samples, delimited by a pre-input timestamp watermark. This
does not label manuals from input or selector100.124 offline Python tests pass
(two new capture regressions); diff check passes. Saved sanitized discoveries in
research/manual-observations-35d933e.json. New runner changes must pass CI before
real use. Next: repeat known +/-20000 holds with per-trial native samples and
correlate actual manual/nose-manual evidence; both already happened on Deck.

LATEST HARDWARE CHECKPOINT: CI38504144171 passed ALL four gates for35d933e
(Windows/model, Linux, macOS, tested release); separate Deck CI38504144182 passed.
Explicit standing-authorized development update completed to
35d933e919c405132e8fa948f30de2eadb2511d9, installed DLL SHA256
9a79deffb6c6104c131f20e15666475a29def3b32d6ba27ef2905b688a3bb17a.
Stock backup remains5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039;
updatesfalse, recoveryfalse. Game and launcher were confirmed stopped before
installer deployment; destination verified afterward. Relauch through existing
Steam shortcut initiated. DO NOT restore stock between experiments.
Next: enter Skate, verify fresh heartbeat/probe; run six manual candidates from
COMBO_EVENTS.md and inspect numeric traces plus private screenshots. Runtime
model/HUD/native failure are still NOT integrated. Local uncommitted450ms float
boundary fix passes30,496 sanitizer checks but is not part of deployed35d933e.
Diagnostic launch succeeded; fresh source-bound physics traces received, zero
queue drops. On-foot504/mode0 matched screenshot; existing Y recipe mounted the
board. First six-candidate manual matrix completed with neutralization and idle
return PASS for all six, but every heartbeat remained100/mode1. No manual native
identity established. This does NOT mean no manual occurred: user confirmed
trial3 tail manual (+20000 Linux RY) and trial6 nose manual (-20000 RY).
Trial002/1.png independently shows Skate's native "Manual" label. Both are
DISCOVERED on real hardware, not yet repeatability/integration validated.
The coarse100/mode1 detector missed the distinction; fix observation rather
than reject working gestures. Private batch artifacts/trick-batch-20261013T203914-30124;
tracked plan research/manual-discovery-35d933e.json; sanitized trace private
artifacts/combo-manual-neutral-trace.json. A positive control subsequently
produced native "Ollie Sketchy" and100/103/200/201 transitions. Next correlate
the known-working +/-20000 holds with a minimal actual manual-state observable.
Do not require a new selector ID or infer actual state from stick intent.
No rolling-speed variant ran. Keep35d933e installed and stock backup untouched.

Starting HEAD 0979224 on arcade-mode, origin notPho-Real/ReSkate; clean
worktree before this task. Preserve A/X gestures and the active development
installation. No stock restore between experiments. New scope is combo links,
native revert/manual observation, persistent grind/manual fatigue, inertial
balance and its HUD. Pure model is being extended first; runtime integration
and hardware acceptance are NOT complete. Existing model's informational grace
and linear cursor drift do not meet this task's specification.
Phase1 local result: 483 C++ checks passed with strict warnings,119 Python
tests passed, git diff --check passed. Implemented automatic grace banking,
single-use revert window, persistent normalized mode/global fatigue, seeded
240Hz inertial dynamics and tunable difficulty/edges. Runtime still does not
drive the model. Added bounded read-only physics-step transition instrumentation
to identify manual/revert; details in research/COMBO_EVENTS.md. Windows compile
and release CI are the next gate before deploying this observer.
Real Deck preflight succeeded; installed f669926/hash c67bb9dd... and preserved
stock5512a15e... match prior evidence. Game/launcher running, updatesfalse.
Private screenshot artifacts/combo-start shows Hub menu. Existing tested
exit-hub robot recipe ran and neutralized; resulting context not yet reviewed.
No deployment or stock restore occurred.
Checkpoint8cd48cc pushed; CI38501933776 Windows building, Linux/macOS green;
separate Deck CI38501933781 green. Exit-hub screenshot verified on-board skating.
Follow-up local work: balance entry now adds link fatigue once (continuous entry
still idempotent);27 deterministic envelope runs saved,483 C++ checks and122
Python tests pass (three new strict numeric combo-trace sanitizer tests).
Python compileall, all shell syntax and diff checks passed. Manual/revert mapping,
HUD/runtime driving and native physical failure remain unverified/incomplete.
Second pushed checkpoint97f071f: CI38502612665 pending Windows; Deck installer
CI38502612733 passed. Additional local tests now pass30,493 checks with address
and undefined-behavior sanitizers, including10,000 seeded varied event sequences.
Prepared raw controller axes on the existing overlay-bypass sample and pure
balance correction/meter geometry (horizontal grind, vertical up-positive manual).
These are not connected to gameplay or a rendered HUD yet. No input filter slot
changed. Waiting for first diagnostic CI before native manual candidate batch;
do not deploy an unverified local DLL or call partial geometry an integrated HUD.
Checkpoint31dedcc preserves raw axes/helper plus30,493 sanitizer checks.
First Windows run38501933776 FAILED: MSVC C4456 /WX, observer sample count
shadowed existing button count. Renamed to sample_count. Neither8cd48cc nor
97f071f/31dedcc is deployable until a fixed source passes every CI gate.
No diagnostic DLL reached the Deck; it still runs f669926. Latest real evidence
is successful exit-hub recipe and on-board screenshot, not combo acceptance.
Verified repository observations: air 200–299, wipeout300, upstream measured
grind400–499 (especially401). Manual and revert identities are unverified;
button intent must not substitute for those events. Exact next action: implement
and test deterministic lifecycle/dynamics, then inspect minimal native event
instrumentation. No real Deck changes in this task so far.

## Wake helper — 2026-10-13

User explicitly approved narrow administrator changes to enable Wi-Fi wake.
sudo -n confirmed a password is required. Created lab/enable_wifi_wake.py and
Enable WiFi Wake.command, syntax-checked and opened in Mac Terminal for direct
remote sudo password entry (never captured/stored). The script requires the
inspected rtw88_8822ce baseline (profile0x1, driver disabled, device disabled),
sets active profile magic0x8, enables iw magic-packet, enables device wake,
verifies all three and attempts complete rollback on failure. No automatic
suspend/reconnect, daemon, read-only-mode change or game-file mutation.
User completed terminal authentication. Read-only verification CONFIRMED:
iw magic-packet enabled, device power/wakeup enabled, profile wake-on-wlan
0x8. Configuration is enabled; actual wake and reboot persistence UNVERIFIED.
Baseline /sys/power/suspend_stats/success=7. login1 CanSuspend returns challenge,
so user was asked to press power once and confirm Asleep for the packet test.
Do not equate enabled settings with successful wake. No game files changed.

REAL HARDWARE WAKE TEST PASSED once: user confirmed Asleep; wake.py sent
three magic packets; subsequent strictly host-key-verified SSH returned, and
suspend_stats/success advanced from 7 to 8. iw still reports magic-packet
enabled and device power/wakeup still enabled after resume. This verifies one
controlled suspend/wake cycle, not reliability across repeated cycles or
reboot persistence. No game files or SteamOS read-only setting changed.

Follow-up real Deck read-only inspection after user woke it: phy0/wlan0,
rtw88_8822ce driver, kernel 6.18.50-valve2-1-neptune-618-gc7289a96b14d.
`iw phy` explicitly advertises magic-packet WoWLAN support. `iw phy phy0
wowlan show` reports disabled; device power/wakeup also disabled.
NetworkManager reports modifying connection settings requires authentication.
Capability advertisement is not proof of successful suspend/wake. No wake
setting has been changed. Enabling device wake/driver magic-packet handling
crosses the user's no-root/system-change boundary: obtain explicit narrow
authorization before doing so; never store password or disable read-only mode.

User requested a Wake-on-LAN sender while X hardware work was connectivity
blocked. Added lab/wake.py and Wake Steam Deck.command: three 102-byte magic
packets to local subnet UDP port 9; no SSH, root, deployment or Deck settings
changes. MAC/subnet from the user's earlier photo are kept only in ignored
tools/arcade-deck/wake.json (0600), not committed. CLI --mac/--broadcast can
also be supplied; never infer successful wake from sendto success.
119 Python tests passed including five packet/validation/bounds/cleanup tests;
shell syntax passed. Three actual packets sent; subsequent strict-key SSH to
the last known IP timed out. Wake UNVERIFIED, Deck connectivity still blocked.
No new DLL reached Deck. X build run 38489414985 remains Windows-in-progress,
Linux/macOS success as last checked. Resume that work after connectivity and
green CI, without restoring stock. Repo action works with its private config;
if copied elsewhere, supply --mac and --broadcast explicitly.

## X tricks continuation — 2026-10-13

Starting HEAD 5f479ba. Deck connection verified read-only: active f669926,
game/launcher running, expected dev hash and original stock hash intact,
updates=false. Temporary mDNS failure resolved after wake; IP was checked
against the existing paired hostname key, with no new key acceptance.
Upstream fetched read-only at 28118efc2735a71a0c38bda2e7955d35cb2e7d24:
71 commits behind, including relevant grind-exit gamepad filtering. Integrated
on arcade-mode before new trick work, preserving both upstream filter and our
outer-game-read remap. Local 114 Python tests and C++ model pass after merge;
Windows integration still needs CI. No deployment or gameplay input yet.
Merge checkpoint: 25507d6. Shared bounded A/X gesture engine now implemented
locally: X neutral/left = discovered kickflip, X right = discovered heelflip,
direction selected by D-pad before left stick. Preserves measured A timing,
physical right-stick cancellation, no queued/repeated hold, menu release gate.
Only grounded state100/mode1 starts; midair ollie->flip remains unsupported and
must be characterized rather than counted as integrated. C++ tests include 20
repetitions per flip profile in simulation plus exact A boundary equivalence.
X implementation commit 7eb6b3b. Its Windows CI configure failed before compile:
upstream Runtime.cmake globbed an unfetched windows_x86_64_msvc Cargo import
library. Linux/macOS passed. Workflow now fetches locked Windows dependencies
before CMake (preserving upstream features). Next: rerun CI, then deploy/test
and record actual hardware outcomes. Second run 38431924270 passed configure
and built dependencies but MSVC /WX rejected an int-to-uint16 button mask in
arcade_gesture.h. Explicitly narrowed the bounded mask; rerun required. No
development DLL update has occurred. Private game guide captures confirm
diagonal flip directions and describe scoop paths; see research/TRICK_FINDINGS.md.
Other
directional trick names remain selection-only
until native gestures are measured. Do not call the flips repeatable/integrated.

Current X source checkpoint: adbf6ee24918648a97673fceb228b5b20f403191.
Strict local -Wall -Wextra -Wconversion -Werror model build/run passes.
CI 38489414985: Linux/macOS passed, Windows pending at this checkpoint.
Deck later became unreachable at both paired hostname and known IP (bounded
strict-key read-only checks timed out). User wake request is pending. Attempted
graceful close did NOT reach the Deck, and no DLL deployment occurred. Last
observed screen was Hub's menu after leaving Skatepedia; do not send a trick
until on-board skating is freshly verified. No restore is requested or needed.
Resume: wait for this CI, verify immutable release, regain SSH, gracefully close
game/launcher, update active development DLL through controller, relaunch,
run an A/X neutral/X-left/X-right batch before repetitions and chain boundaries.

## Autonomy implementation — 2026-10-12

Starting HEAD for this task: `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`.
Repository remains notPho-Real/ReSkate, branch arcade-mode. Preserve the older
flip selector milestone and uncommitted research recipes; no upstream edits.

Added `.codex/config.toml` (workspace-write, network off, automatic escalation
review, high reasoning) and absolute-path command rules. They require a trusted
project reload and cannot override this session's managed read-only policy.
No global configuration is modified. `inspect_project.py` exposes fixed git,
fork Actions/release and robot/controller read-only commands without forwarded
host/options. `trick_batch.py` accepts only bounded JSON controller data and
pins the target, source hash, DLL hash and original stock hash. See
`lab/TRICK_BATCH.md` and `.codex/README.md` for exact interfaces and limitations.

The batch runner reuses single-run robot cleanup/locking/capture, checks every
candidate, continues ordinary criterion failures, checkpoints private summaries,
and never deploys/restores/publishes. New read-only native heartbeat every 250 ms
provides fresh numeric state/mode for conservative on-board context gating.
No heartbeat means SKIPPED_CONTEXT. Named trick remains UNVERIFIED; sampled
state sequence is not proof of trick identity or integration. Recovery currently
waits boundedly for existing game/HIL readiness; no automatic launcher clicking.

Local verification: 114 Python tests passed (23 new batch tests), C++ pure model
executable passed. Includes fake Deck exact stock roundtrip, existing 21 robot
tests and new failed-candidate continuation/integrity/interrupt/evidence bounds.
Current CLI accepts auto_review/workspace-write and parses the rules; positive
batch rule matched. Python compileall, shell syntax and git diff checks passed;
arbitrary python, ssh and a same-named script outside this checkout matched no
project allow rule. Both workflow YAML files parsed. 15 Python files and 11
shell files passed syntax validation.
No new batch was executed on the real Deck during this task.
Last known installed build remains f669926 and stock backup remains preserved.

Implementation commits: `5c1be16b78bbb856c6d831a7d9d99bf52b2eb32f` and
`e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60`, pushed only to origin arcade-mode,
along with the preserved earlier 8ff309f milestone. Final code CI
https://github.com/notPho-Real/ReSkate/actions/runs/38356744624 passed all four
jobs: Windows build/model, Linux tests, macOS lab and gated release. Separate
installer run 38356744628 passed too. Local suite: 114 passed, zero failures;
pure C++ executable passed. Hosted jobs include the same batch tests.

Immutable release `arcade-build-e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60` and
rolling channel were downloaded and verified with the existing pinned-origin,
commit, archive hash, DLL hash and x64 PE validators. DLL SHA-256:
`c44fd019365bed73ab97e1bb0f9ee4912e743d5a1e5f75bf27c18bcdf0223f45`.
Example JSON now pins this build. This final handoff/data-pin commit does not
change executable code and deliberately skips another identical CI build.

Exact next action: reload this trusted project so Codex can apply local policy
(managed host policy still wins), use the established standing-authorized
deployment procedure to update the active development installation to the
verified e63dbe1 build, launch Skate into on-board skating, then run the command
in lab/TRICK_BATCH.md. No new runner operation or SSH access occurred during
this autonomy implementation; hardware behavior of heartbeat/batching remains
UNVERIFIED. Last known installed DLL remains f669926; do not mistake release
publication for deployment. Keep dev installed. Resume controlled repeatability
and chain tests, not claims of completed flips. Recovery is bounded readiness
waiting, not automatic launcher navigation. Skipped-context trials and named
trick UNVERIFIED must not be promoted to passes.

## Live Deck research session (update immediately after each milestone)

Latest user instructions establish **ACTIVE HARDWARE-IN-THE-LOOP DEVELOPMENT**.
Leave the current tested Arcade development DLL installed across iterations;
the preserved stock backup is emergency/final rollback, not the default between
experiments. The user grants standing authorization for bounded HIL candidates,
repo edits/tests/CI, and atomically updating an active dev installation with
a newly CI-verified development DLL. Restore only on explicit user request,
declared end of development, inability to start the game, failed integrity
requiring recovery, or a concrete safety invariant. Never restore because a
trick probe fails or a handoff is being written. Do not claim any trick
implemented from one HUD observation.

Trick maturity: DISCOVERED (>=1 confirmed), REPEATABLE (>=20 controlled trials
plus varied speed/respawn), INTEGRATED (composes with ollie/landing/grind/manual
contexts, boundaries and physical-input precedence characterized). Current A
ollie is hardware demonstrated twice, Kickflip once, Heelflip at least once;
NONE meets the full new integrated standard. Immediate chain baseline is
ollie -> land; kickflip -> land; heelflip -> land; ollie -> kickflip -> land;
ollie -> heelflip -> land. Grind/manual chains follow once controllable.
Use data-driven bounded candidate tables and machine result files under
`contrib/arcade-mode/research/`. Keep game/dev build hot, neutralize temporary
virtual input after each candidate, and do not publish raw frames/logs.

The user explicitly approved redeploying the previously tested f669926 DLL
for native flip research, then restoring stock. Read-only preflight passed:
both processes stopped, tested release source f669926 and expected DLL hash
`c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7`.
`controller.py deploy` then succeeded and verified this exact installed hash,
preserved original backup hash
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`,
active=true, updates=false. The later standing authorization above supersedes
the original restore-after-research instruction. Keep development installed.
The new X selector code remains only in
local commit `8ff309f569446b45fb0203fb4b6c5ab83acd57f9`, not on the Deck.

Live update: ReSkate launcher READY with updates off; Play launched Skate.
The tested virtual controller exited Hub with B and mounted the board with Y.
On flat ground, right stick down (XInput Y=-32768) for 600 ms then upper-left
(-28000,+28000) for 250 ms produced the in-game HUD **Heelflip Sketchy**.
The mirror upper-right (+28000,+28000) produced **Kickflip Sketchy**. Four
private Gamescope frames per run; the named HUD is visible in frame 2 of each
`artifacts/flip-candidate-left-01` and `...right-01`. These are REAL Deck
observations, but each is one attempt and "Sketchy" means the current timing
may need refinement. The Classic Controls mapping is left X=Kickflip/right
X=Heelflip, so the native right-stick **direction is opposite** the Arcade
direction for these two names. No X runtime build has been deployed.

## Active X flip-trick milestone (2026-10-12 Pacific)

Starting HEAD `970b2d93048d46c41506ed35a8a5df05076aefaf`, branch
`arcade-mode`, clean at start. User requested exact Arcade flip tricks on
X plus left stick or D-pad. The Arcade control table
specifies Xbox X + left/right/up/down = Kickflip/Heelflip/Impossible/Pop
Shove-It, and top-left/top-right/bottom-left/bottom-right = Hardflip/Inward
Heelflip/Varial Kickflip/Varial Heelflip. This is a verified classic input
mapping; the native Skate right-stick sequences for those eight moves are
**not yet verified**. The real Deck was confirmed reachable and STOCK by a
read-only `controller.py status`; game and launcher stopped. Do not deploy a
new DLL without explicit approval for the tested source. Immediate work:
derive bounded native gesture candidates and test with the stock game where
possible, then implement pure direction selection and verified native X
remapping. Preserve A behavior and rerun local/CI gates.

Progress: `arcade_flip.h` now has a pure eight-direction selector for both
XInput D-pad and left stick, with D-pad precedence, a stick deadzone, and
invalid opposing D-pad rejection. C++ model tests pass locally. `lab/robot.py`
has two bounded, explicitly experimental upper-diagonal right-stick recipes,
with fake-device tests and instructions in `lab/ROBOT.md`. Python Deck/Mac
suite passes 91/91 with permitted temporary filesystem access. The initial
read-only sandbox run failed 71 tests because the sandbox disallowed *all*
temporary directories; the exact suite passed unsandboxed. No X runtime hook
or native trick claim yet; no new Deck game mutation. Await stock Skate being
launched before attempting screen-based gesture identification.
Read-only remote robot preflight found the Deck connected with Gamescope and
writable uinput, but `skate_running=false`, so no gesture was sent and no
native trick identity was asserted. The existing ReSkate Steam shortcut was
identified by read-only lookup; shortcuts.vdf was not edited. Python syntax
passes for changed files. These changes are research and pure selection only,
not an X gameplay implementation; do not call this milestone complete.

Stock-game attempt: launched the existing ReSkate Steam shortcut solely for
gesture research; its launcher displayed **"A ReSkate update is ready"** and
offered UPDATE, so no button was selected. Launched Skate's own installed Steam
app separately; private Gamescope evidence showed startup error E111000B:
"Wine, Proton, and Steam Deck are not supported by this application." Skate
never reached gameplay. The launcher window opened by this attempt was closed
via its verified X window ID, without pressing update. Final read-only status
showed game=false, launcher=false, active Arcade=false, updates=true, current
DLL and original backup still exactly
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No candidate flip gesture was sent. Private images are ignored under
`artifacts/flip-*` and must never be published.

Current external blocker for **native trick identification**: stock Skate
cannot start directly under Proton, while the ReSkate launcher currently
requires a pending stock update to expose Play. Do not click UPDATE or deploy
a development DLL implicitly. Next action is to resolve that launcher state
through an explicitly authorized safe operation (or obtain native gesture
evidence through another primary route), then run the candidate recipes on
the actual game, identify HUD results, add only confirmed gesture profiles to
the guarded runtime X remapper, and pass C++/Python/Windows/Linux/macOS gates.

Updated 2026-10-13 UTC (2026-10-12 Pacific). This first section is authoritative.
The historical sections below are retained for provenance and are superseded.

## Current handoff

Repository: notPho-Real/ReSkate, branch `arcade-mode` ONLY. Never change
main/master/upstream, replace Skate.exe, use Deck root/sudo, disable SteamOS
read-only mode, save a Deck password, or edit shortcuts.vdf. Origin is
https://github.com/notPho-Real/ReSkate.git. Starting HEAD for the most recent
A-ollie implementation was 2a410720c61aecc2895f10bbc213bfff8ade16bb.
Feature commit f669926ab790d1328bc234a9828c290fb46ee276 was pushed only
to arcade-mode; a documentation-only final handoff commit may follow. No PR.

The REAL Steam Deck is currently STOCK: ReSkate.dll and preserved first-stock
backup both hash SHA-256
5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Skate and ReSkateLauncher were both stopped after the test. Arcade receipt
active=false, recovery_pending=false, and the original ReSkate updater
preference was restored to true. User explicitly approved deploying and testing
feature f669926, then restoring; the approved session is finished. Another
development DLL deployment requires an explicit user operation/authorization.

## Implemented architecture

Arcade Mode is native in ReSkate.dll because stock ReSkate does not load
arbitrary Plugins/*.dll and custom Lua scripts are currently disabled. Its
startup and client-tick hooks already exist; the overlay controller reader
provides A/B/X/Y edges and the runtime observer. A=ollie, X=flip/trick family,
B=grab, Y=grind/context are intended controls; only A ollie has gameplay
implementation. The existing pure `arcade_model.h` handles combo scoring,
banking, fatigue, balance/grace, and bail reset without affecting game state.
Future multiplayer knockdowns should be host-authoritative events that invoke
native bail, not player rigid-body collisions.

Feature f669926 adds pure `arcade_ollie.h` and hooks the already-present
XInput game-read paths, including Steam's front hook. A triggers a native
right-stick gesture only when guarded client-tick reads show a fresh local
on-board idle state (state 100, rig mode 1), game focus and no interactive
overlay. It suppresses the triggering game A, waits 1 s, holds the right stick
down 600 ms, releases up 150 ms, then returns to neutral. Holding A starts once;
physical right-stick movement or menu capture cancels it. The binary signatures,
owner checks and five-second bounded numeric diagnostic trace remain in
`arcade_native_probe.h` and `arcade_mode.cpp`. This path does not write game
memory or guess a native trick function. The major technical risk remains
commanding Skate's native trick state machine cleanly across contexts.

## Build, release and tests

Feature source f669926 passed local 91/91 Python Deck/Mac simulations, pure C++
model tests, and diff check. GitHub run 38338735186 passed Windows x64 DLL build
and C++ tests, Linux simulations/shell syntax, hosted macOS lab tests/Python
syntax, and gated Deck packaging. Installer workflow 38338735189 passed.
The public immutable `arcade-build-f669926ab790d1328bc234a9828c290fb46ee276`
and rolling tested `arcade-deck-latest` point to source f669926 and DLL SHA-256
c67bb9dd2a2cec4db0b683df7cb96de165a801dd7153e8a2010b35a2485008c7
(13049344 bytes). The channel was verified with commit/hash/PE/origin checks.
There is no expiring artifact or Deck GitHub-auth requirement. CI advances the
rolling channel only after Windows, Linux and macOS gates; older late workflows
cannot roll it backward.

Mac lab controller is `contrib/arcade-mode/lab/controller.py`, Deck installer
and updater are `deck_installer.py` and `deck_updater.py`, and autonomous
virtual pad/screenshot runner is `lab/robot.py`. The controller offers
self-test, pair, read-only preflight, explicit deploy, status, collect, restore,
and numeric trace. Pairing uses a dedicated ed25519 key and known_hosts under
the user's HOME; password goes only to the SSH terminal. Host-key checking
stays strict. Preflight performs only reads. Deploy validates pinned release
metadata, refuses running processes, preserves the first stock DLL and previous
updater preference, atomically installs and verifies source/hash. Restore
requires stopped processes and recovers exact stock. Bootstrap runs its tests
before installing the HOME controller and never deploys automatically.
`LAB_PROTOCOL.md` and `lab/ROBOT.md` specify the interfaces and safety rules.

## Real hardware evidence

Diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 established a native
right-stick ollie baseline on this Deck and was restored to stock. That result
is at
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/bbd1def2eaf42945d43f4e0708574852eee3b817.json .
A private Gamescope frame showed Skate's “Ollie”; complete numeric capture
transitioned state/mode 100/1 -> 100/0 -> 103/0 -> 200/2 -> 201/2 -> 100/1.
A prior A-only board baseline was contaminated by an in-game traffic collision,
not an application crash; do not use it as a clean control.

For feature f669926, a fresh read-only preflight passed with the game closed,
stock and backup matching, and sufficient space. After explicit authorization,
`Lab.deploy` installed and verified the feature DLL, retained the backup and
disabled updates. ReSkate launcher showed READY and updates-off; Skate started,
`arcade_mode_initialized` was true, “Arcade Mode ready” logged, and B exited
Hub. Y mounted the skater on the board on a sidewalk. The bounded virtual-pad
`idle` recipe sent ONLY a 100 ms A tap with NO right-stick input. Four private
Gamescope frames showed crouch, Skate's native “Ollie” HUD, and board landing.
Complete native capture id 1 had 21 samples and the same state/mode sequence as
the ordinary right-stick ollie. A continuous 3000 ms A hold generated exactly
one additional complete capture id 2 (21 samples), with the same native
sequence, and no repeated rising edge. Skate did not unexpectedly crash.
A/B/Y observer events registered; a short 120 ms X tap was missed after virtual
pad hotplug, while a later one-second X hold registered. Do NOT claim that short
X tap passed.

Live evidence was saved before graceful game close. After both game processes
stopped, restore verified the exact stock SHA above, active=false and
updates=true. Final sanitized observer report has 13/13 PASS (holds,
native-controls and no-crash include operator observation):
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276.json .
Separate sanitized machine-readable A-specific result, with no full logs,
screenshots, credentials, IPs or personal paths:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276-ollie.json .
Private frames remain ignored under `artifacts/robot-feature-*`; Mac HOME
contains sanitized results at `~/.local/share/reskate-arcade-lab/results`.
No Mac controller bootstrap/pair/config was installed or changed in this run.
The Deck game DLL/updater were modified only for the expressly approved test
and restored; transient virtual input devices and Gamescope temporary frames
were created by the test.

## Exact next action and unknowns

A -> native-quality Skate ollie is VERIFIED for two bounded Deck inputs in this
single session. Next gameplay milestone: X -> one fixed native flip, followed
by grounded/air/grind/manual/landing/bail observation. First review this state
and the published result, then design the fixed X action against native input
evidence; run pure/CI gates before any new explicit deployment. The A mapping
still needs testing with physical controllers, varied terrain, repeated sessions
and online contexts. Short X taps near virtual-controller hotplug are
unreliable in this setup; use a settled pad or longer hold when testing.
No current external blocker. Never claim broader hardware compatibility from
this one session.

## Historical robot implementation progression (superseded by current handoff)
LATEST WORK (2026-10-12): user requested autonomous virtual controller input over
SSH with periodic screenshots. Start HEAD 80ec088f0906482dcff926798825deec5b9bef1d,
clean arcade-mode, origin pinned to notPho-Real/ReSkate. Strict read-only SSH
capability attempts failed: hostname resolution, then last known IP timed out.
No remote command ran. Deck input/capture permissions are UNKNOWN, not denied.
Implementing lab/robot.py: stdlib temporary Linux x86_64 uinput Xbox-style device,
fixed bounded experiments, neutralization/destruction on failure, same-owner
Gamescope SIGUSR2 screenshots via pidfd, private local frames, no auto publication
or deployment. No sudo, permission changes, system installation or game writes.
Upstream Linux uinput headers/docs and Valve Gamescope main.cpp confirm interfaces;
installed SteamOS version/support and Steam player assignment remain UNVERIFIED.
Tests are in progress. Exact next action: finish offline tests, then retry read-only
robot preflight; run a bounded test only when game is intentionally open/visible.
The diagnostic DLL remains undeployed pending the prior explicit deploy question.
UPDATE: after user woke Deck, real read-only SSH succeeded. Linux x86_64,
/dev/uinput character-device write access and Python pidfd support are VERIFIED.
Skate was closed. Display is same-user gamescope-wl, argv0 gamescope, SIGUSR2
handler installed. /proc/PID/exe returns EACCES; do not change permissions to read
it. Identity check uses comm+argv0+UID+caught handler and pidfd for signalling.
Input creation, game delivery and screenshots not yet verified. User asked to
open Skate at a skating scene. Offline 87/87 PASS, 12 Python syntax/11 shell syntax
PASS, C++ model/capture suite PASS; no CI run yet for this work.
REAL HARDWARE UPDATE: temporary neutral virtual gamepad successfully created using
the real Gamepad implementation. /sys/devices/virtual/input matching name was
absent before, present during, absent after (exit 0). No non-neutral input sent,
no game files changed. This proves rootless device creation/cleanup, not Steam
player routing or gameplay. New explicit `robot.py capture --output <new-dir>`
can verify display capture while Skate is closed; it never creates a controller.
REAL SCREENSHOT VERIFIED: this SteamOS emits /tmp/gamescope_<timestamp>.avif,
not PNG. First capture attempts correctly returned no image rather than using
stale files; adapted collector to PNG/AVIF with freshness, ownership, no-symlink,
size, complete-box and stable-metadata checks. Actual AVIF downloaded, hash-checked
and saved under ignored artifacts/robot-screen-01; converted locally with macOS
sips and visually inspected: Steam ReSkateLauncher.exe page, Play button visible.
No game running, so non-neutral input/Steam player routing and periodic in-game
capture still UNVERIFIED. Deck changes: transient neutral input device (removed)
and Gamescope-owned temporary screenshots only. No DLL/config/settings changed.
No Mac HOME installation changed; preview/evidence files stay inside repository
ignored artifacts. Exact next action: once Skate is at a skating scene, run robot
idle experiment, inspect four frames and actual response before any further input.
Latest local validation: 89/89 Python tests PASS (70 existing + 19 robot tests),
12 Python compile checks PASS, 11 shell syntax checks PASS, one C++ model/capture
executable PASS, git diff --check clean. Existing Linux/macOS CI discover the new
tests automatically; bootstrap now includes test_robot.py. CI/channel status will
be recorded after pushing; no workflow definition or runtime DLL code changed.
Implementation commit 367f67da09c686bc5755efdf5644aa552c539c33 pushed only to
origin arcade-mode. Hosted Linux/macOS and installer passed; Windows was building
at last check (development run 38334453685, installer 38334453702). Follow-up
review found screenshot-worker joining could delay neutralization after a replay
error. Fixed by neutralizing before joining and cancelling screenshot waits; two
regression tests added. Latest local suite 91/91 PASS, 12 compile checks PASS.
This supersedes the earlier 89-test count. User has not yet confirmed Skate open;
last real preflight still reports game closed. No non-neutral input has been sent.
SUBSEQUENT AUTONOMOUS STARTUP: under the user's request to perform tests remotely,
captured and inspected a fresh Steam screen showing existing ReSkateLauncher.exe
Play entry selected. Sent exactly one virtual A press (120 ms) through tested
Gamepad API after rechecking no Skate process, then neutralized/destroyed device.
The following screenshot shows a black launch transition. Launcher/game arrival
is not yet verified. No DLL deployment performed. This supersedes the preceding
"no non-neutral input" statement; no gameplay acceptance is inferred from it.
Launcher arrival now visually verified: ReSkate 2.0.7 window with UPDATE button,
not Play. Did not activate update. User EXPLICITLY APPROVED deployment of tested
diagnostic bbd1def2eaf42945d43f4e0708574852eee3b817 and continued automated tests,
including closing launcher and relaunching. This supersedes pending approval.
Next: verify stock/hash, close only launcher, pin immutable bbd1def release,
rerun installer preflight/deploy, relaunch, run bounded inputs/screenshots/traces.
DEPLOYMENT VERIFIED: immutable bbd1def manifest/hash/PE validated; two exact
same-user ReSkateLauncher processes asked to close via SIGTERM/pidfd. Fresh
preflight passed with both processes stopped. Shared Lab.deploy installed DLL
082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468, receipt active,
updates=false. Stock backup remains 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Deck now has DIAGNOSTIC development DLL, not stock. Prior stock/updater preference
preserved. Next: inspect Steam screen, relaunch existing shortcut, enter game,
verify runtime and virtual input delivery with bounded captures. No ollie feature
implemented; still observational native-request instrumentation.

NEW ACTIVE TASK: user authorized proceeding with A -> native-quality ollie.
LATEST VERIFIED BUILD: bbd1def2eaf42945d43f4e0708574852eee3b817. CI run
38313781791 PASS for Windows build/model, Linux, macOS, and gated Deck packaging;
installer run 38313781774 PASS. Earlier runtime-only run 38313325123 also PASS.
Public tested release was downloaded/validated in memory: channel commit bbd1def,
DLL SHA-256 082e804b302e59963961f90bf001320acbf90e3901d1c5686fcb82b363719468,
13045760 bytes. No deployment. Local Python tests 70/70 PASS, C++ model/capture
suite PASS normally and under ASan/UBSan, 10 Python syntax checks PASS, diff check
clean. No workflow definitions changed. Only local ignored research artifacts
were added outside tracked code; real Deck operations in this task were reads.
The user has been asked for explicit diagnostic deployment under the original
deploy-only safety rule. Pending that answer, Deck remains stock. After authorized
deployment: idle five-second A-armed capture, then A-armed ordinary stick ollie,
repeat, `controller.py trace`, and interpret numeric evidence. This is a hardware
input dependency, not completion of A-to-ollie. The unknown native request API
remains the primary blocker; never present this diagnostic as the gameplay feature.

Runtime probe commit: 270a532c01ad39ddfbcfbbe8515f5ed484b032a4, pushed only to
origin arcade-mode. CI 38313325123: Linux/macOS passed; Windows still running
at last check. Installer workflow 38313325113 passed. Follow-up adds read-only
`controller.py trace` collection with numeric allowlist and source/hash/freshness
binding; local suite now 70/70 PASS, including serialized fake-Deck trace and
wrong-DLL refusal. These supersede the earlier 65-test count for this new work.
The first implementation is a focused READ-ONLY native request probe, not an
ollie trigger. See NATIVE_OLLIE.md for exact static evidence and hardware protocol.
Two 32-byte contracts (rig crouch predicate RVA 0x476f130 and producer 0x1009e29)
match the actual binary; runtime disables the new probe on mismatch. A starts a
five-second capture limited to 128 changed samples, no game memory writes or
new hooks. Pure tests cover inactive capture, deduplication, ownership loss,
recovery, time expiry/backwards clock, cap and rearming. Local C++ model/probe
suite PASS with -Wall -Wextra -pedantic; existing Python simulations 65/65 PASS.
Next: Windows/Linux/macOS gates for this build, then explicit diagnostic deployment
and user-performed normal stick ollie to correlate native requests. No safe native
ollie command is established yet. Deck remains stock; never attribute old hardware
acceptance to this new probe build. No new real-target writes in this task.

Active task authorization context: user authorized proceeding after
observer acceptance. Work starts from f814aec2cbdbacfaee2582a5175cc546f8bcd0de,
branch arcade-mode, clean worktree. Native-path investigation is now in scope.
Read-only SSH copied installed Skate.exe to ignored repository artifact
artifacts/arcade-native-research/Skate.exe (144567168 bytes); SHA-256
fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9 matches the
existing 20260929 build contracts exactly. Never commit/publish this binary.
No new DLL deployed; Deck still stock. Existing trainer_jump.cpp changes no-comply
and boneless launch velocities, not an ollie trigger. Existing selector at RVA
0x4745a70 and context request flags are analysis anchors, not an established
ollie command. Next: trace native crouch/pop request producers and consumers in
this verified binary; do not force state IDs or invent a function signature.

Final sanitized report timestamp: 2026-10-12T06:31:01Z; source build
f09872f179434cd3d474c9952b8325363424c308, installation ID
0c790805de1b44d69785aaacac95493e. Report:
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
All 13 criteria PASS, observer_acceptance=PASS. Runtime process/initialization/
ready/fresh log and A/B/X/Y events came from actual SSH evidence; normal controls
and no crashes during the session were user-reported. Holds were user-executed
and verified by exact +1 counter deltas on all four buttons. This is bounded
session evidence, not a guarantee against future crashes.

After user closed the game, restore verified both processes stopped, no pending
recovery, receipt inactive, and installed DLL AND preserved backup both equal
original SHA-256 5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039.
Updater preference restored to true. The final collector correctly retains prior
runtime evidence for this same installation ID; reskate_process_seen=true in
the combined report describes the tested session, not a currently running game.
No game is running at restoration; the Deck currently has STOCK ReSkate, not
the development DLL. Do not redeploy implicitly. Tested release remains f09872f;
documentation changes do not advance the rolling channel. Report publication
changed arcade-lab-results only; STATE.md handoff belongs on arcade-mode.

No current infrastructure/hardware blocker. Exact next development milestone is
A -> native-quality Skate ollie, but native trick reverse engineering is outside
this completed task and requires a new task instruction. Following milestones:
X -> one fixed native flip, then grounded/air/grind/manual/landing/bail observation.
Historical intermediate notes below preserve provenance and are superseded by
this current handoff wherever they describe incomplete pairing/testing.

## Historical deployment and acceptance progression (completed; see current handoff)
All controlled button holds VERIFIED: after user's A/B/Y test the actual counters
are A=16/B=9/X=8/Y=6, exactly +1 each for A/B/Y with X unchanged. Combined with
the prior controlled X test, all four holds generated one rising edge and no
repeats. User confirms normal controls work and previously no crashes so far.
Published report 2026-10-12T06:29:45Z has 11/13 criteria PASS; only stock_restore
and updater_restore remain UNVERIFIED. Overall acceptance correctly stays
UNVERIFIED until restoration is checked. Same source/installation ID and result
URL as below. Holds are marked operator-reported in schema and additionally
supported by the controlled before/after counts documented here.
Exact next action: user exits Skate AND launcher; verify both stopped, run the
existing restore operation, verify original stock hash and updates=true, then
collect/publish the combined report for this same installation. Do not stop the
user's game remotely or replace any DLL while either process is running.

Controlled X hold VERIFIED on real hardware: user released all buttons, held X
once for three seconds, released, and reported done. Read-only log comparison:
X event count/latest counter rose from 7 to 8; A=15, B=8, Y=5 unchanged. Exactly
one X rising-edge event occurred; no repeated events during this controlled hold.
Overall holds remains UNVERIFIED until equivalent A/B/Y holds are checked.
Next action: from current A=15/B=8/Y=5 baseline, user separately holds each once
for three seconds with full releases; expected counters A=16/B=9/Y=6, X=8.

Latest report 2026-10-12T06:26:53Z: user reports no crashes so far; collector
published no_unexpected_crash=PASS as OPERATOR-REPORTED, bringing total to 9/13
PASS, 4 UNVERIFIED. User says holding three seconds produced no new notification;
this is ambiguous between expected absence of repeats and absence of initial edge,
so holds remains UNVERIFIED. Read-only current-session event counts: A=15, B=8,
X=7, Y=5, each latest counter equal to event count. Counts alone cannot establish
which events correspond to holds. Code emits only rising edges, not at three
seconds. Next controlled check: release all buttons, hold X once for three seconds,
release, and compare X counter against baseline 7 (avoid other X presses).
Native controls and actual stock/updater restore remain unverified.

REAL RUNTIME EVIDENCE obtained 2026-10-12T06:24:45Z after user launched Skate.
Actual SSH collection verifies running Skate, fresh session log, Arcade Mode
initialized active, ready marker, and ALL A/B/X/Y observer events: 8/13 criteria
PASS. Remaining 5 criteria UNVERIFIED: holds without repeated rising edges,
native controls, no unexpected crash over test session, exact stock restore,
updater setting restore. Overall observer acceptance remains UNVERIFIED.
Sanitized report successfully published through the tested collector to
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f09872f179434cd3d474c9952b8325363424c308.json
Publication changed only arcade-lab-results, without force; no raw logs published.
Local report is under ~/.local/share/reskate-arcade-lab/results/ with the same SHA
filename. Current next action: verify each three-second hold produces one edge,
obtain operator confirmation of normal controls/no crash, then close game and
perform explicitly authorized stock restoration to verify the last two criteria.
Do not redeploy or restore while the game is running. Historical pending-launch
statements below are superseded by this actual runtime result.

USER-AUTHORIZED REAL DEPLOYMENT COMPLETED after the preflight below. The user
explicitly requested updating the Deck to our build and testing it. Actual
`controller.py deploy` exited 0 with `DEPLOY VERIFIED`: installed commit
f09872f179434cd3d474c9952b8325363424c308 and destination DLL hash
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01` match
the verified release. First stock backup hash matches the pre-deploy DLL exactly:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
Receipt exists and active=true, updates=false (previous setting true), no recovery
pending. Both game and launcher stopped at deployment verification. Game files
HAVE NOW BEEN CHANGED by this explicit deployment; prior no-change statements
below describe earlier preflight only. No game launch or runtime acceptance yet.
Exact next action: user launches Skate normally and exercises A/B/X/Y taps and
holds; collect actual fresh runtime evidence over SSH. Do not deploy again just
to collect. Restore and updater restoration remain unverified on hardware.

After the user completed interactive pairing, the actual controller `preflight`
exited 0 and printed `PREFLIGHT PASS — NO GAME FILES CHANGED`. This verifies
noninteractive dedicated-key SSH with strict host checking on the real Deck.
Authenticated user/host: deck/steamdeck. Discovered Steam library:
`/home/deck/.local/share/Steam`; game:
`/home/deck/.local/share/Steam/steamapps/common/Skate`.
Skate.exe, ReSkateLauncher.exe and ReSkate.dll all present; safe paths pass.
Neither Skate nor launcher running; process check complete. Current DLL SHA-256:
`5512a15eacd074d8c03972467af82975fe033a0d5d5a5cf9001c4520fc1f8039`.
No stock backup, Arcade receipt or config exists; no recovery pending; Arcade
inactive and installed commit unknown. Launcher settings readable; updater true.
Free bytes 24475693056, required bytes 220581888. Verified latest tested release
is f09872f179434cd3d474c9952b8325363424c308, expected development DLL SHA-256
`d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01`.
Pairing necessarily created local SSH identity/trust and authorized the public key
on Deck; this preflight performed only reads. No game files were changed and no
development DLL deployed. Runtime observer, native controls and restore acceptance
remain UNVERIFIED on hardware. Exact next action: obtain an explicit deployment
operation from the user before any game mutation, then intentional launch and
observer validation. Do not repeat pairing or diagnose old reachability failures.

Current tested implementation/release: f09872f179434cd3d474c9952b8325363424c308.
Final HEAD is a subsequent STATE.md-only handoff commit marked [skip ci]; use
`git rev-parse HEAD` for its exact SHA. It does not change the tested code or advance
the release. No essential continuation state depends on the conversation.

## Boundaries and starting state
Only `notPho-Real/ReSkate`, branch `arcade-mode`. Never main/master, upstream,
upstream PRs, root on Deck, SteamOS read-only changes, Skate.exe replacement,
manual shortcuts.vdf edits, password storage, or implicit deployment.
Starting HEAD: f46994894addece6b6c111bae29e474ae9e929f2.
The clean worktree initially used arcade-stage-02-state-probe. arcade-mode was
fast-forwarded from d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc to preserve both newer
commits (b62fe7c isolated stage CI; f469948 read-only native-state correlation).
Current HEAD: use git rev-parse HEAD; this document cannot contain its own commit SHA.
Last known tested release SHA before this run's push:
d83e1e6351d8ddfd6d64f33efa43d61b7f4a24cc; Windows/Linux Actions runs
38176087200 and 38176087218 succeeded. Public channel/archive/commit/PE validation
was executed in memory: DLL SHA-256
5a3d6a79fe8e8c2662e570dd5fad68c41f8d2fa551ec9e647d88ed89e1ed05ca,
13,039,616 bytes. This baseline predates the new macOS gate. No hardware evidence.

## Architecture and verified repository facts
Arcade Mode lives natively in ReSkate.dll: stock ReSkate has no arbitrary
Plugins/*.dll loading contract; custom Lua is explicitly disabled by
Extension/Scripting/custom_script_loader.cpp custom_scripts_enabled=false.
Runtime/runtime_init.cpp calls start(base), records arcade_mode_initialized;
Runtime/client_tick.cpp invokes on_client_tick before the 500 ms model throttle.
CMake includes runtime sources and optional standalone arcade_mode model tests.
The observer uses DingoSDKOverlayReadControllerInput, bits A/B/X/Y =
0x1000/0x2000/0x4000/0x8000. Rising edges log and notify individually; holds do
not repeat. Input unavailable clears held mask (refocus can produce a new edge).
Input requires focus and a closed interactive overlay. No suppression or trick
injection. Existing watched_physics_state diagnostics are sampled, not a
transactional gameplay event stream; preserve them without extending research.

Pure arcade_model.h owns combo, saturating points/multiplier, bank/bail,
balance, correction/drift and fatigue persistent throughout an unbanked combo.
Grace is informational pending actual landing events. None drives Skate yet.
Intended controls: A=ollie, X=flip/trick family, B=grab, Y=grind/context.
Future knockdown: host-authoritative ATTACK/KNOCKDOWN validation and native bail;
reject stale/duplicate/unauthorized requests, not rigid-body player collision.
Major risk: commanding Skate's native trick state machine cleanly.
Next gameplay milestone AFTER observer hardware acceptance: A -> native-quality
Skate ollie, then X -> one fixed native flip, then observe grounded/air/grind/
manual/landing/bail. No trick reverse engineering belongs in this lab task.

## Existing build and deployment
Windows CI builds Release x64 ReSkate.dll and pure model tests. Linux tests
package the offline installer. Immutable arcade-build-<40-char-SHA> releases
feed arcade-deck-latest/channel.json; pinned fork, archive SHA, DLL SHA, commit
checks. Serialized publishing plus branch HEAD check prevents late rollback.
Existing deck_installer.py preserves first stock backup and updater preference;
deck_updater.py fetches public releases without Deck GitHub authentication.
Fixed: old deploy_to_deck.py --pair now returns after pairing, never deploys.

## Evidence and acceptance
Actual log code: Engine/Core/Log/paths.cpp chooses game/logs/ReSkate.log, with
LocalAppData/ReSkate/logs fallback; RESKATE_LOG_DIRECTORY can override. Logger
truncates at session start in the default path. Crash marker comes from
Engine/Core/Log/crash.cpp: ReSkate unhandled exception. Do not invent a Proton
prefix path or publish full logs. Runtime/runtime_state.cpp routes record() to
logging::event. Important review correction: Engine/Core/Log/events.cpp formats
events as text (`Arcade mode initialized: active=true`); raw JSON is trace-only.
The original record() default is debug and hidden at the launcher's normal info
level. Runtime/runtime_init.cpp now emits this one event at info, and the collector
matches its actual text (also supports trace JSON). No gameplay behavior changed.
Hardware acceptance requires actual startup, initialized active, ready, each
A/B/X/Y tap observed, no repeat on holds, normal native controls, exact stock
restore and previous updater preference restored. All UNVERIFIED on hardware.
Absence of a crash log alone cannot prove no unexpected crash.

## Lab implementation
lab/controller.py provides self-test, pair, preflight, deploy, status, collect,
restore and test-first bootstrap. Seven .command actions expose setup/operations.
HOME destination is ~/.local/share/reskate-arcade-lab; dedicated SSH key and
known_hosts live there. No keys were created during development. SSH host policy
is ask only during pairing, strict yes thereafter. Password goes only to SSH tty.
lab/probe.py is a separate read-only payload sent into Python -B memory over SSH;
tests inspect the generated code and run it against fake HOME/library fixtures.
Deploy validates pinned release/archive/commit/DLL hashes and x64 PE, preflights
twice and uses the local trusted shared installer, not downloaded Python code.
Shared installer now uses nonblocking flock, exclusive immutable backup, fsynced
rollback journal, atomic writes, destination verification and interrupted recovery.
Legacy shell helpers are migrated to this same lock/transaction through manual_install.py.
Results allowlist booleans/enums/source SHA/DLL hash/time and unique installation ID.
Log baseline/freshness and latest-ready scoping prevent old/combined-session evidence.
Holds/native behavior/no-crash require labeled operator observations, never inference
from absence. Post-restore collection can join only the same installation's evidence.
GitHub Git Data API publishes only sanitized results to orphan arcade-lab-results;
updates are fast-forward only, no force push, no development checkout mutation.
Publication failure leaves local JSON and installation untouched.
LAB_PROTOCOL.md documents all interfaces, operation semantics and recovery details.
arcade-dev.yml now gates deck packaging/publication on build, linux-tests, macos-lab.

## Tests and remaining milestones
Final offline suite after actual log-format correction: 65 tests passed,
0 failures (macOS execution, fake targets).
This includes actual generated SSH payload execution, exact stock restore, AST
preflight non-mutation, failed writes, flock contention, bootstrap failure gating,
stale log refusal, mocked GitHub publication, an actual child-process exit during
transaction, explicit fallback-log baselining, and legacy shell round-trip.
Python compileall: 10 files passed. Shell bash -n: 11 files passed.
Standalone C++20 model/observation executable: 1 passed, 0 failed.
Both workflow YAML files parsed; explicit needs/build/Linux/macOS runner assertions
passed. Git diff whitespace check passed. Report/channel/manifest schema rejection
and allowlist tests passed within the 65-test suite.
Implementation committed and pushed only to origin arcade-mode:
b4fb40f37acc52f99d61da562b8e43b8a63fdf48, "Add tested Arcade Lab controller, safe
deployment and hardware evidence protocol" (22 files). Credential/key/log audit passed.
Hosted run 38199528643 (separate Linux Deck workflow) passed. Run 38199528667
(Windows + Linux + macOS release-gated workflow) passed all four jobs.
Logging correction committed/pushed as e39227940ef9f536b61fd425bea0cd2823bc31a2.
Its run 38199870622 passed, including Windows and the release job.
Separate Deck run 38199870610 passed. Final self-test presentation safeguard
suppresses fixture success messages (so fake LAB READY cannot confuse users) and
refuses incomplete test installations. Committed/pushed as
f09872f179434cd3d474c9952b8325363424c308 (final implementation source).
Final local rerun: 65/65 passed, compileall and diff check passed.
Its gated workflow is [38200147202](https://github.com/notPho-Real/ReSkate/actions/runs/38200147202):
all four jobs passed. Actual job logs confirm Windows x64 build + CTest 1 passed,
0 failed; linux-tests 65 passed, 0 failed; macos-lab 65 passed, 0 failed;
deck packaging/release tests 65 passed, 0 failed; publication step succeeded.
Separate [Linux Deck run 38200147179](https://github.com/notPho-Real/ReSkate/actions/runs/38200147179)
passed another 65 tests, 0 failed. No hardware or user credentials used by CI.
Earlier run 38199528667 completed all four jobs successfully. Its publication log
explicitly said "Newer arcade-mode commit exists; leaving the update channel
unchanged." Public channel remained d83e1e6, verifying late-workflow protection
in a real hosted run (not a simulated Deck result).
b4fb40f/e392279 are superseded implementation checkpoints.

## Verified release and channel
Public channel was downloaded and validated after final CI success:
`arcade-deck-latest/channel.json` points to source
f09872f179434cd3d474c9952b8325363424c308 and immutable release
`arcade-build-f09872f179434cd3d474c9952b8325363424c308`.
Installer archive SHA-256:
9ff9be15724f977ebc7b3e9bd26c09f33e1657c2df516a307296317e4c5103c4.
DLL SHA-256:
d81bd1045b689ef346fdfce74475d37e9f47d6bc5a7af39be991f620f7d23c01.
DLL size: 13,041,152 bytes. Public archive hash, pinned origin/URL, embedded source
commit, DLL hash and x64 PE validation all passed using verified_release().
Bytes were verified in memory only; no DLL was deployed to the user's Mac or Deck.

## SIMULATED/CI VERIFIED versus REAL HARDWARE VERIFIED
SIMULATED/CI VERIFIED: all local/hosted tests above, fake Deck exact stock-byte
round trip, preferences, interruption recovery, non-mutating preflight, fake SSH,
pairing idempotency, release rejection, sanitized collection and GitHub API mocks.
Real GitHub CI/release integrity and late-workflow protection were also verified.
REAL HARDWARE VERIFIED: NONE. No real Deck SSH connection, preflight, pairing,
installation, game launch, controller event collection or restore occurred.
No persistent Mac lab installation or key was created. Outside the repository,
only temporary test fixtures/tool temporary files were used; fake HOME trees were
cleaned up. Repository-local ignored tools contain the standalone model executable.
No keys, credentials or full runtime logs are tracked.

## Historical setup blockers and actions (resolved; see current handoff)
The following records earlier states, not current blockers or instructions to repeat.
Follow-up after the user reported the Deck powered on: attempted read-only TCP/SSH
to deck@steamdeck.local. Both the socket lookup and actual OpenSSH attempt failed
to resolve steamdeck.local (SSH exit 255). No authenticated connection or Deck-side
command occurred, and no Deck files changed. The default Mac lab directory
~/.local/share/reskate-arcade-lab does not exist, so its dedicated SSH key and
known_hosts are also absent. No pairing or bootstrap was run during this attempt.
Immediate next action: obtain the Deck's current Wi-Fi IP and confirm Mac/Deck are
on the same network; retry read-only connectivity to that specific address. Pairing
will still be required before authenticated preflight unless a valid project key
is subsequently provisioned. Do not bypass host-key verification or deploy.
Subsequent user network-settings photo supplied a concrete private IPv4 address.
Targeted TCP and SSH attempts to that address returned "No route to host";
SSH exited 255. The Mac has an address in the same /24, a direct route via en0,
and an ARP neighbor entry whose MAC matches the photographed Deck. Two targeted
pings received no replies; a second SSH-port check also failed. This establishes
an addressing/neighbor match, NOT authenticated SSH or hardware acceptance.
Possible blockers remain Mac local-network permission, network filtering/isolation,
or Deck availability. Do not assert any one cause as proven. No network settings,
host keys, pairing state, or Deck files were changed. Current private IP/MAC are
omitted from tracked state; obtain the latest address from the user if needed.
Router comparison subsequently performed at the user's request: read the actual
default gateway from the Mac route table, then tested router and Deck in parallel.
Router: 3/3 ICMP replies, 0% loss, TCP port 80 reachable. Deck: 0/3 replies,
TCP port 22 still reports "No route to host"; neighbor MAC still matches photo.
Therefore this process DOES have working local-network access to the router;
a blanket Mac local-network permission block is not supported by these results.
The Deck-specific reachability failure remains unexplained; no authenticated SSH
has occurred. Next diagnosis must distinguish Deck availability/current address
from host-specific filtering or Wi-Fi client isolation, without changing settings
or assuming the router's cached neighbor entry proves the Deck is currently awake.
Important subsequent correction: user enabled Codex's Mac Local Network permission
and supplied a photo showing it on. Immediate retry then received 2/2 Deck ping
replies and successfully connected to TCP port 22. Router reachability had NOT
ruled out this permission issue; do not repeat that inference. Network reachability
and an open SSH port are now verified on the real Deck, but not authentication or
game/runtime behavior. Strict SSH stopped with an unknown ED25519 host key; the
dedicated lab private key is still absent. No host key was accepted or stored,
no public key added, and no remote command/game operation ran. Next prerequisite
is explicit first-time pairing with user host-key verification and the Deck password
entered directly into SSH's terminal prompt, followed by read-only preflight.
Latest pairing follow-up: user reported a launcher permission error. The repository
Pair Steam Deck.command is owned by the current user, mode 0755, with no quarantine
attribute; the source of that reported error is not established. Opened this exact
launcher with macOS `open -a Terminal`, which returned success. A subsequent
metadata-only check confirms the dedicated HOME lab directory and private-key file
now exist, so local pairing setup has started. Key contents were not read. Terminal
UI inspection was refused by the computer-use tool; do not bypass that restriction.
Host trust, successful authentication and authorized_keys changes are NOT verified.
Next action: user completes the visible SSH prompt in Terminal (never sends the
password in chat), then run read-only preflight. No game deployment was performed.
Follow-up connection check after the user's key-creation question: actual SSH with
the dedicated key, BatchMode=yes and StrictHostKeyChecking=yes again reached host
verification and stopped because steamdeck.local has no trusted ED25519 host key
in the dedicated known_hosts. Authentication was not attempted; key acceptance
by the Deck remains unverified. Local key creation is complete, first host trust
is still incomplete. Do not conflate these steps or regenerate the existing key.
Further local cryptographic verification: ssh-keygen successfully parsed the
existing private key; its derived public key exactly matches the saved public key.
Private-key permissions are 0600. No key material was printed. The dedicated
known_hosts file does not exist. A strict, noninteractive SSH test to the user's
supplied numeric Deck address also stopped at unknown ED25519 host identity
(exit 255), before authentication. Thus the key pair is verified valid locally,
but Deck authorization remains untested until first host trust is completed.
Old tools/arcade-deck is empty; no existing project SSH credentials were available
there for a read-only preflight. Deck reachability, SSH enablement, correct game
directory, first backup being genuine stock, actual Proton log path, focus/controller
delivery, native behavior and absence of crashes remain unverified assumptions.
Publication transport is tested with mocks; no live hardware result was invented
or published, and arcade-lab-results will be created only by an actual collection.
Mac gh authentication/publication permission will be checked when collecting.

After resolving reachability, the setup action is to double-click
`contrib/arcade-mode/Install Arcade Lab.command`. It reruns all 65 offline tests,
installs under HOME, optionally pairs via SSH's password prompt, then performs ONLY
read-only preflight. It must print LAB READY before proceeding. Pairing is optional
at setup but necessary before strict SSH preflight if no key was authorized.
The user then chooses Deploy Latest Arcade Mode explicitly, launches Skate, tests
A/B/X/Y taps/holds/native controls, collects, closes the game, restores stock, and
collects again. See LAB_PROTOCOL.md for action locations and precise semantics.
Do not begin A/ollie native reverse engineering until observer acceptance is backed
by real evidence. Following gameplay milestone remains A -> native-quality ollie,
then X -> one fixed native flip, then complete grounded/air/grind/manual/landing/bail
observation. Primary technical risk remains commanding Skate's native trick state machine.
