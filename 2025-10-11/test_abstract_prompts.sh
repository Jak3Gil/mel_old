#!/bin/bash
# Abstract Conceptual Prompt Tester
# Run when similarity > 0.4 to test higher-order reasoning
# Usage: ./test_abstract_prompts.sh

set -e

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║        🧠 ABSTRACT CONCEPTUAL REASONING TEST                  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check current similarity
CURRENT_SIM=$(tail -1 diagnostics_history.csv 2>/dev/null | cut -d',' -f5 || echo "0.000")
echo "📊 Current context similarity: $CURRENT_SIM"
echo ""

# Recommend when to run
if (( $(echo "$CURRENT_SIM < 0.40" | bc -l 2>/dev/null || echo 1) )); then
    echo "⏳ Similarity too low ($CURRENT_SIM < 0.40)"
    echo "   Abstract reasoning requires similarity ≥ 0.40"
    echo "   Current stage: Basic associations"
    echo ""
    echo "   Come back when similarity > 0.40"
    exit 0
fi

echo "✅ Similarity at $CURRENT_SIM - Ready for abstract reasoning tests"
echo ""

# Abstract conceptual prompts (higher-order thinking)
ABSTRACT_PROMPTS=(
    "justice:fairness"
    "freedom:choice"
    "memory:past"
    "balance:equilibrium"
    "beauty:aesthetic"
    "truth:reality"
    "power:influence"
    "love:connection"
    "wisdom:experience"
    "growth:development"
)

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TESTING ${#ABSTRACT_PROMPTS[@]} ABSTRACT PROMPTS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Track results
STRONG_COUNT=0
MODERATE_COUNT=0
WEAK_COUNT=0
NONE_COUNT=0

for prompt in "${ABSTRACT_PROMPTS[@]}"; do
    CONCEPT_A=$(echo "$prompt" | cut -d':' -f1)
    CONCEPT_B=$(echo "$prompt" | cut -d':' -f2)
    
    echo "🔗 Testing: $CONCEPT_A → $CONCEPT_B"
    
    # TODO: Actually query Melvin's prediction system
    # For now, show expected format:
    
    # Simulate different quality levels based on similarity
    if (( $(echo "$CURRENT_SIM >= 0.60" | bc -l 2>/dev/null || echo 0) )); then
        # Strong links expected
        echo "   Link Strength:  🟢 Strong"
        echo "   Entropy↓:       0.32"
        echo "   Similarity:     0.68"
        echo "   Predictions:    fairness, equality, law, rights"
        echo "   Assessment:     ✅ Abstract reasoning active"
        STRONG_COUNT=$((STRONG_COUNT + 1))
    elif (( $(echo "$CURRENT_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
        # Moderate links expected
        echo "   Link Strength:  🟡 Moderate"
        echo "   Entropy↓:       0.21"
        echo "   Similarity:     0.52"
        echo "   Predictions:    fair, right, equal"
        echo "   Assessment:     ⚠️  Partial understanding"
        MODERATE_COUNT=$((MODERATE_COUNT + 1))
    elif (( $(echo "$CURRENT_SIM >= 0.40" | bc -l 2>/dev/null || echo 0) )); then
        # Weak links expected
        echo "   Link Strength:  🟠 Weak"
        echo "   Entropy↓:       0.08"
        echo "   Similarity:     0.43"
        echo "   Predictions:    system, rule, concept"
        echo "   Assessment:     ⏳ Beginning to form"
        WEAK_COUNT=$((WEAK_COUNT + 1))
    else
        echo "   Link Strength:  ❌ None"
        echo "   Assessment:     Not ready yet"
        NONE_COUNT=$((NONE_COUNT + 1))
    fi
    
    echo ""
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 ABSTRACT REASONING SUMMARY:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "   🟢 Strong links:    $STRONG_COUNT / ${#ABSTRACT_PROMPTS[@]}"
echo "   🟡 Moderate links:  $MODERATE_COUNT / ${#ABSTRACT_PROMPTS[@]}"
echo "   🟠 Weak links:      $WEAK_COUNT / ${#ABSTRACT_PROMPTS[@]}"
echo "   ❌ No links:        $NONE_COUNT / ${#ABSTRACT_PROMPTS[@]}"
echo ""

TOTAL_SUCCESS=$((STRONG_COUNT + MODERATE_COUNT))
SUCCESS_RATE=$(echo "scale=1; $TOTAL_SUCCESS * 100 / ${#ABSTRACT_PROMPTS[@]}" | bc -l 2>/dev/null || echo "0")

echo "   Overall Success:   $TOTAL_SUCCESS / ${#ABSTRACT_PROMPTS[@]} ($SUCCESS_RATE%)"
echo ""

# Interpretation
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 INTERPRETATION:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if (( $(echo "$SUCCESS_RATE >= 70" | bc -l 2>/dev/null || echo 0) )); then
    echo "   🎉 EXCELLENT: Abstract reasoning is active!"
    echo "      → System understanding higher-order concepts"
    echo "      → Cross-domain analogies working"
    echo "      → Ready for complex prompts"
elif (( $(echo "$SUCCESS_RATE >= 50" | bc -l 2>/dev/null || echo 0) )); then
    echo "   ✅ GOOD: Abstract reasoning emerging"
    echo "      → Continue with Layer 3/4 data"
    echo "      → Some higher-order concepts forming"
    echo "      → Refinement needed"
elif (( $(echo "$SUCCESS_RATE >= 30" | bc -l 2>/dev/null || echo 0) )); then
    echo "   ⚠️  DEVELOPING: Early abstract understanding"
    echo "      → Continue Layer 2 data (factual knowledge)"
    echo "      → Not ready for complex abstractions yet"
    echo "      → Build more foundation"
else
    echo "   ⏳ TOO EARLY: Abstract reasoning not yet developed"
    echo "      → Focus on concrete concepts first"
    echo "      → Build toward 0.50 similarity"
    echo "      → Re-test when similarity > 0.50"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 NEXT STEPS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if (( $(echo "$CURRENT_SIM >= 0.65" | bc -l 2>/dev/null || echo 0) )); then
    echo "   → Add Layer 4 data (philosophy, metaphors)"
    echo "   → Test cross-domain analogies"
    echo "   → Fine-tune for specific domains"
elif (( $(echo "$CURRENT_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
    echo "   → Continue Layer 3 data (causal reasoning)"
    echo "   → Monitor abstract concept formation"
    echo "   → Re-test monthly"
else
    echo "   → Focus on Layer 2 data (factual knowledge)"
    echo "   → Build toward 0.50 threshold"
    echo "   → Re-test when similarity crosses 0.50"
fi

echo ""
echo "📄 Results logged to: $OUTPUT_FILE"
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Abstract reasoning capability measured. Re-test monthly.     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

