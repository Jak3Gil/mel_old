#!/usr/bin/env python3
"""
Export Melvin Run to Markdown

Exports a complete diagnostic run from the database to a well-formatted
Markdown document suitable for papers, reports, or documentation.

Features:
- Complete run metadata
- Metrics before/after comparison
- Qualitative samples with formatting
- Parameter settings
- Reproducibility information

Usage:
    python3 export_run_to_md.py 7                    # Export run 7
    python3 export_run_to_md.py 7 -o entry7.md       # Custom output
    python3 export_run_to_md.py --latest             # Export most recent run
    python3 export_run_to_md.py --tag milestone1     # Export by tag

Author: AI Research Infrastructure
Created: October 11, 2025
"""

import argparse
import sys
from pathlib import Path
from typing import Dict, Any, List, Optional
from datetime import datetime

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent / 'src'))

try:
    from database_manager import MelvinResearchDB
except ImportError:
    print("❌ Error: database_manager.py not found in src/")
    sys.exit(1)


def format_quality_indicator(quality: str) -> str:
    """Convert quality string to emoji indicator."""
    quality_map = {
        'strong': '🟢',
        'moderate': '🟡',
        'weak': '🟠',
        'none': '❌'
    }
    return quality_map.get(quality, '⚪')


def format_delta(before: float, after: float) -> str:
    """Format before→after with delta."""
    delta = after - before
    sign = '+' if delta >= 0 else ''
    return f"{before:.3f} → {after:.3f} (Δ: {sign}{delta:.3f})"


def export_run_to_markdown(
    run_data: Dict[str, Any],
    samples: List[Dict[str, Any]],
    output_file: Optional[str] = None
) -> str:
    """
    Generate Markdown document for a run.
    
    Args:
        run_data: Run dictionary from database
        samples: List of sample dictionaries
        output_file: Output file path (if None, return string only)
    
    Returns:
        Markdown content as string
    """
    run = run_data
    
    # Build markdown
    md = []
    
    # Header
    md.append(f"# Melvin Diagnostic Run {run['id']} - Entry {run['entry_number']}")
    md.append("")
    md.append(f"**Date:** {run['date']}")
    md.append(f"**Timestamp:** {run['timestamp']}")
    if run['tag']:
        md.append(f"**Tag:** `{run['tag']}`")
    md.append("")
    
    # Git Context
    md.append("## 📂 Git Context")
    md.append("")
    md.append(f"- **Commit Hash:** `{run['git_hash']}`")
    if run['tag']:
        md.append(f"- **Tag:** `{run['tag']}`")
    md.append(f"- **Reproducible:** `git checkout {run['git_hash']}`")
    md.append("")
    
    # Dataset
    md.append("## 📊 Dataset")
    md.append("")
    md.append(f"- **Version:** `{run['data_version']}`")
    md.append(f"- **Layer:** {run['data_layer']}")
    md.append(f"- **Size:** {run['data_size']}")
    md.append("")
    
    # Metrics
    md.append("## 📈 Metrics")
    md.append("")
    md.append("| Metric | Before | After | Delta |")
    md.append("|--------|--------|-------|-------|")
    
    delta_sim = run['similarity_after'] - run['similarity_before']
    delta_ent = run['entropy_after'] - run['entropy_before']
    delta_success = run['leap_success_after'] - run['leap_success_before']
    
    md.append(f"| **Context Similarity** | {run['similarity_before']:.3f} | "
              f"{run['similarity_after']:.3f} | "
              f"{'+' if delta_sim >= 0 else ''}{delta_sim:.3f} |")
    md.append(f"| **Entropy Reduction** | {run['entropy_before']:.3f} | "
              f"{run['entropy_after']:.3f} | "
              f"{'+' if delta_ent >= 0 else ''}{delta_ent:.3f} |")
    md.append(f"| **Leap Success Rate** | {run['leap_success_before']:.1f}% | "
              f"{run['leap_success_after']:.1f}% | "
              f"{'+' if delta_success >= 0 else ''}{delta_success:.1f}% |")
    md.append("")
    
    # Parameters
    md.append("## ⚙️ Parameters")
    md.append("")
    md.append(f"- **lambda_graph_bias:** {run['lambda_graph_bias']}")
    md.append(f"- **leap_entropy_threshold:** {run['leap_entropy_threshold']}")
    md.append(f"- **learning_rate_embeddings:** {run['learning_rate_embeddings']}")
    md.append("")
    
    # Qualitative Samples
    if samples:
        md.append("## 🧪 Qualitative Samples")
        md.append("")
        md.append(f"*Collected {len(samples)} samples during this run*")
        md.append("")
        
        # Group by quality
        quality_groups = {}
        for sample in samples:
            quality = sample['quality'] or 'none'
            if quality not in quality_groups:
                quality_groups[quality] = []
            quality_groups[quality].append(sample)
        
        # Summary
        md.append("### Summary")
        md.append("")
        md.append("| Quality | Count |")
        md.append("|---------|-------|")
        for quality in ['strong', 'moderate', 'weak', 'none']:
            count = len(quality_groups.get(quality, []))
            indicator = format_quality_indicator(quality)
            md.append(f"| {indicator} {quality.capitalize()} | {count} |")
        md.append("")
        
        # Detailed samples
        md.append("### Examples")
        md.append("")
        
        for sample in samples:
            quality = sample['quality'] or 'none'
            indicator = format_quality_indicator(quality)
            
            md.append(f"**Test:** `{sample['prompt']}`")
            md.append(f"- **Quality:** {indicator} {quality.capitalize()}")
            md.append(f"- **Output:** {sample['output'] or '(no output)'}")
            
            if sample['entropy'] is not None:
                md.append(f"- **Entropy:** {sample['entropy']:.3f}")
            if sample['similarity'] is not None:
                md.append(f"- **Similarity:** {sample['similarity']:.3f}")
            if sample['comment']:
                md.append(f"- **Comment:** {sample['comment']}")
            
            md.append("")
    
    # Notes
    if run['notes']:
        md.append("## 📝 Notes")
        md.append("")
        md.append(run['notes'])
        md.append("")
    
    # Footer
    md.append("---")
    md.append("")
    md.append(f"*Exported from Melvin Research Database on {datetime.utcnow().strftime('%Y-%m-%d %H:%M UTC')}*")
    md.append("")
    md.append("**Reproducibility:**")
    md.append("```bash")
    md.append(f"git checkout {run['git_hash']}")
    md.append(f"# Use data from {run['data_version']}")
    md.append("make run_diagnostic_quick")
    md.append(f"# Should reproduce: similarity {run['similarity_before']:.3f} → {run['similarity_after']:.3f}")
    md.append("```")
    
    # Join all lines
    content = "\n".join(md)
    
    # Write to file if requested
    if output_file:
        with open(output_file, 'w') as f:
            f.write(content)
        print(f"✅ Exported run {run['id']} to {output_file}")
    
    return content


