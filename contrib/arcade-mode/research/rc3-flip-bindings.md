# RC3 repeated flip controls — partial native acceptance

| Control after A / Ollie | Requested native action |
| --- | --- |
| X | Existing single Kickflip |
| X, release X, X within 300 ms | Double Kickflip in the same flight |
| Left + X, release X, X within 300 ms | Double Kickflip in the same flight |
| Right + X, release X, X within 300 ms | Double Heelflip in the same flight |

The direction chooses the first trick; the second press extends that trick.
Holding X does not count as another physical press. Native PrepareHeldFlip and
the original held gesture must admit the repeat. If the window has closed, or
the skater lands, the repeat cannot start another Ollie or disconnected flip.
No triple binding is enabled by this change.

The second press supplies the game's authored held-flick input. The original EAT
and animation own looping, catch, count, landing and scoring. Native held count
one means one additional rotation, as corroborated by the unchanged RC2 stock
control captures in `native-held-flip-rc2-controls.json`.

RC3-17 passed its Windows runtime build and exact-source integration gates.
On Mega Park at30Hz, the first repeated-X attempt after launch produced two
measured board turns with90ms between presses; a150ms repeat and Double Heelflip
also produced two turns. All three observed the correct initialized/advancing
native held loop, extra-count1, catch and ground return. The single-X control
retained one turn and extra-count0. The complete eligible window, higher-rate
gameplay and scoring ownership remain pending. Portable tests use simulated
native observations. See `rc3-gameplay-status.md` for capture hashes and limits.

RC3-24 repeated the single-X and150ms double-X controls on Mega Park. They
measured0.991478 and1.991346 native board turns, respectively, with original
held-loop advancement, catch and ground return. The exact current-build receipt
is in `rc3-native-repeated-flip-observations.json`. This does not extend the
accepted timing window or establish numeric scoring.
