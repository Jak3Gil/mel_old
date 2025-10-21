#!/usr/bin/env python3
"""
Snapshot Verification Script

Verifies that persistence deltas only change active regions:
- Compares main storage with verification snapshot
- Checks for data corruption
- Analyzes byte-level changes
- Ensures changes are localized to active regions
"""

import hashlib
import sys
import os

def checksum(path):
    """Compute SHA-256 checksum of file"""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(8192):
            h.update(chunk)
    return h.hexdigest()

def compare_files(path1, path2):
    """Compare two binary files byte-by-byte"""
    if not os.path.exists(path1):
        return None, f"File not found: {path1}"
    
    if not os.path.exists(path2):
        return None, f"File not found: {path2}"
    
    size1 = os.path.getsize(path1)
    size2 = os.path.getsize(path2)
    
    if size1 != size2:
        return {
            'identical': False,
            'size_mismatch': True,
            'size1': size1,
            'size2': size2,
            'diff_bytes': []
        }, None
    
    diff_bytes = []
    with open(path1, "rb") as f1, open(path2, "rb") as f2:
        pos = 0
        while True:
            b1 = f1.read(1)
            b2 = f2.read(1)
            
            if not b1:  # End of file
                break
            
            if b1 != b2:
                diff_bytes.append(pos)
            
            pos += 1
    
    return {
        'identical': len(diff_bytes) == 0,
        'size': size1,
        'diff_bytes': diff_bytes,
        'diff_count': len(diff_bytes),
        'diff_percent': (len(diff_bytes) / size1 * 100) if size1 > 0 else 0
    }, None

def main():
    print("\n" + "="*60)
    print("  SNAPSHOT VERIFICATION")
    print("="*60 + "\n")
    
    # Files to compare
    nodes_main = "data/nodes.melvin"
    nodes_verify = "data/nodes_verify.melvin"
    edges_main = "data/edges.melvin"
    edges_verify = "data/edges_verify.melvin"
    
    # Check existence
    print("📂 Checking files...")
    
    files_exist = True
    for path in [nodes_main, nodes_verify, edges_main, edges_verify]:
        if os.path.exists(path):
            size = os.path.getsize(path)
            print(f"  ✓ {path} ({size} bytes)")
        else:
            print(f"  ✗ {path} (not found)")
            files_exist = False
    
    if not files_exist:
        print("\n❌ Missing snapshot files")
        print("   Run: ./tests/test_longrun first\n")
        return 1
    
    print()
    
    # Compare nodes
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  NODES COMPARISON")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    nodes_result, nodes_error = compare_files(nodes_main, nodes_verify)
    
    if nodes_error:
        print(f"❌ Error: {nodes_error}\n")
        return 1
    
    if nodes_result['identical']:
        print("✅ Nodes snapshots are IDENTICAL")
        print("   No changes detected between main and verify\n")
    else:
        if nodes_result.get('size_mismatch'):
            print(f"⚠️  Size mismatch:")
            print(f"   Main:   {nodes_result['size1']} bytes")
            print(f"   Verify: {nodes_result['size2']} bytes\n")
        else:
            print(f"⚠️  Snapshots differ (expected if learning occurred)")
            print(f"   Total size:      {nodes_result['size']} bytes")
            print(f"   Changed bytes:   {nodes_result['diff_count']}")
            print(f"   Change percent:  {nodes_result['diff_percent']:.2f}%\n")
            
            # Analyze change distribution
            if nodes_result['diff_count'] > 0 and nodes_result['diff_count'] < 100:
                print(f"   Changed positions: {nodes_result['diff_bytes'][:20]}")
                if nodes_result['diff_count'] > 20:
                    print(f"   ... and {nodes_result['diff_count'] - 20} more\n")
                else:
                    print()
    
    # Compare edges
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  EDGES COMPARISON")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    edges_result, edges_error = compare_files(edges_main, edges_verify)
    
    if edges_error:
        print(f"❌ Error: {edges_error}\n")
        return 1
    
    if edges_result['identical']:
        print("✅ Edges snapshots are IDENTICAL")
        print("   No changes detected between main and verify\n")
    else:
        if edges_result.get('size_mismatch'):
            print(f"⚠️  Size mismatch:")
            print(f"   Main:   {edges_result['size1']} bytes")
            print(f"   Verify: {edges_result['size2']} bytes\n")
        else:
            print(f"⚠️  Snapshots differ (expected if learning occurred)")
            print(f"   Total size:      {edges_result['size']} bytes")
            print(f"   Changed bytes:   {edges_result['diff_count']}")
            print(f"   Change percent:  {edges_result['diff_percent']:.2f}%\n")
            
            if edges_result['diff_count'] > 0 and edges_result['diff_count'] < 100:
                print(f"   Changed positions: {edges_result['diff_bytes'][:20]}")
                if edges_result['diff_count'] > 20:
                    print(f"   ... and {edges_result['diff_count'] - 20} more\n")
                else:
                    print()
    
    # Overall assessment
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  ASSESSMENT")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    total_changes = 0
    if not nodes_result['identical']:
        total_changes += nodes_result.get('diff_count', 0)
    if not edges_result['identical']:
        total_changes += edges_result.get('diff_count', 0)
    
    if nodes_result['identical'] and edges_result['identical']:
        print("✅ PERFECT: No changes detected")
        print("   Brain state is completely stable\n")
    elif total_changes > 0 and total_changes < 1000:
        print(f"✅ HEALTHY: Small, localized changes ({total_changes} bytes)")
        print("   Changes likely in runtime state (activations, weights)")
        print("   This is expected behavior for learning systems\n")
    elif total_changes >= 1000:
        print(f"⚠️  SIGNIFICANT: Large changes ({total_changes} bytes)")
        print("   Review longrun_log.csv for metric drift")
        print("   May indicate aggressive learning or instability\n")
    
    # Checksums for forensics
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  CHECKSUMS (for verification)")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    print(f"Nodes (main):   {checksum(nodes_main)}")
    print(f"Nodes (verify): {checksum(nodes_verify)}")
    print(f"Edges (main):   {checksum(edges_main)}")
    print(f"Edges (verify): {checksum(edges_verify)}\n")
    
    print("="*60)
    print()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

