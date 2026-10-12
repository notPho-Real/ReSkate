#!/usr/bin/env bash
set -euo pipefail
[[ $# == 2 ]] || { echo "Expected 2 path argument(s)" >&2; exit 1; }
exec python3 -B "$(dirname -- "$0")/manual_install.py" "$@"
