#!/bin/bash

# UCA Formula Testing Script
# Quick 1-minute smoke tests for each mathematical formula

echo "🧠 UCA Mathematical Formula Testing Suite"
echo "========================================"

# Create test data directory
mkdir -p test_data
cd test_data

# Generate test prompts
cat > prompts.txt << EOF
what are cats
what happened then
cats are mammals
exact match
leap of logic
what happened first then next
cats are reptiles
EOF

echo "📊 Running baseline tests..."

# Baseline test (all formulas enabled)
echo "--- Baseline (all formulas ON) ---"
../melvin_scheduler < prompts.txt > baseline_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 10
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

echo "Baseline confidence metrics:"
if [ -f uca_metrics.csv ]; then
    tail -n +2 uca_metrics.csv | cut -d',' -f2 | awk '{sum+=$1; count++} END {print "Avg confidence:", sum/count}'
fi

# Test each formula individually
echo -e "\n🔬 Running ablation tests..."

# A1: Hub bias (degree normalization)
echo "--- A1: Testing degree normalization ---"
UCA_NO_DEGREE=1 ../melvin_scheduler < prompts.txt > no_degree_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

# A2: Relation priors
echo "--- A2: Testing relation priors ---"
UCA_NO_RELPR=1 ../melvin_scheduler < prompts.txt > no_relpr_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

# A3: Contradiction penalty
echo "--- A3: Testing contradiction penalty ---"
UCA_NO_CONTRA=1 ../melvin_scheduler < prompts.txt > no_contra_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

# A4: Temporal continuity
echo "--- A4: Testing temporal continuity ---"
UCA_NO_CONT=1 ../melvin_scheduler < prompts.txt > no_cont_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

# A5: Multi-hop discount
echo "--- A5: Testing multi-hop discount ---"
UCA_USE_TD=1 ../melvin_scheduler < prompts.txt > use_td_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

# A6: Beam diversity
echo "--- A6: Testing beam diversity ---"
UCA_NO_DIV=1 ../melvin_scheduler < prompts.txt > no_div_output.txt 2>&1 &
SCHEDULER_PID=$!
sleep 5
kill $SCHEDULER_PID 2>/dev/null
wait $SCHEDULER_PID 2>/dev/null

echo -e "\n📈 Analyzing results..."

# Compare confidence levels
if [ -f uca_metrics.csv ]; then
    echo "Confidence comparison:"
    echo "Baseline:"
    tail -n +2 uca_metrics.csv | cut -d',' -f2 | awk '{sum+=$1; count++} END {print "  Avg:", sum/count}'
    
    # Check if metrics show expected patterns
    echo "Formula effectiveness indicators:"
    echo "  Degree norm values:"
    tail -n +2 uca_metrics.csv | cut -d',' -f7 | awk '{sum+=$1; count++} END {print "    Avg:", sum/count}'
    echo "  Diversity penalty values:"
    tail -n +2 uca_metrics.csv | cut -d',' -f5 | awk '{sum+=$1; count++} END {print "    Avg:", sum/count}'
fi

echo -e "\n✅ Smoke tests completed!"
echo "Check uca_metrics.csv for detailed metrics"
echo "Run 'make test-uca' for comprehensive testing"

# Cleanup
cd ..
