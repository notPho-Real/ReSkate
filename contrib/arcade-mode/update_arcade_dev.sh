#!/usr/bin/env bash
# Install a supplied DLL; artifact downloading is intentionally deferred.
set -euo pipefail
fail() { printf '%s\n' "$*" >&2; exit 1; }
[[ $# == 2 ]] || fail "Usage: $0 /path/to/Skate /path/to/development/ReSkate.dll"
game_dir=$(cd -- "$1" && pwd)
source_dll=$2
[[ -f "$source_dll" ]] || fail "Development DLL not found: $source_dll"
[[ -f "$game_dir/Skate.exe" && -f "$game_dir/ReSkateLauncher.exe" ]] || fail "Expected Skate.exe and ReSkateLauncher.exe in game directory."
[[ -f "$game_dir/ReSkate.dll" && ! -L "$game_dir/ReSkate.dll" ]] || fail "Expected a regular, non-symlink ReSkate.dll."
command -v pgrep >/dev/null || fail "pgrep is required to check whether the game is running."
# Match executable names in Proton command lines, including the longer launcher name.
if pgrep -fi '(^|[/\\ ])(Skate|ReSkateLauncher)\.exe([[:space:]]|$)' >/dev/null; then
    fail "Close Skate and ReSkateLauncher before replacing the DLL."
else
    [[ $? == 1 ]] || fail "Could not check running processes."
fi
lock="$game_dir/.arcade-mode-install.lock"
mkdir -- "$lock" 2>/dev/null || fail "Another install/restore is running (or a stale $lock remains)."
trap 'rm -f -- "$lock/ReSkate.dll" "$lock/stock.dll"; rmdir -- "$lock"' EXIT
backup="$game_dir/ReSkate.dll.arcade-stock.bak"
cp -- "$source_dll" "$lock/ReSkate.dll"
# Reject a ZIP, HTML download, truncated header, or wrong architecture before touching stock.
python3 - "$lock/ReSkate.dll" <<'CHECK_PE'
import pathlib, struct, sys
p = pathlib.Path(sys.argv[1])
with p.open('rb') as f:
    head = f.read(64)
    if len(head) != 64 or head[:2] != b'MZ':
        sys.exit('Not a Windows PE DLL')
    offset = struct.unpack_from('<I', head, 0x3c)[0]
    if offset < 64 or offset + 26 > p.stat().st_size:
        sys.exit('Invalid PE header offset')
    f.seek(offset)
    pe = f.read(26)
    if (pe[:4] != b'PE\0\0' or struct.unpack_from('<H', pe, 4)[0] != 0x8664
            or not struct.unpack_from('<H', pe, 22)[0] & 0x2000
            or struct.unpack_from('<H', pe, 24)[0] != 0x20b):
        sys.exit('Expected an x64 PE32+ DLL')
CHECK_PE
if [[ -L "$backup" || ( -e "$backup" && ! -f "$backup" ) ]]; then
    fail "Backup path is not a regular file: $backup"
fi
if [[ ! -e "$backup" ]]; then
    cp -p -- "$game_dir/ReSkate.dll" "$lock/stock.dll"
    mv -- "$lock/stock.dll" "$backup"
fi
chmod 644 "$lock/ReSkate.dll"
mv -f -- "$lock/ReSkate.dll" "$game_dir/ReSkate.dll"
printf '%s\n' "Installed Arcade Mode development DLL. Stock backup: $backup"     'Keep “Install ReSkate updates” OFF while testing.'
