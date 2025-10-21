#!/usr/bin/env python3
"""
Test accurate position reading with fixed payload extraction
"""

import serial
import time
import struct

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
    
    def read_position():
        """Read motor position."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        ser.reset_input_buffer()
        ser.write(cmd)
        time.sleep(0.05)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            print(f"Response: {response.hex()}")
            
            # Extract payload (skip AT and headers)
            if len(response) >= 15:
                payload = response[7:15]  # 8 bytes of motor data
                print(f"Payload:  {payload.hex()}")
                
                if len(payload) >= 2:
                    pos_raw = struct.unpack('<h', payload[0:2])[0]
                    print(f"Position: {pos_raw}\n")
                    return pos_raw
        
        return None
    
    # Read initial position
    print("[1] Initial position:")
    pos1 = read_position()
    time.sleep(1)
    
    # Move forward 2 seconds
    print("[2] Moving forward 2 seconds...")
    jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
    cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
    
    ser.write(cmd)
    time.sleep(2.0)
    
    # Stop and read
    print("\n[3] Position after movement:")
    pos2 = read_position()
    
    # Compare
    if pos1 is not None and pos2 is not None:
        delta = pos2 - pos1
        print(f"[RESULT] Motor moved: {delta:+d} encoder counts")
        
        if delta != 0:
            print("\n[SUCCESS] Position reading is WORKING!")
            print("The calibration will use real encoder values!")
        else:
            print("\n[WARNING] Position didn't change")
            print("Motor might not have moved or decoder needs adjustment")
    
    ser.close()

except Exception as e:
    print(f"[ERROR] {e}")
    import traceback
    traceback.print_exc()

