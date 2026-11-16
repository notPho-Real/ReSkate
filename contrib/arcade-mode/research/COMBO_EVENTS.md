# Combo event evidence

Status: the existing candidate runtime and corrected private UI asset reach
actual gameplay. An Ollie followed by Heelflip visibly produces x2 in the native
circle. Longer captured play exposes two unresolved rendering defects: the circle
is blank above x4, and custom trick names are absent despite their presence in
the native stack. Full corrected-candidate acceptance remains pending.

## Performed Ollies and native sequence replacement

A bounded Deck capture on the previously installed build shows physical states
100 → 103 → 200 → 201 → 100 and animation trick values 1 → 3. The native
ScoringData action list first commits only Heelflip. Counting only that list
therefore loses the preceding performed Ollie. Counting requested buttons or
physical takeoffs would instead award unperformed tricks.

The existing flip-state reader now offers a read-only observation checked
against the local owner before and after the read. The combo adapter joins it
to the existing physics flight identity. An observed Ollie contributes once per
flight; a committed native action whose internal name is `ollie` reconciles that
same contribution rather than adding another. Native points remain unchanged.
Missing ownership, ambiguous historical association and mid-flight joins cannot
manufacture additional credit. Custom trick scoring remains separate.

The original native HUD and its animations remain the display owner. No scoring
settings, native action records or game progression are written. The observer
adds no animation, physics or input actuator.

Focused coverage includes the captured replacement case, standalone Ollies,
late landing commits, repeated flips, multiple flights, held animation states,
missing evidence and ownership changes. The retained 90-sample replay produces
two actions and 75 native points; removing animation evidence leaves one action
and the same points. Four older capture replays preserve their totals. These
cross-process samples are not frame-atomic and do not prove rendered acceptance.

Before owner testing, verify the exact corrected DLL on the Deck: native ring
1x → 2x for Ollie → Heelflip, standalone Ollie 1x, consecutive linked flights,
Casper and other custom credit, banking/bailing, focus loss and Arcade OFF
restoration. Verify the original Skatepedia navigation and input annotations in
the same candidate; portable passes do not replace these device checks.

## Current source reuse checkpoint — 2026-10-14

The chronological notes below retain earlier observations, not current work order.
Native pose-qualified manual observation and20 manual-to-Ollie trials already
exist. Do not repeat the initial manual discovery matrix. The current directional
adapter needs combined-play acceptance, and the prepared community RT recipe
precedes further speculative revert arcs.

