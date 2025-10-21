#!/usr/bin/env python3
"""
Unified Binary Storage for MELVIN

ALL inputs (vision, audio, text) write to SAME binary files:
  • melvin_nodes.bin (all nodes)
  • melvin_edges.bin (all edges)

No separate files per modality!
"""

import struct
import numpy as np
from pathlib import Path

# ============================================================================
# BINARY FORMAT (Matches MELVIN v1 C++ structs)
# ============================================================================

# Node format (from types.h, struct Node)
# NodeID id (uint16_t, 2 bytes)
# uint8_t type_flags (1 byte)
# uint8_t metadata_idx (1 byte)
# uint64_t creation_timestamp (8 bytes)
# uint32_t episode_id (4 bytes)
# -- Runtime fields not stored in basic format --
# Total: 16 bytes minimum

# Edge format (from types.h, struct Edge)
# NodeID from_id (uint16_t, 2 bytes)
# NodeID to_id (uint16_t, 2 bytes)
# RelationType relation (uint8_t, 1 byte) - 0=EXACT, 1=LEAP
# uint8_t weight_scaled (1 byte) - 0-255 for 0.0-1.0
# uint16_t metadata_idx (2 bytes)
# uint64_t creation_timestamp (8 bytes)
# uint32_t episode_id (4 bytes)
# Total: 20 bytes minimum

class UnifiedBinaryStorage:
    """
    Read/write nodes and edges to unified binary files
    
    Compatible with MELVIN v1 C++ Storage class
    """
    
    def __init__(self, nodes_file='melvin_nodes.bin', edges_file='melvin_edges.bin'):
        self.nodes_file = nodes_file
        self.edges_file = edges_file
    
    def read_nodes(self):
        """Read all nodes from binary file"""
        if not Path(self.nodes_file).exists():
            return []
        
        nodes = []
        with open(self.nodes_file, 'rb') as f:
            while True:
                data = f.read(16)  # Minimum node size
                if len(data) < 16:
                    break
                
                # Unpack: H=uint16, B=uint8, Q=uint64, I=uint32
                node_id, type_flags, meta_idx, timestamp, episode = struct.unpack('<HBBQI', data)
                
                nodes.append({
                    'id': node_id,
                    'type_flags': type_flags,
                    'metadata_idx': meta_idx,
                    'timestamp': timestamp,
                    'episode': episode
                })
        
        return nodes
    
    def read_edges(self):
        """Read all edges from binary file"""
        if not Path(self.edges_file).exists():
            return []
        
        edges = []
        with open(self.edges_file, 'rb') as f:
            while True:
                data = f.read(20)  # Minimum edge size
                if len(data) < 20:
                    break
                
                # Unpack
                from_id, to_id, relation, weight, meta_idx, timestamp, episode = struct.unpack('<HHBBHQI', data)
                
                edges.append({
                    'from': from_id,
                    'to': to_id,
                    'relation': relation,
                    'weight': weight / 255.0,  # Scale back to 0.0-1.0
                    'metadata_idx': meta_idx,
                    'timestamp': timestamp,
                    'episode': episode
                })
        
        return edges
    
    def append_node(self, node_id, type_flags=0, metadata_idx=0, timestamp=0, episode=0):
        """Append one node to binary file"""
        with open(self.nodes_file, 'ab') as f:
            data = struct.pack('<HBBQI', 
                             node_id,
                             type_flags,
                             metadata_idx,
                             timestamp,
                             episode)
            f.write(data)
    
    def append_edge(self, from_id, to_id, relation=0, weight=1.0, metadata_idx=0, timestamp=0, episode=0):
        """Append one edge to binary file"""
        weight_scaled = int(np.clip(weight, 0.0, 1.0) * 255)
        
        with open(self.edges_file, 'ab') as f:
            data = struct.pack('<HHBBHQI',
                             from_id,
                             to_id,
                             relation,  # 0=EXACT, 1=LEAP
                             weight_scaled,
                             metadata_idx,
                             timestamp,
                             episode)
            f.write(data)
    
    def get_next_node_id(self):
        """Get next available node ID"""
        nodes = self.read_nodes()
        if not nodes:
            return 0
        return max(n['id'] for n in nodes) + 1
    
    def count_nodes(self):
        """Count total nodes"""
        nodes = self.read_nodes()
        return len(nodes)
    
    def count_edges(self):
        """Count total edges"""
        edges = self.read_edges()
        return len(edges)

# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

def convert_json_to_binary(json_file, storage):
    """Convert existing JSON graph to binary format"""
    import json
    
    if not Path(json_file).exists():
        return 0, 0
    
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    # Get current timestamp
    timestamp = int(time.time() * 1_000_000_000)  # Nanoseconds
    episode = 1  # Vision episode
    
    # Write nodes
    nodes_written = 0
    for node_data in data.get('nodes', []):
        storage.append_node(
            node_id=node_data['id'],
            type_flags=2,  # SENSORY type
            metadata_idx=0,
            timestamp=timestamp,
            episode=episode
        )
        nodes_written += 1
    
    # Write edges
    edges_written = 0
    for edge_data in data.get('edges', []):
        storage.append_edge(
            from_id=edge_data['from'],
            to_id=edge_data['to'],
            relation=0,  # EXACT
            weight=1.0,
            metadata_idx=0,
            timestamp=timestamp,
            episode=episode
        )
        edges_written += 1
    
    return nodes_written, edges_written

# ============================================================================
# TEST
# ============================================================================

if __name__ == "__main__":
    import sys
    import time
    
    print("\n" + "="*70)
    print("💾 Unified Binary Storage Test")
    print("="*70 + "\n")
    
    storage = UnifiedBinaryStorage()
    
    # Show current state
    nodes = storage.read_nodes()
    edges = storage.read_edges()
    
    print(f"Current state:")
    print(f"  melvin_nodes.bin: {len(nodes)} nodes")
    print(f"  melvin_edges.bin: {len(edges)} edges")
    print()
    
    # Check if we should convert JSON
    json_file = 'melvin_focus_graph.json'
    if Path(json_file).exists() and len(sys.argv) > 1 and sys.argv[1] == '--convert':
        print(f"Converting {json_file} to binary...")
        nodes_written, edges_written = convert_json_to_binary(json_file, storage)
        print(f"  ✅ Wrote {nodes_written} nodes to melvin_nodes.bin")
        print(f"  ✅ Wrote {edges_written} edges to melvin_edges.bin")
        print()
        
        # Verify
        nodes_after = storage.read_nodes()
        edges_after = storage.read_edges()
        print(f"New state:")
        print(f"  melvin_nodes.bin: {len(nodes_after)} nodes (+{len(nodes_after) - len(nodes)})")
        print(f"  melvin_edges.bin: {len(edges_after)} edges (+{len(edges_after) - len(edges)})")
        print()
        print("✅ Vision nodes now in unified binary storage!")
    else:
        print("To convert JSON to binary:")
        print(f"  python3 melvin/v2/tools/unified_binary_storage.py --convert")
        print()

