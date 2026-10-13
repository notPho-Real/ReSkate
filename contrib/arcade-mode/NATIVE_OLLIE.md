# Native ollie investigation

The bbd1def diagnostic build was tested and stock restored. Feature f669926
was then explicitly deployed and tested on the real Deck: an A-only tap created
Skate's visible native “Ollie,” and a three-second A hold produced one additional
native ollie/capture. Stock and updater preference were restored afterward.
The initial A milestone is hardware verified in this bounded setting.

## Real hardware input baseline and mapping

The autonomous virtual gamepad pressed A to arm a five-second native capture,
then performed the game's normal right-stick ollie on a board while on a clear
sidewalk. A fresh Gamescope frame displayed “Ollie.” Capture 4 was complete
(30 bounded samples) and showed these client-sampled state/mode transitions:
100/1 at 0 ms, 100/0 at 1634 ms, 103/0 at 1734 ms, 200/2 at 1834 ms,
201/2 at 1867 ms, and 100/1 at 2634 ms. The prior on-board A-only baseline
was contaminated by a traffic collision and is not proof of an A ollie.
The 13/13 sanitized observer/restore report is at
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/bbd1def2eaf42945d43f4e0708574852eee3b817.json .

`arcade_ollie.h` models a single bounded A-to-right-stick gesture. The existing
game XInput hook applies it only with a fresh local on-board idle sample,
preserves physical right-stick priority, suppresses the triggering A from the
game, and provides packet changes for the two synthetic stick phases. There
are no direct native game calls or state writes. Pure model tests exercise
timing, hold behavior, off-board passthrough, cancellation and clock rollback.
The real Deck confirmed Skate uses this hooked path for gameplay on this build.
The sanitized feature result is at
https://github.com/notPho-Real/ReSkate/blob/arcade-lab-results/results/f669926ab790d1328bc234a9828c290fb46ee276-ollie.json .

## Verified binary and evidence

The actual Deck executable was copied read-only to ignored
`artifacts/arcade-native-research/Skate.exe`: 144567168 bytes, SHA-256
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
This matches the existing `Engine/Game/Build/20260929` contracts. Neither the
binary nor full disassembly may be committed or published.

Static analysis facts (addresses below are RVAs):

| Location | Verified behavior | Interpretation limit |
| --- | --- | --- |
| `0x4745a70` | Existing selector reads context flags and current state | Not a command API |
| `0x4746e17` | Calls rig predicate `0x476f130`; true can return 103 at `0x4746e28` | Other selector branches and guards remain relevant |
| `0x476f130` | Reads rig `+0x59a8`, then absolute float `+0x4a04` against `+0x58c`; mode 2 returns true | A predicate, not an ollie request |
| `0x1009e29` | Gets rig through core vtable slot `+0xe8`, writes mode at rig `+0x59a8` | Producer is driven by several expression values; do not guess their meaning |
| `0x6077680` | Array of 20 profile option names includes `egf_quickollie`, `egf_jumpbuttontoollie`, `egf_ollieonrelease` | Names alone prove neither current enablement nor A mapping |
| `0x3f6040` / `0x579a36` | Returns that name array / caller passes it to generic options processing | Not a native trick invocation |

The runtime already hooks gameplay option getters in
`Extension/Settings/local_gameplay_settings.cpp`. An option override is a possible
future route, but there is not yet evidence that it produces the desired A ollie.
Do not change saved preferences or force these flags based only on their names.
`Extension/Trainer/trainer_jump.cpp` scales no-comply/boneless launches already
happening. It does not implement an ollie initiation path.

## Focused read-only diagnostic

`arcade_native_probe.h` contains two independent 32-byte signatures checked
against the real binary locally and checked again at runtime. If either differs,
the new probe is unavailable. Existing owner resolution in `no_bail_skater`
adds its existing contracts and ownership checks. No new hooks or native calls.

An A rising edge starts a five-second capture, capped at 128 changed samples.
Each sample contains eight context words at `+0x13c4..+0x13e0`, the context state
at `+0x1414`, and rig mode at `+0x59a8`. Ownership is rechecked after guarded reads;
unavailability clears the sample. Reads are not a transactional physics snapshot,
and client polling can miss transitions. Never use these samples as authoritative
ground/air events. No raw addresses, other log lines, or personal data are emitted.

Existing `ARCADE MODE DEV` notices retain their meaning. A still only observes
input and arms this diagnostic; normal controls continue. Logs use exact prefixes
`ARCADE NATIVE PROBE`, `ARCADE NATIVE CAPTURE`, `ARCADE NATIVE SAMPLE` in the actual
game `logs/ReSkate.log`. Each capture has an ID; sample milliseconds are relative
to its arm. A new A press starts a new capture ID. A capture ending at 128 samples
may be truncated; lack of a sample does not establish lack of a physics transition.

## Completed diagnostic hardware experiment

The virtual controller and collector completed this on the real Deck. The
feature run then repeated A on a board in a clear area, checked a three-second
hold produces one edge, captured private frames and numeric traces, and restored
the exact stock DLL and previous updater preference. A short X tap was missed
after virtual-controller hotplug; a one-second X hold registered. Future work
should test physical controller ergonomics and additional skating contexts.

The Mac controller has a read-only `trace` operation:
`python3 -B contrib/arcade-mode/lab/controller.py trace`. It outputs sanitized JSON
with source/hash/installation/timestamp and the last eight captures (at most 128
samples each). It refuses stale logs or a changed DLL. It prints no arbitrary log
lines and does not publish or alter the observer-acceptance report. Capture
completion means the end marker's count matches parsed samples, not trick success.

Do not automatically deploy a new feature build or modify the real game while
running. The accepted diagnostic report belongs to bbd1def; the new f669926
feature result is separately published. Remaining risk is broader compatibility
with physical controllers, varied terrain and game sessions.
