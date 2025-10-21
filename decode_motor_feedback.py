#!/usr/bin/env python3
"""
Decode Motor Feedback - Find correct position format
"""

import serial
import time
import struct

def analyze_feedback():
    """Analyze motor feedback to find position encoding."""
    print("=" * 70)
    print("   MOTOR FEEDBACK ANALYSIS")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    motor13_encoding = bytes([0xe8, 0x6c])
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Initialize
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        # Activate motor 13
        ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        print("Motor 13 activated\n")
        time.sleep(1)
        
        def get_feedback():
            """Get motor feedback."""
            stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
            cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
            
            ser.reset_input_buffer()
            ser.write(cmd)
            time.sleep(0.05)
            
            if ser.in_waiting > 0:
                return ser.read(ser.in_waiting)
            return None
        
        # Read initial feedback
        print("[1] Reading initial feedback...")
        resp1 = get_feedback()
        if resp1:
            print(f"    Full response: {resp1.hex()}")
            print(f"    Length: {len(resp1)} bytes")
            print(f"    ASCII: {resp1.decode('ascii', errors='ignore')}")
            
            # Extract the 8-byte payload
            if len(resp1) >= 15:
                # Skip "AT" and headers, get 8-byte payload
                payload = resp1[9:17] if len(resp1) >= 17 else resp1[-10:-2]
                print(f"    Payload (8 bytes): {payload.hex()}")
                print(f"    Bytes: {' '.join(f'{b:02x}' for b in payload)}")
                
                # Try different decodings
                print(f"\n    Decoding attempts:")
                print(f"      Bytes[0:2] big-endian:    {struct.unpack('>h', payload[0:2])[0]}")
                print(f"      Bytes[0:2] little-endian: {struct.unpack('<h', payload[0:2])[0]}")
                print(f"      Bytes[2:4] big-endian:    {struct.unpack('>h', payload[2:4])[0]}")
                print(f"      Bytes[2:4] little-endian: {struct.unpack('<h', payload[2:4])[0]}")
                print(f"      Bytes[4:6] big-endian:    {struct.unpack('>h', payload[4:6])[0]}")
                print(f"      Bytes[4:6] little-endian: {struct.unpack('<h', payload[4:6])[0]}")
        
        # Move motor forward
        print("\n[2] Moving motor forward 2 seconds...")
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        
        ser.write(cmd)
        time.sleep(2.0)
        
        # Stop and read
        resp2 = get_feedback()
        if resp2:
            print(f"    Full response: {resp2.hex()}")
            
            if len(resp2) >= 15:
                payload = resp2[9:17] if len(resp2) >= 17 else resp2[-10:-2]
                print(f"    Payload (8 bytes): {payload.hex()}")
                print(f"    Bytes: {' '.join(f'{b:02x}' for b in payload)}")
                
                print(f"\n    Decoding attempts:")
                print(f"      Bytes[0:2] big-endian:    {struct.unpack('>h', payload[0:2])[0]}")
                print(f"      Bytes[0:2] little-endian: {struct.unpack('<h', payload[0:2])[0]}")
                print(f"      Bytes[2:4] big-endian:    {struct.unpack('>h', payload[2:4])[0]}")
                print(f"      Bytes[2:4] little-endian: {struct.unpack('<h', payload[2:4])[0]}")
                print(f"      Bytes[4:6] big-endian:    {struct.unpack('>h', payload[4:6])[0]}")
                print(f"      Bytes[4:6] little-endian: {struct.unpack('<h', payload[4:6])[0]}")
        
        # Compare
        if resp1 and resp2:
            print("\n[3] Comparing before/after movement...")
            
            payload1 = resp1[9:17] if len(resp1) >= 17 else resp1[-10:-2]
            payload2 = resp2[9:17] if len(resp2) >= 17 else resp2[-10:-2]
            
            print("\n    Payload comparison:")
            for i in range(8):
                if payload1[i] != payload2[i]:
                    print(f"      Byte[{i}]: {payload1[i]:02x} -> {payload2[i]:02x} (CHANGED!)")
                else:
                    print(f"      Byte[{i}]: {payload1[i]:02x} -> {payload2[i]:02x} (same)")
            
            # Check which bytes changed - those are likely position
            print("\n    Bytes that changed are likely position/velocity data!")
        
        ser.close()
        
        print("\n" + "=" * 70)
        print("ANALYSIS COMPLETE")
        print("=" * 70)
        print("\nThe bytes that CHANGED contain position/velocity data.")
        print("Use this to decode position correctly in calibration!")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("\nMotor Feedback Decoder\n")
    analyze_feedback()

