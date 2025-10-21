#!/usr/bin/env python3
"""
MELVIN v2 - Universal Object Detector

Applies MediaPipe-inspired principles to ALL object detection:
1. Multi-scale detection (see objects at different scales)
2. Multiple feature channels (edges, color, texture, gradients)
3. Region proposals (hierarchical segmentation)
4. Temporal consistency (track across frames)
5. Confidence scoring (how sure we are)

Works for: Letters, hands, faces, objects - EVERYTHING!
"""

import cv2
import numpy as np
import json
import time
from pathlib import Path

# ============================================================================
# OBJECT NODE (Memory)
# ============================================================================

class ObjectNode:
    """Persistent memory node"""
    
    def __init__(self, node_id, bbox, features, frame_num):
        self.node_id = node_id
        self.bbox = bbox
        self.features = features
        self.first_seen = frame_num
        self.last_seen = frame_num
        self.times_seen = 1
    
    def update(self, bbox, features, frame_num):
        self.bbox = bbox
        self.features = features
        self.last_seen = frame_num
        self.times_seen += 1
    
    def center(self):
        x, y, w, h = self.bbox
        return (x + w//2, y + h//2)

# ============================================================================
# UNIVERSAL FEATURE EXTRACTOR (MediaPipe-Inspired)
# ============================================================================

class UniversalFeatureExtractor:
    """
    Extract rich features like MediaPipe does
    
    MediaPipe uses deep networks, but we can approximate with:
    - Multiple edge scales
    - Color gradients
    - Texture analysis
    - Gradient orientation
    """
    
    @staticmethod
    def extract_features(roi_gray, roi_color):
        """Extract multi-modal features"""
        if roi_gray.size == 0:
            return {'edge': 0, 'color': 0, 'texture': 0, 'gradient': 0}
        
        # 1. MULTI-SCALE EDGES (like MediaPipe pyramid)
        # Fine edges (fingers, small details)
        edges_fine = cv2.Canny(roi_gray, 10, 30)
        # Medium edges (object boundaries)
        edges_med = cv2.Canny(roi_gray, 30, 90)
        # Coarse edges (large structures)
        edges_coarse = cv2.Canny(roi_gray, 50, 150)
        
        edge_fine_density = np.sum(edges_fine > 0) / edges_fine.size
        edge_med_density = np.sum(edges_med > 0) / edges_med.size
        edge_coarse_density = np.sum(edges_coarse > 0) / edges_coarse.size
        
        # Combined edge score (all scales)
        edge_score = 0.5*edge_fine_density + 0.3*edge_med_density + 0.2*edge_coarse_density
        
        # 2. COLOR GRADIENTS (like skin detection)
        if roi_color.size > 0:
            # Convert to HSV for better color analysis
            hsv = cv2.cvtColor(roi_color, cv2.COLOR_BGR2HSV)
            
            # Hue variance (color diversity)
            hue_var = np.std(hsv[:,:,0]) / 180.0
            # Saturation (color intensity)
            sat_mean = np.mean(hsv[:,:,1]) / 255.0
            
            color_score = (hue_var + sat_mean) / 2.0
        else:
            color_score = 0.0
        
        # 3. TEXTURE ANALYSIS (local patterns)
        # Compute local binary patterns or gradient variance
        if roi_gray.shape[0] > 5 and roi_gray.shape[1] > 5:
            # Gradient in X and Y
            grad_x = cv2.Sobel(roi_gray, cv2.CV_64F, 1, 0, ksize=3)
            grad_y = cv2.Sobel(roi_gray, cv2.CV_64F, 0, 1, ksize=3)
            
            # Gradient magnitude
            gradient_mag = np.sqrt(grad_x**2 + grad_y**2)
            
            # Texture = variance of gradients
            texture_score = np.std(gradient_mag) / 128.0
            gradient_score = np.mean(gradient_mag) / 255.0
        else:
            texture_score = 0.0
            gradient_score = 0.0
        
        return {
            'edge': edge_score,
            'color': color_score,
            'texture': texture_score,
            'gradient': gradient_score
        }

# ============================================================================
# MULTI-METHOD DETECTOR (MediaPipe-Style)
# ============================================================================

class UniversalDetector:
    """
    Universal object detection using multiple methods
    
    Inspired by MediaPipe's approach:
    - Multiple detection streams
    - Confidence-based fusion
    - Temporal tracking
    """
    
    def __init__(self):
        self.min_object_size = 50  # Lower = detect smaller things
        self.feature_extractor = UniversalFeatureExtractor()
        
        # Object memory (like MediaPipe tracking)
        self.object_nodes = {}
        self.next_node_id = 0
        self.node_edges = []
        
        # Frame history
        self.prev_frame = None
        self.frame_count = 0
        
        # Detection confidence threshold
        self.confidence_threshold = 0.3
        
        # IoU for tracking
        self.iou_threshold = 0.3  # Lower = more lenient tracking
    
    def detect_method_1_edges(self, gray):
        """Method 1: Edge-based detection (good for letters, objects)"""
        edges = cv2.Canny(gray, 30, 100)
        
        # Dilate to connect nearby edges
        kernel = np.ones((3,3), np.uint8)
        edges = cv2.dilate(edges, kernel, iterations=2)
        
        contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        return contours, 'edge'
    
    def detect_method_2_threshold(self, gray):
        """Method 2: Brightness threshold (good for contrast)"""
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        return contours, 'threshold'
    
    def detect_method_3_skin(self, frame):
        """Method 3: Skin color detection (good for hands/face!)"""
        # Convert to HSV
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        
        # Skin color range (tuned for various skin tones)
        lower_skin = np.array([0, 20, 70], dtype=np.uint8)
        upper_skin = np.array([20, 255, 255], dtype=np.uint8)
        
        skin_mask = cv2.inRange(hsv, lower_skin, upper_skin)
        
        # Clean up noise
        kernel = np.ones((5,5), np.uint8)
        skin_mask = cv2.morphologyEx(skin_mask, cv2.MORPH_OPEN, kernel)
        skin_mask = cv2.morphologyEx(skin_mask, cv2.MORPH_CLOSE, kernel)
        
        contours, _ = cv2.findContours(skin_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        return contours, 'skin'
    
    def detect_method_4_gradient(self, gray):
        """Method 4: Gradient-based (good for soft boundaries)"""
        # Compute gradients
        grad_x = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=3)
        grad_y = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=3)
        
        gradient_mag = np.sqrt(grad_x**2 + grad_y**2)
        gradient_mag = np.uint8(gradient_mag)
        
        # Threshold gradients
        _, grad_thresh = cv2.threshold(gradient_mag, 20, 255, cv2.THRESH_BINARY)
        
        # Dilate to connect
        kernel = np.ones((5,5), np.uint8)
        grad_thresh = cv2.dilate(grad_thresh, kernel, iterations=2)
        
        contours, _ = cv2.findContours(grad_thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        return contours, 'gradient'
    
    def detect_all_methods(self, frame, gray):
        """Run ALL detection methods and merge results"""
        frame_h, frame_w = gray.shape
        frame_area = frame_h * frame_w
        
        all_detections = []
        
        # Run each detection method
        methods = [
            self.detect_method_1_edges(gray),
            self.detect_method_2_threshold(gray),
            self.detect_method_3_skin(frame),
            self.detect_method_4_gradient(gray)
        ]
        
        # Collect all detections with confidence
        for contours, method_name in methods:
            for contour in contours:
                area = cv2.contourArea(contour)
                
                if area > self.min_object_size and area < frame_area * 0.95:
                    x, y, w, h = cv2.boundingRect(contour)
                    
                    if w >= frame_w * 0.98 and h >= frame_h * 0.98:
                        continue
                    
                    # Extract rich features
                    roi_gray = gray[y:y+h, x:x+w]
                    roi_color = frame[y:y+h, x:x+w]
                    
                    features = self.feature_extractor.extract_features(roi_gray, roi_color)
                    
                    # Compute confidence (higher for multi-modal features)
                    confidence = (
                        0.3 * features['edge'] +
                        0.25 * features['color'] +
                        0.25 * features['texture'] +
                        0.2 * features['gradient']
                    )
                    
                    all_detections.append({
                        'bbox': (x, y, w, h),
                        'area': area,
                        'features': features,
                        'confidence': confidence,
                        'method': method_name
                    })
        
        # Non-maximum suppression (remove duplicates)
        detections = self.non_max_suppression(all_detections)
        
        return detections
    
    def non_max_suppression(self, detections):
        """Remove overlapping detections (keep highest confidence)"""
        if len(detections) == 0:
            return []
        
        # Sort by confidence
        detections.sort(key=lambda d: d['confidence'], reverse=True)
        
        keep = []
        while detections:
            best = detections.pop(0)
            keep.append(best)
            
            # Remove overlapping detections
            detections = [
                d for d in detections
                if self.compute_iou_tuples(best['bbox'], d['bbox']) < 0.5
            ]
        
        return keep
    
    def compute_iou_tuples(self, box1, box2):
        """IoU for tuples"""
        x1, y1, w1, h1 = box1
        x2, y2, w2, h2 = box2
        
        x1_max, y1_max = x1 + w1, y1 + h1
        x2_max, y2_max = x2 + w2, y2 + h2
        
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
        """Find matching node"""
        best_match = None
        best_iou = 0.0
        
        for node_id, node in self.object_nodes.items():
            if self.frame_count - node.last_seen > 30:
                continue
            
            iou = self.compute_iou_tuples(node.bbox, bbox)
            if iou > self.iou_threshold and iou > best_iou:
                best_iou = iou
                best_match = node
        
        return best_match
    
    def update_nodes_and_edges(self, detections):
        """Update nodes and create EXACT spatial edges"""
        current_nodes = []
        
        for detection in detections:
            bbox = detection['bbox']
            features = detection['features']
            confidence = detection['confidence']
            
            # Try to match existing node
            matched_node = self.find_matching_node(bbox)
            
            if matched_node:
                # RECOGNIZED! Update existing
                matched_node.update(bbox, features, self.frame_count)
                detection['node'] = matched_node
                detection['is_new'] = False
            else:
                # NEW! Create node
                node = ObjectNode(self.next_node_id, bbox, features, self.frame_count)
                self.object_nodes[self.next_node_id] = node
                self.next_node_id += 1
                detection['node'] = node
                detection['is_new'] = True
            
            current_nodes.append(detection)
        
        # Create EXACT edges (spatial connections)
        for i, det_a in enumerate(current_nodes):
            if len(current_nodes) <= 1:
                break
            
            node_a = det_a['node']
            cx_a, cy_a = node_a.center()
            
            # Find closest node
            closest_node = None
            closest_dist = float('inf')
            
            for j, det_b in enumerate(current_nodes):
                if i != j:
                    node_b = det_b['node']
                    cx_b, cy_b = node_b.center()
                    
                    dist = np.sqrt((cx_a - cx_b)**2 + (cy_a - cy_b)**2)
                    if dist < closest_dist:
                        closest_dist = dist
                        closest_node = node_b
            
            # Create edge
            if closest_node:
                edge = (node_a.node_id, closest_node.node_id, closest_dist, self.frame_count)
                self.node_edges.append(edge)
        
        return detections
    
    def process_frame(self, frame):
        """Process frame with universal detection"""
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # Run all detection methods
        detections = self.detect_all_methods(frame, gray)
        
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

def draw_universal_view(frame, data, frame_count):
    """Draw with method labels and confidence"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    detections = data['detections']
    nodes = data['nodes']
    edges = data['edges']
    
    # Draw edges (spatial connections)
    recent_edges = [e for e in edges if frame_count - e[3] < 5]
    for node_a_id, node_b_id, dist, edge_frame in recent_edges[-15:]:
        if node_a_id in nodes and node_b_id in nodes:
            node_a = nodes[node_a_id]
            node_b = nodes[node_b_id]
            
            cx_a, cy_a = node_a.center()
            cx_b, cy_b = node_b.center()
            
            # Fade old edges
            age = frame_count - edge_frame
            alpha = max(0, 1.0 - age/5.0)
            color_intensity = int(200 * alpha)
            
            cv2.line(display, (cx_a, cy_a), (cx_b, cy_b),
                    (color_intensity, color_intensity, 255), 2)
    
    # Draw objects
    for detection in detections:
        node = detection['node']
        x, y, w, h = node.bbox
        cx, cy = node.center()
        
        # Color by detection method
        method = detection.get('method', 'unknown')
        if method == 'skin':
            color = (255, 150, 0)  # Orange = skin detection (hands/face!)
        elif method == 'edge':
            color = (0, 255, 255)  # Cyan = edge detection (letters)
        elif method == 'gradient':
            color = (255, 0, 255)  # Magenta = gradient (soft boundaries)
        else:
            color = (255, 255, 255)  # White = threshold
        
        # Green if NEW
        if detection.get('is_new', False):
            color = (0, 255, 0)
        
        # Draw box
        thickness = 2 if detection['confidence'] > 0.5 else 1
        cv2.rectangle(display, (x, y), (x+w, y+h), color, thickness)
        
        # Label
        label = f"#{node.node_id}"
        if detection.get('is_new', False):
            label += " NEW"
        else:
            label += f" ({node.times_seen}x)"
        
        cv2.putText(display, label, (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
        
        # Method + confidence
        method_text = f"{method[:3].upper()} {detection['confidence']:.2f}"
        cv2.putText(display, method_text, (x, y+h+15),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, color, 1)
        
        # Center dot
        cv2.circle(display, (cx, cy), 4, color, -1)
    
    # Info panel
    panel_h = 180
    cv2.rectangle(display, (0, 0), (550, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (550, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Universal Detector + Memory", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Objects: {len(detections)} | Nodes: {len(nodes)}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    new_count = sum(1 for d in detections if d.get('is_new', False))
    cv2.putText(display, f"New: {new_count} | Recognized: {len(detections) - new_count}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    
    y_pos += 25
    cv2.putText(display, "DETECTION METHODS:", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 200, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (15, y_pos-10), (35, y_pos), (255, 150, 0), -1)
    cv2.putText(display, "Skin (hands/face)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 16
    cv2.rectangle(display, (15, y_pos-10), (35, y_pos), (0, 255, 255), -1)
    cv2.putText(display, "Edge (letters/objects)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 16
    cv2.rectangle(display, (15, y_pos-10), (35, y_pos), (255, 0, 255), -1)
    cv2.putText(display, "Gradient (soft bounds)", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 16
    cv2.rectangle(display, (15, y_pos-10), (35, y_pos), (0, 255, 0), -1)
    cv2.putText(display, "NEW node", (45, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN
# ============================================================================

def main():
    print("\n" + "="*70)
    print("🧠 MELVIN v2 - Universal Detector (MediaPipe-Inspired)")
    print("="*70 + "\n")
    print("MULTI-METHOD DETECTION:")
    print("  1. Edge detection     → Letters, objects, shapes")
    print("  2. Threshold          → High-contrast items")
    print("  3. Skin detection     → Hands, face, people! 👤")
    print("  4. Gradient detection → Soft boundaries")
    print()
    print("MEMORY & GRAPH:")
    print("  • Persistent nodes (same object = same ID)")
    print("  • EXACT edges (spatial connections)")
    print("  • Recognition (seen 1x, 2x, 3x...)")
    print()
    
    detector = UniversalDetector()
    
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
    print("Watch boxes colored by detection method:")
    print("  🟠 Orange = Skin detection (YOUR HANDS/FACE!)")
    print("  🔵 Cyan = Edge detection (letters)")
    print("  🟣 Magenta = Gradient detection (soft boundaries)")
    print("  🟢 Green overlay = NEW node")
    print()
    print("="*70 + "\n")
    
    fps_start = time.time()
    fps_count = 0
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            data = detector.process_frame(frame)
            display = draw_universal_view(frame, data, detector.frame_count)
            
            # FPS
            fps_count += 1
            if time.time() - fps_start > 0.5:
                current_fps = fps_count / (time.time() - fps_start)
                fps_start = time.time()
                fps_count = 0
                
                cv2.putText(display, f"FPS: {current_fps:.1f}",
                           (display.shape[1] - 120, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
            
            cv2.imshow('MELVIN Universal Detector', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'):
                break
            elif key == ord('s'):
                print(f"\n📊 STATISTICS:")
                print(f"  Nodes: {len(detector.object_nodes)}")
                print(f"  Edges: {len(detector.node_edges)}")
                
                # Count by method
                methods = {}
                for d in data['detections']:
                    m = d.get('method', 'unknown')
                    methods[m] = methods.get(m, 0) + 1
                
                print(f"  Detection methods:")
                for method, count in methods.items():
                    print(f"    {method}: {count}")
                print()
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        
        print(f"\n✓ Complete")
        print(f"  Frames: {detector.frame_count}")
        print(f"  Nodes: {detector.next_node_id}")

if __name__ == "__main__":
    main()

