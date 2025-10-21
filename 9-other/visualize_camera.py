#!/usr/bin/env python3
"""
Melvin UCA Camera Visualizer
Shows what Melvin sees from your actual camera with real-time attention overlay
"""

import cv2
import numpy as np
import sys

class MelvinCameraVision:
    def __init__(self, camera_id=0):
        self.patch_size = 64  # Larger patches for high-res cameras
        self.focus_history = []
        self.frame_count = 0
        
        # NEW: Visual feature tracking for diversity
        self.color_history = []  # Track recent dominant colors
        self.shape_history = []  # Track recent shapes (edgy vs smooth)
        self.feature_stats = {}  # Track feature saturation
        
        # Motion detection
        self.prev_frame = None  # Previous frame for motion detection
        
        # Try to open camera
        print(f"🔍 Opening camera {camera_id}...")
        self.cap = cv2.VideoCapture(camera_id)
        if not self.cap.isOpened():
            raise RuntimeError(f"Cannot open camera {camera_id}")
        
        # Read one test frame to verify
        ret, test_frame = self.cap.read()
        if not ret or test_frame is None:
            raise RuntimeError(f"Cannot read from camera {camera_id}")
        
        # Get actual resolution
        self.height, self.width = test_frame.shape[:2]
        
        print(f"✅ Camera opened: {self.width}x{self.height}")
        print(f"   Frame shape: {test_frame.shape}")
        
        # Adjust patch size based on resolution
        if self.width > 1280:
            self.patch_size = 64
        else:
            self.patch_size = 32
        print(f"   Using {self.patch_size}×{self.patch_size} patches")
    
    def compute_patch_scores(self, img):
        """Compute attention scores with DIVERSITY SEEKING (Melvin's perception)"""
        scores = []
        
        # Convert to grayscale for motion detection
        gray_frame = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        
        # Compute motion map
        motion_map = None
        if self.prev_frame is not None:
            # Frame difference for motion
            frame_diff = cv2.absdiff(gray_frame, self.prev_frame)
            # Threshold to get binary motion
            _, motion_map = cv2.threshold(frame_diff, 25, 255, cv2.THRESH_BINARY)
        
        # Store current frame for next iteration
        self.prev_frame = gray_frame.copy()
        
        grid_h = self.height // self.patch_size
        grid_w = self.width // self.patch_size
        
        for py in range(grid_h):
            for px in range(grid_w):
                y = py * self.patch_size
                x = px * self.patch_size
                
                if y + self.patch_size > self.height or x + self.patch_size > self.width:
                    continue
                
                patch = img[y:y+self.patch_size, x:x+self.patch_size]
                gray = cv2.cvtColor(patch, cv2.COLOR_BGR2GRAY)
                
                # Extract visual features
                avg_color = np.mean(patch, axis=(0,1))
                dominant_color = self._classify_color(avg_color)
                
                edges = cv2.Canny(gray, 50, 150)
                edge_density = float(np.sum(edges > 0)) / (self.patch_size * self.patch_size)
                shape_type = 'edgy' if edge_density > 0.3 else 'smooth'
                
                # SALIENCY: Contrast (bottom-up attention)
                saliency = float(np.std(gray)) / 128.0
                
                # GOAL: Motion (top-down - looking for movement)
                if motion_map is not None:
                    motion_patch = motion_map[y:y+self.patch_size, x:x+self.patch_size]
                    motion_density = float(np.sum(motion_patch > 0)) / (self.patch_size * self.patch_size)
                    goal = motion_density * 2.0  # Scale up motion importance
                else:
                    goal = 0.0  # No motion on first frame
                
                # CURIOSITY: Edge density (novelty detection)
                curiosity = edge_density
                
                # DIVERSITY: Visual variety seeking (NEW!)
                diversity = self._compute_diversity(dominant_color, shape_type)
                
                # Melvin's attention formula WITH DIVERSITY:
                # F = S + G + C + D (pure sum, no weighting)
                F = saliency + goal + curiosity + diversity
                
                scores.append({
                    'x': px, 'y': py,
                    'cx': x + self.patch_size//2,
                    'cy': y + self.patch_size//2,
                    'saliency': saliency,
                    'goal': goal,
                    'curiosity': curiosity,
                    'diversity': diversity,
                    'focus': F,
                    'color': dominant_color,
                    'shape': shape_type
                })
        
        return scores
    
    def _classify_color(self, avg_bgr):
        """Classify dominant color"""
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
    
    def _compute_diversity(self, color, shape):
        """Compute diversity score - KEY ANTI-STICKING!"""
        diversity = 0.0
        
        # Track recent colors
        recent_colors = self.color_history[-20:] if len(self.color_history) > 20 else self.color_history
        
        if len(recent_colors) > 10:
            color_count = recent_colors.count(color)
            color_ratio = color_count / len(recent_colors)
            
            # If this color is over-represented (>60% of recent)
            if color_ratio > 0.6:
                diversity -= 0.25  # SUPPRESS repetitive color!
            # If this color is under-represented (<20% of recent)
            elif color_ratio < 0.2:
                diversity += 0.25  # BOOST novel color!
        
        # Track recent shapes  
        recent_shapes = self.shape_history[-20:] if len(self.shape_history) > 20 else self.shape_history
        
        if len(recent_shapes) > 10:
            shape_count = recent_shapes.count(shape)
            shape_ratio = shape_count / len(recent_shapes)
            
            # Shape diversity
            if shape_ratio > 0.6:
                diversity -= 0.15  # Too much of same shape type
            elif shape_ratio < 0.2:
                diversity += 0.15  # Novel shape type!
        
        return np.clip(diversity, -0.4, 0.4)  # Cap at ±0.4
    
    def _print_context_status(self, scores):
        """Print context and diversity status to console"""
        print("\n" + "="*70)
        print(f"🧠 CONTEXT STATUS - Frame {self.frame_count}")
        print("="*70)
        
        # Visual diversity
        if len(self.color_history) > 10:
            recent_colors = self.color_history[-20:]
            color_counts = {}
            for c in recent_colors:
                color_counts[c] = color_counts.get(c, 0) + 1
            
            print("\n📊 VISUAL DIVERSITY:")
            for color, count in sorted(color_counts.items(), key=lambda x: -x[1]):
                ratio = count / len(recent_colors)
                bar = '#' * int(ratio * 40)
                status = "SATURATED!" if ratio > 0.6 else "Normal"
                print(f"  {color:10s}: {int(ratio*100):3d}% {bar:40s} {status}")
        
        # Best current focus
        if scores:
            best = max(scores, key=lambda s: s['focus'])
            print(f"\n🎯 CURRENT FOCUS:")
            print(f"  Position: ({best['cx']}, {best['cy']})")
            print(f"  Color:    {best.get('color', 'unknown')}")
            print(f"  Shape:    {best.get('shape', 'unknown')}")
            print(f"  Scores:   S={best['saliency']:.2f} G={best['goal']:.2f} " +
                  f"C={best['curiosity']:.2f} D={best.get('diversity', 0):.2f}")
            print(f"  Focus:    F={best['focus']:.2f}")
        
        # Focus trail
        print(f"\n📍 FOCUS HISTORY: {len(self.focus_history)} recent shifts")
        
        print("="*70 + "\n")
    
    def draw_melvin_view(self, img, scores):
        """Draw what Melvin perceives"""
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
        
        # Find and draw focus (highest attention)
        if scores:
            best = max(scores, key=lambda s: s['focus'])
            cx, cy = best['cx'], best['cy']
            
            # Crosshair on focus
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
            
            # Track focus position
            self.focus_history.append((cx, cy))
            if len(self.focus_history) > 30:
                self.focus_history.pop(0)
            
            # Track focus FEATURES for diversity (NEW!)
            self.color_history.append(best['color'])
            self.shape_history.append(best['shape'])
            if len(self.color_history) > 50:
                self.color_history.pop(0)
            if len(self.shape_history) > 50:
                self.shape_history.pop(0)
            
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
        
        # Title
        cv2.putText(overlay, "Melvin's Vision (UCA v1) - DIVERSITY SEEKING", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
        
        # Formula  
        cv2.putText(overlay, "F = S + G + C + D", (10, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 200), 2)
        cv2.putText(overlay, "(Pure sum, no weighting)", (10, 85),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (150, 150, 150), 1)
        
        # Stats - show feature diversity
        cv2.putText(overlay, f"Frame: {self.frame_count}", (10, self.height - 50),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
        
        # Show color diversity status
        if len(self.color_history) > 10:
            recent = self.color_history[-20:]
            color_counts = {}
            for c in set(recent):
                color_counts[c] = recent.count(c)
            
            y_pos = self.height - 25
            status_text = "Visual: "
            for color, count in sorted(color_counts.items(), key=lambda x: -x[1])[:3]:
                ratio = count / len(recent)
                status_text += f"{color}:{int(ratio*100)}% "
            
            cv2.putText(overlay, status_text, (10, y_pos),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (150, 255, 150), 1)
        
        return overlay
    
    def run(self):
        """Run live camera visualization"""
        print("\n╔═══════════════════════════════════════════════════════════╗")
        print("║  👁️  MELVIN CAMERA VISION - Real-Time                   ║")
        print("╚═══════════════════════════════════════════════════════════╝\n")
        print("Showing what Melvin sees through your camera...")
        print("\nControls:")
        print("  'q' - Quit")
        print("  'p' - Pause/Resume")
        print("  's' - Save snapshot")
        print("  SPACE - Single step (when paused)\n")
        print("Look around! Melvin's attention will follow interesting things.\n")
        
        paused = False
        
        try:
            while True:
                if not paused:
                    ret, frame = self.cap.read()
                    if not ret or frame is None:
                        print("❌ Failed to grab frame")
                        print("   Trying to reconnect...")
                        self.cap.release()
                        import time
                        time.sleep(0.5)
                        self.cap = cv2.VideoCapture(0)
                        continue
                    
                    # Check frame validity
                    if frame.size == 0:
                        print("❌ Empty frame received")
                        continue
                    
                    # Compute attention
                    scores = self.compute_patch_scores(frame)
                    
                    # Draw Melvin's view
                    display = self.draw_melvin_view(frame, scores)
                    
                    self.frame_count += 1
                    
                    # Print context info periodically (every 60 frames = ~2 seconds)
                    if self.frame_count % 60 == 0:
                        self._print_context_status(scores)
                else:
                    # Reuse last frame when paused
                    pass
                
                # Show
                cv2.imshow("Melvin's Perception", display)
                
                # Handle keys
                key = cv2.waitKey(1 if not paused else 0) & 0xFF
                
                if key == ord('q'):
                    break
                elif key == ord('p'):
                    paused = not paused
                    status = "PAUSED" if paused else "RUNNING"
                    print(f"  {status}")
                elif key == ord('s'):
                    filename = f"melvin_view_{self.frame_count}.png"
                    cv2.imwrite(filename, display)
                    print(f"  💾 Saved {filename}")
                elif key == ord(' ') and paused:
                    # Single step
                    paused = False
                    ret, frame = self.cap.read()
                    if ret:
                        scores = self.compute_patch_scores(frame)
                        display = self.draw_melvin_view(frame, scores)
                        self.frame_count += 1
                    paused = True
        
        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted by user")
        
        finally:
            self.cap.release()
            cv2.destroyAllWindows()
            
            print("\n✅ Camera session complete!")
            print(f"   Processed {self.frame_count} frames")
            print(f"   Focus shifts: {len(self.focus_history)}")

def main():
    try:
        print("\n🧠 Melvin UCA Camera Vision")
        print("=" * 60)
        
        # Try to open camera
        viz = MelvinCameraVision(camera_id=0)
        viz.run()
        
    except RuntimeError as e:
        print(f"\n❌ Error: {e}")
        print("\nTroubleshooting:")
        print("  • Make sure your camera is connected")
        print("  • Check if another app is using the camera")
        print("  • Try a different camera_id (0, 1, 2...)")
        print("\n  Or run the test visualization:")
        print("  $ python3 visualize_melvin.py")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())


