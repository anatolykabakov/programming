#!/usr/bin/env bash
# Run bag visualizer from project root: ./run_bag_vis.sh /path/to/bag
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS="$ROOT/app/src/main/scripts"
export PYTHONPATH="$SCRIPTS${PYTHONPATH:+:$PYTHONPATH}"
exec python3 "$SCRIPTS/interactive_visualizer.py" "$@"
