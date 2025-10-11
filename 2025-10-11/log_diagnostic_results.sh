#!/bin/bash
# Helper script to log diagnostic results to history CSV
# Usage: ./log_diagnostic_results.sh "layer1" "10k_exchanges" "TinyChat conversations added"

set -e

HISTORY_FILE="diagnostics_history.csv"
REPORT_FILE="leap_tuning_report.md"

# Get parameters
DATA_LAYER="${1:-unknown}"
DATA_SIZE="${2:-unknown}"
NOTES="${3:-Diagnostic run}"

# Extract metrics from the report
if [ ! -f "$REPORT_FILE" ]; then
    echo "❌ Error: $REPORT_FILE not found. Run diagnostic first."
    exit 1
fi

# Parse the markdown table (robust extraction)
ENTROPY=$(grep "Mean Entropy Reduction" "$REPORT_FILE" | sed -n 's/.*| \([0-9.]*\) |.*/\1/p' | head -1)
SIMILARITY=$(grep "Mean Context Similarity" "$REPORT_FILE" | sed -n 's/.*| \([0-9.]*\) |.*/\1/p' | head -1)
SUCCESS=$(grep "Leap Success Rate" "$REPORT_FILE" | sed -n 's/.*| \([0-9.]*\)% |.*/\1/p' | head -1)

# Get config values (from last run)
LAMBDA="0.5"  # Default, update if you extract from report
THRESHOLD="0.6"
LEARNING_RATE="0.01"

# If auto-tune was run, try to extract optimized values
if [ -f "leap_tuning_results.txt" ]; then
    LAMBDA=$(grep "lambda_graph_bias" leap_tuning_results.txt | tail -1 | grep -oE '[0-9]+\.[0-9]+' | head -1 || echo "0.5")
    THRESHOLD=$(grep "leap_entropy_threshold" leap_tuning_results.txt | tail -1 | grep -oE '[0-9]+\.[0-9]+' | head -1 || echo "0.6")
    LEARNING_RATE=$(grep "learning_rate_embeddings" leap_tuning_results.txt | tail -1 | grep -oE '[0-9]+\.[0-9]+' | head -1 || echo "0.01")
fi

# Get current date
DATE=$(date +%Y-%m-%d)

# Convert success rate to decimal
SUCCESS_DECIMAL=$(echo "scale=2; $SUCCESS / 100" | bc 2>/dev/null || echo "0.0")

# Append to CSV (redundancy - keep CSV for backward compatibility)
echo "$DATE,$DATA_LAYER,$DATA_SIZE,$ENTROPY,$SIMILARITY,$SUCCESS_DECIMAL,$LAMBDA,$THRESHOLD,$LEARNING_RATE,$NOTES" >> "$HISTORY_FILE"

# Also log to database (primary source of truth)
if [ -f "src/database_manager.py" ]; then
    # Get Git context
    GIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
    GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || echo "")
    
    # Determine entry number (count lines in history + 1)
    ENTRY_NUMBER=$(wc -l < "$HISTORY_FILE" 2>/dev/null || echo "1")
    
    # Data version (try to find latest)
    DATA_VERSION=$(ls -t versioned_data/*/metadata.json 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "unknown")
    
    # Try to get node/edge counts from brain snapshot if available
    NODES_COUNT=0
    EDGES_COUNT=0
    if command -v python3 > /dev/null 2>&1; then
        # Simple extraction from snapshot metadata (if we add it later)
        # For now, use defaults
        NODES_COUNT=0
        EDGES_COUNT=0
    fi
    
    # Log to database (before → after, assume before is 0.0 for simplicity)
    python3 src/database_manager.py log_run \
        --entry "$ENTRY_NUMBER" \
        --date "$DATE" \
        --git-hash "$GIT_HASH" \
        --tag "$GIT_TAG" \
        --data-version "$DATA_VERSION" \
        --data-layer "$DATA_LAYER" \
        --data-size "$DATA_SIZE" \
        --sim-before 0.0 \
        --sim-after "$SIMILARITY" \
        --ent-before 0.0 \
        --ent-after "$ENTROPY" \
        --success-before 0.0 \
        --success-after "$SUCCESS" \
        --lambda "$LAMBDA" \
        --threshold "$THRESHOLD" \
        --lr "$LEARNING_RATE" \
        --nodes "$NODES_COUNT" \
        --edges "$EDGES_COUNT" \
        --notes "$NOTES" 2>/dev/null && echo "✅ Logged to database: melvin_research.db"
fi

# Display what was logged
echo ""
echo "✅ Logged to $HISTORY_FILE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📅 Date:                $DATE"
echo "📊 Data Layer:          $DATA_LAYER"
echo "📦 Data Size:           $DATA_SIZE"
echo "📉 Entropy Reduction:   $ENTROPY"
echo "🎯 Context Similarity:  $SIMILARITY"
echo "✅ Leap Success:        $SUCCESS%"
echo "⚙️  Lambda:              $LAMBDA"
echo "⚙️  Threshold:           $THRESHOLD"
echo "⚙️  Learning Rate:       $LEARNING_RATE"
echo "📝 Notes:               $NOTES"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Show recent history
echo "📊 Recent History (last 5 entries):"
echo ""
tail -6 "$HISTORY_FILE" | column -t -s','
echo ""

# Check for milestones
if (( $(echo "$SIMILARITY >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
    if (( $(echo "$SIMILARITY < 0.50" | bc -l) )); then
        echo "🎯 Milestone 1 Progress: Context similarity at $SIMILARITY (target: 0.35)"
    fi
fi

if (( $(echo "$SIMILARITY >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
    echo "🎉 MILESTONE 2 REACHED! Context similarity = $SIMILARITY"
    echo "   → First coherent sentences should be emerging!"
fi

if (( $(echo "$SIMILARITY >= 0.65" | bc -l 2>/dev/null || echo 0) )); then
    echo "🚀 MILESTONE 3 REACHED! Context similarity = $SIMILARITY"
    echo "   → Causal reasoning active!"
fi

if (( $(echo "$SIMILARITY >= 0.75" | bc -l 2>/dev/null || echo 0) )); then
    echo "⭐ MILESTONE 4 REACHED! Context similarity = $SIMILARITY"
    echo "   → Abstract concept mastery achieved!"
fi

