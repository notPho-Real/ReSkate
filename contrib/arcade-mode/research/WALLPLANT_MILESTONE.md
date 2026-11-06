# Arcade RC4: wallplant gameplay milestone

**Status:** EXPERIMENTAL IMPLEMENTATION, BUILD AND HARDWARE VALIDATION PENDING. This file remains the milestone contract for the isolated `arcade-stage-wallplant-rc4` branch; it is not gameplay/CI/hardware acceptance evidence. The guarded rebound and native-asset visual approximation are described in `rc4-wallplant-implementation.md`. Do not promote this branch or communicate a finished feature before a testable build and native acceptance evidence exist.

## Requested gameplay

A **wallplant** is a brief planted-foot contact with an essentially vertical surface while airborne, followed by an immediate outward/upward kick that preserves useful tangential speed and **does not break the existing combo**. It must be a controllable trick rather than an automatic bounce on every wall collision. It is **not** a wallride, ordinary footplant from the ground, grab, grind, or wall collision with no animation.

Control intent: a **fresh A press while airborne and approaching a valid wall**. Ground A remains Ollie. Preserve other owners: X flips and repeat-X, B grabs, Y rails, RT revert, LB/RB spin, LS steering, RS camera, Manual/Nose Manual, board toggle and Arcade OFF. Do not start another Ollie on the wall A edge.

## Mandatory source-first investigation

Read `AGENTS.md`, `.codex/README.md`, `STRUCTURE.md`, and the existing `Extension/ArcadeMode` code, especially `arcade_air_motion.h`, `arcade_input_session.h`, `arcade_surface_geometry.h`, `arcade_footplant.h`, `arcade_native_footplant.h`, and `arcade_native_grab_transition.cpp`. Survey original build-20260929 Skate wall interaction, footplant, one-footer, collision, native motion/animation, and score/combo paths. Reuse identified native mechanics where possible. No guessed native addresses, fake state identities, synthetic scoring, unsolicited replacement animation assets or proprietary game assets in the repo. Keep game-build offsets isolated under `Engine/Game/Build/20260929/`.

### Required wallplant input and physics contract

1. Require original local-player ownership, fresh physical A press, eligible single airborne episode, valid collision contact against a near-vertical **wall** (not floor, sloped ramp, rail, ceiling or another skater), valid wall point/normal, and observed approach velocity **into** the wall. An arbitrary ray near a wall is insufficient proof of a physical contact. Native contact and world/entity generation must agree before and after execution.
2. Require a genuine preceding takeoff; capture original physics/contact/owner/timing. Only one action per A edge, with separation/cooldown requiring leaving the wall and a fresh approach. No stationary wall speed farming or infinite repeated boosts; opposite wall/concave corner and grazing contacts need explicit negative tests.
3. Use the original/guarded game movement mechanism for one bounded wall-normal rebound and modest upward component; preserve bounded tangential velocity, no teleport, geometry intersection, artificial landing, or frame-by-frame overwriting of subsequent native motion. Direction must follow the actual contacted normal, including oblique approach.
4. Find and play a real compatible native planted-foot/footplant animation or native pose through its authentic owner. If no exact pose is available, state the gap and distinguish a visual prototype from an authentic wallplant; do not claim valid actor initialization from mere requested input.
5. Retain one ongoing combo and expose one native-observed trick occurrence only after correct wall contact/impulse/animation and independently verified scoring provenance. A score string/label or input request is not proof. Chain into an immediate airborne flip and ground Manual without a forced extra landing.
6. Cancel on bail, owner/generation change, stale contact, incorrect motion, menu/focus loss, landing, offboard, Arcade OFF, missing asset, incompatible surface, or timeout. OFF leaves native controls/movement untouched; no side effects outside Arcade.

### No Comply must be actually understood and repaired

The No Comply path in `contrib/arcade-mode/research/native-footplant-entry.md` is **not accepted**. The earlier missing-prerequisite failure was resolved in RC3: the original mongo full-push animation produces `CanNoComply`, and subsequent hardware captures observed `Is.NoComply`, native entry, flight and return to ground without bail. RC4 retains that implementation. Complete the remaining original animation, unique launch, landing and scoring verification; do not restart the prerequisite investigation or treat simulated tests as native acceptance. Keep bounded stock Arcade-OFF positive controls versus Arcade ON on clear ground, and record original option readback, request vs source delivery, native intent/action, launch, return/landing and scoring separately.

Do not substitute a normal Ollie or force a No Comply boolean/animation/velocity. Existing controls stay: **Up→A within 300 ms** for No Comply, **Up→release→Up→A** for Boneless and plain A for Ollie. Preserve the demonstrated Boneless native launch. If stock control never executes in this pinned build, provide reproducible native-negative evidence and explain the exact capability barrier; a guess is not an accepted No Comply implementation.

### Game/hardware acceptance, not merely simulated checks

- Build exact-source Windows `ReSkate.dll` with `/W4 /WX`, targeted regressions, normal Arcade development/integration CI; record commit, SHA-256, manifest and non-expired artifact. Keep RC2 rollback and original installed DLL. Do not update `arcade-mode`, `main` or the public updater without owner acceptance.
- Use the **`skate3_megapark`** user-selected map on Deck, verify the live map, original build, installed/mapped DLL SHA and process identity. Serialize all HIL operations through the reviewed trusted runner and shared device lock; native physics/cached-step cadence must be measured (30/60/120 when actually supported). Current-world geometry must be freshly verified.
- At least 20 controlled successful wallplant attempts over regular/fakie, head-on/angled approaches and distinct wall materials/angles. Include separate no-A wall contact, slope/floor/ceiling/rail, wrong-way approach, stale contact, repeated press, post-landing, menu/OFF/owner-loss and corner negatives. Measure wall-normal rebound, bounded vertical velocity, actor initialization/advancement, cleanup/landing, combo continuity, and native score evidence independently.
- Demonstrate one complete input/control chain: **Ollie → Wallplant → Kickflip → Manual (or Nose Manual) → land/bank**, with continuous native combo ownership, no bail and no additional Ollie. Report real failed attempts, not just a successful selected trial. Existing Boneless, Manual/RT, early-X, grab, rail-pop and Arcade OFF behavior must not regress.
- No Comply: demonstrate a separate original native No Comply entry/launch/landing on Deck with its own actor/native intent, not just recognizer output. If definitively unsupported, document the proved source blocker and do not claim it works.
- **Only inform the owner when there is a genuine installable test candidate**: exact downloadable artifact, green exact-source gates, Deck test evidence, and instructions to switch back to the preserved baseline. Do not send research or status-only handoffs. Keep the milestone open if these conditions are unmet.

## Integration notes

Continue from the latest `arcade-stage-rc3-gameplay` while preserving ancestry. This isolated branch may be rebased/integrated explicitly; no silent force-update to an accepted branch. Detailed source and HIL evidence should be committed as original authored notes and bounded hashed receipts, not proprietary assets or raw captures. The current parent `2a3254a73f43b9950c39cae876027585e32331bf` is *not* a verified wallplant implementation.
