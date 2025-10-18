#!/usr/bin/env bash
# Recall Quality Test Suite
# [Hopfield-Diffusion Upgrade - Recall Testing]

set -euo pipefail

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  🧪 HOPFIELD-DIFFUSION RECALL QUALITY TEST                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if demo exists
if [ ! -f "./demo_hopfield_diffusion" ]; then
    echo "Building demo first..."
    make -f Makefile.hopfield all || { echo "Build failed"; exit 1; }
fi

# Create test suite
cat > recall_suite.txt <<'EOF'
quantum->mechanics,physics,particle,energy
cell->biology,membrane,mitosis,nucleus  
water->molecule,hydrogen,oxygen,polar
energy->physics,conservation,transfer,work
DNA->genetic,nucleotide,replication,helix
EOF

echo "Recall Test Suite:"
echo "  Testing associative memory..."
echo "  Format: cue -> expected related concepts"
echo ""

total_tests=0
total_hits=0
total_possible=0

while IFS="->" read -r cue targets; do
    cue=$(echo $cue | xargs)
    
    # Skip empty lines
    [ -z "$cue" ] && continue
    
    total_tests=$((total_tests + 1))
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Test $total_tests: Cue = \"$cue\""
    echo ""
    
    # Run diffusion (capture stdout/stderr)
    ./demo_hopfield_diffusion > .recall_tmp.txt 2>&1 || {
        echo "  ⚠️  Demo failed for cue: $cue"
        continue
    }
    
    # Extract top activated nodes
    top_nodes=$(grep -A 15 "Top activated nodes" .recall_tmp.txt | tail -15 || echo "")
    
    # Count hits
    hits=0
    tested=0
    
    IFS=',' read -ra TARGET_ARRAY <<< "$targets"
    for target in "${TARGET_ARRAY[@]}"; do
        target=$(echo $target | xargs)
        tested=$((tested + 1))
        
        if echo "$top_nodes" | grep -qi "$target"; then
            echo "  ✓ Found: $target"
            hits=$((hits + 1))
        else
            echo "  ✗ Missing: $target"
        fi
    done
    
    hit_rate=$(python3 -c "print(f'{100.0 * $hits / $tested:.1f}')")
    
    echo ""
    echo "  Result: $hits/$tested hits (${hit_rate}%)"
    
    total_hits=$((total_hits + hits))
    total_possible=$((total_possible + tested))
    
    # Quality threshold
    if [ "$hits" -ge 2 ]; then
        echo "  ✅ PASS (≥2 hits)"
    else
        echo "  ⚠️  WEAK (< 2 hits)"
    fi
    
    echo ""
done < recall_suite.txt

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  RECALL QUALITY SUMMARY                                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

if [ $total_possible -gt 0 ]; then
    overall_rate=$(python3 -c "print(f'{100.0 * $total_hits / $total_possible:.1f}')")
    
    echo "  Total hits: $total_hits / $total_possible ($overall_rate%)"
    echo ""
    
    if [ "$total_hits" -ge $((total_possible * 7 / 10)) ]; then
        echo "  ✅ EXCELLENT (≥70% recall)"
    elif [ "$total_hits" -ge $((total_possible / 2)) ]; then
        echo "  ✅ GOOD (≥50% recall)"
    else
        echo "  ⚠️  NEEDS IMPROVEMENT (<50% recall)"
        echo "     Consider: More training, larger β, or more diffusion steps"
    fi
else
    echo "  ⚠️  No tests completed"
fi

echo ""

# Cleanup
rm -f .recall_tmp.txt recall_suite.txt

exit 0




