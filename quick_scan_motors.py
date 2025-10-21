#!/usr/bin/env python3
"""
Quick scan for motors - no input required
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600

try:
    print("Scanning for motors...\n")
    
    ser = serial.Serial(PORT, BAUD, timeout=1.0)
    time.sleep(0.5)
    ser.reset_input_buffer()
    
    # Init
    ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    
    found = []
    
    # Scan key encodings
    test_encodings = [
        (0xe8, 0x6c, "Motor 13 expected"),
        (0xe8, 0x68, "Motor 13 alt?"),
        (0xe8, 0x74, "Motor 14 expected"),
        (0xe8, 0x70, "Motor 14 alt?"),
        (0xe8, 0x78, "Motor 14 alt?"),
    ]
    
    for high, low, desc in test_encodings:
        scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, high, low, 0x01, 0x00, 0x0d, 0x0a])
        ser.reset_input_buffer()
        ser.write(scan_cmd)
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"[FOUND] {high:02x} {low:02x} - {desc}")
            print(f"        Response: {resp.hex()}\n")
            found.append((high, low, desc))
        else:
            print(f"[ --- ] {high:02x} {low:02x} - {desc} (no response)")
    
    ser.close()
    
    print("\n" + "=" * 70)
    print(f"Found {len(found)} motor(s)")
    print("=" * 70)
    
    if len(found) == 1:
        print("\nOnly Motor 13 detected!")
        print("\nCheck Motor 14:")
        print("  - Is it powered? (LED on?)")
        print("  - Is it connected to CAN bus?")
        print("  - Same CANH/CANL as Motor 13?")
    elif len(found) == 2:
        print("\nBoth motors detected!")

except Exception as e:
    print(f"[ERROR] {e}")







