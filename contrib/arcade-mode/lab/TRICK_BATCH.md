# Bounded unattended trick batches

Run from this checkout:

```sh
python3 -B /Users/andrewfarrell/reskate-arcade-mode/contrib/arcade-mode/lab/trick_batch.py contrib/arcade-mode/research/batch-example.json
```

The example is pinned to CI-tested heartbeat build
`e63dbe15623ef18cb1cd2bfe76a85b9f8c64ef60` and the preserved stock hash.
That exact build must be installed before this example can run. Future builds
require updating the source/DLL pins from verified release metadata. The runner
refuses a different installation rather than implicitly deploying anything.
`--validate-only` checks the complete data schema without accessing the Deck.
The runner never installs a DLL. Use a CI-tested build with the `ARCADE HIL
STATE` heartbeat, launch Skate and return to ordinary on-board skating first.
Older builds without that observable produce SKIPPED_CONTEXT, never blind input.

Each phase is a full controller state; omitted axes/buttons are neutral. `ms`
is relative to that trial's replay start after controller readiness. Stick axes range -32768..32767,
triggers 0..255, dpad -1..1; Linux Y is down-positive. Buttons are A/B/X/Y/LB/RB.
Labels contain only lowercase letters, digits and hyphens. Unknown fields,
commands, host overrides and code are rejected. Up to 24 total trials, 64 phases
per candidate, 12 seconds per gesture, 10 seconds per reset, 20 repeats per case.
Optional `capture_frames` is either `4` (default, evenly spaced) or `1`
(full-size end-of-trial image). Native-ID repeatability batches can use `1` to
reduce transfer volume; use `4` when visual transitions are part of the evidence.
The same sanitized native observations and reset checks run in either mode.

The timed writer no longer performs a full process-list scan per phase. The same
read-only readiness inspection runs on a separate thread, with a20ms pause between
checks. Input stops if evidence is older than100ms, the capture owner differs,
inspection fails, capture fails or the existing session deadline expires. The
observer is stopped after neutralization; a blocked shutdown aborts reuse.
Bounded `delivery_timing` rows report scheduled/write times, readiness-check cost
and write/observation cost. These measure the uinput boundary, not game polling.

Before each trial: exact source/DLL/stock hashes, active receipt, updater off,
safe paths and game/HIL readiness must match. Three fresh on-board state=100,
mode=1 samples gate input. This is a conservative skating-context heuristic,
not proof of a specific location or speed. Invalid context consumes a candidate
as SKIPPED_CONTEXT and sends no input. Reset waits for the same observable;
it does not teleport, guess menu navigation or blindly respawn the skater.

The trusted batch dispatch now uses one `ExperimentSession`: a single locked
virtual controller, kernel/SDL observers and capture preflight survive all trials.
Hotplug settling occurs once. Each trial neutralizes before screenshot completion
and reset waiting; observer samples are cleared and copied per trial. Ordinary
recognition or reset failures remain recorded results and the next baseline is
checked before proceeding. Infrastructure failure or interruption closes the
session immediately. The existing single-run wrapper still destroys its device
at the end of its one trial.

The session admits at most 24 trials and has a 1,100-second total watchdog,
including neutral idle time. Each input trial retains its 25-second watchdog.
The existing 1,200-second outer transport timeout and all source/DLL/game/stock
identity checks remain unchanged. This repository implementation must enter the
externally reviewed frozen bundle before hardware use; editing the checkout does
not change an installed trusted runner. No external bundle is updated by these tests.

A lost acknowledgement is never replayed: input may have happened.
Read-only SSH preflight retries three times. Missing game/HIL readiness gets a
bounded recovery wait; this version does not automatically click the launcher.
Unrecoverable readiness, hash/backup/path integrity or unconfirmed neutralization
stops the batch. Wrong tricks and missing expected state sequences continue.
No deploy, stock restore or publication is called between candidates or at exit.

Evidence is private under `artifacts/trick-batch-*/`: checkpointed summary,
exact plan, at most four verified screenshots per input trial (8 MiB each),
at most 128 numeric native-state rows and 32 button rows from a 256 KiB log window.
Trials are delimited by an exact byte boundary; truncation or overflow is explicit
incomplete evidence, never silently graded as an exact native success.
No arbitrary log text is retained. Expected states are an ordered subsequence;
250 ms sampling can miss short transitions. Named trick identity stays UNVERIFIED
until stronger evidence is analyzed. State-sequence PASS is not trick mastery.
Screenshots are never automatically published.

The maximum batch output is 96 frames / 768 MiB plus compact JSON; end-only
capture reduces this to 24 frames. Trusted persistent batches use the 1,200-second
transport timeout and per-trial watchdog described above. The existing remote
device lock prevents overlapping controller use.
Exhaustion, including all-skipped batches, is reported explicitly; it is not a
hardware acceptance PASS. Interrupting the client saves its partial summary;
the remote alarm/finally bounds cleanup, but an unconfirmed ACK remains a stop.

For a saved native rail fixture, `fixture_reset: true` requests the existing
native teleport, remounts once if the measured state is 504, and requires a fresh
stationary riding status at the saved position and heading. Native controls use Y;
Arcade Mode uses the established shoulder chord. The same controller remains
connected. The next trial first rechecks fresh status and skips another teleport
when the preceding reset is still valid. `capture_frames: 0` retains input
readiness checks and numeric evidence.
Missing grind telemetry or an unverified reset stops this fixture batch.

`python3 contrib/arcade-mode/lab/grind_report.py BATCH_1.json BATCH_2.json`
summarizes the sanitized external results without sending input. Supply the four
acceptance batches for the 10/20/10/10 denominator; keep setup pilots separate.
Native 400–499 samples establish grind activity. A miss versus contact without
grinding remains unverified without an independent contact observation. Raw enums
have no invented names. Bail, exit, neutralization and reset outcomes remain
separate, and a failed post-trial reset does not hide an otherwise valid attempt.
