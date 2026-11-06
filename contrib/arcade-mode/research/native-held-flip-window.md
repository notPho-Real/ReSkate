# Original held-flip preparation window

RC3-24 Mega Park capture
`01174e8f11bbbdc1aef670f9f741764a75d7629cba65a5051cd271d83279b96f`
compared physical X repeats100ms and250ms after the first X. The100ms trial
measured1.991295 board turns, an original held loop, extra-count1 and landing100.
The250ms trial measured1.991632 turns but landed504; no original loop
advancement receipt or extra-count1 accompanied that second trial. Rotation
alone is insufficient acceptance. The test stopped before its350ms and
post-landing controls, which remain untested in this capture.

For the failed trial, first X was scheduled at0.52s after A. Original
PrepareHeldFlip became true at0.696s and false at0.7599s. The second physical
X at0.77s therefore arrived after that window. The adapter started a stick
gesture anyway because it was inside the separate300ms Arcade input history.
It withdrew after the existing200ms admission deadline. Native held state
later appeared without the corresponding HeldFlip intent or completed count.

Existing source inventory identifies the original EAT condition23124 and
action10792. They require PrepareHeldFlip and the matching native held gesture;
the action owns HeldFlip intent. Neither an input-history match nor another
board rotation proves admission through that action. Searches of the frozen
upstream Extension tree and surveyed native throwdown fork sources found no
equivalent repeated-button adapter to reuse. This is a routing/timing defect
in the existing Arcade adapter, not missing native animation content.

The correction remembers the first observed preparation window for the current
native flip. Once a fresh matching observation no longer reports preparation,
new repeated-X gestures are retired for that flip. A later flag cannot reopen
that window. Already-started repeats retain their original native admission,
count observation and catch behavior, including preparation before the window
opens. Landing, reset and a new accepted base flip clear this history. No
native flag, count, animation clock, velocity or scoring value is written.

The physical-input regression failed before the correction and passes at
30/60/120Hz for Kickflip and Heelflip. Early repeats and existing owner/landing
rejections still pass. Required hardware follow-up:100/150ms repeats retain a
true double and native catch;250ms after a closed window preserves the single
flip;350ms and post-landing input cause no extra launch. Numeric scoring,
exact trajectory comparison and unseen-window sampling cases remain pending.

RC3-28 hardware follow-up on source `2c2ce125d0a71d01e0decd73c8ec6ba4688660f7`
passed all six bounded Mega Park trials at native 30 Hz. Single X measured
0.991484 turns; 150 ms and 100 ms repeats measured 1.991346 and 1.991373 turns.
Both doubles initialized and advanced the original held loop and reached native
extra-count 1. The 250 ms, 350 ms and post-landing controls retained one turn
and count 0. Every trial returned to ground state 100 without unknown or bail
samples. In particular, the previous 250 ms bail no longer reproduced.
The exact captures and observations are in `rc3-native-repeat-window-28.json`.
These observations establish native rotations and catch for these timings;
numeric scoring and exact trajectory equality remain unverified.
