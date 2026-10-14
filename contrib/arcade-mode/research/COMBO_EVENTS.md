# Combo event evidence

Status: discovery instrumentation; runtime balance integration is NOT complete.

| Event | Existing evidence | Safe interpretation |
|---|---|---|
| Riding baseline | Real Deck state100 / crouch mode1 | On-board baseline, not every grounded context |
| Air | Trainer uses200–299; measured Deck ollie200→201 | Air state family |
| Landing | Deck ollie201→100 | Candidate landing transition; ownership gaps invalidate it |
| Grind | Build20260929/grind_exit.h measured400–499, especially401 | Upstream measured; reproduce on this Deck |
| Bail | no_bail selector300 and wipeout counter | Use actual wipeout, not requested bail |
| Manual / nose manual | No verified identity | Do not infer from stick position |
| Revert | No verified identity | Do not infer from button/trigger position |

`ARCADE COMBO SAMPLE` adds a read-only listener through the existing multi-listener
physics-step API, preserving grind-exit and Hall of Meat ownership. It reads only
the already signature-guarded state/mode/eight request words. Queue64, drain16
per client tick; `ARCADE COMBO GAP` explicitly reports dropped samples. It does
not invoke a native function, force a selector, award points, or drive a HUD.
Reads are not transactional. Numeric changes are evidence, not named events.
Owner loss/menu/respawn and any queue gap must invalidate an inferred sequence.

Next bounded experiments: compare neutral riding against half-magnitude positive
and negative right-Y holds, then inspect actual manual animation/HUD plus state
transitions; repeat only identified candidates. For revert, compare an ordinary
landing with the game's documented native revert action and inspect physics-step
changes. No new Arcade revert button. Capture private screenshots, retain only
sanitized result tables in the repository. Native balance failure actuation is
UNVERIFIED; do not present combo loss as a physical bail.

## Pure model contract

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
