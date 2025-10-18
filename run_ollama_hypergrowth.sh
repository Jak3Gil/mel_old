#!/bin/bash
# OLLAMA HYPERGROWTH MODE - Using Llama 3.2 for HIGH-QUALITY facts!

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  🧠 OLLAMA HYPERGROWTH MODE - MELVIN + LLAMA 3.2!            ║"
echo "║     100% LOCAL, HIGH-QUALITY AI-GENERATED FACTS!             ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "Press Ctrl+C to stop"
echo ""

CYCLE=0

while true; do
    CYCLE=$((CYCLE + 1))
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  🧠 OLLAMA CYCLE $CYCLE - $(date)"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    # Generate HIGH-QUALITY facts using Ollama
    echo "🧠 Generating 100 facts with Llama 3.2..."
    python3 ollama_scraper.py 100 > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
        echo "   ✓ Generated successfully"
    else
        echo "   ⚠️  Ollama generation failed"
        echo "   Make sure Ollama is running: ollama serve"
        sleep 30
        continue
    fi
    
    echo ""
    
    # MEGA ingestion
    echo "⚡ Ultra-fast ingestion..."
    ./optimized_melvin_demo > /tmp/melvin_ollama.log 2>&1
    
    # Extract stats
    FACTS=$(grep "Facts ingested:" /tmp/melvin_ollama.log | tail -1 | awk '{print $3}')
    SPEED=$(grep "Processing speed:" /tmp/melvin_ollama.log | tail -1 | awk '{print $3}')
    NODES=$(grep "Nodes:" /tmp/melvin_ollama.log | tail -1 | awk '{print $2}')
    EDGES=$(grep "Edges:" /tmp/melvin_ollama.log | tail -1 | awk '{print $2}')
    
    echo "   ✓ Processed $FACTS facts at $SPEED facts/sec"
    echo "   📊 Knowledge base: $NODES nodes, $EDGES edges"
    echo ""
    
    # Summary
    echo "╭─────────────────────────────────────────────────────────────╮"
    echo "│  🧠 OLLAMA CYCLE $CYCLE COMPLETE                             "
    echo "├─────────────────────────────────────────────────────────────┤"
    echo "│  Facts: $FACTS (AI-generated)                                "
    echo "│  Speed: $SPEED facts/sec                                     "
    echo "│  Total nodes: $NODES                                         "
    echo "│  Total edges: $EDGES                                         "
    echo "│                                                              "
    echo "│  🔥 AI-POWERED LEARNING ACTIVE!                              "
    echo "╰─────────────────────────────────────────────────────────────╯"
    echo ""
    
    # Quick cycle for hypergrowth
    echo "⏱️  Waiting 10 seconds before next cycle..."
    echo ""
    sleep 10
done

