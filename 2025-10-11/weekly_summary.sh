#!/bin/bash
# Weekly Summary Script - Computes rolling averages and progress metrics
# Usage: ./weekly_summary.sh [weeks]

set -e

HISTORY_FILE="diagnostics_history.csv"
WEEKS="${1:-4}"  # Default to 4 weeks of history

if [ ! -f "$HISTORY_FILE" ]; then
    echo "❌ Error: $HISTORY_FILE not found"
    exit 1
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              📊 WEEKLY PROGRESS SUMMARY                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Count total entries (excluding header)
TOTAL_ENTRIES=$(tail -n +2 "$HISTORY_FILE" | wc -l | tr -d ' ')
echo "📅 Total diagnostic runs: $TOTAL_ENTRIES"
echo "📈 Analysis period: Last $WEEKS weeks"
echo ""

# Get recent entries
RECENT_ENTRIES=$(tail -n $((WEEKS + 1)) "$HISTORY_FILE" | tail -n +2)

if [ -z "$RECENT_ENTRIES" ]; then
    echo "⚠️  No recent data to analyze"
    exit 0
fi

# Compute averages using awk
echo "$RECENT_ENTRIES" | awk -F',' '
BEGIN {
    sum_entropy = 0
    sum_similarity = 0
    sum_success = 0
    count = 0
    max_entropy = 0
    max_similarity = 0
    max_success = 0
}
{
    if (NF >= 6) {
        entropy = $4
        similarity = $5
        success = $6
        
        sum_entropy += entropy
        sum_similarity += similarity
        sum_success += success
        
        if (entropy > max_entropy) max_entropy = entropy
        if (similarity > max_similarity) max_similarity = similarity
        if (success > max_success) max_success = success
        
        count++
    }
}
END {
    if (count > 0) {
        avg_entropy = sum_entropy / count
        avg_similarity = sum_similarity / count
        avg_success = sum_success / count
        
        print "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        print "📊 ROLLING AVERAGES (Last " count " runs):"
        print ""
        printf "   Entropy Reduction:   %.3f  (max: %.3f)\n", avg_entropy, max_entropy
        printf "   Context Similarity:  %.3f  (max: %.3f)\n", avg_similarity, max_similarity
        printf "   Leap Success Rate:   %.1f%% (max: %.1f%%)\n", avg_success*100, max_success*100
        print ""
    }
}'

# Get first and last entry for trend analysis
FIRST_ENTRY=$(tail -n $((WEEKS + 1)) "$HISTORY_FILE" | tail -n +2 | head -1)
LAST_ENTRY=$(tail -1 "$HISTORY_FILE")

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📈 TREND ANALYSIS:"
echo ""

# Extract values
FIRST_ENTROPY=$(echo "$FIRST_ENTRY" | cut -d',' -f4)
LAST_ENTROPY=$(echo "$LAST_ENTRY" | cut -d',' -f4)
FIRST_SIM=$(echo "$FIRST_ENTRY" | cut -d',' -f5)
LAST_SIM=$(echo "$LAST_ENTRY" | cut -d',' -f5)
FIRST_SUCCESS=$(echo "$FIRST_ENTRY" | cut -d',' -f6)
LAST_SUCCESS=$(echo "$LAST_ENTRY" | cut -d',' -f6)

# Compute deltas
ENTROPY_DELTA=$(echo "$LAST_ENTROPY - $FIRST_ENTROPY" | bc -l 2>/dev/null || echo "0")
SIM_DELTA=$(echo "$LAST_SIM - $FIRST_SIM" | bc -l 2>/dev/null || echo "0")
SUCCESS_DELTA=$(echo "$LAST_SUCCESS - $FIRST_SUCCESS" | bc -l 2>/dev/null || echo "0")

