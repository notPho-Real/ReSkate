"""Offline Steam Deck installer. Launched from the generated self-contained .desktop."""
import hashlib
import base64
import fcntl
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import urllib.parse
import time
import uuid

TITLE = "ReSkate Arcade Mode"
SETTINGS = "ReSkateLauncher.settings.json"
BACKUP = "ReSkate.dll.arcade-stock.bak"
RECEIPT = ".arcade-mode-install.json"
JOURNAL = ".arcade-mode-transaction.json"
LOCK = ".arcade-mode-install.lock"


def safe_path(path):
    path = Path(os.path.abspath(path))
    for component in (path, *path.parents):
        if component.is_symlink():
            raise RuntimeError("Refusing a symlink in installation path")
    return path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def validate_manifest(value, commit=None):
    if (not isinstance(value, dict)
            or not isinstance(value.get("commit"), str)
            or not re.fullmatch(r"[0-9a-f]{40}", value["commit"])
            or not isinstance(value.get("sha256"), str)
            or not re.fullmatch(r"[0-9a-f]{64}", value["sha256"])
            or (commit is not None and value["commit"] != commit)):
        raise ValueError("Invalid source commit or DLL hash")
    return value


def dialog(kind, text, buttons=None):
    if shutil.which("zenity"):
        args = ["zenity", "--title=" + TITLE, "--width=500", "--no-markup"]
        if buttons:
            args += ["--question", "--ok-label=" + buttons[0], "--cancel-label=" + buttons[1]]
        else:
            args += ["--" + kind]
        result = subprocess.run(args + ["--text=" + text], check=False)
    elif shutil.which("kdialog"):
        args = ["kdialog", "--title", TITLE]
        if buttons:
            args += ["--yesno", text, "--yes-label", buttons[0], "--no-label", buttons[1]]
        else:
            args += ["--error" if kind == "error" else "--msgbox", text]
        result = subprocess.run(args, check=False)
    else:
        raise RuntimeError("No desktop dialog program found. Open this installer in Steam Deck Desktop Mode.")
    return result.returncode == 0


def valid_game(path):
    return all((path / name).is_file() for name in ("Skate.exe", "ReSkateLauncher.exe", "ReSkate.dll"))


def find_games(home=None, media=None):
    home = Path.home() if home is None else Path(home)
    media = Path("/run/media") if media is None else Path(media)
    libraries = {home / ".local/share/Steam", home / ".steam/steam", home / ".steam/root"}
    for steam in list(libraries):
        vdf = steam / "steamapps/libraryfolders.vdf"
        if vdf.is_file():
            for value in re.findall(r'"path"\s*"([^"\n]+)"', vdf.read_text(errors="replace")):
                libraries.add(Path(value.replace("\\\\", "\\")))
    libraries.update(media.glob("*"))
    libraries.update(media.glob("*/*"))
    standalone = home / "Games/ReSkate"
    found = {standalone} if valid_game(standalone) else set()
    for library in libraries:
        common = library / "steamapps/common"
        if not common.is_dir():
            continue
        for game in common.iterdir():
            if valid_game(game):
                found.add(game.resolve())
    return sorted(found)


def choose_game():
    games = find_games()
    if len(games) == 1:
        return games[0]
    text = ("Choose the Skate folder containing Skate.exe and ReSkateLauncher.exe."
            if not games else "More than one Skate installation was found. Choose the one to use.")
    if shutil.which("zenity"):
        args = ["zenity", "--file-selection", "--directory", "--title=" + text]
    else:
        args = ["kdialog", "--getexistingdirectory", str(Path.home()), "--title", text]
    result = subprocess.run(args, capture_output=True, text=True, check=False)
    if result.returncode:
        return None
    path = Path(result.stdout.strip()).resolve()
    if not valid_game(path):
        raise RuntimeError("That folder does not contain Skate.exe, ReSkateLauncher.exe and ReSkate.dll. "
                           "This installer needs your existing working ReSkate installation.")
    return path


