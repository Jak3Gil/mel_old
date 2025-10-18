#!/usr/bin/env python3
"""
Force reset USB-CAN adapter
"""

import serial
import time

def force_reset():
    """Try to force adapter out of stuck state."""
    port = '/dev/cu.usbserial-10'
    
    print("Attempting force reset...\n")
    
    # Try multiple times with different settings
    for attempt in range(3):
        print(f"Attempt {attempt + 1}/3...")
        
        for baud in [115200, 9600, 19200, 38400, 57600]:
            try:
                # Open with very short timeout
                ser = serial.Serial(port, baud, timeout=0.1)
                
                # Send various reset commands
                for cmd in [b'\x1b', b'C\r', b'c\r', b'\r\n', b'AT\r']:
                    ser.write(cmd)
                    time.sleep(0.05)
                
                # Close and immediately reopen
                ser.close()
                time.sleep(0.1)
                
            except Exception as e:
                pass
    
    # Final test with L91
    print("\nTesting with L91 sequence...")
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        ser.write(b'C\r')
        time.sleep(0.1)
        resp1 = ser.read(100)
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        resp2 = ser.read(100)
        
        ser.write(b'O\r')
        time.sleep(0.1)
        resp3 = ser.read(100)
        
        if resp1 or resp2 or resp3:
            print(f"✓ Got responses!")
            print(f"  C: {resp1.hex() if resp1 else 'none'}")
            print(f"  L91: {resp2.hex() if resp2 else 'none'}")
            print(f"  O: {resp3.hex() if resp3 else 'none'}")
            
            # Try motor enable
            ser.write(b't0018FFFFFFFFFFFFFFFC\r')
            time.sleep(0.2)
            resp = ser.read(100)
            if resp:
                print(f"✓✓✓ Motor responding: {resp.hex()}")
                ser.close()
                return True
        else:
            print("✗ Still no response")
        
        ser.close()
        
    except Exception as e:
        print(f"✗ Error: {e}")
    
    print("\n" + "="*60)
    print("ADAPTER STILL STUCK")
    print("="*60)
    print("\nYOU MUST:")
    print("  1. CLOSE this terminal completely")
    print("  2. CLOSE Robstride PC software if open")
    print("  3. UNPLUG USB-CAN adapter from Mac")
    print("  4. Wait 10 seconds")
    print("  5. PLUG it back in")
    print("  6. Open new terminal and try again")
    
    return False

if __name__ == "__main__":
    force_reset()

