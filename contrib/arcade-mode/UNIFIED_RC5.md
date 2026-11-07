# Unified RC5 — experimental pre-wallplant candidate

The authoritative source checkpoint is `unified-sources.json`. Each runtime and
Deck artifact carries `candidate.json`, generated from that checkpoint and the
same run's source SHA, DLL hash, gameplay fingerprint and compiled feature flags.
This branch does not publish to the accepted Arcade update channel.

## Integration decisions

- Start from RC3 `cd106244`; merge actual official upstream `95b0a436`, not fork main.
  The live upstream head still matched the requested pin. Existing Arcade native
  address headers and fingerprint checks remain pinned to game build 20260929.
- Preserve RC3's newer held-flip Prepare-window guard while deriving native IDs
  from Trickpedia. The canonical registry remains the only trick binding source.
- Upstream's hook and patch services are retained. Remove its obsolete unused
  single-observer and asynchronous world-ray interfaces. Keep Arcade's separate
  synchronous park-surface query and balance-bail request, with their existing
  owner/thread/lifetime checks. Retain the supported multi-listener physics API.
- Preserve RC3 snapshot/retry crash diagnostics alongside upstream's cached
  parent directory and native mod-list fixture. The fixture now expects its real
  native test mod; do not preserve the obsolete blanket zero-mod assertion.
- Cargo always checks its fingerprints and lockfile. Retain the optional-asset
  regeneration helper, plus upstream source dependency enumeration.
- Move Trickpedia and RC4 test targets into the optimized reusable CI graph;
  retain independent Windows preflight, native-menu lifetime, full upstream,
  installer/updater, provenance, security and release-eligibility gates.
- Stage RC4 at its exact pin with `DINGOSDK_ARCADE_WALLPLANTS=OFF`. The native
  contact and animation files are excluded from the runtime target. Its disabled
  runtime has no native dependencies, produces no input permissions or receipts,
  and cannot issue a body transaction. RC4 policy tests still exercise the staged
  implementation. The gate is part of the gameplay fingerprint.
- Enable upstream development tools in this experimental CI build so existing
  observation and regression tooling is retained. Release acceptance is unchanged.

## Evidence and acceptance

RC3 source reports retain native observations of early flips, repeated-X double
rotations, Manual/Nose Manual entry and contact, native revert actors, grab and
fingerflip actors, and selected one-footer Into/Loop/Exit sequences. They do not
establish blanket acceptance of all contexts, scoring, timing or stance cases.
See `research/rc3-gameplay-status.md` and the canonical Trickpedia evidence.

Trickpedia contains 136 canonical entries at this checkpoint, with no entry
claiming full native verification. Native replacement, controller search/Back and
same-session OFF restoration still require the exclusive hardware walkthrough.
The source fingerprint changes on integration; historical evidence is retained
without certifying changed code. A passing build is not gameplay acceptance.

RC4 has native contact evidence but no accepted rebound. Its owner retains the
mechanic. No Deck access, installation or controller input is part of this build.

## Install and rollback

Use the single `Install-ReSkate-Arcade.desktop` from the same-run Deck artifact.
First verify its accompanying `candidate.json`, source commit and DLL SHA-256
against the Windows runtime artifact. This is an experimental test candidate.
Close both game and launcher. Preserve the current DLL, launcher settings and
installation receipt, then run the existing transactional installer. Preserve all
stock, RC2 and RC3 rollback directories. Follow its existing installer prompts.

To roll back, close both processes and restore the preserved DLL, settings and
receipt byte-for-byte. Verify the restored DLL's recorded hash before launching.
The public updater remains on its existing channel; do not promote this branch.

## Next deliberate checkpoint

After the RC4 owner proves native contact and outward/upward rebound, compare that
tested source against `343518f0`; merge only its verified delta and supporting
changes into this same branch. Retain the registry controls and supported upstream
hooks. Update the source checkpoint and compiled-feature manifest policy, enable
the CMake option, rerun affected gameplay/Trickpedia tests and every integration
gate, then package a new exact-source candidate. Preserve this pre-RC4 package as
the immediate rollback. No scheduled or uncontrolled merging is authorized.

Every checkpoint must record new commits, conflicts, affected features, tests and
superseded artifacts. CI supplies exact run evidence; native acceptance requires
an explicitly coordinated exclusive slot on Mega Park with loaded-DLL identity,
native cadence, independent motion/actor evidence and OFF/negative controls.
