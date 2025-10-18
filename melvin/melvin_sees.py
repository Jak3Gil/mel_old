#!/usr/bin/env python3
"""
Easy launcher for Melvin vision - automatically picks working camera
"""

import cv2
import sys
import os

def find_first_working_camera():
    """Find first working camera"""
    print("🔍 Searching for cameras...")
    
    for i in range(3):
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            ret, frame = cap.read()
            if ret and frame is not None and frame.size > 0:
                cap.release()
                print(f"✅ Found working camera: {i}")
                return i
        cap.release()
    
    return None

def main():
    # Check arguments
    camera_id = None
    
    if len(sys.argv) > 1:
        if sys.argv[1] in ['-h', '--help']:
            print("\nMelvin Vision - See what Melvin sees!")
            print("\nUsage:")
            print("  python3 melvin_sees.py        # Auto-detect camera")
            print("  python3 melvin_sees.py 0      # Use camera 0")
            print("  python3 melvin_sees.py 1      # Use camera 1")
            print("\nControls:")
            print("  q      - Quit")
            print("  p      - Pause/Resume")
            print("  s      - Save snapshot")
            print("  SPACE  - Single step")
            return 0
        else:
            try:
                camera_id = int(sys.argv[1])
            except:
                print(f"❌ Invalid camera ID: {sys.argv[1]}")
                return 1
    
    # Auto-detect if not specified
    if camera_id is None:
        camera_id = find_first_working_camera()
        
        if camera_id is None:
            print("\n❌ No working cameras found!")
            print("\nTroubleshooting:")
            print("  1. Check camera permissions")
            print("  2. Close other apps using camera")
            print("  3. Run: python3 test_camera.py")
            print("\nOr try the test visualization:")
            print("  python3 visualize_melvin.py")
            return 1
    
    # Import and run
    try:
        from visualize_camera import MelvinCameraVision
        
        print(f"\n🧠 Starting Melvin Vision with camera {camera_id}...")
        print("   (Press 'q' to quit)\n")
        
        viz = MelvinCameraVision(camera_id=camera_id)
        viz.run()
        
    except ImportError as e:
        print(f"❌ Import error: {e}")
        print("Make sure visualize_camera.py is in the same directory")
        return 1
    except Exception as e:
        print(f"\n❌ Error: {e}")
        print("\nTry:")
        print("  python3 test_camera.py      # Diagnose cameras")
        print("  python3 melvin_sees.py 1    # Try different camera")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