def ensure_stopped():
    result = subprocess.run(["pgrep", "-fi", r"(^|[/\\ ])(Skate|ReSkateLauncher)\.exe([[:space:]]|$)"],
                            stdout=subprocess.DEVNULL, check=False)
    if result.returncode == 0:
        raise RuntimeError("Close Skate and ReSkateLauncher, then open this installer again.")
    if result.returncode != 1:
        raise RuntimeError("Could not check whether Skate is closed. Nothing was installed.")


def atomic_write(path, data):
    safe_path(path)
    fd, name = tempfile.mkstemp(prefix=".arcade-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        os.chmod(name, 0o644)
        os.replace(name, path)
        sync_directory(path.parent)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def sync_directory(path):
    fd = os.open(path, os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def preserve_backup(path, data):
    # Publish a completely written file with an exclusive hard link. Never replace
    # an existing backup, including when another process creates it concurrently.
    fd, name = tempfile.mkstemp(prefix=".arcade-backup-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as output:
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        try:
            os.link(name, path)
            sync_directory(path.parent)
        except FileExistsError:
            raise RuntimeError("Stock backup appeared concurrently; inspect before retrying") from None
    finally:
        os.unlink(name)


def recover_transaction(game):
    journal = game / JOURNAL
    if not journal.exists():
        return
    value = json.loads(journal.read_text())
    if set(value) != {"ReSkate.dll", SETTINGS, RECEIPT}:
        raise RuntimeError("Invalid recovery journal; manual inspection required")
    for name, encoded in value.items():
        path = game / name
        if encoded is None:
            path.unlink(missing_ok=True)
        else:
            data = base64.b64decode(encoded, validate=True)
            if not path.exists() or path.read_bytes() != data:
                atomic_write(path, data)
    journal.unlink()
    sync_directory(game)


def load_settings(game):
    path = game / SETTINGS
    value = json.loads(path.read_text(encoding="utf-8-sig")) if path.exists() else {}
    if not isinstance(value, dict) or ("updates" in value and not isinstance(value["updates"], bool)):
        raise RuntimeError("Launcher settings are not in the expected format. Nothing was installed.")
    return value


def encode(value):
    return (json.dumps(value, indent=2) + "\n").encode()


def validate_dll(data):
    if len(data) < 90 or data[:2] != b"MZ":
        raise RuntimeError("The bundled DLL is damaged. Download the installer again.")
    offset = struct.unpack_from("<I", data, 60)[0]
    if (offset < 64 or offset + 26 > len(data) or data[offset:offset+4] != b"PE\0\0"
            or struct.unpack_from("<H", data, offset+4)[0] != 0x8664
            or not struct.unpack_from("<H", data, offset+22)[0] & 0x2000
            or struct.unpack_from("<H", data, offset+24)[0] != 0x20b):
        raise RuntimeError("The bundled file is not an x64 Windows DLL.")


def change_installation(game, dll=None, commit=None, log_path=None):
    """Serialize both operations; preserve stock, unrelated preferences and rollback data."""
    game = safe_path(game)
    if dll is not None:
        validate_dll(dll)
        validate_manifest({"commit": commit, "sha256": digest(dll)})
    if not valid_game(game):
        raise RuntimeError("The selected Skate installation is incomplete.")
    for name in ("ReSkate.dll", SETTINGS, BACKUP, RECEIPT, JOURNAL, LOCK):
        path = game / name
        if path.is_symlink() or (path.exists() and not path.is_file()):
            raise RuntimeError("Refusing an unexpected file type: " + str(path))
    lock = game / LOCK
    fd = os.open(lock, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise RuntimeError("Another install is running") from None
        ensure_stopped()
        recover_transaction(game)
        settings = load_settings(game)
        receipt_path = game / RECEIPT
        receipt = json.loads(receipt_path.read_text()) if receipt_path.exists() else None
        if receipt is not None and (not isinstance(receipt, dict)
                or not isinstance(receipt.get("updates_before"), bool)):
            raise RuntimeError("Invalid installation receipt; inspect before modifying")
        backup = game / BACKUP
        before = {name: base64.b64encode((game / name).read_bytes()).decode()
                  if (game / name).exists() else None for name in ("ReSkate.dll", SETTINGS, RECEIPT)}
        if dll is None:
            if not backup.is_file():
                raise RuntimeError("No stock backup exists yet. Nothing was changed.")
            if receipt and "updates_before" in receipt:
                settings["updates"] = receipt["updates_before"]
            stock = backup.read_bytes()
            if receipt and receipt.get("stock_sha256") not in (None, digest(stock)):
                raise RuntimeError("Stock backup hash mismatch")
            atomic_write(game / JOURNAL, encode(before))
            try:
                atomic_write(game / "ReSkate.dll", stock)
                if digest((game / "ReSkate.dll").read_bytes()) != digest(stock):
                    raise RuntimeError("Restored DLL verification failed")
                atomic_write(game / SETTINGS, encode(settings))
                if receipt:
                    receipt.update(active=False, restored_sha256=digest(stock),
                                   restored_updates=settings.get("updates", True))
                    atomic_write(receipt_path, encode(receipt))
                (game / JOURNAL).unlink()
                sync_directory(game)
            except BaseException:
                recover_transaction(game)
                raise
            return
        validate_dll(dll)
        old_dll = (game / "ReSkate.dll").read_bytes()
        settings_path = game / SETTINGS
        if not backup.exists():
            if receipt and receipt.get("active"):
                raise RuntimeError("Active development install has lost its stock backup")
            preserve_backup(backup, old_dll)
        if receipt and receipt.get("stock_sha256") not in (None, digest(backup.read_bytes())):
            raise RuntimeError("Stock backup hash mismatch")
        if not receipt or not receipt.get("active"):
            receipt = {"updates_before": settings.get("updates", True)}
        receipt.pop("previous_log_prefix_sha256", None)
        receipt.pop("previous_log_prefix_size", None)
        log = safe_path(log_path if log_path else game / "logs/ReSkate.log")
        if log.name != "ReSkate.log":
            raise ValueError("Expected ReSkate.log")
        receipt.update(active=True, commit=commit, dll_sha256=digest(dll),
                       stock_sha256=digest(backup.read_bytes()), installed_at=time.time(),
                       installation_id=uuid.uuid4().hex,
                       log_path=str(log),
                       previous_log_sha256=digest(log.read_bytes())
                       if log.is_file() and log.stat().st_size <= 16 * 1024 * 1024 else None)
        if log.is_file():
            with log.open("rb") as stream:
                prefix = stream.read(512)
            receipt.update(previous_log_prefix_sha256=digest(prefix), previous_log_prefix_size=len(prefix))
        atomic_write(game / JOURNAL, encode(before))
        settings["updates"] = False
        try:
            atomic_write(receipt_path, encode(receipt))
            atomic_write(settings_path, encode(settings))
            atomic_write(game / "ReSkate.dll", dll)
            if digest((game / "ReSkate.dll").read_bytes()) != digest(dll):
                raise RuntimeError("Destination DLL verification failed")
            (game / JOURNAL).unlink()
            sync_directory(game)
        except BaseException:
            recover_transaction(game)
            raise
    finally:
        os.close(fd)  # flock releases after interruption; never unlink a live lock inode.


def desktop_argument(value):
    # Two escape passes for Desktop Entry Exec; %% preserves a literal percent.
    return '"' + str(value).replace("\\", "\\\\\\\\").replace('"', '\\"').replace("$", "\\$").replace("`", "\\`").replace("%", "%%") + '"'


def install_buttons(bundle, game, manifest, register_steam=True):
    directory = Path.home() / ".local/share/reskate-arcade"
    directory.mkdir(parents=True, exist_ok=True)
    for archive_name, local_name in (("installer.py", "deck_installer.py"), ("updater.py", "deck_updater.py")):
        atomic_write(directory / local_name, bundle.read(archive_name))
    atomic_write(directory / "config.json", encode({"game": str(game)}))
    atomic_write(directory / "installed.json", encode({"commit": manifest["commit"], "dll_sha256": manifest["sha256"]}))
    applications = Path.home() / ".local/share/applications"
    applications.mkdir(parents=True, exist_ok=True)
    desktop = Path.home() / "Desktop"
    desktop.mkdir(exist_ok=True)
    for name, filename, arguments in (("Update Arcade Mode", "reskate-arcade-update.desktop", ""),
                                      ("Restore Stock ReSkate", "reskate-arcade-restore.desktop", " --restore")):
        content = ("[Desktop Entry]\nType=Application\nName=" + name +
                   "\nIcon=applications-games\nTerminal=false\nExec=python3 " +
                   desktop_argument(directory / "deck_updater.py") + arguments + "\n")
        for folder in (applications, desktop):
            atomic_write(folder / filename, content.encode())
            (folder / filename).chmod(0o755)
        marker = directory / (filename + ".steam-requested")
        if register_steam and not marker.exists() and shutil.which("steam"):
            # Steam's Linux add URL imports a desktop launcher into the running client.
            # Leave shortcuts.vdf to Steam; never edit it or stop Steam underneath the user.
            try:
                sentinel = Path("/tmp/addnonsteamgamefile")
                fd = os.open(sentinel, os.O_CREAT | os.O_WRONLY | os.O_NOFOLLOW, 0o600)
                os.close(fd)
                result = subprocess.run(["steam", "steam://addnonsteamgame/" +
                    urllib.parse.quote(str(applications / filename), safe="")],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15, check=False)
                if result.returncode == 0:
                    atomic_write(marker, b"Steam import requested\n")
            except (OSError, subprocess.TimeoutExpired):
                pass  # Desktop/menu buttons still work; remote setup can verify Steam import.


def install_bundle(bundle, game, register_steam=True):
    manifest = validate_manifest(json.loads(bundle.read("manifest.json")))
    dll = bundle.read("ReSkate.dll")
    if hashlib.sha256(dll).hexdigest() != manifest["sha256"]:
        raise RuntimeError("Installer checksum failed. Download it again; nothing was changed.")
    change_installation(game, dll, manifest["commit"])
    install_buttons(bundle, game, manifest, register_steam)


def main(bundle):
    try:
        game = choose_game()
        if game is None:
            return
        installing = True
        if (game / BACKUP).is_file():
            installing = dialog("question", "Skate found at:\n" + str(game) +
                                "\n\nInstall Arcade Mode, or restore your saved stock DLL?",
                                ("Install Arcade Mode", "Restore stock"))
        if not dialog("question", ("Install Arcade Mode?" if installing else "Restore stock ReSkate?") +
                      "\n\n" + str(game) + "\n\nClose Skate and its launcher first.",
                      ("Install" if installing else "Restore", "Cancel")):
            return
        if installing:
            install_bundle(bundle, game)
            message = ("Arcade Mode installed!\n\nYour stock DLL is backed up and automatic ReSkate updates "
                       "are turned off.\n\nReturn to Gaming Mode and start Skate as usual. "
                       "Use Update Arcade Mode for future builds and Restore Stock ReSkate to switch back. "
                       "Both buttons are on your desktop and were submitted to Steam for Gaming Mode. "
                       "You can delete the downloaded installer.")
        else:
            change_installation(game)
            message = "Stock ReSkate restored. Your previous update setting is restored too. Start Skate as usual."
        dialog("info", message)
    except Exception as error:
        dialog("error", str(error))


if __name__ == "__main__":
    main(BUNDLE)
