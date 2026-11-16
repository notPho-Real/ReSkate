import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from unified_candidate import create, expected_features


class CandidateTests(unittest.TestCase):
    def test_identity_and_declared_gate_must_match_actual_build(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "ReSkate.dll").write_bytes(b"test fixture")
            digest = hashlib.sha256(b"test fixture").hexdigest()
            (root / "commit.txt").write_text("a" * 40)
            (root / "arcade-gameplay-fingerprint.txt").write_text("b" * 64)
            manifest = root / "sources.json"
            manifest.write_text(json.dumps({"accepted": False, "game_build": 20260929, "sources": {"rc4": {"enabled": False, "sha": "c" * 40}}}))
            provenance = dict(source_sha="a" * 40, dll_sha256=digest, run_id="1", validation="development")
            (root / "provenance.json").write_text(json.dumps(provenance))
            features = root / "arcade-build-features.json"
            features.write_text(json.dumps(dict(wallplants=False, game_build=20260929)))
            result = create(root, manifest)
            self.assertEqual(result["dll_sha256"], digest)
            self.assertFalse(result["accepted"])
            features.write_text(json.dumps(dict(wallplants=True, game_build=20260929)))
            with self.assertRaises(ValueError): create(root, manifest)
            source = json.loads(manifest.read_text())
            source["sources"]["rc4"]["enabled"] = True
            manifest.write_text(json.dumps(source))
            self.assertTrue(create(root, manifest)["features"]["wallplants"])
            features.write_text(json.dumps(dict(wallplants=False, game_build=20260929)))
            with self.assertRaises(ValueError): create(root, manifest)
            features.write_text(json.dumps(dict(wallplants=True, game_build=20260929)))
            (root / "ReSkate.dll").write_bytes(b"different binary")
            with self.assertRaises(ValueError): create(root, manifest)

    def test_missing_or_ambiguous_feature_policy_is_rejected(self):
        for source in ({}, {"accepted": False, "game_build": 20260929},
                       {"accepted": True, "game_build": 20260929, "sources": {"rc4": {"enabled": True}}},
                       {"accepted": False, "game_build": 20260929, "sources": {"rc4": {"enabled": "false"}}},
                       {"accepted": False, "game_build": 20261001, "sources": {"rc4": {"enabled": True}}}):
            with self.subTest(source=source), self.assertRaises(ValueError):
                expected_features(source)


if __name__ == "__main__": unittest.main()
