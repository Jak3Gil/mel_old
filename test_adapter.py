#!/usr/bin/env python3
"""
Test if USB-CAN adapter is responding at all
"""

import serial
import time

def test_adapter():
    """Test adapter with multiple approaches."""
    print("Testing USB-CAN Adapter...\n")
    
    port = '/dev/cu.usbserial-10'
    
    for baud in [115200, 921600, 500000]:
        print(f"Trying {baud} baud...")
        try:
            ser = serial.Serial(port, baud, timeout=0.5)
            time.sleep(0.2)
            
            # Try multiple commands
            for cmd in [b'V\r', b'v\r', b'C\r', b'L91\r', b'S8\r']:
                ser.write(cmd)
                time.sleep(0.1)
                resp = ser.read(100)
                if resp:
                    print(f"  ✓ {cmd.strip()} got response: {resp.hex()}")
                    ser.close()
                    return True
            
            ser.close()
        except Exception as e:
            print(f"  ✗ Error: {e}")
    
    print("\n✗ Adapter not responding to any commands")
    print("\nThis means:")
    print("  1. Adapter needs to be unplugged/replugged")
    print("  2. OR different software is using it")
    print("  3. OR adapter firmware is in bad state")
    
    return False

if __name__ == "__main__":
    test_adapter()

