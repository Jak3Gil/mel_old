#!/usr/bin/env python3
"""
MELVIN v2 - Hybrid Camera Vision (Grid + Boxes)

Combines:
- Grid-based patch analysis (sees EVERYTHING in a grid)
- Object-based detection (categorizes specific things with boxes)
- Genome-driven weighting (evolution controls both)

Best of both worlds!
"""

import cv2
import numpy as np
import json
from pathlib import Path

# ============================================================================
# GENOME CONFIGURATION
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
        # Edge detection
        self.edge_threshold = 0.30
        self.edge_weight = 0.30
        
        # Motion detection
        self.motion_sensitivity = 0.50
        self.motion_weight = 0.40
        
        # Color analysis
        self.color_variance_threshold = 0.20
        self.color_weight = 0.30
        
        # Grid parameters (OPTIMIZED FOR SPEED)
        self.patch_size = 64  # Larger patches = fewer patches = faster! (was 32)
        self.min_object_size = 200  # Larger objects only = fewer boxes = faster! (was 100)
        self.max_objects = 5  # Limit number of objects to process
        
        # Attention genes
        self.alpha_saliency = 0.40
        self.beta_relevance = 0.30
        self.gamma_curiosity = 0.20
        
        # System weights (grid vs boxes)
        self.grid_weight = 0.6  # How much to trust grid analysis
        self.box_weight = 0.4   # How much to trust object detection
        
        # Performance options
        self.blur_kernel = 11  # Smaller = faster (was 15)
        self.skip_blur_on_darkness = True  # Skip one blur for speed
    
    def load_from_file(self, path):
        """Load from genome JSON file"""
        with open(path, 'r') as f:
            genome = json.load(f)
        
        # Extract vision module genes
        for module in genome.get('modules', []):
            if module['name'] == 'vision':
                for gene in module['genes']:
                    if hasattr(self, gene['key']):
                        setattr(self, gene['key'], gene['value'])
    
    def to_display_string(self):
        """Format for on-screen display"""
        return f"""GENOME VISION:
Grid: {self.patch_size}px patches (weight={self.grid_weight:.2f})
Box:  edge={self.edge_weight:.2f} motion={self.motion_weight:.2f} color={self.color_weight:.2f} (weight={self.box_weight:.2f})"""

# ============================================================================
# HYBRID VISION PROCESSOR
# ============================================================================

