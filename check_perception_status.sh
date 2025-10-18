#!/bin/bash
# Quick status check for live perception

echo "👁️  Checking Melvin Visual Perception Status..."
echo ""

# Check if process is running
if pgrep -f "live_camera_perception.py" > /dev/null; then
    echo "✅ Live perception is RUNNING"
    echo ""
    echo "Process info:"
    ps aux | grep "live_camera_perception.py" | grep -v grep
    echo ""
    echo "Controls:"
    echo "  Press 'q' in the video window to quit"
    echo "  Press 's' in the video window to show stats"
else
    echo "⚠️  Live perception is NOT running"
    echo ""
    echo "To start:"
    echo "  python3 live_camera_perception.py --camera 1"
fi

echo ""

# Check for snapshot file
if [ -f "perception_graph_snapshot.json" ]; then
    echo "📊 Graph snapshot found:"
    echo "  File: perception_graph_snapshot.json"
    
    # Quick stats from JSON
    if command -v jq &> /dev/null; then
        echo ""
        echo "Quick stats:"
        jq '.metadata' perception_graph_snapshot.json 2>/dev/null || echo "  (install jq for JSON parsing)"
    fi
else
    echo "📊 No graph snapshot yet (will be created when you quit)"
fi

echo ""


