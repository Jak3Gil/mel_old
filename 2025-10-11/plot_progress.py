#!/usr/bin/env python3
"""
Plot diagnostic progress over time
Visualizes entropy vs context similarity to identify when reasoning begins

Usage: ./plot_progress.py [--save]
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter
import numpy as np

def plot_diagnostic_progress(save_plot=False):
    """Plot entropy vs similarity and trend over time"""
    
    # Load history
    try:
        df = pd.read_csv('diagnostics_history.csv')
    except FileNotFoundError:
        print("❌ Error: diagnostics_history.csv not found")
        sys.exit(1)
    
    if len(df) < 2:
        print("⚠️  Need at least 2 data points to plot trends")
        sys.exit(0)
    
    # Parse dates
    df['date'] = pd.to_datetime(df['date'])
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('🧠 Melvin Graph-Guided Predictive System Progress', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Entropy Reduction over time
    ax1 = axes[0, 0]
    ax1.plot(df['date'], df['entropy_reduction'], 
             marker='o', linewidth=2, markersize=8, 
             color='#2E86AB', label='Entropy Reduction')
    ax1.axhline(y=0.20, color='r', linestyle='--', 
                linewidth=2, alpha=0.7, label='Target (0.20)')
    ax1.fill_between(df['date'], 0, df['entropy_reduction'], 
                      alpha=0.3, color='#2E86AB')
    ax1.set_xlabel('Date', fontsize=11)
    ax1.set_ylabel('Entropy Reduction', fontsize=11)
    ax1.set_title('📉 Graph Bias Effectiveness', fontsize=12, fontweight='bold')
    ax1.legend(loc='upper left')
    ax1.grid(True, alpha=0.3)
    ax1.set_ylim(bottom=0)
    
    # Plot 2: Context Similarity over time
    ax2 = axes[0, 1]
    ax2.plot(df['date'], df['context_similarity'], 
             marker='s', linewidth=2, markersize=8, 
             color='#A23B72', label='Context Similarity')
    ax2.axhline(y=0.50, color='r', linestyle='--', 
                linewidth=2, alpha=0.7, label='Coherence Threshold (0.50)')
    ax2.axhline(y=0.40, color='orange', linestyle=':', 
                linewidth=2, alpha=0.5, label='Qualitative Check (0.40)')
    ax2.fill_between(df['date'], 0, df['context_similarity'], 
                      alpha=0.3, color='#A23B72')
    ax2.set_xlabel('Date', fontsize=11)
    ax2.set_ylabel('Context Similarity', fontsize=11)
    ax2.set_title('🎯 Embedding Alignment', fontsize=12, fontweight='bold')
    ax2.legend(loc='upper left')
    ax2.grid(True, alpha=0.3)
    ax2.set_ylim(bottom=0, top=1.0)
    
    # Plot 3: Leap Success Rate over time
    ax3 = axes[1, 0]
    ax3.plot(df['date'], df['leap_success_rate'] * 100, 
             marker='^', linewidth=2, markersize=8, 
             color='#F18F01', label='Leap Success Rate')
    ax3.axhline(y=60, color='r', linestyle='--', 
                linewidth=2, alpha=0.7, label='Target (60%)')
    ax3.fill_between(df['date'], 0, df['leap_success_rate'] * 100, 
                      alpha=0.3, color='#F18F01')
    ax3.set_xlabel('Date', fontsize=11)
    ax3.set_ylabel('Success Rate (%)', fontsize=11)
    ax3.set_title('✅ Conceptual Leap Quality', fontsize=12, fontweight='bold')
    ax3.legend(loc='upper left')
    ax3.grid(True, alpha=0.3)
    ax3.set_ylim(bottom=0, top=100)
    
    # Plot 4: THE KEY PLOT - Entropy vs Similarity (shows when reasoning begins)
    ax4 = axes[1, 1]
    
    # Color points by date (oldest = blue, newest = red)
    colors = plt.cm.viridis(np.linspace(0, 1, len(df)))
    
    scatter = ax4.scatter(df['context_similarity'], df['entropy_reduction'], 
                          c=colors, s=100, alpha=0.7, edgecolors='black', linewidth=1)
    
    # Add arrows showing progression
    for i in range(len(df) - 1):
        ax4.annotate('', 
                     xy=(df['context_similarity'].iloc[i+1], 
                         df['entropy_reduction'].iloc[i+1]),
                     xytext=(df['context_similarity'].iloc[i], 
                            df['entropy_reduction'].iloc[i]),
                     arrowprops=dict(arrowstyle='->', lw=1.5, 
                                   color='gray', alpha=0.5))
    
    # Mark start and end
    ax4.plot(df['context_similarity'].iloc[0], df['entropy_reduction'].iloc[0],
             'go', markersize=12, label='Start', markeredgecolor='black', markeredgewidth=2)
    ax4.plot(df['context_similarity'].iloc[-1], df['entropy_reduction'].iloc[-1],
             'r*', markersize=15, label='Current', markeredgecolor='black', markeredgewidth=2)
    
    # Add target box
    target_box = plt.Rectangle((0.5, 0.2), 0.5, 0.5, 
                               fill=True, facecolor='green', 
                               alpha=0.1, edgecolor='green', 
                               linewidth=2, linestyle='--')
    ax4.add_patch(target_box)
    ax4.text(0.75, 0.45, 'TARGET\nZONE', 
             ha='center', va='center', fontsize=12, 
             fontweight='bold', color='green', alpha=0.5)
    
    # Add "reasoning begins" region
    ax4.axvspan(0.4, 1.0, alpha=0.05, color='blue')
    ax4.text(0.7, 0.05, 'Conceptual reasoning active →', 
             ha='center', va='center', fontsize=9, 
             style='italic', color='blue', alpha=0.7)
    
    ax4.set_xlabel('Context Similarity →', fontsize=11)
    ax4.set_ylabel('Entropy Reduction →', fontsize=11)
    ax4.set_title('🔬 Phase Transition: Guessing → Reasoning', 
                  fontsize=12, fontweight='bold')
    ax4.legend(loc='upper left')
    ax4.grid(True, alpha=0.3)
    ax4.set_xlim(-0.05, 1.0)
    ax4.set_ylim(bottom=-0.02)
    
    # Annotations for key insight
    ax4.text(0.02, 0.98, 'Curve flattens when\nmodel starts reasoning', 
             transform=ax4.transAxes, fontsize=9,
             verticalalignment='top', bbox=dict(boxstyle='round', 
             facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    
    if save_plot:
        filename = 'melvin_progress.png'
        plt.savefig(filename, dpi=300, bbox_inches='tight')
        print(f"✅ Plot saved to {filename}")
    else:
        plt.show()
    
    # Print current status
    print("\n📊 CURRENT STATUS:")
    print(f"   Entropy Reduction:  {df['entropy_reduction'].iloc[-1]:.3f}")
    print(f"   Context Similarity: {df['context_similarity'].iloc[-1]:.3f}")
    print(f"   Leap Success:       {df['leap_success_rate'].iloc[-1]*100:.1f}%")
    
    # Check for phase transition
    if len(df) >= 5:
        recent_similarity = df['context_similarity'].iloc[-5:].values
        recent_entropy = df['entropy_reduction'].iloc[-5:].values
        
        # Calculate rate of change
        sim_rate = np.mean(np.diff(recent_similarity))
        entropy_rate = np.mean(np.diff(recent_entropy))
        
        print(f"\n📈 RECENT TRENDS (last 5 runs):")
        print(f"   Similarity rate: {sim_rate:+.4f} per run")
        print(f"   Entropy rate:    {entropy_rate:+.4f} per run")
        
        # Detect phase transition (curve flattening)
        if sim_rate > 0.05 and entropy_rate < 0.02:
            print("\n🎉 PHASE TRANSITION DETECTED!")
            print("   → Similarity climbing while entropy stabilizing")
            print("   → Model transitioning from guessing to reasoning")
        elif sim_rate > 0.03:
            print("\n🚀 GOOD PROGRESS!")
            print("   → Strong upward trend in similarity")
        elif sim_rate < 0.01 and df['context_similarity'].iloc[-1] < 0.5:
            print("\n⚠️  PLATEAU DETECTED")
            print("   → Consider adding more diverse data")
            print("   → Or increase learning rate")

if __name__ == "__main__":
    save = '--save' in sys.argv
    
    print("\n🎨 Generating diagnostic progress plots...")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    try:
        plot_diagnostic_progress(save_plot=save)
    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)
    
    print("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("💡 The bottom-right plot is KEY:")
    print("   When the curve flattens (entropy stable, similarity rising),")
    print("   that's when the model transitions from guessing to reasoning!")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")

