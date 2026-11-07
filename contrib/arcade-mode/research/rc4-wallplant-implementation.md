# RC4 wallplant prototype

This implements an experimental wallplant path. Windows compilation and live
wallplant acceptance are pending. No native score, combo credit, animation state
ID, landing state or bail flag is manufactured. The milestone contract remains
open until the original native evidence establishes its required sequences.

RC4 integrates RC3 through `134e715d41dfe30bc9a20015b1e3c45160e9eec0`
with a merge preserving the original RC4 specification commit. The latest mongo
push No Comply implementation and RC3 findings are retained unchanged.

## Source decisions

The entire current upstream tree and retained fork survey provide no complete
wallplant adapter. Existing wallride/wallie settings and the native ground
Footplant state are distinct mechanisms. Native one-footer input can commit a
ground footplant and additional flight even after release; it is not reused as
an airborne wallplant command.

The original build-20260929 `process_body_contacts` routine supplies solver
contacts before their per-bone summaries. Its verified two-argument callback
is observed, and the original runs exactly once. Records provide point, normal,
other collidable and a complete native shape handle. Existing SDK pool readers
resolve the shape to its rigid body and check both dynamic and keyframed flags.
All new build offsets and byte fingerprints live in
`Engine/Game/Build/20260929/arcade_wall_contact.h`. The 20 source contracts were
checked against the pinned executable. No original executable or extracted
asset is committed.

The public per-bone reader was previously observed only during bails. The new
raw solver observer must establish that usable contacts arrive while riding
and still airborne. Neither a ray hit nor an empty contact observation proves
that capability. Bounded raw-contact telemetry includes empty airborne steps
to distinguish this failure from a missing observer.

A real, recent local collision is mandatory. Five independent rays corroborate
a broad vertical patch of that exact shape, rejecting narrow rails, edges and
corners. They do not create the contact. Ground clearance, native takeoff,
same-owner flight, shape/body lifetime, inward approach, controller ownership
and a fresh journal A are checked separately. Missing evidence leaves A on its
existing path. No second input engine is introduced.

The one-shot velocity transaction reuses the guarded articulated-body adapter.
It retains tangential momentum, selects a bounded outward/upward velocity, and
retires ordinary trajectory assistance. It does not teleport, suppress native
collision, force a flight state, or keep rewriting rebound velocity.

Prototype tuning comes from the retained classic arcade `Physics_Wallplant_*` reference:
20-degree minimum approach, 225 inches/second loss, 200 minimum exit speed,
500 vertical exit speed, 24-inch minimum clearance and 1000ms repeat exclusion.
Units use the existing Arcade metres-per-inch conversion. Existing GameplayProfile
speed limits cap the result. Tangential retention intentionally differs from
scalar horizontal speed loss. These values require measured Skate trials.

## Animation approximation

The original regular/fakie Judo footplant clips are loaded from the installed
game and mirrored through the existing animation library. The guarded local
pose facility plays one bounded clip after a successful rebound. Placement
joints and physics are untouched. Playback ownership tickets prevent cleanup
from cancelling a newer plugin clip. New trick input cancels the approximation.

This is a native asset displayed through the existing pose player, not an
authentic Footplant StateFlow/ActorController transition. The 160ms prototype
playback requires visual testing; clip-clock logs do not prove rendered foot
contact or original actor initialization. A rebound with this approximation
does not close full wallplant acceptance.

## Independent tests and required hardware evidence

Portable tests cover rebound geometry, bounds, body-batch atomicity, contact
and owner lifetimes, separation, cooldown, fresh input, held-A rejection,
ground A, and rejection of unavailable geometry. Query tests use fixture
storage, including narrow rails, seams, changing owners and invalid normals.
Synthetic tests are not native gameplay acceptance.

Hardware preflight observed the existing RC3-24 DLL on `skate3_megapark`, local
state100 and native/cached step 1/30, without input or state changes. The
897-sample capture SHA-256 is
`745b30af3e1d4f164b10d3c788ec5efae6cb248246ccb61c3f9f64c6688cda61`.
This is baseline evidence only, not an RC4 wallplant trial.

Before acceptance, verify the exact built and loaded RC4 DLL, actual riding
wall contact, outward/upward native velocity, continued air, original animation
advancement, native scoring/combo ownership and clean landing. Test head-on,
angled and both stances; no-A, floor, slope, ceiling, rail, stale/repeated
contact and Arcade OFF negatives. Then run every sequence in
`WALLPLANT_MILESTONE.md`, including flip/plant/flip/manual continuity. Preserve
RC2 and all RC3 rollback artifacts. Do not promote the accepted branch.
