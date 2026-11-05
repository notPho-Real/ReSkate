# Native Rocket entry observation

The existing source inventory resolves Rocket Into to resource31585,
`Animation/Dingo/seq_c_proto_onb_grab_rocket_into`, sequence
`f16db680-fa66-411e-f16d-b680fa66411e`. Its original body ActorControllerAsset is
`8183390e-8e15-4aaa-8183-390e8e154aaa`, with a 20-tick authored duration.
This is separate from the Rocket loop already in the grab profile.

On RC3-09, source `b55f5fe01bfaae82e78bd58b204fbc64ba36bb87`, the Mega Park
original-control comparison reached native GrabType10/DetailedGrabType38
with simultaneous hand input. Adding the second hand after Nosegrab stayed
in family6. The original-control capture's uncompressed SHA-256 is
`4feebefad94a84df83ea9ada8306b1a3212356e932958e2c048b1ad74067c536`.

The existing Arcade selector, Rocket Air followed by centered R3+B, also
reached family10/detail38 and returned to ground. Capture SHA-256:
`5fbfea209f43525aebdd7f358e830f185468012408a94437b08a45d9aa753b8b`.
No exact actor acknowledgment was recorded. Up+B,B instead retained Nosegrab
in a separate trial; this transition remains unverified.

The observer now recognizes the exact original Rocket Into actor as well as
the loop, using the existing owner/view, initialization, positive advancement
and native detail checks. It labels the receipt as Into. This neither selects
an animation nor forces its duration, pose, counter, physics or score. As with
the existing footer entry observer, verified entry advancement can acknowledge
a pending physical release without requiring the later loop. Hardware
initialization, advancement, native exit and scoring acceptance remain pending.
