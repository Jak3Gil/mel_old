#!/usr/bin/env python3
"""
Melvin UCA Visualizer
Shows what Melvin sees, thinks, and focuses on in real-time
"""

import cv2
import numpy as np
import subprocess
import json
import os
import time
from pathlib import Path

class MelvinVisualizer:
    def __init__(self):
        self.width = 640
        self.height = 480
        self.patch_size = 32
        self.current_frame = 0
        self.focus_history = []
        self.thoughts = []
        
    def create_test_scene(self, frame_num):
        """Create interesting test scenes"""
        img = np.ones((self.height, self.width, 3), dtype=np.uint8) * 128
        
        # Scene 1: Moving objects
        if frame_num < 50:
            # Red square moving right
            x = (frame_num * 10) % self.width
            cv2.rectangle(img, (x, 100), (x+80, 180), (0, 0, 255), -1)
            
            # Blue circle stationary
            cv2.circle(img, (320, 350), 50, (255, 0, 0), -1)
            
        # Scene 2: Shape changes
        elif frame_num < 100:
            # Pulsing green square
            size = 40 + int(20 * np.sin(frame_num * 0.2))
            cv2.rectangle(img, (200, 200), (200+size, 200+size), (0, 255, 0), -1)
            
            # Yellow triangle
            pts = np.array([[400, 100], [350, 200], [450, 200]], np.int32)
            cv2.fillPoly(img, [pts], (0, 255, 255))
            
        # Scene 3: Multiple objects
        else:
            # Random colored circles
            np.random.seed(frame_num)
            for i in range(5):
                x = np.random.randint(50, self.width-50)
                y = np.random.randint(50, self.height-50)
                color = tuple(np.random.randint(0, 255, 3).tolist())
                cv2.circle(img, (x, y), 30, color, -1)
        
        return img
    
    def draw_grid(self, img):
        """Draw patch grid overlay"""
        overlay = img.copy()
        
        # Draw grid
        for y in range(0, self.height, self.patch_size):
            cv2.line(overlay, (0, y), (self.width, y), (100, 100, 100), 1)
        for x in range(0, self.width, self.patch_size):
            cv2.line(overlay, (x, 0), (x, self.height), (100, 100, 100), 1)
        
        # Blend
        return cv2.addWeighted(img, 0.7, overlay, 0.3, 0)
    
    def compute_patch_scores(self, img):
        """Compute attention scores for visualization"""
        scores = []
        
        grid_h = self.height // self.patch_size
        grid_w = self.width // self.patch_size
        
        for py in range(grid_h):
            for px in range(grid_w):
                y = py * self.patch_size
                x = px * self.patch_size
                
                # Sample patch
                patch = img[y:y+self.patch_size, x:x+self.patch_size]
                
                # Saliency: variance
                gray = cv2.cvtColor(patch, cv2.COLOR_BGR2GRAY)
                saliency = float(np.std(gray)) / 128.0
                
                # Goal: placeholder (would come from active concepts)
                goal = 0.3
                
                # Curiosity: edge density
                edges = cv2.Canny(gray, 50, 150)
                curiosity = float(np.sum(edges > 0)) / (self.patch_size * self.patch_size)
                
                # Combined focus score
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
    
    def draw_attention_heatmap(self, img, scores):
        """Draw attention heatmap"""
        overlay = np.zeros_like(img)
        
        for s in scores:
            intensity = int(s['focus'] * 255)
            color = (0, intensity, intensity)  # Cyan for attention
            
            x = s['x'] * self.patch_size
            y = s['y'] * self.patch_size
            
            cv2.rectangle(overlay, 
                         (x, y), 
                         (x + self.patch_size, y + self.patch_size),
                         color, -1)
        
        return cv2.addWeighted(img, 0.6, overlay, 0.4, 0)
    
    def draw_focus(self, img, scores):
        """Draw current focus (highest attention)"""
        if not scores:
            return img
        
        # Find highest focus
        best = max(scores, key=lambda s: s['focus'])
        
        # Draw crosshair
        cx, cy = best['cx'], best['cy']
        size = 40
        
        # Crosshair
        cv2.line(img, (cx - size, cy), (cx + size, cy), (0, 255, 255), 3)
        cv2.line(img, (cx, cy - size), (cx, cy + size), (0, 255, 255), 3)
        cv2.circle(img, (cx, cy), size, (0, 255, 255), 3)
        
        # Label
        label = f"F={best['focus']:.2f}"
        cv2.putText(img, label, (cx + 50, cy), 
                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        
        # Store focus
        self.focus_history.append((cx, cy))
        if len(self.focus_history) > 20:
            self.focus_history.pop(0)
        
        # Draw focus trail
        for i in range(1, len(self.focus_history)):
            cv2.line(img, self.focus_history[i-1], self.focus_history[i],
                    (0, 200, 200), 2)
        
        return img, best
    
    def draw_scores_panel(self, best_score):
        """Draw score breakdown panel"""
        panel = np.ones((200, 300, 3), dtype=np.uint8) * 30
        
        y = 30
        cv2.putText(panel, "Attention Scores:", (10, y),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        y += 40
        # Saliency bar
        s_width = int(best_score['saliency'] * 250)
        cv2.rectangle(panel, (10, y), (10 + s_width, y+20), (0, 255, 0), -1)
        cv2.putText(panel, f"S: {best_score['saliency']:.2f}", (10, y+15),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        y += 40
        # Goal bar
        g_width = int(best_score['goal'] * 250)
        cv2.rectangle(panel, (10, y), (10 + g_width, y+20), (255, 255, 0), -1)
        cv2.putText(panel, f"G: {best_score['goal']:.2f}", (10, y+15),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        y += 40
        # Curiosity bar
        c_width = int(best_score['curiosity'] * 250)
        cv2.rectangle(panel, (10, y), (10 + c_width, y+20), (255, 0, 255), -1)
        cv2.putText(panel, f"C: {best_score['curiosity']:.2f}", (10, y+15),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        y += 40
        # Combined focus
        f_width = int(best_score['focus'] * 250)
        cv2.rectangle(panel, (10, y), (10 + f_width, y+20), (0, 255, 255), -1)
        cv2.putText(panel, f"F: {best_score['focus']:.2f}", (10, y+15),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        
        return panel
    
    def draw_thought_panel(self, frame_num):
        """Draw thought/reasoning panel"""
        panel = np.ones((200, 300, 3), dtype=np.uint8) * 30
        
        y = 30
        cv2.putText(panel, "Melvin's Thoughts:", (10, y),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        y += 40
        # Simulate thoughts based on what's visible
        if frame_num < 50:
            thought = "Moving object"
            cv2.putText(panel, thought, (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
            y += 30
            cv2.putText(panel, "detected!", (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)
        elif frame_num < 100:
            thought = "Shape changing"
            cv2.putText(panel, thought, (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            y += 30
            cv2.putText(panel, "Curious!", (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 255), 1)
        else:
            thought = "Multiple objects"
            cv2.putText(panel, thought, (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
            y += 30
            cv2.putText(panel, "Exploring...", (10, y),
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
        
        y += 50
        cv2.putText(panel, f"Frame: {frame_num}", (10, y),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (150, 150, 150), 1)
        
        return panel
    
    def create_visualization(self, frame_num):
        """Create complete visualization"""
        # Generate scene
        img = self.create_test_scene(frame_num)
        
        # Compute attention scores
        scores = self.compute_patch_scores(img)
        
        # Create visualizations
        img_grid = self.draw_grid(img.copy())
        img_heatmap = self.draw_attention_heatmap(img.copy(), scores)
        img_focus, best_score = self.draw_focus(img.copy(), scores)
        
        # Create panels
        score_panel = self.draw_scores_panel(best_score)
        thought_panel = self.draw_thought_panel(frame_num)
        
        # Combine into layout
        # Top row: original | grid | heatmap
        top_row = np.hstack([
            cv2.resize(img, (300, 225)),
            cv2.resize(img_grid, (300, 225)),
            cv2.resize(img_heatmap, (300, 225))
        ])
        
        # Middle row: focused view
        middle_row = cv2.resize(img_focus, (900, 450))
        
        # Bottom row: panels
        bottom_row = np.hstack([
            np.ones((200, 300, 3), dtype=np.uint8) * 30,  # Spacer
            score_panel,
            thought_panel
        ])
        
        # Add labels
        cv2.putText(top_row, "Input", (10, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(top_row, "Patches", (310, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(top_row, "Attention", (610, 30),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        cv2.putText(middle_row, "Melvin's Focus", (10, 40),
                   cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 255), 3)
        
        # Combine all
        display = np.vstack([top_row, middle_row, bottom_row])
        
        return display
    
    def run(self, num_frames=200):
        """Run visualization"""
        print("\n╔═══════════════════════════════════════════════════════════╗")
        print("║  👁️  MELVIN VISION - Real-Time Visualization            ║")
        print("╚═══════════════════════════════════════════════════════════╝\n")
        print("Showing what Melvin sees and thinks...")
        print("Press 'q' to quit, 'p' to pause, SPACE to step\n")
        
        paused = False
        
        for frame_num in range(num_frames):
            # Create visualization
            display = self.create_visualization(frame_num)
            
            # Show
            cv2.imshow("Melvin's Perception", display)
            
            # Handle input
            if paused:
                key = cv2.waitKey(0) & 0xFF
                if key == ord('p'):
                    paused = False
                elif key == ord('q'):
                    break
                elif key == ord(' '):
                    continue
            else:
                key = cv2.waitKey(50) & 0xFF
                if key == ord('q'):
                    break
                elif key == ord('p'):
                    paused = True
            
            self.current_frame = frame_num
        
        cv2.destroyAllWindows()
        
        print("\n✅ Visualization complete!")
        print(f"   Processed {frame_num + 1} frames")
        print(f"   Focus shifted {len(self.focus_history)} times")

def main():
    viz = MelvinVisualizer()
    viz.run(num_frames=150)

if __name__ == "__main__":
    main()


