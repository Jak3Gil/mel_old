#!/usr/bin/env python3
"""
Quick test after cable change - verify adapter communication
"""

import serial
import time

print("=" * 60)
print("QUICK ADAPTER TEST AFTER CABLE CHANGE")
print("=" * 60)
print()

port = "COM3"

# Test 1: Can we open the port?
print("Test 1: Opening COM3...")
try:
    ser = serial.Serial(port, baudrate=115200, timeout=1)
    print("[OK] COM3 opened successfully")
    
    # Test 2: Send simple CAN enable command
    print()
    print("Test 2: Sending CAN command to motor...")
    # Enable command in L91 format: t 001 8 FF FF FF FF FF FF FF FC
    can_frame = "t0018FFFFFFFFFFFFFFFC\r"
    ser.write(can_frame.encode())
    print(f"[OK] Sent: {can_frame.strip()}")
    
    # Wait and check for response
    time.sleep(0.3)
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting)
        print(f"[OK] Motor responded: {response.hex()}")
        print("     --> CAN communication WORKING!")
    else:
        print("[!] No response from motor")
        print("    --> Either motor not responding OR adapter not passing data")
    
    ser.close()
    print()
    print("=" * 60)
    print("RESULT:")
    print("=" * 60)
    print()
    print("If motor responded: Cable is good, CAN works")
    print("If no response: Try the clean connection sequence")
    print()
    
except Exception as e:
    print(f"[X] Error: {e}")
    print()
    print("Possible causes:")
    print("  - Motor Studio still has port open")
    print("  - Driver issue with new cable")
    print("  - USB port problem")
    print()

