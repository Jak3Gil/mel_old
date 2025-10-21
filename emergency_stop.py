#!/usr/bin/env python3
"""
EMERGENCY STOP - Stop all motors immediately
"""

import serial
import time

def emergency_stop():
    """Stop all motors NOW."""
    PORT = 'COM3'
    BAUD = 921600
    
    print("EMERGENCY STOP - Stopping all motors...")
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.5)
        time.sleep(0.1)
        
        # Stop command data
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        
        # Stop Motor 13
        stop_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08
        ]) + stop_data + bytes([0x0d, 0x0a])
        
        # Stop Motor 14
        stop_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08
        ]) + stop_data + bytes([0x0d, 0x0a])
        
        # Send stop to both motors multiple times
        for i in range(5):
            ser.write(stop_13)
            time.sleep(0.01)
            ser.write(stop_14)
            time.sleep(0.01)
        
        print("STOP commands sent!")
        
        ser.close()
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    emergency_stop()

