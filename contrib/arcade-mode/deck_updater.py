"""Permanent Deck update/restore buttons; only consume tested releases from this fork."""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import urllib.error
import urllib.request
import zipfile

import deck_installer as installer

RELEASES = "https://github.com/notPho-Real/ReSkate/releases/download/"
CHANNEL = RELEASES + "arcade-deck-latest/channel.json"
MAX_DOWNLOAD = 64 * 1024 * 1024


def download(url, limit=MAX_DOWNLOAD):
    request = urllib.request.Request(url, headers={"User-Agent": "ReSkate-Arcade-Deck-Updater"})
    with urllib.request.urlopen(request, timeout=45) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError("The update download is unexpectedly large.")
    return data


def validate_channel(value):
    if (not isinstance(value, dict) or value.get("schema") != 1
            or not isinstance(value.get("commit"), str)
            or not isinstance(value.get("sha256"), str)
            or not re.fullmatch(r"[0-9a-f]{40}", value.get("commit", ""))
            or not re.fullmatch(r"[0-9a-f]{64}", value.get("sha256", ""))):
        raise ValueError("The update information is invalid. Your installed version is unchanged.")
    expected = RELEASES + "arcade-build-" + value["commit"] + "/Install-ReSkate-Arcade.desktop"
    if value.get("url") != expected:
        raise ValueError("The update did not point to a versioned Arcade Mode release.")
    return value


def unpack_download(data, expected_hash):
    if hashlib.sha256(data).hexdigest() != expected_hash:
        raise ValueError("The download checksum failed. Your installed version is unchanged.")
    text = data.decode("utf-8")
    payload = text.split("# ARCADE_PAYLOAD\n", 1)[1]
    archive = zipfile.ZipFile(io.BytesIO(base64.b64decode("".join(line[1:] for line in payload.splitlines()), validate=True)))
    names = archive.namelist()
    if len(names) != 4 or set(names) != {"ReSkate.dll", "installer.py", "updater.py", "manifest.json"}:
        archive.close()
        raise ValueError("Unexpected archive members")
    if sum(info.file_size for info in archive.infolist()) > MAX_DOWNLOAD:
        raise ValueError("The unpacked update is unexpectedly large.")
    return archive


def verified_release(fetch=download):
    channel = validate_channel(json.loads(fetch(CHANNEL, 65536)))
    with unpack_download(fetch(channel["url"]), channel["sha256"]) as bundle:
        manifest = installer.validate_manifest(json.loads(bundle.read("manifest.json")), channel["commit"])
        dll = bundle.read("ReSkate.dll")
        if installer.digest(dll) != manifest["sha256"]:
            raise ValueError("Source DLL hash mismatch")
        installer.validate_dll(dll)
    return manifest, dll


def update(game, directory):
    installer.ensure_stopped()
    channel = validate_channel(json.loads(download(CHANNEL, 65536)))
    state_path = directory / "installed.json"
    if state_path.is_file():
        state = json.loads(state_path.read_text())
        dll = game / "ReSkate.dll"
        if (state.get("commit") == channel["commit"] and dll.is_file()
                and hashlib.sha256(dll.read_bytes()).hexdigest() == state.get("dll_sha256")
                and installer.load_settings(game).get("updates") is False):
            return "Arcade Mode is already up to date. Start Skate as usual."
    data = download(channel["url"])
    with unpack_download(data, channel["sha256"]) as bundle:
        manifest = json.loads(bundle.read("manifest.json"))
        if manifest["commit"] != channel["commit"]:
            raise ValueError("The downloaded build does not match the update information.")
        # The verified package includes the latest installer/updater, so this button can evolve too.
        namespace = {"__name__": "arcade_update_install"}
        exec(compile(bundle.read("installer.py"), "installer.py", "exec"), namespace)
        namespace["install_bundle"](bundle, game, register_steam=False)
    return "Arcade Mode updated! Start Skate as usual. Your stock backup is safe."


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--restore", action="store_true")
    args = parser.parse_args()
    directory = Path(__file__).resolve().parent
    try:
        config = json.loads((directory / "config.json").read_text())
        game = Path(config["game"])
        if args.restore:
            if not installer.dialog("question", "Restore stock ReSkate and your previous update setting?",
                                    ("Restore", "Cancel")):
                return
            installer.change_installation(game)
            message = "Stock ReSkate restored. Use Update Arcade Mode whenever you want to switch back."
        else:
            # Keep a window visible while downloading, including when launched from Gaming Mode.
            progress = None
            if installer.shutil.which("zenity"):
                progress = subprocess.Popen(["zenity", "--progress", "--pulsate", "--no-cancel",
                    "--auto-close", "--title=" + installer.TITLE, "--text=Checking for an Arcade Mode update..."],
                    stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                message = update(game, directory)
            finally:
                if progress:
                    progress.communicate(b"100\n")
        installer.dialog("info", message)
    except (urllib.error.URLError, TimeoutError):
        installer.dialog("error", "Cannot reach the update server. Check Wi-Fi and try again. "
                         "Your installed build and stock backup have not been changed.")
    except Exception as error:
        installer.dialog("error", str(error))


if __name__ == "__main__":
    main()
