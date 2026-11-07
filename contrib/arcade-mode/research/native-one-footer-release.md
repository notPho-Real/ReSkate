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

Higher-airtime follow-up

The owner identified inadequate approach speed in those trials. A straight
approach at roughly12m/s subsequently gave Airwalk, Benihana and One Foot Tailgrab exact original
Into/Loop/Exit advancement and clean ground100 returns. Retain the earlier
failures, but do not attribute them to a selector-retention defect. Judo's
longer exit was still active at its landing in the corresponding trial.

The existing source index resolves the second exit-to-loop condition as
EBool.Anim.OneFoot.Exit.ToLoop (41088f69-90ad-9497-4108-8f6990ad9497).
It references CanOneFoot, CanGrabDouble, TweakLeftRight and TweakFrontBack;
the transition uses an8-tick DiffBlend. The separate unconditional return uses
a16-tick VelocityBlend. The expression itself has not been fully decoded.
These dependencies motivate a matched original-input catch comparison, not a
boolean write, forced transition, arbitrary catch timer or scoring claim.

The repeated-B Christ Air request was recognized, but the original family
remained Nose6 and OneFooterType stayed0. A subsequent Tailgrab from stale
family1 also selected the wrong family. Repeating that same Tailgrab/one-foot
binding after native family clearance selected footer8, advanced its original
Into/Loop/Exit and landed100. These are routing/state-context failures separate
from the airtime problem. No enum reset, forced chooser or substitute actor is
an acceptable repair. See the captures in rc3-native-high-footer-30.json.

Family handoff correction under development

The existing lease replaced its selected profile immediately on repeated B.
Airwalk's Nose hand could therefore remain asserted while Christ Air waited
for BS eligibility that this existing native chooser could not provide.
The original Into chooser does not choose every update; the surveyed source
and upstream/fork audit in native-grab-transition.md already distinguish its
native reset/exit from changing a blendspace endpoint. No separate reusable
family-handoff adapter was found in that audit.

The pending correction withdraws the old feet first, retains the old hand and
selector only while independently observed original footer tags remain active,
then withdraws both. It waits for readable IsGrabbing=false, CanGrab=false and
GrabType=None before applying the new profile through the existing producer.
The new native initialization bound starts at that observed admission; the
physical press timestamp is unchanged. The old cleanup remains bounded by
the existing initialization timeout, owner/flight and heartbeat guards.
Same-family BS/Melon/Method changes retain their current path. No new actor,
forced chooser, enum reset, score write or physics change is introduced.

Independent hardware acceptance still requires the repeated physical binding
to show the old catch/exit, native family clearance, the correct Christ Air
Into/Loop/Exit, hold/release and landing. Direct R3+B Christ Air is a separate
control. A first grab from unrelated stale family state is also a separate
unresolved case; this correction must not claim to clear arbitrary game state.
