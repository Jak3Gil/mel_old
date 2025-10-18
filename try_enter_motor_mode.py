#!/usr/bin/env python3
"""
Try "Enter Motor Mode" command with extended CAN IDs
Based on CyberGear/similar motor protocols
"""

import serial
import time
import glob

def try_enter_motor_mode():
    """Try entering motor operational mode."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   TRYING ENTER MOTOR MODE COMMAND                     ║")
    print("║   Based on CyberGear/Extended ID Protocol             ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.3)
    
    # Initialize with L91
    print("[1] Initializing with L91...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    print("    ✓ L91 init complete\n")
    
    # Try extended CAN IDs for mode commands
    motor_id = 0x01
    master_id = 0x00
    
    print("[2] Trying 'Enter Motor Mode' with extended CAN IDs...")
    print("    (This is how CyberGear and similar motors work)\n")
    
    # Command 0x03 = Enter Motor Mode (extended ID format)
    # Format: T[CMD][MASTER][MOTOR]0\r
    # where CMD=03, MASTER=00, MOTOR=01
    
    commands_to_try = [
        (b'T030000010\r', "Enter Motor Mode (0x03)"),
        (b'T010000010\r', "Enable (0x01)"),
        (b'T020000010\r', "Reset/Init (0x02)"),
        (b'T060000010\r', "Zero Position (0x06)"),
    ]
    
    for cmd, desc in commands_to_try:
        print(f"  Trying: {desc}")
        print(f"    Command: {cmd.strip()}")
        ser.write(cmd)
        time.sleep(0.2)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    ✓ Response: {resp.hex()}")
        else:
            print(f"    (no response)")
        time.sleep(0.1)
    
    # Now try standard MIT commands
    print("\n[3] Now trying MIT position control...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')  # Enable
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')  # Zero
    time.sleep(0.1)
    ser.read(1000)
    
    # Position command with high gains
    print("    Sending position commands (1 radian, high gains)...")
    for i in range(20):
        ser.write(b't00188A3D7FF666666A7FF\r')
        time.sleep(0.05)
    
    print("    ✓ Commands sent\n")
    
    print("[4] Testing extended ID position control...")
    # Some motors use extended ID for position control too
    # Format: T[CMD][DATA...]\r
    ser.write(b'T020000018000080000000000\r')  # Extended position command
    time.sleep(0.2)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    Response: {resp.hex()}")
    
    ser.close()
    
    print("\n" + "="*60)
    print("DID THE MOTOR MOVE?")
    print("="*60)
    print("\nIf YES: We found the right initialization!")
    print("If NO: Motor may need:")
    print("  - Firmware upload/update")
    print("  - Factory reset via special hardware")
    print("  - Official Robstride config tool first")

if __name__ == "__main__":
    try_enter_motor_mode()
    
    print("\n\n💡 IMPORTANT:")
    print("If motor still doesn't move, it likely needs:")
    print("1. One-time setup with Robstride official software")
    print("2. Encoder calibration that can't be done via CAN")
    print("3. Motor may be locked and needs factory reset")

