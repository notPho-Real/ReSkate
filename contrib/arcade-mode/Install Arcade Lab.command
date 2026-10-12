#!/bin/bash
cd -- "$(dirname -- "$0")" || exit 1
python3 -B lab/controller.py bootstrap "$@"
result=$?
printf "\nPress Return to close..."
read -r ignored
exit "$result"
