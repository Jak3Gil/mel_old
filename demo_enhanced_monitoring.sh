#!/bin/bash

# Enhanced demo script for Melvin Live Growth Monitoring with Composition Tracking
# This script demonstrates the enhanced monitoring system with realistic growth patterns

echo "📈 Melvin Enhanced Growth Monitoring Demo"
echo "========================================"
echo ""

# Create output directory
mkdir -p out

# Create a sample CSV file with enhanced composition metrics
echo "timestamp,step,nodes,edges,paths,taught_nodes,thought_nodes,rel_exact,rel_temporal,rel_leap,avg_weight_bits,max_weight_bits,avg_path_len,max_path_len,edge_rate,e2n_ratio,attn_entropy,diversity,top2_margin,health,latency_ms,fanout,profile,dataset" > out/metrics_live.csv

# Generate realistic Melvin growth data with composition tracking
echo "📊 Generating enhanced growth data with composition tracking..."

# Initialize counters
taught_nodes=0
thought_nodes=0
rel_exact=0
rel_temporal=0
rel_leap=0
total_weight=0
max_weight=0
avg_path_len=0
max_path_len=0

for i in {1..100}; do
    # Simulate realistic Melvin growth patterns with composition
    nodes=$((1200 + i * 12 + RANDOM % 15))
    edges=$((2100 + i * 20 + RANDOM % 25))
    paths=$((150 + i * 2 + RANDOM % 4))
    
    # Composition tracking
    if [ $((i % 3)) -eq 0 ]; then
        # Learning burst - more taught nodes
        taught_nodes=$((taught_nodes + 8 + RANDOM % 5))
        thought_nodes=$((thought_nodes + 2 + RANDOM % 3))
    else
        # Reasoning phase - more thought nodes
        taught_nodes=$((taught_nodes + 1 + RANDOM % 2))
        thought_nodes=$((thought_nodes + 5 + RANDOM % 4))
    fi
    
    # Edge type composition (realistic patterns)
    if [ $((i % 10)) -eq 0 ]; then
        # Consolidation phase - more temporal edges
        rel_temporal=$((rel_temporal + 15 + RANDOM % 10))
        rel_exact=$((rel_exact + 5 + RANDOM % 3))
        rel_leap=$((rel_leap + 2 + RANDOM % 2))
    elif [ $((i % 7)) -eq 0 ]; then
        # Creative phase - more leap edges
        rel_temporal=$((rel_temporal + 3 + RANDOM % 2))
        rel_exact=$((rel_exact + 4 + RANDOM % 3))
        rel_leap=$((rel_leap + 8 + RANDOM % 5))
    else
        # Normal phase - balanced growth
        rel_temporal=$((rel_temporal + 6 + RANDOM % 4))
        rel_exact=$((rel_exact + 8 + RANDOM % 3))
        rel_leap=$((rel_leap + 3 + RANDOM % 3))
    fi
    
    # Weight tracking
    weight=$(echo "scale=2; 0.5 + (RANDOM % 50) / 100" | bc -l)
    total_weight=$(echo "scale=2; $total_weight + $weight" | bc -l)
    avg_weight=$(echo "scale=2; $total_weight / $i" | bc -l)
    if (( $(echo "$weight > $max_weight" | bc -l) )); then
        max_weight=$weight
    fi
    
    # Path length tracking
    path_len=$(echo "scale=1; 2 + (RANDOM % 8)" | bc -l)
    avg_path_len=$(echo "scale=1; $avg_path_len * 0.95 + $path_len * 0.05" | bc -l)
    if (( $(echo "$path_len > $max_path_len" | bc -l) )); then
        max_path_len=$path_len
    fi
    
    # Rate calculations
    if [ $i -gt 1 ]; then
        edge_rate=$(echo "scale=2; ($edges - 2100) / ($i * 10)" | bc -l)
        e2n_ratio=$(echo "scale=2; $edges / $nodes" | bc -l)
    else
        edge_rate=0
        e2n_ratio=$(echo "scale=2; $edges / $nodes" | bc -l)
    fi
    
    # Metrics with realistic patterns
    entropy=$(echo "scale=3; 0.08 + (i * 0.001) + (RANDOM % 15) / 1000" | bc -l)
    diversity=$(echo "scale=3; 0.45 + (i * 0.002) + (RANDOM % 20) / 1000" | bc -l)
    top2=$(echo "scale=3; 0.20 + (i * 0.0005) + (RANDOM % 10) / 1000" | bc -l)
    health=$(echo "scale=1; 60 + (i * 0.3) + (RANDOM % 8)" | bc -l)
    latency=$((10 + RANDOM % 4))
    fanout=$((5 + RANDOM % 3))
    
    # Add some variation for realism
    if [ $((i % 15)) -eq 0 ]; then
        # Periodic "learning bursts"
        nodes=$((nodes + 30))
        edges=$((edges + 50))
        paths=$((paths + 8))
        health=$(echo "scale=1; $health + 3" | bc -l)
        rel_temporal=$((rel_temporal + 10))
    fi
    
    # Simulate occasional stalls or issues
    if [ $((i % 25)) -eq 0 ] && [ $i -gt 10 ]; then
        entropy=$(echo "scale=3; $entropy + 0.1" | bc -l)  # High entropy spike
        diversity=$(echo "scale=3; $diversity - 0.05" | bc -l)  # Low diversity
    fi
    
    timestamp=$(date +%s).$(printf "%03d" $((RANDOM % 1000)))
    
    echo "$timestamp,$i,$nodes,$edges,$paths,$taught_nodes,$thought_nodes,$rel_exact,$rel_temporal,$rel_leap,$avg_weight,$max_weight,$avg_path_len,$max_path_len,$edge_rate,$e2n_ratio,$entropy,$diversity,$top2,$health,$latency,$fanout,Balanced,commonsense_qa" >> out/metrics_live.csv
    
    # Small delay to simulate real-time data
    sleep 0.08
done

echo "✅ Generated 100 samples of enhanced growth data with composition tracking"
echo ""

echo "🚀 Starting enhanced growth monitor..."
echo "📊 File: out/metrics_live.csv"
echo "🔄 Press Ctrl+C to exit"
echo ""

# Start the enhanced growth monitor
./tools/growth_monitor out/metrics_live.csv 50
