#!/usr/bin/env python3
"""
Test different baud rates to find the correct one
"""

import serial
import time

def test_baud_rate(baud):
    """Test a specific baud rate."""
    PORT = 'COM3'
    
    try:
        print(f"\n{'='*60}")
        print(f"Testing baud rate: {baud}")
        print(f"{'='*60}")
        
        ser = serial.Serial(PORT, baud, timeout=1.0)
        time.sleep(0.5)
        
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Send AT+AT handshake
        print("  Sending AT+AT...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"  *** GOT RESPONSE: {resp.hex()} ***")
            print(f"  ASCII: {resp.decode('ascii', errors='ignore')}")
            ser.close()
            return True
        else:
            print(f"  No response")
        
        # Try scan command
        print("  Sending scan command...")
        scan_cmd = bytes([
            0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a
        ])
        ser.write(scan_cmd)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"  *** GOT RESPONSE: {resp.hex()} ***")
            ser.close()
            return True
        else:
            print(f"  No response")
        
        ser.close()
        return False
        
    except Exception as e:
        print(f"  Error: {e}")
        return False

def main():
    """Test common baud rates."""
    print("="*60)
    print("BAUD RATE TEST")
    print("Testing different baud rates to find correct one")
    print("="*60)
    
    # Common baud rates
    baud_rates = [
        9600,      # Very common
        19200,     # Common
        38400,     # Common
        57600,     # Common
        115200,    # What I've been using
        230400,    # High speed
        460800,    # Very high speed
        921600,    # Maximum common
        1000000,   # CAN bus speed sometimes
        2000000,   # Very high
    ]
    
    working_bauds = []
    
    for baud in baud_rates:
        if test_baud_rate(baud):
            working_bauds.append(baud)
            print(f"\n  >>> BAUD {baud} GOT A RESPONSE! <<<\n")
        time.sleep(0.5)
    
    print("\n" + "="*60)
    print("RESULTS")
    print("="*60)
    
    if working_bauds:
        print(f"\nBaud rates that got responses: {working_bauds}")
        print(f"\nTry using baud rate: {working_bauds[0]}")
    else:
        print("\nNo responses at any baud rate!")
        print("\nPossible reasons:")
        print("  1. Motor needs to be initialized by Robstride first")
        print("  2. Wrong COM port")
        print("  3. Motor not powered")
        print("  4. Different protocol needed")
        print("\nWhat to check:")
        print("  - Is motor powered? (LED on?)")
        print("  - Is COM3 the right port? (Check Device Manager)")
        print("  - Does motor work with Robstride software?")

if __name__ == "__main__":
    print("\nRobstride Motor - Baud Rate Test\n")
    print("This will try different baud rates to find the right one.\n")
    time.sleep(1)
    
    main()

