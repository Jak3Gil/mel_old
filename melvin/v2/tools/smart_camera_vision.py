#!/usr/bin/env python3
"""
MELVIN v2 - Smart Camera Vision with Object Memory

Features:
- Detects objects of ALL sizes (big and small)
- Creates persistent nodes for each object (memory!)
- Inhibition of return (doesn't linger on same object)
- Context-aware attention (novelty + relevance)
- Gradient visualization (optional, for humans)
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
    """Persistent memory node for a tracked object"""
    
    def __init__(self, node_id, bbox, features, frame_num):
        self.node_id = node_id
        self.bbox = bbox  # (x, y, w, h)
        self.features = features  # {edge, motion, color}
        
        # Tracking
        self.first_seen = frame_num
        self.last_seen = frame_num
        self.times_focused = 0
        self.total_focus_duration = 0
        
        # Memory
        self.is_novel = True  # New object
        self.relevance_score = 0.5  # How relevant to current context
        
    def update(self, bbox, features, frame_num):
        """Update object state"""
        self.bbox = bbox
        self.features = features
        self.last_seen = frame_num
        
    def get_age(self, current_frame):
        """How long since first seen"""
        return current_frame - self.first_seen
    
    def get_recency(self, current_frame):
        """How recently was it seen"""
        return current_frame - self.last_seen
    
    def mark_focused(self):
        """Mark that this object was focused on"""
        self.times_focused += 1
        self.total_focus_duration += 1
        if self.times_focused > 3:
            self.is_novel = False  # Familiar now

# ============================================================================
# SMART VISION PROCESSOR
# ============================================================================

class SmartVisionProcessor:
    """
    Vision with object memory and smart attention
    
    - Detects ALL objects (big and small)
    - Creates persistent nodes
    - Inhibition of return (moves focus away from recent)
    - Context-aware (prefers novel + relevant)
    """
    
    def __init__(self, use_gradient=True):
        self.use_gradient = use_gradient
        
        # Object memory
        self.object_nodes = {}  # node_id -> ObjectNode
        self.next_node_id = 0
        
        # Attention state
        self.current_focus = None
        self.focus_history = []  # Recent focus targets
        self.focus_start_frame = 0
        
        # Frame history for motion detection
        self.prev_frame = None
        
        # Parameters
        self.iou_threshold = 0.4  # For matching objects across frames
        self.min_object_size = 25  # SMALL! Detect everything
        self.focus_duration_limit = 30  # Max frames to focus on same thing
        self.novelty_bonus = 0.5  # Boost for new objects
        self.inhibition_penalty = 0.8  # Reduce attention to recently focused
        
        # Performance
        self.frame_count = 0
        
    def compute_iou(self, box1, box2):
        """Intersection over Union for box matching"""
        x1_min, y1_min, w1, h1 = box1
        x2_min, y2_min, w2, h2 = box2
        
        x1_max, y1_max = x1_min + w1, y1_min + h1
        x2_max, y2_max = x2_min + w2, y2_min + h2
        
        # Intersection
        inter_xmin = max(x1_min, x2_min)
        inter_ymin = max(y1_min, y2_min)
        inter_xmax = min(x1_max, x2_max)
        inter_ymax = min(y1_max, y2_max)
        
        if inter_xmax < inter_xmin or inter_ymax < inter_ymin:
            return 0.0
        
        inter_area = (inter_xmax - inter_xmin) * (inter_ymax - inter_ymin)
        
        # Union
        box1_area = w1 * h1
        box2_area = w2 * h2
        union_area = box1_area + box2_area - inter_area
        
        return inter_area / union_area if union_area > 0 else 0.0
    
    def find_matching_node(self, bbox, features):
        """Find if this detection matches an existing node"""
        best_match = None
        best_iou = 0.0
        
        for node_id, node in self.object_nodes.items():
            # Skip if not seen recently (object left scene)
            if self.frame_count - node.last_seen > 30:
                continue
            
            iou = self.compute_iou(node.bbox, bbox)
            if iou > self.iou_threshold and iou > best_iou:
                best_iou = iou
                best_match = node
        
        return best_match, best_iou
    
    def detect_all_objects(self, frame, gray):
        """Detect ALL objects (big and small) - SENSITIVE"""
        # Method 1: Background subtraction (motion/difference)
        # Simple frame difference for moving objects
        if self.prev_frame is not None:
            frame_diff = cv2.absdiff(gray, self.prev_frame)
            _, motion_thresh = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)
        else:
            motion_thresh = np.zeros_like(gray)
        
        # Method 2: Multiple thresholds to catch different lighting
        _, thresh_low = cv2.threshold(gray, 60, 255, cv2.THRESH_BINARY)
        _, thresh_mid = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        _, thresh_high = cv2.threshold(gray, 140, 255, cv2.THRESH_BINARY)
        
        # Method 3: Edge detection (for object boundaries)
        edges = cv2.Canny(gray, 30, 100)  # Lower thresholds = more edges
        
        # Method 4: Inverse threshold (for dark objects on bright background)
        _, thresh_inv = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY_INV)
        
        # COMBINE ALL METHODS (generous - catches everything!)
        combined = cv2.bitwise_or(thresh_low, thresh_mid)
        combined = cv2.bitwise_or(combined, thresh_high)
        combined = cv2.bitwise_or(combined, edges)
        combined = cv2.bitwise_or(combined, motion_thresh)
        combined = cv2.bitwise_or(combined, thresh_inv)
        
        # Morphology to clean up and connect nearby regions
        kernel = np.ones((5,5), np.uint8)
        combined = cv2.morphologyEx(combined, cv2.MORPH_CLOSE, kernel)
        combined = cv2.dilate(combined, kernel, iterations=2)
        
        # Find contours
        contours, _ = cv2.findContours(combined, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # Store for next frame
        self.prev_frame = gray.copy()
        
        # Get frame dimensions
        frame_h, frame_w = gray.shape
        frame_area = frame_h * frame_w
        
        detections = []
        for contour in contours:
            area = cv2.contourArea(contour)
            
            # Filter: Must be larger than min size but not entire frame
            if area > self.min_object_size and area < frame_area * 0.95:  # Not more than 95% of frame!
                x, y, w, h = cv2.boundingRect(contour)
                
                # Skip ONLY if it's basically the entire frame (very lenient)
                if w >= frame_w * 0.98 and h >= frame_h * 0.98:
                    continue
                
                # Extract features
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
    
    def update_object_memory(self, detections):
        """Update persistent object nodes"""
        matched_nodes = set()
        
        for detection in detections:
            bbox = detection['bbox']
            features = detection['features']
            
            # Try to match with existing node
            matched_node, iou = self.find_matching_node(bbox, features)
            
            if matched_node:
                # UPDATE existing node
                matched_node.update(bbox, features, self.frame_count)
                matched_nodes.add(matched_node.node_id)
                detection['node'] = matched_node
            else:
                # CREATE new node
                node = ObjectNode(self.next_node_id, bbox, features, self.frame_count)
                self.object_nodes[self.next_node_id] = node
                self.next_node_id += 1
                detection['node'] = node
                matched_nodes.add(node.node_id)
        
        # Cleanup old nodes (not seen in 50 frames)
        to_remove = []
        for node_id, node in self.object_nodes.items():
            if self.frame_count - node.last_seen > 50:
                to_remove.append(node_id)
        
        for node_id in to_remove:
            del self.object_nodes[node_id]
        
        return detections
    
    def compute_attention_scores(self, detections):
        """
        Compute attention with:
        - Saliency (bottom-up: edges, color)
        - Novelty (new objects get boost)
        - Inhibition of return (recently focused get penalty)
        - Context (relevance based on what we're looking for)
        """
        for detection in detections:
            node = detection['node']
            features = node.features
            
            # BASE SALIENCY (features)
            saliency = (
                0.4 * features['edge'] +
                0.3 * features['color'] +
                0.3 * (features['size'] / 10000.0)  # Size normalized
            )
            
            # NOVELTY BONUS (new objects more interesting)
            novelty = self.novelty_bonus if node.is_novel else 0.0
            
            # INHIBITION OF RETURN (recently focused less interesting)
            inhibition = 0.0
            for i, (hist_node_id, hist_frame) in enumerate(self.focus_history[-10:]):
                if hist_node_id == node.node_id:
                    recency = self.frame_count - hist_frame
                    if recency < 30:
                        # Recently focused → penalize
                        inhibition = self.inhibition_penalty * (1.0 - recency/30.0)
                        break
            
            # CONTEXT RELEVANCE (placeholder - could be goal-driven)
            relevance = node.relevance_score
            
            # FINAL ATTENTION SCORE
            attention = saliency + novelty + relevance - inhibition
            attention = max(0.0, attention)  # Non-negative
            
            detection['attention'] = attention
            detection['saliency'] = saliency
            detection['novelty'] = novelty
            detection['inhibition'] = inhibition
        
        return detections
    
    def select_focus(self, detections):
        """
        Select best focus target
        
        Smart rules:
        - If current focus still valid and not too long → keep it
        - Otherwise → pick highest attention
        """
        if not detections:
            self.current_focus = None
            return None
        
        # Check if we should keep current focus
        if self.current_focus:
            focus_duration = self.frame_count - self.focus_start_frame
            
            # Find current focus in detections
            current_still_visible = False
            for detection in detections:
                if detection['node'].node_id == self.current_focus:
                    current_still_visible = True
                    
                    # Keep focus if not too long and still interesting
                    if focus_duration < self.focus_duration_limit:
                        if detection['attention'] > 0.3:  # Still interesting
                            detection['node'].mark_focused()
                            return detection
                    break
        
        # Switch focus: pick highest attention
        best_detection = max(detections, key=lambda d: d['attention'])
        
        # Update focus
        if self.current_focus != best_detection['node'].node_id:
            # Focus changed!
            self.current_focus = best_detection['node'].node_id
            self.focus_start_frame = self.frame_count
            
            # Add to history
            self.focus_history.append((self.current_focus, self.frame_count))
            if len(self.focus_history) > 20:
                self.focus_history.pop(0)
        
        best_detection['node'].mark_focused()
        return best_detection
    
    def process_frame(self, frame):
        """Main processing loop"""
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # 1. Detect all objects
        detections = self.detect_all_objects(frame, gray)
        
        # 2. Update object memory (match with existing nodes)
        detections = self.update_object_memory(detections)
        
        # 3. Compute attention scores (saliency + novelty + inhibition)
        detections = self.compute_attention_scores(detections)
        
        # 4. Select focus (smart selection with context)
        focus = self.select_focus(detections)
        
        return {
            'detections': detections,
            'focus': focus,
            'nodes': self.object_nodes
        }

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_smart_overlay(frame, data, use_gradient):
    """Draw visualization with object memory"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    detections = data['detections']
    focus = data['focus']
    nodes = data['nodes']
    
    # Optional: Gradient darkening/brightening
    if use_gradient and len(detections) > 0:
        # Create attention heatmap
        heatmap = np.zeros((h, w), dtype=np.float32)
        
        # Add attention around each object
        for detection in detections:
            x, y, w_box, h_box = detection['bbox']
            att = detection['attention']
            
            # Create attention blob
            cx, cy = x + w_box//2, y + h_box//2
            radius = max(w_box, h_box) // 2 + 20
            
            # Draw circular attention region
            for dy in range(-radius, radius):
                for dx in range(-radius, radius):
                    py, px = cy + dy, cx + dx
                    if 0 <= py < h and 0 <= px < w:
                        dist = np.sqrt(dx*dx + dy*dy)
                        if dist <= radius:
                            # Gaussian falloff
                            val = att * np.exp(-(dist / radius) ** 2)
                            heatmap[py, px] = max(heatmap[py, px], val)
        
        # Normalize heatmap
        if heatmap.max() > 0:
            heatmap = heatmap / heatmap.max()
        
        # Darken areas with low attention
        darkness_mask = (1.0 - heatmap) * 60  # Max 60 darkness
        darkness_mask = darkness_mask.astype(np.uint8)
        darkness_3ch = cv2.cvtColor(darkness_mask, cv2.COLOR_GRAY2BGR)
        
        display = cv2.subtract(display, darkness_3ch)
        
        # Brighten areas with high attention (subtle)
        brightness_mask = (heatmap * 30).astype(np.uint8)  # Max 30 brightness
        brightness_3ch = cv2.cvtColor(brightness_mask, cv2.COLOR_GRAY2BGR)
        
        display = cv2.add(display, brightness_3ch)
    
    # Draw all objects
    for detection in detections:
        node = detection['node']
        x, y, w, h = detection['bbox']
        
        # Color based on state
        if node.is_novel:
            color = (0, 255, 0)  # Green = new
            status = "NEW"
        elif node.times_focused > 5:
            color = (128, 128, 128)  # Gray = familiar/boring
            status = "SEEN"
        else:
            color = (200, 200, 200)  # White = normal
            status = ""
        
        # Box thickness based on attention
        thickness = 1 + int(detection['attention'] * 2)
        
        cv2.rectangle(display, (x, y), (x+w, y+h), color, thickness)
        
        # Label
        label = f"#{node.node_id} {status}"
        cv2.putText(display, label, (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)
    
    # Draw focus (large yellow indicator)
    if focus:
        node = focus['node']
        x, y, w, h = node.bbox
        cx, cy = x + w//2, y + h//2
        
        # Yellow circle + crosshair
        cv2.circle(display, (cx, cy), max(w, h)//2 + 10, (0, 255, 255), 3)
        cv2.drawMarker(display, (cx, cy), (0, 255, 255),
                      cv2.MARKER_CROSS, 40, 3)
        
        # Focus label
        label = f"FOCUS #{node.node_id} (seen {node.times_focused}x)"
        cv2.putText(display, label, (cx - 80, cy - 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        
        # Attention breakdown
        y_off = cy + 50
        cv2.putText(display, f"Sal:{focus['saliency']:.2f}", (cx-60, y_off),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 255, 150), 1)
        cv2.putText(display, f"Nov:{focus['novelty']:.2f}", (cx, y_off),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 150), 1)
        cv2.putText(display, f"Inh:{focus['inhibition']:.2f}", (cx+60, y_off),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 150, 150), 1)
    
    # Info panel
    panel_h = 140
    cv2.rectangle(display, (0, 0), (500, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (500, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Smart Vision + Memory", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    if len(detections) == 0:
        cv2.putText(display, "Objects: 0 detected", (10, y_pos),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 100, 100), 1)
        y_pos += 15
        cv2.putText(display, "  (Point at edges/objects)", (10, y_pos),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (200, 200, 200), 1)
    else:
        cv2.putText(display, f"Objects: {len(detections)} detected", (10, y_pos),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 255, 100), 1)
    
    y_pos += 20
    cv2.putText(display, f"Memory: {len(nodes)} nodes", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    novel_count = sum(1 for n in nodes.values() if n.is_novel)
    cv2.putText(display, f"Novel: {novel_count} new objects", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    
    y_pos += 25
    if focus:
        focus_dur = focus['node'].total_focus_duration
        cv2.putText(display, f"Focus: #{focus['node'].node_id} ({focus_dur} frames)", (10, y_pos),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
    
    # Legend
    legend_y = h - 80
    cv2.rectangle(display, (w-220, legend_y), (w, h), (0, 0, 0), -1)
    cv2.rectangle(display, (w-220, legend_y), (w, h), (255, 255, 255), 1)
    
    y_pos = legend_y + 20
    cv2.putText(display, "LEGEND:", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-210, y_pos-10), (w-190, y_pos), (0, 255, 0), 2)
    cv2.putText(display, "New object", (w-180, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-210, y_pos-10), (w-190, y_pos), (200, 200, 200), 2)
    cv2.putText(display, "Known object", (w-180, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.circle(display, (w-200, y_pos-5), 8, (0, 255, 255), 2)
    cv2.putText(display, "Focused", (w-180, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN
# ============================================================================

def main():
    import argparse
    
    parser = argparse.ArgumentParser()
    parser.add_argument('--gradient', action='store_true',
                       help='Enable gradient effect (disabled by default for clarity)')
    parser.add_argument('--camera', type=int, default=0,
                       help='Camera index')
    args = parser.parse_args()
    
    print("\n" + "="*70)
    print("🧠 MELVIN v2 - Smart Vision with Object Memory")
    print("="*70 + "\n")
    print("Features:")
    print("  • Detects ALL objects (big & small)")
    print("  • Creates persistent memory nodes")
    print("  • Inhibition of return (doesn't linger)")
    print("  • Context-aware attention")
    print()
    print("NOTE: Gradient disabled by default (press 'g' to enable)")
    print("      This lets you see detection clearly first!")
    print()
    
    # Initialize (gradient OFF by default for clearer view)
    vision = SmartVisionProcessor(use_gradient=args.gradient)
    
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        print(f"❌ Cannot open camera {args.camera}")
        return
    
    print(f"✓ Camera {args.camera} opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  s - Show statistics")
    print("  g - Toggle gradient")
    print()
    print("Watch as MELVIN:")
    print("  • Detects objects and creates nodes")
    print("  • Moves focus between novel objects")
    print("  • Remembers what it's seen")
    print("  • Doesn't get stuck on one thing!")
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
            display = draw_smart_overlay(frame, data, vision.use_gradient)
            
            # FPS
            fps_count += 1
            if time.time() - fps_start > 0.5:
                current_fps = fps_count / (time.time() - fps_start)
                fps_start = time.time()
                fps_count = 0
            
            cv2.putText(display, f"FPS: {current_fps:.1f}", 
                       (display.shape[1] - 120, 30),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            cv2.imshow('MELVIN Smart Vision', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('g'):
                vision.use_gradient = not vision.use_gradient
                print(f"Gradient: {'ON' if vision.use_gradient else 'OFF'}")
            elif key == ord('s'):
                print(f"\n📊 STATISTICS:")
                print(f"  Total nodes: {len(vision.object_nodes)}")
                print(f"  Novel objects: {sum(1 for n in vision.object_nodes.values() if n.is_novel)}")
                print(f"  Current focus: {vision.current_focus}")
                print(f"  Focus history: {len(vision.focus_history)}")
                print()
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        
        print(f"\n✓ Session complete")
        print(f"  Frames: {vision.frame_count}")
        print(f"  Total nodes created: {vision.next_node_id}")
        print(f"  Nodes in memory: {len(vision.object_nodes)}")

if __name__ == "__main__":
    main()

