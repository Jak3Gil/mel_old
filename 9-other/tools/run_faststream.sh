#!/bin/bash
#
# High-Throughput Streaming Pipeline Launcher
#
# Starts both the ingestion server and data streamer
# Handles cleanup on exit
#

# Trap Ctrl+C to cleanup child processes
cleanup() {
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  SHUTTING DOWN PIPELINE"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    if [ ! -z "$SERVER_PID" ]; then
        echo "🛑 Stopping ingestion server (PID $SERVER_PID)..."
        kill -TERM $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    fi
    
    if [ ! -z "$STREAMER_PID" ]; then
        echo "🛑 Stopping data streamer (PID $STREAMER_PID)..."
        kill -TERM $STREAMER_PID 2>/dev/null
        wait $STREAMER_PID 2>/dev/null
    fi
    
    echo ""
    echo "✅ Pipeline shutdown complete"
    echo ""
    
    exit 0
}

trap cleanup SIGINT SIGTERM

# ============================================================================
# START PIPELINE
# ============================================================================

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  STEP 1: Starting Ingestion Server"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Start server in background
./tools/stream_ingest &
SERVER_PID=$!

# Give server time to start
sleep 2

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "✗ Server failed to start"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  STEP 2: Starting Data Streamer"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Start streamer in background
python3 scripts/data_streamer_fast.py &
STREAMER_PID=$!

# Give streamer time to start
sleep 1

# Check if streamer is running
if ! kill -0 $STREAMER_PID 2>/dev/null; then
    echo "✗ Streamer failed to start"
    kill -TERM $SERVER_PID 2>/dev/null
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  PIPELINE RUNNING"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  Server PID:     $SERVER_PID"
echo "  Streamer PID:   $STREAMER_PID"
echo ""
echo "  Press Ctrl+C to stop"
echo ""

# Wait for both processes
wait

