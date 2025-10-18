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
        """Compute attention scores (Melvin's perception)"""
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
                gray = cv2.cvtColor(patch, cv2.COLOR_BGR2GRAY)
                
                # SALIENCY: Contrast (bottom-up attention)
                saliency = float(np.std(gray)) / 128.0
                
                # GOAL: Motion (top-down - looking for movement)
                goal = 0.3  # Placeholder - would come from active concepts
                
                # CURIOSITY: Edge density (novelty detection)
                edges = cv2.Canny(gray, 50, 150)
                curiosity = float(np.sum(edges > 0)) / (self.patch_size * self.patch_size)
                
                # Melvin's attention formula: F = α·S + β·G + γ·C
                F = 0.45 * saliency + 0.35 * goal + 0.20 * curiosity
                
                scores.append({
                    'x': px, 'y': py,
                    'cx': x + self.patch_size//2,
                    'cy': y + self.patch_size//2,
                    'saliency': saliency,
                    'goal': goal,
                    'curiosity': curiosity,
                    'focus': F
                })
        
        return scores
    
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
        
        # Title
        cv2.putText(overlay, "Melvin's Vision (UCA v1)", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 255), 2)
        
        # Formula
        cv2.putText(overlay, "F = 0.45*S + 0.35*G + 0.20*C", (10, 60),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (200, 200, 200), 2)
        
        # Stats
        cv2.putText(overlay, f"Frame: {self.frame_count}", (10, self.height - 10),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
        
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


