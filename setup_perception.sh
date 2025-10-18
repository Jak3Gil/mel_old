#!/bin/bash
# Setup script for Melvin Visual Perception Engine

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  👁️  Melvin Visual Perception Setup                            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Install Python dependencies
echo "[1/3] Installing Python dependencies..."
pip3 install -r requirements.txt

# Step 2: Build core Melvin
echo ""
echo "[2/3] Building Melvin core..."
cd melvin
make clean
make
cd ..

# Step 3: Build perception module
echo ""
echo "[3/3] Building perception module..."
make -f Makefile.perception

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅  Setup Complete!                                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Next steps:"
echo "  1. Run tests: ./test_perception"
echo "  2. Test with image: ./test_perception path/to/image.jpg"
echo "  3. Read docs: cat README_PERCEPTION.md"
echo ""