class HybridVisionProcessor:
    """Processes frames with both grid and box detection"""
    
    def __init__(self, genome_config):
        self.config = genome_config
        self.prev_frame = None
        self.object_id_counter = 0
        self.frame_count = 0
        
        # Focus history
        self.focus_history = []
        
    def process_frame(self, frame):
        """
        Process frame with BOTH grid and box methods
        
        Returns: {
            'patches': list of grid patches with scores,
            'objects': list of detected objects with boxes,
            'focus': best focus target (either patch or object)
        }
        """
        self.frame_count += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        h, w = frame.shape[:2]
        
        # METHOD 1: GRID ANALYSIS (sees everything)
        patches = self.analyze_grid(frame, gray, h, w)
        
        # METHOD 2: OBJECT DETECTION (categorizes specific things)
        objects = self.detect_objects(frame, gray)
        
        # HYBRID FUSION: Combine both methods
        focus = self.select_hybrid_focus(patches, objects)
        
        # Update history
        self.prev_frame = gray.copy()
        
        return {
            'patches': patches,
            'objects': objects,
            'focus': focus
        }
    
    def analyze_grid(self, frame, gray, h, w):
        """Grid-based patch analysis (old system)"""
        patches = []
        
        patch_size = self.config.patch_size
        grid_h = h // patch_size
        grid_w = w // patch_size
        
        for py in range(grid_h):
            for px in range(grid_w):
                y = py * patch_size
                x = px * patch_size
                
                if y + patch_size > h or x + patch_size > w:
                    continue
                
                # Extract patch
                patch_color = frame[y:y+patch_size, x:x+patch_size]
                patch_gray = gray[y:y+patch_size, x:x+patch_size]
                
                # Compute features
                edge_score = self.compute_patch_edges(patch_gray)
                motion_score = self.compute_patch_motion(x, y, patch_size)
                color_score = self.compute_patch_color(patch_color)
                
                # Genome-weighted saliency
                saliency = (
                    self.config.edge_weight * edge_score +
                    self.config.motion_weight * motion_score +
                    self.config.color_weight * color_score
                )
                
                patches.append({
                    'type': 'patch',
                    'grid_x': px,
                    'grid_y': py,
                    'x': x,
                    'y': y,
                    'w': patch_size,
                    'h': patch_size,
                    'cx': x + patch_size // 2,
                    'cy': y + patch_size // 2,
                    'edge_score': edge_score,
                    'motion_score': motion_score,
                    'color_score': color_score,
                    'saliency': saliency,
                    'source': 'grid'
                })
        
        return patches
    
    def detect_objects(self, frame, gray):
        """Object detection with bounding boxes (new system) - OPTIMIZED"""
        # Threshold to find regions
        _, thresh = cv2.threshold(gray, 100, 255, cv2.THRESH_BINARY)
        
        # Find contours
        contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        
        # OPTIMIZATION: Pre-filter by area and sort by size
        valid_contours = []
        for contour in contours:
            area = cv2.contourArea(contour)
            if area > self.config.min_object_size:
                valid_contours.append((area, contour))
        
        # Sort by area (largest first) and limit
        valid_contours.sort(reverse=True, key=lambda x: x[0])
        valid_contours = valid_contours[:self.config.max_objects]  # LIMIT!
        
        objects = []
        for area, contour in valid_contours:
            x, y, w, h = cv2.boundingRect(contour)
            
            # Extract ROI
            roi_gray = gray[y:y+h, x:x+w]
            roi_color = frame[y:y+h, x:x+w]
            
            # Compute features
            edge_score = self.compute_object_edges(roi_gray)
            motion_score = 0.0  # Placeholder for now
            color_score = self.compute_object_color(roi_color)
            
            # Genome-weighted saliency
            saliency = (
                self.config.edge_weight * edge_score +
                self.config.motion_weight * motion_score +
                self.config.color_weight * color_score
            )
            
            objects.append({
                'type': 'object',
                'id': self.object_id_counter,
                'x': x,
                'y': y,
                'w': w,
                'h': h,
                'cx': x + w // 2,
                'cy': y + h // 2,
                'area': area,
                'edge_score': edge_score,
                'motion_score': motion_score,
                'color_score': color_score,
                'saliency': saliency,
                'source': 'box'
            })
            
            self.object_id_counter += 1
        
        return objects
    
    def compute_patch_edges(self, patch_gray):
        """Edge density for grid patch"""
        if patch_gray.size == 0:
            return 0.0
        
        threshold1 = int(self.config.edge_threshold * 100)
        edges = cv2.Canny(patch_gray, threshold1, threshold1 * 2)
        return np.sum(edges > 0) / edges.size
    
    def compute_object_edges(self, roi_gray):
        """Edge density for object ROI"""
        if roi_gray.size == 0:
            return 0.0
        
        threshold1 = int(self.config.edge_threshold * 100)
        edges = cv2.Canny(roi_gray, threshold1, threshold1 * 2)
        return np.sum(edges > 0) / edges.size
    
    def compute_patch_motion(self, x, y, size):
        """Motion for grid patch (placeholder)"""
        # In full version: optical flow
        return 0.5  # Placeholder
    
    def compute_patch_color(self, patch_color):
        """Color variance for grid patch"""
        if patch_color.size == 0:
            return 0.0
        
        std_dev = np.std(patch_color)
        normalized = std_dev / 128.0
        
        if normalized < self.config.color_variance_threshold:
            return 0.0
        
        return normalized
    
    def compute_object_color(self, roi_color):
        """Color variance for object ROI"""
        if roi_color.size == 0:
            return 0.0
        
        std_dev = np.std(roi_color)
        normalized = std_dev / 128.0
        
        if normalized < self.config.color_variance_threshold:
            return 0.0
        
        return normalized
    
    def select_hybrid_focus(self, patches, objects):
        """Select best focus using BOTH grid and box information"""
        candidates = []
        
        # Add grid patches (weighted by grid_weight)
        for patch in patches:
            patch['weighted_score'] = patch['saliency'] * self.config.grid_weight
            candidates.append(patch)
        
        # Add objects (weighted by box_weight)
        for obj in objects:
            obj['weighted_score'] = obj['saliency'] * self.config.box_weight
            candidates.append(obj)
        
        if not candidates:
            return None
        
        # Select highest weighted score
        best = max(candidates, key=lambda c: c['weighted_score'])
        
        # Track focus
        self.focus_history.append((best['cx'], best['cy']))
        if len(self.focus_history) > 30:
            self.focus_history.pop(0)
        
        return best

