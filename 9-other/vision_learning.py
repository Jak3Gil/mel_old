#!/usr/bin/env python3
"""
Melvin Vision Learning - Camera to Brain Graph
Converts visual attention into nodes and EXACT/LEAP edges

Uses Adaptive EXACT Window:
- New visual concepts → large temporal window (up to 50 frames)
- Familiar visual concepts → small temporal window (5 frames)
- EXACT edges decay exponentially with frame distance
"""

import cv2
import numpy as np
import sys
import struct
import os
from pathlib import Path
from collections import deque

class VisionLearning:
    def __init__(self, camera_id=0, graph_path="data"):
        self.camera_id = camera_id
        self.graph_path = Path(graph_path)
        self.patch_size = 64
        
        # Graph data structures
        self.nodes = {}  # label -> node_id
        self.edges = []  # list of (from, to, type, weight)
        self.next_node_id = 1
        
        # Temporal tracking with ADAPTIVE WINDOW
        self.temporal_buffer = deque(maxlen=100)  # Keep last 100 frames
        self.node_activation_counts = {}  # Track how often each node appears
        self.frame_count = 0
        
        # Edge types
        self.EXACT = 0  # Direct observation (adaptive temporal window)
        self.LEAP = 1   # Inferred temporal (beyond EXACT window)
        
        # Adaptive window config (from adaptive_window_config.h)
        self.N_min = 5           # Minimum frames to connect
        self.N_max = 50          # Maximum frames to connect
        self.novelty_decay = 0.1
        self.novelty_boost = 0.5
        self.temporal_decay_lambda = 0.1
        self.min_edge_weight = 0.01
        
        # Motion detection
        self.prev_frame_gray = None
        
        # Visualization (from old system)
        self.focus_history = []
        self.color_history = []
        self.shape_history = []
        
        # Load existing graph if it exists
        self.load_graph()
        
        # Camera setup
        print(f"🔍 Opening camera {camera_id}...")
        self.cap = cv2.VideoCapture(camera_id)
        if not self.cap.isOpened():
            raise RuntimeError(f"Cannot open camera {camera_id}")
        
        ret, test_frame = self.cap.read()
        if not ret or test_frame is None:
            raise RuntimeError(f"Cannot read from camera {camera_id}")
        
        self.height, self.width = test_frame.shape[:2]
        print(f"✅ Camera opened: {self.width}x{self.height}")
        print(f"   Graph: {len(self.nodes)} nodes, {len(self.edges)} edges")
    
    def load_graph(self):
        """Load existing graph from disk"""
        nodes_file = self.graph_path / "vision_nodes.dat"
        edges_file = self.graph_path / "vision_edges.dat"
        
        if nodes_file.exists():
            with open(nodes_file, 'r') as f:
                for line in f:
                    parts = line.strip().split('\t')
                    if len(parts) == 2:
                        node_id, label = int(parts[0]), parts[1]
                        self.nodes[label] = node_id
                        self.next_node_id = max(self.next_node_id, node_id + 1)
        
        if edges_file.exists():
            with open(edges_file, 'r') as f:
                for line in f:
                    parts = line.strip().split('\t')
                    if len(parts) == 4:
                        from_id = int(parts[0])
                        to_id = int(parts[1])
                        edge_type = int(parts[2])
                        weight = float(parts[3])
                        self.edges.append((from_id, to_id, edge_type, weight))
    
    def save_graph(self):
        """Save graph to disk"""
        self.graph_path.mkdir(exist_ok=True)
        
        # Save nodes
        with open(self.graph_path / "vision_nodes.dat", 'w') as f:
            for label, node_id in sorted(self.nodes.items(), key=lambda x: x[1]):
                f.write(f"{node_id}\t{label}\n")
        
        # Save edges
        with open(self.graph_path / "vision_edges.dat", 'w') as f:
            for from_id, to_id, edge_type, weight in self.edges:
                f.write(f"{from_id}\t{to_id}\t{edge_type}\t{weight}\n")
    
    def get_or_create_node(self, label):
        """Get existing node or create new one"""
        if label in self.nodes:
            return self.nodes[label]
        
        node_id = self.next_node_id
        self.nodes[label] = node_id
        self.next_node_id += 1
        return node_id
    
    def add_edge(self, from_id, to_id, edge_type, weight=1.0):
        """Add edge to graph (or increment if exists)"""
        # Check if edge exists
        for i, (f, t, et, w) in enumerate(self.edges):
            if f == from_id and t == to_id and et == edge_type:
                # Update weight
                self.edges[i] = (f, t, et, w + weight)
                return
        
        # New edge
        self.edges.append((from_id, to_id, edge_type, weight))
    
    def compute_attention(self, img):
        """Compute attention and extract visual concepts (with full scores for viz)"""
        gray_frame = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        
        # Motion map
        motion_map = None
        if self.prev_frame_gray is not None:
            frame_diff = cv2.absdiff(gray_frame, self.prev_frame_gray)
            _, motion_map = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)
        self.prev_frame_gray = gray_frame.copy()
        
        scores = []
        grid_h = self.height // self.patch_size
        grid_w = self.width // self.patch_size
        
        for py in range(grid_h):
            for px in range(grid_w):
                y = py * self.patch_size
                x = px * self.patch_size
                
                if y + self.patch_size > self.height or x + self.patch_size > self.width:
                    continue
                
                patch = img[y:y+self.patch_size, x:x+self.patch_size]
                gray = gray_frame[y:y+self.patch_size, x:x+self.patch_size]
                
                # Features
                avg_color = np.mean(patch, axis=(0,1))
                color_name = self.classify_color(avg_color)
                
                # Saliency
                saliency = float(np.std(gray)) / 128.0
                
                # Motion (Goal)
                goal = 0.0
                if motion_map is not None:
                    motion_patch = motion_map[y:y+self.patch_size, x:x+self.patch_size]
                    goal = float(np.sum(motion_patch > 0)) / (self.patch_size * self.patch_size) * 2.0
                
                # Curiosity (edges)
                edges = cv2.Canny(gray, 50, 150)
                curiosity = float(np.sum(edges > 0)) / (self.patch_size * self.patch_size)
                shape_type = 'edgy' if curiosity > 0.3 else 'smooth'
                
                # Diversity
                diversity = self._compute_diversity(color_name, shape_type)
                
                # Total focus: F = S + G + C + D
                focus = saliency + goal + curiosity + diversity
                
                # Store for visualization
                scores.append({
                    'x': px, 'y': py,
                    'cx': x + self.patch_size//2,
                    'cy': y + self.patch_size//2,
                    'saliency': saliency,
                    'goal': goal,
                    'curiosity': curiosity,
                    'diversity': diversity,
                    'focus': focus,
                    'color': color_name,
                    'shape': shape_type
                })
        
        # ONLY create concept if crosshair is on a CLUSTER of highlighted boxes
        concepts = []
        if scores:
            best = max(scores, key=lambda s: s['focus'])
            
            # Count how many neighboring patches are also bright (cluster detection)
            cluster_threshold = 0.6  # Patches above this are "highlighted"
            cluster_size = 0
            
            for s in scores:
                # Check if patch is near the focus point (within 2 patches)
                dx = abs(s['x'] - best['x'])
                dy = abs(s['y'] - best['y'])
                
                if dx <= 2 and dy <= 2 and s['focus'] > cluster_threshold:
                    cluster_size += 1
            
            # Only create node if there's a cluster (3+ highlighted patches together)
            if cluster_size >= 3:
                # Add spatial region for granularity
                region_x = best['x'] // 5
                region_y = best['y'] // 3
                
                concept = f"visual:{best['color']}:{best['shape']}:r{region_x}{region_y}"
                concepts.append({
                    'label': concept,
                    'focus': best['focus'],
                    'color': best['color'],
                    'shape': best['shape'],
                    'x': best['x'],
                    'y': best['y'],
                    'cluster_size': cluster_size
                })
        
        return scores, concepts
    
    def _compute_diversity(self, color, shape):
        """Compute diversity score"""
        diversity = 0.0
        recent_colors = self.color_history[-20:] if len(self.color_history) > 20 else self.color_history
        
        if len(recent_colors) > 10:
            color_count = recent_colors.count(color)
            color_ratio = color_count / len(recent_colors)
            if color_ratio > 0.6:
                diversity -= 0.25
            elif color_ratio < 0.2:
                diversity += 0.25
        
        recent_shapes = self.shape_history[-20:] if len(self.shape_history) > 20 else self.shape_history
        if len(recent_shapes) > 10:
            shape_count = recent_shapes.count(shape)
            shape_ratio = shape_count / len(recent_shapes)
            if shape_ratio > 0.6:
                diversity -= 0.15
            elif shape_ratio < 0.2:
                diversity += 0.15
        
        return np.clip(diversity, -0.4, 0.4)
    
    def classify_color(self, avg_bgr):
        """Classify color"""
        b, g, r = avg_bgr
        if r > 150 and r > g + 30 and r > b + 30:
            return 'red'
        elif b > 150 and b > r + 30 and b > g + 30:
            return 'blue'
        elif g > 150 and g > r + 30 and g > b + 30:
            return 'green'
        elif r > 200 and g > 200 and b > 200:
            return 'bright'
        elif r < 80 and g < 80 and b < 80:
            return 'dark'
        else:
            return 'neutral'
    
    def calculate_novelty(self, node_id):
        """Calculate novelty based on activation count"""
        activations = self.node_activation_counts.get(node_id, 0)
        return 1.0 / (1.0 + activations * self.novelty_decay)
    
    def calculate_strength(self, node_id):
        """Calculate strength (simplified: use activation frequency)"""
        activations = self.node_activation_counts.get(node_id, 0)
        max_activations = 100.0  # Normalize to 100 frames
        return min(1.0, activations / max_activations)
    
    def calculate_window_size(self, novelty, strength):
        """Calculate adaptive N_exact window size"""
        base_factor = 1.0 - strength
        novelty_factor = 1.0 + (novelty * self.novelty_boost)
        
        window_range = self.N_max - self.N_min
        window_size = self.N_min + (window_range * base_factor * novelty_factor)
        
        return int(min(self.N_max, max(self.N_min, window_size)))
    
    def calculate_temporal_weight(self, distance):
        """Calculate edge weight with exponential decay"""
        import math
        weight = math.exp(-self.temporal_decay_lambda * distance)
        return weight if weight >= self.min_edge_weight else 0.0
    
    def process_frame(self, img):
        """Process frame and update graph with ADAPTIVE EXACT WINDOW"""
        scores, concepts = self.compute_attention(img)
        
        # Track which nodes are new this frame
        new_nodes_created = []
        
        # Create nodes for concepts
        current_node_ids = []
        for concept in concepts:
            # Check if this is a NEW node
            is_new = concept['label'] not in self.nodes
            
            node_id = self.get_or_create_node(concept['label'])
            current_node_ids.append(node_id)
            
            # Update activation count
            self.node_activation_counts[node_id] = self.node_activation_counts.get(node_id, 0) + 1
            
            # Calculate adaptive window for this node
            novelty = self.calculate_novelty(node_id)
            strength = self.calculate_strength(node_id)
            N_exact = self.calculate_window_size(novelty, strength)
            
            # Track new node creation for visualization
            if is_new:
                new_nodes_created.append({
                    'label': concept['label'],
                    'patch_x': concept.get('x', 0),
                    'patch_y': concept.get('y', 0),
                    'cluster_size': concept.get('cluster_size', 0),
                    'novelty': novelty,
                    'window': N_exact
                })
            
            # Create EXACT edges to previous frames (adaptive window!)
            frames_to_connect = min(N_exact, len(self.temporal_buffer))
            
            for i in range(frames_to_connect):
                frame_distance = i + 1
                prev_frame_nodes = self.temporal_buffer[-(i+1)]
                
                # Calculate temporal decay weight
                weight = self.calculate_temporal_weight(frame_distance)
                
                if weight > 0:
                    for prev_node_id in prev_frame_nodes:
                        if prev_node_id != node_id:
                            self.add_edge(prev_node_id, node_id, self.EXACT, weight)
            
            # Track for diversity
            self.color_history.append(concept['color'])
            self.shape_history.append(concept['shape'])
            if len(self.color_history) > 50:
                self.color_history.pop(0)
            if len(self.shape_history) > 50:
                self.shape_history.pop(0)
        
        # Add current frame to temporal buffer
        self.temporal_buffer.append(current_node_ids)
        
        # LEAP edges: beyond EXACT window (for long-range inference)
        # Create LEAPs to frames outside the adaptive window
        if len(self.temporal_buffer) > self.N_max:
            far_past_frames = list(self.temporal_buffer)[:-self.N_max]
            for old_frame_nodes in far_past_frames:
                for old_node_id in old_frame_nodes:
                    for curr_node_id in current_node_ids:
                        if old_node_id != curr_node_id:
                            self.add_edge(old_node_id, curr_node_id, self.LEAP, 0.3)
        
        self.frame_count += 1
        
        return scores, concepts, new_nodes_created
    
    def draw_visualization(self, img, scores, new_nodes=None):
        """Draw attention visualization (from old system)"""
        overlay = img.copy()
        
        # Draw attention heatmap
        heatmap = np.zeros_like(img)
        for s in scores:
            intensity = int(s['focus'] * 255)
            color = (intensity // 2, intensity, intensity)
            x = s['x'] * self.patch_size
            y = s['y'] * self.patch_size
            cv2.rectangle(heatmap, (x, y), 
                         (x + self.patch_size, y + self.patch_size),
                         color, -1)
        
        # Blend heatmap
        overlay = cv2.addWeighted(img, 0.6, heatmap, 0.4, 0)
        
        # Highlight patches that created nodes (BRIGHT GREEN BORDER)
        if new_nodes:
            for node_info in new_nodes:
                if 'patch_x' in node_info and 'patch_y' in node_info:
                    x = node_info['patch_x'] * self.patch_size
                    y = node_info['patch_y'] * self.patch_size
                    # Bright green border for node creation
                    cv2.rectangle(overlay, (x, y), 
                                 (x + self.patch_size, y + self.patch_size),
                                 (0, 255, 0), 4)
                    # Label the node
                    cv2.putText(overlay, "NODE!", (x + 5, y + 20),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        
        # Find and draw focus
        if scores:
            best = max(scores, key=lambda s: s['focus'])
            cx, cy = best['cx'], best['cy']
            
            # Crosshair
            cv2.drawMarker(overlay, (cx, cy), (0, 255, 255),
                          cv2.MARKER_CROSS, 60, 3)
            cv2.circle(overlay, (cx, cy), 50, (0, 255, 255), 3)
            
            # Label
            label = f"FOCUS F={best['focus']:.2f}"
            cv2.putText(overlay, label, (cx - 80, cy - 60),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
            
            # Score breakdown
            y_offset = cy + 70
            cv2.putText(overlay, f"S:{best['saliency']:.2f}", (cx - 80, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            cv2.putText(overlay, f"G:{best['goal']:.2f}", (cx - 20, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)
            cv2.putText(overlay, f"C:{best['curiosity']:.2f}", (cx + 40, y_offset),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)
            
            # Track focus
            self.focus_history.append((cx, cy))
            if len(self.focus_history) > 30:
                self.focus_history.pop(0)
            
            # Draw focus trail
            for i in range(1, len(self.focus_history)):
                alpha = i / len(self.focus_history)
                thickness = max(1, int(3 * alpha))
                cv2.line(overlay, self.focus_history[i-1], self.focus_history[i],
                        (0, int(200*alpha), int(200*alpha)), thickness)
        
        # Draw grid
        for y in range(0, self.height, self.patch_size):
            cv2.line(overlay, (0, y), (self.width, y), (80, 80, 80), 1)
        for x in range(0, self.width, self.patch_size):
            cv2.line(overlay, (x, 0), (x, self.height), (80, 80, 80), 1)
        
        # Title & Info
        cv2.putText(overlay, "Melvin Vision Learning - Building Graph", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
        cv2.putText(overlay, "F = S + G + C + D", (10, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 200), 2)
        
        # Stats overlay (LARGER and MORE VISIBLE)
        cv2.putText(overlay, f"Frame: {self.frame_count}", (10, self.height - 120),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        cv2.putText(overlay, f"Nodes: {len(self.nodes)}", (10, self.height - 80),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        cv2.putText(overlay, f"Edges: {len(self.edges)}", (10, self.height - 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        
        # Show new node creation alert with adaptive window info
        if new_nodes:
            alert_y = 120
            for node_info in new_nodes:
                cluster_info = f" [cluster:{node_info.get('cluster_size', 0)}]"
                window_info = f" [window:{node_info.get('window', 0)}]"
                novelty_info = f" novelty:{node_info.get('novelty', 0):.2f}"
                cv2.putText(overlay, f"NEW: {node_info['label'][:30]}", (10, alert_y),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                cv2.putText(overlay, f"{cluster_info}{window_info} {novelty_info}", (10, alert_y + 25),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
                alert_y += 55
        
        return overlay
    
    def run(self):
        """Run vision learning loop"""
        print("\n╔═══════════════════════════════════════════════════════════╗")
        print("║  🧠 MELVIN VISION LEARNING - Saving to Brain             ║")
        print("╚═══════════════════════════════════════════════════════════╝\n")
        print("Camera → Attention → Graph Nodes & Edges")
        print("\nControls:")
        print("  'q' - Quit and save")
        print("  's' - Save graph now")
        print("  'p' - Print stats\n")
        
        save_interval = 60  # Save every 60 frames
        
        try:
            while True:
                ret, frame = self.cap.read()
                if not ret or frame is None:
                    print("❌ Failed to grab frame")
                    break
                
                # Process and update graph
                scores, concepts, new_nodes = self.process_frame(frame)
                
                # Draw beautiful visualization with node creation highlights
                display = self.draw_visualization(frame, scores, new_nodes)
                
                cv2.imshow("Melvin Vision Learning", display)
                
                # Auto-save periodically
                if self.frame_count % save_interval == 0:
                    self.save_graph()
                    print(f"💾 Auto-saved at frame {self.frame_count}")
                
                # Keys
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q'):
                    break
                elif key == ord('s'):
                    self.save_graph()
                    print(f"💾 Saved: {len(self.nodes)} nodes, {len(self.edges)} edges")
                elif key == ord('p'):
                    self.print_stats()
        
        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted")
        
        finally:
            print("\n💾 Saving final graph...")
            self.save_graph()
            self.cap.release()
            cv2.destroyAllWindows()
            self.print_stats()
    
    def print_stats(self):
        """Print graph statistics"""
        print("\n╔═══════════════════════════════════════════════════════════╗")
        print("║  📊 BRAIN GRAPH STATISTICS                               ║")
        print("╚═══════════════════════════════════════════════════════════╝\n")
        print(f"  Frames processed:  {self.frame_count}")
        print(f"  Visual nodes:      {len(self.nodes)}")
        print(f"  Total edges:       {len(self.edges)}")
        
        # Count edge types
        exact_count = sum(1 for e in self.edges if e[2] == self.EXACT)
        leap_count = sum(1 for e in self.edges if e[2] == self.LEAP)
        print(f"    EXACT (spatial): {exact_count}")
        print(f"    LEAP (temporal): {leap_count}")
        
        # Top concepts
        print(f"\n  Top visual concepts:")
        concept_counts = {}
        for label in self.nodes.keys():
            concept_counts[label] = concept_counts.get(label, 0) + 1
        
        for label in sorted(concept_counts.keys())[:10]:
            print(f"    - {label}")
        
        print(f"\n  Graph saved to: {self.graph_path}/")
        print()

def main():
    try:
        camera_id = 0 if len(sys.argv) == 1 else int(sys.argv[1])
        
        learner = VisionLearning(camera_id=camera_id)
        learner.run()
        
    except RuntimeError as e:
        print(f"\n❌ Error: {e}")
        return 1
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

