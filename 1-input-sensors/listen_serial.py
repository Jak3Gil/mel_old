#!/usr/bin/env python3
"""
Just listen to the serial port to see if anything is coming through.
"""

import serial
import time

def listen(port, baud=115200, duration=5):
    """Listen to serial port for any data."""
    print(f"Opening {port} at {baud} baud...")
    print(f"Listening for {duration} seconds...")
    print("Press Ctrl+C to stop early\n")
    print("="*70)
    
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        start_time = time.time()
        data_received = False
        
        while (time.time() - start_time) < duration:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                data_received = True
                print(f"\n[{time.time() - start_time:.2f}s] Received {len(data)} bytes:")
                print(f"  HEX: {data.hex()}")
                print(f"  ASCII: {data.decode('ascii', errors='ignore')}")
        
        ser.close()
        
        if not data_received:
            print("\n✗ No data received")
            print("\nThis means:")
            print("  - The adapter isn't sending any data")
            print("  - Or the motors aren't powered/connected")
        else:
            print("\n✓ Data was received!")
        
        print("="*70)
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    print("\nSerial Port Listener\n")
    print("This will listen for ANY data on the serial port.\n")
    
    # Try both cu and tty
    for port in ['/dev/cu.usbserial-110', '/dev/tty.usbserial-110']:
        print(f"\n{'='*70}")
        print(f"Testing: {port}")
        print('='*70 + "\n")
        listen(port, baud=115200, duration=3)
        time.sleep(1)
    
    print("\n\n" + "="*70)
    print("IMPORTANT QUESTIONS:")
    print("="*70)
    print("1. Are the motors POWERED ON? (LEDs lit?)")
    print("2. Can you move them with Robstride software on this Mac?")
    print("3. What type of USB-CAN adapter are you using?")
    print("4. Is the USB cable plugged in securely?")



