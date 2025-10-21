#!/usr/bin/env python3
"""
YOLO Object Detection for Melvin's Visual Perception
Outputs JSON with detected objects and bounding boxes
"""

import sys
import json
import cv2
import numpy as np
from pathlib import Path

try:
    from ultralytics import YOLO
    YOLO_AVAILABLE = True
except ImportError:
    YOLO_AVAILABLE = False
    print("Warning: ultralytics not installed. Install with: pip install ultralytics", file=sys.stderr)


class MelvinObjectDetector:
    """YOLO-based object detector for Melvin's visual perception"""
    
    def __init__(self, model_name='yolov8n.pt', confidence_threshold=0.25):
        """
        Initialize YOLO detector
        
        Args:
            model_name: YOLO model to use (yolov8n.pt, yolov8s.pt, yolov8m.pt, etc.)
            confidence_threshold: Minimum confidence for detections
        """
        if not YOLO_AVAILABLE:
            raise ImportError("ultralytics package required. Install with: pip install ultralytics")
        
        self.model = YOLO(model_name)
        self.confidence_threshold = confidence_threshold
    
    def detect_from_image_path(self, image_path):
        """
        Detect objects in an image file
        
        Returns:
            list of dict: [{'label': str, 'confidence': float, 'x1': int, 'y1': int, 'x2': int, 'y2': int}, ...]
        """
        # Run YOLO inference
        results = self.model(image_path, conf=self.confidence_threshold, verbose=False)
        
        detections = []
        
        # Parse results
        for result in results:
            boxes = result.boxes
            for box in boxes:
                # Extract bounding box coordinates
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                
                # Get class label and confidence
                cls_id = int(box.cls[0].cpu().numpy())
                confidence = float(box.conf[0].cpu().numpy())
                label = result.names[cls_id]
                
                detections.append({
                    'label': label,
                    'confidence': confidence,
                    'x1': int(x1),
                    'y1': int(y1),
                    'x2': int(x2),
                    'y2': int(y2)
                })
        
        return detections
    
    def detect_from_frame(self, frame):
        """
        Detect objects in a numpy array (OpenCV frame)
        
        Args:
            frame: numpy array from cv2.VideoCapture
            
        Returns:
            list of dict: detections
        """
        results = self.model(frame, conf=self.confidence_threshold, verbose=False)
        
        detections = []
        
        for result in results:
            boxes = result.boxes
            for box in boxes:
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                cls_id = int(box.cls[0].cpu().numpy())
                confidence = float(box.conf[0].cpu().numpy())
                label = result.names[cls_id]
                
                detections.append({
                    'label': label,
                    'confidence': confidence,
                    'x1': int(x1),
                    'y1': int(y1),
                    'x2': int(x2),
                    'y2': int(y2)
                })
        
        return detections


def main():
    """
    Command-line interface for object detection
    Usage: python detect_objects.py <image_path> [model_name] [confidence_threshold]
    """
    if len(sys.argv) < 2:
        print("Usage: python detect_objects.py <image_path> [model_name] [confidence_threshold]")
        print("Example: python detect_objects.py frame.jpg yolov8n.pt 0.25")
        sys.exit(1)
    
    image_path = sys.argv[1]
    model_name = sys.argv[2] if len(sys.argv) > 2 else 'yolov8n.pt'
    confidence_threshold = float(sys.argv[3]) if len(sys.argv) > 3 else 0.25
    
    if not Path(image_path).exists():
        print(f"Error: Image file not found: {image_path}", file=sys.stderr)
        sys.exit(1)
    
    try:
        detector = MelvinObjectDetector(model_name, confidence_threshold)
        detections = detector.detect_from_image_path(image_path)
        
        # Output as JSON
        print(json.dumps(detections, indent=2))
        
    except Exception as e:
        print(f"Error during detection: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()


