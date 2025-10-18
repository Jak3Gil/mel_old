#!/usr/bin/env python3
"""
Quick demo: Image → YOLO → Knowledge Graph
Shows the vision system working without camera
"""

import json
import subprocess
import sys

def detect_objects(image_path):
    """Run YOLO detection"""
    cmd = f"python3 melvin/io/detect_objects.py {image_path}"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return json.loads(result.stdout)

def create_graph_nodes(detections, frame_num=1):
    """Convert detections to graph format"""
    nodes = []
    edges = []
    
    for i, det in enumerate(detections):
        # Create node
        node = {
            'id': i,
            'content': f"object:{det['label']}:{frame_num}:{i}",
            'type': 'SENSORY',
            'activation': det['confidence'],
            'bbox': [det['x1'], det['y1'], det['x2'], det['y2']]
        }
        nodes.append(node)
    
    # Create EXACT edges (intra-frame: all objects connected)
    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            edges.append({
                'from': i,
                'to': j,
                'type': 'EXACT',
                'weight': 1.0
            })
            edges.append({
                'from': j,
                'to': i,
                'type': 'EXACT',
                'weight': 1.0
            })
    
    return nodes, edges

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 demo_image_to_graph.py <image_path>")
        sys.exit(1)
    
    image_path = sys.argv[1]
    
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  👁️  IMAGE → YOLO → KNOWLEDGE GRAPH DEMO                       ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    # Step 1: Detect objects
    print(f"[1/3] Running YOLO detection on: {image_path}")
    detections = detect_objects(image_path)
    print(f"✓ Detected {len(detections)} objects")
    print()
    
    # Step 2: Convert to graph
    print("[2/3] Converting to knowledge graph...")
    nodes, edges = create_graph_nodes(detections)
    print(f"✓ Created {len(nodes)} nodes")
    print(f"✓ Created {len(edges)} edges (EXACT)")
    print()
    
    # Step 3: Display results
    print("[3/3] Knowledge Graph Preview:")
    print()
    print("NODES (SENSORY type):")
    for node in nodes:
        conf_pct = int(node['activation'] * 100)
        print(f"  [{node['id']}] {node['content']}")
        print(f"      confidence: {conf_pct}%")
    
    print()
    print("EDGES (EXACT type - spatial co-occurrence):")
    edge_pairs = set()
    for edge in edges:
        pair = tuple(sorted([edge['from'], edge['to']]))
        if pair not in edge_pairs:
            edge_pairs.add(pair)
            from_label = nodes[edge['from']]['content'].split(':')[1]
            to_label = nodes[edge['to']]['content'].split(':')[1]
            print(f"  {from_label} ←→ {to_label}")
    
    print()
    print("GRAPH STRUCTURE:")
    print(f"  Total nodes: {len(nodes)}")
    print(f"  Total edges: {len(edges)}")
    print(f"  Avg degree: {len(edges) / len(nodes):.1f} edges per node")
    
    # Save to file
    graph = {
        'metadata': {
            'source_image': image_path,
            'frame_number': 1,
            'num_objects': len(detections)
        },
        'nodes': nodes,
        'edges': edges
    }
    
    output_file = 'demo_graph_output.json'
    with open(output_file, 'w') as f:
        json.dump(graph, f, indent=2)
    
    print()
    print(f"💾 Saved to: {output_file}")
    print()
    print("✨ This is what gets added to Melvin's brain!")
    print("   (In C++ version, goes directly to nodes.melvin/edges.melvin)")
    print()

if __name__ == '__main__':
    main()

