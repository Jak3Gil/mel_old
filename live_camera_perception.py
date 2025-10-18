#!/usr/bin/env python3
"""
Live Camera Visual Perception for Melvin
Real-time YOLO detection → Knowledge Graph
"""

import cv2
import json
import time
import sys
from pathlib import Path

try:
    from ultralytics import YOLO
except ImportError:
    print("Error: ultralytics not installed. Run: pip3 install ultralytics")
    sys.exit(1)

class LivePerception:
    def __init__(self, camera_index=0, model_name='yolov8n.pt', confidence=0.25):
        """Initialize live perception with YOLO"""
        print(f"🔧 Loading YOLO model: {model_name}")
        self.model = YOLO(model_name)
        self.confidence = confidence
        self.camera_index = camera_index
        
        # Statistics
        self.frame_count = 0
        self.total_objects = 0
        self.start_time = time.time()
        
        # Knowledge graph simulation (in real system, this would write to nodes.bin/edges.bin)
        self.graph_nodes = []
        self.graph_edges = []
        
    def run(self):
        """Start live camera capture and perception"""
        print(f"\n👁️  Opening camera {self.camera_index}...")
        cap = cv2.VideoCapture(self.camera_index)
        
        if not cap.isOpened():
            print(f"❌ Error: Could not open camera {self.camera_index}")
            print("\nAvailable cameras:")
            print("  0 - FaceTime HD Camera (built-in)")
            print("  1 - UVC Camera (USB)")
            print("  2 - iPhone Camera")
            return
        
        # Get camera properties
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = cap.get(cv2.CAP_PROP_FPS)
        
        print(f"✅ Camera opened: {width}x{height} @ {fps}fps")
        print(f"🎯 Confidence threshold: {self.confidence}")
        print(f"🧠 Building knowledge graph in real-time...")
        print(f"\n📺 Press 'q' to quit, 's' to show stats\n")
        
        # Previous frame objects for temporal edges
        prev_objects = []
        
        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    print("⚠️  Failed to read frame")
                    break
                
                self.frame_count += 1
                
                # Run YOLO detection
                results = self.model(frame, conf=self.confidence, verbose=False)
                
                # Process detections
                frame_objects = []
                
                for result in results:
                    boxes = result.boxes
                    for box in boxes:
                        # Extract bounding box
                        x1, y1, x2, y2 = map(int, box.xyxy[0].cpu().numpy())
                        
                        # Get class and confidence
                        cls_id = int(box.cls[0].cpu().numpy())
                        conf = float(box.conf[0].cpu().numpy())
                        label = result.names[cls_id]
                        
                        frame_objects.append({
                            'label': label,
                            'confidence': conf,
                            'bbox': (x1, y1, x2, y2)
                        })
                        
                        # Create node in knowledge graph
                        node_id = len(self.graph_nodes)
                        node_content = f"object:{label}:{self.frame_count}:{len(frame_objects)-1}"
                        self.graph_nodes.append({
                            'id': node_id,
                            'content': node_content,
                            'type': 'SENSORY',
                            'activation': conf
                        })
                        
                        # Draw bounding box
                        color = (0, 255, 0)
                        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                        
                        # Draw label
                        label_text = f"{label} {conf:.2f}"
                        cv2.putText(frame, label_text, (x1, y1 - 10),
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
                
                # Create EXACT edges (intra-frame: same frame co-occurrence)
                num_objects = len(frame_objects)
                if num_objects > 1:
                    for i in range(num_objects):
                        for j in range(i + 1, num_objects):
                            self.graph_edges.append({
                                'from': len(self.graph_nodes) - num_objects + i,
                                'to': len(self.graph_nodes) - num_objects + j,
                                'type': 'EXACT',
                                'weight': 1.0
                            })
                
                # Create LEAP edges (inter-frame: temporal connections)
                if prev_objects:
                    for curr_obj in frame_objects:
                        for prev_obj in prev_objects:
                            # Link same object types across frames
                            if curr_obj['label'] == prev_obj['label']:
                                self.graph_edges.append({
                                    'from': prev_obj['node_id'],
                                    'to': len(self.graph_nodes) - len(frame_objects),
                                    'type': 'LEAP',
                                    'weight': 0.5
                                })
                
                # Store for next frame
                for i, obj in enumerate(frame_objects):
                    obj['node_id'] = len(self.graph_nodes) - len(frame_objects) + i
                prev_objects = frame_objects
                
                self.total_objects += num_objects
                
                # Add stats overlay
                elapsed = time.time() - self.start_time
                fps_actual = self.frame_count / elapsed if elapsed > 0 else 0
                
                stats_text = f"Frame: {self.frame_count} | Objects: {num_objects} | Nodes: {len(self.graph_nodes)} | FPS: {fps_actual:.1f}"
                cv2.putText(frame, stats_text, (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
                
                # Show frame
                cv2.imshow('Melvin Visual Perception', frame)
                
                # Handle keyboard
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q'):
                    print("\n👋 Stopping...")
                    break
                elif key == ord('s'):
                    self.print_stats()
                
        finally:
            cap.release()
            cv2.destroyAllWindows()
            self.print_final_stats()
    
    def print_stats(self):
        """Print current statistics"""
        elapsed = time.time() - self.start_time
        print(f"\n📊 Current Stats:")
        print(f"  Frames: {self.frame_count}")
        print(f"  Objects detected: {self.total_objects}")
        print(f"  Nodes in graph: {len(self.graph_nodes)}")
        print(f"  Edges in graph: {len(self.graph_edges)}")
        print(f"  EXACT edges: {sum(1 for e in self.graph_edges if e['type'] == 'EXACT')}")
        print(f"  LEAP edges: {sum(1 for e in self.graph_edges if e['type'] == 'LEAP')}")
        print(f"  Avg objects/frame: {self.total_objects/self.frame_count:.2f}")
        print(f"  FPS: {self.frame_count/elapsed:.2f}")
        print()
    
    def print_final_stats(self):
        """Print final statistics"""
        elapsed = time.time() - self.start_time
        
        print("\n╔════════════════════════════════════════════════════════════════╗")
        print("║  👁️  LIVE PERCEPTION SESSION COMPLETE                          ║")
        print("╚════════════════════════════════════════════════════════════════╝\n")
        
        print(f"📊 Final Statistics:")
        print(f"  Duration:            {elapsed:.2f} seconds")
        print(f"  Frames processed:    {self.frame_count}")
        print(f"  Objects detected:    {self.total_objects}")
        print(f"  Nodes created:       {len(self.graph_nodes)}")
        print(f"  Edges created:       {len(self.graph_edges)}")
        print(f"    ├─ EXACT (intra):  {sum(1 for e in self.graph_edges if e['type'] == 'EXACT')}")
        print(f"    └─ LEAP (inter):   {sum(1 for e in self.graph_edges if e['type'] == 'LEAP')}")
        print(f"  Avg objects/frame:   {self.total_objects/self.frame_count if self.frame_count > 0 else 0:.2f}")
        print(f"  Processing FPS:      {self.frame_count/elapsed if elapsed > 0 else 0:.2f}")
        print()
        
        # Save graph snapshot
        self.save_graph_snapshot()
    
    def save_graph_snapshot(self):
        """Save knowledge graph to JSON"""
        snapshot = {
            'metadata': {
                'frames': self.frame_count,
                'total_objects': self.total_objects,
                'duration': time.time() - self.start_time
            },
            'nodes': self.graph_nodes,
            'edges': self.graph_edges
        }
        
        output_file = 'perception_graph_snapshot.json'
        with open(output_file, 'w') as f:
            json.dump(snapshot, f, indent=2)
        
        print(f"💾 Graph snapshot saved to: {output_file}")
        print(f"   (In production, this would be saved to nodes.melvin/edges.melvin)")
        print()

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Live camera visual perception for Melvin')
    parser.add_argument('--camera', type=int, default=1, 
                       help='Camera index (0=FaceTime, 1=USB, 2=iPhone)')
    parser.add_argument('--model', type=str, default='yolov8n.pt',
                       help='YOLO model (n/s/m/l/x)')
    parser.add_argument('--confidence', type=float, default=0.25,
                       help='Confidence threshold (0.0-1.0)')
    
    args = parser.parse_args()
    
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  👁️  MELVIN LIVE VISUAL PERCEPTION                             ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    print("🎯 Real-time YOLO Detection → Knowledge Graph")
    print()
    
    perception = LivePerception(
        camera_index=args.camera,
        model_name=args.model,
        confidence=args.confidence
    )
    
    perception.run()

if __name__ == '__main__':
    main()


