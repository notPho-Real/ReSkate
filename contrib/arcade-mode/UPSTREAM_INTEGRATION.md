# Upstream integration candidate

This branch merges original upstream `9506a39e052e88a6cad2d50e624910f217e914a8`
into the gameplay-verified native flip source
`65d34a0bc06034125b28779d10490e479df37fba`. It is a build-validation candidate;
compiled tests do not establish native gameplay equivalence. No hardware deployment
is part of this integration.

The verified DLL is pinned by SHA-256
`0949252c47746e881ef98af78164674aeda5fb96f122cd596bdc63ef568f42a1`.
The integration worktree's ignored `artifacts/upstream-integration/rollback/`
contains its DLL, original CI archive, commit record, acceptance evidence and a Git
bundle of the source. The local tag `arcade-rollback-native-flips-65d34a0` also pins
the source. This is the rollback binary, not the integrated candidate.

## Merge decisions

There were 32 conflicted files: 28 content conflicts, one add/add instruction-file
conflict, and three modify/delete conflicts for the former built-in grind-exit
module. Each was reconciled with the replacement upstream APIs and lifecycle.

- Keep the native flip transition implementation, entry logic, and build-specific
  ABI/fingerprint contract unchanged from the verified source.
- Use upstream `skater_physics` for the former Hall of Meat physics reader, and
  `20260929/launches.h` for constants formerly in `grind_exit.h`. The removed
  built-in Hall of Meat, Road Rash, Board Break and grind-exit modules stay removed;
  their general upstream plugin support stays present.
- Keep Arcade's per-operation HID completion ownership and raw/returned XInput
  capture. Add upstream scripted-pad, preview-stick, Dark Pop and plugin filtering
  at the existing delivery points. A scripted pad retires Arcade's physical source
  and is never admitted as physical Arcade input. Use upstream's corrected padded
  Bluetooth report CRC implementation.
- Keep Arcade auto-push and air-motion integration beside upstream's plugin and
  Trainer physics functions, with the original Arcade ownership checks intact.
- Keep both spectator camera predicates: Arcade's requested-view predicate and
  upstream's requested-or-returning predicate. Retain camera write diagnostics
  around upstream's verified free-camera activation helper.
- Retain upstream's redesigned Trainer tuning implementation: typed-object
  discovery, seven-curve validation, output-only scaling, per-flight timing and
  replacement-skater refresh. The older fork four-curve scaler is superseded;
  this is separate from the unchanged native Ollie-to-flip bridge. Its historical
  helper and regression fixture remain available for review.
- Retain upstream startup, plugin initialization, data-build validation, launcher
  mod handling and diagnostics, alongside the fork's child-identity checks,
  redacted download output and Arcade initialization.

## Upstream pull request preparation

The comparison must use `Dingo-Shenanigans/ReSkate:main`. This integrated branch
is not a suitable single upstream PR: the initial comparison spans roughly 472
files and 270,806 added lines, including 163,530 lines of grind research results.

Upstream's current AGENTS.md requires general C++ SDK APIs and modes implemented
as sandboxed Luau plugins. Arcade remains a built-in C++ mode to preserve the
verified implementation during integration. Agree on the required general
native transition/input APIs and the plugin boundary with upstream before a PR.
Do not replace the verified bridge merely to tidy this development branch.

Prepare a separate branch from current upstream when acceptance is complete:

1. General native ABI, event/ownership, input and physics APIs, with independent
   regression tests and explicit permission boundaries for any plugin access.
2. Arcade mode logic, UI/preferences and only its necessary runtime wiring.
3. Focused regression fixtures and a concise build/gameplay evidence record.

Review support infrastructure separately: lab controllers, deployment/update
channel workflows, launcher recovery, rotating logs, camera diagnostics, fixture
geometry and source-survey tooling. Trainer tuning changes, broad research dumps,
historical session logs and unrelated world/traffic support should not silently
enlarge the Arcade PR. Move addresses still embedded in feature sources into the
upstream per-build headers before submission. Match upstream naming and style.

Keep GPL-3.0 source availability and the existing Belcross-Dev and davad00 source
attributions and commit pins. Audit reference fixtures and research JSON for
extracted EA data or decompiled source; upstream prohibits both. The rollback DLL
and source bundle are ignored local artifacts, not source-tree additions. New
third-party code must follow External/manifest.json and license requirements.

## Subsequent feature integration

Leave `arcade-stage-manual-episode-lifetime`, `arcade-stage-grind-transitions` and
`arcade-stage-flips-grabs` in their current worktrees. Do not reset or rebase them.
After this integration passes validation, take a fresh staging branch from the
updated Arcade head for one completed feature at a time. Compare each feature's
commits after the verified flip baseline, use `git cherry`/patch comparison to
detect already-integrated work, and review shared input, physics and runtime files
before a regular merge or explicitly selected finished commits. Test and accept
that feature before starting the next. Never cherry-pick the common baseline or
the upstream merge again. Preserve original feature branches and acceptance pins.

The primary local `arcade-mode` checkout was already dirty and at a different
local history from remote `arcade-mode`; it must not be reset to promote this
candidate. Reconcile its additional documentation commit separately and preserve
all uncommitted work. Remote Arcade promotion must be non-forced and conditional
on validation of the candidate being promoted.
