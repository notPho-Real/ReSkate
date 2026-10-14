import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import deck_installer as installer
import package_deck_installer as package
import deck_updater as updater
import hashlib


def fake_dll():
    data = bytearray(128)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 60, 64)
    data[64:68] = b"PE\0\0"
    struct.pack_into("<H", data, 68, 0x8664)
    struct.pack_into("<H", data, 86, 0x2000)
    struct.pack_into("<H", data, 88, 0x20b)
    return bytes(data)


def game_at(path):
    path.mkdir(parents=True)
    for name in ("Skate.exe", "ReSkateLauncher.exe", "ReSkate.dll"):
        (path / name).write_bytes(b"stock")
    return path


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.game = game_at(self.root / "Skate with spaces")
        self.stopped = patch.object(installer, "ensure_stopped")
        self.stopped.start()
        self.addCleanup(self.stopped.stop)

    def test_install_repeat_restore_preserves_preferences(self):
        settings = self.game / installer.SETTINGS
        settings.write_text(json.dumps({"updates": True, "width": 1280, "arguments": "keep me"}))
        for _ in range(2):
            installer.change_installation(self.game, fake_dll(), "a" * 40)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), fake_dll())
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), b"stock")
        value = json.loads(settings.read_text())
        self.assertFalse(value["updates"])
        value["width"] = 1600
        settings.write_text(json.dumps(value))
        installer.change_installation(self.game)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertEqual(json.loads(settings.read_text()), {"updates": True, "width": 1600, "arguments": "keep me"})

    def test_existing_disabled_updates_stay_disabled_after_restore(self):
        (self.game / installer.SETTINGS).write_text('{"updates": false}')
        installer.change_installation(self.game, fake_dll(), "a" * 40)
        installer.change_installation(self.game)
        self.assertFalse(installer.load_settings(self.game)["updates"])

    def test_install_and_restore_never_replace_game_or_delete_stock(self):
        before = {name: (self.game / name).read_bytes() for name in ('Skate.exe', 'ReSkateLauncher.exe')}
        installer.change_installation(self.game, fake_dll(), 'a' * 40)
        for operation in (lambda: installer.change_installation(self.game, fake_dll(), 'b' * 40),
                          lambda: installer.change_installation(self.game)):
            operation()
            self.assertEqual((self.game / installer.BACKUP).read_bytes(), b'stock')
            for name, content in before.items():
                self.assertEqual((self.game / name).read_bytes(), content)

    def test_missing_settings_use_launcher_default_on_restore(self):
        installer.change_installation(self.game, fake_dll(), "a" * 40)
        installer.change_installation(self.game)
        self.assertTrue(installer.load_settings(self.game)["updates"])

    def test_discovery_internal_sd_and_steam_library(self):
        home = self.root / "home"
        internal = game_at(home / ".local/share/Steam/steamapps/common/skate")
        sd = game_at(self.root / "media/deck/Card/steamapps/common/skate")
        external = game_at(self.root / "custom library/steamapps/common/skate")
        (home / ".local/share/Steam/steamapps/libraryfolders.vdf").write_text(
            '"path" "' + str(self.root / "custom library") + '"')
        self.assertEqual(set(installer.find_games(home, self.root / "media")), {internal, sd, external})

    def test_invalid_dll_and_settings_do_not_change_stock(self):
        with self.assertRaises(RuntimeError):
            installer.change_installation(self.game, b"bad", "a" * 40)
        self.assertFalse((self.game / installer.BACKUP).exists())
        (self.game / installer.SETTINGS).write_text("not json")
        with self.assertRaises(ValueError):
            installer.change_installation(self.game, fake_dll(), "a" * 40)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_lock_symlink_and_running_game_refused(self):
        lock = self.game / ".arcade-mode-install.lock"
        lock.mkdir()
        with self.assertRaises(RuntimeError):
            installer.change_installation(self.game, fake_dll(), "a" * 40)
        lock.rmdir()
        (self.game / installer.SETTINGS).symlink_to(self.root / "elsewhere")
        with self.assertRaises(RuntimeError):
            installer.change_installation(self.game, fake_dll(), "a" * 40)
        (self.game / installer.SETTINGS).unlink()
        installer.ensure_stopped.side_effect = RuntimeError("Running")
        with self.assertRaises(RuntimeError):
            installer.change_installation(self.game, fake_dll(), "a" * 40)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")

    def test_failed_dll_replace_rolls_back_update_setting(self):
        (self.game / installer.SETTINGS).write_text('{"updates":true,"width":800}')
        real_write = installer.atomic_write
        def fail_dll(path, data):
            if path.name == "ReSkate.dll":
                raise OSError("disk full")
            real_write(path, data)
        with patch.object(installer, "atomic_write", side_effect=fail_dll):
            with self.assertRaises(OSError):
                installer.change_installation(self.game, fake_dll(), "a" * 40)
        self.assertTrue(installer.load_settings(self.game)["updates"])
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), b"stock")

    def test_offline_desktop_exec_installs_embedded_dll(self):
        home = self.root / "home"
        game = game_at(home / ".local/share/Steam/steamapps/common/skate")
        dll = self.root / "build.dll"
        dll.write_bytes(fake_dll())
        desktop = self.root / "Install Arcade (1).desktop"
        package.build(dll, "a" * 40, desktop)
        # First the desktop key-value escape pass, then Exec argument quoting.
        line = next(line[5:] for line in desktop.read_text().splitlines() if line.startswith("Exec="))
        argv = shlex.split(line.replace("\\\\", "\\"))
        self.assertEqual(argv[-1], "%k")
        argv[0] = sys.executable
        argv[-1] = desktop.as_uri()
        bin_dir = self.root / "bin"
        bin_dir.mkdir()
        for name, exit_code in (("zenity", 0), ("pgrep", 1)):
            tool = bin_dir / name
            tool.write_text("#!/bin/sh\nexit " + str(exit_code) + "\n")
            tool.chmod(0o755)
        env = dict(os.environ, HOME=str(home), PATH=str(bin_dir))
        subprocess.run(argv, env=env, check=True, capture_output=True)
        self.assertEqual((game / "ReSkate.dll").read_bytes(), fake_dll())
        self.assertFalse(installer.load_settings(game)["updates"])


class UpdaterTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.game = game_at(self.root / "game")
        self.home = self.root / "home"
        self.home.mkdir()
        self.directory = self.home / ".local/share/reskate-arcade"
        self.directory.mkdir(parents=True)
        self.dll = self.root / "ReSkate.dll"
        self.dll.write_bytes(fake_dll())
        self.desktop = self.root / "Install.desktop"
        package.build(self.dll, "b" * 40, self.desktop)
        self.channel = json.loads(self.desktop.with_name("channel.json").read_text())
        self.patches = [patch.object(installer, "ensure_stopped"),
                        patch("pathlib.Path.home", return_value=self.home),
                        patch("subprocess.run", return_value=subprocess.CompletedProcess([], 1))]
        for mock in self.patches:
            mock.start()
            self.addCleanup(mock.stop)

    def test_update_download_verify_install_and_repeat(self):
        # The downloaded installer calls pgrep; return its 'not running' code.
        with patch.object(updater, "download", side_effect=[json.dumps(self.channel).encode(), self.desktop.read_bytes()]):
            self.assertIn("updated", updater.update(self.game, self.directory))
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), fake_dll())
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), b"stock")
        self.assertFalse(installer.load_settings(self.game)["updates"])
        self.assertTrue((self.home / "Desktop/reskate-arcade-update.desktop").exists())
        with patch.object(updater, "download", return_value=json.dumps(self.channel).encode()) as fetch:
            self.assertIn("already up to date", updater.update(self.game, self.directory))
            self.assertEqual(fetch.call_count, 1)
        installer.change_installation(self.game)
        with patch.object(updater, "download", side_effect=[json.dumps(self.channel).encode(), self.desktop.read_bytes()]):
            self.assertIn("updated", updater.update(self.game, self.directory))
        self.assertEqual((self.game / installer.BACKUP).read_bytes(), b"stock")

    def test_bad_download_and_offline_leave_dll_unchanged(self):
        with patch.object(updater, "download", side_effect=[json.dumps(self.channel).encode(), b"corrupted"]):
            with self.assertRaises(ValueError):
                updater.update(self.game, self.directory)
        with patch.object(updater, "download", side_effect=updater.urllib.error.URLError("offline")):
            with self.assertRaises(updater.urllib.error.URLError):
                updater.update(self.game, self.directory)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")
        self.assertFalse((self.game / installer.BACKUP).exists())

    def test_other_repository_or_unversioned_update_refused(self):
        for url in ("https://example.com/update", updater.RELEASES + "arcade-deck-latest/Install-ReSkate-Arcade.desktop"):
            bad = dict(self.channel, url=url)
            with self.assertRaises(ValueError):
                updater.validate_channel(bad)

    def test_manifest_commit_must_match_channel(self):
        wrong = dict(self.channel, commit="c" * 40,
                     url=updater.RELEASES + "arcade-build-" + "c" * 40 + "/Install-ReSkate-Arcade.desktop")
        with patch.object(updater, "download", side_effect=[json.dumps(wrong).encode(), self.desktop.read_bytes()]):
            with self.assertRaises(ValueError):
                updater.update(self.game, self.directory)
        self.assertEqual((self.game / "ReSkate.dll").read_bytes(), b"stock")


if __name__ == "__main__":
    unittest.main()
