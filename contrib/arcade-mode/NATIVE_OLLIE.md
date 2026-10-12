# Native ollie investigation

This is the active A-button milestone after observer acceptance. An A-to-ollie
command is **not implemented**. Do not treat the diagnostic build as one.

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

## Next hardware experiment

1. Wait for all release gates, then explicitly deploy the tested diagnostic build.
2. Launch normally. In a clear flat area, tap A once and do nothing for five seconds.
3. Tap A once, then perform one ordinary ollie with the game's existing stick
   control within five seconds. Avoid further A presses during that capture.
4. Repeat once to separate repeatable crouch/pop requests from incidental changes.
5. Collect only the matching diagnostic fields plus installed source/hash and
   freshness evidence. Compare idle and native-jump captures; preserve findings
   here and in STATE.md before implementing any request producer.

The Mac controller has a read-only `trace` operation:
`python3 -B contrib/arcade-mode/lab/controller.py trace`. It outputs sanitized JSON
with source/hash/installation/timestamp and the last eight captures (at most 128
samples each). It refuses stale logs or a changed DLL. It prints no arbitrary log
lines and does not publish or alter the observer-acceptance report. Capture
completion means the end marker's count matches parsed samples, not trick success.

Do not automatically deploy this build or modify the real game while running.
The previously accepted observer report belongs to f09872f, not this new build.
The remaining blocker to claiming an ollie implementation is identifying and
validating its native input/request path, including ownership, calling thread,
grounded gating, stale-request invalidation and one action per A press.
