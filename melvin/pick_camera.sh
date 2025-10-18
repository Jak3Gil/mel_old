#!/bin/bash

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  📹 Select Camera for Melvin                             ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Run camera test
python3 test_camera.py

echo ""
echo "Which camera do you want to use with Melvin?"
echo ""
echo "  0) Camera 0 (usually built-in)"
echo "  1) Camera 1 (usually external or continuity)"
echo "  t) Test camera live first"
echo "  q) Quit"
echo ""
read -p "Enter choice: " choice

case $choice in
    0)
        echo ""
        echo "▶️  Starting Melvin with Camera 0..."
        python3 -c "
import sys
sys.path.insert(0, '.')
from visualize_camera import MelvinCameraVision

viz = MelvinCameraVision(camera_id=0)
viz.run()
"
        ;;
    1)
        echo ""
        echo "▶️  Starting Melvin with Camera 1..."
        python3 -c "
import sys
sys.path.insert(0, '.')
from visualize_camera import MelvinCameraVision

viz = MelvinCameraVision(camera_id=1)
viz.run()
"
        ;;
    t)
        echo ""
        echo "Testing cameras..."
        echo "Press 'q' in the video window to close"
        python3 test_camera.py test 0
        ;;
    q)
        echo "Goodbye! 👋"
        exit 0
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "✅ Done!"

