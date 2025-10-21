#!/usr/bin/env python3
"""
Debug Motor 14 response format
"""

import serial
import time

PORT = 'COM3'
BAUD = 921600
motor14_encoding = bytes([0xe8, 0x74])

try:
    ser = serial.Serial(PORT, BAUD, timeout=1.0)
    time.sleep(0.5)
    ser.reset_input_buffer()
    
    # Initialize
    ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    
    # Activate motor 14
    ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
    time.sleep(0.3)
    ser.read(1000)
    
    print("Motor 14 activated\n")
    
    print("Sending stop command to Motor 14 to get feedback...")
    
    stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
    cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
    
    ser.reset_input_buffer()
    ser.write(cmd)
    time.sleep(0.1)
    
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting)
        print(f"Full response: {response.hex()}")
        print(f"Length: {len(response)} bytes")
        print(f"ASCII: {response.decode('ascii', errors='ignore')}")
        
        # Show byte by byte
        print(f"\nByte-by-byte:")
        for i, b in enumerate(response):
            print(f"  [{i:2d}] 0x{b:02x} = {b:3d} '{chr(b) if 32 <= b < 127 else '.'}'")
        
        # Try extracting payload at different positions
        print(f"\nTrying different payload positions:")
        for start_pos in range(max(0, len(response) - 10), len(response) - 1):
            if start_pos + 2 <= len(response):
                try:
                    import struct
                    pos_raw = struct.unpack('<h', response[start_pos:start_pos+2])[0]
                    print(f"  Bytes[{start_pos}:{start_pos+2}] = {pos_raw}")
                except:
                    pass
    else:
        print("NO RESPONSE from Motor 14!")
        print("Motor 14 might not be responding to this command format")
    
    ser.close()

except Exception as e:
    print(f"[ERROR] {e}")
    import traceback
    traceback.print_exc()

