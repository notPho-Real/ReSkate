# Arcade Lab and Deck updates

Start with [STATE.md](STATE.md) for persistent engineering context and
[LAB_PROTOCOL.md](LAB_PROTOCOL.md) for the tested Mac lab workflow. Double-click
**Install Arcade Lab.command** to run offline self-tests, install the controller
under HOME, optionally pair, and run read-only preflight. This never deploys.
Use **Deploy Latest Arcade Mode.command** separately when ready for hardware testing.

The existing standalone Deck package can install the **Update Arcade
Mode** and **Restore Stock ReSkate** Steam library entries and verify them there.
The entries are submitted to Steam by the installer; their appearance and
controller operation still need hardware verification. Desktop Mode is not a
required part of this user's acceptance path.

After setup: close Skate, open **Update Arcade Mode**, and wait for the success
message. Start Skate normally. No browser, terminal, GitHub account, repeated
installer download, or manual DLL copying. Restore Stock works offline. Updating
after restoring stock switches back to Arcade Mode without overwriting the backup.
The updater needs Wi-Fi only when checking/downloading a new build. It can update
its own installer/updater code as well as the DLL.

## SSH deployment from this Mac

Default host: `deck@steamdeck.local`. Pairing and the trusted external runner are
working. See STATE.md for the current installed build and hardware checkpoint.
The Deck must be awake and reachable from the Mac. A sleeping/unreachable Deck
cannot be installed remotely. No need to switch to Desktop Mode for SSH.

Double-click `Pair-Steam-Deck.command` on the Mac once and enter the Deck's local
Linux password into SSH's terminal prompt. It creates a dedicated key in this
HOME at `~/.local/share/reskate-arcade-lab/`, and adds only that public key to the Deck.
Pairing stops there and never installs a DLL or buttons. The password is never stored.
Subsequently `Deploy-to-Steam-Deck.command` deploys without a password. Codex can
run the same helper. If mDNS fails, pass `--host deck@LOCAL_IP`; if multiple game
folders are found, pass `--game /exact/Skate/folder`. Host keys remain checked;
changed keys are not silently accepted. Keys and pairing state are not committed.

Steam import uses its Linux add-non-Steam URL and sentinel mechanism, leaving
Steam to write its own configuration. It never edits shortcuts.vdf or restarts
Steam. The installer records import requests to avoid duplicates. A request is
not proof that the current Steam version imported the shortcut: verify both
buttons in the Deck library during SSH/on-device acceptance. Desktop buttons
remain usable if Steam import is unavailable. The mechanism is documented by the
[steam-add-nonsteam-game project](https://github.com/GloriousEggroll/steam-add-nonsteam-game).

## Safe update and publication path

The Windows workflow builds the current arcade-mode DLL and runs the model test.
Only after Windows, Linux and hosted macOS lab gates pass does its Linux job package the Deck installer, publish
an immutable `arcade-build-COMMIT` development release in **notPho-Real/ReSkate**,
and advance the `arcade-deck-latest/channel.json` pointer. An older build finishing
after a newer branch commit cannot advance the pointer. Nothing publishes from
main or upstream. The separate Deck test workflow supplies fast Linux feedback.

The updater validates the channel schema, pinned fork URL, commit, installer SHA256,
embedded DLL SHA256 and x64 PE header before installing. No Actions tokens or
expiring artifact URLs are required on the Deck. HTTP/checksum failures preserve
the installed build. A transient channel replacement failure can be retried.

The first stock DLL stays in `ReSkate.dll.arcade-stock.bak`. The installer disables
`updates` in `ReSkateLauncher.settings.json`, preserving other settings. Restore
puts back stock and the previous update preference. Game/launcher processes must
be closed; installation uses the existing lock and atomic DLL replacement. Steam
launch options are unchanged. Permanent scripts/config live in
`~/.local/share/reskate-arcade` and need no root access.

## Tests and remaining hardware verification

`python3 -m unittest discover -s contrib/arcade-mode -p 'test_*.py' -v`

Tests cover Steam/SD discovery, install/reinstall/restore, preference preservation,
invalid files, locks, running games, write rollback, desktop payload execution,
update-to-latest, already-current detection, restore-then-update, failed downloads,
checksum mismatch, wrong commit and wrong download origin.

Steam library import and its controller operation still need on-device acceptance.
The installed development DLL has native Y rail engagement verified in the
50-trial fixed-rail matrix, plus repeatable native Ollie and Kickflip exits.
Grind/manual meters follow the actual balance model, with full-size Deck captures
and native physical balance-failure evidence. Automatic skating, camera routing,
directional manuals and shoulder rotation retain their existing acceptance scope.
The complete named grind table, monetary combo scoring and final mixed gameplay
acceptance remain incomplete. Grabs are deferred; their implementation is preserved.
The known revert recipe has native FS/BS confirmations but remains experimental.
Read STATE.md and the native-method matrix for source-specific evidence and limits.
