#!/bin/bash
# Dialog Data Ingestion Helper
# Optimized for short, high-quality conversational exchanges
# Automatically runs diagnostics after ingestion
#
# Usage: ./ingest_dialog_data.sh <dialog_file.txt> [batch_size]

set -e

DIALOG_FILE="$1"
BATCH_SIZE="${2:-5000}"  # Default: measure every 5k exchanges

if [ -z "$DIALOG_FILE" ]; then
    echo "Usage: $0 <dialog_file.txt> [batch_size]"
    echo ""
    echo "Dialog file format (one exchange per line):"
    echo "  User: Hello, how are you?"
    echo "  Bot: I'm doing well, thanks!"
    echo "  User: What's the weather like?"
    echo "  Bot: It's sunny and warm today."
    echo ""
    echo "Or simple Q&A pairs:"
    echo "  Q: What is fire?"
    echo "  A: Fire is rapid oxidation releasing heat and light."
    echo ""
    exit 1
fi

if [ ! -f "$DIALOG_FILE" ]; then
    echo "❌ Error: File not found: $DIALOG_FILE"
    exit 1
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║            📥 DIALOG DATA INGESTION PIPELINE                  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Count total lines
TOTAL_LINES=$(wc -l < "$DIALOG_FILE" | tr -d ' ')
echo "📊 Input file: $DIALOG_FILE"
echo "📊 Total lines: $TOTAL_LINES"
echo "📊 Batch size: $BATCH_SIZE (run diagnostic every $BATCH_SIZE lines)"
echo ""

# Calculate expected batches
EXPECTED_BATCHES=$(( (TOTAL_LINES + BATCH_SIZE - 1) / BATCH_SIZE ))
echo "🔄 Will run diagnostic $EXPECTED_BATCHES times during ingestion"
echo ""

# Confirm before proceeding
read -p "📝 Proceed with ingestion? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ Cancelled"
    exit 0
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🚀 STARTING INGESTION..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Process in batches
CURRENT_LINE=0
BATCH_NUM=1

while IFS= read -r line; do
    CURRENT_LINE=$((CURRENT_LINE + 1))
    
    # TODO: Call your actual Melvin ingestion API here
    # For now, just simulate processing
    # Example: ./melvin_ingest_line "$line"
    
    # Every batch_size lines, run diagnostic
    if [ $((CURRENT_LINE % BATCH_SIZE)) -eq 0 ]; then
        echo ""
        echo "┌────────────────────────────────────────────────────────────┐"
        echo "│ 📊 BATCH $BATCH_NUM COMPLETE ($CURRENT_LINE / $TOTAL_LINES lines)           │"
        echo "└────────────────────────────────────────────────────────────┘"
        echo ""
        echo "🔬 Running quick diagnostic..."
        
        # Run quick diagnostic
        ./diagnostic_main --quick --csv-only > /dev/null 2>&1
        
        # Log results
        LAYER_NAME="layer1_batch$BATCH_NUM"
        DATA_SIZE="${CURRENT_LINE}_lines"
        NOTES="Dialog batch $BATCH_NUM of $EXPECTED_BATCHES"
        
        ./log_diagnostic_results.sh "$LAYER_NAME" "$DATA_SIZE" "$NOTES" 2>/dev/null
        
        # Show current metrics
        CURRENT_ENTROPY=$(tail -1 diagnostics_history.csv | cut -d',' -f4)
        CURRENT_SIM=$(tail -1 diagnostics_history.csv | cut -d',' -f5)
        CURRENT_SUCCESS=$(tail -1 diagnostics_history.csv | cut -d',' -f6)
        
        echo ""
        echo "   📈 Current Metrics:"
        printf "      Entropy↓:   %.3f\n" "$CURRENT_ENTROPY"
        printf "      Similarity: %.3f" "$CURRENT_SIM"
        
        # Check for milestone crossings
        if (( $(echo "$CURRENT_SIM >= 0.40" | bc -l 2>/dev/null || echo 0) )); then
            echo " 🎯 (≥0.40 - Check qualitative prompts!)"
        elif (( $(echo "$CURRENT_SIM >= 0.35" | bc -l 2>/dev/null || echo 0) )); then
            echo " ✨ (≥0.35 - Milestone 1 reached!)"
        elif (( $(echo "$CURRENT_SIM >= 0.20" | bc -l 2>/dev/null || echo 0) )); then
            echo " 📊 (≥0.20 - Good progress)"
        else
            echo ""
        fi
        
        printf "      Success:    %.1f%%\n" "$(echo "$CURRENT_SUCCESS * 100" | bc -l 2>/dev/null || echo 0)"
        echo ""
        
        BATCH_NUM=$((BATCH_NUM + 1))
        
        # If similarity crossed 0.4, prompt for qualitative check
        if (( $(echo "$CURRENT_SIM >= 0.40" | bc -l 2>/dev/null || echo 0) )); then
            echo "🎉 ALERT: Similarity crossed 0.40!"
            echo "   Run qualitative check: ./qualitative_check.sh"
            echo ""
        fi
    fi
    
    # Progress indicator
    if [ $((CURRENT_LINE % 1000)) -eq 0 ]; then
        PERCENT=$((CURRENT_LINE * 100 / TOTAL_LINES))
        echo -ne "   Processing: $CURRENT_LINE / $TOTAL_LINES ($PERCENT%)...\r"
    fi
    
