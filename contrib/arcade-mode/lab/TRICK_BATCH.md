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
at most 128 numeric native-state rows and 32 button rows from a 64 KiB log tail.
No arbitrary log text is retained. Expected states are an ordered subsequence;
250 ms sampling can miss short transitions. Named trick identity stays UNVERIFIED
until stronger evidence is analyzed. State-sequence PASS is not trick mastery.
Screenshots are never automatically published.

The maximum batch output is 96 frames / 768 MiB plus compact JSON. Each remote
request times out at 55 seconds and each virtual controller has a 25-second
alarm. The existing remote device lock prevents overlapping controller use.
Exhaustion, including all-skipped batches, is reported explicitly; it is not a
hardware acceptance PASS. Interrupting the client saves its partial summary;
the remote alarm/finally bounds cleanup, but an unconfirmed ACK remains a stop.
