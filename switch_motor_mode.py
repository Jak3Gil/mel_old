#!/usr/bin/env python3
"""
Switch Robstride motor to MIT/Position Control mode
"""

import serial
import time
import glob

def switch_to_mit_mode():
    """Try different mode switching commands."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   SWITCHING MOTOR TO MIT/POSITION MODE                ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')
    if not port:
        print("✗ USB adapter not found. Reconnect it.")
        return
    
    port = port[0]
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
    print("    ✓ L91 init\n")
    
    # Try mode switching commands
    print("[2] Trying MODE SWITCH commands...")
    print("="*60)
    
    mode_commands = [
        # Extended CAN ID commands (CyberGear style)
        (b'T030000010\r', "Enter Motor Mode (0x03)"),
        (b'T070000010\r', "Switch to MIT Mode (0x07)"),
        (b'T080000010\r', "Enter Servo Mode (0x08)"),
        
        # Standard ID mode commands
        (b't00180100000000000000\r', "Mode switch 0x01"),
        (b't00180300000000000000\r', "Mode switch 0x03"),
        (b't00180700000000000000\r', "Mode switch 0x07"),
        (b't00180800000000000000\r', "Mode switch 0x08"),
        
        # CANopen NMT commands
        (b't00001018\r', "NMT Start Node"),
        (b't00001810\r', "NMT Reset"),
    ]
    
    for cmd, desc in mode_commands:
        print(f"\n  {desc}")
        print(f"    Command: {cmd.strip().decode()}")
        ser.write(cmd)
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
    
    # Now try MIT commands
    print("\n\n[3] Testing MIT Commands After Mode Switch...")
    print("="*60)
    
    print("\n  Enable motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    ser.read(1000)
    
    print("  Zero position...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.2)
    ser.read(1000)
    
    print("\n  Sending STRONG position command (2 radians)...")
    print("  Using MAXIMUM gains (kp=500, kd=5)...")
    
    # Very strong command - motor MUST move if in correct mode
    for i in range(30):
        ser.write(b't0018C7FFFFF50000FFFF\r')  # pos=2rad, max gains
        time.sleep(0.03)
        
        if i == 0 and ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"  Response: {resp.hex()}")
    
    print("\n  ✓ Commands sent")
    
    # Disable
    print("\n  Disabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.1)
    
    ser.close()
    
    print("\n" + "="*60)
    print("MODE SWITCH ATTEMPTS COMPLETE")
    print("="*60)
    print("\n🔍 DID THE MOTOR MOVE?")
    print("\nIf YES: We found the right mode!")
    print("If NO: Check Robstride PC software - what mode is selected?")

if __name__ == "__main__":
    switch_to_mit_mode()
    
    print("\n\n💡 IF STILL NOT MOVING:")
    print("="*60)
    print("\nIn Robstride PC Software, look for:")
    print("  • 'Operating Mode' or 'Control Mode' setting")
    print("  • Should show: MIT / Position / Servo mode")
    print("  • NOT: CANopen / Fieldbus / Torque-only mode")
    print("\nSwitch to MIT/Position mode in software, then:")
    print("  python3 robstride_motor.py")

