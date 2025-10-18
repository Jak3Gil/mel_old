#!/usr/bin/env python3
"""
Graph Growth Analysis Script

Visualizes Melvin's brain expansion over time:
- Node count growth
- Edge count growth
- EXACT vs LEAP connection breakdown
- Growth rate analysis
"""

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def main():
    print("\n" + "="*60)
    print("  GRAPH GROWTH ANALYSIS")
    print("="*60 + "\n")
    
    # Load data
    if not os.path.exists("data/graph_growth.csv"):
        print("✗ Error: data/graph_growth.csv not found")
        print("  Run: ./tests/test_graph_growth first\n")
        return 1
    
    print("📂 Loading data...")
    df = pd.read_csv("data/graph_growth.csv")
    
    print(f"  ✓ Loaded {len(df)} cycles\n")
    
    # Create figure
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle("Melvin's Brain Growth Over Time", fontsize=16, fontweight='bold')
    
    # Subplot 1: Overall growth
    print("📊 Plotting overall growth...")
    ax1 = axes[0, 0]
    ax1.plot(df['cycle'], df['nodes'], label='Nodes', linewidth=2, color='#2ecc71')
    ax1.plot(df['cycle'], df['edges'], label='Edges', linewidth=2, color='#3498db')
    
    ax1.set_xlabel("Cycle", fontsize=11)
    ax1.set_ylabel("Count", fontsize=11)
    ax1.set_title("Total Nodes & Edges", fontsize=12)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Subplot 2: Edge type breakdown
    print("📊 Plotting edge types...")
    ax2 = axes[0, 1]
    ax2.plot(df['cycle'], df['exact'], label='EXACT (taught)', 
             linewidth=2, linestyle='-', color='#e74c3c')
    ax2.plot(df['cycle'], df['leap'], label='LEAP (inferred)', 
             linewidth=2, linestyle='--', color='#9b59b6')
    
    ax2.set_xlabel("Cycle", fontsize=11)
    ax2.set_ylabel("Count", fontsize=11)
    ax2.set_title("Connection Types", fontsize=12)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    
    # Subplot 3: Growth rates
    print("📊 Plotting growth rates...")
    ax3 = axes[1, 0]
    
    # Calculate growth (delta from previous)
    node_growth = df['nodes'].diff().fillna(0)
    edge_growth = df['edges'].diff().fillna(0)
    
    # Rolling average for smoother visualization
    window = max(1, len(df) // 20)
    node_growth_smooth = node_growth.rolling(window=window).mean()
    edge_growth_smooth = edge_growth.rolling(window=window).mean()
    
    ax3.plot(df['cycle'], node_growth_smooth, label='Nodes/cycle', 
             linewidth=2, color='#2ecc71')
    ax3.plot(df['cycle'], edge_growth_smooth, label='Edges/cycle', 
             linewidth=2, color='#3498db')
    
    ax3.set_xlabel("Cycle", fontsize=11)
    ax3.set_ylabel("Growth Rate", fontsize=11)
    ax3.set_title(f"Growth Rates (rolling avg, window={window})", fontsize=12)
    ax3.legend(fontsize=10)
    ax3.grid(True, alpha=0.3)
    ax3.axhline(y=0, color='k', linestyle='-', linewidth=0.5, alpha=0.3)
    
    # Subplot 4: LEAP inference effectiveness
    print("📊 Plotting LEAP growth...")
    ax4 = axes[1, 1]
    
    # Calculate LEAP as percentage of total edges
    leap_percent = (df['leap'] / df['edges'] * 100).fillna(0)
    
    ax4.plot(df['cycle'], leap_percent, linewidth=2, color='#9b59b6')
    ax4.axhline(y=leap_percent.mean(), color='r', linestyle='--', 
                linewidth=1, label=f'Mean: {leap_percent.mean():.1f}%')
    
    ax4.set_xlabel("Cycle", fontsize=11)
    ax4.set_ylabel("LEAP Percentage (%)", fontsize=11)
    ax4.set_title("LEAP Connections as % of Total", fontsize=12)
    ax4.legend(fontsize=10)
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = "data/graph_growth.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n✅ Graph saved: {output_path}\n")
    
    # Statistics
    print("="*60)
    print("  STATISTICS")
    print("="*60 + "\n")
    
    initial = df.iloc[0]
    final = df.iloc[-1]
    
    print("Initial State:")
    print(f"  Nodes:            {initial['nodes']:.0f}")
    print(f"  Edges:            {initial['edges']:.0f}")
    print(f"  EXACT:            {initial['exact']:.0f}")
    print(f"  LEAP:             {initial['leap']:.0f}\n")
    
    print("Final State:")
    print(f"  Nodes:            {final['nodes']:.0f} (+{final['nodes'] - initial['nodes']:.0f})")
    print(f"  Edges:            {final['edges']:.0f} (+{final['edges'] - initial['edges']:.0f})")
    print(f"  EXACT:            {final['exact']:.0f} (+{final['exact'] - initial['exact']:.0f})")
    print(f"  LEAP:             {final['leap']:.0f} (+{final['leap'] - initial['leap']:.0f})\n")
    
    print("Growth Rates:")
    total_cycles_actual = df['cycle'].max() - df['cycle'].min()
    if total_cycles_actual > 0:
        node_rate = (final['nodes'] - initial['nodes']) / total_cycles_actual
        edge_rate = (final['edges'] - initial['edges']) / total_cycles_actual
        exact_rate = (final['exact'] - initial['exact']) / total_cycles_actual
        leap_rate = (final['leap'] - initial['leap']) / total_cycles_actual
        
        print(f"  Nodes/cycle:      {node_rate:.3f}")
        print(f"  Edges/cycle:      {edge_rate:.3f}")
        print(f"  EXACT/cycle:      {exact_rate:.3f}")
        print(f"  LEAP/cycle:       {leap_rate:.3f}\n")
    
    print("LEAP Inference:")
    print(f"  LEAP percentage:  {leap_percent.iloc[-1]:.1f}%")
    print(f"  LEAP growth:      {(final['leap'] - initial['leap']):.0f} connections\n")
    
    print("Performance:")
    print(f"  Avg time/cycle:   {df['time_ms'].mean():.1f} ms")
    print(f"  Total time:       {df['time_ms'].sum() / 1000:.1f} seconds\n")
    
    # Validation
    print("="*60)
    print("  VALIDATION")
    print("="*60 + "\n")
    
    checks_passed = 0
    checks_total = 4
    
    # Check 1: Brain is growing
    if final['nodes'] > initial['nodes'] or final['edges'] > initial['edges']:
        print("✅ Brain is growing (nodes or edges increased)")
        checks_passed += 1
    else:
        print("⚠️  No growth detected")
    
    # Check 2: LEAP inference working
    if final['leap'] > initial['leap']:
        print("✅ LEAP inference working (LEAP connections added)")
        checks_passed += 1
    else:
        print("⚠️  No LEAP connections created")
    
    # Check 3: Performance stable
    time_increase = (df['time_ms'].iloc[-100:].mean() - df['time_ms'].iloc[:100].mean())
    if abs(time_increase) < 10:
        print("✅ Performance stable (time change < 10ms)")
        checks_passed += 1
    else:
        print(f"⚠️  Performance drift detected ({time_increase:.1f}ms)")
    
    # Check 4: No runaway growth
    max_growth = max(node_growth.max(), edge_growth.max())
    if max_growth < 100:
        print("✅ Growth controlled (no runaway expansion)")
        checks_passed += 1
    else:
        print(f"⚠️  Rapid growth detected (max delta: {max_growth:.0f})")
    
    print(f"\n🎯 Validation: {checks_passed}/{checks_total} checks passed\n")
    
    if checks_passed == checks_total:
        print("╔═══════════════════════════════════════════════════════╗")
        print("║  🎉 BRAIN GROWTH VALIDATED                            ║")
        print("╚═══════════════════════════════════════════════════════╝\n")
    else:
        print("⚠️  Some growth patterns may need investigation\n")
    
    # Show plot
    print("📊 Opening visualization...")
    plt.show()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

