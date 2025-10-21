#!/usr/bin/env python3
"""
Autonomous Thinking Visualizer

Visualize Melvin's autonomous thought process:
- Energy evolution without input
- Curiosity-driven activation
- Attention emergence
"""

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def main():
    print("\n" + "="*60)
    print("  AUTONOMOUS THINKING VISUALIZATION")
    print("="*60 + "\n")
    
    # Load data
    if not os.path.exists("data/autonomous_thinking.csv"):
        print("✗ Error: data/autonomous_thinking.csv not found")
        print("  Run: ./demos/test_autonomous_thinking first\n")
        return 1
    
    print("📂 Loading data...")
    df = pd.read_csv("data/autonomous_thinking.csv")
    
    print(f"  ✓ Loaded {len(df)} thinking steps\n")
    
    # Create figure
    fig, axes = plt.subplots(3, 1, figsize=(12, 10))
    fig.suptitle("Autonomous Thinking - Energy Without Input", fontsize=16, fontweight='bold')
    
    # Subplot 1: Total energy
    print("📊 Plotting energy evolution...")
    ax1 = axes[0]
    ax1.plot(df['step'], df['total_energy'], linewidth=2, color='#e74c3c')
    ax1.axhline(y=0.1, color='orange', linestyle='--', linewidth=1, label='Regen threshold')
    ax1.set_xlabel("Step", fontsize=11)
    ax1.set_ylabel("Total Energy", fontsize=11)
    ax1.set_title("Energy Evolution (No External Input!)", fontsize=12)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    
    # Subplot 2: Max activation (attention focus)
    print("📊 Plotting attention focus...")
    ax2 = axes[1]
    ax2.plot(df['step'], df['max_activation'], linewidth=2, color='#3498db')
    ax2.set_xlabel("Step", fontsize=11)
    ax2.set_ylabel("Max Node Activation", fontsize=11)
    ax2.set_title("Attention Focus (Strongest Node)", fontsize=12)
    ax2.grid(True, alpha=0.3)
    
    # Subplot 3: Curiosity injection
    print("📊 Plotting curiosity...")
    ax3 = axes[2]
    cumulative_curiosity = df['curiosity'].cumsum()
    ax3.plot(df['step'], cumulative_curiosity, linewidth=2, color='#2ecc71')
    ax3.set_xlabel("Step", fontsize=11)
    ax3.set_ylabel("Cumulative Curiosity", fontsize=11)
    ax3.set_title("Curiosity Accumulated (From Prediction Errors)", fontsize=12)
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = "data/autonomous_thinking.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n✅ Graph saved: {output_path}\n")
    
    # Statistics
    print("="*60)
    print("  STATISTICS")
    print("="*60 + "\n")
    
    initial = df.iloc[0]
    final = df.iloc[-1]
    
    print("Initial State:")
    print(f"  Total energy:     {initial['total_energy']:.4f}")
    print(f"  Max activation:   {initial['max_activation']:.4f}\n")
    
    print("Final State:")
    print(f"  Total energy:     {final['total_energy']:.4f}")
    print(f"  Max activation:   {final['max_activation']:.4f}\n")
    
    print("Autonomous Thinking:")
    print(f"  Total curiosity:  {df['curiosity'].sum():.4f}")
    print(f"  Avg energy:       {df['total_energy'].mean():.4f}")
    print(f"  Energy variance:  {df['total_energy'].std():.4f}\n")
    
    # Validation
    print("="*60)
    print("  VALIDATION")
    print("="*60 + "\n")
    
    checks_passed = 0
    checks_total = 4
    
    # Check 1: Energy persists
    if df['total_energy'].min() > 0:
        print("✅ Energy persists (never drops to zero)")
        checks_passed += 1
    else:
        print("⚠️  Energy dropped to zero")
    
    # Check 2: Curiosity generated
    if df['curiosity'].sum() > 0:
        print("✅ Curiosity generated (prediction errors drive thinking)")
        checks_passed += 1
    else:
        print("⚠️  No curiosity generated")
    
    # Check 3: Attention shifts
    attention_variance = df['max_activation'].std()
    if attention_variance > 0.01:
        print(f"✅ Attention shifts (variance: {attention_variance:.4f})")
        checks_passed += 1
    else:
        print("⚠️  Attention static")
    
    # Check 4: Energy decays but regenerates
    energy_changes = df['total_energy'].diff().dropna()
    has_decay = (energy_changes < 0).any()
    has_regen = (energy_changes > 0).any()
    
    if has_decay and has_regen:
        print("✅ Energy dynamics working (both decay and regeneration)")
        checks_passed += 1
    else:
        print("⚠️  Energy dynamics may need tuning")
    
    print(f"\n🎯 Validation: {checks_passed}/{checks_total} checks passed\n")
    
    if checks_passed == checks_total:
        print("╔═══════════════════════════════════════════════════════╗")
        print("║  🎉 AUTONOMOUS THINKING VALIDATED                     ║")
        print("╚═══════════════════════════════════════════════════════╝\n")
        print("Melvin can truly think on his own! 🧠✨\n")
    else:
        print("⚠️  Some aspects may need tuning\n")
    
    # Show plot
    print("📊 Opening visualization...")
    plt.show()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

