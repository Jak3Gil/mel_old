#!/usr/bin/env python3
"""
Plot Melvin Progress from Database

Generates diagnostic progress visualizations directly from the SQLite database.
Replaces CSV-based plotting with queryable database backend.

Features:
- Multi-run time series plotting
- Entropy vs similarity phase diagram
- Leap success tracking
- Custom date ranges and filtering
- High-quality PNG output

Usage:
    python3 plot_from_db.py --save              # Generate melvin_progress_db.png
    python3 plot_from_db.py --since 2025-10-01  # Plot runs since October
    python3 plot_from_db.py --entry-min 5       # Only Entry 5 onwards
    python3 plot_from_db.py --query "similarity_after >= 0.3"  # Custom filter

Author: AI Research Infrastructure
Created: October 11, 2025
"""

import argparse
import sys
from pathlib import Path
from typing import List, Dict, Any, Optional

try:
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    from datetime import datetime
except ImportError:
    print("❌ Error: matplotlib required. Install with: pip install matplotlib")
    sys.exit(1)

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent / 'src'))

try:
    from database_manager import MelvinResearchDB
except ImportError:
    print("❌ Error: database_manager.py not found in src/")
    sys.exit(1)


def fetch_runs_for_plotting(
    db: MelvinResearchDB,
    where_clause: Optional[str] = None,
    entry_min: Optional[int] = None,
    entry_max: Optional[int] = None,
    since_date: Optional[str] = None
) -> List[Dict[str, Any]]:
    """
    Fetch runs from database with optional filtering.
    
    Args:
        db: Database connection
        where_clause: Custom SQL WHERE clause
        entry_min: Minimum entry number
        entry_max: Maximum entry number
        since_date: Only runs on or after this date (YYYY-MM-DD)
    
    Returns:
        List of run dictionaries sorted by date
    """
    conditions = []
    
    if where_clause:
        conditions.append(f"({where_clause})")
    if entry_min is not None:
        conditions.append(f"entry_number >= {entry_min}")
    if entry_max is not None:
        conditions.append(f"entry_number <= {entry_max}")
    if since_date:
        conditions.append(f"date >= '{since_date}'")
    
    where = " AND ".join(conditions) if conditions else None
    
    runs = db.fetch_runs(where_clause=where, order_by="date ASC")
    
    return runs


