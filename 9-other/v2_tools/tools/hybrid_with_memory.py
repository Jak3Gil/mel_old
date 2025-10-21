#!/usr/bin/env python3
"""
MELVIN v2 - Hybrid Vision with Object Memory & Graph Connections

Combines:
- Working detection (from hybrid_camera_vision.py)
- Persistent nodes (memory for each object)
- EXACT edges (connections between nearby objects)
- Recognition (same object = same node)
"""

import cv2
import numpy as np
import json
import time
from pathlib import Path

# ============================================================================
# OBJECT NODE (Persistent Memory)
# ============================================================================

class ObjectNode:
    """Persistent memory node for a detected object"""
    
    def __init__(self, node_id, bbox, features, frame_num):
        self.node_id = node_id
        self.bbox = bbox  # (x, y, w, h)
        self.features = features
        self.first_seen = frame_num
        self.last_seen = frame_num
        self.times_seen = 1
        
    def update(self, bbox, features, frame_num):
        """Update existing node"""
        self.bbox = bbox
        self.features = features
        self.last_seen = frame_num
        self.times_seen += 1
    
    def center(self):
        """Get center point"""
        x, y, w, h = self.bbox
        return (x + w//2, y + h//2)

# ============================================================================
# VISION WITH MEMORY
# ============================================================================

class HybridVisionWithMemory:
    """Working detection + persistent nodes + spatial edges"""
    
    def __init__(self):
        # Object memory
        self.object_nodes = {}  # node_id -> ObjectNode
        self.next_node_id = 0
        self.node_edges = []  # List of (node_a, node_b, distance)
        
        # Detection params (from working hybrid)
        self.min_object_size = 200  # From working version
        self.iou_threshold = 0.4  # For matching objects across frames
        
        # Frame tracking
        self.frame_count = 0
        self.prev_frame = None
        
    def compute_iou(self, box1, box2):
        """Intersection over Union"""
        x1, y1, w1, h1 = box1
        x2, y2, w2, h2 = box2
        
        x1_max, y1_max = x1 + w1, y1 + h1
        x2_max, y2_max = x2 + w2, y2 + h2
        
        # Intersection
        inter_xmin = max(x1, x2)
        inter_ymin = max(y1, y2)
        inter_xmax = min(x1_max, x2_max)
        inter_ymax = min(y1_max, y2_max)
        
        if inter_xmax < inter_xmin or inter_ymax < inter_ymin:
            return 0.0
        
        inter_area = (inter_xmax - inter_xmin) * (inter_ymax - inter_ymin)
        union_area = w1*h1 + w2*h2 - inter_area
        
        return inter_area / union_area if union_area > 0 else 0.0
    
    def find_matching_node(self, bbox):
        """Find if this detection matches an existing node"""
        best_match = None
        best_iou = 0.0
        
        for node_id, node in self.object_nodes.items():
            # Only check recently seen nodes
            if self.frame_count - node.last_seen > 30:
                continue
            
            iou = self.compute_iou(node.bbox, bbox)
            if iou > self.iou_threshold and iou > best_iou:
                best_iou = iou
                best_match = node
        
        return best_match
    
    def detect_objects(self, frame, gray):
        """Detect objects using working hybrid method"""
        # Simple threshold (from working version)
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        
        # Find contours
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        frame_h, frame_w = gray.shape
        frame_area = frame_h * frame_w
        
        detections = []
        for contour in contours:
            area = cv2.contourArea(contour)
            
            # Filter (from working version)
            if area > self.min_object_size and area < frame_area * 0.95:
                x, y, w, h = cv2.boundingRect(contour)
                
                if w >= frame_w * 0.98 and h >= frame_h * 0.98:
                    continue
                
                # Compute features
                roi_gray = gray[y:y+h, x:x+w]
                roi_color = frame[y:y+h, x:x+w]
                
                features = {
                    'edge': self.compute_edges(roi_gray),
                    'color': self.compute_color(roi_color),
                    'size': area
                }
                
                detections.append({
                    'bbox': (x, y, w, h),
                    'area': area,
                    'features': features
                })
        
        return detections
    
    def compute_edges(self, roi_gray):
        """Edge density"""
        if roi_gray.size == 0:
            return 0.0
        edges = cv2.Canny(roi_gray, 30, 90)
        return np.sum(edges > 0) / edges.size
    
    def compute_color(self, roi_color):
        """Color variance"""
        if roi_color.size == 0:
            return 0.0
        return np.std(roi_color) / 128.0
    
    def compute_distance(self, node_a, node_b):
        """Euclidean distance between node centers"""
        cx_a, cy_a = node_a.center()
        cx_b, cy_b = node_b.center()
        return np.sqrt((cx_a - cx_b)**2 + (cy_a - cy_b)**2)
    
    def update_nodes_and_edges(self, detections):
        """
        Update persistent nodes and create EXACT edges
        
        For each detection:
        1. Match with existing node OR create new
        2. Create EXACT edge to closest other node
        """
        current_frame_nodes = []
        
        # Update/create nodes
        for detection in detections:
            bbox = detection['bbox']
            features = detection['features']
            
            # Try to match existing node
            matched_node = self.find_matching_node(bbox)
            
            if matched_node:
                # UPDATE existing node
                matched_node.update(bbox, features, self.frame_count)
                detection['node'] = matched_node
                current_frame_nodes.append(matched_node)
            else:
                # CREATE new node
                node = ObjectNode(self.next_node_id, bbox, features, self.frame_count)
                self.object_nodes[self.next_node_id] = node
                self.next_node_id += 1
                detection['node'] = node
                current_frame_nodes.append(node)
        
        # Create EXACT edges between current frame nodes
        # For each node, connect to closest other node
        for i, node_a in enumerate(current_frame_nodes):
            if len(current_frame_nodes) <= 1:
                break
            
            # Find closest node
            closest_node = None
            closest_dist = float('inf')
            
            for j, node_b in enumerate(current_frame_nodes):
                if i != j:
                    dist = self.compute_distance(node_a, node_b)
                    if dist < closest_dist:
                        closest_dist = dist
                        closest_node = node_b
            
            # Create EXACT edge to closest
            if closest_node:
                edge = (node_a.node_id, closest_node.node_id, closest_dist, self.frame_count)
                self.node_edges.append(edge)
        
        # Cleanup old nodes
        to_remove = []
        for node_id, node in self.object_nodes.items():
            if self.frame_count - node.last_seen > 50:
                to_remove.append(node_id)
        
        for node_id in to_remove:
            del self.object_nodes[node_id]
        
        return detections
    
    def process_frame(self, frame):
        """Main processing"""
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # Detect objects
        detections = self.detect_objects(frame, gray)
        
        # Update nodes and create edges
        detections = self.update_nodes_and_edges(detections)
        
        self.prev_frame = gray.copy()
        
        return {
            'detections': detections,
            'nodes': self.object_nodes,
            'edges': self.node_edges
        }

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_with_memory(frame, data, frame_count):
    """Draw objects, nodes, and edges"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    detections = data['detections']
    nodes = data['nodes']
    edges = data['edges']
    
    # Draw edges between current frame objects (last 10 frames)
    recent_edges = [e for e in edges if frame_count - e[3] < 10]
    for node_a_id, node_b_id, dist, edge_frame in recent_edges[-20:]:  # Last 20 edges
        if node_a_id in nodes and node_b_id in nodes:
            node_a = nodes[node_a_id]
            node_b = nodes[node_b_id]
            
            cx_a, cy_a = node_a.center()
            cx_b, cy_b = node_b.center()
            
            # Draw line (EXACT edge)
            age = frame_count - edge_frame
            alpha = max(0, 1.0 - age/10.0)
            color_intensity = int(255 * alpha)
            
            cv2.line(display, (cx_a, cy_a), (cx_b, cy_b),
                    (color_intensity, color_intensity, 255), 2)
            
            # Distance label
            mid_x, mid_y = (cx_a + cx_b)//2, (cy_a + cy_b)//2
            cv2.putText(display, f"{int(dist)}px", (mid_x, mid_y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 255), 1)
    
    # Draw all objects with node info
    for detection in detections:
        node = detection['node']
        x, y, w, h = node.bbox
        
        # Color based on if node is new or recognized
        if node.times_seen == 1:
            color = (0, 255, 0)  # Green = NEW node
            label = f"#{node.node_id} NEW"
        else:
            color = (255, 255, 255)  # White = RECOGNIZED
            label = f"#{node.node_id} (seen {node.times_seen}x)"
        
        # Draw box
        cv2.rectangle(display, (x, y), (x+w, y+h), color, 2)
        
        # Draw node ID
        cv2.putText(display, label, (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
        
        # Draw center point
        cx, cy = node.center()
        cv2.circle(display, (cx, cy), 5, color, -1)
    
    # Info panel
    panel_h = 160
    cv2.rectangle(display, (0, 0), (550, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (550, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Vision with Memory & Spatial Graph", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Frame: {frame_count}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    cv2.putText(display, f"Current objects: {len(detections)}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 255, 100), 1)
    
    y_pos += 20
    cv2.putText(display, f"Total nodes: {len(nodes)} (in memory)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    new_count = sum(1 for n in nodes.values() if n.times_seen == 1)
    cv2.putText(display, f"New nodes: {new_count} | Recognized: {len(nodes) - new_count}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    
    y_pos += 20
    cv2.putText(display, f"EXACT edges: {len(recent_edges)} (recent)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (150, 150, 255), 1)
    
    y_pos += 25
    cv2.putText(display, "Lines = spatial connections (EXACT edges)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)
    
    # Legend
    legend_y = h - 90
    cv2.rectangle(display, (w-250, legend_y), (w, h), (0, 0, 0), -1)
    cv2.rectangle(display, (w-250, legend_y), (w, h), (255, 255, 255), 1)
    
    y_pos = legend_y + 20
    cv2.putText(display, "LEGEND:", (w-240, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-240, y_pos-10), (w-220, y_pos), (0, 255, 0), 2)
    cv2.putText(display, "New node", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-240, y_pos-10), (w-220, y_pos), (255, 255, 255), 2)
    cv2.putText(display, "Recognized node", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.line(display, (w-240, y_pos-5), (w-220, y_pos-5), (200, 200, 255), 2)
    cv2.putText(display, "EXACT edge", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN
# ============================================================================

def main():
    print("\n" + "="*70)
    print("🧠 MELVIN v2 - Vision with Memory & Spatial Graph")
    print("="*70 + "\n")
    print("Features:")
    print("  • Working detection (from hybrid)")
    print("  • Persistent nodes (recognizes objects)")
    print("  • EXACT edges (spatial connections)")
    print("  • Same object = same node ID!")
    print()
    
    vision = HybridVisionWithMemory()
    
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ Cannot open camera")
        return
    
    print("✓ Camera opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  s - Show statistics")
    print()
    print("Watch as MELVIN:")
    print("  • Detects objects and creates nodes")
    print("  • Recognizes same objects (same node ID)")
    print("  • Draws EXACT edges to closest objects")
    print("  • Builds spatial graph of your scene!")
    print()
    print("="*70 + "\n")
    
    fps_start = time.time()
    fps_count = 0
    current_fps = 0.0
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Process
            data = vision.process_frame(frame)
            
            # Draw
            display = draw_with_memory(frame, data, vision.frame_count)
            
            # FPS
            fps_count += 1
            if time.time() - fps_start > 0.5:
                current_fps = fps_count / (time.time() - fps_start)
                fps_start = time.time()
                fps_count = 0
            
            cv2.putText(display, f"FPS: {current_fps:.1f}",
                       (display.shape[1] - 120, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            cv2.imshow('MELVIN Vision + Memory', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                print(f"\n📊 STATISTICS:")
                print(f"  Frames: {vision.frame_count}")
                print(f"  Total nodes created: {vision.next_node_id}")
                print(f"  Nodes in memory: {len(vision.object_nodes)}")
                print(f"  Total edges: {len(vision.node_edges)}")
                print(f"  Nodes:")
                for nid, node in vision.object_nodes.items():
                    print(f"    #{nid}: seen {node.times_seen}x, center {node.center()}")
                print()
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        
        print(f"\n✓ Session complete")
        print(f"  Frames: {vision.frame_count}")
        print(f"  Nodes created: {vision.next_node_id}")
        print(f"  Nodes remembered: {len(vision.object_nodes)}")
        print(f"  Edges created: {len(vision.node_edges)}")
        
        # Save graph
        graph_data = {
            'nodes': [
                {
                    'id': node.node_id,
                    'bbox': node.bbox,
                    'center': node.center(),
                    'times_seen': node.times_seen,
                    'first_seen': node.first_seen,
                    'last_seen': node.last_seen
                }
                for node in vision.object_nodes.values()
            ],
            'edges': [
                {
                    'from': e[0],
                    'to': e[1],
                    'distance': e[2],
                    'frame': e[3]
                }
                for e in vision.node_edges
            ]
        }
        
        with open('melvin_spatial_graph.json', 'w') as f:
            json.dump(graph_data, f, indent=2)
        
        print(f"\n💾 Saved spatial graph to: melvin_spatial_graph.json")

if __name__ == "__main__":
    main()

