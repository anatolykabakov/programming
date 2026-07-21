#!/usr/bin/env bash
# Run MetaDrive sim from project root: ./run_sim.sh --controller pure_pursuit --show
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS="$ROOT/app/src/main/scripts"
export PYTHONPATH="$SCRIPTS${PYTHONPATH:+:$PYTHONPATH}"
cd "$SCRIPTS"
exec python3 -m sim.main "$@"
