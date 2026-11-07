# Ordinary grab actor pass on Mega Park

Source `5c841c20c82b5782f6ff4ac861aa0e9683159d97`, DLL SHA-256
`358ac7603dad377ff939e593ba44b815ad75f911383773b13c5bafc44fdf1d7a`.
The current evidence ledger now contains exact original Loop initialization and
positive advancement for all 44 ordinary profiles in the 57-profile selector.
The other 13 profiles are one-footers and retain separate acceptance requirements.
This does not account for all native transitions or demonstrate scoring acceptance.

The November 6 pass added 28 ordinary-profile receipts through the actual
Skater > Cheats > R3+B grab selector. Each trial used a normal A Ollie, neutral
left stick, held R3, B at 620 ms, and physical release at 1100 ms. The selector
screen was captured for each choice. The independent observer recorded the
native family/detail, exact initialized actor and positive animation delta,
then GrabType zero with native GrabExit before a ground100 return. No bail
occurred between that entry and landing in these 28 trials.

The added profiles are Crail, Seatbelt, Stale, Weddle; Tail BS/FS Shifty and
Tail Nosebone; Nose BS/FS Shifty, Nose Tailbone, alternate Nosegrab and Crail FS
Shifty from Nose; Seatbelt FS Shifty; Crail FS/BS Shifty and alternate Crail BS
Shifty; Stale Nosebone, Tailbone and all three Tuck Knee poses; Weddle Japan,
alternate Weddle, Weddle Nose/Tail; Rocket Air, Seatbelt BS Shifty and Double Grab.

Each machine-readable receipt in `rc3-native-grab-observations.json` records
its raw and compressed capture hashes, exact native actor, source/DLL, binding,
request serial/episode and entry/release/landing times. Shared batch captures
are retained once in compressed form. The generated `rc3-grab-coverage.json`
joins these observations to original declarations without inventing identities.
Seatbelt BS Shifty still emits the game's Nose BS Shifty detail24; declaration17
is not written or substituted.

These are short regular-stance Ollie trials. Extended holds, changed stances,
exact Exit actors, final pose weights, numeric scoring/combo attribution and
owner visual acceptance remain unverified. The earlier one-footer failures,
airborne Coffin handoff and unresolved enum-only identities remain visible in
the coverage report. No overall feature-complete status is assigned.
