#!/bin/bash
#
# Watch and Ingest - Continuous Learning Loop
#
# Watches the stream queue and continuously ingests new facts into Melvin
# Works with stream_commoncrawl.py for live web learning
#

QUEUE_FILE="data/stream_queue.csv"
INGEST_INTERVAL=10  # Seconds between ingestion checks
MIN_FACTS=5         # Minimum facts before ingesting

echo ""
echo "╔═══════════════════════════════════════════════════════╗"
echo "║  WATCH & INGEST - Continuous Learning                 ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""
echo "⚙️  Configuration:"
echo "  Queue file:         $QUEUE_FILE"
echo "  Check interval:     $INGEST_INTERVAL seconds"
echo "  Min facts:          $MIN_FACTS"
echo ""
echo "🔄 Starting continuous ingestion loop..."
echo "   Press Ctrl+C to stop"
echo ""

iterations=0
total_ingested=0

while true; do
    # Check if queue file exists and has content
    if [ -f "$QUEUE_FILE" ]; then
        # Count lines (minus header)
        fact_count=$(($(wc -l < "$QUEUE_FILE") - 1))
        
        if [ $fact_count -ge $MIN_FACTS ]; then
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo "  INGESTION #$iterations"
            echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
            echo ""
            echo "📥 Found $fact_count facts in queue"
            
            # Ingest the facts
            ./tools/melvin_ingest -t csv "$QUEUE_FILE" | grep -E "(Ingestion|Final state|Connection types)"
            
            # Clear the queue
            rm -f "$QUEUE_FILE"
            
            total_ingested=$((total_ingested + fact_count))
            iterations=$((iterations + 1))
            
            echo ""
            echo "  ✓ Ingested $fact_count facts"
            echo "  📊 Total: $total_ingested facts in $iterations batches"
            echo ""
            
            # Show current brain state
            echo "🧠 Current brain state:"
            make stats | grep -E "(Total Nodes|Total Connections|EXACT|LEAP)" | head -4
            echo ""
        fi
    fi
    
    # Wait before checking again
    sleep $INGEST_INTERVAL
done

