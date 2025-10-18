#!/bin/bash
# Check live vision system status

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  👁️  LIVE VISION SYSTEM STATUS                                 ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if running
if pgrep -f "live_camera_perception.py" > /dev/null; then
    PID=$(pgrep -f "live_camera_perception.py")
    echo "✅ Status: RUNNING (PID: $PID)"
    echo ""
    
    # Show process info
    echo "Process Info:"
    ps aux | grep $PID | grep -v grep | awk '{printf "  CPU: %s%%  MEM: %s%%  TIME: %s\n", $3, $4, $10}'
    echo ""
    
    # Check for output files
    echo "Output Files:"
    if [ -f "perception_graph_snapshot.json" ]; then
        SIZE=$(ls -lh perception_graph_snapshot.json | awk '{print $5}')
        echo "  ✅ perception_graph_snapshot.json ($SIZE)"
        
        # Show quick stats
        if command -v jq &> /dev/null; then
            FRAMES=$(jq -r '.metadata.frames // 0' perception_graph_snapshot.json 2>/dev/null)
            NODES=$(jq '.nodes | length' perception_graph_snapshot.json 2>/dev/null)
            echo "     Frames: $FRAMES, Nodes: $NODES"
        fi
    else
        echo "  ⏳ Waiting for first snapshot..."
    fi
    echo ""
    
    echo "Controls:"
    echo "  - Press 'q' in video window to quit"
    echo "  - Press 's' in video window for stats"
    echo "  - Or run: pkill -f live_camera_perception"
    
else
    echo "❌ Status: NOT RUNNING"
    echo ""
    echo "To start:"
    echo "  python3 live_camera_perception.py --camera 1"
fi

echo ""

