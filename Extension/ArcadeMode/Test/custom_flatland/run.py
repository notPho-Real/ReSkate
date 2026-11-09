#!/usr/bin/env python3
"""Small portable proof; no root CMake configure, downloads or native runtime."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--cxx", default="c++")
parser.add_argument("--integration", action="store_true")
args = parser.parse_args()
root = Path(__file__).resolve().parents[4]
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
(out / "tmp").mkdir(exist_ok=True)
env = dict(os.environ, TMPDIR=str(out / "tmp"))
common = [args.cxx, "-std=c++20", "-I", str(root)]
if sys.platform == "darwin":
    # Avoid xcrun cache writes outside the chosen bounded output directory.
    sdk = Path("/Library/Developer/CommandLineTools/SDKs/MacOSX14.4.sdk")
    if sdk.exists():
        common += ["-isysroot", str(sdk), "-isystem", str(sdk / "usr/include/c++/v1")]
strict = ["-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Wsign-conversion", "-Werror"]
def run(command):
    subprocess.run(command, cwd=root, env=env, check=True)

core = out / "anim_core.o"
run(common + ["-c", str(root / "Extension/AnimationStudio/anim_core.cpp"), "-o", str(core)])
test = out / "casper_spike_tests"
run(common + strict + [str(Path(__file__).with_name("flatland_tests.cpp")), str(core), "-o", str(test)])
run([str(test)])
if args.integration:
    # Relevant boundaries only, not the exhaustive Arcade campaign.
    results = []
    for name in ("arcade_input_pattern_dispatch_tests", "arcade_ground_motion_step_tests",
                 "arcade_ground_control_tests", "arcade_input_session_tests",
                 "arcade_manual_balance_lifecycle_tests", "arcade_combo_trace_tests"):
        binary = out / name
        # This old combo fixture uses float-to-bool assertions; its existing
        # warning policy is Wall/Wextra/Werror, not Arcade's conversion policy.
        warnings = ["-Wall", "-Wextra", "-Werror"] if name == "arcade_combo_trace_tests" else strict
        build = subprocess.run(common + warnings + [str(root / "Extension/ArcadeMode/Test" / (name + ".cpp")),
                               "-o", str(binary)], cwd=root, env=env, capture_output=True, text=True)
        test_run = subprocess.run([str(binary)], env=env, capture_output=True, text=True) if build.returncode == 0 else None
        result = {"name": name, "build": build.returncode, "run": test_run.returncode if test_run else None,
                  "output": build.stdout + build.stderr + (test_run.stdout + test_run.stderr if test_run else "")}
        results.append(result)
        print(json.dumps(result), flush=True)
    (out / "integration-results.json").write_text(json.dumps(results, indent=2))
    # Keep the known RC5 recognizer failure visible; do not turn it green.
    if any(item["build"] or item["run"] for item in results):
        sys.exit(1)
