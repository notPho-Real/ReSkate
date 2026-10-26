"""Generate the portable Manual lifecycle composition from current source.

Only clocks, probe/presenter observations, outer permissions and nonactuating
sinks are controlled. This is software composition evidence, not native action
initialization/out timing or player acceptance. The optional HUD target also
executes the current renderer against the resulting admitted display state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


def unique(text, marker):
    if text.count(marker) != 1:
        raise ValueError("source marker must occur exactly once: " + marker)
    return text.index(marker)


def between(text, first, last):
    start = unique(text, first)
    end = unique(text, last)
    if end <= start:
        raise ValueError("source markers reversed: " + first)
    return text[start:end]


def function(text, marker):
    start = unique(text, marker)
    # Ignore comments and quoted literals when counting braces. Leave their
    # lengths intact so returned bytes are still the original source bytes.
    tokens = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    masked = re.sub(tokens, lambda match: " " * len(match.group()), text)
    opening = masked.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        if end == len(masked):
            raise ValueError("unterminated source function: " + marker)
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return text[start:end]


def replace_once(text, old, new):
    unique(text, old)
    return text.replace(old, new, 1)


def generated(root):
    folder = root / "Extension/ArcadeMode/Test/manual_lifecycle"
    mode = (root / "Extension/ArcadeMode/arcade_mode.cpp").read_text(encoding="utf-8")
    hud = (root / "Extension/ArcadeMode/arcade_balance_hud.cpp").read_text(encoding="utf-8")
    acquisition = (root / "Extension/ArcadeMode/arcade_manual_acquisition.h").read_text(encoding="utf-8")
    parts = {
        "CAMERA_STATE": between(mode, "CameraOrbit camera_orbit;", "std::atomic<unsigned> authored_status"),
        "CAMERA_TICK": function(mode, "void on_camera_tick("),
        "STATE": between(mode, "ComboSession combo_session(", "void update_combo("),
        "INPUT_STATE": mode[unique(mode, "ArcadeInput arcade_input("):mode.index(";", unique(mode, "ArcadeInput arcade_input("))+1],
        "UPDATE": function(mode, "void update_combo("),
        "PACKET_RETIRE": function(mode, "void retire_playback_packets("),
        "HUD_PUBLISH": function(hud, "void publish_balance_hud("),
        "HUD_PENDING": function(hud, "bool balance_hud_pending("),
        "HUD_DRAW": function(hud, "void draw_balance_hud("),
        "SET_ENABLED": function(mode, "void set_enabled("),
        "OBSERVER": between(mode, "            const auto frame = native_input_state.observe(",
                            "            if(frame.valid)capture_event"),
        "POLL": between(mode, "        result = arcade_input.poll(", "        const auto capture_proof="),
        "DELIVERY": between(mode, "        const auto& action = result.action;",
                            "        result.buttons &= ~action.consume_buttons;"),
        "HOP": between(mode, "        const auto pose = manual_observed.load(std::memory_order_relaxed);",
                       "        const bool rotation_enabled"),
    }
    source = (folder / "fixture.cpp.in").read_text(encoding="utf-8")
    for name, body in parts.items():
        source = replace_once(source, "@@" + name + "@@", body)
    source = replace_once(source, "@@ACQUISITION_INCLUDE@@",
                          '#include "Extension/ArcadeMode/arcade_manual_acquisition.h"')
    if "@@" in source:
        raise ValueError("unresolved template marker")
    for name, body in parts.items():
        if source.count(body) != 1:
            raise ValueError("generated source correspondence failed: " + name)
    outputs = {"current.cpp": source}
    # Each control removes one correction from the exact same generated fixture.
    # Expected failure is checked by assertion message, not CTest WILL_FAIL (which
    # could incorrectly treat an unrelated crash/tool error as a regression).
    controls = {
        "camera_contention": ("std::unique_lock lock(ollie_mutex);\n            const auto sampled_at = GetTickCount64();",
                              "const auto sampled_at = GetTickCount64();\n            std::unique_lock lock(ollie_mutex,std::try_to_lock);"),
        "camera_snapshot_clock": ("std::unique_lock lock(ollie_mutex);\n            const auto sampled_at = GetTickCount64();",
                                  "const auto sampled_at = GetTickCount64();\n            std::unique_lock lock(ollie_mutex);"),
        "camera_recheck": ("if (!enabled() || camera_reset_generation.load(std::memory_order_acquire) != generation ||\n            !input.same_binding(read_input())) {", "if (false) {"),
        "camera_publish": (" ||\n            !enabled() || camera_reset_generation.load(std::memory_order_acquire) != generation ||\n            !input.same_binding(read_input())", ""),
        "camera_toggle": ("camera_reset_generation.fetch_add(1, std::memory_order_release);", ""),
        "model_reset": ("if (!physical.valid) { ground_manual_balance.reset(); manual_hop.reset(); }",
                        "if (!physical.valid) { ground_manual_balance.reset(); manual_hop.reset(); manual_acquisition.reset(); }"),
        "producer_revoke": ("if (manual_acquisition.observe_native(now,frame)) arcade_input.cancel_manuals();", ""),
        "buffered_release": ("manual_release.retire_observed(now,manual_completion);", "(void)manual_completion;"),
    }
    for name, (old, new) in controls.items():
        outputs[name + ".cpp"] = replace_once(source, old, new)
    outputs["hud_source.cpp"] = replace_once(source,
        "BalanceHudSource hud_source{native_frame.sampled_at,native_frame.valid}",
        "BalanceHudSource hud_source{now,native_frame.valid}")
    outputs["hud_axis.cpp"] = replace_once(source,
        "point(view.vertical ? view.cursor_y : view.cursor_x, 0)",
        "point(view.cursor_x, 0)")
    outputs["hud_clear.cpp"] = replace_once(source,
        "    publish_balance_hud({}, 0, {});", "    // HUD clear removed by this control.")
    outputs["bail.cpp"] = replace_once(source,
        '#include "Extension/ArcadeMode/arcade_manual_acquisition.h"',
        '#include "Extension/ArcadeMode/arcade_manual_acquisition.h"\n#include "acquisition_without_bail.h"')
    outputs["bail.cpp"] = replace_once(outputs["bail.cpp"],
        "ManualAcquisition manual_acquisition;", "ManualAcquisitionWithoutBail manual_acquisition;")
    # All dependencies still resolve to the original repository headers. Only
    # the existing positive-bail predicate is removed in this negative control.
    rollback = replace_once(acquisition, "frame.state == 300 || ", "")
    rollback = rollback.replace("ManualAcquisition", "ManualAcquisitionWithoutBail")
    rollback = rollback.replace("manual_acquisition_pose", "manual_acquisition_pose_without_bail")
    rollback = re.sub(r'#include "(arcade_[^"/]+\.h)"',
                      r'#include "Extension/ArcadeMode/\1"', rollback)
    outputs["acquisition_without_bail.h"] = rollback
    manifest = {
        "source_chunks_sha256": {name: hashlib.sha256(body.encode()).hexdigest()
                                 for name, body in parts.items()},
        "acquisition_header_sha256": hashlib.sha256(acquisition.encode()).hexdigest(),
        "generated_sha256": {name: hashlib.sha256(body.encode()).hexdigest()
                             for name, body in outputs.items()},
    }
    outputs["correspondence.json"] = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    return outputs


FAILURES = {
    "camera_contention": "contention must inspect the snapshot after its publisher completes",
    "camera_snapshot_clock": "camera snapshot freshness must include time waiting for its publisher",
    "camera_recheck": "changed camera generation cannot continue stale orbit or leave a newly raced lease active",
    "camera_publish": "mode toggle during camera callback must retire ownership before the callback returns",
    "camera_toggle": "deferred camera retirement never acknowledges the toggle before cleanup succeeds",
    "model_reset": "still-acquired same-owner contact retains original bounded cleanup after model interval loss",
    "producer_revoke": "hard native exit seen between client callbacks must retire old command and cleanup before original lease expires",
    "buffered_release": "original observed physical completion between clients must retire old pending release before recovery contact",
    "bail": "old Manual cleanup cannot survive positive native exit into unrelated recovery contact",
    "hud_source": "client publication retains the original native HUD timestamp",
    "hud_axis": "admitted physical position reaches the rendered cursor on the correct axis",
    "hud_clear": "actual OFF callback clears the HUD publication source immediately",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--expect-failure", choices=FAILURES)
    parser.add_argument("--binary", type=Path)
    args = parser.parse_args()
    if args.expect_failure:
        if not args.binary:
            parser.error("--expect-failure requires --binary")
        result = subprocess.run([str(args.binary.resolve())], capture_output=True, text=True, timeout=60)
        print(result.stdout, end="")
        print(result.stderr, end="", file=sys.stderr)
        if result.returncode != 1 or FAILURES[args.expect_failure] not in result.stderr.splitlines():
            raise ValueError("rollback did not fail at its intended assertion: " + args.expect_failure)
        print("Expected lifecycle rollback assertion verified: " + args.expect_failure)
        return
    if not args.root or not args.output:
        parser.error("generation/check requires --root and --output")
    outputs = generated(args.root.resolve())
    if args.check:
        for name, body in outputs.items():
            if (args.output / name).read_text(encoding="utf-8") != body:
                raise ValueError("stale generated source: " + name)
        print("Manual lifecycle source correspondence verified")
    else:
        args.output.mkdir(parents=True, exist_ok=True)
        for name, body in outputs.items():
            (args.output / name).write_text(body, encoding="utf-8")


if __name__ == "__main__":
    main()
