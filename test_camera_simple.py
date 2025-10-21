#!/usr/bin/env python3
"""
Simple Camera Test for Windows
"""

import cv2
import sys

def test_camera(camera_id):
    """Test specific camera"""
    print(f"\nTesting camera {camera_id}...")
    
    cap = cv2.VideoCapture(camera_id)
    
    if not cap.isOpened():
        print(f"  [FAIL] Cannot open camera {camera_id}")
        return False
    
    # Try to read a frame
    ret, frame = cap.read()
    
    if not ret or frame is None:
        print(f"  [FAIL] Cannot read from camera {camera_id}")
        cap.release()
        return False
    
    # Get properties
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = int(cap.get(cv2.CAP_PROP_FPS))
    
    print(f"  [OK] Camera {camera_id} working!")
    print(f"       Resolution: {width}x{height}")
    print(f"       FPS: {fps}")
    print(f"       Frame shape: {frame.shape}")
    
    # Show a test window
    print(f"       Showing test window (2 seconds)...")
    cv2.imshow(f"Camera {camera_id} Test", frame)
    cv2.waitKey(2000)
    cv2.destroyAllWindows()
    
    cap.release()
    return True

def main():
    print("\n" + "="*60)
    print(" Camera Diagnostic Tool")
    print("="*60)
    
    print("\nSearching for available cameras (0-4)...")
    print("This may take a moment...\n")
    
    working_cameras = []
    
    # Test camera IDs 0-4
    for i in range(5):
        if test_camera(i):
            working_cameras.append(i)
    
    print("\n" + "="*60)
    print(" RESULTS")
    print("="*60)
    
    if working_cameras:
        print(f"\n[SUCCESS] Found {len(working_cameras)} working camera(s):")
        for cam_id in working_cameras:
            print(f"  - Camera {cam_id}")
        
        print("\nTo test camera live:")
        print(f"  python test_camera_simple.py live")
    else:
        print("\n[FAIL] No working cameras found!")
        print("\nTroubleshooting:")
        print("  1. Check if camera is physically connected")
        print("  2. Grant camera permissions to Python")
        print("  3. Close other apps using the camera")
        print("  4. Try external USB camera")
    
    print()

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == 'live':
        camera_id = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        print(f"\nStarting live preview (Camera {camera_id})...")
        print("Press 'q' to quit\n")
        
        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            print(f"[FAIL] Cannot open camera {camera_id}")
            sys.exit(1)
        
        frame_count = 0
        try:
            while True:
                ret, frame = cap.read()
                if not ret:
                    break
                
                cv2.putText(frame, f"Camera {camera_id} - Frame {frame_count}", 
                           (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                cv2.putText(frame, "Press 'q' to quit", 
                           (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                
                cv2.imshow(f"Camera {camera_id} Live", frame)
                frame_count += 1
                
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
        except KeyboardInterrupt:
            pass
        finally:
            cap.release()
            cv2.destroyAllWindows()
            print(f"\nCaptured {frame_count} frames")
    else:
        main()






