# Arcade Mode acceptance stack

Only notPho-Real/ReSkate is used. No upstream PRs. Main changes only after the
user explicitly accepts the feature on the Deck and authorizes its integration.
A green build is not in-game acceptance.

| Branch | Parent | Deliverable | Acceptance |
| --- | --- | --- | --- |
| arcade-mode | Existing baseline | Controller observer, model, Deck updater | Still needs Deck verification |
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
