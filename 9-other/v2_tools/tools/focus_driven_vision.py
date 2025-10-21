#!/usr/bin/env python3
"""
MELVIN v2 - Focus-Driven Vision

KEY CONCEPT:
- Detect many objects (candidates)
- FOCUS on ONE at a time (highest attention)
- Only create NODE for focused object
- Create EXACT edge from previous focus to new focus

Clean, efficient, memory only for what matters!
"""

import cv2
import numpy as np
import json
import time
from pathlib import Path

# ============================================================================
# FOCUS NODE (Only Focused Objects Become Nodes!)
# ============================================================================

class FocusNode:
    """Node created when object is focused on"""
    
    def __init__(self, node_id, bbox, features, frame_num):
        self.node_id = node_id
        self.bbox = bbox
        self.features = features
        self.created_frame = frame_num
        self.last_focused_frame = frame_num
        self.total_focus_time = 1
        
    def center(self):
        x, y, w, h = self.bbox
        return (x + w//2, y + h//2)

# ============================================================================
# FOCUS-DRIVEN VISION
# ============================================================================

class FocusDrivenVision:
    """
    Only create nodes for focused objects
    
    Flow:
    1. Detect all candidates
    2. Pick ONE focus (highest attention)
    3. Create node ONLY for focused object
    4. Create EXACT edge from previous focus to new focus
    
    PERSISTENT MEMORY:
    - Loads previous session on startup
    - Recognizes old nodes
    - Continues building same graph!
    """
    
    def __init__(self, graph_file='melvin_focus_graph.json'):
        self.graph_file = graph_file
        
        # Nodes (only focused objects!)
        self.focus_nodes = {}  # node_id -> FocusNode
        self.next_node_id = 0
        
        # Focus tracking
        self.current_focus_node = None
        self.previous_focus_node = None
        
        # Edges (connections between consecutive focuses)
        self.focus_edges = []  # (from_node, to_node, distance, frame)
        
        # Frame tracking
        self.frame_count = 0
        self.prev_frame = None
        self.session_start_frame = 0
        
        # Detection params (from working hybrid)
        self.min_object_size = 200
        
        # LOAD PREVIOUS SESSION
        self.load_previous_graph()
    
    def load_previous_graph(self):
        """Load nodes and edges from previous session"""
        if not Path(self.graph_file).exists():
            print("  ℹ️  No previous session found (starting fresh)")
            return
        
        try:
            with open(self.graph_file, 'r') as f:
                data = json.load(f)
            
            # Load nodes
            for node_data in data.get('nodes', []):
                node = FocusNode(
                    node_data['id'],
                    tuple(node_data['bbox']),
                    {},  # Features not saved
                    node_data['created_frame']
                )
                node.last_seen_frame = node_data.get('created_frame', 0)
                self.focus_nodes[node.node_id] = node
            
            # Load edges
            for edge_data in data.get('edges', []):
                self.focus_edges.append({
                    'from': edge_data['from'],
                    'to': edge_data['to'],
                    'distance': edge_data['distance'],
                    'frame': edge_data['frame']
                })
            
            # Set next node ID
            if self.focus_nodes:
                self.next_node_id = max(self.focus_nodes.keys()) + 1
                self.session_start_frame = max(node.created_frame for node in self.focus_nodes.values())
            
            print(f"  ✅ Loaded previous session:")
            print(f"     • {len(self.focus_nodes)} nodes remembered")
            print(f"     • {len(self.focus_edges)} edges remembered")
            print(f"     • Next node ID: #{self.next_node_id}")
            print(f"     • Will recognize objects from before!")
            
        except Exception as e:
            print(f"  ⚠️  Error loading previous session: {e}")
            print(f"     Starting fresh")
    
    def find_matching_historical_node(self, bbox):
        """Check if this bbox matches a node from previous session"""
        best_match = None
        best_score = 0.0
        
        cx_new = bbox[0] + bbox[2]//2
        cy_new = bbox[1] + bbox[3]//2
        
        for node_id, node in self.focus_nodes.items():
            # Get node center
            cx_old, cy_old = node.center()
            
            # Distance-based matching (within 100px = same object)
            dist = np.sqrt((cx_new - cx_old)**2 + (cy_new - cy_old)**2)
            
            # Size similarity
            size_new = bbox[2] * bbox[3]
            size_old = node.bbox[2] * node.bbox[3]
            size_ratio = min(size_new, size_old) / max(size_new, size_old) if max(size_new, size_old) > 0 else 0
            
            # Combined score
            if dist < 100 and size_ratio > 0.5:
                score = (1.0 - dist/100.0) * size_ratio
                if score > best_score:
                    best_score = score
                    best_match = node
        
        return best_match if best_score > 0.4 else None
    
    def detect_candidates(self, frame, gray):
        """Detect all candidate objects"""
        # Use working threshold method
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Also detect skin regions (for hands/face)
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        lower_skin = np.array([0, 20, 70], dtype=np.uint8)
        upper_skin = np.array([20, 255, 255], dtype=np.uint8)
        skin_mask = cv2.inRange(hsv, lower_skin, upper_skin)
        
        # Clean up skin mask
        kernel = np.ones((7,7), np.uint8)
        skin_mask = cv2.morphologyEx(skin_mask, cv2.MORPH_OPEN, kernel)
        skin_mask = cv2.morphologyEx(skin_mask, cv2.MORPH_CLOSE, kernel)
        
        skin_contours, _ = cv2.findContours(skin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Combine both methods
        frame_h, frame_w = gray.shape
        frame_area = frame_h * frame_w
        
        candidates = []
        
        # Process threshold contours
        for contour in contours:
            area = cv2.contourArea(contour)
            if area > self.min_object_size and area < frame_area * 0.95:
                x, y, w, h = cv2.boundingRect(contour)
                if w < frame_w * 0.98 and h < frame_h * 0.98:
                    candidates.append({
                        'bbox': (x, y, w, h),
                        'area': area,
                        'method': 'object'
                    })
        
        # Process skin contours  
        for contour in skin_contours:
            area = cv2.contourArea(contour)
            if area > self.min_object_size and area < frame_area * 0.95:
                x, y, w, h = cv2.boundingRect(contour)
                if w < frame_w * 0.98 and h < frame_h * 0.98:
                    candidates.append({
                        'bbox': (x, y, w, h),
                        'area': area,
                        'method': 'skin'
                    })
        
        # Compute attention scores
        for candidate in candidates:
            x, y, w, h = candidate['bbox']
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = frame[y:y+h, x:x+w]
            
            # Features
            edge_score = self.compute_edges(roi_gray)
            color_score = self.compute_color(roi_color)
            size_score = min(1.0, area / 10000.0)
            
            # Bonus for skin (hands/face more interesting!)
            skin_bonus = 0.3 if candidate['method'] == 'skin' else 0.0
            
            # Attention
            attention = (
                0.4 * edge_score +
                0.3 * color_score +
                0.3 * size_score +
                skin_bonus
            )
            
            candidate['attention'] = attention
            candidate['edge_score'] = edge_score
            candidate['color_score'] = color_score
        
        return candidates
    
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
    
    def select_focus(self, candidates):
        """Pick ONE focus from candidates with dynamic shifting"""
        if not candidates:
            return None
        
        # Check if current focus still exists and hasn't been focused too long
        if self.current_focus_node:
            focus_duration = self.frame_count - self.current_focus_node.last_focused_frame
            
            # Add habituation: the longer focused, the less interesting
            # After 60 frames, force attention to shift
            if focus_duration > 60:
                # Been focused too long, must shift!
                # Pick second-best to force change
                candidates_sorted = sorted(candidates, key=lambda c: c['attention'], reverse=True)
                if len(candidates_sorted) > 1:
                    return candidates_sorted[1]  # Pick second best
        
        # Select highest attention
        focus = max(candidates, key=lambda c: c['attention'])
        return focus
    
    def create_or_update_node(self, focus):
        """Create node for focused object (or update/recognize if seen before)"""
        bbox = focus['bbox']
        
        # FIRST: Check if this matches current focus (same object continuing)
        if self.current_focus_node:
            # Compute IoU with current focus
            x1, y1, w1, h1 = self.current_focus_node.bbox
            x2, y2, w2, h2 = bbox
            
            x1_max, y1_max = x1 + w1, y1 + h1
            x2_max, y2_max = x2 + w2, y2 + h2
            
            inter_xmin = max(x1, x2)
            inter_ymin = max(y1, y2)
            inter_xmax = min(x1_max, x2_max)
            inter_ymax = min(y1_max, y2_max)
            
            if inter_xmax >= inter_xmin and inter_ymax >= inter_ymin:
                inter_area = (inter_xmax - inter_xmin) * (inter_ymax - inter_ymin)
                union_area = w1*h1 + w2*h2 - inter_area
                iou = inter_area / union_area if union_area > 0 else 0.0
                
                if iou > 0.3:  # Same object, just update
                    self.current_focus_node.bbox = bbox
                    self.current_focus_node.last_focused_frame = self.frame_count
                    self.current_focus_node.total_focus_time += 1
                    return self.current_focus_node, False  # Not new
        
        # SECOND: Check if this matches a node from PREVIOUS SESSIONS!
        historical_match = self.find_matching_historical_node(bbox)
        if historical_match:
            # RECOGNIZED from previous session!
            historical_match.bbox = bbox
            historical_match.last_focused_frame = self.frame_count
            
            # Create edge from current to recognized historical node
            if self.current_focus_node and self.current_focus_node.node_id != historical_match.node_id:
                cx_prev, cy_prev = self.current_focus_node.center()
                cx_new, cy_new = historical_match.center()
                distance = np.sqrt((cx_new - cx_prev)**2 + (cy_new - cy_prev)**2)
                
                edge = {
                    'from': self.current_focus_node.node_id,
                    'to': historical_match.node_id,
                    'distance': distance,
                    'frame': self.frame_count
                }
                self.focus_edges.append(edge)
            
            self.previous_focus_node = self.current_focus_node
            self.current_focus_node = historical_match
            
            return historical_match, 'recognized'  # Recognized from history!
        
        # NEW FOCUS! Create node
        node = FocusNode(
            self.next_node_id,
            bbox,
            focus,
            self.frame_count
        )
        
        self.focus_nodes[self.next_node_id] = node
        self.next_node_id += 1
        
        # Create EXACT edge from previous to new focus
        if self.current_focus_node:
            cx_prev, cy_prev = self.current_focus_node.center()
            cx_new, cy_new = node.center()
            
            distance = np.sqrt((cx_new - cx_prev)**2 + (cy_new - cy_prev)**2)
            
            edge = {
                'from': self.current_focus_node.node_id,
                'to': node.node_id,
                'distance': distance,
                'frame': self.frame_count
            }
            
            self.focus_edges.append(edge)
        
        # Update focus tracking
        self.previous_focus_node = self.current_focus_node
        self.current_focus_node = node
        
        return node, True  # New node
    
    def process_frame(self, frame):
        """Main processing"""
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # 1. Detect all candidates
        candidates = self.detect_candidates(frame, gray)
        
        # 2. Select ONE focus
        focus = self.select_focus(candidates)
        
        # 3. Create/update node only for focused object
        focus_node = None
        is_new = False
        if focus:
            focus_node, is_new = self.create_or_update_node(focus)
            focus['node'] = focus_node
            focus['is_new'] = is_new
        
        self.prev_frame = gray.copy()
        
        return {
            'candidates': candidates,
            'focus': focus,
            'nodes': self.focus_nodes,
            'edges': self.focus_edges
        }

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_focus_view(frame, data, frame_count):
    """Draw with focus emphasis"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    candidates = data['candidates']
    focus = data['focus']
    nodes = data['nodes']
    edges = data['edges']
    
    # Draw focus trail (edges between focus points)
    for edge in edges[-20:]:  # Last 20 focus transitions
        from_node = nodes.get(edge['from'])
        to_node = nodes.get(edge['to'])
        
        if from_node and to_node:
            cx_from, cy_from = from_node.center()
            cx_to, cy_to = to_node.center()
            
            # Draw thick line showing attention path
            cv2.line(display, (cx_from, cy_from), (cx_to, cy_to),
                    (0, 255, 255), 3)
            
            # Draw arrow
            cv2.arrowedLine(display, (cx_from, cy_from), (cx_to, cy_to),
                           (0, 200, 255), 2, tipLength=0.3)
            
            # Distance
            mid_x = (cx_from + cx_to) // 2
            mid_y = (cy_from + cy_to) // 2
            cv2.putText(display, f"{int(edge['distance'])}px", (mid_x, mid_y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 2)
    
    # Draw ALL candidates (clear boxes for visual clarity)
    for candidate in candidates:
        x, y, w, h = candidate['bbox']
        
        # Color by detection method (like working version)
        if candidate['method'] == 'skin':
            color = (150, 150, 150)  # Gray for skin candidates
        else:
            color = (200, 200, 200)  # Light gray for objects
        
        # Draw box (clear outline)
        cv2.rectangle(display, (x, y), (x+w, y+h), color, 2)
        
        # Attention score above box
        att = candidate.get('attention', 0)
        cv2.putText(display, f"A:{att:.2f}", (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)
    
    # Draw FOCUS (large, bright)
    if focus:
        x, y, w, h = focus['bbox']
        cx, cy = x + w//2, y + h//2
        
        # Method color
        if focus['method'] == 'skin':
            color = (255, 150, 0)  # Orange = skin (hands/face!)
        else:
            color = (0, 255, 255)  # Yellow = object
        
        # Large box
        cv2.rectangle(display, (x, y), (x+w, y+h), color, 4)
        
        # Circle + crosshair
        cv2.circle(display, (cx, cy), max(w, h)//2 + 15, color, 3)
        cv2.drawMarker(display, (cx, cy), color, cv2.MARKER_CROSS, 50, 3)
        
        # Node info
        node = focus.get('node')
        if node:
            is_new_status = focus.get('is_new', False)
            if is_new_status == 'recognized':
                label = f"FOCUS: Node #{node.node_id} (REMEMBERED!)"
                label_color = (255, 0, 255)  # Magenta for recognized!
            elif is_new_status == True:
                label = f"FOCUS: Node #{node.node_id} (NEW!)"
                label_color = (0, 255, 0)
            else:
                label = f"FOCUS: Node #{node.node_id} ({node.total_focus_time} frames)"
                label_color = color
            
            cv2.putText(display, label, (cx - 120, cy - 60),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, label_color, 2)
            
            # Method
            method_text = f"[{focus['method'].upper()}]"
            cv2.putText(display, method_text, (cx - 50, cy + 70),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
            
            # WHY is this focused? Show feature breakdown!
            y_breakdown = cy + 95
            cv2.putText(display, "WHY FOCUSED:", (cx - 80, y_breakdown),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 0), 1)
            
            y_breakdown += 18
            edge_val = focus.get('edge_score', 0)
            edge_contrib = edge_val * 0.4
            bar_len = int(edge_contrib * 200)
            cv2.rectangle(display, (cx - 80, y_breakdown - 10), 
                         (cx - 80 + bar_len, y_breakdown), (100, 200, 255), -1)
            cv2.putText(display, f"Edge: {edge_val:.2f} (×0.4={edge_contrib:.2f})", 
                       (cx - 75, y_breakdown - 2),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
            
            y_breakdown += 15
            color_val = focus.get('color_score', 0)
            color_contrib = color_val * 0.3
            bar_len = int(color_contrib * 200)
            cv2.rectangle(display, (cx - 80, y_breakdown - 10), 
                         (cx - 80 + bar_len, y_breakdown), (255, 200, 100), -1)
            cv2.putText(display, f"Color: {color_val:.2f} (×0.3={color_contrib:.2f})", 
                       (cx - 75, y_breakdown - 2),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
            
            y_breakdown += 15
            if focus['method'] == 'skin':
                cv2.rectangle(display, (cx - 80, y_breakdown - 10), 
                             (cx - 20, y_breakdown), (255, 150, 0), -1)
                cv2.putText(display, "SKIN BONUS: +0.3", 
                           (cx - 75, y_breakdown - 2),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
            
            y_breakdown += 20
            total_att = focus.get('attention', 0)
            cv2.putText(display, f"TOTAL ATTENTION: {total_att:.2f}", 
                       (cx - 80, y_breakdown),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 255), 2)
    
    # Info panel
    panel_h = 160
    cv2.rectangle(display, (0, 0), (550, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (550, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Focus-Driven Vision", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Detected: {len(candidates)} objects (boxes shown)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
    
    y_pos += 20
    cv2.putText(display, f"Total nodes: {len(nodes)} (across all sessions)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
    
    y_pos += 20
    cv2.putText(display, f"Edges: {len(edges)} (focus transitions)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
    
    y_pos += 20
    cv2.putText(display, "💾 Memory persists between sessions!", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 150, 255), 1)
    
    y_pos += 25
    cv2.putText(display, "Gray boxes = Objects detected", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.45, (180, 180, 180), 1)
    
    y_pos += 18
    cv2.putText(display, "Yellow = FOCUS (becomes node when changed)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.45, (100, 200, 255), 1)
    
    y_pos += 20
    if focus:
        method = focus.get('method', 'object')
        if method == 'skin':
            cv2.putText(display, "Focusing: SKIN (hand/face)", (10, y_pos),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 150, 0), 2)
        else:
            cv2.putText(display, f"Focusing: {method.upper()}", (10, y_pos),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
    
    # Attention comparison panel (right side)
    if len(candidates) > 0:
        comp_panel_x = w - 320
        comp_panel_y = 10
        comp_panel_w = 310
        comp_panel_h = min(200, 30 + len(candidates[:5]) * 35)
        
        cv2.rectangle(display, (comp_panel_x, comp_panel_y), 
                     (comp_panel_x + comp_panel_w, comp_panel_y + comp_panel_h),
                     (0, 0, 0), -1)
        cv2.rectangle(display, (comp_panel_x, comp_panel_y), 
                     (comp_panel_x + comp_panel_w, comp_panel_y + comp_panel_h),
                     (255, 255, 255), 2)
        
        y_comp = comp_panel_y + 20
        cv2.putText(display, "ATTENTION SCORES:", (comp_panel_x + 10, y_comp),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
        
        # Show top 5 candidates with scores
        top_candidates = sorted(candidates, key=lambda c: c['attention'], reverse=True)[:5]
        for i, cand in enumerate(top_candidates):
            y_comp += 30
            
            att = cand['attention']
            method = cand['method'][:4].upper()
            
            # Color for focused vs not
            if focus and cand['bbox'] == focus['bbox']:
                text_color = (0, 255, 255)  # Yellow = focused!
                prefix = "→"
            else:
                text_color = (180, 180, 180)
                prefix = " "
            
            # Attention bar
            bar_len = int(att * 150)
            cv2.rectangle(display, (comp_panel_x + 10, y_comp - 12), 
                         (comp_panel_x + 10 + bar_len, y_comp - 2),
                         text_color, -1)
            
            # Text
            text = f"{prefix} {method}: {att:.2f}"
            cv2.putText(display, text, (comp_panel_x + 10, y_comp),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.4, text_color, 1)
    
    # Legend
    legend_y = h - 110
    cv2.rectangle(display, (w-320, legend_y), (w, h), (0, 0, 0), -1)
    cv2.rectangle(display, (w-320, legend_y), (w, h), (255, 255, 255), 1)
    
    y_pos = legend_y + 20
    cv2.putText(display, "LEGEND:", (w-310, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    cv2.rectangle(display, (w-310, y_pos-10), (w-290, y_pos), (200, 200, 200), 2)
    cv2.putText(display, "Gray = Candidate", (w-280, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-310, y_pos-10), (w-290, y_pos), (0, 255, 0), 4)
    cv2.putText(display, "Green = NEW node", (w-280, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-310, y_pos-10), (w-290, y_pos), (255, 0, 255), 4)
    cv2.putText(display, "Magenta = REMEMBERED!", (w-280, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.line(display, (w-310, y_pos-5), (w-290, y_pos-5), (0, 255, 255), 3)
    cv2.putText(display, "Arrow = EXACT edge", (w-280, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN
# ============================================================================

def main():
    print("\n" + "="*70)
    print("🎯 MELVIN v2 - Focus-Driven Vision with Memory")
    print("="*70 + "\n")
    print("CONCEPT:")
    print("  • Boxes around ALL detected objects (visual clarity)")
    print("  • Gray boxes = Just detected (no node yet)")
    print("  • Yellow box = FOCUS (highest attention)")
    print("  • Node created ONLY when focus changes")
    print("  • EXACT edge: previous focus → new focus")
    print()
    print("FEATURES:")
    print("  • Detects objects AND skin (hands/face!)")
    print("  • Boxes clarify what objects are")
    print("  • Clean memory (only focused = nodes)")
    print("  • Yellow arrows = Attention path (EXACT edges)")
    print()
    
    vision = FocusDrivenVision()
    
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ Cannot open camera")
        return
    
    print("✓ Camera opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  s - Show graph")
    print()
    print("Watch as MELVIN:")
    print("  • Shows boxes around ALL detected objects (clarity!)")
    print("  • Focuses on ONE at a time (yellow box)")
    print("  • Creates node when focus changes")
    print("  • Draws EXACT edge (arrow) showing focus shift")
    print("  • Builds clean graph of attention path!")
    print()
    print("="*70 + "\n")
    
    fps_start = time.time()
    fps_count = 0
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Process
            data = vision.process_frame(frame)
            
            # Draw
            display = draw_focus_view(frame, data, vision.frame_count)
            
            # FPS
            fps_count += 1
            if time.time() - fps_start > 0.5:
                current_fps = fps_count / (time.time() - fps_start)
                fps_start = time.time()
                fps_count = 0
                
                cv2.putText(display, f"FPS: {current_fps:.1f}",
                           (display.shape[1] - 120, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            cv2.imshow('MELVIN Focus-Driven Vision', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                print(f"\n📊 FOCUS GRAPH:")
                print(f"  Total focus nodes: {len(vision.focus_nodes)}")
                print(f"  Focus transitions (edges): {len(vision.focus_edges)}")
                print()
                print("  Focus sequence:")
                for edge in vision.focus_edges[-10:]:
                    print(f"    Node #{edge['from']} → Node #{edge['to']} ({int(edge['distance'])}px)")
                print()
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        
        print(f"\n✓ Session complete")
        print(f"  Frames: {vision.frame_count}")
        print(f"  Focus nodes created: {len(vision.focus_nodes)}")
        print(f"  Focus edges: {len(vision.focus_edges)}")
        
        # Save focus graph
        graph = {
            'nodes': [
                {
                    'id': node.node_id,
                    'bbox': node.bbox,
                    'center': node.center(),
                    'created_frame': node.created_frame,
                    'total_focus_time': node.total_focus_time
                }
                for node in vision.focus_nodes.values()
            ],
            'edges': vision.focus_edges
        }
        
        with open('melvin_focus_graph.json', 'w') as f:
            json.dump(graph, f, indent=2)
        
        print(f"\n💾 Saved focus graph: melvin_focus_graph.json")
        print(f"   {len(vision.focus_nodes)} nodes (only what was focused on)")
        print(f"   {len(vision.focus_edges)} edges (focus transitions)")

if __name__ == "__main__":
    main()

