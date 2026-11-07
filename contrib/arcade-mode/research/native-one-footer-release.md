# Native one-footer release ordering

The original-control comparison in `rc3-native-footer-release-28.json` isolates
a release-order failure on Skate 3 Mega Park. Both trials enter Airwalk12 and
the original OneFootExit state. Releasing foot and hand at3.75s bails504;
releasing the foot at3.75s while retaining the hand until4.75s returns to ground100.
Native launch sampling and peak height differ slightly; this is not proof of
identical physics trajectories. Neither trial proves native scoring.

The existing Arcade adapter withdrew both inputs on physical B release after
actor acknowledgment. That is a routing/lifetime defect. Original
`OneFoot.Exit.Node` (5c0c8794-dde2-5b44-5c0c-8794dde25b44) has a transition to
`Grab.Loop.Exit` (transition10f8f0ac-29a5-5e58-10f8-f0ac29a55e58) conditioned on
CanGrab=false. The normal return to the grab loop is a separate transition.
The exit node owns Bool.State.Is.OneFootAir and Bool.Anim.Is.OneFootExit.

Searches of Arcade, the frozen official integration tree and existing surveyed
fork/source notes found no reusable one-footer release adapter. Extend the
existing Lease, actor acknowledgment, state-block resolver and owner checks.
On release, withdraw foot input and retain the established native hand/selector
while fresh original one-footer tags remain active. When both tags clear,
no active tag can be read, the owner/air episode ends, the heartbeat expires or
Arcade turns off, withdraw the hand. Restore the existing tweak request while
the native exit owns the pose. No fixed-duration exit, animation clock write,
identity write, new asset, velocity change or scoring write is used.

Local checks cover all13 profiles at30/60/120Hz, original foot release before
hand withdrawal, native-state completion and invalid-owner/observation cleanup.
Ordinary grabs keep their immediate acknowledged-release behavior. These are
software checks. The new runtime must still demonstrate exact original exit
actor advancement, clean landing and no foot retrigger on the Deck. Repeat the
Airwalk comparison and test other one-footer families before acceptance.

RC3-30 hardware follow-up

Two Airwalk trials still bailed. The first retired at B release; the second
retained its hand until both native tags cleared, then bailed after returning
to the grab. The first-flight exit field may not yet have a state slot. Read
the tags independently and retain ownership when either verified tag is true;
unknown fields never count as active. Preserve the original lowercase dingo
in the exit asset lookup. This addresses admission of the release hold, not
the still-unverified return blend/catch. See rc3-native-footer-release-30.json.
