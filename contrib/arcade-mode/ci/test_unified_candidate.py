import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from unified_candidate import create


class CandidateTests(unittest.TestCase):
    def test_identity_and_disabled_gate_must_match_actual_build(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "ReSkate.dll").write_bytes(b"test fixture")
            digest = hashlib.sha256(b"test fixture").hexdigest()
            (root / "commit.txt").write_text("a" * 40)
            (root / "arcade-gameplay-fingerprint.txt").write_text("b" * 64)
            manifest = root / "sources.json"
            manifest.write_text(json.dumps({"accepted": False, "sources": {"rc3": {"sha": "c" * 40}}}))
            provenance = dict(source_sha="a" * 40, dll_sha256=digest, run_id="1", validation="development")
            (root / "provenance.json").write_text(json.dumps(provenance))
            features = root / "arcade-build-features.json"
            features.write_text(json.dumps(dict(wallplants=False, game_build=20260929)))
            result = create(root, manifest)
            self.assertEqual(result["dll_sha256"], digest)
            self.assertFalse(result["accepted"])
            features.write_text(json.dumps(dict(wallplants=True, game_build=20260929)))
            with self.assertRaises(ValueError): create(root, manifest)
            features.write_text(json.dumps(dict(wallplants=False, game_build=20260929)))
            (root / "ReSkate.dll").write_bytes(b"different binary")
            with self.assertRaises(ValueError): create(root, manifest)


if __name__ == "__main__": unittest.main()
