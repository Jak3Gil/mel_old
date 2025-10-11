#!/bin/bash
# Collect Qualitative Samples Weekly
# Stores human-readable evidence of reasoning alongside metrics
# Usage: ./collect_qualitative_samples.sh

set -e

SAMPLES_DIR="qualitative_samples"
mkdir -p "$SAMPLES_DIR"

DATE=$(date +%Y%m%d)
SAMPLE_FILE="$SAMPLES_DIR/samples_$DATE.txt"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║         📝 COLLECT WEEKLY QUALITATIVE SAMPLES                 ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Get current metrics
CURRENT_SIM=$(tail -1 diagnostics_history.csv 2>/dev/null | cut -d',' -f5 || echo "0.000")
CURRENT_ENTROPY=$(tail -1 diagnostics_history.csv 2>/dev/null | cut -d',' -f4 || echo "0.000")

echo "📊 Current similarity: $CURRENT_SIM"
echo "📊 Current entropy reduction: $CURRENT_ENTROPY"
echo ""

# Standard test prompts to collect
TEST_PROMPTS=(
    "fire→water"
    "music→emotion"
    "robot→person"
    "thought→memory"
)

# Initialize sample file
cat > "$SAMPLE_FILE" << EOF
═══════════════════════════════════════════════════════════════
QUALITATIVE SAMPLES - $(date)
═══════════════════════════════════════════════════════════════

