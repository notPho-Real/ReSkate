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
