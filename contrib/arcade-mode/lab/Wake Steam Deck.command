#!/bin/bash
cd -- "$(dirname -- "$0")" || exit 1
python3 -B wake.py
result=$?
read -r -p "Press Return to close. "
exit "$result"
