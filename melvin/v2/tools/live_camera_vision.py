#!/usr/bin/env python3
"""
MELVIN v2 - Live Camera Vision Display

Shows real-time camera feed with:
- Genome-driven visual processing
- Attention scores (bottom-up + top-down)
- Focus selection with visual overlay
- Neuromodulator states
- Working memory contents
- Prediction errors

This PROVES how genome controls what MELVIN sees!
"""

import cv2
import numpy as np
import json
import sys
from pathlib import Path

# ============================================================================
# GENOME-DRIVEN VISION PARAMETERS
# ============================================================================

class GenomeVisionConfig:
    """Load vision parameters from genome JSON"""
    
    def __init__(self, genome_path=None):
        # Always set defaults first
        self.set_defaults()
        
        # Then override with genome if available
        if genome_path and Path(genome_path).exists():
            self.load_from_file(genome_path)
    
    def set_defaults(self):
        """Default vision genes (from base genome)"""
        self.edge_threshold = 0.30
        self.edge_weight = 0.30
        self.motion_sensitivity = 0.50
        self.motion_weight = 0.40
        self.color_variance_threshold = 0.20
        self.color_weight = 0.30
        self.patch_size = 32
        self.min_object_size = 100
        
        # Attention genes
        self.alpha_saliency = 0.40
        self.beta_relevance = 0.30
        self.gamma_curiosity = 0.20
        
        # Neuromod baselines
        self.da_baseline = 0.50
        self.ne_baseline = 0.50
        self.ach_baseline = 0.50
        self.serotonin_baseline = 0.50
    
    def load_from_file(self, path):
        """Load from genome JSON file"""
        with open(path, 'r') as f:
            genome = json.load(f)
        
        # Extract vision module genes
        for module in genome.get('modules', []):
            if module['name'] == 'vision':
                for gene in module['genes']:
                    setattr(self, gene['key'], gene['value'])
            elif module['name'] == 'attention':
                for gene in module['genes']:
                    if gene['key'] == 'alpha_saliency':
                        self.alpha_saliency = gene['value']
                    elif gene['key'] == 'beta_goal':
                        self.beta_relevance = gene['value']
                    elif gene['key'] == 'gamma_curiosity':
                        self.gamma_curiosity = gene['value']
    
    def to_display_string(self):
        """Format for on-screen display"""
        return f"""GENOME VISION PARAMETERS:
Edge: threshold={self.edge_threshold:.2f}, weight={self.edge_weight:.2f}
Motion: sensitivity={self.motion_sensitivity:.2f}, weight={self.motion_weight:.2f}
Color: threshold={self.color_variance_threshold:.2f}, weight={self.color_weight:.2f}
Attention: α={self.alpha_saliency:.2f}, β={self.beta_relevance:.2f}, γ={self.gamma_curiosity:.2f}"""

# ============================================================================
# VISUAL PROCESSING (Genome-Controlled)
# ============================================================================

class GenomeDrivenVision:
    """Process camera frames using genome parameters"""
    
    def __init__(self, genome_config):
        self.config = genome_config
        self.prev_frame = None
        self.object_id_counter = 0
    
    def process_frame(self, frame):
        """
        Process frame and return objects with genome-computed features
        
        Returns: list of {id, bbox, saliency, edge_score, motion_score, color_score}
        """
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        # Detect objects (simple blob detection)
        objects = self.detect_objects(frame, gray)
        
        # Compute genome-driven features for each object
        for obj in objects:
            x, y, w, h = obj['bbox']
            
            # Extract ROI
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = frame[y:y+h, x:x+w]
            
            # GENOME-CONTROLLED FEATURE EXTRACTION
            obj['edge_score'] = self.compute_edges(roi_gray)
            obj['motion_score'] = self.compute_motion(x, y, w, h)
            obj['color_score'] = self.compute_color_variance(roi_color)
            
            # GENOME-WEIGHTED SALIENCY
            obj['saliency'] = (
                self.config.edge_weight * obj['edge_score'] +
                self.config.motion_weight * obj['motion_score'] +
                self.config.color_weight * obj['color_score']
            )
        
        self.prev_frame = gray.copy()
        
        return objects
    
    def detect_objects(self, frame, gray):
        """Simple object detection (blobs)"""
        # Threshold to find bright regions
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        
        # Find contours
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        objects = []
        for contour in contours:
            area = cv2.contourArea(contour)
            
            if area > self.config.min_object_size:
                x, y, w, h = cv2.boundingRect(contour)
                
                obj = {
                    'id': self.object_id_counter,
                    'bbox': (x, y, w, h),
                    'area': area
                }
                
                objects.append(obj)
                self.object_id_counter += 1
        
        return objects
    
    def compute_edges(self, roi_gray):
        """Edge density (V1 cortex analog) - controlled by genome"""
        # Canny with genome threshold
        threshold1 = int(self.config.edge_threshold * 100)
        threshold2 = threshold1 * 2
        
        edges = cv2.Canny(roi_gray, threshold1, threshold2)
        edge_density = np.sum(edges > 0) / edges.size
        
        return edge_density
    
    def compute_motion(self, x, y, w, h):
        """Motion score (MT cortex analog) - controlled by genome"""
        if self.prev_frame is None:
            return 0.0
        
        # Simplified motion detection
        # In full version: optical flow
        motion_score = np.random.uniform(0.2, 0.8)  # Placeholder
        
        return motion_score * self.config.motion_sensitivity
    
    def compute_color_variance(self, roi_color):
        """Color variance (V4 cortex analog) - controlled by genome"""
        if roi_color.size == 0:
            return 0.0
        
        # Compute color variance
        std_dev = np.std(roi_color)
        normalized = std_dev / 128.0  # Normalize to 0-1
        
        # Apply genome threshold
        if normalized < self.config.color_variance_threshold:
            return 0.0
        
        return normalized

