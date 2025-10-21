#!/usr/bin/env python3
"""
Camera Diagnostic Tool
Tests camera access and shows available cameras
"""

import cv2
import sys

def test_camera(camera_id):
    """Test specific camera"""
    print(f"\n🔍 Testing camera {camera_id}...")
    
    cap = cv2.VideoCapture(camera_id)
    
    if not cap.isOpened():
        print(f"  ❌ Cannot open camera {camera_id}")
        return False
    
    # Try to read a frame
    ret, frame = cap.read()
    
    if not ret or frame is None:
        print(f"  ❌ Cannot read from camera {camera_id}")
        cap.release()
        return False
    
    # Get properties
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    fps = int(cap.get(cv2.CAP_PROP_FPS))
    
    print(f"  ✅ Camera {camera_id} working!")
    print(f"     Resolution: {width}x{height}")
    print(f"     FPS: {fps}")
    print(f"     Frame shape: {frame.shape}")
    
    # Show a test window
    print(f"     Showing test window (press any key to continue)...")
    cv2.imshow(f"Camera {camera_id} Test", frame)
    cv2.waitKey(2000)  # Show for 2 seconds
    cv2.destroyAllWindows()
    
    cap.release()
    return True

def find_cameras():
    """Find all available cameras"""
    print("\n╔═══════════════════════════════════════════════════════════╗")
    print("║  📹 Camera Diagnostic Tool                               ║")
    print("╚═══════════════════════════════════════════════════════════╝")
    
    print("\nSearching for available cameras...")
    print("(This may take a moment...)")
    
    working_cameras = []
    
    # Test camera IDs 0-4
    for i in range(5):
        if test_camera(i):
            working_cameras.append(i)
    
    print("\n" + "="*60)
    print("SUMMARY")
    print("="*60)
    
    if working_cameras:
        print(f"\n✅ Found {len(working_cameras)} working camera(s):")
        for cam_id in working_cameras:
            print(f"   • Camera {cam_id}")
        
        print("\n📝 To use a specific camera:")
        for cam_id in working_cameras:
            print(f"   Camera {cam_id}: Edit visualize_camera.py")
            print(f"             Change: camera_id={cam_id}")
    else:
        print("\n❌ No working cameras found!")
        print("\nTroubleshooting:")
        print("  1. Check if camera is physically connected")
        print("  2. Grant camera permissions to Terminal/Python")
        print("  3. Close other apps using the camera (Zoom, FaceTime, etc.)")
        print("  4. Try external USB camera")
        print("\nOn macOS:")
        print("  System Preferences → Security & Privacy → Camera")
        print("  Make sure Terminal (or your IDE) has camera access")
    
    print("\n")
    return working_cameras

def continuous_test(camera_id):
    """Test camera with live preview"""
    print(f"\n🎥 Starting live camera test (camera {camera_id})...")
    print("Press 'q' to quit\n")
    
    cap = cv2.VideoCapture(camera_id)
    
    if not cap.isOpened():
        print(f"❌ Cannot open camera {camera_id}")
        return
    
    frame_count = 0
    
    try:
        while True:
            ret, frame = cap.read()
            
            if not ret:
                print(f"❌ Failed to read frame {frame_count}")
                break
            
            # Add info overlay
            cv2.putText(frame, f"Camera {camera_id} - Frame {frame_count}", 
                       (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.putText(frame, "Press 'q' to quit", 
                       (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            
            cv2.imshow(f"Camera {camera_id} Live Test", frame)
            
            frame_count += 1
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
    
    except KeyboardInterrupt:
        print("\n⚠️  Interrupted")
    
    finally:
        cap.release()
        cv2.destroyAllWindows()
        print(f"\n✅ Captured {frame_count} frames")

def main():
    if len(sys.argv) > 1:
        if sys.argv[1] == 'test':
            # Live test mode
            camera_id = int(sys.argv[2]) if len(sys.argv) > 2 else 0
            continuous_test(camera_id)
        else:
            print("Usage:")
            print("  python3 test_camera.py           # Find all cameras")
            print("  python3 test_camera.py test [N]  # Live test camera N")
    else:
        # Find cameras
        working = find_cameras()
        
        if working:
            print("Want to do a live test?")
            print("Run: python3 test_camera.py test 0")

if __name__ == "__main__":
    main()