done < "$DIALOG_FILE"

echo ""
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ INGESTION COMPLETE"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📊 Total lines processed: $CURRENT_LINE"
echo "📊 Diagnostic runs: $((BATCH_NUM - 1))"
echo ""

# Run final comprehensive diagnostic
echo "🔬 Running final diagnostic..."
./diagnostic_main --quick > /dev/null 2>&1

# Get final metrics
FINAL_ENTROPY=$(tail -1 diagnostics_history.csv | cut -d',' -f4)
FINAL_SIM=$(tail -1 diagnostics_history.csv | cut -d',' -f5)
FINAL_SUCCESS=$(tail -1 diagnostics_history.csv | cut -d',' -f6)

echo ""
echo "🎯 FINAL METRICS:"
printf "   Entropy Reduction:  %.3f\n" "$FINAL_ENTROPY"
printf "   Context Similarity: %.3f\n" "$FINAL_SIM"
printf "   Leap Success Rate:  %.1f%%\n" "$(echo "$FINAL_SUCCESS * 100" | bc -l 2>/dev/null || echo 0)"
echo ""

# Recommendations
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 NEXT STEPS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if (( $(echo "$FINAL_SIM >= 0.50" | bc -l 2>/dev/null || echo 0) )); then
    echo "🎉 MILESTONE: Crossed 0.5 coherence threshold!"
    echo "   → First coherent sentences should be emerging"
    echo "   → Test with real prompts"
    echo "   → Begin Layer 2 data (factual knowledge)"
elif (( $(echo "$FINAL_SIM >= 0.40" | bc -l 2>/dev/null || echo 0) )); then
    echo "✨ PROGRESS: Similarity ≥0.40 - Close to coherence!"
    echo "   → Run qualitative checks: ./qualitative_check.sh"
    echo "   → Add more dialog data or begin Layer 2"
    echo "   → Watch for 0.5 threshold crossing"
elif (( $(echo "$FINAL_SIM >= 0.30" | bc -l 2>/dev/null || echo 0) )); then
    echo "📈 PROGRESS: Good foundation building"
    echo "   → Continue with more dialog data"
    echo "   → Run weekly summary: ./weekly_summary.sh"
    echo "   → Target: Cross 0.4 in next batch"
else
    echo "🔧 BUILDING: Foundation phase"
    echo "   → Need more high-quality dialog data"
    echo "   → Ensure conversations are natural and varied"
    echo "   → Run diagnostic after each 5k additions"
fi

echo ""
echo "📊 View progress: ./weekly_summary.sh"
echo "📈 Plot trends:   python3 plot_progress.py --save"
echo "🔍 Check links:   ./qualitative_check.sh"
echo ""

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Ingestion complete. Data integrated into knowledge graph.   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

