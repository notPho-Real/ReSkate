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
RC3-11 observer recognizes only the four primary straight actors. Steering
selection versus prior state is now isolated. The next observer revision adds
the six exact original turning actors below, with separate initialization and
advancement records for each blended actor. Hardware acknowledgment is still
pending; source membership alone is not animation evidence. The D-pad result does
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

## Turning actor observation

The FS, BS and Double ground loop blendspaces (resources23725,5604,5938)
reference these original sequences. Bounded original sequence metadata joins
identify their ActorController assets; no synthesized pose or animation is used.

| Ground family | Turn | Sequence resource | Actor GUID |
|---|---|---|---|
| FS | Left | 6580 | `7584f4d2-f4ab-9f20-7584-f4d2f4ab9f20` |
| FS | Right | 35124 | `eb7edd26-52cb-71b7-eb7e-dd2652cb71b7` |
| BS | Left | 33922 | `8645468e-a411-e756-8645-468ea411e756` |
| BS | Right | 27998 | `dba7155d-1180-ae8e-dba7-155d1180ae8e` |
| Double | Left | 35073 | `87321af5-93c3-725f-8732-1af593c3725f` |
| Double | Right | 13465 | `e7986fa6-c581-e5b6-e798-6fa6c581e5b6` |

All ten straight/turning actors remain restricted to their original ground
family, the current request serial, local owner and state100. One blended
actor cannot replace another actor's initialization or advancement record.
The observer does not change the1200ms entry deadline, inputs, StateFlow,
cleanup or scoring. Tests reject wrong-family and unknown actor identities.
The analog-versus-D-pad Mega Park capture must be repeated on the new build;
no turning sequence is promoted to accepted coverage from this source change.

## Original Into admission before the Loop

RC3-24 capture `c101d9eb786bb8f4429ae652b253a8b781516a4be9de6abd96398dec654e8b9c`
retested all four analog bindings on Mega Park. Double and Coffin acknowledged
their exact Loop actors. FS/BS stayed in100 and reached native family/hand
states, but expired without a Loop receipt. Their native hand/family changes
arrived at1.197s and1.064s respectively after trial start; B had been pressed
at0.100s. GrabType cleared around1.36s and1.34s despite B remaining held until
1.8s. This reproduces the earlier analog failure with the turning observer present.

The existing Ground.Into StateFlow node4952 delegates to the original ground
entry database12559. Its FS, BS and Double sequences6291,30264,17958 contain
body actors `669b57cf-d9b0-01b5-669b-57cfd9b001b5`,
`af0cacf0-bd53-1bed-af0c-acf0bd531bed` and
`b0e696f1-234d-be9a-b0e6-96f1234dbe9a`, each at instance1. Their authored lengths
are33,40,31 ticks. A1200ms deadline conditioned only on later Loop advancement
can withdraw input while the original entry still owns the animation. This is
an admission-observation/timing defect. The existing air-grab Into observer
provides the pattern; the frozen official upstream and existing fork survey
provide no equivalent Arcade ground input owner to substitute.

The adapter now recognizes these exact original Into actors through the same
local owner/view, initialization and positive-delta checks. A fresh advancing
Into after preparation, while CanGroundGrab is true, proves entry and retires
the unacknowledged-entry deadline. It does not claim a Loop, native hand tags,
an exit, a completed Coffin or scoring. Into and Loop receipts remain separate.
Existing initialized/advancing Loop acknowledgment is retained once observed.
Release, invalid ownership/readback, non-ground state, stale heartbeat, backward
step/clock and Arcade OFF still withdraw the input immediately.

The observed late-Loop regression failed before the change. Production-header
tests cover30/60/120Hz entry-to-Loop gaps plus missing actor, stale preparation,
Coffin exclusion and all existing cancellation guards; all20 targeted suites
pass. Exact-source Windows build and new native FS/BS Into/Loop/hold/release
acceptance remain required. No timeout increase, pose or scoring write was added.
