"""Build-system regression: optional assets must regenerate only when needed."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(shutil.which("cmake"), "CMake is required for generator integration")
class OptionalAssetTests(unittest.TestCase):
    def test_add_remove_modify_and_noop(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "source"
            build = Path(temporary) / "build"
            source.mkdir()
            module = (ROOT / "cmake/OptionalAssets.cmake").as_posix()
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.24)\nproject(Assets NONE)\n'
                f'include("{module}")\n'
                'message(STATUS "ASSET_CONFIGURE")\n'
                'get_property(watched DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS)\n'
                'file(WRITE "${CMAKE_BINARY_DIR}/watched.txt" "${watched}")\n')

            def run(*args):
                return subprocess.run(["cmake", *map(str, args)], check=True,
                                      capture_output=True, text=True).stdout

            run("-S", source, "-B", build)
            self.assertNotIn("ASSET_CONFIGURE", run("--build", build))
            asset = source / "assets/emotes/emotes.json"
            asset.parent.mkdir(parents=True)
            asset.write_text("{}")
            self.assertIn("ASSET_CONFIGURE", run("--build", build))
            watched = (build / "watched.txt").read_text().split(";")
            self.assertTrue(any(asset.samefile(name) for name in watched))
            self.assertTrue(all(Path(name).is_file() for name in watched))
            self.assertNotIn("ASSET_CONFIGURE", run("--build", build))
            asset.write_text('{"changed":true}')
            # Coarse Windows filesystem timestamps must still make this newer
            # than the generated stamp; no timer sleep is needed in the test.
            stamp = build / "CMakeCache.txt"
            newer = max(asset.stat().st_mtime, stamp.stat().st_mtime) + 2
            os.utime(asset, (newer, newer))
            self.assertIn("ASSET_CONFIGURE", run("--build", build))
            asset.unlink()
            self.assertIn("ASSET_CONFIGURE", run("--build", build))
            self.assertEqual((build / "watched.txt").read_text(), "")
            self.assertNotIn("ASSET_CONFIGURE", run("--build", build))


if __name__ == "__main__":
    unittest.main()
