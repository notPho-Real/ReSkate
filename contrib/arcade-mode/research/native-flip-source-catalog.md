# Native flip source audit

This is a source inventory, not gameplay acceptance or a Stage B implementation
claim. Stage A's three original grab actors have executed at measured 30 Hz, but
release/scoring acceptance remains open. No runtime bridge or control binding is
changed by this audit.

`native-flip-source-catalog.json` records original identities, namespace-qualified
IDs, sequence references, native contexts, existing Arcade mappings and gaps.
The inventory contains 32 ordinary flip identities, including nollie variants,
and 16 separately selected underflip identities. Other entries in the ordinary
enum are launch helpers, wallies or special states; they are not counted as
additional ordinary flips. The enum also declares underflip names in its own
namespace; those declarations are retained separately from executable underflip
selector IDs.

## Evidence and method

The installed executable is pinned to SHA-256
`fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9`.
The read-only archive audit extracted 96 authored gestures, 369 sequence/controller assets,
and 31 relevant motion/selection database assets. Each compressed record was
verified against its original manifest SHA-1 before decoding. Original binary
assets remain local and are not included in the repository.

`OnBoardRightGestureType` is config EBX 2820, GUID
`12c41540-7f6a-9e06-12c4-15407f6a9e06`, with 59 declarations including None.
It is the actual enumeration referenced by `EnumGS.Intent.FlipTrick` and
`EnumGS.Anim.CurrentFlipTrick`. `Enum.Gesture` is an unrelated emote enumeration
and was excluded. `OnBoardUnderflipGestureType` is shared EBX 2816, GUID
`17d0dace-cbf5-1fdb-17d0-dacecbf51fdb`, with 16 non-None declarations.
Its request asset is `EnumGS.Intent.UnderflipType`, GUID
`1fcdb5a8-4378-184a-1fcd-b5a84378184a`.

Gesture IDs are read through each resource's actual `AnimationEnum` instance
reference. The first integer in a file can instead belong to `OverwritingTricks`.
Underflip IDs are kept separate: underflip 10 is Flowerflip, whereas ordinary
10 is 360PopShuvit. Conflating these would produce a false catalog.

Seven original ContextDatabase assets link selector IDs to sequence GUIDs.
Their `FieldMap` identifies the selector byte column; `EntriesV` stores sixteen
lanes per vector, with a vector per byte column. The audit reads the original
bytes rather than round-tripping floating-point text, which loses packed NaN
payloads. It preserves masks, field columns, row numbers and wildcard rows.
Sequence names cross-check all 32 ordinary identities. Unresolved helper/Wallie
references remain explicit; they are not mapped to another animation.

Ordinary, light-catch, dark-catch and late selection databases have different
coverage. For example, native late entry includes only 16 ordinary identities;
its light/dark variants include eight. These are source restrictions, not
permission to use those routes in Arcade Mode.

## Concrete implementation gaps

| Native identities | Source status | Current Arcade gap |
|---|---|---|
| FSPopShuvit 7 / N_FSPopShuvit 22 | Original gestures and sequences present | No semantic/binding |
| FS360PopShuvit 13 / N_FS360PopShuvit 25 | Original gestures and sequences present | No semantic/binding; ordinary bridge admission also excludes them |
| FrontFoot_Impossible 37 / N_BackFoot_Impossible 39 | Original gestures and sequences present | No semantic/binding |
| Laserflip 14 / N_Laserflip 27 | Original gestures and sequences present | Existing semantic `heelflip_360` displays “360 Heelflip”; catalog preserves the native name |
| Sixteen underflips, including Hospitalflip and Flowerflip | Original gestures and three sequences each | Separate native owner/transition and nonconflicting bindings need verification |

Double/triple Kickflip, Heelflip and Impossible, and 540 Shove-It remain
unsupported in the current ordinary mapping. They have no separate ordinary
enum counterparts, but the native held-flip mechanism is present and must be
investigated before deciding their actual support. `TRICK_FINDINGS.md` already
identified this distinction from the native guide and composite scoring data.
In particular, `DoubleKickflip` in
`Enum.FingerflipType` belongs to Double Grab fingerflips; its name does not prove
an ordinary two-rotation Kickflip.

## Native held rotations and freestyle exits

An additional 47 original resources establish native held-flip Into/Loop/Exit
selection for 16 base IDs: 2, 3, 4, 7, 11, 14, 17, 18, 19, 22, 27, 29, 36, 37,
38 and 39. All three databases resolve to original assets in the sequence audit.
`native-held-flip-source.json` preserves these selection rows and 27 original
sequences with tags for `Float.Anim.TrickHeldFlipCount`.

The Kickflip wrapper (1705) loops its original subject (35099) from tick 16 to
170. Original StateFlow transitions and the EAT HeldTrick action own entry and
exit. The composite native trick record separately stores held-flip amount and
category. This is an existing native mechanism, not permission to increment a
counter, invent rotations, stretch an animation or bypass the EAT owner. Exact
counter units, physical repeat selection, native entry from a bridge-started
flip, catch timing and scoring still require verification.

The source also declares `CasperHeelflipOut` and `Casper360FlipOut` in
`Enum.FreestyleFlipTrickExitType`. Their original Casper exit database references
are catalogued with unresolved asset definitions and explicit freestyle context.
They cannot be treated as ordinary airborne flips.

Runtime expansion must follow the Stage A gate. It must preserve the existing
Ollie bridge, retain original physical press timing and avoid assigning a new
meaning to an already working binding. Repeated X requires proof of native
mid-flip reselection; the presence of a higher-tier ID alone is insufficient.
All newly audited entries remain unverified for native animation, landing and
scoring until independently captured through the compiled implementation.
