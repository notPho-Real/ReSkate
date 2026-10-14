# Combo event evidence

Status: discovery instrumentation; runtime balance integration is NOT complete.

| Event | Existing evidence | Safe interpretation |
|---|---|---|
| Riding baseline | Real Deck state100 / crouch mode1 | On-board baseline, not every grounded context |
| Air | Trainer uses200–299; measured Deck ollie200→201 | Air state family |
| Landing | Deck ollie201→100 | Candidate landing transition; ownership gaps invalidate it |
| Grind | Build20260929/grind_exit.h measured400–499, especially401 | Upstream measured; reproduce on this Deck |
| Bail | no_bail selector300 and wipeout counter | Use actual wipeout, not requested bail |
| Manual / nose manual | Real Deck trials3/6 confirmed by user; trial3 native HUD says Manual; selector remains100/mode1 | Both DISCOVERED; numeric identity still unverified, do not infer from stick position |
| Revert | No verified identity | Do not infer from button/trigger position |

`ARCADE COMBO SAMPLE` adds a read-only listener through the existing multi-listener
physics-step API, preserving grind-exit and Hall of Meat ownership. It reads only
the already signature-guarded state/mode/eight request words. Queue64, drain16
per client tick; `ARCADE COMBO GAP` explicitly reports dropped samples. It does
not invoke a native function, force a selector, award points, or drive a HUD.
Reads are not transactional. Numeric changes are evidence, not named events.
Owner loss/menu/respawn and any queue gap must invalidate an inferred sequence.

Correction from real six-trial batch: +20000 Linux right-Y produced manual on
trial3; -20000 produced nose manual on trial6 (user watching actual Deck).
Private trial002/1.png also clearly displays native "Manual" scoring text.
State100/mode1 persisted, so that coarse selector cannot distinguish manuals
from ordinary riding. These are successful discoveries, not failed gestures.
Three-second holds, neutral before/after; repeatability and numeric event
identity remain unverified. Do not discard these successes or require speed
as a prerequisite without evidence. Fix observation and correlate repetitions.

Next bounded experiments: compare neutral riding against half-magnitude positive
and negative right-Y holds, then inspect actual manual animation/HUD plus state
transitions; repeat only identified candidates. For revert, compare an ordinary
landing with the game's documented native revert action and inspect physics-step
changes. No new Arcade revert button. Capture private screenshots, retain only
sanitized result tables in the repository. Native balance failure actuation is
UNVERIFIED; do not present combo loss as a physical bail.

Real Deck guide inspection (2026-10-13, old f669926 still installed): Skatepedia
Riding entry18 describes a small downward RIGHT-stick hold for a tail manual;
entry16 is FS Revert, with a LEFT-stick curved gesture and180-degree stance
change. This is documentation of native intent, NOT observed successful manual
or revert state evidence. Private captures: artifacts/combo-guide-riding-18 and
artifacts/combo-guide-fs-revert. No new revert button is warranted. Native X is
back-foot push, also confirmed in that guide; experimental Arcade X overrides it
only where its gesture eligibility/cancellation permits.
Repeatable guide navigation: Escape (Hub), Tab (side menu), Down twice, Return
(Skatepedia), Riding tab; Down18 for Manual, Up twice for FS Revert. Use350–500ms
between keys. The existing virtual Start recipe did not open Hub on this trial;
targeted Escape on the verified Skate window did. No settings were modified.
Additional private guide captures confirm nose manual uses slight upward RIGHT
stick; manual revert uses a quick LEFT-stick rotation while holding the manual.
BS Revert mirrors the FS curved-stick icon. Paths: combo-guide-nose-manual,
combo-guide-manual-revert, combo-guide-bs-revert. First manual discovery matrix:
RIGHT-Y Linux magnitudes12000/16000/20000 with both signs, hold3s, neutral before
and after, six candidates; then refine only candidates with animation AND native
state evidence. Revert matrix follows measured manual identities, using left-stick
arcs rather than inventing a face-button mapping.

## Pure model contract

Further real batch `trick-batch-20261013T210055-33354`: two neutral controls,
two +20000 holds, two -20000 holds. Six controller neutralizations and reset
checks passed. Native HUD confirmed Manual in trial003/1.png and Nose Manual
in trial005/2.png. Word1 at context+0x13c8, mask0xc000, changed from idle0xc000
to tail0x4000 or nose0x8000 and back on release. Air200/201 has0; crouch103 can
transiently have0x4000. Hypothesis: front/rear wheel contacts, not command intent.
This needs negative controls and repeated state-qualified correlation before use.
Quiet log baselines caused the first tail trace to be omitted by the timestamp
watermark; screenshots remain valid. A byte-boundary collector fix is locally
tested, not yet CI approved. Do not count missing trace as missing trick.

`trick(points)` consumes pending links and ends balance. `landing()` opens450ms
grace once. `revert()` succeeds once during landing grace, opening a single450ms
pending window. `begin_balance(kind)` consumes it; duplicate continuous entries
are idempotent. `end_balance()` only ends balance: the observation layer must
report landing separately. Grace expiry banks into `take_banked_points()`;
explicit `bank()` returns points directly. `bail()` never awards active points.

`State::fatigue` is global combo fatigue (existing field preserved); grind and
manual fatigue are independent and persist across mode exits. All are0..1.
Bank/bail reset all three.240Hz fixed integration carries fractional time between
ticks; invalid dt is ignored and individual calls clamp to1s. Correction is
signed[-1,1]. No raw controller/game pointer enters the model. Seeded phase,
low-frequency summed sine, inertia, reduced damping/control and edge instability
are deterministic. The default curve is simulation-tested, NOT Deck-tuned.

## Measured simulation envelope

`Extension/ArcadeMode/Test/balance_envelope.cpp` emits the27 runs in
`balance-envelope.json`: ages0/10/30s ×30/60/120Hz × neutral, wrong-way hold,
simple proportional/velocity feedback. At60Hz neutral fails at5.75/3.33/2.57s;
wrong-way hold at1.57/1.43/1.28s. Feedback delays failure to16.6/13.83/14.6s.
The non-monotonic last pair reflects disturbance phase, not lower difficulty;
initial effective difficulty is0.078/0.598/0.65. Failure timing varies by at most
one30Hz input frame across sampled rates. These are deterministic simulations,
not human ability, native rail physics, or proof of acceptable Deck feel.

Reproduce from repository root:
`c++ -std=c++20 -I. Extension/ArcadeMode/Test/balance_envelope.cpp -o /tmp/arcade_balance_envelope`
then `/tmp/arcade_balance_envelope`.
