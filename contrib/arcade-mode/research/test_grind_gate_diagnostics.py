import importlib.util
import json
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("diagnostics", Path(__file__).with_name("grind_gate_diagnostics.py"))
diagnostics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(diagnostics)


class DiagnosticsTests(unittest.TestCase):
    def test_failed_segment_values_and_site(self):
        def validate_leg_state():
            state, contact, named = 402, True, 21
            observed_segment = (100, bytes.fromhex("02000000000000000000000000000000"), b"current")
            assert observed_segment == (100, bytes(16), b"expected"), "segment changed"
        try:
            validate_leg_state()
        except AssertionError as exc:
            result = diagnostics.gate_failure(exc, "cache_read", {"leg": 2, "segment_reference": (100, bytes(16), b"expected")})
        self.assertEqual(result["message"], "segment changed")
        failed = result["frames"][-1]
        self.assertEqual(failed["function"], "validate_leg_state")
        self.assertEqual(failed["observed"]["state"], 402)
        self.assertEqual(failed["observed"]["named"], 21)
        self.assertNotEqual(failed["observed"]["observed_segment"], result["context"]["segment_reference"])
        self.assertFalse(result["new_native_reads"] or result["atomic_frame"])
        json.dumps(result, allow_nan=False)

    def test_unlisted_locals_and_objects_never_inspected(self):
        class Unsafe:
            def __repr__(self):
                raise AssertionError("must not invoke repr")
        private = Unsafe()
        result = diagnostics.gate_failure(AssertionError(), "owned", {"actor": private, "secret": "omitted"})
        self.assertEqual(result["context"], {"actor": "not captured"})
        self.assertEqual(result["frames"], [])
        self.assertIsNone(result["message"])

    def test_nested_values_and_message_bounded(self):
        huge = [[[["deep"]] * 100] * 100] * 100
        result = diagnostics.gate_failure(AssertionError("x" * 10000), "b" * 1000,
                                          {"angles": huge, "ends": b"a" * 10000})
        self.assertEqual(len(result["message"]), 256)
        self.assertEqual(len(result["boundary"]), 80)
        self.assertEqual(len(result["context"]["ends"]["hex"]), 192)
        self.assertLess(len(json.dumps(result)), 30000)

    def test_trace_depth_bounded(self):
        def recurse(n):
            if n:
                recurse(n - 1)
            else:
                raise AssertionError("depth")
        try:
            recurse(100)
        except AssertionError as exc:
            result = diagnostics.gate_failure(exc, "actor", {})
        self.assertEqual(len(result["frames"]), 8)
        self.assertTrue(result["trace_truncated"])

    def test_nonfinite_value_does_not_break_json(self):
        result = diagnostics.gate_failure(AssertionError(float("nan")), "angle", {"pitch": float("inf")})
        self.assertEqual(result["message"], "nonfinite")
        self.assertEqual(result["context"]["pitch"], "nonfinite")
        json.dumps(result, allow_nan=False)

    def test_integer_size_bounded(self):
        result = diagnostics.gate_failure(AssertionError(), "owner", {"actor": 1 << 100000})
        self.assertEqual(result["context"]["actor"], "integer out of range")


if __name__ == "__main__":
    unittest.main()
