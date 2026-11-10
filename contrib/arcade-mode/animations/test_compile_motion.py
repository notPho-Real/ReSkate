import copy
import json
from pathlib import Path
import unittest
from compile_motion import compile_motion

class MotionDataTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(Path(__file__).with_name("casper.motion.json").read_text())

    def test_generated_data_matches(self):
        target = Path(__file__).resolve().parents[3] / "Extension/ArcadeMode/arcade_casper_motion.inl"
        self.assertEqual(compile_motion(json.dumps(self.data)), target.read_text())

    def test_rejects_invalid_data(self):
        cases = []
        for value in (float("nan"), float("inf"), 10001):
            d = copy.deepcopy(self.data)
            d["phases"]["entry"][1][1] = value
            cases.append(d)
        d = copy.deepcopy(self.data); d["phases"]["entry"][1][0] = 0; cases.append(d)
        d = copy.deepcopy(self.data); d["phases"]["entry"][1][-1] = 2; cases.append(d)
        d = copy.deepcopy(self.data); d["phases"]["entry"][1].pop(); cases.append(d)
        d = copy.deepcopy(self.data); d["channels"][1] = "bad; identifier"; cases.append(d)
        d = copy.deepcopy(self.data); d["namespace"] = "bad{}"; cases.append(d)
        for data in cases:
            with self.assertRaises(ValueError):
                compile_motion(json.dumps(data))

if __name__ == "__main__":
    unittest.main()
