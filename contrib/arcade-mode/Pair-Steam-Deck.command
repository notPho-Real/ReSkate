#!/bin/bash
cd -- "$(dirname -- "$0")" || exit 1
python3 deploy_to_deck.py --pair "$@"
result=$?
printf "\nPress Return to close..."
read -r ignored
exit "$result"
