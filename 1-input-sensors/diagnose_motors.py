#!/usr/bin/env python3
"""
Diagnose motor communication - see what's actually happening.
"""

import serial
import time

def hex_dump(data, label=""):
    """Pretty print hex data."""
    if label:
        print(f"  {label}")
    hex_str = ' '.join(f'{b:02x}' for b in data)
    ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data)
    print(f"    HEX: {hex_str}")
    print(f"    ASCII: {ascii_str}")
    print(f"    Length: {len(data)} bytes")

def diagnose():
    """Diagnose motor communication."""
    PORT = '/dev/tty.usbserial-110'
    BAUD = 921600
    
    print("=" * 70)
    print("   MOTOR COMMUNICATION DIAGNOSTIC")
    print("=" * 70 + "\n")
    
    try:
        # Connect
        print(f"Connecting to {PORT} at {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=2.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("✓ Connected\n")
        
        # Test 1: AT+AT handshake
        print("[Test 1] AT+AT Handshake")
        print("-" * 70)
        cmd = bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a])
        hex_dump(cmd, "Sending:")
        ser.write(cmd)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            hex_dump(resp, "Received:")
            print("✓ Got response!\n")
        else:
            print("✗ No response\n")
        
        time.sleep(0.5)
        
        # Test 2: Scan Motor 13
        print("[Test 2] Scan for Motor 13 (CAN ID 13)")
        print("-" * 70)
        cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a])
        hex_dump(cmd, "Sending:")
        ser.write(cmd)
        time.sleep(1.0)  # Wait longer
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            hex_dump(resp, "Received:")
            print("✓ Motor 13 responded!\n")
        else:
            print("✗ No response from Motor 13\n")
        
        time.sleep(0.5)
        
        # Test 3: Scan Motor 14
        print("[Test 3] Scan for Motor 14 (CAN ID 14)")
        print("-" * 70)
        cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x70, 0x01, 0x00, 0x0d, 0x0a])
        hex_dump(cmd, "Sending:")
        ser.write(cmd)
        time.sleep(1.0)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            hex_dump(resp, "Received:")
            print("✓ Motor 14 responded!\n")
        else:
            print("✗ No response from Motor 14\n")
        
        time.sleep(0.5)
        
        # Test 4: Try alternative scan - scan all motors
        print("[Test 4] Broadcast scan (all motors)")
        print("-" * 70)
        # Try scanning with broadcast
        for can_id in [13, 14, 15]:
            # Calculate encoding
            encoded = bytes([0xe8, 0x6c + (can_id - 13) * 4])
            cmd = bytes([0x41, 0x54, 0x00, 0x07]) + encoded + bytes([0x01, 0x00, 0x0d, 0x0a])
            print(f"\nTrying CAN ID {can_id}: encoding = {encoded.hex()}")
            hex_dump(cmd, "Sending:")
            ser.write(cmd)
            time.sleep(0.8)
            
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                hex_dump(resp, "Received:")
                print(f"✓ Got response for ID {can_id}!")
            else:
                print(f"✗ No response for ID {can_id}")
        
        print("\n" + "=" * 70)
        
        # Test 5: Try movement command anyway
        print("\n[Test 5] Attempting movement (Motor 13) without scan response")
        print("-" * 70)
        print("Sending JOG forward command...")
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        hex_dump(jog_cmd, "Command:")
        ser.write(jog_cmd)
        print("\n>>> DID MOTOR 13 MOVE? (watching for 1 second) <<<")
        time.sleep(1.0)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd)
        print("Stop command sent\n")
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            hex_dump(resp, "Response:")
        
        ser.close()
        
        print("\n" + "=" * 70)
        print("DIAGNOSTIC COMPLETE")
        print("=" * 70)
        print("\nQuestions:")
        print("  1. Did you see 'OK' response to AT+AT?")
        print("  2. Did motors respond to scan?")
        print("  3. Did Motor 13 move during Test 5?")
        print("  4. Are the motors powered on? (check LEDs)")
        print("  5. Can you move them with Robstride software?")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    diagnose()



