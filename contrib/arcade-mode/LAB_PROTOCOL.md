# Arcade Lab protocol

Repository memory is [STATE.md](STATE.md). Only `notPho-Real/ReSkate:arcade-mode`
may be developed or pushed. Results alone use `arcade-lab-results`. Nothing here
authorizes main/master, upstream, or an implicit deployment.

## CI and release

`.github/workflows/arcade-dev.yml` requires Windows x64 build/model tests, Linux
installer/lab simulations, and hosted macOS lab/bootstrap simulations before its
`deck` job may package or publish. The separate `arcade-deck.yml` supplies extra
Linux feedback; it is not the release gate. No job has Deck access or keys.
Stage branches may build artifacts, but only arcade-mode may publish a release.

`arcade-build-<40-character-source-sha>` is immutable. A retry reuses its original
installer rather than replacing it. Publication is serialized; the current
arcade-mode HEAD must equal the workflow SHA before the rolling pointer advances.
The immutable asset is published before `arcade-deck-latest/channel.json`.
The pointer identifies the pinned fork URL, commit and installer SHA-256;
the embedded manifest identifies the source commit and DLL SHA-256. The controller
checks all of these plus PE x64 DLL/PE32+ headers. Deck downloads need no GitHub
account or expiring Actions artifact. Hashes provide integrity within the trusted
fork/release channel; they are not independent signatures.

## Mac setup and operations

Prerequisites: Python 3.9+ (stdlib), macOS SSH tools, and an awake Deck with its
user SSH service already enabled. No sudo, OS read-only changes or automatic
service enablement. `gh` authenticated for this fork is optional for publication.

Double-click **Install Arcade Lab.command** in this directory. It runs the entire
offline Python suite before writing any permanent HOME files. A failed suite
stops installation. A successful setup copies the controller/tests/actions to
`~/.local/share/reskate-arcade-lab/controller`, creates a dedicated ed25519 key in
`~/.local/share/reskate-arcade-lab`, offers pairing, and runs read-only preflight.
It prints LAB READY only after preflight passes. If SSH or releases are unavailable,
the installed controller remains available but setup reports that blocker.
It never deploys a DLL. It does not install dependencies or GitHub credentials.

The `lab/` folder (also copied into the installation) contains these actions:

| Action | Effect |
| --- | --- |
| Pair Steam Deck | Creates the dedicated key if absent; adds only its public key to authorized_keys |
| Preflight Steam Deck | Verifies the tested release, reads Deck installation/process/space state |
| Deploy Latest Arcade Mode | Explicitly deploys the verified DLL after fresh preflight |
| Arcade Lab Status | Reads current Deck state; does not need release HTTP access |
| Collect Runtime Report | Reads only bounded ReSkate evidence, saves sanitized JSON, attempts publication |
| Restore Stock ReSkate | Restores the preserved stock DLL and previous updater preference |

Default target is `deck@steamdeck.local`. CLI equivalents:

```sh
python3 -B contrib/arcade-mode/lab/controller.py self-test
python3 -B contrib/arcade-mode/lab/controller.py preflight
python3 -B contrib/arcade-mode/lab/controller.py deploy
```

Every operation accepts `--host user@host` and `--game /exact/game/directory`.
Multiple or incomplete installations fail closed. Double-click actions accept
the same arguments when invoked from Terminal. Key material stays outside Git.
Pair uses SSH's own terminal password prompt and host fingerprint confirmation;
normal operations use strict known-host verification and BatchMode. Changed host
keys fail; resolve them manually only after verifying the Deck identity.
The old Pair-Steam-Deck action now pairs ONLY; its previous implicit deploy was removed.

## Read-only preflight

The controller sends `lab/probe.py` directly into remote Python memory with `-B`.
It never installs the probe, imports the installer, writes bytecode, creates a
backup/lock/config, edits settings, signals a process, adds a shortcut or launches
the game. Tests inspect its AST and execute the actual generated payload against
a fake Deck before/after filesystem comparison.

Output includes connectivity, user/host, discovered libraries, exact game path,
three required files, DLL hash, stock backup, receipt/config, installed commit,
updater setting, Skate/launcher process state, available/required space, latest
tested SHA and expected DLL hash. An unknown process check fails closed.
Success prints **PREFLIGHT PASS — NO GAME FILES CHANGED**. Paths/hostname are local
diagnostics and are never included in GitHub results.

## Explicit deployment and recovery

Only `deploy` replaces a development DLL. It validates release bytes before SSH,
runs read-only preflight, repeats it remotely, checks source commit/hash again,
then uses the shared `deck_installer.change_installation`. No downloaded Python
is executed by the lab. Close Skate and ReSkateLauncher and keep them closed
until completion; the game itself does not participate in the installer lock.