printf "   Entropy Reduction:   %.3f → %.3f " "$FIRST_ENTROPY" "$LAST_ENTROPY"
if (( $(echo "$ENTROPY_DELTA > 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(+%.3f) ✅\n" "$ENTROPY_DELTA"
elif (( $(echo "$ENTROPY_DELTA < 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(%.3f) ⚠️\n" "$ENTROPY_DELTA"
else
    printf "(no change)\n"
fi

printf "   Context Similarity:  %.3f → %.3f " "$FIRST_SIM" "$LAST_SIM"
if (( $(echo "$SIM_DELTA > 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(+%.3f) ✅\n" "$SIM_DELTA"
elif (( $(echo "$SIM_DELTA < 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(%.3f) ⚠️\n" "$SIM_DELTA"
else
    printf "(no change)\n"
fi

printf "   Leap Success Rate:   %.1f%% → %.1f%% " "$(echo "$FIRST_SUCCESS * 100" | bc -l 2>/dev/null || echo 0)" "$(echo "$LAST_SUCCESS * 100" | bc -l 2>/dev/null || echo 0)"
SUCCESS_DELTA_PCT=$(echo "$SUCCESS_DELTA * 100" | bc -l 2>/dev/null || echo "0")
if (( $(echo "$SUCCESS_DELTA > 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(+%.1f%%) ✅\n" "$SUCCESS_DELTA_PCT"
elif (( $(echo "$SUCCESS_DELTA < 0" | bc -l 2>/dev/null || echo 0) )); then
    printf "(%.1f%%) ⚠️\n" "$SUCCESS_DELTA_PCT"
else
    printf "(no change)\n"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 MILESTONE PROGRESS:"
echo ""

# Check milestones
CURRENT_ENTROPY=$(echo "$LAST_ENTROPY" | bc -l 2>/dev/null || echo "0")
CURRENT_SIM=$(echo "$LAST_SIM" | bc -l 2>/dev/null || echo "0")
CURRENT_SUCCESS=$(echo "$LAST_SUCCESS" | bc -l 2>/dev/null || echo "0")

# Milestone 1: Basic associations
if (( $(echo "$CURRENT_SIM >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
    echo "   ✅ Milestone 1: Basic Associations (similarity ≥0.35)"
else
    NEEDED=$(echo "0.35 - $CURRENT_SIM" | bc -l 2>/dev/null || echo "0.35")
    printf "   ⏳ Milestone 1: Need +%.3f similarity\n" "$NEEDED"
fi

# Milestone 2: Coherence begins
if (( $(echo "$CURRENT_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
    echo "   🎉 Milestone 2: COHERENCE BEGINS (similarity ≥0.50)"
else
    NEEDED=$(echo "0.50 - $CURRENT_SIM" | bc -l 2>/dev/null || echo "0.50")
    printf "   ⏳ Milestone 2: Need +%.3f similarity (CRITICAL)\n" "$NEEDED"
fi

# Milestone 3: Causal reasoning
if (( $(echo "$CURRENT_SIM >= 0.65" | bc -l 2>/dev/null || echo 0) )); then
    echo "   🚀 Milestone 3: Causal Reasoning (similarity ≥0.65)"
else
    NEEDED=$(echo "0.65 - $CURRENT_SIM" | bc -l 2>/dev/null || echo "0.65")
    printf "   ⏳ Milestone 3: Need +%.3f similarity\n" "$NEEDED"
fi

# Milestone 4: Abstract mastery
if (( $(echo "$CURRENT_SIM >= 0.75" | bc -l 2>/dev/null || echo 0) )); then
    echo "   ⭐ Milestone 4: Abstract Concept Mastery (similarity ≥0.75)"
else
    NEEDED=$(echo "0.75 - $CURRENT_SIM" | bc -l 2>/dev/null || echo "0.75")
    printf "   ⏳ Milestone 4: Need +%.3f similarity\n" "$NEEDED"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 RECOMMENDATIONS:"
echo ""

# Provide specific recommendations based on current state
if (( $(echo "$CURRENT_SIM < 0.20" | bc -l 2>/dev/null || echo 1) )); then
    echo "   🎯 Priority: Add Layer 1 data (conversations)"
    echo "      - Need 10k-50k dialog exchanges"
    echo "      - Focus on high-quality, natural conversations"
    echo "      - Run diagnostic after every 5k additions"
elif (( $(echo "$CURRENT_SIM < 0.40" | bc -l 2>/dev/null || echo 1) )); then
    echo "   🎯 Priority: Continue Layer 1, begin embeddings training"
    echo "      - Run 100+ prediction cycles with feedback"
    echo "      - Check nearest neighbors for intuition"
    echo "      - Add more conversational variety"
elif (( $(echo "$CURRENT_SIM < 0.50" | bc -l 2>/dev/null || echo 1) )); then
    echo "   🎯 Priority: Push toward 0.5 coherence threshold!"
    echo "      - Add Layer 2 data (factual knowledge)"
    echo "      - Increase embedding learning rate if stuck"
    echo "      - Run qualitative prompt checks"
elif (( $(echo "$CURRENT_SIM < 0.65" | bc -l 2>/dev/null || echo 1) )); then
    echo "   🎯 Priority: Build causal reasoning capability"
    echo "      - Add Layer 3 data (Q&A, explanations)"
    echo "      - Test multi-hop reasoning chains"
    echo "      - Monitor leap success rate"
else
    echo "   🎯 Priority: Achieve mastery with abstract concepts"
    echo "      - Add Layer 4 data (philosophy, metaphors)"
    echo "      - Test analogical reasoning"
    echo "      - Fine-tune all parameters"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 RECENT HISTORY:"
echo ""

# Show last 5 entries in table format
tail -6 "$HISTORY_FILE" | column -t -s',' | head -6

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Run this weekly to track progress toward 0.5 threshold      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

