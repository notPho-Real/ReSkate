# Steam Deck: download, open, install

Download **Install-ReSkate-Arcade.desktop** from the fork's Arcade Mode development
pre-release. In Steam Deck Desktop Mode, open it from Downloads, accept KDE's
Run/Execute confirmation if shown, and press **Install**. Return to Gaming Mode
and launch Skate normally. Existing ReSkate must already work on your Deck.

No terminal, archive extraction, GitHub sign-in, administrator password, package
installation, or manual DLL copying. The file includes the verified Windows x64
DLL and the installer; it works offline after downloading. Keep the file to
switch back: open it again, choose **Restore stock**, and confirm.

The installer finds Steam's internal library, registered extra libraries, and SD
card libraries under /run/media. If no installation or multiple installations are
found, a folder picker asks which Skate folder to use. It never changes Steam
launch options or installs a launcher. A working ReSkateLauncher.exe, Skate.exe,
and ReSkate.dll must already exist beside each other.

It refuses a running game/launcher, preserves the first stock DLL in
ReSkate.dll.arcade-stock.bak, and atomically installs the development DLL. It sets
`updates: false` in the launcher's actual `ReSkateLauncher.settings.json`, preserving
unrelated settings. Restore puts back the stock DLL and the prior update preference.
The existing command-line helpers remain available for manual installs.

## Build and verification

The single .desktop contains a compressed, checksummed payload in comment lines.
KDE passes the launcher's own file location through `%k` as a separate argument;
no downloaded path is substituted into a shell command. Python verifies the
payload and DLL checksum, then runs the embedded installer. Zenity or KDialog
provides the normal desktop prompts. Steam Deck's Python 3 and pgrep are required.
Downloaded desktop launchers may require KDE's normal trust/execute confirmation.

`python3 -m unittest discover -s contrib/arcade-mode -p 'test_*.py' -v` tests
Steam/SD discovery, install/reinstall/restore, preference preservation, malformed
input, locks, symlinks, running-game refusal, write-failure rollback and executing
the generated launcher's embedded payload (including a filename with spaces).
The GUI and hardware still need a real Deck smoke test.

The packaging workflow pins the already-green Windows runtime build:
`dc15bac9feaa67d5520d6dabe5dc03f6ea25cf74`, Actions run `38114122001`.
This packaging change does not alter runtime code. The DLL manifest records its
source commit separately from the installer commit. The original Actions artifact
expires after 30 days; archive/re-pin a newly validated runtime before rebuilding
after expiration. The published self-contained installer remains usable offline.

To package a verified downloaded DLL manually:

```sh
python3 contrib/arcade-mode/package_deck_installer.py /path/to/ReSkate.dll FULL_DLL_COMMIT_SHA /path/to/Install-ReSkate-Arcade.desktop
```

The development pre-release is only in notPho-Real/ReSkate. The original v0.1
runtime artifact remains available in Actions; the pre-release adds a public direct
installer download so a Deck does not need GitHub authentication to fetch it.