def plot_progress(
    runs: List[Dict[str, Any]],
    output_file: str = "melvin_progress_db.png"
) -> None:
    """
    Generate 4-panel progress plot from database runs.
    
    Args:
        runs: List of run dictionaries from database
        output_file: Output PNG filename
    """
    if not runs:
        print("⚠️  No runs to plot")
        return
    
    # Extract data
    dates = [datetime.fromisoformat(run['date']) for run in runs]
    entries = [run['entry_number'] for run in runs]
    
    entropy_reduction = [run['entropy_after'] for run in runs]
    context_similarity = [run['similarity_after'] for run in runs]
    leap_success = [run['leap_success_after'] for run in runs]
    
    # Create figure with 4 subplots
    fig = plt.figure(figsize=(16, 10))
    gs = fig.add_gridspec(2, 2, hspace=0.3, wspace=0.3)
    
    # Style
    plt.style.use('seaborn-v0_8-darkgrid')
    
    # Subplot 1: Entropy Reduction over Time
    ax1 = fig.add_subplot(gs[0, 0])
    ax1.plot(dates, entropy_reduction, 'o-', color='#2E86AB', linewidth=2, 
             markersize=8, label='Entropy Reduction')
    ax1.axhline(y=0.08, color='green', linestyle='--', alpha=0.5, label='Target: 0.08')
    ax1.set_xlabel('Date', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Entropy Reduction', fontsize=12, fontweight='bold')
    ax1.set_title('📉 Entropy Reduction Over Time', fontsize=14, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    ax1.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45)
    
    # Subplot 2: Context Similarity over Time
    ax2 = fig.add_subplot(gs[0, 1])
    ax2.plot(dates, context_similarity, 'o-', color='#A23B72', linewidth=2,
             markersize=8, label='Context Similarity')
    ax2.axhline(y=0.35, color='orange', linestyle='--', alpha=0.5, label='Milestone 1: 0.35')
    ax2.axhline(y=0.50, color='red', linestyle='--', alpha=0.5, label='Coherence: 0.50')
    ax2.set_xlabel('Date', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Context Similarity', fontsize=12, fontweight='bold')
    ax2.set_title('🎯 Context Similarity Over Time', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    plt.setp(ax2.xaxis.get_majorticklabels(), rotation=45)
    
    # Subplot 3: Leap Success Rate over Time
    ax3 = fig.add_subplot(gs[1, 0])
    ax3.plot(dates, leap_success, 'o-', color='#F18F01', linewidth=2,
             markersize=8, label='Leap Success Rate')
    ax3.axhline(y=70, color='green', linestyle='--', alpha=0.5, label='Target: 70%')
    ax3.set_xlabel('Date', fontsize=12, fontweight='bold')
    ax3.set_ylabel('Leap Success Rate (%)', fontsize=12, fontweight='bold')
    ax3.set_title('🚀 Leap Success Rate Over Time', fontsize=14, fontweight='bold')
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    plt.setp(ax3.xaxis.get_majorticklabels(), rotation=45)
    
    # Subplot 4: Phase Diagram (Entropy vs Similarity) - THE KEY PLOT
    ax4 = fig.add_subplot(gs[1, 1])
    
    # Color by entry number for trajectory visualization
    scatter = ax4.scatter(context_similarity, entropy_reduction, 
                         c=entries, cmap='viridis', s=150, 
                         alpha=0.7, edgecolors='black', linewidth=1.5)
    
    # Connect points to show trajectory
    ax4.plot(context_similarity, entropy_reduction, 'k-', alpha=0.3, linewidth=1)
    
    # Mark start and end
    if len(runs) > 1:
        ax4.scatter(context_similarity[0], entropy_reduction[0], 
                   s=300, marker='o', color='green', edgecolors='black',
                   linewidth=2, label='Start', zorder=10)
        ax4.scatter(context_similarity[-1], entropy_reduction[-1],
                   s=300, marker='*', color='red', edgecolors='black',
                   linewidth=2, label='Latest', zorder=10)
    
    # Reference regions
    ax4.axvline(x=0.35, color='orange', linestyle='--', alpha=0.3, label='Milestone 1')
    ax4.axvline(x=0.50, color='red', linestyle='--', alpha=0.3, label='Coherence')
    ax4.axhline(y=0.08, color='green', linestyle='--', alpha=0.3)
    
    ax4.set_xlabel('Context Similarity', fontsize=12, fontweight='bold')
    ax4.set_ylabel('Entropy Reduction', fontsize=12, fontweight='bold')
    ax4.set_title('🧠 Phase Diagram: Learning → Reasoning Transition', 
                  fontsize=14, fontweight='bold')
    ax4.grid(True, alpha=0.3)
    
    # Colorbar for entry progression
    cbar = plt.colorbar(scatter, ax=ax4)
    cbar.set_label('Entry Number', fontsize=10, fontweight='bold')
    
    ax4.legend(loc='upper left', fontsize=9)
    
    # Add annotation
    ax4.text(0.02, 0.98, 
             'When curve flattens (entropy stable, similarity rising),\n'
             'the model transitions from guessing to reasoning!',
             transform=ax4.transAxes, fontsize=9, 
             verticalalignment='top', bbox=dict(boxstyle='round', 
             facecolor='wheat', alpha=0.5))
    
    # Overall title
    latest_sim = context_similarity[-1] if context_similarity else 0.0
    latest_ent = entropy_reduction[-1] if entropy_reduction else 0.0
    fig.suptitle(f'Melvin Diagnostic Progress - {len(runs)} Runs | '
                 f'Latest: Sim={latest_sim:.3f}, Ent={latest_ent:.3f}',
                 fontsize=16, fontweight='bold', y=0.98)
    
    # Save
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\n✅ Plot saved to {output_file}")
    
    # Print stats
    print(f"\n📊 CURRENT STATUS:")
    print(f"   Entropy Reduction:  {latest_ent:.3f}")
    print(f"   Context Similarity: {latest_sim:.3f}")
    print(f"   Leap Success:       {leap_success[-1]:.1f}%")
    
    # Close to avoid memory issues
    plt.close()


def plot_parameter_evolution(
    runs: List[Dict[str, Any]],
    output_file: str = "melvin_parameters_db.png"
) -> None:
    """
    Plot evolution of tuning parameters over time.
    
    Args:
        runs: List of run dictionaries from database
        output_file: Output PNG filename
    """
    if not runs:
        print("⚠️  No runs to plot")
        return
    
    dates = [datetime.fromisoformat(run['date']) for run in runs]
    lambda_vals = [run['lambda_graph_bias'] for run in runs]
    threshold_vals = [run['leap_entropy_threshold'] for run in runs]
    lr_vals = [run['learning_rate_embeddings'] for run in runs]
    
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle('Parameter Evolution Over Time', fontsize=16, fontweight='bold')
    
    # Lambda
    ax1.plot(dates, lambda_vals, 'o-', color='#2E86AB', linewidth=2, markersize=8)
    ax1.set_ylabel('lambda_graph_bias', fontsize=11, fontweight='bold')
    ax1.set_title('Graph Bias Strength', fontsize=12)
    ax1.grid(True, alpha=0.3)
    ax1.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    
    # Threshold
    ax2.plot(dates, threshold_vals, 'o-', color='#A23B72', linewidth=2, markersize=8)
    ax2.set_ylabel('leap_entropy_threshold', fontsize=11, fontweight='bold')
    ax2.set_title('Leap Entropy Threshold', fontsize=12)
    ax2.grid(True, alpha=0.3)
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    
    # Learning rate
    ax3.plot(dates, lr_vals, 'o-', color='#F18F01', linewidth=2, markersize=8)
    ax3.set_ylabel('learning_rate_embeddings', fontsize=11, fontweight='bold')
    ax3.set_xlabel('Date', fontsize=12, fontweight='bold')
    ax3.set_title('Embedding Learning Rate', fontsize=12)
    ax3.grid(True, alpha=0.3)
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d'))
    
    for ax in [ax1, ax2, ax3]:
        plt.setp(ax.xaxis.get_majorticklabels(), rotation=45)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✅ Parameter evolution plot saved to {output_file}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description="Plot Melvin progress from database",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 plot_from_db.py --save
  python3 plot_from_db.py --since 2025-10-01
  python3 plot_from_db.py --entry-min 5 --entry-max 10
  python3 plot_from_db.py --query "similarity_after >= 0.3"
  python3 plot_from_db.py --parameters  # Plot parameter evolution
        """
    )
    
    parser.add_argument('--db', default='melvin_research.db',
                       help='Database file (default: melvin_research.db)')
    parser.add_argument('--save', action='store_true',
                       help='Save plot as PNG')
    parser.add_argument('--output', default='melvin_progress_db.png',
                       help='Output filename (default: melvin_progress_db.png)')
    parser.add_argument('--since', dest='since_date',
                       help='Only plot runs on or after this date (YYYY-MM-DD)')
    parser.add_argument('--entry-min', type=int,
                       help='Minimum entry number')
    parser.add_argument('--entry-max', type=int,
                       help='Maximum entry number')
    parser.add_argument('--query', dest='where_clause',
                       help='Custom SQL WHERE clause')
    parser.add_argument('--parameters', action='store_true',
                       help='Plot parameter evolution instead')
    parser.add_argument('--show', action='store_true',
                       help='Display plot interactively (requires display)')
    
    args = parser.parse_args()
    
    # Check if database exists
    if not Path(args.db).exists():
        print(f"❌ Error: Database not found: {args.db}")
        print("   Initialize with: python3 src/database_manager.py init")
        sys.exit(1)
    
    print(f"\n🔍 Reading database: {args.db}")
    
    # Connect to database
    with MelvinResearchDB(args.db) as db:
        # Fetch runs
        runs = fetch_runs_for_plotting(
            db,
            where_clause=args.where_clause,
            entry_min=args.entry_min,
            entry_max=args.entry_max,
            since_date=args.since_date
        )
        
        if not runs:
            print("⚠️  No runs found matching criteria")
            sys.exit(1)
        
        print(f"✅ Found {len(runs)} runs to plot")
        print(f"   Date range: {runs[0]['date']} to {runs[-1]['date']}")
        print(f"   Entry range: {runs[0]['entry_number']} to {runs[-1]['entry_number']}")
        
        # Generate appropriate plot
        if args.parameters:
            output = args.output.replace('.png', '_parameters.png')
            plot_parameter_evolution(runs, output)
        else:
            plot_progress(runs, args.output)
        
        # Show database stats
        print("\n📊 Database Overview:")
        stats = db.get_stats()
        print(f"   Total runs: {stats['total_runs']}")
        print(f"   Total samples: {stats['total_samples']}")
        print(f"   Best similarity: {stats['best_similarity']:.3f}")
        
        if args.show:
            print("\n📊 Opening plot...")
            plt.show()


if __name__ == "__main__":
    main()

