"""Exercise the production CMake fingerprint with real temporary source changes."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


@unittest.skipUnless(shutil.which("cmake"), "CMake is required")
class EvidenceFingerprintTests(unittest.TestCase):
    def test_evidence_is_independent_and_line_endings_are_portable(self):
        module = pathlib.Path(__file__).resolve().parents[2] / "cmake/ArcadeEvidence.cmake"
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            arcade = root / "Extension/ArcadeMode"
            arcade.mkdir(parents=True)
            (root / "Engine").mkdir()
            (root / "cmake").mkdir()
            shutil.copyfile(module, root / "cmake/ArcadeEvidence.cmake")
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.24)\nproject(receipt CXX)\n"
                "add_library(dingosdk_runtime STATIC dummy.cpp)\n"
                "add_library(dingosdk_overlay STATIC dummy.cpp)\n"
                "include(cmake/ArcadeEvidence.cmake)\n", encoding="utf-8")
            (root / "dummy.cpp").write_text("int receipt_fixture;\n", encoding="utf-8")
            gameplay = arcade / "arcade_trick_registry.h"
            gameplay.write_bytes(b"binding = A;\nhandler = native;\n")

            def fingerprint(wallplants="OFF"):
                subprocess.run(["cmake", "-S", str(root), "-B", str(root / "build"),
                                "-DDINGOSDK_ARCADE_WALLPLANTS=" + wallplants],
                               check=True, capture_output=True, text=True)
                return (root / "build/arcade-gameplay-fingerprint.txt").read_text().strip()

            original = fingerprint()
            self.assertNotEqual(original, fingerprint("ON"))
            self.assertEqual(original, fingerprint("OFF"))
            for name in ("arcade_trick_evidence.inl", "arcade_trick_capture_data.inl",
                         "arcade_trick_acceptance.inl", "arcade_evidence_types.h",
                         "arcade_trickpedia.h", "arcade_registry_validation.h"):
                (arcade / name).write_text("new evidence, validation or presentation\n", encoding="utf-8")
                self.assertEqual(original, fingerprint(), name)
            gameplay.write_bytes(b"binding = A;\r\nhandler = native;\r\n")
            self.assertEqual(original, fingerprint())
            gameplay.write_bytes(b"binding = X;\nhandler = native;\n")
            self.assertNotEqual(original, fingerprint())


if __name__ == "__main__":
    unittest.main()
