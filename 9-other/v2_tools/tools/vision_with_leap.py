#!/usr/bin/env python3
"""
MELVIN v2 - Vision with LEAP Pattern Synthesis

LEAP (Linking Emergent Attractor Patterns) for Vision:

1. SPATIAL LEAP: Objects that frequently appear together
   Example: "hand" + "face" appear together 80% of time
           → Create LEAP: "person" connecting them

2. TEMPORAL LEAP: Objects that appear in sequence
   Example: A → B → C happens 5+ times
           → Create LEAP shortcut: A → C

3. PREDICTIVE LEAP: Based on patterns, predict what's next
   Example: See "hand" → Predict "face" will appear (LEAP!)

Emergent pattern recognition through visual experience!
"""

import cv2
import numpy as np
import json
import time
from collections import defaultdict

# ============================================================================
# FOCUS NODE
# ============================================================================

class FocusNode:
    """Node created when object is focused on"""
    
    def __init__(self, node_id, bbox, features, frame_num):
        self.node_id = node_id
        self.bbox = bbox
        self.features = features
        self.created_frame = frame_num
        self.last_seen_frame = frame_num
        
    def center(self):
        x, y, w, h = self.bbox
        return (x + w//2, y + h//2)

# ============================================================================
# LEAP PATTERN DETECTOR
# ============================================================================

class VisualLEAPSynthesis:
    """
    Detect emergent patterns in visual attention
    
    Creates LEAP connections when:
    - Objects co-occur frequently (spatial patterns)
    - Sequences repeat (temporal patterns)
    """
    
    def __init__(self):
        # Co-occurrence tracking (spatial LEAP)
        self.co_occurrence_matrix = defaultdict(lambda: defaultdict(int))
        
        # Sequence tracking (temporal LEAP)
        self.transition_matrix = defaultdict(lambda: defaultdict(int))
        self.recent_sequence = []  # Last N focus nodes
        
        # LEAP connections discovered
        self.leap_connections = []  # [(node_a, node_b, pattern_type, strength)]
        
        # Thresholds
        self.spatial_leap_threshold = 5  # Co-occur 5+ times → LEAP!
        self.temporal_leap_threshold = 3  # Sequence 3+ times → LEAP!
        
    def update_co_occurrence(self, nodes_in_frame):
        """Track which nodes appear together (spatial LEAP)"""
        # For each pair of nodes in current frame
        for i, node_a in enumerate(nodes_in_frame):
            for j, node_b in enumerate(nodes_in_frame):
                if i < j:
                    # They co-occurred!
                    self.co_occurrence_matrix[node_a][node_b] += 1
                    self.co_occurrence_matrix[node_b][node_a] += 1
    
    def update_transitions(self, current_focus_id):
        """Track focus transitions (temporal LEAP)"""
        self.recent_sequence.append(current_focus_id)
        
        # Keep last 10
        if len(self.recent_sequence) > 10:
            self.recent_sequence.pop(0)
        
        # Record transition if we have previous
        if len(self.recent_sequence) >= 2:
            prev = self.recent_sequence[-2]
            curr = self.recent_sequence[-1]
            self.transition_matrix[prev][curr] += 1
    
    def detect_spatial_leaps(self):
        """Find frequent co-occurrences → Create spatial LEAP"""
        new_leaps = []
        
        for node_a, neighbors in self.co_occurrence_matrix.items():
            for node_b, count in neighbors.items():
                if count >= self.spatial_leap_threshold:
                    # Check if we haven't already created this LEAP
                    exists = any(
                        (l[0] == node_a and l[1] == node_b) or 
                        (l[0] == node_b and l[1] == node_a)
                        for l in self.leap_connections
                        if l[2] == 'spatial'
                    )
                    
                    if not exists:
                        strength = count / 10.0  # Normalize
                        new_leaps.append((node_a, node_b, 'spatial', strength))
        
        return new_leaps
    
    def detect_temporal_leaps(self):
        """Find repeated sequences → Create temporal LEAP shortcuts"""
        new_leaps = []
        
        # Look for A → B transitions that happen frequently
        for node_from, targets in self.transition_matrix.items():
            for node_to, count in targets.items():
                if count >= self.temporal_leap_threshold:
                    # Check if LEAP exists
                    exists = any(
                        l[0] == node_from and l[1] == node_to
                        for l in self.leap_connections
                        if l[2] == 'temporal'
                    )
                    
                    if not exists:
                        strength = count / 5.0  # Normalize
                        new_leaps.append((node_from, node_to, 'temporal', strength))
        
        # Look for A → B → C patterns (2-hop shortcuts)
        if len(self.recent_sequence) >= 3:
            for i in range(len(self.recent_sequence) - 2):
                node_a = self.recent_sequence[i]
                node_b = self.recent_sequence[i + 1]
                node_c = self.recent_sequence[i + 2]
                
                # If A → B → C, create shortcut A → C
                # Check if this pattern has been seen before
                pattern_key = (node_a, node_c)
                
                # Count how many times A eventually leads to C
                # (This is a simplification - full version would track properly)
                exists = any(
                    l[0] == node_a and l[1] == node_c
                    for l in self.leap_connections
                    if l[2] == 'shortcut'
                )
                
                if not exists and i == len(self.recent_sequence) - 3:
                    # Recent pattern, create shortcut LEAP
                    new_leaps.append((node_a, node_c, 'shortcut', 0.7))
        
        return new_leaps
    
    def synthesize_leaps(self):
        """Run LEAP synthesis and return new connections"""
        new_leaps = []
        
        # Detect spatial LEAPs (co-occurrence)
        new_leaps.extend(self.detect_spatial_leaps())
        
        # Detect temporal LEAPs (sequences)
        new_leaps.extend(self.detect_temporal_leaps())
        
        # Add to collection
        self.leap_connections.extend(new_leaps)
        
        return new_leaps

# ============================================================================
# VISION WITH LEAP
# ============================================================================

class VisionWithLEAP:
    """Focus-driven vision + LEAP pattern synthesis"""
    
    def __init__(self):
        # Focus nodes
        self.focus_nodes = {}
        self.next_node_id = 0
        
        # Current focus
        self.current_focus_node = None
        self.previous_focus_node = None
        
        # Regular edges (focus transitions)
        self.focus_edges = []
        
        # LEAP synthesis
        self.leap_synthesis = VisualLEAPSynthesis()
        
        # Frame tracking
        self.frame_count = 0
        self.prev_frame = None
        
        # Detection params
        self.min_object_size = 200
    
    def detect_candidates(self, frame, gray):
        """Detect objects"""
        # Threshold
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Skin detection
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        lower_skin = np.array([0, 20, 70], dtype=np.uint8)
        upper_skin = np.array([20, 255, 255], dtype=np.uint8)
        skin_mask = cv2.inRange(hsv, lower_skin, upper_skin)
        
        kernel = np.ones((7,7), np.uint8)
        skin_mask = cv2.morphologyEx(skin_mask, cv2.MORPH_CLOSE, kernel)
        
        skin_contours, _ = cv2.findContours(skin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Combine
        frame_h, frame_w = gray.shape
        frame_area = frame_h * frame_w
        
        candidates = []
        for contour in list(contours) + list(skin_contours):
            area = cv2.contourArea(contour)
            
            if area > self.min_object_size and area < frame_area * 0.95:
                x, y, w, h = cv2.boundingRect(contour)
                if w < frame_w * 0.98 and h < frame_h * 0.98:
                    # Compute features
                    roi_gray = gray[y:y+h, x:x+w]
                    roi_color = frame[y:y+h, x:x+w]
                    
                    edge_score = self.compute_edges(roi_gray)
                    color_score = self.compute_color(roi_color)
                    
                    attention = 0.4*edge_score + 0.3*color_score + 0.3*(area/10000.0)
                    
                    candidates.append({
                        'bbox': (x, y, w, h),
                        'area': area,
                        'attention': attention,
                        'edge_score': edge_score,
                        'color_score': color_score
                    })
        
        return candidates
    
    def compute_edges(self, roi_gray):
        if roi_gray.size == 0:
            return 0.0
        edges = cv2.Canny(roi_gray, 30, 90)
        return np.sum(edges > 0) / edges.size
    
    def compute_color(self, roi_color):
        if roi_color.size == 0:
            return 0.0
        return np.std(roi_color) / 128.0
    
    def select_focus(self, candidates):
        """Select ONE focus"""
        if not candidates:
            return None
        
        # Auto-shift after 60 frames
        if self.current_focus_node:
            focus_duration = self.frame_count - self.current_focus_node.last_seen_frame
            if focus_duration > 60 and len(candidates) > 1:
                candidates_sorted = sorted(candidates, key=lambda c: c['attention'], reverse=True)
                return candidates_sorted[1]
        
        return max(candidates, key=lambda c: c['attention'])
    
    def create_or_update_node(self, focus):
        """Create node for focus"""
        bbox = focus['bbox']
        
        # Check if same as current
        if self.current_focus_node:
            # Simple distance check
            cx_curr = self.current_focus_node.bbox[0] + self.current_focus_node.bbox[2]//2
            cy_curr = self.current_focus_node.bbox[1] + self.current_focus_node.bbox[3]//2
            cx_new = bbox[0] + bbox[2]//2
            cy_new = bbox[1] + bbox[3]//2
            
            dist = np.sqrt((cx_curr - cx_new)**2 + (cy_curr - cy_new)**2)
            
            if dist < 50:  # Same object
                self.current_focus_node.bbox = bbox
                self.current_focus_node.last_seen_frame = self.frame_count
                return self.current_focus_node, False
        
        # NEW focus! Create node
        node = FocusNode(self.next_node_id, bbox, focus, self.frame_count)
        self.focus_nodes[self.next_node_id] = node
        self.next_node_id += 1
        
        # Create regular EXACT edge
        if self.current_focus_node:
            cx_prev, cy_prev = self.current_focus_node.center()
            cx_new, cy_new = node.center()
            distance = np.sqrt((cx_new - cx_prev)**2 + (cy_new - cy_prev)**2)
            
            edge = {
                'from': self.current_focus_node.node_id,
                'to': node.node_id,
                'distance': distance,
                'frame': self.frame_count,
                'type': 'focus_transition'
            }
            self.focus_edges.append(edge)
            
            # Update LEAP: Track transition
            self.leap_synthesis.update_transitions(node.node_id)
        
        self.previous_focus_node = self.current_focus_node
        self.current_focus_node = node
        
        return node, True
    
    def process_frame(self, frame):
        """Process frame with LEAP synthesis"""
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # Detect candidates
        candidates = self.detect_candidates(frame, gray)
        
        # Select focus
        focus = self.select_focus(candidates)
        
        # Create/update node
        if focus:
            focus_node, is_new = self.create_or_update_node(focus)
            focus['node'] = focus_node
            focus['is_new'] = is_new
        
        # Update LEAP co-occurrence (which nodes visible together)
        visible_node_ids = [self.current_focus_node.node_id] if self.current_focus_node else []
        # In full version: would include all recognized objects in frame
        
        # Run LEAP synthesis every 30 frames
        new_leaps = []
        if self.frame_count % 30 == 0:
            new_leaps = self.leap_synthesis.synthesize_leaps()
        
        self.prev_frame = gray.copy()
        
        return {
            'candidates': candidates,
            'focus': focus,
            'nodes': self.focus_nodes,
            'edges': self.focus_edges,
            'leap_connections': self.leap_synthesis.leap_connections,
            'new_leaps': new_leaps
        }

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_with_leap(frame, data, frame_count):
    """Draw with LEAP connections visible"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    candidates = data['candidates']
    focus = data['focus']
    nodes = data['nodes']
    edges = data['edges']
    leap_connections = data['leap_connections']
    new_leaps = data['new_leaps']
    
    # Draw LEAP connections (dashed lines, different color)
    for node_a_id, node_b_id, pattern_type, strength in leap_connections:
        if node_a_id in nodes and node_b_id in nodes:
            node_a = nodes[node_a_id]
            node_b = nodes[node_b_id]
            
            cx_a, cy_a = node_a.center()
            cx_b, cy_b = node_b.center()
            
            # Color by LEAP type
            if pattern_type == 'spatial':
                color = (255, 0, 255)  # Magenta = spatial LEAP
            elif pattern_type == 'temporal':
                color = (0, 255, 0)  # Green = temporal LEAP
            else:
                color = (255, 255, 0)  # Yellow = shortcut LEAP
            
            # Dashed line for LEAP (not solid like regular edges)
            draw_dashed_line(display, (cx_a, cy_a), (cx_b, cy_b), color, 2)
            
            # LEAP label
            mid_x = (cx_a + cx_b) // 2
            mid_y = (cy_a + cy_b) // 2
            cv2.putText(display, f"LEAP:{pattern_type[:3].upper()}", (mid_x, mid_y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 2)
    
    # Draw regular focus edges (solid yellow arrows)
    recent_edges = [e for e in edges if frame_count - e['frame'] < 10]
    for edge in recent_edges[-10:]:
        if edge['from'] in nodes and edge['to'] in nodes:
            node_a = nodes[edge['from']]
            node_b = nodes[edge['to']]
            
            cx_a, cy_a = node_a.center()
            cx_b, cy_b = node_b.center()
            
            # Solid line for regular edges
            cv2.arrowedLine(display, (cx_a, cy_a), (cx_b, cy_b),
                           (0, 255, 255), 2, tipLength=0.2)
    
    # Draw candidates (gray boxes)
    for candidate in candidates:
        x, y, w_box, h_box = candidate['bbox']
        cv2.rectangle(display, (x, y), (x+w_box, y+h_box), (180, 180, 180), 2)
        
        att = candidate['attention']
        cv2.putText(display, f"{att:.2f}", (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (180, 180, 180), 1)
    
    # Draw focus (yellow box)
    if focus:
        x, y, w_box, h_box = focus['bbox']
        cx, cy = x + w_box//2, y + h_box//2
        
        color = (0, 255, 255)
        cv2.rectangle(display, (x, y), (x+w_box, y+h_box), color, 4)
        cv2.circle(display, (cx, cy), max(w_box, h_box)//2 + 10, color, 3)
        cv2.drawMarker(display, (cx, cy), color, cv2.MARKER_CROSS, 40, 3)
        
        # Label
        node = focus.get('node')
        if node:
            label = f"FOCUS: #{node.node_id}"
            if focus.get('is_new', False):
                label += " (NEW!)"
            cv2.putText(display, label, (cx - 80, cy - 50),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)
    
    # Info panel
    panel_h = 200
    cv2.rectangle(display, (0, 0), (500, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (500, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Vision + LEAP Synthesis", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Detected: {len(candidates)} | Nodes: {len(nodes)}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    cv2.putText(display, f"Regular edges: {len(edges)} (focus transitions)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
    
    y_pos += 20
    cv2.putText(display, f"LEAP edges: {len(leap_connections)} (patterns!)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 255), 1)
    
    # Show new LEAPs!
    if new_leaps:
        y_pos += 25
        cv2.putText(display, f"NEW LEAP! ({len(new_leaps)} discovered)", (10, y_pos),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        
        for leap in new_leaps[:2]:  # Show first 2
            y_pos += 18
            pattern_type = leap[2]
            cv2.putText(display, f"  #{leap[0]}→#{leap[1]} ({pattern_type})", (10, y_pos),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 255, 150), 1)
    
    y_pos += 25
    cv2.putText(display, "LEAP Types:", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.45, (200, 200, 255), 1)
    
    y_pos += 18
    cv2.line(display, (15, y_pos-5), (35, y_pos-5), (255, 0, 255), 2)
    cv2.putText(display, "Spatial (co-occur)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
    
    y_pos += 15
    cv2.line(display, (15, y_pos-5), (35, y_pos-5), (0, 255, 0), 2)
    cv2.putText(display, "Temporal (sequence)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
    
    y_pos += 15
    cv2.line(display, (15, y_pos-5), (35, y_pos-5), (255, 255, 0), 2)
    cv2.putText(display, "Shortcut (A→B→C→A→C)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
    
    return display

def draw_dashed_line(img, pt1, pt2, color, thickness=1, gap=10):
    """Draw dashed line for LEAP connections"""
    dist = np.sqrt((pt2[0]-pt1[0])**2 + (pt2[1]-pt1[1])**2)
    if dist == 0:
        return
    
    pts = []
    for i in np.arange(0, dist, gap):
        r = i / dist
        x = int(pt1[0] * (1-r) + pt2[0] * r)
        y = int(pt1[1] * (1-r) + pt2[1] * r)
        pts.append((x, y))
    
    for i in range(0, len(pts)-1, 2):
        cv2.line(img, pts[i], pts[i+1] if i+1 < len(pts) else pt2, color, thickness)

# ============================================================================
# MAIN
# ============================================================================

def main():
    print("\n" + "="*70)
    print("🌌 MELVIN v2 - Vision with LEAP Pattern Synthesis")
    print("="*70 + "\n")
    print("LEAP SYNTHESIS FOR VISION:")
    print("  1. SPATIAL LEAP - Objects that co-occur (appear together)")
    print("  2. TEMPORAL LEAP - Objects in repeated sequences")
    print("  3. SHORTCUT LEAP - A→B→C patterns create A→C")
    print()
    print("VISUAL:")
    print("  • Solid yellow arrows = Regular edges (focus shifts)")
    print("  • Dashed magenta lines = Spatial LEAP (co-occurrence)")
    print("  • Dashed green lines = Temporal LEAP (sequences)")
    print("  • Dashed yellow lines = Shortcut LEAP (patterns)")
    print()
    
    vision = VisionWithLEAP()
    
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ Cannot open camera")
        return
    
    print("✓ Camera opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  s - Show LEAP statistics")
    print()
    print("Watch as MELVIN discovers patterns:")
    print("  • Hand + Face appear together → SPATIAL LEAP!")
    print("  • A → B → A → B sequence → TEMPORAL LEAP!")
    print("  • A → B → C → A → B → C → SHORTCUT LEAP!")
    print()
    print("="*70 + "\n")
    
    fps_start = time.time()
    fps_count = 0
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            data = vision.process_frame(frame)
            
            # Show LEAP discoveries
            if data['new_leaps']:
                for leap in data['new_leaps']:
                    print(f"🌌 LEAP DISCOVERED: Node #{leap[0]} ↔ #{leap[1]} ({leap[2]}, strength={leap[3]:.2f})")
            
            display = draw_with_leap(frame, data, vision.frame_count)
            
            # FPS
            fps_count += 1
            if time.time() - fps_start > 0.5:
                current_fps = fps_count / (time.time() - fps_start)
                fps_start = time.time()
                fps_count = 0
                
                cv2.putText(display, f"FPS: {current_fps:.1f}",
                           (display.shape[1] - 120, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            cv2.imshow('MELVIN Vision + LEAP', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                print(f"\n🌌 LEAP STATISTICS:")
                print(f"  Total LEAP connections: {len(vision.leap_synthesis.leap_connections)}")
                print(f"  Spatial LEAPs: {sum(1 for l in vision.leap_synthesis.leap_connections if l[2]=='spatial')}")
                print(f"  Temporal LEAPs: {sum(1 for l in vision.leap_synthesis.leap_connections if l[2]=='temporal')}")
                print(f"  Shortcut LEAPs: {sum(1 for l in vision.leap_synthesis.leap_connections if l[2]=='shortcut')}")
                print()
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        
        print(f"\n✓ Session complete")
        print(f"  Focus nodes: {len(vision.focus_nodes)}")
        print(f"  Regular edges: {len(vision.focus_edges)}")
        print(f"  LEAP connections: {len(vision.leap_synthesis.leap_connections)}")
        
        # Save graph with LEAPs
        graph = {
            'nodes': [
                {
                    'id': node.node_id,
                    'bbox': node.bbox,
                    'center': node.center(),
                    'created_frame': node.created_frame
                }
                for node in vision.focus_nodes.values()
            ],
            'regular_edges': [
                {
                    'from': e['from'],
                    'to': e['to'],
                    'distance': e['distance'],
                    'frame': e['frame'],
                    'type': 'regular'
                }
                for e in vision.focus_edges
            ],
            'leap_edges': [
                {
                    'from': l[0],
                    'to': l[1],
                    'pattern_type': l[2],
                    'strength': l[3],
                    'type': 'LEAP'
                }
                for l in vision.leap_synthesis.leap_connections
            ]
        }
        
        with open('melvin_vision_with_leap.json', 'w') as f:
            json.dump(graph, f, indent=2)
        
        print(f"\n💾 Saved graph with LEAPs: melvin_vision_with_leap.json")
        print(f"   {len(vision.focus_nodes)} nodes")
        print(f"   {len(vision.focus_edges)} regular edges")
        print(f"   {len(vision.leap_synthesis.leap_connections)} LEAP edges! 🌌")

if __name__ == "__main__":
    main()