The shared installer refuses symlinks, incomplete installs and running processes.
A persistent regular `.arcade-mode-install.lock` uses nonblocking flock; do not
delete this lock file. Old directory locks fail closed and need manual inspection.
The first `ReSkate.dll.arcade-stock.bak` is published exclusively and never
overwritten. Initial stock provenance is an operator responsibility: hashes cannot
establish that an arbitrary pre-existing DLL is stock.

A fsynced `.arcade-mode-transaction.json` preserves prior DLL, settings and receipt
before replacement. Receipt records commit, installed hash, stock hash, original
updater preference, deployment time, unique installation ID and log baseline.
Writes use a same-directory temporary file and atomic replacement. Destination
hash is checked before transaction completion; failures restore previous state.
A killed process releases flock; the next explicit restore recovers the journal
before restoring stock. Read-only preflight detects a pending journal and refuses
deployment. If recovery itself fails (e.g. persistent I/O failure), preserve the
journal and repair the storage problem before retrying restore.

Development installation disables only `updates` in launcher settings. Restore
verifies the preserved backup hash, restores exact bytes and the saved preference,
retains unrelated current settings and the backup, and records restoration evidence.
`Skate.exe`, SteamOS, launch options and shortcuts.vdf are never modified.
The Mac lab installs no permanent Deck updater files or Steam shortcuts. Existing
offline installer/updater buttons remain supported and tested separately.
Legacy local shell helpers now share this transaction; supplied development DLLs
also require sibling `commit.txt` and `ReSkate.dll.sha256` from the CI artifact.

## Runtime evidence and acceptance

After explicit deployment, the user intentionally launches Skate. Tap and hold
each A/B/X/Y, release and tap again; verify native controls behave normally.
Run Collect Runtime Report while Skate is running. It asks about holds, native
controls and unexpected crashes, accepting pass/fail/unknown. These are explicitly
labeled operator reports. CLI: `--holds pass --native pass --no-crash pass` only
when actually observed. Unknown is never converted into a pass.

Actual logging is defined by Engine/Core/Log/paths.cpp, logger.cpp, crash.cpp,
Launcher/launch.cpp, and Runtime/runtime_state.cpp (`record` routes to logging::event).
Engine/Core/Log/events.cpp formats initialization as `Arcade mode initialized:
active=true`; JSON is emitted only at trace level. Arcade initialization explicitly
uses info level in Runtime/runtime_init.cpp so normal launches expose it. The
collector recognizes the actual formatted message and optional trace JSON.
The primary file is `<game>/logs/ReSkate.log`, truncated by the launcher at startup.
Fallback is Windows LocalAppData/ReSkate/logs or RESKATE_LOG_DIRECTORY; the lab does
not guess a Proton prefix. If the real installation uses a fallback, specify the
actual `--log /.../ReSkate.log` on deploy to baseline it, then on collect if needed.
A different log without a matching deployment baseline cannot establish acceptance.

Only a bounded log changed after deployment, with a changed startup prefix and
matching installed DLL hash, can contribute evidence. Only the latest ready
session contributes button events. The remote collector sends booleans/enums,
hashes and source SHA, never raw log lines, arbitrary paths, environment, keys,
addresses, or unrelated game information. Absence of a crash marker remains unknown
unless the operator confirms the session. Holds cannot be proven from sparse edge
logs. Process observation alone cannot prove runtime initialization.

Close Skate, run Restore Stock, then Collect again. The second collection retains
runtime evidence only for the same unique installation ID and adds exact stock
hash and updater restoration results. Complete observer_acceptance requires all
criteria; otherwise it remains UNVERIFIED or FAIL. This validates the observer,
not an ollie, flip, complete native state observer, or gameplay compatibility beyond
the actual exercised session. Reinstalling the same source starts a new evidence ID.

## Result publication

Reports first save to `~/.local/share/reskate-arcade-lab/results/<source-sha>.json`.
Only an explicit collection attempts publication (use `--no-publish` for local only).
The GitHub Git Data API creates an orphan `arcade-lab-results` branch containing
sanitized reports; subsequent commits preserve existing results and update
`results/<source-sha>.json`. GitHub history retains prior runs of the same build.
All endpoints are pinned to notPho-Real/ReSkate. Reference updates use force=false;
concurrent publication conflicts fail rather than force-pushing. Neither arcade-mode
nor main is touched, and no local Git checkout is switched by collection.

No credentials or publication failure affects the Deck installation. The complete
sanitized JSON remains local; retry Collect when gh authentication/network access
is ready. Publication is a separate blocker from hardware evidence completeness.

## Bounded autonomous input experiments

See [lab/ROBOT.md](lab/ROBOT.md) for the optional virtual gamepad and periodic
Gamescope screenshots. `robot.py preflight` is read-only; `robot.py run` explicitly
sends input to an already running game. Neither deploys. Rootless uinput access,
installed capture support and Steam controller routing require hardware checks.
Screenshots stay private locally; only separately sanitized runtime reports may
be published. The existing observer acceptance and deploy/restore rules remain.
