# Native trick findings

## Current source reuse checkpoint — 2026-10-14

The older exploratory notes below are historical. Current source is based on
`2f941376a3ec9f3972e2095fb5d8171022ee0a3a`: the92-pattern authored loader,
directional X table, playback A/B modes and guarded action/manual adapters already
exist. Do not restart endpoint searches or parser work from the earlier sections.
See `native-method-matrix.json` for all63 action rows and40 broader requirements,
and `upstream-method-survey.json` for exact source pins, licenses and evidence.

Prepared delivery/reference A/B remains the first Deck experiment. The full
community translator review found that default complex flips use recordings,
and its human verification scheduler differs from production. Neither it nor
the two additional controller implementations supplies stronger repeated native
acceptance than our current observations.

GoalTrains supplies a distinct LS-only revert fallback, spin prewind and rail
input continuity. Test the already-prepared community revert first. The official
native Darkslide method uses RB dark catch around a flip followed by rail contact;
our Y selector still needs actual actuation and shared bumper ownership.

Held flips deserve explicit review before declaring exact multiple flips absent.
The installed native guide already says holding adds rotations. Official native
documentation includes held Impossible variants, and the existing Throwdowns
composite record contains held-flip amount in addition to base trick identity.
Exact count units, release timing and free-roam observation remain unverified.
No reviewed source supplies the complete named Y table or the four unresolved
grab equivalents. Absence from these sources is not proof of native absence.

## Evidence standard

DISCOVERED is one named hardware observation; REPEATABLE requires controlled
repetitions and varied conditions; INTEGRATED requires useful chains and a
documented failure envelope. No flip currently meets the latter two levels.
Simulation repetition tests validate the mapper, not the game's responses.

## Independent native identifiers — 2026-10-14

On source 3fa4a43b68d0730063297c1e80130d715a99ab36, two A trials observed
native FlipTrickType 1. A later gameplay-ready batch observed ID 2 for both
left-stick + X trials and ID 3 for both right-stick + X trials. Each returned
to riding and neutral input with no physics/identifier drops or evidence gap.
The existing timed adapter is still in use; these results do not validate the
new authored adapter, other stances, or air/grind chaining.

The preceding D-pad batch opened the native Social panel. Subsequent X presses
entered its settings, and no flips occurred. A short setup Back press did not
close it. Two delayed 300 ms Back presses closed the settings and parent panel;
fresh gameplay eligibility was checked before the valid left-stick batch.
Menu-interrupted trials are excluded from gameplay success counts. Both input
adapters now consume D-pad modifiers only during Arcade gameplay; native menu
navigation and Arcade OFF retain them. That correction is awaiting deployment.

The authored catalog failed to load on that build. The d799c63 follow-up loads
92 patterns with four rejected assets. Its first authored trials produced Ollie
ID 1 twice, but requested Kickflip/Heelflip produced opposite IDs 3/2, matching
the lateral discrepancy with the upstream test. The complete primary matrix
continued and remained neutralized; it did not establish correct advanced flips.
The next correction reflects authored lateral input for the measured clear-bit
stance. Other stance contexts and corrected exact results remain acceptance
items. See authored-control-results.json and authored-flip-catalog.json.

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

## Historical implementation and unanswered questions

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
