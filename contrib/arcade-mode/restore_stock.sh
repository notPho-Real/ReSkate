#!/usr/bin/env bash
# Restore the first pre-development DLL without consuming its backup.
set -euo pipefail
fail() { printf '%s\n' "$*" >&2; exit 1; }
[[ $# == 1 ]] || fail "Usage: $0 /path/to/Skate"
game_dir=$(cd -- "$1" && pwd)
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
[[ -f "$backup" && ! -L "$backup" ]] || fail "No regular stock backup found: $backup"
cp -p -- "$backup" "$lock/ReSkate.dll"
mv -f -- "$lock/ReSkate.dll" "$game_dir/ReSkate.dll"
printf '%s\n' 'Stock DLL restored; backup retained. You may re-enable “Install ReSkate updates”.'
