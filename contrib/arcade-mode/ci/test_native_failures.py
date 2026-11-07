"""Negative controls for native compilation and CTest exit propagation."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class NativeFailureTests(unittest.TestCase):
    def test_compilation_and_test_failures_are_nonzero(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, build = root / "source", root / "build"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.24)\nproject(FailureControl LANGUAGES CXX)\n'
                'enable_testing()\nadd_executable(control control.cpp)\n'
                'add_test(NAME control COMMAND control)\n')
            (source / "control.cpp").write_text('#error deliberate CI negative control\n')
            def run(*args):
                return subprocess.run(args, capture_output=True, text=True)
            configure = run("cmake", "-S", str(source), "-B", str(build))
            self.assertEqual(configure.returncode, 0, configure.stdout + configure.stderr)
            compile_bad = run("cmake", "--build", str(build), "--config", "Release")
            self.assertNotEqual(compile_bad.returncode, 0, compile_bad.stdout)
            (source / "control.cpp").write_text('int main() { return 1; }\n')
            compile_good = run("cmake", "--build", str(build), "--config", "Release")
            self.assertEqual(compile_good.returncode, 0, compile_good.stdout + compile_good.stderr)
            test_bad = run("ctest", "--test-dir", str(build), "-C", "Release",
                           "--output-on-failure", "--no-tests=error")
            self.assertNotEqual(test_bad.returncode, 0, test_bad.stdout)
            self.assertIn("Failed", test_bad.stdout)
            (source / "control.cpp").write_text('int main() { return 0; }\n')
            self.assertEqual(run("cmake", "--build", str(build), "--config", "Release").returncode, 0)
            test_good = run("ctest", "--test-dir", str(build), "-C", "Release",
                            "--output-on-failure", "--no-tests=error")
            self.assertEqual(test_good.returncode, 0, test_good.stdout)


if __name__ == "__main__":
    unittest.main()

