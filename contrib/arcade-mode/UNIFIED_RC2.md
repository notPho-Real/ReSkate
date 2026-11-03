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

The first full Windows run exposed two test compatibility defects: the Trainer
curve test passed a conditional integer expression to a float parameter under
`/WX`, and the upstream crash test still looked for fatal-handler evidence in
the ordinary log. The corrected tests use float literals and require the actual
handler record in Arcade's separate bounded fatal log. Native-handler negative
controls, dump capture, exception chaining and attachment checks remain enabled.
No runtime gameplay implementation changes are needed for these corrections.

The next run compiled every configured Windows target and passed 122 of 123
tests, exposing missing mod-folder metadata in native crash capture. The helper
previously queried the parent's executable path after process exit. It now keeps
that observed folder before signaling readiness, then uses it for all captures.
No unknown folder is fabricated. The existing real child-process crash tests
continue to require mod metadata, exception chaining and native dump provenance;
they run before the expensive full runtime build as well as in the full suite.

Local and hosted results, exact candidate source and binary hashes, download
locations and skipped checks belong in the staged RC2 manifest after completion.
Do not install, advance `arcade-mode`, or mark RC2 accepted merely because CI is
green. Preserve RC1 as rollback until the owner accepts this exact candidate.

## Early Ollie-to-manual correction

Owner feedback identified that Up–Down early in an Ollie worked only near landing.
The production-composed reproduction lost the original request at age726ms at
30Hz: both ManualActuation and GroundManualBalance applied the700ms initial
request deadline unless the final physical direction remained held. A neutral
stick could therefore retire valid airborne preparation before landing.

An admitted airborne request now retains its recognition and original native
flight identity after stick release. Fresh matching owner/flight observations
authorize continued pressure; the first verified landing starts the existing
bounded contact acquisition. Independent native delivery and contact checks
remain required. Unadmitted requests retain their original expiry. A brief
landing/rebound between input polls retains only the original700ms preparation
window; the new flight cannot inherit extended authority or renew that deadline.
Owner loss, stale producer, OFF, bail, rail, offboard and conflicting grab retire
the preparation without synthetic putdown or automatic re-acquisition.

Production-composed checks cover both directions, neutral-stick flights lasting
1,2.5 and5 seconds at30/60/120Hz, landing admission, HUD absence in air and presence
after contact, and the cancellation cases above. Existing ground holds and both
revert chains remain required. These are offline regressions; live acceptance of
the new early-input behavior remains pending.
