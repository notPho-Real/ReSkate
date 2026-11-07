# Native grab source inventory

`native-grab-source-catalog.json` preserves original identities from four native
namespaces and a hash-checked inventory of 300 original grab sequences. This is
not a claim that all declared tricks are executable in Arcade Mode.

| Namespace | Non-None declarations | Evidence in this audit |
|---|---:|---|
| Enum.GrabType | 90 | Original identity declarations; full selector/scoring relation unresolved |
| Enum.DetailedGrabType | 38 | Original pose declarations; 37 have tagged sequences in this inventory |
| Enum.OneFooterType | 13 | Original declarations and one-footer assets; transition closure unresolved |
| Enum.FingerflipType | 30 | Original declarations and fingerflip assets; transition closure unresolved |

These counts overlap and must not be added to report a number of distinct tricks.
The three pilot entries—BS, BS_Melon and BS_Method—link to their independently
observed native actors at measured 30 Hz. Their release/scoring caveats remain
visible. Every other entry has no verified Arcade binding and no native gameplay
acceptance claim.

The audit reads `IntegerGameStateTagAsset` references to the original
`Enum.GS.DetailedGrabType` asset
(`92445c52-d077-a5e5-9244-5c52d077a5e5`) and joins them to their original sequence
and actor GUIDs. It does not assign identity by a filename alone. Original
intermediate blend poses can emit a different detail from the filename, so
native selection, initialized actor, advancing clock and tag must be considered
together.

One concrete anomaly is level resource 15021,
`seq_c_proto_onb_inair_grab_seatbelt_bs_shifty_loop`: its original detail tag is
24 (`Nose_BS_Shifty`), whereas the declaration `Seatbelt_BS_Shifty` is 17. The
audit found no sequence emitting 17 in these 300 assets. This does not authorize
rewriting the tag, substituting another sequence, or claiming a correct scored
Seatbelt variant. The unresolved relationship is retained explicitly.

The native declarations extend well beyond the original Arcade table: CrossBone,
TuckKnee, FrigidAir, Seatbelt, Crail, Stale/Weddle variants, Double Grab variants,
Superdude, Tailwalk, DogPiss, Coffin and numerous fingerflip/varial transitions.
Their existence is established separately from legal airborne entry, hold,
release, stance and chaining behavior. Requested labels such as Indy, Madonna,
Sacktap or One Foot Japan must not be assigned to one of these assets merely
because it looks similar.

Stage A remains the runtime expansion gate. The remaining evidence is a clean
original-control release comparison, confirmed native release/landing behavior,
local-owner scoring attribution and native tests at the remaining rates.
The two original-control Method comparisons bailed and therefore do not settle
release behavior. The protected native flip bridge is unchanged by this audit.
