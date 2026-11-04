# RC3 repeated flip controls — native acceptance pending

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

RC3 X/X execution still needs a Windows runtime build and native actor/rotation
acceptance. Portable tests use explicit simulated native observations and are
not a substitute for that capture. See `rc3-gameplay-status.md` for current limits.
