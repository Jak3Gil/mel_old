#!/bin/bash
# HYPERGROWTH MODE - Maximum knowledge acquisition rate!

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  🚀 MELVIN HYPERGROWTH MODE - MAXIMUM LEARNING!              ║"
echo "║     Processing 500+ facts per cycle!                         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Press Ctrl+C to stop"
echo ""

CYCLE=0

while true; do
    CYCLE=$((CYCLE + 1))
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  HYPERGROWTH CYCLE $CYCLE - $(date)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    # Scrape LOTS of knowledge
    echo "📡 Scraping 500+ diverse facts from multiple sources..."
    
    # Wikipedia scraping (100 facts)
    python3 intelligent_scraper.py 100 > /dev/null 2>&1
    mv internet_facts.txt internet_facts_1.txt
    
    # More Wikipedia (100 facts)
    python3 intelligent_scraper.py 100 > /dev/null 2>&1  
    mv internet_facts.txt internet_facts_2.txt
    
    # Even more (100 facts)
    python3 intelligent_scraper.py 100 > /dev/null 2>&1
    mv internet_facts.txt internet_facts_3.txt
    
    # More variety (100 facts)
    python3 intelligent_scraper.py 100 > /dev/null 2>&1
    mv internet_facts.txt internet_facts_4.txt
    
    # Final batch (100 facts)
    python3 intelligent_scraper.py 100 > /dev/null 2>&1
    mv internet_facts.txt internet_facts_5.txt
    
    # Combine all facts
    cat internet_facts_*.txt > internet_facts_combined.txt
    TOTAL_FACTS=$(wc -l < internet_facts_combined.txt)
    
    echo "   ✓ Scraped $TOTAL_FACTS facts"
    echo ""
    
    # MEGA ingestion
    echo "🧠 MEGA ULTRA-FAST INGESTION..."
    
    # Use optimized demo with combined facts
    cp internet_facts_combined.txt internet_facts.txt
    ./optimized_melvin_demo > /tmp/melvin_hypergrowth.log 2>&1
    
    # Extract stats
    FACTS=$(grep "Facts ingested:" /tmp/melvin_hypergrowth.log | tail -1 | awk '{print $3}')
    SPEED=$(grep "Processing speed:" /tmp/melvin_hypergrowth.log | tail -1 | awk '{print $3}')
    NODES=$(grep "Nodes:" /tmp/melvin_hypergrowth.log | tail -1 | awk '{print $2}')
    EDGES=$(grep "Edges:" /tmp/melvin_hypergrowth.log | tail -1 | awk '{print $2}')
    
    echo "   ✓ Processed $FACTS facts at $SPEED facts/sec"
    echo "   📊 Knowledge base: $NODES nodes, $EDGES edges"
    echo ""
    
    # Cleanup
    rm -f internet_facts_*.txt
    
    # Summary
    echo "╭─────────────────────────────────────────────────────────────╮"
    echo "│  🚀 HYPERGROWTH CYCLE $CYCLE COMPLETE                        "
    echo "├─────────────────────────────────────────────────────────────┤"
    echo "│  Facts: $FACTS                                               "
    echo "│  Speed: $SPEED facts/sec                                     "
    echo "│  Total nodes: $NODES                                         "
    echo "│  Total edges: $EDGES                                         "
    echo "│                                                              "
    echo "│  🔥 MASSIVE GROWTH MODE ACTIVE!                              "
    echo "╰─────────────────────────────────────────────────────────────╯"
    echo ""
    
    # Shorter wait for hypergrowth
    echo "⏱️  Waiting 15 seconds before next HYPERGROWTH cycle..."
    echo ""
    sleep 15
done

