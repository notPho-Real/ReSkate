# Trickpedia evidence and compatibility review

Game build: **20260929**. This is an evidence import and source compatibility
review, not a new hardware acceptance result. Gameplay branches were not merged.
The exact reviewed target implementation fingerprint is
`20767feb8272c1ae8f8f6b0bd893a74db44d2c9aa0a6647a274d2b01d8dbb290`.
It is the normalized gameplay-source manifest defined by
`cmake/ArcadeEvidence.cmake`. Any different fingerprint needs another scoped
source review or fresh native validation. Documentation, evidence, acceptance
policy, validation and presentation files are excluded from that manifest;
canonical bindings and production execution remain included.

## Derivation and acceptance

`arcade_trick_capture_data.inl` attaches original observations to canonical IDs.
It stores no primary status or maturity. A trial is identified by the digest of
its native capture and its documented execution segment. Separate report rows,
observer samples or duplicate imports do not create independent successes.
Several documented attempts within one capture may count independently, but
only under their distinct source trial names/serials. Counts are successful
execution observations, **not all attempts**. A successful entry may have a
failed exit; both facts remain visible.

An executable with no compatible successful execution is Implemented; one is
Observed; two comparable independent executions are Repeatable. Repeating the
core flow twice gives Acceptance Pending. Verified additionally requires all
applicable criteria repeatedly established in comparable conditions, provenance,
build/fingerprint compatibility, no conflicting records, no invalidation and no
applicable current failure in another context. Earlier successes do not hide a
later failure. Mixed pass/fail records are labelled without inventing chronology.
Unresolved inconclusive results for an applicable criterion also prevent Verified,
even when other trials succeeded. They remain distinct from demonstrated failures.

The compact checklist includes current-compatible observations from all contexts
so a tested criterion is not called untested. These checklist rows alone cannot
certify Verified by combining unrelated conditions. Technical details retain
each context, source revision, capture, observation and limitation. Incomplete
legacy records, invalidated records, historical records and original-control
comparisons are distinguishable. None can certify the current implementation.

`arcade_trick_acceptance.inl` defines obligations by the actual executor:

| Behavior | Native sources and required acceptance |
| --- | --- |
| Ollie | Launch descriptor and air motion: fresh input, native identity, single legitimate launch, trajectory, clean landing, stance and combo. |
| Flips | Native flip/held transition and original rotation observations: actor, board rotation, catch, landing, applicable input timing/repetition, stance and combo. No grab-release requirement. |
| Grabs / finger transitions | Native grab transition and finger entry: exact actor/pose, hold, release/native exit, landing, timing, stance and scoring. Fingers additionally require catch and completed destination transition. Actor entry alone does not prove switch completion. |
| Manuals / revert | Manual input and lifetime observations: native contact, balance lifetime, transition, actual pose, HUD, stance/revert coverage, timing and combo preservation. No airborne landing requirement. |
| Grinds | Grind-change executor and native exit observations: named contact, transition, exit/pop, landing, timing/source coverage, stance and combo. |
| Footplants | Native footplant entry: prerequisite, actual planted-foot action, one launch, trajectory, landing, timing, stance and combo. Intent alone is insufficient. |
| Grounded grab / turn | Original ground-grab actor and ground-rotation owners: appropriate contact, pose/rotation, hold/release or transition, stance and combo. |
| Planned wall tricks | Actual collision, planted-foot action, bounded rebound, airborne continuation, landing, stance and combo; no executable is introduced by these obligations. |

The policy references the corresponding repository source paths. Synthetic
tests prove applicability and fail closed when metadata or criteria are missing;
they do not establish gameplay success. Untested specials remain Planned.

## Source compatibility scope

The target is the registry migration of RC3 baseline
`9906af3b5d4816c15d4506430d51afb7d860b970`, with native executors preserved.
The migration was inspected against production selection/mapping/lease and
InputSession adapters; existing model, recognizer and native binding tests cover
those adapters. Those tests support the source comparison, not gameplay claims.

* RC3-24 `5c841c20c82b5782f6ff4ac861aa0e9683159d97`: the diff to the baseline
  changes grab transition/ground-grab paths and held-flip preparation retirement,
  plus fixture/observer code. Single-flip selection, native Ollie launch,
  manual entry and manual/revert execution paths retain their relevant native
  calls and conditions. Carry forward only the observed ordinary single flips,
  pre-flip Ollie launch stages, manual entry and moving nose/revert contexts.
  The **double** flip observations stay historical because held preparation
  changed. Old grab observations also stay historical; no broad compatibility
  exemption is applied to that family.
* RC3-28 `2c2ce125d0a71d01e0decd73c8ec6ba4688660f7`: the subsequent production
  delta to the baseline is confined to ground-grab descriptors/entry. Held-flip
  and finger-transition paths are unchanged before the registry adapter
  migration. Carry forward only the documented native 30 Hz ordinary flip
  and ramp finger contexts, preserving their timing/exit failures and limits.
* RC3-22 `92b727f92da5331713367f8a3136d623f8df98e9`: footplant recognizer and
  dispatch paths are unchanged to baseline. The native-ground-input delta is
  the separate ground-grab actor route. Carry forward the one documented
  No Comply prerequisite/entry/flight/landing trial, without claiming exact
  planted-foot animation, single-launch classification or scoring.
* Older Boneless, tail-fakie manual, grab and failed prerequisite observations
  remain historical. No unreviewed equivalence with those implementations is
  assumed. RC4 wallplants remain isolated and Planned here.

