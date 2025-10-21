#!/usr/bin/env python3
"""
Long-Run Analysis Script

Analyzes extended endurance test results:
- Metric stability over time
- Learning progression
- Performance trends
- Drift detection
"""

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def main():
    print("\n" + "="*60)
    print("  LONG-RUN ANALYSIS")
    print("="*60 + "\n")
    
    # Load data
    if not os.path.exists("data/longrun_log.csv"):
        print("✗ Error: data/longrun_log.csv not found")
        print("  Run: ./tests/test_longrun first\n")
        return 1
    
    print("📂 Loading data...")
    df = pd.read_csv("data/longrun_log.csv")
    
    print(f"  ✓ Loaded {len(df)} samples")
    print(f"  ✓ Duration: cycle {df['cycle'].min()} to {df['cycle'].max()}\n")
    
    # Create figure with 4 subplots
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle("Long-Run Endurance Test Analysis", fontsize=16, fontweight='bold')
    
    # Subplot 1: Energy over time
    print("📊 Plotting energy trends...")
    ax1 = axes[0, 0]
    for query in df['query'].unique():
        subset = df[df['query'] == query]
        ax1.plot(subset['cycle'], subset['energy'], label=query, alpha=0.7, linewidth=1)
    
    ax1.set_xlabel("Cycle", fontsize=10)
    ax1.set_ylabel("Energy", fontsize=10)
    ax1.set_title("Reasoning Energy Over Time", fontsize=11)
    ax1.legend(fontsize=8)
    ax1.grid(True, alpha=0.3)
    
    # Subplot 2: Weight accumulation
    print("📊 Plotting weight evolution...")
    ax2 = axes[0, 1]
    
    # Average by cycle
    cycle_avg = df.groupby('cycle').agg({
        'avg_node_weight': 'mean',
        'avg_edge_weight': 'mean'
    }).reset_index()
    
    ax2.plot(cycle_avg['cycle'], cycle_avg['avg_node_weight'], 
             label='Avg Node Weight', linewidth=2, color='#2ecc71')
    ax2.plot(cycle_avg['cycle'], cycle_avg['avg_edge_weight'], 
             label='Avg Edge Weight', linewidth=2, color='#3498db')
    
    ax2.set_xlabel("Cycle", fontsize=10)
    ax2.set_ylabel("Weight", fontsize=10)
    ax2.set_title("Weight Accumulation", fontsize=11)
    ax2.legend(fontsize=9)
    ax2.grid(True, alpha=0.3)
    
    # Subplot 3: Active nodes/edges
    print("📊 Plotting activity...")
    ax3 = axes[1, 0]
    
    ax3.plot(cycle_avg['cycle'], df.groupby('cycle')['active_nodes'].mean(), 
             label='Active Nodes', linewidth=2, color='#e74c3c')
    ax3.plot(cycle_avg['cycle'], df.groupby('cycle')['active_edges'].mean(), 
             label='Active Edges', linewidth=2, color='#9b59b6')
    
    ax3.set_xlabel("Cycle", fontsize=10)
    ax3.set_ylabel("Count", fontsize=10)
    ax3.set_title("Active Nodes/Edges", fontsize=11)
    ax3.legend(fontsize=9)
    ax3.grid(True, alpha=0.3)
    
    # Subplot 4: Performance
    print("📊 Plotting performance...")
    ax4 = axes[1, 1]
    
    # Rolling average for smoother visualization
    window = max(1, len(df) // 50)
    time_rolling = df.groupby('cycle')['time_ms'].mean().rolling(window=window).mean()
    
    ax4.plot(time_rolling.index, time_rolling.values, 
             linewidth=2, color='#f39c12')
    ax4.axhline(y=df['time_ms'].mean(), color='r', linestyle='--', 
                linewidth=1, label=f'Mean: {df["time_ms"].mean():.1f}ms')
    
    ax4.set_xlabel("Cycle", fontsize=10)
    ax4.set_ylabel("Time (ms)", fontsize=10)
    ax4.set_title(f"Performance (rolling avg, window={window})", fontsize=11)
    ax4.legend(fontsize=9)
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = "data/longrun_analysis.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n✅ Graph saved: {output_path}\n")
    
    # Statistics
    print("="*60)
    print("  STATISTICS")
    print("="*60 + "\n")
    
    print(f"Total Samples:    {len(df)}")
    print(f"Unique Cycles:    {df['cycle'].nunique()}")
    print(f"Unique Queries:   {df['query'].nunique()}\n")
    
    print("Energy Metrics:")
    print(f"  Mean:           {df['energy'].mean():.4f}")
    print(f"  Std Dev:        {df['energy'].std():.4f}")
    print(f"  Min:            {df['energy'].min():.4f}")
    print(f"  Max:            {df['energy'].max():.4f}\n")
    
    print("Weight Evolution:")
    print(f"  Initial node:   {df['avg_node_weight'].iloc[0]:.4f}")
    print(f"  Final node:     {df['avg_node_weight'].iloc[-1]:.4f}")
    print(f"  Change:         {(df['avg_node_weight'].iloc[-1] - df['avg_node_weight'].iloc[0]):.4f}")
    print(f"  Initial edge:   {df['avg_edge_weight'].iloc[0]:.4f}")
    print(f"  Final edge:     {df['avg_edge_weight'].iloc[-1]:.4f}")
    print(f"  Change:         {(df['avg_edge_weight'].iloc[-1] - df['avg_edge_weight'].iloc[0]):.4f}\n")
    
    print("Performance:")
    print(f"  Mean time:      {df['time_ms'].mean():.2f} ms")
    print(f"  Std dev:        {df['time_ms'].std():.2f} ms")
    print(f"  Min time:       {df['time_ms'].min():.2f} ms")
    print(f"  Max time:       {df['time_ms'].max():.2f} ms\n")
    
    print("Active State:")
    print(f"  Initial nodes:  {df['active_nodes'].iloc[0]}")
    print(f"  Final nodes:    {df['active_nodes'].iloc[-1]}")
    print(f"  Initial edges:  {df['active_edges'].iloc[0]}")
    print(f"  Final edges:    {df['active_edges'].iloc[-1]}\n")
    
    # Drift detection
    print("="*60)
    print("  STABILITY ANALYSIS")
    print("="*60 + "\n")
    
    # Check for metric drift
    energy_drift = abs(df['energy'].iloc[-100:].mean() - df['energy'].iloc[:100].mean())
    time_drift = abs(df['time_ms'].iloc[-100:].mean() - df['time_ms'].iloc[:100].mean())
    
    print(f"Energy drift (first vs last 100):  {energy_drift:.4f}")
    print(f"Time drift (first vs last 100):    {time_drift:.2f} ms\n")
    
    if energy_drift < 0.1:
        print("✅ Energy stable (drift < 0.1)")
    else:
        print(f"⚠️  Energy drift detected ({energy_drift:.4f})")
    
    if time_drift < 5.0:
        print("✅ Performance stable (drift < 5ms)")
    else:
        print(f"⚠️  Performance drift detected ({time_drift:.2f}ms)")
    
    print()
    
    # Show plot
    print("📊 Opening visualization...")
    plt.show()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

