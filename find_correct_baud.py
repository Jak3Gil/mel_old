#!/usr/bin/env python3
"""
Find the correct baud rate by testing common values.
"""

import serial
import time

def test_baud(port, baud):
    """Test a specific baud rate."""
    try:
        ser = serial.Serial(port, baud, timeout=1.0)
        time.sleep(0.3)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Send AT+AT
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            ser.close()
            return True, resp
        
        ser.close()
        return False, None
    except Exception as e:
        return False, str(e)

def main():
    """Test all common baud rates."""
    PORT = '/dev/cu.usbserial-110'  # Use cu device on macOS
    
    # Common baud rates (most common first)
    BAUD_RATES = [
        115200,
        921600,
        460800,
        230400,
        9600,
        19200,
        38400,
        57600,
        1000000,
        1500000,
        2000000
    ]
    
    print("=" * 70)
    print("   BAUD RATE DETECTION")
    print("=" * 70 + "\n")
    print(f"Testing port: {PORT}\n")
    
    for baud in BAUD_RATES:
        print(f"Testing {baud:>8} baud...", end=" ", flush=True)
        success, data = test_baud(PORT, baud)
        
        if success:
            print(f"✓ SUCCESS!")
            print(f"  Response: {data.decode('ascii', errors='ignore').strip()}")
            print(f"  Hex: {data.hex()}")
            print(f"\n{'='*70}")
            print(f"FOUND IT! Use baud rate: {baud}")
            print(f"{'='*70}")
            return baud
        else:
            print("✗ No response")
    
    print(f"\n{'='*70}")
    print("No working baud rate found!")
    print("=" * 70)
    print("\nPossible issues:")
    print("  1. Motors not powered on")
    print("  2. USB-CAN adapter not connected properly")
    print("  3. Wrong serial port (check: ls /dev/tty.usb*)")
    print("  4. Adapter needs special initialization")
    return None

if __name__ == "__main__":
    main()

