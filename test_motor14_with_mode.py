#!/usr/bin/env python3
"""
Test Motor 14 with proper MIT mode initialization
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600
motor14_encoding = bytes([0xe8, 0x74])

try:
    print("=" * 70)
    print("   MOTOR 14 - WITH MIT MODE SETUP")
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
    ser.read(1000)
    print("    [OK]\n")
    
    # Activate Motor 14
    print("[3] Activating Motor 14...")
    scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a])
    ser.write(scan_cmd)
    time.sleep(0.5)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    Motor 14 detected: {resp.hex()}")
    print("    [OK]\n")
    
    # Enable MIT mode for Motor 14
    print("[4] Enabling MIT control mode for Motor 14...")
    
    # SET_RUN_MODE (from Motor 13 command, adapted for Motor 14)
    set_run_mode = bytes([
        0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08,
        0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0d, 0x0a
    ])
    print(f"    Sending SET_RUN_MODE...")
    ser.write(set_run_mode)
    time.sleep(0.3)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    Response: {resp.hex()}")
    
    # SET_MODE
    set_mode = bytes([
        0x41, 0x54, 0x18, 0x07, 0xe8, 0x74, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0d, 0x0a
    ])
    print(f"    Sending SET_MODE...")
    ser.write(set_mode)
    time.sleep(0.3)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    Response: {resp.hex()}")
    
    print("    [OK] MIT mode enabled\n")
    
    time.sleep(1)
    
    # Now test movement
    print("[5] Testing Motor 14 JOG...")
    print("    Forward 3 seconds...")
    
    jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
    cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
    
    ser.write(cmd)
    print("    >>> MOTOR 14 SHOULD BE MOVING NOW! <<<")
    time.sleep(3)
    
    # Stop
    stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
    stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
    
    for i in range(5):
        ser.write(stop_cmd)
        time.sleep(0.02)
    
    print("    [STOPPED]\n")
    
    ser.close()
    
    print("=" * 70)
    print("DID MOTOR 14 MOVE?")
    print("=" * 70)
    
    print("\nIf NO:")
    print("  - Motor 14 might not be powered")
    print("  - Check power LED on Motor 14")
    print("  - Verify CANH/CANL connections")
    print("  - Check if Motor 14 shows in Robstride software")

except Exception as e:
    print(f"\n[ERROR] {e}")
    import traceback
    traceback.print_exc()













