#!/usr/bin/env bash
set -euo pipefail
[[ $# == 1 ]] || { echo "Expected 1 path argument(s)" >&2; exit 1; }
exec python3 -B "$(dirname -- "$0")/manual_install.py" "$@"
