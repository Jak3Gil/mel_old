#!/bin/bash

# macOS .command file - double-click to run!
# Shows what Melvin sees through USB camera

cd "$(dirname "$0")/melvin"

clear
echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  👁️ MELVIN CAMERA VISION                                 ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "🔍 Auto-detecting camera..."

# Run Melvin's vision with auto-detect
python3 melvin_sees.py 2>/dev/null

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ Camera not available"
    echo ""
    echo "Running test visualization instead..."
    sleep 2
    python3 visualize_melvin.py 2>/dev/null
fi

echo ""
echo "Press any key to close..."
read -n 1

