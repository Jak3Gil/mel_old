#!/bin/bash
# Anomaly Detection - Track Curve Shape, Not Just Values
# Detects sudden drops, spikes, or plateaus that indicate issues
# Usage: ./detect_anomalies.sh

set -e

HISTORY_FILE="diagnostics_history.csv"

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           🔍 ANOMALY DETECTION - CURVE SHAPE ANALYSIS         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

if [ ! -f "$HISTORY_FILE" ]; then
    echo "❌ Error: $HISTORY_FILE not found"
    exit 1
fi

# Need at least 3 data points for trend analysis
ENTRIES=$(tail -n +2 "$HISTORY_FILE" | wc -l | tr -d ' ')
if [ "$ENTRIES" -lt 3 ]; then
    echo "ℹ️  Need at least 3 diagnostic runs for anomaly detection"
    echo "   Current: $ENTRIES entries"
    echo "   Run more diagnostics and come back"
    exit 0
fi

echo "📊 Analyzing last 5 runs for anomalies..."
echo ""

# Get last 5 entries
RECENT=$(tail -6 "$HISTORY_FILE" | tail -5)

# Extract metrics into arrays
SIMILARITIES=()
ENTROPIES=()
SUCCESSES=()
DATES=()

while IFS=',' read -r date layer size entropy sim success lambda thresh lr notes; do
    DATES+=("$date")
    ENTROPIES+=("$entropy")
    SIMILARITIES+=("$sim")
    SUCCESSES+=("$success")
done <<< "$RECENT"

