#!/usr/bin/env python3
"""
Find Motor 14's correct activation encoding
Test all encodings that responded in scan
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600

# From earlier scan, these responded:
# Motor 13: e8 6c (MCU: 3c3d30209c233711)
# Motor 14: e8 74 (MCU: bc7d30209c23370d)

motor14_test_encodings = [
    (0xe8, 0x74, "Primary Motor 14"),
    (0xe8, 0x70, "Alt Motor 14"),
    (0xe9, 0x74, "e9 variant"),
    (0xea, 0x74, "ea variant"),
]

try:
    ser = serial.Serial(PORT, BAUD, timeout=2.0)
    time.sleep(0.5)
    ser.reset_input_buffer()
    
    # Init
    ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    
    print("=" * 70)
    print("TESTING MOTOR 14 ENCODINGS")
    print("=" * 70 + "\n")
    
    for high, low, desc in motor14_test_encodings:
        print(f"Testing {high:02x} {low:02x} - {desc}")
        
        # Scan
        scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, high, low, 0x01, 0x00, 0x0d, 0x0a])
        ser.reset_input_buffer()
        ser.write(scan_cmd)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"  Scan response: {resp.hex()}")
            
            # Check if it's Motor 14's MCU ID
            if b'\xbc\x7d\x30\x20' in resp or b'\x0d\x7d\xbc' in resp:
                print(f"  *** This looks like Motor 14! ***")
        else:
            print(f"  No response")
        
        # Try moving with this encoding
        print(f"  Testing movement...")
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, high, low, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        
        ser.write(cmd)
        print(f"  >>> WATCH MOTORS - Which one moved? <<<")
        time.sleep(2.0)
        
        # Stop
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, high, low, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        for i in range(3):
            ser.write(stop_cmd)
            time.sleep(0.02)
        
        response = input(f"  Which motor moved? (13/14/none): ").strip()
        
        if response == '14':
            print(f"\n  *** FOUND MOTOR 14 ENCODING: {high:02x} {low:02x} ***")
            
            with open('motor14_working_encoding.txt', 'w') as f:
                f.write(f"Motor 14 encoding: {high:02x} {low:02x}\n")
            
            print(f"  Saved to motor14_working_encoding.txt")
            break
        elif response == '13':
            print(f"  This controls Motor 13, not 14\n")
        else:
            print(f"  No movement\n")
        
        time.sleep(1)
    
    ser.close()

except Exception as e:
    print(f"\n[ERROR] {e}")
    import traceback
    traceback.print_exc()







