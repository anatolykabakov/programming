#!/bin/bash
set -e
cd "$(dirname "$0")"
PROTO_DIR="../proto"
OUTPUT_DIR="./vis/proto"
mkdir -p "$OUTPUT_DIR"

PROTOC="${PROTOC:-}"
if [ -z "$PROTOC" ]; then
  if [ -x "$HOME/.local/protoc/bin/protoc" ]; then
    PROTOC="$HOME/.local/protoc/bin/protoc"
  elif command -v protoc >/dev/null 2>&1; then
    PROTOC="$(command -v protoc)"
  else
    echo "protoc not found (install or set PROTOC=)"
    exit 1
  fi
fi

echo "Using $PROTOC ($($PROTOC --version))"
"$PROTOC" --proto_path="$PROTO_DIR" --proto_path="$HOME/.local/protoc/include" \
  --python_out="$OUTPUT_DIR" "$PROTO_DIR"/*.proto
ls -1 "$OUTPUT_DIR"/*_pb2.py
