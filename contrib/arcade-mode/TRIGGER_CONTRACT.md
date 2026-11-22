# Contextual trigger ownership

The core input owner maintains `TriggerJournal` in `UnifiedInput`. It reads
the existing `InputHistory`; adapters must not sample another set of edges.
`TriggerPress` retains its original press/release events, input context and
native owner/episode observation. Held state never becomes a fresh press on
takeoff, touchdown, owner replacement or Arcade OFF/ON.
LT and RT both cross the same strict30/31 physical dead zone for press, held
state and release; the journal is the sole threshold/edge producer.

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
The original `trigger.native` always retains its flight provenance. Only a
transfer sets the request queue's continued-AIR episode gate: grounded native
observations retain prior flight numbers, which must not cancel Switch/Pivot.

Landing qualification belongs to the vert controller: at most 180 ms before a
geometry-corroborated touchdown and 250 ms after actual native contact. Ground
before takeoff is ineligible. The session requalifies the retained request
before using the shared auxiliary Revert actuator. A request/claim is not
native movement, support, stance, animation or scoring evidence.

`NativeGesture::Output::vert_trigger_owned` exposes only the currently held LT/RT
channels claimed for transfer or landing Revert. The session resolves that claim
before treating a trigger as a conflicting flip/grab/Manual modifier. A consumed
trigger therefore cannot cancel an existing trick or block an eligible follow-up;
another unclaimed channel still conflicts normally. This mask grants no native
actor, stick, support or trajectory permission. Native readiness and owner checks
remain in each action adapter.

The Switch adapter receives only a `switch_stance` claim from ordinary ground.
It emits the supported build's native ChangeStance B double-tap through the final
controller packet. Original RT release does not cancel an initiated sequence;
actor conflict, owner loss, stale state or OFF cancels output. A delivered phase
advances only with a new controller tick and native physics sample. Native
leading-foot change must follow the actual second press and final release before
completion is reported. This adapter does not implement a ground Revert or a
render-only stance change.

The ground engineer owns `arcade_input_events.h`, `arcade_trigger_input.h`,
`arcade_input_dispatch.h` and `arcade_input_session.h`. The vert engineer owns
the flight/landing qualifier and physical transfer controller. Shared runtime
wiring is integrated deliberately; do not merge competing input handlers.
