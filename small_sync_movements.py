#!/usr/bin/env python3
"""
Small Synchronized Movements - Both Motors
Debug Motor 14 and test tiny movements
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600
motor13_encoding = bytes([0xe8, 0x6c])
motor14_encoding = bytes([0xe8, 0x74])

try:
    print("=" * 70)
    print("   SMALL SYNCHRONIZED MOVEMENTS")
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
    
    # Activate Motor 13
    print("    Activating Motor 13...")
    ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    resp = ser.read(1000)
    print(f"    M13 response: {resp.hex()[:40]}...")
    
    # MIT mode Motor 13
    ser.write(bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    ser.write(bytes([0x41, 0x54, 0x18, 0x07, 0xe8, 0x6c, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    print("    [OK] Motor 13 ready")
    
    # Activate Motor 14
    print("    Activating Motor 14...")
    ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    resp = ser.read(1000)
    if resp:
        print(f"    M14 response: {resp.hex()[:40]}...")
    else:
        print(f"    WARNING: Motor 14 no response to activation")
    
    # MIT mode Motor 14
    print("    Enabling MIT mode Motor 14...")
    ser.write(bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    resp = ser.read(1000)
    if resp:
        print(f"    M14 SET_RUN_MODE response: {resp.hex()[:40]}...")
    
    ser.write(bytes([0x41, 0x54, 0x18, 0x07, 0xe8, 0x74, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    resp = ser.read(1000)
    if resp:
        print(f"    M14 SET_MODE response: {resp.hex()[:40]}...")
    print("    [OK] Motor 14 ready\n")
    
    print("[3] Both motors initialized!\n")
    time.sleep(1)
    
    def jog_both(direction=1, duration=0.1):
        """Jog both motors."""
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        # Motor 13
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + data + bytes([0x0d, 0x0a])
        ser.write(cmd_13)
        
        # Motor 14
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + data + bytes([0x0d, 0x0a])
        ser.write(cmd_14)
        
        time.sleep(duration)
        
        # Stop both
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        stop_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        for i in range(3):
            ser.write(stop_13)
            ser.write(stop_14)
            time.sleep(0.02)
    
    print("=" * 70)
    print("SMALL SYNCHRONIZED MOVEMENTS")
    print("=" * 70 + "\n")
    
    # Test 1: Tiny pulses forward
    print("[1] Tiny pulses forward (100ms each, 5x)...")
    for i in range(5):
        print(f"    Pulse {i+1}/5")
        jog_both(direction=1, duration=0.1)
        time.sleep(0.3)
    print("    Done\n")
    
    # Test 2: Tiny pulses reverse
    print("[2] Tiny pulses reverse (100ms each, 5x)...")
    for i in range(5):
        print(f"    Pulse {i+1}/5")
        jog_both(direction=-1, duration=0.1)
        time.sleep(0.3)
    print("    Done\n")
    
    # Test 3: Back and forth
    print("[3] Small back and forth movements (10 cycles)...")
    for i in range(10):
        print(f"    Cycle {i+1}/10 - Forward")
        jog_both(direction=1, duration=0.08)
        time.sleep(0.2)
        print(f"    Cycle {i+1}/10 - Reverse")
        jog_both(direction=-1, duration=0.08)
        time.sleep(0.2)
    print("    Done\n")
    
    # Test 4: Very tiny movements
    print("[4] Micro movements (50ms each, 10x forward)...")
    for i in range(10):
        print(f"    Micro {i+1}/10")
        jog_both(direction=1, duration=0.05)
        time.sleep(0.15)
    print("    Done\n")
    
    ser.close()
    
    print("=" * 70)
    print("COMPLETE")
    print("=" * 70)
    
    print("\nDid you see BOTH motors moving?")
    print("  Motor 13: Should have moved")
    print("  Motor 14: Check if it moved")
    print("\nIf Motor 14 didn't move:")
    print("  - Check power to Motor 14")
    print("  - Verify it moves in Robstride software")
    print("  - Check CAN wiring")

except Exception as e:
    print(f"\n[ERROR] {e}")
    import traceback
    traceback.print_exc()







