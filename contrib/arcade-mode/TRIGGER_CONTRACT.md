# Contextual trigger ownership

The core input owner maintains `TriggerJournal` in `UnifiedInput`. It reads
the existing `InputHistory`; adapters must not sample another set of edges.
`TriggerPress` retains its original press/release events, input context and
native owner/episode observation. Held state never becomes a fresh press on
takeoff, touchdown, owner replacement or Arcade OFF/ON.

Priority is established Manual Pivot, qualified landing Revert, eligible vert
transfer, then ordinary ground setup/Switch. LT setup is also available in
Manual/grind context without claiming the balance sticks. Ground RT must not
fall back to a Revert trajectory.

`TriggerRouting::qualify` receives the original record, current native context
and current time. It may return `landing_revert` or `transfer` with an absolute
deadline no later than original press + 500 ms. `ready=false` reserves the edge
while awaiting actual native contact; `ready=true` allows the existing action
adapter to attempt execution. Reservation consumes the same physical journal
footprint. Later polls cannot renew its deadline or assign a different action.

Landing qualification belongs to the vert controller: at most 180 ms before a
geometry-corroborated touchdown and 250 ms after actual native contact. Ground
before takeoff is ineligible. The session requalifies the retained request
before using the shared auxiliary Revert actuator. A request/claim is not
native movement, support, stance, animation or scoring evidence.

The ground engineer owns `arcade_input_events.h`, `arcade_trigger_input.h`,
`arcade_input_dispatch.h` and `arcade_input_session.h`. The vert engineer owns
the flight/landing qualifier and physical transfer controller. Shared runtime
wiring is integrated deliberately; do not merge competing input handlers.
