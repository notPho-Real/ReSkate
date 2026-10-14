# Native trick findings

## Evidence standard

DISCOVERED is one named hardware observation; REPEATABLE requires controlled
repetitions and varied conditions; INTEGRATED requires useful chains and a
documented failure envelope. No flip currently meets the latter two levels.
Simulation repetition tests validate the mapper, not the game's responses.

## Known observations before X mapping

Installed source: f669926ab790d1328bc234a9828c290fb46ee276. XInput coordinates
(Y up-positive): down (0,-32768) for 600 ms, then (+28000,+28000) for 250 ms
produced Kickflip Sketchy once; mirrored lateral sign produced Heelflip Sketchy.
Horizontal-left flick produced Heelflip Clean once; quarter/half-circle left
step paths produced Heelflip Sketchy, and down/lower-left/neutral had no named
HUD result. These are sparse observations, not reliable mappings for all stances.

Private hardware captures on 2026-10-13 show the game's own guide:
`artifacts/x-guide-kickflip/0.png` and `x-guide-fs-shove/0.png` (the latter is
actually the heelflip entry). Both flip descriptions distinguish toeside vs
heelside and say a hold can add rotations. This supports testing release duration
and stance, rather than assuming 250 ms is universally correct.
`x-guide-fs-shove-02/0.png` names FS Pop-Shuvit and shows a large curved scoop
from down toward toeside/up. This is guidance, not an executed shove-it.

## Current implementation and unanswered questions

Shared profiles in arcade_gesture.h preserve A timing. Experimental X neutral
or left selects the discovered kickflip endpoint; X right selects heelflip.
D-pad wins over left stick. All other named selections remain unimplemented.
Only a fresh on-board state100/mode1 permits starting. Therefore X during an
existing ollie currently cannot initiate a flip: air chains are an explicit
unresolved requirement, not claimed integration. Physical right-stick input
above the established deadzone cancels synthetic input; holds do not retrigger.

Next bounded batch: A, neutral X, X+left, X+right, each followed by land/reset.
Inspect actual HUD identity and native samples. Then 20 controlled repetitions
per supported flip, followed by hold/mash/physical-stick and landing-timing
boundaries. Preserve the first failed cases rather than counting skipped
contexts or a numeric state100 match as successful tricks.

Following scoop search hypothesis: shape/velocity distinguishes shove-its from
flips. Use constant-radius interpolated semicircles with 8 samples, setup fixed
at 600 ms, sweep durations 80/160/320 ms, one sign at a time. Earlier stepped
diagonal endpoints did not prove continuous circular paths invalid. Do not
promote these candidates to X mappings without named native outcomes.
