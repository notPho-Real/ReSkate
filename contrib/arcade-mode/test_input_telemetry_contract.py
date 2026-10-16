"""Wire compatibility and hot-path boundary checks; never accesses hardware."""
import importlib.util
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("arcade_probe", ROOT / "contrib/arcade-mode/lab/probe.py")
PROBE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PROBE)


class InputTelemetryContract(unittest.TestCase):
    def test_repeated_gap_rows_are_counted_regardless_of_channel(self):
        for channel in ("Skater", "Command"):
            text = "\n".join(f"[2026-10-16 12:00:00.000] [7:8] {channel}: ARCADE {kind} GAP: dropped=1."
                             for kind in ("PAD", "COMBO", "PAD", "COMBO"))
            self.assertEqual(PROBE.parse_pad_samples(text)["dropped"], 2)
            self.assertEqual(PROBE.parse_combo_samples(text)["dropped"], 2)

    def test_gap_sink_bypasses_identical_message_suppression(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_mode.cpp").read_text()
        gap = source.split("void write_input_gap(", 1)[1].split("DWORD WINAPI input_telemetry_worker", 1)[0]
        for kind in ("PAD", "COMBO"):
            self.assertRegex(gap, rf'logging::Channel::command,\s*"ARCADE {kind} GAP: dropped=\{{\}}\."')
        logger = (ROOT / "Engine/Core/Log/logger.cpp").read_text()
        self.assertIn("channel != Channel::command && level != Level::critical", logger)

    def test_continuous_producers_do_not_format_or_log(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_mode.cpp").read_text()
        regions = [source.split("void update_combo(", 1)[1].split("void write_client_telemetry", 1)[0],
                   source.split("void observe_combo_step(", 1)[1].split("bool validate_request_probe", 1)[0],
                   source.split("void observe_xinput_source(", 1)[1]]
        for region in regions:
            self.assertNotIn("logging::", region)
            self.assertNotIn("std::format", region)
        client = source.split("void on_client_tick()", 1)[1].split("void observe_xinput_source", 1)[0]
        # Bounded startup catalog emission is the only direct client log left.
        self.assertEqual(re.findall(r'"(ARCADE [^":]+):', client),
                         ["ARCADE AUTHORED GESTURE", "ARCADE AUTHORED LABEL", "ARCADE AUTHORED ARC"])

    def test_native_ui_observer_only_publishes_detached_rows(self):
        source = (ROOT / "Extension/ArcadeMode/arcade_score_probe.cpp").read_text()
        self.assertNotIn("logging::", source)
        self.assertNotIn("std::format", source)
        self.assertNotIn("score_probe_path_text(", source)
        self.assertIn("game::ModelWriteLock lock(manager);", source)
        self.assertIn("enqueue_client_telemetry(ScorePresenceEvidence", source)

    def test_existing_score_parser_accepts_unchanged_bounded_paths(self):
        path = "9b74fc7c.00000000.ffffffff.000123ab"
        text = "\n".join([
            f"Skater: ARCADE SCORE NODE: path={path} kind=7 size=8.",
            f"Skater: ARCADE SCORE LABEL: ms=101 path={path} id=25.",
            f"Skater: ARCADE SCORE VALUE: ms=101 path={path} value=25.",
            "Skater: ARCADE SCORE PRESENCE: ms=100 source=1 rows=5 stage=0 parts=1 labels=4.",
            "Skater: ARCADE SCORE LIMIT: reached=true."])
        result = PROBE.parse_score_samples(text)
        self.assertEqual(result["nodes"][0]["path"], path)
        self.assertEqual(result["labels"][0]["id"], 25)
        self.assertEqual(result["values"][0]["value"], 25)
        self.assertTrue(result["limit_reached"])
        self.assertEqual(PROBE.parse_score_presence(text)["samples"][0]["rows"], 5)


if __name__ == "__main__":
    unittest.main()
