#!/usr/bin/env python3
"""
Export Melvin snapshot to human-readable format
"""

import sys
import json
from pathlib import Path
from datetime import datetime

def export_snapshot(snapshot_dir, output_file):
    """Export snapshot to JSON or Markdown"""
    
    print(f"╔═══════════════════════════════════════════════════════╗")
    print(f"║  MELVIN SNAPSHOT EXPORTER                             ║")
    print(f"╚═══════════════════════════════════════════════════════╝")
    print()
    
    snapshot_path = Path(snapshot_dir)
    if not snapshot_path.exists():
        print(f"❌ Snapshot directory not found: {snapshot_dir}")
        return 1
    
    # Read snapshot files
    nodes_file = snapshot_path / "nodes.melvin"
    edges_file = snapshot_path / "edges.melvin"
    
    if not nodes_file.exists() or not edges_file.exists():
        print(f"❌ Snapshot files not found in {snapshot_dir}")
        return 1
    
    print(f"📂 Reading snapshot from: {snapshot_dir}")
    
    # TODO: Implement binary parsing
    # For now, create a basic export structure
    
    export_data = {
        "metadata": {
            "export_time": datetime.now().isoformat(),
            "snapshot_dir": str(snapshot_dir),
        },
        "nodes": [],
        "edges": [],
        "statistics": {
            "node_count": 0,
            "edge_count": 0,
        }
    }
    
    output_path = Path(output_file)
    
    if output_path.suffix == ".json":
        with open(output_path, 'w') as f:
            json.dump(export_data, f, indent=2)
        print(f"✅ Exported to JSON: {output_file}")
    elif output_path.suffix == ".md":
        with open(output_path, 'w') as f:
            f.write(f"# Melvin Snapshot Export\n\n")
            f.write(f"**Exported:** {export_data['metadata']['export_time']}\n\n")
            f.write(f"**Source:** {snapshot_dir}\n\n")
            f.write(f"## Statistics\n\n")
            f.write(f"- Nodes: {export_data['statistics']['node_count']}\n")
            f.write(f"- Edges: {export_data['statistics']['edge_count']}\n\n")
        print(f"✅ Exported to Markdown: {output_file}")
    else:
        print(f"❌ Unsupported output format: {output_path.suffix}")
        return 1
    
    return 0

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: export_snapshot.py <snapshot_dir> <output_file>")
        print("  output_file: .json or .md")
        sys.exit(1)
    
    sys.exit(export_snapshot(sys.argv[1], sys.argv[2]))

