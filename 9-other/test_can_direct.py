#!/usr/bin/env python3
"""
Try direct CAN commands without AT protocol wrapper.
This might work if the adapter is in "transparent CAN" mode.
"""

import serial
import struct
import time

def test_direct_can():
    """Test sending raw CAN frames directly."""
    PORT = '/dev/cu.usbserial-110'
    BAUD = 1000000  # CAN bus standard 1Mbps
    
    print("=" * 70)
    print("   TESTING DIRECT CAN MODE")
    print("=" * 70 + "\n")
    
    try:
        # Try multiple baud rates that are CAN-standard
        for baud in [1000000, 500000, 250000, 921600]:
            print(f"\nTrying baud rate: {baud}")
            print("-" * 70)
            
            ser = serial.Serial(PORT, baud, timeout=1.0)
            time.sleep(0.3)
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Try sending a simple CAN frame for motor 13
            # CAN ID 0x0D (13), 8 bytes of data
            # This is a "motor enable" or "status request" command
            can_frame = bytes([
                0x0D,  # CAN ID (motor 13)
                0x08,  # Data length
                0x01, 0x00, 0x00, 0x00,  # Command: Enable/Query
                0x00, 0x00, 0x00, 0x00   # Padding
            ])
            
            print(f"Sending CAN frame: {can_frame.hex()}")
            ser.write(can_frame)
            time.sleep(0.5)
            
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                print(f"✓ GOT RESPONSE!")
                print(f"  HEX: {resp.hex()}")
                print(f"  Length: {len(resp)} bytes")
                ser.close()
                return True, baud
            else:
                print("✗ No response")
            
            ser.close()
            time.sleep(0.2)
        
        return False, None
        
    except Exception as e:
        print(f"Error: {e}")
        return False, None

if __name__ == "__main__":
    print("\nDirect CAN Test\n")
    print("This tests if the adapter is in 'transparent CAN' mode")
    print("instead of AT command mode.\n")
    
    success, baud = test_direct_can()
    
    if success:
        print(f"\n{'='*70}")
        print(f"SUCCESS! Adapter responds at {baud} in direct CAN mode!")
        print("="*70)
    else:
        print(f"\n{'='*70}")
        print("No response in direct CAN mode either.")
        print("="*70)
        print("\nNext steps:")
        print("1. Are you using K-Scale Servo Studio software?")
        print("2. Can you move motors with that software?")
        print("3. What happens when you try to move them there?")
        print("4. Do you need to install: pip install actuator")
        print("   (requires Rust, but disk is full)")



