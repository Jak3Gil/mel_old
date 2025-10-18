#!/usr/bin/env python3
"""
Live Camera Visual Perception for Melvin (SMART VERSION)
With object persistence tracking - reuses nodes for the same objects
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

def compute_iou(box1, box2):
    """Compute Intersection over Union between two bounding boxes"""
    x1_min, y1_min, x1_max, y1_max = box1
    x2_min, y2_min, x2_max, y2_max = box2
    
    # Intersection
    inter_xmin = max(x1_min, x2_min)
    inter_ymin = max(y1_min, y2_min)
    inter_xmax = min(x1_max, x2_max)
    inter_ymax = min(y1_max, y2_max)
    
    if inter_xmax < inter_xmin or inter_ymax < inter_ymin:
        return 0.0
    
    inter_area = (inter_xmax - inter_xmin) * (inter_ymax - inter_ymin)
    
    # Union
    box1_area = (x1_max - x1_min) * (y1_max - y1_min)
    box2_area = (x2_max - x2_min) * (y2_max - y2_min)
    union_area = box1_area + box2_area - inter_area
    
    return inter_area / union_area if union_area > 0 else 0.0


class TrackedObject:
    """Represents a persistent object across frames"""
    def __init__(self, node_id, label, bbox, confidence, frame_num):
        self.node_id = node_id
        self.label = label
        self.bbox = bbox
        self.confidence = confidence
        self.first_seen_frame = frame_num
        self.last_seen_frame = frame_num
        self.detection_count = 1
        self.total_confidence = confidence
    
    def update(self, bbox, confidence, frame_num):
        """Update with new detection"""
        self.bbox = bbox
        self.confidence = confidence
        self.last_seen_frame = frame_num
        self.detection_count += 1
        self.total_confidence += confidence
    
    def avg_confidence(self):
        return self.total_confidence / self.detection_count


class LivePerceptionSmart:
    def __init__(self, camera_index=0, model_name='yolov8n.pt', confidence=0.25):
        """Initialize live perception with smart object tracking"""
        print(f"🔧 Loading YOLO model: {model_name}")
        self.model = YOLO(model_name)
        self.confidence = confidence
        self.camera_index = camera_index
        
        # Smart tracking
        self.tracked_objects = []  # List of TrackedObject
        self.next_node_id = 0
        self.iou_threshold = 0.5  # IoU threshold for considering same object
        self.max_missing_frames = 5  # Remove object if not seen for N frames
        
        # Statistics
        self.frame_count = 0
        self.total_detections = 0
        self.unique_objects = 0  # Unique nodes created
        self.reused_nodes = 0     # Times we reused existing node
        self.start_time = time.time()
        
        # Knowledge graph
        self.graph_nodes = {}  # node_id -> node data
        self.graph_edges = []
        
    def find_matching_object(self, label, bbox, frame_num):
        """Find if this detection matches an existing tracked object"""
        best_match = None
        best_iou = 0.0
        
        for tracked in self.tracked_objects:
            # Must be same label
            if tracked.label != label:
                continue
            
            # Must be seen recently
            if frame_num - tracked.last_seen_frame > self.max_missing_frames:
                continue
            
            # Check IoU
            iou = compute_iou(tracked.bbox, bbox)
            if iou > self.iou_threshold and iou > best_iou:
                best_iou = iou
                best_match = tracked
        
        return best_match, best_iou
    
    def cleanup_old_objects(self, current_frame):
        """Remove objects that haven't been seen recently"""
        self.tracked_objects = [
            obj for obj in self.tracked_objects
            if current_frame - obj.last_seen_frame <= self.max_missing_frames
        ]
    
    def run(self):
        """Start live camera capture and perception"""
        print(f"\n👁️  Opening camera {self.camera_index}...")
        cap = cv2.VideoCapture(self.camera_index)
        
        if not cap.isOpened():
            print(f"❌ Error: Could not open camera {self.camera_index}")
            print("\nTry camera 0 (built-in): python3 live_camera_perception_smart.py --camera 0")
            return
        
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        fps = cap.get(cv2.CAP_PROP_FPS)
        
        print(f"✅ Camera opened: {width}x{height} @ {fps}fps")
        print(f"🎯 Confidence threshold: {self.confidence}")
        print(f"🧠 Smart object tracking enabled (IoU threshold: {self.iou_threshold})")
        print(f"\n📺 Press 'q' to quit, 's' to show stats\n")
        
        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    print("⚠️  Failed to read frame")
                    break
                
                self.frame_count += 1
                
                # Run YOLO detection
                results = self.model(frame, conf=self.confidence, verbose=False)
                
                # Process detections with smart tracking
                frame_objects = []
                
                for result in results:
                    boxes = result.boxes
                    for box in boxes:
                        # Extract detection
                        x1, y1, x2, y2 = map(int, box.xyxy[0].cpu().numpy())
                        bbox = (x1, y1, x2, y2)
                        
                        cls_id = int(box.cls[0].cpu().numpy())
                        conf = float(box.conf[0].cpu().numpy())
                        label = result.names[cls_id]
                        
                        self.total_detections += 1
                        
                        # Smart tracking: Find if this is an existing object
                        matched_obj, iou = self.find_matching_object(label, bbox, self.frame_count)
                        
                        if matched_obj:
                            # REUSE existing node!
                            matched_obj.update(bbox, conf, self.frame_count)
                            node_id = matched_obj.node_id
                            self.reused_nodes += 1
                            
                            # Update node in graph (refresh confidence)
                            self.graph_nodes[node_id]['activation'] = conf
                            self.graph_nodes[node_id]['last_seen_frame'] = self.frame_count
                            
                            color = (0, 255, 255)  # Yellow for tracked objects
                            
                        else:
                            # NEW object - create new node
                            node_id = self.next_node_id
                            self.next_node_id += 1
                            self.unique_objects += 1
                            
                            # Create tracked object
                            tracked = TrackedObject(node_id, label, bbox, conf, self.frame_count)
                            self.tracked_objects.append(tracked)
                            
                            # Create node in graph
                            node_content = f"object:{label}:#{node_id}"
                            self.graph_nodes[node_id] = {
                                'id': node_id,
                                'content': node_content,
                                'type': 'SENSORY',
                                'activation': conf,
                                'label': label,
                                'first_seen_frame': self.frame_count,
                                'last_seen_frame': self.frame_count,
                                'bbox': bbox
                            }
                            
                            color = (0, 255, 0)  # Green for new objects
                        
                        frame_objects.append({
                            'node_id': node_id,
                            'label': label,
                            'bbox': bbox,
                            'is_new': matched_obj is None
                        })
                        
                        # Draw bounding box
                        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                        
                        # Draw label
                        label_text = f"#{node_id} {label} {conf:.2f}"
                        if matched_obj:
                            label_text += f" (seen {matched_obj.detection_count}x)"
                        
                        cv2.putText(frame, label_text, (x1, y1 - 5),
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
                
                # Create EXACT edges (intra-frame: objects appearing together)
                if len(frame_objects) > 1:
                    for i in range(len(frame_objects)):
                        for j in range(i + 1, len(frame_objects)):
                            edge = {
                                'from': frame_objects[i]['node_id'],
                                'to': frame_objects[j]['node_id'],
                                'type': 'EXACT',
                                'weight': 1.0,
                                'frame': self.frame_count
                            }
                            self.graph_edges.append(edge)
                
                # Cleanup old objects
                self.cleanup_old_objects(self.frame_count)
                
                # Stats overlay
                elapsed = time.time() - self.start_time
                fps_actual = self.frame_count / elapsed if elapsed > 0 else 0
                
                stats_lines = [
                    f"Frame: {self.frame_count} | FPS: {fps_actual:.1f}",
                    f"Unique Objects: {self.unique_objects} | Active: {len(self.tracked_objects)}",
                    f"Detections: {self.total_detections} | Reused: {self.reused_nodes}",
                    f"Nodes: {len(self.graph_nodes)} | Edges: {len(self.graph_edges)}"
                ]
                
                for i, line in enumerate(stats_lines):
                    cv2.putText(frame, line, (10, 30 + i*30),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
                
                # Legend
                cv2.putText(frame, "Green=New  Yellow=Tracked", (10, height - 20),
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
                
                # Show frame
                cv2.imshow('Melvin Smart Vision', frame)
                
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
        print(f"  Total detections: {self.total_detections}")
        print(f"  Unique objects: {self.unique_objects} (nodes created)")
        print(f"  Reused nodes: {self.reused_nodes}")
        print(f"  Active tracked: {len(self.tracked_objects)}")
        print(f"  Nodes in graph: {len(self.graph_nodes)}")
        print(f"  Edges in graph: {len(self.graph_edges)}")
        print(f"  Node reuse rate: {self.reused_nodes/self.total_detections*100:.1f}%")
        print(f"  FPS: {self.frame_count/elapsed:.2f}")
        print()
    
    def print_final_stats(self):
        """Print final statistics"""
        elapsed = time.time() - self.start_time
        
        print("\n╔════════════════════════════════════════════════════════════════╗")
        print("║  👁️  SMART PERCEPTION SESSION COMPLETE                         ║")
        print("╚════════════════════════════════════════════════════════════════╝\n")
        
        print(f"📊 Final Statistics:")
        print(f"  Duration:            {elapsed:.2f} seconds")
        print(f"  Frames processed:    {self.frame_count}")
        print(f"  Total detections:    {self.total_detections}")
        print(f"  Unique objects:      {self.unique_objects} ✨ (nodes created)")
        print(f"  Node reuses:         {self.reused_nodes} 🔄")
        print(f"  Nodes in graph:      {len(self.graph_nodes)}")
        print(f"  Edges created:       {len(self.graph_edges)}")
        print(f"  Processing FPS:      {self.frame_count/elapsed if elapsed > 0 else 0:.2f}")
        print()
        print(f"📈 Efficiency:")
        print(f"  Node reuse rate:     {self.reused_nodes/self.total_detections*100:.1f}%")
        print(f"  Detections per node: {self.total_detections/self.unique_objects if self.unique_objects > 0 else 0:.1f}x")
        print()
        
        # Show tracked object summary
        print(f"🎯 Tracked Objects:")
        object_counts = {}
        for node_id, node in self.graph_nodes.items():
            label = node['label']
            object_counts[label] = object_counts.get(label, 0) + 1
        
        for label, count in sorted(object_counts.items(), key=lambda x: x[1], reverse=True):
            print(f"  {label}: {count} unique instance(s)")
        print()
        
        # Save graph snapshot
        self.save_graph_snapshot()
    
    def save_graph_snapshot(self):
        """Save knowledge graph to JSON"""
        snapshot = {
            'metadata': {
                'frames': self.frame_count,
                'total_detections': self.total_detections,
                'unique_objects': self.unique_objects,
                'reused_nodes': self.reused_nodes,
                'node_reuse_rate': f"{self.reused_nodes/self.total_detections*100:.1f}%",
                'duration': time.time() - self.start_time
            },
            'nodes': list(self.graph_nodes.values()),
            'edges': self.graph_edges
        }
        
        output_file = 'perception_graph_snapshot.json'
        with open(output_file, 'w') as f:
            json.dump(snapshot, f, indent=2)
        
        print(f"💾 Graph snapshot saved to: {output_file}")
        print(f"   With SMART tracking: {self.unique_objects} nodes instead of {self.total_detections}!")
        print(f"   Saved {self.total_detections - self.unique_objects} redundant nodes! 🎉")
        print()


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Smart live camera perception for Melvin')
    parser.add_argument('--camera', type=int, default=0, 
                       help='Camera index (0=built-in, 1=USB)')
    parser.add_argument('--model', type=str, default='yolov8n.pt',
                       help='YOLO model (n/s/m/l/x)')
    parser.add_argument('--confidence', type=float, default=0.25,
                       help='Confidence threshold (0.0-1.0)')
    parser.add_argument('--iou-threshold', type=float, default=0.5,
                       help='IoU threshold for object tracking (0.0-1.0)')
    
    args = parser.parse_args()
    
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  👁️  MELVIN SMART VISUAL PERCEPTION                            ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    print("🎯 Smart Object Tracking Enabled")
    print("   - Reuses nodes for the same objects")
    print("   - Green boxes = new objects")
    print("   - Yellow boxes = tracked objects")
    print()
    
    perception = LivePerceptionSmart(
        camera_index=args.camera,
        model_name=args.model,
        confidence=args.confidence
    )
    
    if args.iou_threshold != 0.5:
        perception.iou_threshold = args.iou_threshold
    
    perception.run()


if __name__ == '__main__':
    main()

