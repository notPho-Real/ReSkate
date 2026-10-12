"""Legacy local CLI, now sharing the lab's transaction and verification path."""
import argparse
from pathlib import Path
import deck_installer as installer


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("game")
    parser.add_argument("dll", nargs="?")
    args = parser.parse_args()
    if args.dll:
        source = Path(args.dll)
        manifest = installer.validate_manifest({
            "commit": source.with_name("commit.txt").read_text().strip(),
            "sha256": source.with_name("ReSkate.dll.sha256").read_text().split()[0]})
        data = source.read_bytes()
        if installer.digest(data) != manifest["sha256"]:
            raise ValueError("Source hash mismatch")
        installer.change_installation(args.game, data, manifest["commit"])
        print("Development DLL installed and verified")
    else:
        installer.change_installation(args.game)
        print("Stock DLL and previous update preference restored")


if __name__ == "__main__":
    main()
