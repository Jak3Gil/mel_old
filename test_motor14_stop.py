#!/usr/bin/env python3
"""
Test Motor 14 Stop Command
Verify that motor 14 responds to stop commands
"""

import serial
import time

def test_stop():
    """Test if motor 14 stops properly."""
    print("=" * 70)
    print("   MOTOR 14 STOP TEST")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Initialize
        print("[1] Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        # Activate motor 14
        print("[2] Activating motor 14...")
        scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a])
        ser.write(scan_cmd)
        time.sleep(0.5)
        ser.read(1000)
        print("    [OK]\n")
        
        # Test 1: Start and stop quickly
        print("[3] Test quick start/stop...")
        
        # Start motor 14
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        start_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        
        print("    Starting motor 14...")
        ser.write(start_cmd)
        time.sleep(1.0)
        
        # Stop motor 14
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        print("    Sending STOP...")
        for i in range(3):
            ser.write(stop_cmd)
            time.sleep(0.05)
        
        print("    [OK] Stop sent\n")
        time.sleep(1)
        
        # Test 2: Longer run with stop
        print("[4] Test longer run (3 seconds)...")
        print("    Starting motor 14...")
        ser.write(start_cmd)
        time.sleep(3.0)
        
        print("    Sending STOP...")
        for i in range(5):
            ser.write(stop_cmd)
            time.sleep(0.05)
        
        print("    [OK] Stop sent\n")
        
        ser.close()
        
        print("=" * 70)
        print("TEST COMPLETE")
        print("=" * 70)
        print("\nDid motor 14 stop both times?")
        print("  [YES] - Stop command works")
        print("  [NO]  - Motor 14 may have a different stop protocol")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("\nMotor 14 Stop Command Test\n")
    input("Press ENTER to start test...")
    test_stop()

