#!/bin/bash
cd -- "$(dirname -- "$0")" || exit 1
echo "Enter your Steam Deck administrator password at the remote sudo prompt."
echo "Typing is hidden. Nothing saves the password."
python3 -B enable_wifi_wake.py
result=$?
read -r -p "Press Return to close. "
exit "$result"