def main():
    parser = argparse.ArgumentParser(
        description="Export Melvin diagnostic run to Markdown",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 export_run_to_md.py 7
  python3 export_run_to_md.py 7 -o reports/entry7.md
  python3 export_run_to_md.py --latest
  python3 export_run_to_md.py --entry 5
  python3 export_run_to_md.py --tag milestone1
        """
    )
    
    parser.add_argument('run_id', nargs='?', type=int,
                       help='Run ID to export')
    parser.add_argument('--db', default='melvin_research.db',
                       help='Database file (default: melvin_research.db)')
    parser.add_argument('-o', '--output', dest='output_file',
                       help='Output Markdown file (default: run_ID.md)')
    parser.add_argument('--latest', action='store_true',
                       help='Export most recent run')
    parser.add_argument('--entry', type=int,
                       help='Export by entry number')
    parser.add_argument('--tag',
                       help='Export by Git tag')
    parser.add_argument('--print', action='store_true', dest='print_only',
                       help='Print to stdout instead of file')
    
    args = parser.parse_args()
    
    # Check database exists
    if not Path(args.db).exists():
        print(f"❌ Error: Database not found: {args.db}")
        print("   Initialize with: python3 src/database_manager.py init")
        sys.exit(1)
    
    # Connect to database
    with MelvinResearchDB(args.db) as db:
        # Determine which run to export
        run_data = None
        
        if args.latest:
            runs = db.fetch_runs(order_by="date DESC", limit=1)
            if runs:
                run_data = runs[0]
            else:
                print("❌ No runs found in database")
                sys.exit(1)
        
        elif args.entry is not None:
            runs = db.fetch_runs(f"entry_number = {args.entry}", limit=1)
            if runs:
                run_data = runs[0]
            else:
                print(f"❌ No run found for entry {args.entry}")
                sys.exit(1)
        
        elif args.tag:
            runs = db.fetch_runs(f"tag = '{args.tag}'", limit=1)
            if runs:
                run_data = runs[0]
            else:
                print(f"❌ No run found with tag '{args.tag}'")
                sys.exit(1)
        
        elif args.run_id is not None:
            runs = db.fetch_runs(f"id = {args.run_id}")
            if runs:
                run_data = runs[0]
            else:
                print(f"❌ Run {args.run_id} not found")
                sys.exit(1)
        
        else:
            print("❌ Must specify run ID, --latest, --entry, or --tag")
            parser.print_help()
            sys.exit(1)
        
        # Fetch samples
        samples = db.fetch_samples(run_id=run_data['id'])
        
        # Determine output file
        if args.print_only:
            output_file = None
        elif args.output_file:
            output_file = args.output_file
        else:
            output_file = f"run_{run_data['id']}.md"
        
        # Export
        print(f"\n📄 Exporting Run {run_data['id']} (Entry {run_data['entry_number']})")
        print(f"   Date: {run_data['date']}")
        print(f"   Similarity: {run_data['similarity_after']:.3f}")
        print(f"   Samples: {len(samples)}")
        
        content = export_run_to_markdown(run_data, samples, output_file)
        
        if args.print_only:
            print("\n" + "="*70)
            print(content)
            print("="*70)
        
        print(f"\n✅ Export complete")


if __name__ == "__main__":
    main()

