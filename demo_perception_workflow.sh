#!/bin/bash
# Demo workflow: Download sample image, run YOLO, build graph

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  👁️  Melvin Perception Demo Workflow                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Create temp directory for demo
DEMO_DIR="perception_demo"
mkdir -p $DEMO_DIR

echo "[Step 1] Downloading sample image..."
# Download a sample image (people on a street)
if command -v curl &> /dev/null; then
    curl -L "https://ultralytics.com/images/bus.jpg" -o $DEMO_DIR/sample.jpg
elif command -v wget &> /dev/null; then
    wget "https://ultralytics.com/images/bus.jpg" -O $DEMO_DIR/sample.jpg
else
    echo "Error: Neither curl nor wget found. Please download manually:"
    echo "  https://ultralytics.com/images/bus.jpg"
    exit 1
fi

echo ""
echo "[Step 2] Running YOLO detection..."
python3 melvin/io/detect_objects.py $DEMO_DIR/sample.jpg | tee $DEMO_DIR/detections.json
echo ""

echo "[Step 3] Building knowledge graph..."
./test_perception $DEMO_DIR/sample.jpg | tee $DEMO_DIR/output.log

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅  Demo Complete!                                            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Results saved to: $DEMO_DIR/"
echo "  - sample.jpg         (input image)"
echo "  - detections.json    (YOLO output)"
echo "  - output.log         (processing log)"
echo ""
echo "Knowledge graph files:"
echo "  - nodes.melvin"
echo "  - edges.melvin"
echo ""


