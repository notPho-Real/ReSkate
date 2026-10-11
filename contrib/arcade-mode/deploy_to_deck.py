"""Mac -> Deck setup/deploy, using the same verified public update channel."""
import argparse
import io
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import zipfile

import deck_updater as updater

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = ROOT / "tools/arcade-deck"


def ssh_arguments(host, pair=False):
    args = ["ssh", "-o", "ConnectTimeout=10", "-o", "StrictHostKeyChecking=accept-new",
            "-o", "UserKnownHostsFile=" + str(PRIVATE / "known_hosts")]
    key = PRIVATE / "id_ed25519"
    if key.exists():
        args += ["-i", str(key), "-o", "IdentitiesOnly=yes"]
    if not pair:
        args += ["-o", "BatchMode=yes"]
    return args + [host]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="deck@steamdeck.local")
    parser.add_argument("--pair", action="store_true", help="one-time interactive SSH key pairing")
    parser.add_argument("--game", help="optional exact Skate folder on the Deck")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+@[A-Za-z0-9_.:-]+", args.host):
        parser.error("Use user@hostname or user@IP")
    PRIVATE.mkdir(parents=True, exist_ok=True)
    if args.pair:
        key = PRIVATE / "id_ed25519"
        if not key.exists():
            subprocess.run(["ssh-keygen", "-t", "ed25519", "-f", str(key), "-N", "", "-C", "reskate-arcade-deck"], check=True)
        public_key = key.with_suffix(".pub").read_text().strip()
        # The password is entered directly into SSH's terminal prompt, never stored by this helper.
        remote = "umask 077; mkdir -p ~/.ssh; touch ~/.ssh/authorized_keys; " + \
            "grep -qxF " + shlex.quote(public_key) + " ~/.ssh/authorized_keys || printf '%s\\n' " + \
            shlex.quote(public_key) + " >> ~/.ssh/authorized_keys"
        subprocess.run(ssh_arguments(args.host, pair=True) + [remote], check=True)
    subprocess.run(ssh_arguments(args.host) + ["printf 'Deck connected\\n'"], check=True)
    channel = updater.validate_channel(json.loads(updater.download(updater.CHANNEL, 65536)))
    data = updater.download(channel["url"])
    with updater.unpack_download(data, channel["sha256"]) as bundle:
        if json.loads(bundle.read("manifest.json"))["commit"] != channel["commit"]:
            raise ValueError("Release manifest mismatch")
        payload = io.BytesIO()
        with zipfile.ZipFile(payload, "w", zipfile.ZIP_DEFLATED) as archive:
            for name in ("installer.py", "updater.py", "manifest.json", "ReSkate.dll"):
                archive.writestr(name, bundle.read(name))
    remote = r'''import io,json,os,sys,zipfile
from pathlib import Path
# Preserve only graphical-session variables needed to register Steam shortcuts over SSH.
for process in Path('/proc').glob('[0-9]*'):
    try:
        if process.stat().st_uid != os.getuid() or process.joinpath('comm').read_text().strip() != 'steam':
            continue
        for item in process.joinpath('environ').read_bytes().split(b'\0'):
            name,sep,value=item.partition(b'=')
            if sep and name in (b'DISPLAY',b'XAUTHORITY',b'DBUS_SESSION_BUS_ADDRESS',b'XDG_RUNTIME_DIR',b'WAYLAND_DISPLAY'):
                os.environ[name.decode()]=value.decode()
        break
    except (OSError,UnicodeError):
        pass
bundle=zipfile.ZipFile(io.BytesIO(sys.stdin.buffer.read()))
namespace={'__name__':'arcade_remote_setup'}
exec(compile(bundle.read('installer.py'),'installer.py','exec'),namespace)
games=[Path(sys.argv[1])] if len(sys.argv)>1 else namespace['find_games']()
if len(games)!=1:
    sys.exit('Expected one Skate install; use --game with its folder. Found: '+str(games))
namespace['install_bundle'](bundle,games[0])
print('Installed Arcade Mode and persistent update/restore buttons in '+str(games[0]))
print('Steam shortcut imports requested; verify Update Arcade Mode in the Steam library.')
'''
    command = "python3 -c " + shlex.quote(remote)
    if args.game:
        command += " " + shlex.quote(args.game)
    subprocess.run(ssh_arguments(args.host) + [command], input=payload.getvalue(), check=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print("Deck setup could not finish: " + str(error), file=sys.stderr)
        sys.exit(1)