Compatibility applies to the listed observed behavior/context only. It neither
fills untested criteria nor asserts that every variant in a family is reliable.
The exact target fingerprint, scope and this review are stored on each carried
trial. Changing evidence does not create or update a compatibility fingerprint.

## Imported observations and limits

The report snapshot manifest is `trickpedia-evidence-sources.json`. It records
immutable report revisions and SHA-256 values, distinct from the revision of
the DLL actually tested. Newer RC3 reports were read at committed snapshot
`a18aa002a438080cc58b78b53de059774b5ba2aa`; no gameplay from it was integrated.
Capture digests and individual trial segments are retained in the canonical
evidence data. A report reference is provenance, not a promise that raw captures
are distributed in the installable build.

| Trick | Derived result in the reviewed build | What remains open |
| --- | --- | --- |
| Ollie | Experimental / Repeatable; three qualifying native launch-stage executions and one inconclusive motion trial preceding the separately measured flips | Subsequent flip landings do not prove a standalone Ollie landing/combo. One raw trajectory anomaly requires investigation; stance coverage also remains open. |
| Kickflip | Experimental / Acceptance Pending; two comparable rotation/actor/catch/landing observations | Complete timing, stance and numerical scoring/combo coverage. |
| Heelflip | Experimental / Observed; one compatible single-flip trial | Repeatability and remaining timing/stance/scoring obligations. |
| Double Kickflip | Experimental / Acceptance Pending; two RC3-28 positives at 100/150 ms | Complete timing, stance and scoring. RC3-24 positive and late-repeat bail retained historically. |
| Double Heelflip | Experimental / Implemented in current build; historical Observed | The single RC3-24 double used the earlier preparation guard; fresh validation or narrower compatibility review is required. |
| Manual | Experimental / Repeatable; 18 explicitly qualified grounded entries in the 60 Hz matrix | Sustained balance, actual pose, transitions, stance/timing, HUD and combo; older moving tail-fakie lifetime remains historical. |
| Nose Manual | Experimental / Repeatable; 19 qualified entries | Current moving-revert observations include sustained contact, inconclusive continuity samples and observed low-speed fakie loss; no full acceptance claim. |
| Grounded manual / revert / fakie manual | Experimental / Repeatable for the three comparable moving nose-revert entries, with low-speed stance failures visible | Exact final pose/blend, repeated complete balance flow, low-speed/chained transitions, stance and scoring. Unknown raw samples are inconclusive balance evidence, not manufactured gameplay failures. Tail-fakie lifetime remains historical. |
| No Comply | Experimental / Observed; one compatible native entry/flight/landing | Planted-foot animation, single-launch classification, repeatability, timing/stance and scoring. |
| Boneless | Experimental / Implemented; historical Observed | No exact-target review for the old native entry; animation, single launch and scoring also open. |
| Benihana | Experimental / Implemented; historical Observed | One ramp lifecycle reaches native ground; exact Loop/Exit and scoring open. Earlier cleanup failure retained. Changed grab path prevents certifying current reliability. |
| Finger transitions | Per canonical identity: current 10/23 Observed; 9 Repeatable with landing failures; 12 Observed with landing failure; older 1/11 historical | Exact exit, completed switch blend, timing/stance and scoring remain open. Holding B through landing does not establish airborne release. |

All records remain Experimental or Planned; none has been promoted merely to
produce green entries. The exact counts are derived from trials, not this table.

The original compressed RC3-24 timing/heel captures were also inspected read-only,
and their decompressed SHA-256 values matched the report digests. Each of the
four named segments has one native 103 entry and Ollie identity 1 before flip
identity 2/3. The two timing segments and heel/double segment rise approximately
0.599 m before the flip, with peak vertical velocity about 5.497 m/s. Native and
cached steps both read 0.033333331 s. These three support the launch-stage motion
claim. The heel/single segment instead moves from 0.070 m at 0.343 s to 1.089 m
at 0.410 s (25.327 m/s), then 0.576 m at 0.439 s (-15.404 m/s). That discontinuity
is **inconclusive appropriate-trajectory/single-legitimate-launch evidence**,
not a fourth successful Ollie. Its input/identity observations are retained.
This audit neither diagnoses the older engine behavior nor claims that a flip's
later landing is standalone Ollie acceptance.

`GRIND_TRANSITIONS_2026_10_30.md`, `grind-fixture-results.json` and
`grind-exit-results.json` were reviewed. Constituent native-control physics,
unnamed contact and old rail-exit captures do not establish a correlated,
named, integrated current Arcade transition. No named grind success was
invented from these records. Airwalk original-control release comparisons are
retained as comparisons only, separate from Arcade failures and later RC3 fixes.

## Validation and hardware boundary

The evidence tests run inside the existing Trickpedia targeted/full-preflight
suite. They cover the full progression, trial deduplication, incomparable
contexts, applicability, missing versus failed criteria, build/fingerprint
staleness, explicit compatibility, invalidation, incomplete legacy metadata,
original-control exclusions, presentation and unchanged runtime binding data.
The CMake fingerprint integration test checks evidence-only edits, actual binding
changes, and LF/CRLF equivalence using the production module.

Native menu/controller navigation, installed/loaded DLL identity and same-session
Arcade ON/OFF still require the exclusive scheduled Deck slot. Source review,
portable tests and a Windows build do not replace that acceptance.
