# Arcade Mode acceptance stack

Only notPho-Real/ReSkate is used. No upstream PRs. Main changes only after the
user explicitly accepts the feature on the Deck and authorizes its integration.
A green build is not in-game acceptance.

## Current work (2026-10-13)

The table below is historical. Work continues only on arcade-mode. Native A
ollie has prior Deck evidence; X kickflip/heelflip remain experimental and their
repeatability/integration must not be inferred from discovery. Current task is
combo linking and balance, not new flip discovery. Pure lifecycle, one-use revert
grace, persistent three-part fatigue and inertial dynamics are implemented and
simulation-tested. Native manual/revert events, gameplay-driven meters and native
balance failure remain unverified. See contrib/arcade-mode/STATE.md and
contrib/arcade-mode/research/COMBO_EVENTS.md for current evidence and exact next
action. Keep the active development DLL installed between experiments.

| Branch | Parent | Deliverable | Acceptance |
| --- | --- | --- | --- |
| arcade-mode | Existing baseline | Controller observer, model, Deck updater | f09872f observer: 13/13 hardware criteria PASS; native ollie remains unimplemented |
| arcade-stage-01-builds | arcade-mode | Isolated CI for stage branches | Windows/model/package CI |
| arcade-stage-02-state-probe | arcade-stage-01-builds | Read-only state and button correlation | Observe normal riding, jumps and bails on Deck |

Stage branches build Windows DLLs and complete Deck packages, downloadable from
that commit's Actions run. They cannot publish releases or advance the normal
Deck update channel. Artifact names include the full commit SHA. Packaged stage
installers retain the normal updater channel; selecting a stage for the Gaming
Mode updater is not implemented yet. Do not describe artifact availability as
one-button stage selection.

Keep each successor based on its predecessor. After acceptance, merge the
accepted branch preserving ancestry; do not squash a parent out from under its
successors. Rebuild the next branch against any fixes before presenting it for
acceptance. Do not silently merge or promote a branch on a successful CI run.
Record the tested commit and user's acceptance here when those exist.

## Remaining feature gates

1. Controller notices and Gaming Mode Update/Restore library entries work on Deck.
2. A calls an identified, signature-validated native ollie path. Existing
   trainer_jump.cpp adjusts launches already happening; it is not an ollie API.
3. X calls one verified native flip; correct ownership and game-build guards.
4. Ground/air/grind/manual/landing/bail observation with invalidation across
   respawn, loading and loss of local ownership. Raw selector samples can miss
   transitions and must not be treated as a complete gameplay event stream.
5. Combo scoring, landing grace, revert/manual linking and balance. The pure
   model is tested, but runtime event wiring and native balance failure are not.
6. Host-authoritative attack/knockdown protocol and verified native bail, with
   rejection of stale, duplicate and unauthorized requests.

Unverified native calls are not implemented by guessing addresses or forcing
selector state IDs. Later feature branches are created when there is a concrete,
reviewable implementation; empty branches are not ready features.

## State-probe evidence

The probe uses no_bail.h's existing watched_physics_state() reader. Its source
has a 500 ms ownership lease, refreshed by existing trainer observation; the
probe does not add hooks or take ownership of that watcher. A valid watch with
state zero is not yet an observed selector state. Individual fields are atomic
but the snapshot is not transactional. Use raw values for diagnostics only.
The build-specific no_bail header identifies wipeout 300; trainer.cpp identifies
air states 200–299. Neither establishes a native ollie command or manual identity.

## Probe acceptance on Deck

Run the existing observer checks first: tap and hold each face button, then
release and press again. Notices must remain one per press. Ride, perform a
normal native jump using the existing controls, land, bail, respawn and return
to menus. The ReSkate log should include `ARCADE MODE STATE` when the observed
snapshot changes and `ARCADE MODE INPUT` beside each button notice. Watch loss
must clear state rather than retaining a last-known grounded/air value. No new
tricks, score, suppression or control remapping are expected in this stage.
This evidence is for identifying transitions before attempting native control;
it is not proof that a sampled state transition was caused by a face button.
