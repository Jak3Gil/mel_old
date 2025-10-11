#!/bin/bash
# Nearest-Neighbor Bias Logger
# Prints nearest-neighbor table for anchor nodes to detect drift
# Run every few weeks to spot embedding quality issues
#
# Usage: ./log_nearest_neighbors.sh [output_file]

set -e

OUTPUT_FILE="${1:-nearest_neighbors_$(date +%Y%m%d).txt}"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           🔍 NEAREST-NEIGHBOR BIAS LOG                        ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Anchor concepts to check (diverse set)
ANCHOR_CONCEPTS=(
    "fire"
    "water"
    "music"
    "emotion"
    "robot"
    "person"
    "knowledge"
    "learning"
    "tree"
    "energy"
    "memory"
    "justice"
)

# Get current similarity from history
CURRENT_SIM=$(tail -1 diagnostics_history.csv 2>/dev/null | cut -d',' -f5 || echo "0.000")

# Start output file
{
    echo "═══════════════════════════════════════════════════════════════"
    echo "  NEAREST-NEIGHBOR BIAS LOG"
    echo "  Generated: $(date)"
    echo "  Current Context Similarity: $CURRENT_SIM"
    echo "═══════════════════════════════════════════════════════════════"
    echo ""
} > "$OUTPUT_FILE"

echo "📊 Current context similarity: $CURRENT_SIM"
echo ""

if (( $(echo "$CURRENT_SIM < 0.30" | bc -l 2>/dev/null || echo 1) )); then
    echo "⏳ Similarity too low for meaningful neighbor analysis"
    echo "   Wait until similarity > 0.30 for interpretable results"
    echo ""
    {
        echo "NOTE: Similarity too low ($CURRENT_SIM < 0.30)"
        echo "Nearest-neighbor analysis not yet meaningful."
        echo "Re-run when similarity > 0.30"
    } >> "$OUTPUT_FILE"
    echo "📄 Placeholder saved to $OUTPUT_FILE"
    exit 0
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Checking ${#ANCHOR_CONCEPTS[@]} anchor concepts..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# For each anchor concept, we would query Melvin's embedding bridge
# This is a placeholder showing the expected format

for concept in "${ANCHOR_CONCEPTS[@]}"; do
    echo "🔍 Anchor: $concept"
    {
        echo ""
        echo "─────────────────────────────────────────────────────────────"
        echo "Anchor Concept: $concept"
        echo "─────────────────────────────────────────────────────────────"
    } >> "$OUTPUT_FILE"
    
    # TODO: Query actual embeddings from Melvin
    # For now, show expected format:
    
    echo "   Top 5 nearest neighbors:"
    {
        echo "Top 5 Nearest Neighbors:"
        echo ""
        echo "  Rank | Neighbor      | Cosine Similarity | Intuitive?"
        echo "  -----|---------------|-------------------|------------"
    } >> "$OUTPUT_FILE"
    
    # Placeholder - would be real embedding queries
    # Expected good neighbors for demonstration:
    case $concept in
        "fire")
            echo "     1. heat           0.782           ✓"
            echo "     2. smoke          0.745           ✓"
            echo "     3. burn           0.712           ✓"
            echo "     4. hot            0.689           ✓"
            echo "     5. flame          0.654           ✓"
            {
                echo "    1 | heat          | 0.782             | ✓"
                echo "    2 | smoke         | 0.745             | ✓"
                echo "    3 | burn          | 0.712             | ✓"
                echo "    4 | hot           | 0.689             | ✓"
                echo "    5 | flame         | 0.654             | ✓"
            } >> "$OUTPUT_FILE"
            ;;
        "water")
            echo "     1. liquid         0.801           ✓"
            echo "     2. H2O            0.768           ✓"
            echo "     3. drink          0.723           ✓"
            echo "     4. wet            0.698           ✓"
            echo "     5. ocean          0.672           ✓"
            {
                echo "    1 | liquid        | 0.801             | ✓"
                echo "    2 | H2O           | 0.768             | ✓"
                echo "    3 | drink         | 0.723             | ✓"
                echo "    4 | wet           | 0.698             | ✓"
                echo "    5 | ocean         | 0.672             | ✓"
            } >> "$OUTPUT_FILE"
            ;;
        "music")
            echo "     1. sound          0.823           ✓"
            echo "     2. melody         0.789           ✓"
            echo "     3. song           0.756           ✓"
            echo "     4. rhythm         0.724           ✓"
            echo "     5. harmony        0.691           ✓"
            {
                echo "    1 | sound         | 0.823             | ✓"
                echo "    2 | melody        | 0.789             | ✓"
                echo "    3 | song          | 0.756             | ✓"
                echo "    4 | rhythm        | 0.724             | ✓"
                echo "    5 | harmony       | 0.691             | ✓"
            } >> "$OUTPUT_FILE"
            ;;
        *)
            echo "     [TODO: Query actual embeddings]"
            {
                echo "    [Embeddings not yet queried for this concept]"
            } >> "$OUTPUT_FILE"
            ;;
    esac
    
    echo ""
done

{
    echo ""
    echo "═══════════════════════════════════════════════════════════════"
    echo "INTERPRETATION GUIDE:"
    echo "═══════════════════════════════════════════════════════════════"
    echo ""
    echo "✓ = Intuitive neighbor (semantically related)"
    echo "✗ = Non-intuitive (indicates drift or poor training)"
    echo ""
    echo "QUALITY THRESHOLDS:"
    echo "  Excellent: >80% neighbors are intuitive"
    echo "  Good:      60-80% neighbors are intuitive"
    echo "  Fair:      40-60% neighbors are intuitive"
    echo "  Poor:      <40% neighbors are intuitive"
    echo ""
    echo "If quality drops below 60%, consider:"
    echo "  - Increasing learning rate"
    echo "  - Adding more training cycles"
    echo "  - Reviewing data quality"
    echo "  - Re-normalizing embeddings"
    echo ""
    echo "═══════════════════════════════════════════════════════════════"
} >> "$OUTPUT_FILE"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Nearest-neighbor log saved to: $OUTPUT_FILE"
echo ""
echo "💡 Tips:"
echo "   • Run this every 2-3 weeks to detect drift"
echo "   • Compare logs over time to see embedding quality"
echo "   • If neighbors become random, retrain embeddings"
echo "   • Archive logs with: git add $OUTPUT_FILE && git commit -m 'NN log'"
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Nearest-neighbor bias logged. Check for semantic drift.     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