A concrete native scoring-event implementation was found in
[reskate-custom-mods at de837826](https://github.com/SmokeyDawg58/reskate-custom-mods/tree/de837826417b55e2a5756c303f4805ae6a39ae89),
under GPL-3. Its `one_up_native.cpp` observes actual line-start, line-end and
sequence-ended expression inputs on the same supported build. The source author's
recorded game evidence includes a22-point expiry bank confirmed by the player,
a rejected840-point bailed line, and11 finalized sequence scores totaling18,518
matching the native display. These are external Windows observations, not our
Deck repeatability. Contract hashes and extraction limits are pinned in
`upstream-method-survey.json` under `native-line-events`.

The next score integration should extract that read-only event path through the
existing expression hooks and schema binder. No extra game mode or UI is needed.
Keep native scoring settings unchanged; omit its multiplier-rule writes and
policy retaining previously landed points after a bail. Our unbanked combo must
be lost. Sequence scores may already include multipliers; do not multiply twice.

Before using these observations as combo truth, establish local ownership,
world lifetime, duplicate/recursive delivery and occurrence identity. Client realm
alone does not establish local ownership. The two line-end listeners must not
bank twice, and sequence payloads do not expose a demonstrated native serial.
Publish bounded sanitized records without game-thread logging or allocation,
report drops, then connect the existing combo engine. Native line timing and
Arcade link grace remain distinct until their integration is tested.

The existing strict numeric HUD observer remains a useful independent cross-check.
Its presentation lifetime and persistent labels are not a banking oracle. Physical
failure can reuse pinned native bail-request methods; no new combo engine is needed.

## Historical native observations

Native UI observation verified on276a396: score-schema-276a396.json records28
fields from the actual ScoringHUDViewModel. Boolean kind10/size1 and integer
kind8/size4 are observed, not guessed. Most text is in nested structures (sizes
80/104); two arrays exist. Follow-up probe is bounded to depth3 and256 nodes in
this single scoring schema, array count<=8, with no object-reference traversal.
Only numeric values and exact Manual/Nose Manual/FS Revert/BS Revert labels can
leave the probe. This is discovery evidence, not an authoritative event feed yet.

| Event | Existing evidence | Safe interpretation |
|---|---|---|
| Riding baseline | Real Deck state100 / crouch mode1 | On-board baseline, not every grounded context |
| Air | Trainer uses200–299; measured Deck ollie200→201 | Air state family |
| Landing | Deck ollie201→100 | Candidate landing transition; ownership gaps invalidate it |
| Grind | Build20260929/grind_exit.h measured400–499, especially401 | Upstream measured; reproduce on this Deck |
| Bail | no_bail selector300 and wipeout counter | Use actual wipeout, not requested bail |
| Manual / nose manual | Real Deck trials3/6 confirmed by user; trial3 native HUD says Manual; selector remains100/mode1 | Both DISCOVERED; numeric identity still unverified, do not infer from stick position |
| Revert | No verified identity | Do not infer from button/trigger position |

`ARCADE COMBO SAMPLE` adds a read-only listener through the existing multi-listener
physics-step API, preserving grind-exit and Hall of Meat ownership. It reads only
the already signature-guarded state/mode/eight request words. Queue64, drain16
per client tick; `ARCADE COMBO GAP` explicitly reports dropped samples. It does
not invoke a native function, force a selector, award points, or drive a HUD.
Reads are not transactional. Numeric changes are evidence, not named events.
Owner loss/menu/respawn and any queue gap must invalidate an inferred sequence.

Correction from real six-trial batch: +20000 Linux right-Y produced manual on
trial3; -20000 produced nose manual on trial6 (user watching actual Deck).
Private trial002/1.png also clearly displays native "Manual" scoring text.
State100/mode1 persisted, so that coarse selector cannot distinguish manuals
from ordinary riding. These are successful discoveries, not failed gestures.
Three-second holds, neutral before/after; repeatability and numeric event
identity remain unverified. Do not discard these successes or require speed
as a prerequisite without evidence. Fix observation and correlate repetitions.

Next bounded experiments: compare neutral riding against half-magnitude positive
and negative right-Y holds, then inspect actual manual animation/HUD plus state
transitions; repeat only identified candidates. For revert, compare an ordinary
landing with the game's documented native revert action and inspect physics-step
changes. No new Arcade revert button. Capture private screenshots, retain only
sanitized result tables in the repository. Native balance failure actuation is
UNVERIFIED; do not present combo loss as a physical bail.

Real Deck guide inspection (2026-10-13, old f669926 still installed): Skatepedia
Riding entry18 describes a small downward RIGHT-stick hold for a tail manual;
entry16 is FS Revert, with a LEFT-stick curved gesture and180-degree stance
change. This is documentation of native intent, NOT observed successful manual
or revert state evidence. Private captures: artifacts/combo-guide-riding-18 and
artifacts/combo-guide-fs-revert. No new revert button is warranted. Native X is
back-foot push, also confirmed in that guide; experimental Arcade X overrides it
only where its gesture eligibility/cancellation permits.
Repeatable guide navigation: Escape (Hub), Tab (side menu), Down twice, Return
(Skatepedia), Riding tab; Down18 for Manual, Up twice for FS Revert. Use350–500ms
between keys. The existing virtual Start recipe did not open Hub on this trial;
targeted Escape on the verified Skate window did. No settings were modified.
Additional private guide captures confirm nose manual uses slight upward RIGHT
stick; manual revert uses a quick LEFT-stick rotation while holding the manual.
BS Revert mirrors the FS curved-stick icon. Paths: combo-guide-nose-manual,
combo-guide-manual-revert, combo-guide-bs-revert. First manual discovery matrix:
RIGHT-Y Linux magnitudes12000/16000/20000 with both signs, hold3s, neutral before
and after, six candidates; then refine only candidates with animation AND native
state evidence. Revert matrix follows measured manual identities, using left-stick
arcs rather than inventing a face-button mapping.

## Pure model contract

IMPORTANT NEGATIVE CONTROL: subsequent left-stick semicircles also produced
word1 mask0x4000 while turning, without establishing a manual or revert. Thus
contact flags alone are NOT an actual manual event. Magnitude controls12000/16000
remained unchanged;20000 repeated both manual patterns. All16 follow-up trials
are summarized in manual-revert-controls.json. Four left-stick arcs (both signs,
400/800ms) did not establish a reliable revert identity. Next observable: native
ScoringHUDViewModel, already read elsewhere in ReSkate. New probe enumerates only
bounded schema metadata, booleans, and exact Manual/Nose Manual/FS Revert/BS Revert
labels; it never logs arbitrary strings or writes native state. Pending CI/HIL.

Further real batch `trick-batch-20261013T210055-33354`: two neutral controls,
two +20000 holds, two -20000 holds. Six controller neutralizations and reset
checks passed. Native HUD confirmed Manual in trial003/1.png and Nose Manual
in trial005/2.png. Word1 at context+0x13c8, mask0xc000, changed from idle0xc000
to tail0x4000 or nose0x8000 and back on release. Air200/201 has0; crouch103 can
transiently have0x4000. Hypothesis: front/rear wheel contacts, not command intent.
This needs negative controls and repeated state-qualified correlation before use.
Quiet log baselines caused the first tail trace to be omitted by the timestamp
watermark; screenshots remain valid. A byte-boundary collector fix is locally
tested, not yet CI approved. Do not count missing trace as missing trick.

`trick(points)` consumes pending links and ends balance. `landing()` opens450ms
grace once. `revert()` succeeds once during landing grace, opening a single450ms
pending window. `begin_balance(kind)` consumes it; duplicate continuous entries
are idempotent. `end_balance()` only ends balance: the observation layer must
report landing separately. Grace expiry banks into `take_banked_points()`;
explicit `bank()` returns points directly. `bail()` never awards active points.

`State::fatigue` is global combo fatigue (existing field preserved); grind and
manual fatigue are independent and persist across mode exits. All are0..1.
Bank/bail reset all three.240Hz fixed integration carries fractional time between
ticks; invalid dt is ignored and individual calls clamp to1s. Correction is
signed[-1,1]. No raw controller/game pointer enters the model. Seeded phase,
low-frequency summed sine, inertia, reduced damping/control and edge instability
are deterministic. The default curve is simulation-tested, NOT Deck-tuned.

## Measured simulation envelope

`Extension/ArcadeMode/Test/balance_envelope.cpp` emits the27 runs in
`balance-envelope.json`: ages0/10/30s ×30/60/120Hz × neutral, wrong-way hold,
simple proportional/velocity feedback. At60Hz neutral fails at5.75/3.33/2.57s;
wrong-way hold at1.57/1.43/1.28s. Feedback delays failure to16.6/13.83/14.6s.
The non-monotonic last pair reflects disturbance phase, not lower difficulty;
initial effective difficulty is0.078/0.598/0.65. Failure timing varies by at most
one30Hz input frame across sampled rates. These are deterministic simulations,
not human ability, native rail physics, or proof of acceptable Deck feel.

Reproduce from repository root:
`c++ -std=c++20 -I. Extension/ArcadeMode/Test/balance_envelope.cpp -o /tmp/arcade_balance_envelope`
then `/tmp/arcade_balance_envelope`.

## Nested scoring controls on823c3ad

Six real neutral/manual/nose trials completed, followed by turn/ollie controls.
Fields7f3e74e7 and7aab675c activate for BOTH manuals and ollies, so neither alone
is a manual identity. Neutral and ordinary left-stick turns leave them unchanged.
The158-node schema exposes ten kind15/size4 scalar fields inside the two80-byte
structures84db5623/1a93e458; these were omitted by the initial value allowlist.
Next diagnostic reads those bounded32-bit scalars, without assigning semantics
until controlled traces distinguish them. No native pointer calls or writes.
Manual→ollie repetition evidence:20/20 named chains in one flat near-stationary
context,11 Clean/7 Sketchy/2 unqualified; not Arcade model integration.


## Deferred native phase and score continuity

The native device trial of `da7efdec` performed an Ollie followed by Heelflip,
with a committed native action and 150 native points, but displayed zero Arcade
points. The client score transaction consumed a null observation during an
already admitted client/physics phase handoff. That discarded the inactive
baseline or invalidated a running score even though the combo lifecycle correctly
waited for the matching original physics callback.

The score transaction now shares that existing bounded phase deferral. It does
not read new evidence, award actions, renew a presentation, or clear the prior
baseline in that gap. Actual focus/ownership withdrawal still clears normally.
The source-composed regression executes the real transaction: the old code fails,
the correction shows one action for the observed Ollie and two after the native
Heelflip commit, retains them through landing, and refuses recovery after genuine
evidence loss. The rollback control must fail at the phase-boundary assertion.
The exact `886d3091` DLL built successfully and was tested on the device at a
measured 60 Hz native physics cadence. Ollie followed by Heelflip displayed 2x
through landing. Manual, Casper, Casper Flip and Ollie retained counts 1, 2, 3
and 4. The longer flatland sequence reached 12 performed actions, and the
balance-bail trial produced a failed-combo event without banking it.

This candidate did not pass full acceptance. At counts 7, 9 and 12 the original
HUD model contained the expected integer and text, but the circle's number was
not visibly rendered. Casper and Casper Flip existed in the native stack model
without visible names. Registry publication is not rendering proof. The rail
chain also remains unverified: the aligned attempt bailed before its first grind
request. These results require widget diagnosis and another bounded native check,
not a scoring-success claim based only on model values.

The Windows preflight compiled the new phase regressions but its existing test
selector omitted their original names. Registering them as
`arcade_manual_lifecycle_score_phase` and
`arcade_manual_lifecycle_score_phase_rollback` puts both under that selector.
Local CTest selection and execution pass; Windows execution of these renamed
registrations remains pending the next substantive candidate build.

The private authored-widget inspection identifies `Multiplier_Texture` inside
`ScoringMultiplier_Widget`. Its graph selects texture artwork; it has no text
label element. The field previously called `multiplier_text` is the root's
`LineMultiplier` label model, not the circle's drawing source. Updating that
string alone cannot display larger counts. The existing `ScoringAssetElements`
list accepts fill styles, not arbitrary child widgets, so it is not an attachment
point for a number label. A correction must attach native text inside the
multiplier's existing container, preserving its layout and animation transforms.
No replacement score HUD or capped numeric multiplier is an accepted solution.

The custom-row candidate now copies only a live native name label's styling
before setting its own text. It never copies the row's score or parent-stack
reference. Source-composed tests cover style retention and partial initialization
cleanup; the previous implementation fails the new style assertion. This is a
candidate initialization correction, not proof that its names render on device.

### Native circle text candidate after the 886 device capture

The circle now has a locally authored native label inside the same container as
its number texture. It inherits the texture's layout and the original container's
motion. A native float comparison selects the text above four and retains the
original texture at or below four. The text binds to the existing LineMultiplier
label model; it does not create a second score owner. Publication includes the
native `x` prefix. Native scoring, action admission and physics are unchanged.

Startup generates this amendment from the installed game's four UI assets into
a private built-in mod directory. The existing mod merger loads that output.
No extracted assets are embedded in the DLL or added to distributable source.
Equal generated files retain their timestamps, preserving the merge cache.
The amendment is absent when an older DLL without its factory builds the mod
catalogue. Device verification must include restoring the original presentation
when Arcade Mode is disabled, and the prior DLL rollback.

A sanitizer-enabled private export/import check preserved all 2,569 original
widget fields, including boxed animation data. Only the container's children,
the widget's components and the root's property connections were extended.
The graph adds one native label and three native binding/visibility components.
The generic EBX authoring support explicitly imports only non-array types,
remaps field descriptors across assets, and preserves array reflection hashes
when cloning. Focused tests cover round trips, malformed/conflicting donors,
source preservation, and a negative control without field remapping. Native
score-presentation, custom-row and existing EBX merge regressions also pass.

These checks establish source/asset consistency only. The new text has not yet
been rendered on hardware. Native custom-row visibility, high-count rendering,
Arcade OFF restoration and the remaining requested rail/grab chain still need
exact-source device acceptance before this candidate is ready for owner testing.


### Native pack startup regression

The first native test of the text candidate did not reach gameplay. The generated
pack lacked the ordinary game-version stamp and its archive omitted the required
Win32 directory. After correcting those two details in a temporary local mod,
the launcher admitted the asset, but the game exited during loading. No fatal log,
core dump or kernel out-of-memory event established the exit mechanism. The
prior DLL, settings and receipt were restored; no successful visual claim follows
from that attempt.

Offline comparison then found a definite export defect. Adding the label's EFIX
type inserts a signature before the GUID-only suffix. Existing type references
kept their previous slot numbers and consequently resolved to different GUIDs.
The exporter now moves parsed references, opaque boxed payload references and
extra type records with that suffix. Primitive and empty references stay intact.
The authored regression fails on the previous exporter and passes with the fix.
The private graph check now compares resolved type identities, including boxed
values, instead of comparing only encoded slot numbers. An independent EFIX
reader distinguishes the failed asset and confirms preservation in the correction.

Generated packs now receive the same supported-game stamp checked for ordinary
native mods, and their archives use the same Win32 path as GameArchives::read.
An authored pack regression reads the edited payload through GameData, checks
its hash and identity, preserves the source archive and verifies that regeneration
does not change an equal file's timestamp. The local run uses production codecs,
factory and archive readers with macOS file/hash shims; Windows integration is
still required. The corrected asset still requires native loading and rendering
acceptance using the existing candidate DLL before a consolidated runtime build.


The corrected private asset was subsequently tested with the existing candidate
runtime on the device. The launcher accepted the ordinary mod, the startup log
confirmed the scoring widget override and mounted the merged patch, but the
game again exited during initial level loading before a valid local skater was
available. No fatal log, recent crash report, core dump or kernel out-of-memory
event identified the mechanism. This is a failed native loading check, not a
gameplay or visual pass, and does not establish that the type-reference defect
was the only cause.

Both test-owned packs were removed by exact file manifests, and the prior
runtime, settings and install receipt were restored. All 122 rollback sets and
stock files were verified; the session transport was revoked before the explicit
stopped hardware handoff. No additional runtime build was started.

The next isolated loading controls retain every native widget instance and
field: an unmodified exporter round trip, followed by reflection-only label type
import. These private controls separate general rewrite/loading problems from
the added UI graph. Their local consistency checks cannot replace device loading
acceptance. Private game-derived assets and captures remain outside source and
distributable packages.


A second independent binary comparison found metadata loss even in an unchanged
widget round trip: its enum array retained the integer values but its EBXX
class reference changed from the native enum type to the primitive sentinel.
The exporter previously emitted type identities only for structure arrays.
It now resolves both structure and enum array identities through EFIX; primitive
arrays retain the sentinel. An authored test uses different reflection and EFIX
indices and signed enum values. It fails on the previous exporter and passes with
the correction, together with the five focused sanitizer checks.

The failed device asset was recovered from its retained private CAS for the
comparison. An independent EBXX reader confirms that the corrected candidate
preserves the original enum array identity while the failed candidate does not.
This identifies another export defect, not yet a proven complete explanation
of the native exit. No new runtime build or successful native loading claim
follows from the offline result.


The enum-corrected private asset was next exercised with the existing candidate
runtime, without another DLL build. The mod was admitted and the game displayed
the native map/menu. This does not establish a valid loaded level or local skater:
the follow-up native state check failed before gameplay acceptance. A map-close
menu input had already been sent by that helper. Normal window close did not
stop the process; targeted graceful termination of the verified test process was
required. The separate outdated-mod warning was observed but is not proven to
explain the shutdown failure. Both session packs were removed and the prior
installation and all 123 rollback sets were verified before hardware return.

The next asset-only trial separates native level observation from menu input and
requires the selected test map and local skater before gameplay. Multiplier text,
custom history visibility and full combo acceptance remain unverified. The new
Python metadata comparison can recheck retained assets without compiling or
using hardware; its authored regressions and retained negative control pass.


## Latest asset-only gameplay check — 2026-11-15

The existing runtime at 38f8b06f was reused with the private widget generated
from 380d9692; this was not a new integrated DLL. Native observations verified
the actual Mega Park level and a live local skater before gameplay inputs. The
short Ollie/Heelflip trial reached states 100, 103, 200 and 201; the captured
native circle visibly showed x2, 75 points and a 150-point line. A visible x1
stage was not established. No fractional multiplier was observed in this trial.

The longer flatland chain reached model multipliers through x12. Its native
trick stack contained owned Casper and Casper Flip rows alongside the original
native rows. Actual rendered frames showed x2, x3 and x4, but the circle became
blank at higher counts and the custom names were not visible. Model publication
and successful controller execution are therefore not visual acceptance.
Do not repeat loading or row-insertion discovery to investigate these failures.

Offline inspection of the retained native widget declarations confirms that the
authored comparison uses the existing A, B and A>B property ports and that the
native label listener donor also uses an empty DataType with Expand enabled.
These checks rule out those specific declaration mismatches; they do not prove
runtime listener binding, label visibility or font resolution. The native trick
row has additional label and alpha bindings, so copying its text/style alone is
not proof that the complete row renders.

The session ended after this failure. Both temporary packs were removed, the
prior installation was restored, all 124 rollback sets were preserved, and the
transport was revoked before the stopped hardware return. Further source and
retained-evidence analysis takes precedence over another build or device trial.
The remaining acceptance matrix includes higher rendered counts, custom names,
banking/bailing, cancellation, Arcade OFF, Skatepedia and the requested combined
flatland and rail/grab sequences on the final candidate.


The multiplier failure now has a source-backed binding defect: the added
listener copied its donor's path and fields but omitted both incoming property
connections from the widget interface. The new clone preserves those inputs
before connecting its text output. A focused authored regression rejects the
old clone behavior, and the regenerated private asset retains all 2,569 original
fields. An independent text-graph inspection finds zero inputs on the failed
listener and both native inputs on the correction; the separate metadata check
also passes. This is an offline correction awaiting rendered acceptance, not a
new DLL or a completed Casper-name fix.


The follow-up context-binding trial reused the existing DLL with the regenerated
private widget. The expected DLL was mapped, but navigation remained in the
native hub. Neither keyboard menu actions nor a settled controller Back press
established the requested test level. The out-of-trial controller-route samples
reported no available source; they do not prove why menu navigation failed.
No combo or rendered-count acceptance was obtained. The verified test process
required targeted termination after both normal close methods failed. Both exact
temporary packs were removed, the previous installation was restored, and all
125 rollback sets were verified before revoking the session and returning the
stopped device.

Offline source comparison identified a separate row integration mismatch:
the standalone score-line factory initializes stack items with the d-pad stack's
transition. Native-history augmentation now replaces that owned item's transition
with the destination trick stack's DefaultTransitionStyle, using the existing
typed copy API, just as native stack insertion elsewhere does. It does not copy
a native row or its scoring back-reference. The focused regression uses distinct
transition values, rejects the old path, preserves every native item byte, and
covers partial-copy cleanup under sanitizers. This corrects transition ownership;
rendered Casper names still require native verification.

The retained native row graph also shows that TrickName alpha is computed from
its measured width and the stack-supplied Index. Its Visible, font and RawText
inputs come through the standard label binding. A text/style copy alone therefore
does not prove visibility; the next bounded capture must inspect the actual row
and its stack transition alongside rendered frames.
