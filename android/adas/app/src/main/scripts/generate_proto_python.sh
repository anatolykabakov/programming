#!/bin/bash
#
# Generate Python protobuf files from .proto definitions
#

set -e

PROTO_DIR="../proto"
OUTPUT_DIR="$PROTO_DIR"

echo "🔧 Generating Python protobuf files..."
echo "   Proto directory: $PROTO_DIR"
echo "   Output directory: $OUTPUT_DIR"
echo ""

cd "$(dirname "$0")"

# Check if protoc is installed
if ! command -v protoc &> /dev/null; then
    echo "❌ ERROR: protoc not found!"
    echo "   Please install protobuf compiler:"
    echo "   - Ubuntu/Debian: sudo apt-get install protobuf-compiler python3-protobuf"
    echo "   - MacOS: brew install protobuf"
    exit 1
fi

echo "✅ protoc found: $(protoc --version)"
echo ""

# Generate Python files
echo "📦 Generating Python protobuf files..."

protoc \
    --proto_path="$PROTO_DIR" \
    --python_out="$OUTPUT_DIR" \
    "$PROTO_DIR"/*.proto

echo ""
echo "✅ Generated Python protobuf files:"
ls -lh "$OUTPUT_DIR"/*_pb2.py 2>/dev/null || echo "   (No _pb2.py files found)"

echo ""
echo "🎉 Done! You can now use send_steering_command.py"











































