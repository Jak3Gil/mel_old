#!/usr/bin/env python3
"""
Quick vision capture for Melvin - captures one frame and runs YOLO
Outputs JSON to stdout
"""
import cv2
import json
import sys
from ultralytics import YOLO

try:
    # Capture one frame
    cap = cv2.VideoCapture(0)
    ret, frame = cap.read()
    cap.release()
    
    if not ret:
        print("[]", flush=True)  # Empty array if capture failed
        sys.exit(0)
    
    # Run YOLO
    model = YOLO('yolov8n.pt')
    results = model(frame, conf=0.3, verbose=False)
    
    # Extract detections
    detections = []
    for r in results:
        for box in r.boxes:
            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
            label = r.names[int(box.cls[0])]
            conf = float(box.conf[0])
            
            detections.append({
                'label': label,
                'confidence': conf,
                'x1': int(x1),
                'y1': int(y1),
                'x2': int(x2),
                'y2': int(y2)
            })
    
    # Output JSON
    print(json.dumps(detections), flush=True)
    
except Exception as e:
    # Output empty array on error
    print("[]", flush=True)
    sys.exit(0)

