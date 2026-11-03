# Unified Arcade release candidate 2

The owner approved advancing RC1 to upstream integration after reporting working
manual reverts and Ollies out of grinds. This is an RC2 build candidate; its
gameplay acceptance remains a separate owner gate.

## Frozen inputs

- Accepted RC1: `1b92abbeb7e6e2cb9d31131268fb255e03758402`.
- RC1 DLL SHA-256: `d572266c704854f7a7a455d87b4b3840ca12e3b73f263be3ed74d8197d0aecbc`.
- Local annotated checkpoint: `arcade-approved-rc1-01-1b92abb`.
- Original upstream main, fetched for this integration:
  `919b04e9a5aa708974b8ea7298c61333104d3905`.
- Dedicated candidate branch: `arcade-stage-unified-rc2-upstream`.

The merge starts from accepted RC1. The older upstream integration is used only
as a conflict-resolution reference; it is not the candidate's parent or accepted
build baseline. RC1 and all feature branch histories remain unchanged.
The initial upstream merge at `da508eb` is followed by the eight upstream
corrections through `919b04e`, including build-header and test-target repairs.

## Compatibility decisions

- Keep RC1's native flip bridge, animation fingerprints, launch implementation,
  manual episode state machine, manual-revert handoff and rail-pop implementation.
- Adapt the physics reader to upstream's `skater_physics` API and the renamed
  per-build `launches` constants. Retired built-in grind-exit and Hall of Meat
  modules remain removed; their upstream replacements are retained.
- Preserve upstream plugins, placement physics, controller replacement APIs,
  visual cadence, tuning, launcher and multiplayer changes.
- A plugin-replaced XInput packet reaches the game through upstream's filters
  and retires Arcade physical ownership. It cannot become an Arcade face/stick
  gesture or receive an Arcade actuation receipt. Menu capture expires plugin
  replacement leases. Scripted character-room input remains a separate owner.
- Keep correlated HID completion ownership and upstream full-report replacement,
  including Bluetooth CRC handling, rather than selecting either entire input file.
- Keep upstream Trainer/plugin gameplay-setting overrides alongside the bounded
  Arcade grind expression override. The override does not persist a user setting.
- Keep upstream teleport equipment preferences alongside Arcade's independent
  native balance-failure request and existing fixture teleport overload.

No grab/grind catalog expansion, replacement animation or new manual behavior is
part of this merge. RC1's documented partial grab and grind limitations remain.

## Validation and acceptance

`arcade-dev` retains the required production-composed Windows, Linux, macOS and
packaging gates. `arcade-fast` retains the twelve focused native regressions.
`arcade-integration` builds all configured upstream Windows targets and tests,
plus the Linux server. Its upstream test build does not enable the legacy broad
Arcade test group: the required Arcade composition suite runs in the other two
workflows. Historical non-gating Arcade differential failures are not relabeled
as passes. The upstream `trainer_tuning_names` asset test requires a separately
supplied game installation and is explicitly excluded on hosted CI.

Local and hosted results, exact candidate source and binary hashes, download
locations and skipped checks belong in the staged RC2 manifest after completion.
Do not install, advance `arcade-mode`, or mark RC2 accepted merely because CI is
green. Preserve RC1 as rollback until the owner accepts this exact candidate.
