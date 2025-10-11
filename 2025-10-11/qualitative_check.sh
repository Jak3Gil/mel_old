#!/bin/bash
# Qualitative Prompt Checker
# Run this when context similarity crosses ~0.4 to see emerging conceptual links
# Usage: ./qualitative_check.sh

set -e

HISTORY_FILE="diagnostics_history.csv"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           🔍 QUALITATIVE CONCEPTUAL LINK CHECK                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check current similarity level
if [ -f "$HISTORY_FILE" ]; then
    CURRENT_SIM=$(tail -1 "$HISTORY_FILE" | cut -d',' -f5)
    echo "📊 Current context similarity: $CURRENT_SIM"
    echo ""
    
    # Recommend when to run this
    if (( $(echo "$CURRENT_SIM < 0.30" | bc -l 2>/dev/null || echo 1) )); then
        echo "⏳ Similarity too low ($CURRENT_SIM < 0.30)"
        echo "   Wait until similarity reaches ~0.35-0.40"
        echo "   Current stage: Foundation building"
        echo "   Expected: Random or no connections yet"
        echo ""
        echo "   Come back when similarity > 0.35"
        exit 0
    elif (( $(echo "$CURRENT_SIM >= 0.40" | bc -l 2>/dev/null || echo 0) )); then
        echo "✅ Similarity at $CURRENT_SIM - Good time to check!"
        echo "   Should start seeing faint conceptual links"
    else
        echo "📈 Similarity at $CURRENT_SIM - Borderline, let's check"
        echo "   May see weak links emerging"
    fi
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TESTING CONCEPTUAL PROMPTS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Run diagnostic with CSV-only mode to get fresh data
echo "Running diagnostic tests..."
./diagnostic_main --csv-only > /dev/null 2>&1

# Parse results and show qualitative assessment
echo "Analyzing conceptual links..."
echo ""

# Check specific high-value prompts
check_prompt() {
    local prompt="$1"
    local expected="$2"
    
    # Extract from CSV
    local result=$(grep "^$prompt," leap_diagnostics.csv | tail -1)
    
    if [ -z "$result" ]; then
        echo "⚠️  $prompt: No data"
        return
    fi
    
    local entropy_reduction=$(echo "$result" | cut -d',' -f4)
    local similarity=$(echo "$result" | cut -d',' -f8)
    local success=$(echo "$result" | cut -d',' -f10)
    local top_tokens=$(echo "$result" | cut -d',' -f11-13 | tr ',' ' ')
    
    # Determine link strength
    local link_strength="❌ No link"
    local color=""
    
    if (( $(echo "$entropy_reduction > 0.15 && $similarity > 0.30" | bc -l 2>/dev/null || echo 0) )); then
        link_strength="🟢 Strong link"
    elif (( $(echo "$entropy_reduction > 0.08 || $similarity > 0.20" | bc -l 2>/dev/null || echo 0) )); then
        link_strength="🟡 Weak link"
    elif (( $(echo "$entropy_reduction > 0.02 || $similarity > 0.10" | bc -l 2>/dev/null || echo 0) )); then
        link_strength="🟠 Faint link"
    fi
    
    echo "🔗 $prompt"
    echo "   Status:     $link_strength"
    echo "   Entropy↓:   $entropy_reduction"
    echo "   Similarity: $similarity"
    echo "   Tokens:     $top_tokens"
    
    # Check if expected concept appears
    if [ -n "$expected" ]; then
        if echo "$top_tokens" | grep -qi "$expected"; then
            echo "   ✨ Expected concept '$expected' found!"
        else
            echo "   → Looking for: $expected"
        fi
    fi
    
    echo ""
}

# High-priority conceptual pairs to check
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 CORE CONCEPTUAL PAIRS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

check_prompt "fire→water" "steam|cool|extinguish"
check_prompt "music→emotion" "feeling|mood|joy"
check_prompt "robot→person" "human|intelligence|artificial"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🌟 ABSTRACT RELATIONS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

check_prompt "thought→memory" "remember|recall|cognitive"
check_prompt "food→energy" "fuel|nutrition|power"
check_prompt "bird→flight" "fly|wing|air"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 INTERPRETATION GUIDE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🟢 Strong Link (Entropy↓ >0.15, Sim >0.30):"
echo "   → Concept well-represented in graph"
echo "   → Embeddings aligned"
echo "   → True semantic understanding"
echo ""
echo "🟡 Weak Link (Entropy↓ >0.08, Sim >0.20):"
echo "   → Concept partially represented"
echo "   → Embeddings learning"
echo "   → Path to target exists but indirect"
echo ""
echo "🟠 Faint Link (Entropy↓ >0.02, Sim >0.10):"
echo "   → Concept barely represented"
echo "   → Just beginning to form connections"
echo "   → Need more training data"
echo ""
echo "❌ No Link (Entropy↓ ≈0, Sim ≈0):"
echo "   → Concept missing from graph"
echo "   → No training data for this connection"
echo "   → Add relevant examples"
echo ""

# Provide next steps based on overall performance
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 RECOMMENDATIONS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$HISTORY_FILE" ]; then
    CURRENT_SIM=$(tail -1 "$HISTORY_FILE" | cut -d',' -f5)
    
    if (( $(echo "$CURRENT_SIM < 0.30" | bc -l 2>/dev/null || echo 1) )); then
        echo "📌 FOCUS: Build foundation"
        echo "   → Add 10k-20k more conversational exchanges"
        echo "   → Focus on common concepts and relationships"
        echo "   → Run diagnostic after each 5k additions"
    elif (( $(echo "$CURRENT_SIM < 0.45" | bc -l 2>/dev/null || echo 1) )); then
        echo "📌 FOCUS: Strengthen embeddings"
        echo "   → Continue conversational data"
        echo "   → Run 50-100 training cycles with feedback"
        echo "   → Check nearest neighbors for intuition"
        echo "   → Run this qualitative check weekly"
    elif (( $(echo "$CURRENT_SIM < 0.55" | bc -l 2>/dev/null || echo 1) )); then
        echo "📌 FOCUS: Push toward coherence (SO CLOSE!)"
        echo "   → Add factual/encyclopedic content"
        echo "   → Run auto-tune to optimize parameters"
        echo "   → You're approaching the 0.5 threshold!"
    else
        echo "🎉 FOCUS: You've crossed 0.5 - coherence active!"
        echo "   → Look for multi-word conceptual phrases"
        echo "   → Test causal reasoning chains"
        echo "   → Begin Layer 3 (reasoning) data"
    fi
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Re-run this after adding data to see links emerge           ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

