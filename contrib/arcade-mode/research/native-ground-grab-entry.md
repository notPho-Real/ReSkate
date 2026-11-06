# Native ground grab entry

RC3 implements four ordinary-ground bindings through the original controller
producer and the game's Ground Grab StateFlow. No actor, identity, score,
physics state, pose, velocity or animation phase is written.

| Native identity | Arcade control | Original input | Primary original sequence |
|---|---|---|---|
| Enum.GrabType 53, FS Ground Grab | R3+Right+B | LT | 24316, `seq_c_proto_onb_grab_ground_fs_loop` |
| Enum.GrabType 54, BS Ground Grab | R3+Left+B | RT | 3927, `seq_c_proto_onb_grab_ground_bs_loop` |
| Enum.GrabType 55, Double Ground Grab | R3+B, neutral LS | LT+RT | 8921, `seq_c_proto_onb_grab_ground_dbl_loop` |
| Enum.GrabType 56, Coffin | R3+Down+B | Both hands, then original A+X+LT+RT | 6986, `seq_c_proto_onb_coffin_loop` |

Hold B to maintain the input; R3 can be released after entry. Release B to
withdraw every supplied input. Normal B remains masked from native braking.
Direction selects only at the physical B press. These controls require riding,
native state100 and no competing manual, flip, grind, footplant or trigger owner.
They never repurpose a balance or airborne request.

## Original source path

The pinned build's Gamepad producer EBX34108/RES7573 computes Input_Coffin
from A, X and both fully pressed triggers. GroundTricks_Coffin condition
EBX23975/RES14279 additionally requires Bool.GP.Grab.LeftHand and RightHand.
GroundTricks_Coffin action EBX16431 owns the original RideCoffin intent.
StateFlow Ground.Coffin.Into EBX31781 enters from that intent, then reaches
Ground.Coffin.Loop EBX775. The loop's original RideCoffin condition and
transition EBX2619 lead to its native exit. The skater intent tree's CoffinExit
branch EBX7485/RES5944 observes the original Is.Coffin state.

Ordinary Ground.Into EBX4952 and Ground.Loop EBX23569 use
Bool.GP.CanGroundGrab; matching is owned by the original hand/family expression.
Their switch and exit transitions remain native. Source records include
regular/fakie and turning blendspaces; Arcade does not replace their selection.
The sequence names above identify the primary straight loop, not all native
stance and turn variants.

The adapter first supplies only hands. Coffin's feet are withheld until a later
native physics step reports CanGroundGrab and both GP hands. This is important:
A+X without the original grab context can request a hippy jump. No fallback is
used. A missing entry expires after1200ms. Readback older than150ms, a backward
clock/step, another owner, leaving state100, mode/focus loss or B release all
withdraw controls. Heartbeat withdrawal also occurs in the client tick.

The shared ground-input reader uses GUID-checked original game-state assets.
The existing ActorController hooks also observe the four original loop body
actors, in FS/BS/Double/Coffin order:

- `44c08c5d-685f-5fdf-44c0-8c5d685f5fdf`
- `8a7797d8-9d9c-83a6-8a77-97d89d9c83a6`
- `ef77c2be-aa08-41af-ef77-c2beaa0841af`
- `e2ffe68e-3c21-2199-e2ff-e68e3c212199`

Acknowledgment requires the matching actor to initialize in the same local
skater's state view, then advance with positive delta through the original
evaluate call. The request serial, actor object, GUID and context must still
match afterward. Hand or Coffin flags alone cannot remove the entry deadline.
These hooks only observe; they do not initialize or advance actors themselves.

Footplants and ground grabs retain separate readiness and leases, and cannot
acquire concurrently. Footplant settings remain scoped to footplants; ground
grabs do not alter gameplay options.

## Evidence and remaining work

RC3-11 source `01ecfd50aaf66e5e1bcccfbe750efec24f9ee5b0` was tested on
Mega Park in capture
`2de0529e039c66ae5852c09772661ae3100cd21853c92969fc81ea67478727c1`.
Ground Double and Coffin initialized and positively advanced their exact
original loop actors and acknowledged SDK input release. State remained100
and board height remained below0.029m for both: no jump impulse was observed.
Native exit actors and scoring remain unverified. Ground FS and BS reached
their native hand/family states but did not acknowledge the expected actors;
they remain failed animation-acceptance cases, not successful entries.

D-pad comparison `069cd6559f01603dbf3fe06f443cb01b97ad656fb4a3bac6521d08e2bce4b069`
then acknowledged exact FS and BS loop actors and SDK release while remaining
in ground100. Repeating the analog selection afterward failed again in capture
`40418a51b10f65b88a1890bb0042f6c03b862001bd0eb00276956f42a22cbb29`.
The original ground loop blendspaces contain separate turning actors; the
current observer recognizes only the four primary straight actors. Steering
selection versus prior state is now isolated, but the actual turning actors
must still be observed before extending acknowledgment. The D-pad result does
not establish reliable analog hold or full stance/turn coverage.

Status: IMPLEMENTED — GAMEPLAY PENDING. The tests exercise production input
recognition, session composition and the bounded native input owner at
30/60/120Hz, including ten-second holds and negative controls. Injected state
observations do not prove native animation or scoring. Runtime animation records
contain initialization and positive advancement evidence when observed; end
records distinguish that acknowledgment from still-unverified scoring and exit
acceptance. The capture above covers only the explicitly observed actors.

Hardware acceptance must record original actor initialization and positive
advancement for each binding, hold, B release, cleanup and native history/score.
Coffin must enter without a jump impulse. Also check both stances, ordinary
ground B/X, RT revert, manual entry, auto-push, board toggle and Arcade OFF.
Airborne Coffin, Coffin fingerflips, grab changes while held and landing into a
ground grab are separate contexts still awaiting an adapter and acceptance.
