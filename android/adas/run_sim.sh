#!/usr/bin/env bash
# Run MetaDrive sim from project root: ./run_sim.sh --controller pure_pursuit --show
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRIPTS="$ROOT/app/src/main/scripts"
export PYTHONPATH="$SCRIPTS${PYTHONPATH:+:$PYTHONPATH}"
export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python
cd "$SCRIPTS"
exec python3 -m sim.main "$@"
