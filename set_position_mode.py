#!/usr/bin/env python3
"""
Set Robstride motor to Position Control Mode
Using CANopen SDO Write to parameter 0x7005
"""

import serial
import time
import glob
import struct

def set_position_mode():
    """Set motor to Position Control Mode via CANopen."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   SET ROBSTRIDE MOTOR TO POSITION MODE                ║")
    print("║   Using CANopen SDO Write to 0x7005                   ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')
    if not port:
        print("✗ USB adapter not found. Reconnect it.")
        return False
    
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
    print("    ✓ L91 initialized\n")
    
    motor_id = 0x01
    
    # CANopen SDO Write to 0x7005 (run_mode parameter)
    # SDO Write: COB-ID = 0x600 + NodeID
    # Format: [CMD][Index_Lo][Index_Hi][SubIndex][Data_Bytes...]
    # CMD = 0x2B for 1 byte write, 0x2F for 2 bytes, 0x23 for 4 bytes
    
    print("[2] Setting run_mode to Position Control (0x7005 = 1)...")
    print("    CANopen SDO Write Command\n")
    
    # Construct SDO write message
    # 0x2F = Write 1 byte
    # 0x7005 = index (0x05, 0x70 in little endian)
    # 0x00 = subindex
    # 0x01 = Position mode value
    
    sdo_cob_id = 0x600 + motor_id  # 0x601
    
    # SDO Write: 1 byte to 0x7005, subindex 0, value 1
    sdo_data = bytes([
        0x2F,  # Command: write 1 byte
        0x05,  # Index low byte
        0x70,  # Index high byte  
        0x00,  # Subindex
        0x01,  # Value: Position mode (1)
        0x00,  # Padding
        0x00,  # Padding
        0x00   # Padding
    ])
    
    # Format as SLCAN: tXXXLDD...DD
    msg = f"t{sdo_cob_id:03X}8{sdo_data.hex().upper()}\r"
    print(f"    SLCAN Message: {msg.strip()}")
    
    ser.write(msg.encode())
    time.sleep(0.3)
    
    # Check for SDO response (0x580 + NodeID)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    ✓ Response: {resp.hex()}")
        if b'580' in resp or b'581' in resp:
            print("    ✓ SDO Write confirmed!")
    else:
        print("    (no immediate response)")
    
    # Save parameters (some motors need this)
    print("\n[3] Saving parameters to EEPROM...")
    # CANopen "Store Parameters" command (0x1010)
    save_data = bytes([
        0x23,  # Write 4 bytes
        0x10,  # Index low
        0x10,  # Index high
        0x01,  # Subindex (save all)
        0x65, 0x76, 0x61, 0x73  # "save" in ASCII
    ])
    
    msg = f"t{sdo_cob_id:03X}8{save_data.hex().upper()}\r"
    ser.write(msg.encode())
    time.sleep(0.5)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"    Response: {resp.hex()}")
    
    print("\n[4] Testing motor in Position Mode...")
    print("="*60)
    
    # Enable motor
    print("\n  Enabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    ser.read(1000)
    
    # Zero position
    print("  Setting zero position...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.2)
    ser.read(1000)
    
    # Position command with high gains
    print("\n  Sending position command (1 radian)...")
    for i in range(30):
        ser.write(b't00188A3D7FF666666A7FF\r')
        time.sleep(0.03)
    
    print("  ✓ Commands sent")
    
    # Disable
    print("\n  Disabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.1)
    
    ser.close()
    
    print("\n" + "="*60)
    print("POSITION MODE SET!")
    print("="*60)
    print("\n🔍 DID THE MOTOR MOVE THIS TIME?")
    print("\nIf YES: Success! Motor is now in Position mode")
    print("If NO: Power cycle motor and try again:")
    print("       python3 robstride_motor.py")
    
    return True

if __name__ == "__main__":
    success = set_position_mode()
    
    if success:
        print("\n\n💡 To use the motor now:")
        print("="*60)
        print("\n1. Power cycle the motor (OFF 10s, then ON)")
        print("2. Run: python3 robstride_motor.py")
        print("\nThe motor should now respond to position commands!")

