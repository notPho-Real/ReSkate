"""Compatibility entry: pairing never deploys; default invocation explicitly deploys."""
import sys
from lab.controller import main

if __name__ == "__main__":
    arguments = sys.argv[1:]
    pairing = "--pair" in arguments
    arguments = [arg for arg in arguments if arg != "--pair"]
    sys.exit(main(["pair" if pairing else "deploy", *arguments]))
