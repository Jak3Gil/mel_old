#!/bin/bash
# Continuous learning loop for Melvin

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  🚀 MELVIN CONTINUOUS LEARNING - ULTRA-FAST MODE              ║"
echo "║     Learning at 2,000-10,000 facts/sec!                      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Press Ctrl+C to stop"
echo ""

CYCLE=0

while true; do
    CYCLE=$((CYCLE + 1))
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  CYCLE $CYCLE - $(date)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    # Scrape fresh knowledge (100 facts per cycle for faster growth!)
    echo "📡 Scraping fresh knowledge..."
    python3 intelligent_scraper.py 100 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "   ✓ Scraped successfully"
    else
        echo "   ⚠️  Scraper failed, using existing data"
    fi
    echo ""
    
    # Fast ingestion
    echo "🧠 Ultra-fast ingestion..."
    ./optimized_melvin_demo > /tmp/melvin_cycle.log 2>&1
    
    # Extract stats
    FACTS=$(grep "Facts ingested:" /tmp/melvin_cycle.log | tail -1 | awk '{print $3}')
    SPEED=$(grep "Processing speed:" /tmp/melvin_cycle.log | tail -1 | awk '{print $3}')
    NODES=$(grep "Nodes:" /tmp/melvin_cycle.log | tail -1 | awk '{print $2}')
    EDGES=$(grep "Edges:" /tmp/melvin_cycle.log | tail -1 | awk '{print $2}')
    
    echo "   ✓ Processed $FACTS facts at $SPEED facts/sec"
    echo "   📊 Knowledge base: $NODES nodes, $EDGES edges"
    echo ""
    
    # Summary
    echo "╭─────────────────────────────────────────────────────────────╮"
    echo "│  CYCLE $CYCLE COMPLETE                                       "
    echo "├─────────────────────────────────────────────────────────────┤"
    echo "│  Facts: $FACTS                                               "
    echo "│  Speed: $SPEED facts/sec                                     "
    echo "│  Total nodes: $NODES                                         "
    echo "│  Total edges: $EDGES                                         "
    echo "╰─────────────────────────────────────────────────────────────╯"
    echo ""
    
    # Wait before next cycle
    echo "⏱️  Waiting 30 seconds before next cycle..."
    echo ""
    sleep 30
done

