#!/bin/bash
#
# Streaming Demo - Quick demonstration of continuous learning
#

echo ""
echo "╔═══════════════════════════════════════════════════════╗"
echo "║  STREAMING DEMO - Continuous Learning                 ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""
echo "This demo shows Melvin learning continuously from a stream."
echo ""

# Save current brain state
echo "📊 Initial brain state:"
make stats 2>&1 | grep -E "(Total Nodes|Total Connections)" | head -2
echo ""

# Create temporary queue
QUEUE="data/stream_queue_demo.csv"
rm -f "$QUEUE"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  STEP 1: Generate streaming facts (10 seconds)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Run streamer briefly
cat > /tmp/quick_stream.py << 'EOF'
import sys
sys.path.insert(0, '/Users/jakegilbert/Desktop/Melvin/Melvin/melvin/scripts')
from stream_commoncrawl import simulate_web_stream, extract_facts, append_to_queue
import time

queue = "data/stream_queue_demo.csv"
batch = []

for i, text in enumerate(simulate_web_stream()):
    facts = extract_facts(text)
    if facts:
        batch.extend(facts)
        print(f"[Doc {i+1}] Extracted {len(facts)} facts")
    
    if i >= 9:  # 10 documents
        break

if batch:
    append_to_queue(batch, queue)
    print(f"\n✓ Queued {len(batch)} facts total")
EOF

python3 /tmp/quick_stream.py

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  STEP 2: Ingest into Melvin's brain"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$QUEUE" ]; then
    fact_count=$(($(wc -l < "$QUEUE") - 1))
    echo "📥 Ingesting $fact_count facts..."
    echo ""
    
    ./tools/melvin_ingest -t csv "$QUEUE" 2>&1 | grep -E "(Ingestion|Records|Final state)" | head -10
    
    rm -f "$QUEUE"
else
    echo "⚠️  No facts to ingest"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  STEP 3: Verify brain growth"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "📊 Updated brain state:"
make stats 2>&1 | grep -E "(Total Nodes|Total Connections)" | head -2
echo ""

echo "✅ Streaming demo complete!"
echo ""
echo "💡 For continuous learning, run:"
echo "   Terminal 1: ./scripts/watch_and_ingest.sh"
echo "   Terminal 2: python3 scripts/stream_commoncrawl.py"
echo ""