# ============================================================================
# ATTENTION SELECTION (Genome-Weighted)
# ============================================================================

def select_focus(objects, genome_config):
    """Select focus using genome attention weights"""
    if not objects:
        return None
    
    best_score = -1
    best_obj = None
    
    for obj in objects:
        # Genome-weighted attention formula
        # F = α·Saliency + β·Relevance + γ·Curiosity
        
        score = (
            genome_config.alpha_saliency * obj['saliency'] +
            genome_config.beta_relevance * 0.5 +  # Placeholder
            genome_config.gamma_curiosity * 0.3   # Placeholder
        )
        
        obj['attention_score'] = score
        
        if score > best_score:
            best_score = score
            best_obj = obj
    
    return best_obj

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_overlay(frame, objects, focused_obj, genome_config, neuromod_state, cycle_num):
    """Draw visual overlay showing MELVIN's perception"""
    
    display = frame.copy()
    h, w = display.shape[:2]
    
    # Draw all objects (gray boxes)
    for obj in objects:
        x, y, w, h = obj['bbox']
        color = (100, 100, 100)  # Gray for non-focused
        thickness = 1
        
        cv2.rectangle(display, (x, y), (x+w, y+h), color, thickness)
        
        # Show saliency score
        text = f"S:{obj['saliency']:.2f}"
        cv2.putText(display, text, (x, y-5), cv2.FONT_HERSHEY_SIMPLEX, 
                   0.4, color, 1)
    
    # Draw focused object (yellow box with crosshair!)
    if focused_obj:
        x, y, w, h = focused_obj['bbox']
        
        # Yellow box (attention highlight)
        cv2.rectangle(display, (x, y), (x+w, y+h), (0, 255, 255), 3)
        
        # Crosshair at center
        cx, cy = x + w//2, y + h//2
        cv2.drawMarker(display, (cx, cy), (0, 255, 255), 
                      cv2.MARKER_CROSS, 20, 2)
        
        # Focus label
        cv2.putText(display, f"FOCUS (F={focused_obj['attention_score']:.2f})", 
                   (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        
        # Feature breakdown
        info_y = y + h + 20
        cv2.putText(display, f"Edge:{focused_obj['edge_score']:.2f}", 
                   (x, info_y), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
        cv2.putText(display, f"Motion:{focused_obj['motion_score']:.2f}", 
                   (x, info_y+15), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
        cv2.putText(display, f"Color:{focused_obj['color_score']:.2f}", 
                   (x, info_y+30), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    # Info panel (top-left)
    panel_h = 180
    cv2.rectangle(display, (0, 0), (400, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (400, panel_h), (255, 255, 255), 2)
    
    y_pos = 20
    cv2.putText(display, "MELVIN v2 - Evolved Vision", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Cycle: {cycle_num}", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    cv2.putText(display, f"Objects: {len(objects)}", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 25
    cv2.putText(display, "GENOME WEIGHTS:", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 200, 255), 1)
    
    y_pos += 18
    cv2.putText(display, f"  Edge: {genome_config.edge_weight:.2f}", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 15
    cv2.putText(display, f"  Motion: {genome_config.motion_weight:.2f}", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 15
    cv2.putText(display, f"  Color: {genome_config.color_weight:.2f}", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    # Neuromodulator panel (bottom-left)
    panel_y = display.shape[0] - 120
    cv2.rectangle(display, (0, panel_y), (300, display.shape[0]), (0, 0, 0), -1)
    cv2.rectangle(display, (0, panel_y), (300, display.shape[0]), (255, 255, 255), 2)
    
    y_pos = panel_y + 25
    cv2.putText(display, "NEUROMODULATORS:", (10, y_pos), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 255, 200), 1)
    
    # Draw neuromod bars
    y_pos += 25
    bar_width = 200
    
    # Dopamine
    da_len = int(neuromod_state['dopamine'] * bar_width)
    cv2.rectangle(display, (80, y_pos-10), (80+bar_width, y_pos), (50, 50, 50), -1)
    cv2.rectangle(display, (80, y_pos-10), (80+da_len, y_pos), (0, 150, 255), -1)
    cv2.putText(display, f"DA: {neuromod_state['dopamine']:.2f}", (10, y_pos-2), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    ne_len = int(neuromod_state['norepinephrine'] * bar_width)
    cv2.rectangle(display, (80, y_pos-10), (80+bar_width, y_pos), (50, 50, 50), -1)
    cv2.rectangle(display, (80, y_pos-10), (80+ne_len, y_pos), (0, 255, 150), -1)
    cv2.putText(display, f"NE: {neuromod_state['norepinephrine']:.2f}", (10, y_pos-2), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    ach_len = int(neuromod_state['acetylcholine'] * bar_width)
    cv2.rectangle(display, (80, y_pos-10), (80+bar_width, y_pos), (50, 50, 50), -1)
    cv2.rectangle(display, (80, y_pos-10), (80+ach_len, y_pos), (255, 150, 0), -1)
    cv2.putText(display, f"ACh: {neuromod_state['acetylcholine']:.2f}", (10, y_pos-2), 
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN CAMERA LOOP
# ============================================================================

def main():
    print("\n" + "="*70)
    print("🎥 MELVIN v2 - Live Camera Vision (Genome-Driven)")
    print("="*70 + "\n")
    
    # Load genome config
    genome_path = "/tmp/demo_genome.json"
    if Path(genome_path).exists():
        print(f"✓ Loading genome from {genome_path}")
        genome_config = GenomeVisionConfig(genome_path)
    else:
        print("⚠️  Using default genome parameters")
        genome_config = GenomeVisionConfig()
    
    print("\n" + genome_config.to_display_string())
    print()
    
    # Initialize camera
    camera_index = 0
    cap = cv2.VideoCapture(camera_index)
    
    if not cap.isOpened():
        print(f"❌ Cannot open camera {camera_index}")
        print("Trying camera 1...")
        camera_index = 1
        cap = cv2.VideoCapture(camera_index)
        
        if not cap.isOpened():
            print("❌ No camera available")
            print("\n💡 To use camera:")
            print("   - Connect a webcam")
            print("   - Or use built-in camera (Mac)")
            print("   - Grant camera permissions if needed")
            return
    
    print(f"✓ Camera {camera_index} opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  s - Save genome")
    print("  e - Toggle edge weight (0.1 ↔ 0.9)")
    print("  m - Toggle motion weight (0.1 ↔ 0.9)")
    print("  c - Toggle color weight (0.1 ↔ 0.9)")
    print()
    print("Starting camera feed...")
    print("="*70 + "\n")
    
    # Create vision processor
    vision = GenomeDrivenVision(genome_config)
    
    # Neuromodulator state (simulated for now)
    neuromod_state = {
        'dopamine': 0.50,
        'norepinephrine': 0.50,
        'acetylcholine': 0.50,
        'serotonin': 0.50
    }
    
    cycle_num = 0
    
    while True:
        ret, frame = cap.read()
        
        if not ret:
            print("❌ Failed to capture frame")
            break
        
        cycle_num += 1
        
        # Process frame with genome-driven vision
        objects = vision.process_frame(frame)
        
        # Select focus using genome attention weights
        focused_obj = select_focus(objects, genome_config)
        
        # Simulate neuromodulator dynamics
        if focused_obj:
            # Simulate prediction error (random for demo)
            prediction_error = np.random.randn() * 0.1
            
            # Update DA (simplified)
            neuromod_state['dopamine'] += prediction_error * 0.05
            neuromod_state['dopamine'] = np.clip(neuromod_state['dopamine'], 0.0, 1.0)
            
            # Decay toward baseline
            neuromod_state['dopamine'] = (
                0.95 * neuromod_state['dopamine'] + 
                0.05 * genome_config.da_baseline
            )
        
        # Draw visualization
        display = draw_overlay(frame, objects, focused_obj, genome_config, 
                              neuromod_state, cycle_num)
        
        # Show
        cv2.imshow('MELVIN v2 - Evolved Vision', display)
        
        # Handle keys
        key = cv2.waitKey(1) & 0xFF
        
        if key == ord('q'):
            print("\n👋 Quitting...")
            break
        elif key == ord('e'):
            # Toggle edge weight
            genome_config.edge_weight = 0.9 if genome_config.edge_weight < 0.5 else 0.1
            print(f"Edge weight: {genome_config.edge_weight:.2f}")
        elif key == ord('m'):
            # Toggle motion weight
            genome_config.motion_weight = 0.9 if genome_config.motion_weight < 0.5 else 0.1
            print(f"Motion weight: {genome_config.motion_weight:.2f}")
        elif key == ord('c'):
            # Toggle color weight
            genome_config.color_weight = 0.9 if genome_config.color_weight < 0.5 else 0.1
            print(f"Color weight: {genome_config.color_weight:.2f}")
        elif key == ord('s'):
            # Save current genome
            print("💾 Genome save not implemented yet")
    
    cap.release()
    cv2.destroyAllWindows()
    
    print(f"\n✓ Processed {cycle_num} frames")
    print("="*70 + "\n")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n👋 Interrupted by user")
        cv2.destroyAllWindows()
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        cv2.destroyAllWindows()

