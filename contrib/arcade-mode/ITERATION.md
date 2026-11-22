# RC5 iteration workflow

Make useful progress offline before building or using the Deck. A full DLL build
and a hardware session are escalation steps, not the default feedback loop.

1. Recover the existing branch, dirty work, source identities, test receipts and
   captures. Keep previous progress and rollback artifacts.
2. State the defect and the next unresolved question. Inspect the owning source
   and retained evidence first. Do not repeat an investigation already recorded.
3. Use existing binaries, offline capture replay, Python checks and asset inspection.
   Reuse passing results while their relevant source, inputs and toolchain remain
   unchanged. Re-run only affected checks after an edit.
4. If a changed algorithm cannot be exercised otherwise, compile only its focused
   test target. Keep the existing build directory and dependency cache. Batch
   related fixes before considering a runtime build.
5. Use the current DLL for compatible animation, UI asset and configuration
   experiments. Record the DLL and asset identities separately. Do not describe
   this combination as an exact-source integrated build.
6. Build a DLL only when the next necessary experiment requires changed compiled
   runtime code and no available binary or offline test can answer it. Record
   that reason and the completed focused checks before starting one consolidated
   build. Do not dispatch CI just to poll progress or validate documentation.
7. Use the Deck only for a specific unresolved native behavior, timing, contact,
   rendering or input question that offline work cannot answer. Prepare inputs,
   acceptance evidence, negative controls and stop conditions before taking the
   exclusive slot. Use a bounded trial, inspect the result, and return offline
   when a failure exposes a source or data defect.
8. Complete required integration and exact-candidate native acceptance before
   declaring gameplay ready for owner testing. Fewer hardware sessions does not
   waive final HIL. Preserve the existing resource lock, focus/map checks and
   exact cleanup/rollback procedure.

## Before either expensive step

Record four short items in the current SSD task receipt:

- The unresolved question.
- Why existing evidence and cheaper checks cannot answer it.
- The smallest build target or native trial that can answer it.
- The result that determines the next action.

This is an engineering decision, not another owner approval checkpoint.
The owner permits Deck use until they say they are picking it up. Do not ask
for permission again; retain the shared hardware lock and rollback checks.
Use it only after no useful feature implementation can proceed offline, rather
than treating a native acceptance gap in one feature as a blocker for all work.
Do not add parallel playback systems, repeated full test runs or new process
frameworks when existing tools cover the need.

## UI asset feedback without rebuilding the runtime

Use the existing authoring tools to regenerate only the private changed asset.
Check instance identity, field preservation, bindings and native metadata before
launching. A successful writer/reader round trip alone is insufficient: both may
share the same mistaken interpretation.

The independent metadata check needs only Python and existing files:

```sh
python3 -B contrib/arcade-mode/compare_ebx_metadata.py original.ebx roundtrip.ebx
python3 -B contrib/arcade-mode/compare_ebx_metadata.py original.ebx edited.ebx --allow-non-enum-array-edits
python3 -B contrib/arcade-mode/test_ebx_metadata.py
```

The default compares all array and boxed records by resolved type identity,
including duplicate multiplicity, while permitting relocation and type-table
renumbering. The explicit graph-edit option checks enum arrays and boxed records
only; separately validate every intended graph/field change. Neither command
proves full asset validity or rendering. The report deliberately says so.

Keep extracted inputs, regenerated private assets and captures on the external
workspace, outside tracked source and distributable packages. Use the workspace
launcher for local commands so temporary output also stays on the SSD.

In a native UI trial, distinguish startup, menu rendering, a valid loaded level,
a local skater and actual gameplay. Inspect the current level before sending
gameplay inputs. A screenshot of the map is not a loading or scoring pass.
