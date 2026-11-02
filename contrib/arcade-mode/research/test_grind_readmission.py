"""Inert checks for source-derived prerequisite diagnostics; no process access."""
import importlib.util
import math
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("readmission", Path(__file__).with_name("analyze_grind_readmission.py"))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
inputs = module.straight_return_inputs


class StraightReturnInputs(unittest.TestCase):
    def test_negative_pitch_comparison_operand_order(self):
        for entry in (0, 1):
            for pitch in (-.4, -0., 0., .4):
                expected = (entry == 0 and pitch < 0) or (entry == 1 and pitch >= 0)
                self.assertEqual(inputs(0., 0., pitch, entry)["entry_differs_from_negative_pitch"], expected)

    def test_held_and_neutral_smith_are_different_failures(self):
        self.assertEqual(inputs(-.3117068409919739, .4000000059604645, .2, 0), {
            "heading_band": True, "entry_differs_from_negative_pitch": False, "small_requested_pitch": False})
        self.assertEqual(inputs(-.45633161067962646, 0., .2, 0), {
            "heading_band": False, "entry_differs_from_negative_pitch": False, "small_requested_pitch": True})
        # The measured Crooked eligible context has a different captured bit.
        self.assertTrue(all(inputs(.34070485830307007, 0., .2, 1).values()))

    def test_strict_heading_and_inclusive_pitch_bounds(self):
        for heading, expected in ((0., True), (.3490658402442932, False),
                                  (-.3490658402442932, False), (2.792526960372925, False),
                                  (-2.792526960372925, False), (3., True), (-3., True),
                                  (2 * math.pi + .1, True)):
            self.assertEqual(inputs(heading, 0., .2, 1)["heading_band"], expected)
        for pitch, expected in ((.15000000596046448, True), (-.15000000596046448, True),
                                (.151, False), (-.151, False)):
            self.assertEqual(inputs(0., pitch, .2, 1)["small_requested_pitch"], expected)

    def test_invalid_input_is_not_admission_evidence(self):
        for args in ((math.nan, 0., 0., 0), (0., math.inf, 0., 0),
                     (0., 0., -math.inf, 0), (0., 0., 0., 2)):
            with self.assertRaises(AssertionError):
                inputs(*args)


if __name__ == "__main__":
    unittest.main()
