# Unified RC5 — experimental Casper integration

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
  animation file remains excluded from the runtime target. The shared board-contact
  reader is compiled for Casper's independently fingerprinted, opt-in surface
  material observation; this does not enable the wallplant rig hook or rebound.
  The disabled wallplant runtime has no native dependencies, produces no input
  permissions or receipts, and cannot issue a body transaction. RC4 policy tests
  still exercise the staged implementation. The gate is part of the gameplay
  fingerprint.
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
mechanic. The combined candidate requires a separately coordinated exclusive Deck session.

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


## Casper integration checkpoint

The owner authorized this integration on November 12, 2026. The existing RC5
worktree was clean at `b12327c3`, and the Casper worktree was clean at `2b566b87`;
both matched their refreshed remote heads. RC5 is an ancestor of Casper, so the
integration fast-forwards all 20 existing commits without conflict resolution or
reimplementation. No later RC4, Trickpedia or upstream changes are included.

This checkpoint retains the authored entry, held balance corrections, horizontal
flip rotation, direct and Ollie exits, contact pose, shoulder rotation and
surface-dependent resistance. It also retains the Animation Studio authoring,
interpolation, stance, native-clock playback and cancellation changes. Casper
remains a development-only, unsaved opt-in (`arcadeflatland on`); ordinary Manual
and Arcade OFF behavior remain the defaults. Wallplants stay compiled OFF.

The prior exact Casper DLL and the prior RC5 package remain rollback artifacts.
A new candidate must carry this source manifest and its own CI run, source SHA,
DLL hash and installer identity. It supersedes neither historical evidence nor
the accepted update channel. The published candidate remains unaccepted.

### Regression scope

- Run the focused authoring/export, animation core, material selection, input,
  Manual lifetime, combo ownership and release-disabled tests with sanitizers.
- Run the existing targeted suite, including Trickpedia lifetime and the disabled
  wallplant link test, then all required Windows and cross-platform CI gates.
- On the exact combined DLL, verify Mega Park, source identity and native cadence
  before inputs. Exercise both stances through entry, moving hold, flip, direct
  and Ollie exits; include rotation held across exits and surface selection.
- Check ordinary flips, grabs, Manual/Nose Manual, revert and combo continuation,
  plus Trickpedia controller search/Back and same-session Arcade OFF restoration.
  Use native actors, state and ownership evidence alongside actual rendering.
- Exercise cancellation by focus/controller withdrawal, bail and menu transition,
  and confirm disabled wallplants issue no ownership or rebound.

The 19 native cases on the prior `2b566b87` DLL are historical supporting
evidence, not automatic certification of a newly built DLL. New CI and native
results belong in the exact-source artifact receipts. No combined native pass
is claimed by this document.

Owner visual approval is still pending. Slopes/edges and another body/footwear
configuration remain unverified. The displayed inverted board and hooked foot
use native Manual rigid-body support; this is not physical inverted-tail support
or a guarantee of mesh clearance for every outfit and frame. No custom native
scoring or trick-ID acceptance is claimed.

## Official upstream refresh

The owner requested the current official main on November 12, 2026. The source
pin advances from 95b0a436 to 355bf71e. The existing RC5 checkpoint 6de06957 and
all Casper authoring, playback, surface resistance and rotation changes remain
in history. RC4 stays disabled at its previous integration pin.

Resolve the new HUB front page and Trickpedia section collision with distinct
shared constants. Carry the Arcade switch and Trickpedia link into upstream's
new SKATER options, retaining native Back and level cleanup. Preserve the
bounded HID completion ownership while applying upstream plugin button masking
inside the admitted report handler; keep Steam-front masking before recording
the returned controller packet. Retain both park surface observation and the
new editor ground-grid request.

Upstream render-time pose replay is retained alongside the existing custom
clip and board publication path. Pose placement, PlayStation report, native
menu, score-line and news tests join the Windows preflight for this checkpoint.
The previous exact-source native results remain historical. In particular,
the prior Trickpedia failure is not declared fixed by a successful merge or
test build: the new menu requires an exclusive native walkthrough.

### Live native page discovery

The upstream refresh passed the full Windows compile and 128 integration tests.
Native regression testing then exposed an existing Trickpedia discovery defect:
the live screen registers a PageViewModel root directly, without the resource
wrapper the interception previously expected. The replacement now selects the
focused page whose original Back command is SP_Back. It retains the existing
typed copies, Back action and restoration path. A focused map page is an explicit
negative control; the production-body regression fails with the former lookup.

The revised page lookup still requires an exact-source Windows build and native
ON/OFF, search, Back and page-reload checks before candidate acceptance. Initial
merged-build Casper checks cover goofy entry, flip, direct return, Ollie and a
legitimate high-balance bail; they do not establish the remaining regression
matrix or complete foot-contact clearance.
