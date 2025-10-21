#!/usr/bin/env python3
"""
GNN Training Visualizer

Watch Melvin's neural graph learn over time:
- Prediction error (should decrease)
- Embedding norm (should stabilize)
- Total loss (should decrease)
"""

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def main():
    print("\n" + "="*60)
    print("  GNN TRAINING VISUALIZATION")
    print("="*60 + "\n")
    
    # Load data
    if not os.path.exists("data/gnn_training.csv"):
        print("✗ Error: data/gnn_training.csv not found")
        print("  Run: ./demos/test_gnn_predictor first\n")
        return 1
    
    print("📂 Loading training data...")
    df = pd.read_csv("data/gnn_training.csv")
    
    print(f"  ✓ Loaded {len(df)} training steps\n")
    
    # Create figure
    fig, axes = plt.subplots(3, 1, figsize=(12, 10))
    fig.suptitle("GNN Predictor Training", fontsize=16, fontweight='bold')
    
    # Subplot 1: Prediction error
    print("📊 Plotting prediction error...")
    ax1 = axes[0]
    ax1.plot(df['step'], df['avg_error'], linewidth=2, color='#e74c3c')
    ax1.set_xlabel("Training Step", fontsize=11)
    ax1.set_ylabel("Average Prediction Error", fontsize=11)
    ax1.set_title("Prediction Error (Should Decrease)", fontsize=12)
    ax1.grid(True, alpha=0.3)
    
    # Subplot 2: Total loss
    print("📊 Plotting total loss...")
    ax2 = axes[1]
    ax2.plot(df['step'], df['total_loss'], linewidth=2, color='#3498db')
    ax2.set_xlabel("Training Step", fontsize=11)
    ax2.set_ylabel("Total Loss (MSE)", fontsize=11)
    ax2.set_title("Training Loss", fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.set_yscale('log')  # Log scale for better visualization
    
    # Subplot 3: Embedding norm
    print("📊 Plotting embedding norm...")
    ax3 = axes[2]
    ax3.plot(df['step'], df['avg_norm'], linewidth=2, color='#2ecc71')
    ax3.set_xlabel("Training Step", fontsize=11)
    ax3.set_ylabel("Average Embedding Norm", fontsize=11)
    ax3.set_title("Embedding Norm (Should Stabilize)", fontsize=12)
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save figure
    output_path = "data/gnn_training.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"\n✅ Graph saved: {output_path}\n")
    
    # Statistics
    print("="*60)
    print("  TRAINING STATISTICS")
    print("="*60 + "\n")
    
    initial = df.iloc[0]
    final = df.iloc[-1]
    
    print("Initial State:")
    print(f"  Avg error:        {initial['avg_error']:.4f}")
    print(f"  Total loss:       {initial['total_loss']:.4f}")
    print(f"  Embedding norm:   {initial['avg_norm']:.4f}\n")
    
    print("Final State:")
    print(f"  Avg error:        {final['avg_error']:.4f}")
    print(f"  Total loss:       {final['total_loss']:.4f}")
    print(f"  Embedding norm:   {final['avg_norm']:.4f}\n")
    
    print("Improvement:")
    error_reduction = (initial['avg_error'] - final['avg_error']) / initial['avg_error'] * 100
    loss_reduction = (initial['total_loss'] - final['total_loss']) / initial['total_loss'] * 100
    
    print(f"  Error reduction:  {error_reduction:.1f}%")
    print(f"  Loss reduction:   {loss_reduction:.1f}%\n")
    
    # Validation
    print("="*60)
    print("  VALIDATION")
    print("="*60 + "\n")
    
    checks_passed = 0
    checks_total = 3
    
    # Check 1: Error decreased
    if final['avg_error'] < initial['avg_error']:
        print("✅ Prediction error decreased (learning working!)")
        checks_passed += 1
    else:
        print("⚠️  Prediction error did not decrease")
    
    # Check 2: Loss decreased
    if final['total_loss'] < initial['total_loss']:
        print("✅ Total loss decreased (optimization working!)")
        checks_passed += 1
    else:
        print("⚠️  Total loss did not decrease")
    
    # Check 3: Embeddings stabilized
    norm_change = abs(final['avg_norm'] - df['avg_norm'].iloc[-10:].mean())
    if norm_change < 0.1:
        print("✅ Embeddings stabilized (norm converged)")
        checks_passed += 1
    else:
        print(f"⚠️  Embeddings still changing (variance: {norm_change:.3f})")
    
    print(f"\n🎯 Validation: {checks_passed}/{checks_total} checks passed\n")
    
    if checks_passed == checks_total:
        print("╔═══════════════════════════════════════════════════════╗")
        print("║  🎉 GNN PREDICTOR VALIDATED                           ║")
        print("╚═══════════════════════════════════════════════════════╝\n")
    else:
        print("⚠️  GNN may need more training steps\n")
    
    # Show plot
    print("📊 Opening visualization...")
    plt.show()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