# Anomaly detection
ANOMALIES_FOUND=0

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 CHECKING FOR ANOMALIES:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check 1: Sudden drops in similarity
echo "1. Checking for sudden similarity drops..."
for i in $(seq 1 $((${#SIMILARITIES[@]} - 1))); do
    PREV="${SIMILARITIES[$((i-1))]}"
    CURR="${SIMILARITIES[$i]}"
    
    DELTA=$(echo "$CURR - $PREV" | bc -l 2>/dev/null || echo "0")
    
    if (( $(echo "$DELTA < -0.10" | bc -l 2>/dev/null || echo 0) )); then
        echo "   ⚠️  ANOMALY: Similarity dropped by $DELTA"
        echo "      ${DATES[$((i-1))]} → ${DATES[$i]}: $PREV → $CURR"
        echo "      Possible causes:"
        echo "        • Bad data batch ingested"
        echo "        • Embedding normalization issue"
        echo "        • Parameter changed incorrectly"
        echo "      Action: Review recent data, check config"
        ANOMALIES_FOUND=$((ANOMALIES_FOUND + 1))
        echo ""
    fi
done

# Check 2: Sudden spikes in entropy (when should be stable)
echo "2. Checking for entropy spikes..."
for i in $(seq 1 $((${#ENTROPIES[@]} - 1))); do
    PREV="${ENTROPIES[$((i-1))]}"
    CURR="${ENTROPIES[$i]}"
    
    DELTA=$(echo "$CURR - $PREV" | bc -l 2>/dev/null || echo "0")
    
    # Only flag if similarity is decent (should be stable)
    CURR_SIM="${SIMILARITIES[$i]}"
    if (( $(echo "$CURR_SIM > 0.30 && $DELTA > 0.10" | bc -l 2>/dev/null || echo 0) )); then
        echo "   ⚠️  ANOMALY: Entropy spiked by +$DELTA"
        echo "      ${DATES[$((i-1))]} → ${DATES[$i]}: $PREV → $CURR"
        echo "      Possible causes:"
        echo "        • Bias weight (lambda) decreased"
        echo "        • Graph structure degraded"
        echo "        • Embeddings de-normalized"
        echo "      Action: Check lambda_graph_bias, re-normalize embeddings"
        ANOMALIES_FOUND=$((ANOMALIES_FOUND + 1))
        echo ""
    fi
done

# Check 3: Unexpected plateaus (no improvement over 3+ runs)
echo "3. Checking for unexpected plateaus..."
if [ "${#SIMILARITIES[@]}" -ge 4 ]; then
    # Get last 4 similarities
    LAST4=("${SIMILARITIES[@]: -4}")
    
    # Check if all within 0.02 of each other
    MAX=$(printf '%s\n' "${LAST4[@]}" | sort -nr | head -1)
    MIN=$(printf '%s\n' "${LAST4[@]}" | sort -n | head -1)
    RANGE=$(echo "$MAX - $MIN" | bc -l 2>/dev/null || echo "0")
    
    # If stuck at low similarity with no progress
    if (( $(echo "$RANGE < 0.02 && $MAX < 0.40" | bc -l 2>/dev/null || echo 0) )); then
        echo "   ⚠️  PLATEAU: Similarity stuck at $MAX for 4+ runs"
        echo "      Possible causes:"
        echo "        • Need more diverse data"
        echo "        • Learning rate too low"
        echo "        • Insufficient training cycles"
        echo "      Action: Run auto-tune, add new data source"
        ANOMALIES_FOUND=$((ANOMALIES_FOUND + 1))
        echo ""
    fi
fi

# Check 4: Inverted relationship (entropy rising while similarity rising)
echo "4. Checking for inverted relationships..."
if [ "${#SIMILARITIES[@]}" -ge 3 ]; then
    # Compare last 3 points
    for i in $(seq 2 $((${#SIMILARITIES[@]} - 1))); do
        SIM_DELTA=$(echo "${SIMILARITIES[$i]} - ${SIMILARITIES[$((i-1))]}" | bc -l)
        ENT_DELTA=$(echo "${ENTROPIES[$i]} - ${ENTROPIES[$((i-1))]}" | bc -l)
        
        # Both should generally increase together early, or entropy stable later
        # Flag if similarity increasing but entropy decreasing (weird)
        if (( $(echo "$SIM_DELTA > 0.05 && $ENT_DELTA < -0.05" | bc -l 2>/dev/null || echo 0) )); then
            echo "   ⚠️  ANOMALY: Similarity rising but entropy falling"
            echo "      ${DATES[$((i-1))]} → ${DATES[$i]}"
            echo "      Similarity: ${SIMILARITIES[$((i-1))]} → ${SIMILARITIES[$i]} (+$SIM_DELTA)"
            echo "      Entropy: ${ENTROPIES[$((i-1))]} → ${ENTROPIES[$i]} ($ENT_DELTA)"
            echo "      This is unusual - investigate data or config changes"
            ANOMALIES_FOUND=$((ANOMALIES_FOUND + 1))
            echo ""
        fi
    done
fi

# Summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ "$ANOMALIES_FOUND" -eq 0 ]; then
    echo "✅ NO ANOMALIES DETECTED"
    echo ""
    echo "   System progressing normally."
    echo "   Curve shape is healthy."
    echo "   Continue current strategy."
else
    echo "⚠️  ANOMALIES DETECTED: $ANOMALIES_FOUND"
    echo ""
    echo "   Review the issues above and take corrective action."
    echo "   Run diagnostics again after fixes to verify."
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Show curve trend
echo "📈 CURVE TREND (Last 5 runs):"
echo ""

for i in $(seq 0 $((${#SIMILARITIES[@]} - 1))); do
    DATE="${DATES[$i]}"
    SIM="${SIMILARITIES[$i]}"
    ENT="${ENTROPIES[$i]}"
    
    printf "   %s: Sim %.3f, Ent %.3f" "$DATE" "$SIM" "$ENT"
    
    # Show direction
    if [ "$i" -gt 0 ]; then
        SIM_PREV="${SIMILARITIES[$((i-1))]}"
        ENT_PREV="${ENTROPIES[$((i-1))]}"
        
        SIM_DELTA=$(echo "$SIM - $SIM_PREV" | bc -l)
        ENT_DELTA=$(echo "$ENT - $ENT_PREV" | bc -l)
        
        printf " (Δsim: %+.3f, Δent: %+.3f)" "$SIM_DELTA" "$ENT_DELTA"
        
        # Visual indicator
        if (( $(echo "$SIM_DELTA > 0.03" | bc -l 2>/dev/null || echo 0) )); then
            echo " ↗️ Good"
        elif (( $(echo "$SIM_DELTA < -0.03" | bc -l 2>/dev/null || echo 0) )); then
            echo " ↘️ Warning"
        else
            echo " → Stable"
        fi
    else
        echo ""
    fi
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 TIP: A sudden drop or spike usually means:"
echo "   • Bias weight (lambda) changed"
echo "   • Embedding normalization slipped"
echo "   • Bad data batch"
echo "   • Configuration reset"
echo ""
echo "   Catch these early by running this weekly!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Anomaly detection complete. Curve shape is $( [ "$ANOMALIES_FOUND" -eq 0 ] && echo "healthy." || echo "concerning." )           ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

exit $ANOMALIES_FOUND

