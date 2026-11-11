# Casper investigation and portable prototype

Source checkpoint: `b12327c3ee302e6be5e9d0e5478157581434d23a` on
`arcade-stage-unified-rc5`; isolated branch `arcade-stage-casper-spike`.
The checkpoint passed [integration run 42341810077](https://github.com/notPho-Real/ReSkate/actions/runs/42341810077).
This is experimental source work, not an accepted runtime or native gameplay result.
Sections 1–7 record the original portable prototype. The continuation section
below supersedes their runtime-status and playback-transition statements.

## Audit scope and hypotheses

Read AGENTS.md, .codex/README.md, STRUCTURE.md, MANUAL_EPISODE.md,
CLASSIC_CONTROLS.md, UNIFIED_RC5.md and unified-sources.json. Searched the complete
checked-out upstream engine/extension tree, Arcade, and the existing fork survey
and saved comparison responses. Upstream is pinned at `95b0a436`; the fork survey
is historical, not a new exhaustive remote inventory. Its friction-tuning lead
changes global settings and is unsuitable for an episode-scoped custom trick.
No reference animations, game assets or extracted native code are imported.

Three bounded hypotheses: (1) the shared journal can dispatch flatland gestures
without releasing Manual; (2) existing original-step admission and resistance
math can supply monotonic trick-local drag; (3) newly authored body/board keyframes
can use the existing finite clip lease. These are portable software hypotheses.
No build download, full Windows build, hardware session, merge or promotion is
needed to test them. RC4 and the unified integration checkout remain untouched.

## 1. Original event grammar and Arcade mapping

Reference: the original implementation repository supplied in the assignment,
revision `98b4e24921446ccd4b157453e25697f9574f0053` (SwagSoftware).
`Scripts/game/skater/manualtricks.q`: `TRIGGER_MANUAL_BRANCHFLIP` (6, ordered
Square→Square within 200 ms), `Trick_Casper` (119),
`CasperBranches` (151), `FlatLandBranches` (187), `Manual` (258),
`ManualLink` (637). Casper is a Manual script configuration with distinct entry,
range and out animations, board rotation and friction 0.5. Manual installs its
specific extra table before the common flatland table (422–427), excluding the
current trick name. Consequently the repeated common X→Y row falls through to
normal Manual when Casper itself is excluded; this is contextual fallback, not
a timeout action. Casper Flip uses ManualLink and returns to the Casper range.
The spike deliberately chooses the permitted alternative of returning to Manual
after the flip, limiting the first visual lease.

`Code/Gel/Components/trickcomponent.cpp::QueryEvents`, InOrder case (866–913),
scans backward from the latest event for the second then first press. Releases
and unrelated events may intervene. At query time **either event age >= window
fails**: these are strict 300/200 ms windows, not just inter-press intervals.
Only a complete match commits MaybeUsed to Used. `TriggerAnyExtraTrick`
(2372–2465) considers defined, nonexcluded rows in table order, with
`USED_BY_EXTRA_TRICK` and reject mask `~USED_BY_MANUAL_TRICK`.
`TriggerAnyExtraTricks` (2471) checks special and installed tables in order;
scripts can kill the old table immediately. `IsExcluded`/`CalculateIgnoreMask`
use trick names, not a second parser. Simultaneous sampled edges have journal
order, not proof of distinct physical chronology; the spike requires a prior X
for reserving a new Y.

Arcade already implements these tables in
`arcade_input_dispatch.h::UnifiedInput::recognize_branches`: context 0 is Manual,
2 is Casper, 42 is its flip continuation; X→Y is target 2, X→X in context 2
is target 42, and X→Y there returns target 0. It uses
`InputHistory::match/consume`, original timestamps, the same strict windows,
specific-before-common priority and branch consumption. Branch recognition runs
before ordinary face actions. `InputSession::receive` already forwards these
semantic requests to `InputAdapter`. Missing behavior is a concrete custom
adapter, plus routing/ownership of Y—not an absent grammar.

`InputSession::poll` already masks ground X/B from ManualActuation cancellation
(`~0x6000`), so the first X survives on this checkpoint. Y remains in the
`ManualActuation::sample` cancellation mask and is sampled **before** recognition.
The opt-in spike reserves Y only when an established, independently admitted
Manual has an unconsumed X within 300 ms in that same shared journal. A rejected
adapter withdraws the provisional reservation in the second actuator sample.
Holding an accepted Y does not cancel on the next poll. A alone exits immediately;
plain/expired Y retains its previous immediate behavior. Nothing waits for a
timer before dispatching an ordinary airborne flip or an ordinary grind. X
alone has no grounded flip to replay. OFF/reset require normal release/rearm.

The original spin branches use shoulder-trigger events. Arcade's existing
LB/RB rotation owns those controls; the spike adds no second spin handler and
does not repurpose RT (native revert). Live Casper spin/contact compatibility
is still unverified. Up/Down correction and A keep their established roles.

## 2. What friction 0.5 actually means

`SkaterCorePhysicsComponent.cpp::apply_rolling_friction` (2085–2114) subtracts
the vector `velocity / |velocity| * 60 * m_rolling_friction * m_frame_length`.
It stops at zero instead of reversing; very small speeds are zeroed. The source
uses inches and seconds (speed documentation at 542–543; `Update` assigns
`Tmr::FrameLength` at 182). Thus 0.5 means **30 inches/s² = 0.762 m/s²** of
rolling deceleration: at 60 Hz, 0.5 inches/s per step. It is not dimensionless
physical Coulomb friction or a percentage of speed. The original vector is
full velocity; the spike deliberately limits its added drag to horizontal motion.

`handle_ground_friction` bypasses both wind and rolling resistance when autokick
is off, not bailing, and the rolling value is default. `handle_rolling_resistance`
also yields to `slide_off_slow_steep_slope`; wind resistance is separate and
quadratic in speed. The original 0.5 cannot promise a particular stopping distance
independent of those forces. `physics.q` declares the default and explains the
constant deceleration, with differing physics blocks; do not assume one global
script default across all contexts.

`Manual` explicitly sets the supplied value or Default (357–361). `ManualOut`
only clears trick presentation/extra tables; it is **not** a friction destructor.
`tricks.q` ground initialization (308) and air initialization (378), and bailout
scripts (e.g. `bails.q:909`), explicitly restore Default. The C++ setter (701–709)
honors Default only while `m_special_friction_duration == 0`; timed special
friction restores its own default on expiry (1454–1463). A reset must not be
inferred from an animation ending or a component method called Reset.

Arcade already has the inch conversion, wind and rolling selection in
`arcade_movement.h::GroundFriction/GroundResistance`. The proposed stronger
Casper default is **6 m/s² rolling drag**, a new provisional tuning choice, not
a historical constant. Suggested proof-only mappings are ordinary/unknown 6,
rough 8, sealed 3, verified slick 0.75 m/s². These values require later gameplay
tuning. Original wind math remains in the reused resistance adapter.

## 3. Surface evidence: data exists, live contact binding is missing

`Engine/Resource/material_grid.h/.cpp` describes and combines the real physics
MaterialGridData. Collision words pack flags in bits 0–5, material slot in 6–18
and property slot in 19–31. MaterialIndexMap resolves rows of MaterialProperties
and InteractionGrid. Custom maps remap slots; a bare slot is not a stable global
identity. `Engine/Game/Build/20260929/native_vfx.h` identifies a live grid and
packed impact material for effects. These are real source leads, not proof that
Arcade currently reads friction under the supporting wheel.

`arcade_surface_geometry.h::SurfaceGeometrySample` carries body identity, point,
query normal and lifetime context. It explicitly is query geometry, **not**
support/contact proof, and has no material/coefficient field. NativeInputFrame
has wheel contacts/state/generations but no material. `placement_adapters.cpp`
has moving-object grind friction handling, not a supported static ground
material reader; physics tuning changes global behavior. Render materials and
shine do not supply physical coefficients.

Missing: a fingerprinted read of the **actual supporting collision part's packed
material/property**, its current world/grid generation and remapped row, and the
verified friction-property/interaction semantics for board-ground contact.
The most promising next step is joining that native contact to the existing grid
decoder, with bounded row checks and owner/world invalidation. There is no live
producer in this spike. All native surfaces therefore remain unknown/default;
lower-friction test fixtures are explicitly supplied evidence, not detected metal.

## 4. Custom animation and contact preservation

`AnimationStudio/anim_core.h/.cpp` offers Clip, Track, Key, quaternion interpolation,
`set_key`, `apply_edits` and board tracks. A clip can be authored from an owned
395-joint seed pose; it need not select a preexisting trick. `game_clips::Library`
loads local clips and can mirror across rig-aware paired joints, but requires the
user's local rig/game data. `skater_animation::record/take_recording` is another
local seed source. No seed recording is committed here.

`skater_animation.h/.cpp` applies body poses at render submission because native
body physics rebuilds the animation buffer first. Placement joints remain
native. Its Play contains the body's Clip and the placement-relative board track;
`board_pose.h` separately documents the board's 16 render bones. Neither path
changes rigid-body orientation, contact shape, truck support, force feedback or
native trick identity. Keep the real Manual RS/contact adapter active and invert
only the rendered board. This is an explicit visual approximation; sole/board
intersections, banks and moving supports still require actual render/contact QA.

The prototype authors additive entry, held body pose and flip-exit keys with
`anim_core` and samples them into ordinary Clips. Unmodified joints preserve the
seed's proportions. Board roll uses several sub-180° keys so shortest-arc slerp
does not erase a full flip. Regular/fakie movement uses the same local pose and
native world placement; moving backward must not reverse drag. Goofy requires
a correctly mirrored rig/seed and remains a separate live validation case.

`try_play`, `owns_play`, `cancel_play` already provide ticket ownership and refuse
to steal another caller's clip. `try_play` rejects loop/paused clips. Therefore
this spike uses a bounded 15-second entry/hold/return clip and a finite flip exit,
not an unbounded loop or repeated resubmission. The next reusable improvement is
an owner-checked replace/loop/seek lease with phase-preserving blends; ordinary
play/seek/pause must continue to revoke the old ticket. Existing playback uses
wall time; synchronizing poses with native simulation time is another explicit
live integration requirement. Physics drag always uses native dt.

## 5. Manual lifetime, balance and scoring

Reuse `ManualEpisode::serial`, `actuatable`, `observe`, `delivery` and its
independent contact/owner admission, `GroundManualBalance::sample_episode`,
`ManualActuation`, `ComboSession::step` and the normal balance HUD. The custom
state is a presentation subtype attached to one admitted Manual serial, not a
new Manual, new balance model, fabricated native contact or scoring receipt.
Keep original request timestamps, balance drift/fatigue and combo identity.
Original `SkaterBalanceTrickComponent::CallMemberFunction(DoBalanceTrick)` likewise
returns without resetting when the balance type is unchanged (170–172).

The prototype does not add points, multiplier, native labels or native trick IDs.
Casper Flip is a newly authored visual exit, not proof of native score acceptance.
Clean landing/banking and bail remain owned by existing physical observations
and score settlement. A, OFF, owner/episode loss, stale observations, competing
playback, bail, offboard, rail contact and reset revoke the subtype and its drag.
No global Skate rolling-friction parameter is written or restored.

## 6. Minimal implementation and bounded validation

Add an opt-in Y reservation in InputSession, a presentation-only flatland adapter,
authored Clip factory, and a drag selector composed with GroundMotionStep and
GroundResistance. Use the same native writer transaction in a future live wiring:
`client_noclip.cpp` already revalidates NoBailSkater/OwnedMotionBodies, admits
original steps, computes scalar resistance along XZ velocity and applies one
delta to the owned body set. Replace that step's friction selection; **do not**
install a second writer or run normal and custom resistance budgets together.
Keep GroundMotionStep's watermark across cancellation/OFF to prevent duplicate
spending; gaps rebaseline instead of accumulating missed time.

The branch remains portable-only: no runtime switch, default-enabled producer,
Windows build or hardware claim. Focused tests will exercise production
InputSession/recognizer, original-observation admission, resistance and anim_core;
injected native observations/player endpoints are simulations and are labeled so.
Independent future acceptance requires native Manual support through the gesture,
loaded binary identity, real cached dt at 30/60/120 Hz where available, motion
decay along measured XZ velocity, owner cancellation and frame-by-frame body/board
continuity. Negative controls: OFF, ordinary Manual, plain Y, expired X prefix,
unknown versus verified surface, another animation owner, bail and reset. Any
later test must obtain the existing exclusive hardware lock after RC4/RC5 users,
verify Mega Park, and preserve rollback bytes. No hardware is requested now.

## 7. Delivered result and remaining gates

Implemented on the isolated branch:

* `arcade_input_session.h`: default-OFF capability, a separate
  `custom_flatland_branch` field (not native `observed_balance_branch`), and
  pre-recognition Y reservation with same-poll rejection cleanup.
* `arcade_custom_flatland.h`: finite presentation subtype bound to full native
  owner and Manual serial, explicit surface-evidence boundary, native-step drag,
  and lifecycle cancellation. It never calls balance setup or score ingestion.
* `arcade_custom_flatland_clips.h`: original body/board entry, hold, and 540°
  visual flip-to-upright exit authored through the existing animation math.
* `arcade_custom_flatland_playback.h`: client-thread try_play/owns_play/cancel_play
  wrapper, compiled in the portable target but not installed in the runtime.
* `Test/custom_flatland`: standalone CMake target, small Python compiler driver,
  run instructions, and a production-composed simulation.

Final portable validation on the local Apple Clang toolchain:

| Target | Result |
|---|---|
| Custom flatland slice | 4,006 checks passed |
| Ground motion original-step ownership | 3,751 passed |
| Ground controls/resistance | 5,000 passed |
| Production input session | 6,485 plus 326 jump-idle checks passed |
| Composed Manual balance lifecycle | 285 passed |
| Interleaved combo traces | 1,736 passed |
| Existing input-pattern dispatcher | Fails the contextual air-table 300 ms assertion on both spike and untouched RC5 |

The failing assertion is in `arcade_input_pattern_dispatch_tests.cpp:246`, in
`contextual_predicates`, using UnifiedInput directly. It precedes the session
cases and does not execute the changed Y reservation. Rebuilt with the exact
baseline InputSession header and otherwise unchanged baseline dependencies: the
same failure reproduced. Do not repair unrelated air-table semantics in this
spike or claim all integration gates green. `run.py --integration` preserves a
nonzero result for it. No Windows CI/build was launched.

The spike/test translation unit compiles with Wall, Wextra, Wshadow, Wconversion,
Wsign-conversion and Werror. Upstream anim_core is compiled separately (it has
existing unused JSON helpers and signedness warnings). The old combo trace
fixture uses its Wall/Wextra/Werror policy; adding conversion warnings exposes
existing float-to-bool assertions. No upstream code was edited to suppress these.

The balance composition drives the real ManualPhysics and ComboSession alongside
an ordinary-Manual control, checking drift, RNG, fatigue, HUD state, points and
episode continuity throughout entry/flip/return, then native-observation bailout.
The animation tests sample and serialize actual authored Clips. The player and
native observations are injected; these are **not** live rendering, native Casper,
collision compatibility, landing/bank or score-acceptance evidence.

Concrete remaining work before an in-game candidate:

1. Add a bounded client/native bridge under the existing input/physics ownership
   gates; prewarm the current animation hooks and obtain a local owned seed.
   Route exactly one scalar resistance budget through the existing body writer.
2. Replace the finite prototype's stop/reborrow transition with a generic owned
   loop/replace lease, synchronize playback time, and validate blends and sole/
   board clearance on real poses. A seed is mandatory; the synthetic test rig
   is not a production animation asset. Regular/fakie, goofy and banked contacts
   need visual/native validation. Shoulder spin remains the existing controller
   responsibility and is not accepted by this test.
3. Implement the actual support-material/grid join described above before
   enabling surface-specific behavior. Unknown and stale surfaces use default.
4. Review GroundResistance's inherited unknown-fast-arrival protection: at speeds
   above its known ordinary range, it withholds resistance until ordinary limits
   are established. The spike intentionally does not defeat that existing gate.
   Any extension must preserve native impulses rather than clamp unknown boosts.
5. Resolve or independently qualify the preexisting air-table integration failure
   with the integration owner before a full Windows candidate. Coordinate later
   hardware through the exclusive lock; no remote task messages were sent.

## Storage receipt

Audited project artifacts (5.2 GB), shared git objects, registered managed
worktrees and historical Documents/Codex checkouts by size. No external SSD was
mounted. Existing cleanup receipts already protect historical runtime/installer
bytes, native captures and RC2/RC3/stock rollback files. No active RC4, RC5,
Trickpedia or CI worktree was cleaned or altered.

Compressed three inactive historical text logs after verifying decompressed
bytes and SHA-256: `ci-local-tests.log` (66,109→11,121), `ci-local-final.log`
(66,046→11,069), `source-survey/tests.log` (26,624→4,903). Receipt:
`artifacts/casper-tests/cleanup-receipt.json` in the primary project checkout.
The new worktree is sparse (source directories only), avoiding retained copies
of unused assets and vendor trees. Only bounded compiler outputs were generated;
their hashes and source provenance are retained in the validation receipt before
removing regenerable binaries/objects. No source evidence was downloaded twice
or copied into production code.

## Continuation from the preserved runtime checkpoint

Continued the existing clean `arcade-stage-casper-spike` worktree at
`c610705daec398f6b29d3b989bd2d74eb555cd38`, preserving both that WIP and
`99f06938686d9f01e95979107cfb9cdb22c49f9b`. Reused the source survey above;
no new implementation or investigation branch was created.

The WIP already wires the admitted Manual context through arcade_mode.cpp,
the client/native mailbox, the existing resistance writer in client_noclip.cpp,
and skater_animation's native seed observation and controlled playback.
The bridge is compiled live only with DINGOSDK_DEV_TOOLS. Its release symbols
remain inert. The controlled player holds the authored entry at 0.3 seconds
and advances the flip by admitted native dt; the finite portable wrapper remains
available for the original tests.

The demonstrated gap was transition timing/ownership: the mailbox cancelled
and reborrowed the entry ticket for the flip, restarting the player's blend from
the native pose. Added generic `skater_animation::replace_owned` to replace a
current externally clocked clip while retaining its ticket and blend progress.
The existing authored hold and flip-start poses match across all body joints and
the board. A failed or stolen replacement cancels the custom phase, cannot
reborrow the player, and cannot authorize drag. Animation loss now also withdraws
input readiness before the next client tick.

Ordinary seek/pause revoke the controlled ticket and clear its heartbeat expiry
so the ordinary player's continued playback is not timed out by the former
owner. Even a redundant pause revokes the ticket. Animation tick revokes a
controlled play on gameplay disable, owner change or timeout. These native
animation changes require the Windows build and live render validation; the
portable endpoint is simulated and does not prove those code paths.

Validation before edits: 4,006 flatland and 1,248 runtime checks passed. The new
transition regression then failed on the unchanged runtime, and passed after
the fix. Updated focused results: 4,798 flatland checks, 1,318 runtime checks,
and the release-disabled link/run check. Five related suites pass: ground
motion (3,751), ground controls (5,000), input session (6,485 plus 326 jump-idle),
Manual balance lifecycle (285), and combo traces (1,736).
The input-pattern suite fails `a failed category/adjacency query leaves the
fresh face footprint unconsumed` on both the preserved checkpoint and the
continuation. The WIP's earlier fixture-policy edit already moves execution
past the historical 300 ms failure; this continuation does not modify that
suite or claim an all-green integration run.

The Python runner builds the portable code with its existing strict warnings
as errors. No supported MSVC/Windows runtime toolchain is available in this
local macOS session; no Windows runtime build or hardware trial was performed.
This remains an unaccepted dev candidate. Next gates are a clean supported
Windows build, the unresolved recognizer integration failure, and the native
acceptance/negative controls already specified above (including verified Mega
Park, real clock/cached dt, render continuity and physical support).
The outer runtime call sites and native animation implementation are not
covered by the portable executable. Live support-material detection remains
absent; all native surfaces use the existing unknown/default choice.


## Candidate test procedure

The next candidate preserves the same implementation and adds an explicit,
unsaved `arcadeflatland on/off` dev switch. It starts OFF on every process launch.
`arcadeflatland` reports the switch, last phase, seed readiness and current
animation receipt; those fields describe presentation ownership, not native
trick acceptance. Opening the console cancels gameplay ownership.

The input-pattern integration failure is resolved. A failed directional grab
query discarded its selected direction, allowing the neutral fallback to consume
the same B press. The fallback now consults recent canonical direction history
even if another action cleared the cached direction. The existing rejection
tests pass, with additional positive controls for actual neutral grabs and
expired directional history. All six focused integration suites pass, including
9,174 input-pattern checks. The Casper suites pass 4,798 flatland checks and
1,352 adapter checks plus the release-disabled check. Adjacent regressions pass:
native grabs (20,465), neutral flip release (9,014), control state (67,111),
input consolidation (1,015), and action dispatch (124).

Full and targeted CMake builds now include the three Casper executables.
The targeted Windows lane also runs the input-pattern suite. The release test
explicitly undefines the dev macro inherited from a full dev build.
Native seed observation now snapshots and revalidates one owner generation
under its existing mutex, including board-holder changes, before publishing
a pose. It cannot tag an old evaluation with a new owner's entity.

The first hosted candidate passed targeted MSVC Casper/input tests. Full CI
also exposed two GCC signedness warnings in the shared revert/board headers and
a missing disabled custom-animation boundary in the Manual source-composition
fixture. The follow-up preserves strict warnings and the production call
extraction: the existing Manual fixture supplies no custom-animation authority.
Local strict builds pass 689,573 lifecycle and 698,774 reservation checks; all
five reservation rollback controls still fail at their intended assertions.
The focused Casper/input/motion suites also pass again. The controller
publication fixture also needs a nonactuating custom-animation cancellation
sink. Its source-bound checks now require selected-controller withdrawal to
cancel Casper once and unrelated-controller withdrawal to preserve ownership.
The controller/publication tests pass; removing the actual cancellation makes
the new assertion fail. Hosted validation of the corrected candidate remains
required.

Before gameplay, require the exact candidate's successful Windows runtime build
and CI tests, and record source SHA, run/attempt/artifact ID and DLL SHA-256.
The hosted CI artifact is a candidate for testing, not gameplay acceptance.
Use the existing reviewed installation/handoff procedure; do not replace the
trusted runner's pins or use hardware occupied by RC4.

1. Verify Skate build 20260929 and that Skate 3 Mega Park is loaded. Start on
   flat ground in regular stance, with ordinary Manual working.
2. Enable Arcade Mode with `arcademode on`, then `arcadeflatland on`. Close
   the console, release the buttons, and establish a fresh ordinary Manual.
3. Tap X, then Y within 300 ms while retaining Manual. The rendered board should
   turn into Casper and hold. Native Manual support/balance must continue.
4. After entry settles, tap X twice within 200 ms. The board should perform the
   authored flip exit and return to the continuing Manual without a pose flash.
   Repeat X then Y during the hold to return directly. A should exit immediately
   through the normal launch path.
5. Repeat with `arcadeflatland off`, plain Y, an expired X prefix, focus loss,
   bail and another animation owner. These must not leave a Casper pose or drag.
   Retain native contact/owner and original-step evidence independently of HUD
   labels. For drag comparisons verify actual clock cadence and cached dt first.

First-test scope is regular, flat-ground presentation and cancellation. Goofy,
banks, moving supports, shoulder-spin compatibility and surface-specific drag
remain separate gameplay questions. The authored pose is a rough prototype;
body/board clearance still requires visual review. No custom native score is
claimed. Rollback is the switch OFF plus the existing validated DLL rollback
procedure if required, preserving all stock/development backup bytes.


## Deck admission failure and correction — 2026-11-09

The exact 90ea3bb0 candidate passed full CI 42556682210 and was then tested on
Steam Deck in the owner's exclusive Casper slot. Loaded DLL mappings and hash,
the pinned 20260929 executable, and the live Mega Park descriptor were verified.
Ordinary Manual admitted the native tail pose, rear-axle support, balance and
history. The original physics and cached skater timestep both read 1/30 second.
This is a baseline, not completed cadence or Casper acceptance.

The first completed X-to-Y trial failed: Y withdrew Manual pressure and the
board returned to both axles while the Manual HUD label lingered. The existing
runtime input gate required `!authored`, but that variable means a nonempty
prepared gesture catalog, not active authored playback. A loaded catalog
therefore disabled the Casper capability on every input poll. The correction
removes that availability veto; InputSession still rejects actual authored,
auxiliary and external owners before reserving Y. Gameplay, control ownership,
release, grind, trigger and menu gates remain in place.

The adapter tests now compile the actual outer admission statements from
arcade_mode.cpp, through both the portable runner and CMake. With the old gate,
the new fixture fails to enable a fresh matching seed. With the correction,
1,382 adapter checks, 4,798 flatland checks, the release-disabled check and the
six adjacent integration suites pass locally. Prior animation-system work is
unchanged. A clean Windows rebuild and repeated live entry, exit, cancellation,
render/contact continuity and motion trials remain required. This candidate is
not yet ready for owner testing.


## Deck validation of d7568193 — 2026-11-09

The preserved implementation, including the animation-system changes, now passes
the supported Windows build and every required job in
[full integration 42698106033](https://github.com/notPho-Real/ReSkate/actions/runs/42698106033).
The runtime artifact is 13711708834; the Deck artifact is 13713455438. Both identify
source `d7568193a351dd041286bc07e91f9005a2eb1903`. The tested DLL is 44,216,320 bytes,
SHA-256 `9d7f1e9e4fde249d7bf6c099ce27721fe6845c31e0287859d155912996fc511a`.
The MSVC follow-up only removes a test variable shadow; it retains the admission
fix and all previous animation work.

The agent drove the actual Steam Deck remotely with bounded virtual-controller
input, exclusive `/dev/uinput` locking, private video and independent native
observations. The loaded DLL, pinned executable and live Mega Park descriptor
were checked. Local evidence is under the shared SSD artifact directory
`artifacts/casper-hil-d7568193`; `test_matrix.json` records accepted evidence and
failed or inconclusive attempts separately. No captured game assets are added
to this repository.

| Check | Observed result |
| --- | --- |
| Default OFF and ordinary Manual | Passed; fresh launch reported OFF, baseline admitted Manual and rear-axle support. |
| X then Y entry and hold | Passed; inverted rendered board with the same native owner, Manual episode and rear support. A longer hold recorded 61 updates/61 renders and zero stale samples. |
| X then X flip exit | Passed at measured 30 and 60 Hz; continuous authored flip, then continuing native Manual. Frame-by-frame review found no transition pose flash. |
| X then Y direct return | Passed in `direct_short30_after_archive`; native Manual remains active after the return. The earlier long trial reached the ordinary balance limit and is not its acceptance proof. |
| A exit | Passed; normal native launch/air/landing sequence `100 -> 103 -> 200/201 -> 100`, with custom pose cleared. |
| OFF, plain Y, expired X prefix | Passed; no Casper entry and ordinary Manual release. The expired interval measured 442.644 ms. |
| Controller withdrawal and bail | Passed; custom presentation cleared when the controller binding was revoked and on independently observed native bail. |
| Another animation owner | Passed in `owner30_v2`; ordinary plugin playback took over during Casper and remained playing after Casper cleanup. The temporary fixture was removed and host unload confirmed. |
| Gameplay focus transferred to ReSkate menu | Passed in `focus_menu30`; the production focus flag became false, Manual intent became zero, two-axle riding returned, and video showed the custom pose cleared. |
| OS foreground transferred to another Wine application | Passed in `os_focus30_deferred`; actual foreground changed, the binding was revoked, native Manual cleared, and the normal board pose returned. Temporary window and compositor controls were cleaned up. |
| Horizontal drag and cancellation control | Casper decelerated more strongly than the matched ordinary control. A final ordinary Manual after all cancellation trials matched the initial control closely and showed no retained custom pose or added drag. |

The motion comparison is live forward, regular-stance evidence, not a measured
coefficient for every surface. At 1.8 seconds, horizontal speed was approximately
0.019 m/s with Casper, 0.104 m/s in the initial ordinary control, and 0.106 m/s
in the final ordinary control. Exact scalar resistance, reverse-vector behavior,
and the absence of a vertical write are also covered by the portable tests;
a live backward/fakie trial is not claimed.

Measured native callback rates were 30.0099 Hz and 59.9874 Hz, with physics and
cached timesteps matching their requested rates. Requesting 120 Hz produced
90.708 callbacks/s on this setup; it is not a true 120 Hz pass. Both timestep
fields were restored to 1/30 second before handoff.

Operating-system foreground loss passed in `os_focus30_deferred`. The reviewed
video shows the inverted Casper pose before a temporary Notepad window in the
existing Wine session takes foreground. Production source-6 evidence changes
focus/gameplay to false and revokes the controller binding. Independent native
reads change Manual intent from 1 to 0, rear support to two-axle riding, with
state 100 retained. The returned game renders the ordinary board pose. The
foreground window stayed selected for the bounded 0.5-second interval; it was
closed normally, both temporary Gamescope window controls were restored to zero,
and no Notepad window/process remained. Physics and cached steps were 1/30;
the measured stable-owner callback rate was 29.3658 Hz during this trial.

Earlier overlay, helper-window, unmap and minimize attempts did not establish
Wine foreground loss and remain excluded. Setup attempts with native menus open
or without a native Manual prerequisite also remain excluded. The successful
trial launches the foreground target only after Casper entry. Its initial speed
was higher than the matched drag comparison above; it is cancellation evidence,
not a replacement matched-speed experiment.

The authored pose has now been visually reviewed on the tested character. It
remains a rough prototype; sampled native joint positions are not proof of the
rendered override's exact sole/board clearance. Goofy, banks, moving supports,
shoulder spins and support-material-specific drag retain their earlier exclusions.
No custom native trick ID or score acceptance is claimed.

The final ordinary control passed at 30.0021 Hz. The game was then closed normally
for a user-requested temporary RC4 hardware slot. Independent stopped checks
confirmed no Skate or launcher process. All 47 prior rollback directories
(141 files) verified unchanged, and an additional exact Casper DLL/settings/receipt
backup was created for RC4 to restore. The hardware release receipt is
`handoff_to_rc4.json`. No Casper hardware operation may overlap RC4's slot.

RC4 subsequently returned the exclusive hardware slot. `verify_rc4_return.json`
independently confirmed the exact Casper DLL/settings/receipt, stock backup and
all 48 rollback directories before the final foreground test. After that test,
native riding/Manual-zero and both 1/30 timesteps were verified; the game exited
normally. The first shutdown check caught the launcher's brief shutdown tail;
`final_launcher_inventory.json` and `final_stopped_verification.json` then
confirmed both processes stopped and the exact installation and all backups
unchanged. Temporary animation and foreground fixtures are absent.

The regular-stance, flat-ground prototype is ready for owner testing. This
supersedes the older pending-build, untested-entry and pending-focus statements.
It does not broaden the scope exclusions above or establish true 120 Hz operation.
The tested d7568193 runtime remains installed; launch normally, enable
`arcademode on` and `arcadeflatland on`, close the console, and establish a fresh
Manual before trying the entry/exit sequences above.


## Held turns and contact materials — candidate, 2026-11-09

This extends the existing d7568193 spike and its owned animation player.
It has not yet passed the new Windows build or Deck acceptance matrix.
Earlier acceptance above applies only to the earlier flat-ground prototype.

Original reference CasperBranches uses L2/R2. ManualLink rotates the display
360 degrees in 0.75 seconds, with opposite signs. LT/RT now continue that
rate while held in Casper; release or opposing triggers freezes the display
angle. Native placement and physics orientation remain owned by Skate.
Preheld triggers do not enter Casper, and the active lease consumes them
without interrupting Manual balance correction. X-X retains the display
angle through the authored flip; cancellation releases the display lease.

The supported native collision publisher resolves both colliding materials,
including terrain and per-triangle mesh materials. The owner-relative contact
copy selects the other surface into raw contact +0x44. The pinned contracts
are in contact_material.h; no engine material is changed. The existing board
contact hook captures the material and publishes only after the original
processor returns. The reader requires the same owner, physics thread, flat
static support, matching contact identity and a new publication in the current
native step interval. Disagreeing supports are unknown.

The live world's MaterialIndexMap remaps the packed slot into its property
row. A bounded double read resolves the gameplay property and named enum,
rather than assuming a stable slot number. Developer Deck research confirmed
Default at the Mega Park flat test lane and resolved the named enum catalog.
The property's other floating values have not been identified as friction.

Design resistance is 6 m/s² for ordinary/default, 8 for rough, 3 for sealed
wood/plastic and 0.75 for slick metal/marble/glass. These are explicit Casper
tuning values. Unknown or stale evidence retains 6 m/s². The existing single
velocity transaction applies the selected resistance; there is no second
physics writer or global material mutation.

Portable coverage includes live slot remapping, malformed/ambiguous properties,
enum validation, changed reads, stale/duplicate publications, owner reuse,
native time mismatch, surface selection and rate-independent drag. Developer
status reports bounded request counters and the last requested resistance/yaw.
These are requests, not proof of final movement or rendered pose.

Private source and native research, transport receipts and subsequent hardware
evidence live under artifacts/casper-spin-surface outside the repository.

## Authored motion candidate — November 10, 2026

This continuation preserves the accepted gameplay source at ec391bdd and changes
presentation. Native visual acceptance is still pending.

The motion source is animations/casper.motion.json. Entry loads the rear leg,
separates the feet while the board rolls, then catches a steep nose-up inverted
board. The held range has a front instep target on the grip side and a rear sole
target at the tail. The native seed supplies stance and bone lengths. The flip
has anticipation, release, clearance and recovery keys. Direct return reverses
entry and fades to the current native pose. Existing Ollie, bail, OFF,
focus/controller loss and competing-owner boundaries cancel immediately; an exit
animation never delays native gameplay.

physical_manual.snapshot().position selects the held range; its dynamics, input,
fatigue and ownership are unchanged. The first 0.10 seconds of hold ease into
that range. Flip/direct exits join the selected range pose. Shoulder display
rotation remains through exit and disappears with its blend envelope. Direct
return stops custom drag when requested, as before. No material classification,
friction coefficient, controller mapping or native contact is changed.

### Reusable authoring workflow

1. Author named phase keys in JSON. The compiler rejects invalid dimensions,
   non-finite values, unordered times and invalid weights. Regenerate with:

       python contrib/arcade-mode/animations/compile_motion.py contrib/arcade-mode/animations/casper.motion.json Extension/ArcadeMode/arcade_casper_motion.inl

   Add --check to detect stale generated data without writing it.

2. Resolve the target rig from a fresh native pose. Work in its placement frame,
   preserve parent-local bone translations, and determine stance from feet relative
   to the board. Animation Studio's motion_authoring.h offers a generic two-bone
   target solver, named phase baking, smooth quaternion/position interpolation,
   reversal and continuous phase joins. It uses the existing pose math and Clip
   representation. There is no additional runtime player. Imported regular clips
   retain the existing rig-aware Library::mirror; blend masks mirror with joints.

3. Bake phases into Animation Studio clips. Supply intermediate orientation keys
   for turns greater than 180 degrees. Per-joint masks keep unowned face/finger
   motion native. The envelope blends body and board back to the current native
   pose. Weighted files use FAS4; unweighted exports remain FAS3, and FAS1–3 imports
   retain previous defaults. Export/import validate shapes, finite values, times
   and blend bounds.

4. Run portable authoring and gameplay tests before a full runtime build:

       python Extension/ArcadeMode/Test/custom_flatland/run.py --output <SSD-output> --integration --sanitize

   Tests cover both stances, three synthetic body sizes, range contact error,
   unchanged limb lengths, phase continuity, reversal, blend serialization,
   malformed files and existing input/drag/ownership boundaries. Sanitizers caught
   a board-track ID incorrectly used as a body-joint index; that error is fixed.

5. Build an exact-source experimental DLL once a candidate is coherent. Inspect
   actual front, side and rear rendering in both stances, entry, spin, flip,
   direct/Ollie exit and cancellation. Synthetic target error is not proof of
   shoe-mesh contact, ground support or acceptable animation.

### Reference and limits

The sequence follows the previously audited entry/range/out behavior. Public
gameplay footage around 2:13:16–2:13:18 supplied visual reference for foot
separation, the steep held board and upper-body counterbalance. Reference URLs
and extraction notes remain in the private SSD evidence directory. No proprietary
skeletal animation, mesh or extracted game asset is included in source/packages.

Source skeleton/animation formats and existing import tools were inspected, but
no local source-game installation or compatible extracted rig was verified.
These are newly authored controls retargeted to a live pose, not converted assets.

The rendered board remains separate from the native rigid board and contacts.
The solver places visible ankles; it does not create support, move physical
geometry, suppress bails or establish custom scoring/trick IDs. Native visual
and contact acceptance, goofy gameplay and exact DLL installation must be
recorded after the coordinated hardware session.

### Native contact correction

The first authored candidate was rendered in Mega Park on the Deck. Entry,
balance-driven hold, left/right display rotation, and the flip returning to
Manual were observed. Frame review rejected its floating lower board tip,
over-raised hands, and limited shoe clearance during the flip. The first cold
animation-hook installation also exceeded the existing controller selection
lease; a warm trial retained ownership. None of those observations is a final
visual acceptance.

The follow-up uses the existing playback board support geometry in authoring,
fits the displayed held board to the native skeleton's Ground height, places
the hands below their shoulders, and increases released-foot clearance. It
prepares the existing animation hooks while the opt-in is idle, before Manual.
No collision contact, rigid-body position, balance rule, friction selection, or
controller mapping changes. Synthetic support, hand-height, mid-flip clearance,
and cold opt-in tests cover those fixes; the revised exact-source DLL still
requires native visual and full regression acceptance.

The second exact-source candidate passed the fresh-process entry check and
retained Manual through left/right rotation and flip recovery. Native rendering
showed improved released-foot clearance, but an elevated native clavicle still
raised one hand and the fixed-world wrist orientation curled the hand. The next
correction projects shoulder spread horizontally, solves from the posed shoulder
with a downward elbow pole, and preserves wrist-local rotation. An elevated-
clavicle regression fails before the correction and passes afterward in both
stances. A private native-pose replay also keeps both hands below the shoulders
across the balance range; that data remains outside source and packages. This
arm-only revision still requires native visual acceptance. Board-ground contact
is numerically aligned to the native Ground joint but remains visually unaccepted.


### Low stance and active balance correction

The owner rejected the previous steep-board, high-knee presentation. Those
captures establish gameplay continuity only; they are not visual acceptance.
The revised authored hold keeps the inverted board about 12 degrees from the
support plane, with both feet low and separated. Entry and flip preparation use
the same revised endpoint. Incoming Manual pitch is removed gradually, while
heading and native placement remain unchanged. A bounded authoring-only pelvis
fit keeps both low foot targets reachable on a tall, rear-weighted seed without
stretching bones or moving native rigid bodies.

A steady balance value no longer freezes the hold. Animation Studio's reusable
RangeMotion offsets the existing authored range with quiet weight shifts and
quicker corrections as the absolute native balance approaches either limit.
The clock advances once per admitted physics step, including at 30 and 60 Hz;
duplicate steps and client redraws cannot advance it. Range interpolation is
linear through its center so subtle motion remains visible. Entry and exit
interpolation stays eased. The native balance value, fatigue, controller mapping,
friction policy and collision/bail behavior are untouched. Exits join the actual
current animated pose on the same ownership path.

Focused checks cover low board inclination independent of incoming Manual
pitch, both low foot targets, bounded support settling, steady-balance motion,
stronger corrections at both limits, native-step clocking and cancellation.
Private replay on a captured rig checks reach but is not rendered contact proof.
New front/side/rear captures, both stances, balance extremes and exit continuity
must be reviewed in game before this revision is called ready. Owner-supplied
reference images and captured rig data stay outside the repository and packages.

### Refresh the native entry pose

Native review of the low-stance candidate found a timing defect at rolling
60 Hz entry: the initial Manual seed still contained the last pushing-foot
rotation, although the live foot was planted before the accepted gesture.
The runtime now keeps the existing native-pose observation awake while ready
and reauthors the queued entry and its exits once, immediately before acquiring
the controlled player. It retains the accepted gesture, native episode and
logical ownership; an active clip is never reseeded. Missing, mismatched or
stale native samples refuse the custom submission and preserve ordinary Manual.

The regression reproduces the early pushing seed followed by a planted foot,
checks observation renewal, unchanged active ownership and exit continuity, and
rejects absent, wrong-owner, expired, negative-age and nonfinite samples. Focused
sanitizer and six related integration suites pass. The exact-source Windows
build and repeated native rolling-entry visual check remain required.