Current Metrics:
  Context Similarity:  $CURRENT_SIM
  Entropy Reduction:   $CURRENT_ENTROPY
  Leap Success:        $(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%

Purpose:
  Human-readable evidence of reasoning quality.
  These samples complement quantitative metrics.
  
Test Prompts: ${TEST_PROMPTS[@]}

═══════════════════════════════════════════════════════════════

EOF

echo "🧪 Collecting samples for ${#TEST_PROMPTS[@]} test prompts..."
echo ""

# For each prompt, extract from latest diagnostic
for prompt in "${TEST_PROMPTS[@]}"; do
    echo "Testing: $prompt"
    
    # Extract from CSV
    RESULT=$(grep "^$prompt," leap_diagnostics.csv 2>/dev/null | tail -1 || echo "")
    
    if [ -z "$RESULT" ]; then
        cat >> "$SAMPLE_FILE" << EOF
Test: $prompt
  Status: No data available
  
EOF
        echo "   → No data"
        continue
    fi
    
    # Parse result
    ENTROPY_RED=$(echo "$RESULT" | cut -d',' -f4)
    SIM=$(echo "$RESULT" | cut -d',' -f8)
    SUCCESS=$(echo "$RESULT" | cut -d',' -f10)
    TOP1=$(echo "$RESULT" | cut -d',' -f11)
    TOP2=$(echo "$RESULT" | cut -d',' -f12)
    TOP3=$(echo "$RESULT" | cut -d',' -f13)
    COMMENT=$(echo "$RESULT" | cut -d',' -f14)
    
    # Determine link quality
    LINK_QUALITY="❌ No link"
    if (( $(echo "$ENTROPY_RED > 0.20 && $SIM > 0.40" | bc -l 2>/dev/null || echo 0) )); then
        LINK_QUALITY="🟢 Strong"
    elif (( $(echo "$ENTROPY_RED > 0.10 || $SIM > 0.25" | bc -l 2>/dev/null || echo 0) )); then
        LINK_QUALITY="🟡 Moderate"
    elif (( $(echo "$ENTROPY_RED > 0.03 || $SIM > 0.10" | bc -l 2>/dev/null || echo 0) )); then
        LINK_QUALITY="🟠 Weak"
    fi
    
    cat >> "$SAMPLE_FILE" << EOF
Test: $prompt
  Link Quality:      $LINK_QUALITY
  Entropy Reduction: $ENTROPY_RED
  Similarity:        $SIM
  Predictions:       $TOP1, $TOP2, $TOP3
  Comment:           $COMMENT
  
EOF

    echo "   → $LINK_QUALITY"
    echo "   Predictions: $TOP1, $TOP2, $TOP3"
done

echo ""

# Add interpretation section
cat >> "$SAMPLE_FILE" << EOF
═══════════════════════════════════════════════════════════════
INTERPRETATION:
═══════════════════════════════════════════════════════════════

EOF

# Count link qualities
STRONG=$(grep -c "🟢 Strong" "$SAMPLE_FILE" || echo "0")
MODERATE=$(grep -c "🟡 Moderate" "$SAMPLE_FILE" || echo "0")
WEAK=$(grep -c "🟠 Weak" "$SAMPLE_FILE" || echo "0")
NONE=$(grep -c "❌ No link" "$SAMPLE_FILE" || echo "0")

cat >> "$SAMPLE_FILE" << EOF
Summary:
  Strong links:    $STRONG / ${#TEST_PROMPTS[@]}
  Moderate links:  $MODERATE / ${#TEST_PROMPTS[@]}
  Weak links:      $WEAK / ${#TEST_PROMPTS[@]}
  No links:        $NONE / ${#TEST_PROMPTS[@]}

Overall Assessment:
EOF

TOTAL_SUCCESS=$((STRONG + MODERATE))
if (( TOTAL_SUCCESS >= 3 )); then
    echo "  ✅ Good - Conceptual reasoning evident" >> "$SAMPLE_FILE"
elif (( TOTAL_SUCCESS >= 2 )); then
    echo "  📈 Moderate - Reasoning emerging" >> "$SAMPLE_FILE"
elif (( WEAK >= 2 )); then
    echo "  ⏳ Developing - Faint patterns appearing" >> "$SAMPLE_FILE"
else
    echo "  ⚠️  Early stage - More data needed" >> "$SAMPLE_FILE"
fi

cat >> "$SAMPLE_FILE" << EOF

These qualitative samples provide human-readable evidence that
complements the quantitative metrics. When predictions make
intuitive sense AND numbers are good, reasoning is real.

═══════════════════════════════════════════════════════════════
EOF

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Qualitative samples collected: $SAMPLE_FILE"
echo ""
echo "📊 Summary:"
echo "   Strong links:   $STRONG / ${#TEST_PROMPTS[@]}"
echo "   Moderate links: $MODERATE / ${#TEST_PROMPTS[@]}"
echo "   Weak links:     $WEAK / ${#TEST_PROMPTS[@]}"
echo ""

# Archive if this is a milestone
if (( $(echo "$CURRENT_SIM >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
    cp "$SAMPLE_FILE" "$SNAPSHOT_DIR/" 2>/dev/null || true
fi

# Git commit
if [ -d .git ]; then
    git add "$SAMPLE_FILE" 2>/dev/null || true
    git add "$SAMPLES_DIR/" 2>/dev/null || true
    
    if ! git diff --cached --quiet 2>/dev/null; then
        git commit -m "📝 Weekly qualitative samples - $(date +%Y-%m-%d)

Similarity: $CURRENT_SIM
Strong links: $STRONG, Moderate: $MODERATE, Weak: $WEAK

Human-readable evidence of reasoning quality." 2>/dev/null || true
        
        echo "✅ Samples committed to Git"
    fi
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 USAGE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Run this EVERY WEEK to build human-readable evidence:"
echo ""
echo "   $ ./collect_qualitative_samples.sh"
echo ""
echo "Over time, you'll have a series showing reasoning emergence:"
echo ""
echo "   Week 1: samples_20251011.txt  [All ❌ No links]"
echo "   Week 2: samples_20251018.txt  [Some 🟠 Weak links]"
echo "   Week 4: samples_20251025.txt  [Several 🟡 Moderate]"
echo "   Week 6: samples_20251101.txt  [Many 🟢 Strong] ← Breakthrough!"
echo ""
echo "These samples prove reasoning is REAL, not just good numbers."
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Qualitative evidence archived. Numbers + meaning tracked.   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

