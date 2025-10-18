#!/bin/bash
# Quick-start script for Melvin Fast Visual Perception

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  🚀 MELVIN FAST VISUAL PERCEPTION - QUICK START                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Check OpenCV
echo "[1/4] Checking OpenCV installation..."
if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    echo "✅ OpenCV $OPENCV_VERSION found"
else
    echo "❌ OpenCV not found"
    echo ""
    echo "Install OpenCV:"
    echo "  macOS:  brew install opencv"
    echo "  Ubuntu: sudo apt-get install libopencv-dev"
    echo ""
    exit 1
fi

# Step 2: Build
echo ""
echo "[2/4] Building C++ fast vision system..."
make -f Makefile.fast_vision clean
make -f Makefile.fast_vision

# Step 3: Check camera
echo ""
echo "[3/4] Detecting cameras..."
echo "Available cameras on macOS:"
system_profiler SPCameraDataType 2>/dev/null | grep "Model ID:" | nl -v 0 || echo "  (Run on macOS to detect cameras)"

# Step 4: Run
echo ""
echo "[4/4] Starting fast vision system..."
echo ""
echo "Configuration:"
echo "  Camera: 0 (change with: ./demo_fast_vision 1)"
echo "  FPS: 20"
echo "  Duration: unlimited (Ctrl+C to stop)"
echo ""
echo "Press Ctrl+C to stop or 'q' in video window"
echo ""

# Run with camera 0, 20 FPS, unlimited duration
./demo_fast_vision 0 20

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ SESSION COMPLETE                                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Knowledge graph saved to:"
echo "  melvin/data/nodes.melvin"
echo "  melvin/data/edges.melvin"
echo ""

