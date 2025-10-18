#!/bin/bash
# Quick status check for Melvin

cat << 'EOF'

╔═══════════════════════════════════════════════════════════════╗
║  🧠 MELVIN STATUS CHECK                                       ║
╚═══════════════════════════════════════════════════════════════╝

EOF

echo "📊 CURRENT KNOWLEDGE BASE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Get stats from storage
if [ -f "melvin/data/nodes.melvin" ]; then
    # Count approximate nodes/edges from file size
    node_size=$(wc -c < melvin/data/nodes.melvin)
    edge_size=$(wc -c < melvin/data/edges.melvin)
    
    # Rough estimates (4 bytes per node header, 8 bytes per edge)
    echo "  Knowledge files:"
    echo "    nodes.melvin: $node_size bytes"
    echo "    edges.melvin: $edge_size bytes"
    echo ""
    
    # Run inspector for accurate count
    if [ -f "./inspect_kb" ]; then
        ./inspect_kb 2>&1 | grep "Total nodes\|Total edges\|EXACT\|LEAP" | head -8
    fi
else
    echo "  ⚠ No knowledge base found - run ./build_proper_graph first"
fi

echo ""
echo "🚀 AVAILABLE COMMANDS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Learning:"
echo "    ./learn_internet [cycles]           - Learn from Wikipedia API"
echo "    ./expand_diversity [count]          - Add diverse random facts"
echo "    ./learn_continuous [epochs]         - Reinforce knowledge"
echo ""
echo "  Testing:"
echo "    ./test_reasoning                    - Run test suite"
echo "    ./inspect_kb                        - View all nodes"
echo "    ./diagnose_plateau                  - Analyze growth patterns"
echo ""
echo "  Interactive:"
echo "    cd melvin && ./melvin               - Ask Melvin questions"
echo ""
echo "  Building:"
echo "    cd melvin && make                   - Rebuild core system"
echo ""

cat << 'EOF'

✨ Quick Start:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

  # Grow knowledge from internet
  ./learn_internet 20

  # Check what was learned
  ./inspect_kb | head -50

  # Test reasoning
  ./test_reasoning

See WHY_990_NODES.md for explanation of node plateau.

EOF

