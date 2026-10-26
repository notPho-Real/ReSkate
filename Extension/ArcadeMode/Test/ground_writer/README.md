# Optional ground-writer observation

The existing motion writer can emit `ARCADE GROUND WRITER` scalar rows only while
Arcade Mode and the existing Trainer logging lease are active, inside a finite
schema3 controller-poll capture, and when published Down or native brake request
bits are present. Schema4 is excluded. The existing client queue and worker are
reused; no native reads, writes, input, waits or physics permissions are added.

At most 16 rows are admitted per capture, at most one per original physics step.
Duplicate, over-budget, contended, stale and invalid eligible observations use
the existing client loss counter. Queue-full rejection uses the same counter.
Such loss can make the accompanying capture incomplete. Missing rows never
mean that no writer ran, and a complete input capture does not establish complete
writer coverage. Early returns before this observer, unsupported captures and
unavailable anonymous owner joins have no writer receipt. A terminal receipt may
arrive after its capture footer; its original capture ID is retained.

When `motion_valid=true`, original time, step and generations come from the exact
frame consumed by the motion adapter. With `motion_valid=false`, they identify
only the latest original observer context for the same owner, not permission.
Published input proof fields and `controls_current` remain separate. The owner
number is the existing capture-local anonymous token; native addresses are never
serialized. Current sampled request/contact fields may differ from that original
frame and must not be merged into a new proof.
`ms` is entry into `arcade_air_apply`, not completion of the write loop; the
receipt adds no completion timestamp or host/native clock mapping.

`stage` is 0 observed, 1 policy evaluated, 2 staged for the commit function,
3 prewrite source/owner validated, 4 write loop entered, or 5 all existing verified
body writes returned. Policy fields are known only after stage1; the selected
brake is known only when `selected_known` is true. Requested resistance is the
existing policy result; staged resistance and staged velocity are known at
stage2, before any write validation or completion. Staging can include the
existing air/turn changes as well as resistance.

`completed` counts bodies whose entire existing linear/angular/dirty-flag write
sequence returned. Each existing `body_write` already checks copy and read-back
equality. A stage4 exception may have changed part of the next body even when
`completed=0`. Stage5 is not an atomic all-body snapshot, a post-loop owner proof,
or final engine velocity. `unwinding` separately reports an exception during or
after the loop. Earlier native/Trainer forces and later engine work remain outside
this observation; it cannot establish the engine's native B input mapping.

The portable test compiles exact production observer, `body_write`, and commit
function bodies against nonactuating memory/owner boundaries. It checks disabled
and capture gates, source/owner failure, every partial-copy/read-back boundary,
bounded loss/closure, and queue recovery. Existing scalar Manual fixtures retain
the exact updated consumer with an inactive observation scope. These are offline
software checks, not native motion acceptance.
