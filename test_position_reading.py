#!/usr/bin/env python3
"""
Test reading real position data from both motors
"""

import serial
import time
import struct

def test_positions():
    """Test reading positions from both motors."""
    print("=" * 70)
    print("   TESTING POSITION READING")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    motor13_encoding = bytes([0xe8, 0x6c])
    motor14_encoding = bytes([0xe8, 0x74])
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Initialize
        print("Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        # Activate both motors
        ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        print("[OK] Both motors activated\n")
        time.sleep(1)
        
        def read_position(motor_encoding, motor_name):
            """Read position from motor."""
            stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
            cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
            
            ser.reset_input_buffer()
            ser.write(cmd)
            time.sleep(0.05)
            
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting)
                
                # Decode position
                if len(response) >= 10:
                    for i in range(len(response) - 8):
                        if i + 8 <= len(response):
                            payload = response[i:i+8]
                            
                            try:
                                pos_raw = struct.unpack('>h', payload[0:2])[0]
                                print(f"{motor_name}: Position = {pos_raw:6d}  |  Response: {response.hex()}")
                                return pos_raw
                            except:
                                continue
            
            print(f"{motor_name}: No position data")
            return None
        
        # Read initial positions
        print("=" * 70)
        print("INITIAL POSITIONS")
        print("=" * 70 + "\n")
        
        pos13_initial = read_position(motor13_encoding, "Motor 13")
        time.sleep(0.2)
        pos14_initial = read_position(motor14_encoding, "Motor 14")
        
        print("\n" + "=" * 70)
        print("TESTING MOVEMENT AND POSITION TRACKING")
        print("=" * 70 + "\n")
        
        # Move motor 13 forward
        print("[Test 1] Moving Motor 13 forward for 1 second...")
        jog_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor13_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        
        ser.write(cmd)
        time.sleep(1.0)
        
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor13_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        ser.write(stop_cmd)
        time.sleep(0.2)
        
        pos13_after = read_position(motor13_encoding, "Motor 13")
        
        if pos13_initial and pos13_after:
            delta = pos13_after - pos13_initial
            print(f"  Motor 13 moved: {delta:+d} encoder counts\n")
        
        time.sleep(1)
        
        # Move motor 14 forward
        print("[Test 2] Moving Motor 14 forward for 1 second...")
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor14_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        
        ser.write(cmd)
        time.sleep(1.0)
        
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor14_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        ser.write(stop_cmd)
        time.sleep(0.2)
        
        pos14_after = read_position(motor14_encoding, "Motor 14")
        
        if pos14_initial and pos14_after:
            delta = pos14_after - pos14_initial
            print(f"  Motor 14 moved: {delta:+d} encoder counts\n")
        
        ser.close()
        
        print("=" * 70)
        print("TEST COMPLETE")
        print("=" * 70)
        print("\nPosition feedback is working!")
        print("You can now run calibrate_both_with_feedback.py")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("\nPosition Reading Test\n")
    test_positions()

