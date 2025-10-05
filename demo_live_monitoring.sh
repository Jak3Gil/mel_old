#!/bin/bash

# Demo script for Melvin Live Growth Monitoring
# This script demonstrates the live monitoring system with simulated data

echo "📈 Melvin Live Growth Monitoring Demo"
echo "====================================="
echo ""

# Create output directory
mkdir -p out

# Create a sample CSV file with simulated Melvin growth data
echo "timestamp,step,nodes,edges,paths,attn_entropy,diversity,top2_margin,health,latency_ms,fanout,profile,dataset" > out/metrics_live.csv

# Generate simulated data showing healthy Melvin growth
echo "📊 Generating simulated growth data..."

for i in {1..50}; do
    # Simulate realistic Melvin growth patterns
    nodes=$((1200 + i * 15 + RANDOM % 20))
    edges=$((2100 + i * 25 + RANDOM % 30))
    paths=$((150 + i * 3 + RANDOM % 5))
    entropy=$(echo "scale=3; 0.08 + (i * 0.002) + (RANDOM % 10) / 1000" | bc -l)
    diversity=$(echo "scale=3; 0.45 + (i * 0.003) + (RANDOM % 20) / 1000" | bc -l)
    top2=$(echo "scale=3; 0.20 + (i * 0.001) + (RANDOM % 10) / 1000" | bc -l)
    health=$(echo "scale=1; 60 + (i * 0.5) + (RANDOM % 10)" | bc -l)
    latency=$((10 + RANDOM % 5))
    fanout=$((5 + RANDOM % 3))
    
    # Add some variation for realism
    if [ $((i % 10)) -eq 0 ]; then
        # Occasional "learning bursts"
        nodes=$((nodes + 50))
        edges=$((edges + 80))
        paths=$((paths + 10))
        health=$(echo "scale=1; $health + 5" | bc -l)
    fi
    
    timestamp=$(date +%s).$(printf "%03d" $((RANDOM % 1000)))
    
    echo "$timestamp,$i,$nodes,$edges,$paths,$entropy,$diversity,$top2,$health,$latency,$fanout,Balanced,commonsense_qa" >> out/metrics_live.csv
    
    # Small delay to simulate real-time data
    sleep 0.1
done

echo "✅ Generated 50 samples of simulated growth data"
echo ""

echo "🚀 Starting live growth monitor..."
echo "📊 File: out/metrics_live.csv"
echo "🔄 Press Ctrl+C to exit"
echo ""

# Start the growth monitor
./tools/growth_monitor out/metrics_live.csv 30
