#!/usr/bin/env python3
"""
Test Motor 14 Only - Debug why it's not moving
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600
motor14_encoding = bytes([0xe8, 0x74])

try:
    print("=" * 70)
    print("   MOTOR 14 ISOLATED TEST")
    print("=" * 70 + "\n")
    
    ser = serial.Serial(PORT, BAUD, timeout=2.0)
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    print("[1] Connected\n")
    
    # Initialize
    print("[2] Initializing...")
    ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
    time.sleep(0.3)
    resp = ser.read(1000)
    print(f"    Handshake: {resp.decode('ascii', errors='ignore').strip()}\n")
    
    # Scan for Motor 14
    print("[3] Scanning for Motor 14 (encoding e8 74)...")
    scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a])
    ser.write(scan_cmd)
    time.sleep(0.5)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    *** Motor 14 RESPONDED! ***")
        print(f"    Response: {resp.hex()}")
        
        # Check MCU ID in response
        if b'\xbc\x7d\x30\x20\x9c\x23\x37' in resp:
            print(f"    MCU ID confirmed: 0xd37239c20307dbc")
    else:
        print(f"    WARNING: No response from Motor 14")
        print(f"    Motor 14 might not be powered or on CAN bus")
    
    print()
    time.sleep(1)
    
    # Test movement
    print("[4] Testing Motor 14 movement...")
    print("    Sending JOG forward for 3 seconds...")
    
    jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
    cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
    
    print(f"    Command: {cmd.hex()}")
    ser.write(cmd)
    print("    >>> WATCH MOTOR 14 - SHOULD BE MOVING! <<<")
    time.sleep(3)
    
    # Stop
    stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
    stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
    
    print("    Sending STOP...")
    for i in range(5):
        ser.write(stop_cmd)
        time.sleep(0.02)
    
    print("    [OK] Stop sent\n")
    time.sleep(1)
    
    # Try reverse
    print("[5] Testing reverse...")
    jog_rev = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
    cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_rev + bytes([0x0d, 0x0a])
    
    ser.write(cmd)
    print("    >>> WATCH MOTOR 14 - SHOULD BE MOVING REVERSE! <<<")
    time.sleep(3)
    
    # Stop
    for i in range(5):
        ser.write(stop_cmd)
        time.sleep(0.02)
    
    print("    [OK] Stop sent\n")
    
    ser.close()
    
    print("=" * 70)
    print("TEST COMPLETE")
    print("=" * 70)
    
    print("\nDID MOTOR 14 MOVE?")
    print("  [YES] - Motor 14 works, encoding is correct")
    print("  [NO]  - Possible issues:")
    print("         1. Motor 14 not powered")
    print("         2. Motor 14 not on CAN bus")
    print("         3. Different encoding needed")
    print("         4. Motor 14 needs different initialization")

except Exception as e:
    print(f"\n[ERROR] {e}")
    import traceback
    traceback.print_exc()







