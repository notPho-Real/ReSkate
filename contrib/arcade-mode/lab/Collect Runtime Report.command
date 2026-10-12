#!/bin/bash
cd -- "$(dirname -- "$0")" || exit 1
python3 -B controller.py collect "$@"
result=$?
printf "\nPress Return to close..."
read -r ignored
exit "$result"