# ============================================================================
# VISUALIZATION
# ============================================================================

def draw_hybrid_overlay(frame, data, genome_config, frame_count):
    """Draw hybrid visualization with grid AND boxes"""
    display = frame.copy()
    h, w = display.shape[:2]
    
    patches = data['patches']
    objects = data['objects']
    focus = data['focus']
    
    # LAYER 1: Smooth gradient heatmap (darken low attention, brighten high)
    # Create darkening overlay first
    dark_overlay = np.zeros_like(frame, dtype=np.uint8)
    
    # Create gradient heatmap
    heatmap = np.zeros_like(frame, dtype=np.float32)
    
    for patch in patches:
        intensity = patch['saliency']  # 0.0 to 1.0
        
        # Color based on attention intensity
        # Low attention → dark/blue
        # Medium attention → cyan
        # High attention → yellow/bright
        if intensity < 0.3:
            # Low attention: darken with blue tint
            color = (int(40 * intensity / 0.3), 
                    int(20 * intensity / 0.3), 
                    int(10 * intensity / 0.3))
        elif intensity < 0.6:
            # Medium attention: cyan/green
            t = (intensity - 0.3) / 0.3
            color = (int(80 + 80 * t), 
                    int(120 + 80 * t),
                    int(40 + 40 * t))
        else:
            # High attention: yellow/bright
            t = (intensity - 0.6) / 0.4
            color = (int(180 + 75 * t),
                    int(200 + 55 * t),
                    int(100 + 100 * t))
        
        cv2.rectangle(heatmap,
                     (patch['x'], patch['y']),
                     (patch['x'] + patch['w'], patch['y'] + patch['h']),
                     color, -1)
    
    # Convert to uint8
    heatmap = heatmap.astype(np.uint8)
    
    # Apply Gaussian blur for smooth gradient (CONFIGURABLE)
    kernel_size = genome_config.blur_kernel
    heatmap = cv2.GaussianBlur(heatmap, (kernel_size, kernel_size), 0)
    
    # Darken uninteresting areas more aggressively
    # Create darkening mask based on inverse attention
    darkness = np.zeros_like(frame, dtype=np.float32)
    for patch in patches:
        # Low saliency → more darkness
        darkness_factor = 1.0 - patch['saliency']  # Inverse
        dark_intensity = int(darkness_factor * 80)  # Max 80 darkness
        
        cv2.rectangle(darkness,
                     (patch['x'], patch['y']),
                     (patch['x'] + patch['w'], patch['y'] + patch['h']),
                     (dark_intensity, dark_intensity, dark_intensity), -1)
    
    darkness = darkness.astype(np.uint8)
    
    # OPTIONAL: Skip blur on darkness for speed
    if not genome_config.skip_blur_on_darkness:
        darkness = cv2.GaussianBlur(darkness, (kernel_size, kernel_size), 0)
    
    # Apply darkening first (subtract darkness)
    display = cv2.subtract(display, darkness)
    
    # Then add colorful highlights
    display = cv2.addWeighted(display, 0.65, heatmap, 0.35, 0)
    
    # LAYER 2: Grid lines (subtle)
    patch_size = genome_config.patch_size
    for y in range(0, h, patch_size):
        cv2.line(display, (0, y), (w, y), (60, 60, 60), 1)
    for x in range(0, w, patch_size):
        cv2.line(display, (x, 0), (x, h), (60, 60, 60), 1)
    
    # LAYER 3: Object boxes (white outlines)
    for obj in objects:
        x, y, w, h = obj['x'], obj['y'], obj['w'], obj['h']
        
        # White box for detected objects
        cv2.rectangle(display, (x, y), (x+w, y+h), (255, 255, 255), 2)
        
        # Saliency score
        text = f"S:{obj['saliency']:.2f}"
        cv2.putText(display, text, (x, y-5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    # LAYER 4: Focus indicator (yellow crosshair + circle)
    if focus:
        cx, cy = focus['cx'], focus['cy']
        
        # Large yellow circle
        cv2.circle(display, (cx, cy), 50, (0, 255, 255), 3)
        
        # Crosshair
        cv2.drawMarker(display, (cx, cy), (0, 255, 255),
                      cv2.MARKER_CROSS, 60, 3)
        
        # Focus label
        source_label = f"[{focus['source'].upper()}]"
        focus_text = f"FOCUS {source_label} F={focus['weighted_score']:.2f}"
        cv2.putText(display, focus_text, (cx - 100, cy - 70),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
        
        # Score breakdown
        y_offset = cy + 70
        cv2.putText(display, f"E:{focus['edge_score']:.2f}", (cx - 80, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 200, 255), 1)
        cv2.putText(display, f"M:{focus['motion_score']:.2f}", (cx - 20, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (100, 255, 200), 1)
        cv2.putText(display, f"C:{focus['color_score']:.2f}", (cx + 40, y_offset),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 200, 100), 1)
    
    # Info panel (top-left)
    panel_h = 120
    cv2.rectangle(display, (0, 0), (500, panel_h), (0, 0, 0), -1)
    cv2.rectangle(display, (0, 0), (500, panel_h), (255, 255, 255), 2)
    
    y_pos = 25
    cv2.putText(display, "MELVIN v2 - Hybrid Vision (Grid + Boxes)", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    
    y_pos += 25
    cv2.putText(display, f"Frame: {frame_count}", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 20
    cv2.putText(display, f"Grid: {len(patches)} patches | Boxes: {len(objects)} objects", (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 25
    cv2.putText(display, genome_config.to_display_string().split('\n')[1], (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 200, 255), 1)
    
    y_pos += 15
    cv2.putText(display, genome_config.to_display_string().split('\n')[2], (10, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 200, 255), 1)
    
    # Legend (bottom-right)
    legend_y = h - 80
    cv2.rectangle(display, (w-250, legend_y), (w, h), (0, 0, 0), -1)
    cv2.rectangle(display, (w-250, legend_y), (w, h), (255, 255, 255), 1)
    
    y_pos = legend_y + 20
    cv2.putText(display, "LEGEND:", (w-240, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-240, y_pos-10), (w-220, y_pos), (60, 60, 60), -1)
    cv2.putText(display, "Grid overlay", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.rectangle(display, (w-240, y_pos-10), (w-220, y_pos), (255, 255, 255), 2)
    cv2.putText(display, "Object boxes", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    y_pos += 18
    cv2.circle(display, (w-230, y_pos-5), 8, (0, 255, 255), 2)
    cv2.putText(display, "Focus (hybrid)", (w-210, y_pos),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)
    
    return display

# ============================================================================
# MAIN LOOP
# ============================================================================

def main():
    import time
    
    print("\n" + "="*70)
    print("🎥 MELVIN v2 - Hybrid Camera Vision (Grid + Boxes) - FAST MODE")
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
    print("⚡ PERFORMANCE OPTIMIZATIONS:")
    print(f"  • Patch size: {genome_config.patch_size}×{genome_config.patch_size} (larger = faster)")
    print(f"  • Max objects: {genome_config.max_objects} (limited for speed)")
    print(f"  • Min object size: {genome_config.min_object_size}px")
    print(f"  • Blur kernel: {genome_config.blur_kernel}×{genome_config.blur_kernel}")
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
            return
    
    print(f"✓ Camera {camera_index} opened")
    print()
    print("CONTROLS:")
    print("  q - Quit")
    print("  e - Toggle edge weight")
    print("  m - Toggle motion weight")
    print("  c - Toggle color weight")
    print("  g - Toggle grid/box balance")
    print("  f - Toggle FPS display")
    print()
    print("Starting hybrid vision...")
    print("="*70 + "\n")
    
    # Create vision processor
    vision = HybridVisionProcessor(genome_config)
    
    # FPS tracking
    fps_start_time = time.time()
    fps_frame_count = 0
    current_fps = 0.0
    show_fps = True
    
    try:
        while True:
            loop_start = time.time()
            ret, frame = cap.read()
            
            if not ret:
                print("❌ Failed to capture frame")
                break
            
            # Process with BOTH grid and boxes
            data = vision.process_frame(frame)
            
            # Draw hybrid visualization
            display = draw_hybrid_overlay(frame, data, genome_config, vision.frame_count)
            
            # Calculate FPS
            fps_frame_count += 1
            elapsed = time.time() - fps_start_time
            if elapsed > 0.5:  # Update every 0.5 seconds
                current_fps = fps_frame_count / elapsed
                fps_start_time = time.time()
                fps_frame_count = 0
            
            # Add FPS overlay
            if show_fps:
                fps_text = f"FPS: {current_fps:.1f}"
                cv2.putText(display, fps_text, (display.shape[1] - 120, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                
                # Add performance stats
                grid_count = len(data['patches'])
                box_count = len(data['objects'])
                stats_text = f"Grid:{grid_count} Box:{box_count}"
                cv2.putText(display, stats_text, (display.shape[1] - 180, 60),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (150, 255, 150), 1)
            
            # Show
            cv2.imshow('MELVIN v2 - Hybrid Vision', display)
            
            # Handle keys
            key = cv2.waitKey(1) & 0xFF
            
            if key == ord('q'):
                print("\n👋 Quitting...")
                break
            elif key == ord('e'):
                genome_config.edge_weight = 0.9 if genome_config.edge_weight < 0.5 else 0.1
                print(f"Edge weight: {genome_config.edge_weight:.2f}")
            elif key == ord('m'):
                genome_config.motion_weight = 0.9 if genome_config.motion_weight < 0.5 else 0.1
                print(f"Motion weight: {genome_config.motion_weight:.2f}")
            elif key == ord('c'):
                genome_config.color_weight = 0.9 if genome_config.color_weight < 0.5 else 0.1
                print(f"Color weight: {genome_config.color_weight:.2f}")
            elif key == ord('g'):
                # Toggle grid vs box preference
                if genome_config.grid_weight > 0.5:
                    genome_config.grid_weight = 0.3
                    genome_config.box_weight = 0.7
                    print("Box-focused mode (boxes weighted higher)")
                else:
                    genome_config.grid_weight = 0.7
                    genome_config.box_weight = 0.3
                    print("Grid-focused mode (patches weighted higher)")
            elif key == ord('f'):
                # Toggle FPS display
                show_fps = not show_fps
                print(f"FPS display: {'ON' if show_fps else 'OFF'}")
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
    
    print(f"\n✓ Processed {vision.frame_count} frames")
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
