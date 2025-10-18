#!/usr/bin/env python3
"""
Motor Diagnostic - Show responses from motor
"""

import serial
import time
import glob

def find_usb_port():
    """Find USB-CAN adapter."""
    ports = glob.glob('/dev/cu.usbserial*')
    return ports[0] if ports else None

def test_with_responses():
    """Test motor and show all responses."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   MOTOR DIAGNOSTIC - Checking Responses               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = find_usb_port()
    if not port:
        print("✗ No USB adapter found!")
        return
    
    print(f"Port: {port}\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        # Initialize with L91
        print("[1] Initializing with L91...")
        ser.write(b'C\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    C response: {resp.hex() if resp else '(none)'}")
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    L91 response: {resp.hex() if resp else '(none)'}")
        if resp:
            print(f"    ASCII: {repr(resp)}")
        
        ser.write(b'O\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    O response: {resp.hex() if resp else '(none)'}")
        
        # Enable motor
        print("\n[2] Sending Enable command...")
        ser.write(b't0018FFFFFFFFFFFFFFFC\r')
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    ✓ Motor responded!")
            print(f"    Hex: {resp.hex().upper()}")
            print(f"    Length: {len(resp)} bytes")
            print(f"    ASCII: {repr(resp)}")
        else:
            print(f"    ✗ No response from motor!")
            print(f"    This means motor is NOT communicating")
        
        # Zero position
        print("\n[3] Sending Zero Position command...")
        ser.write(b't0018FFFFFFFFFFFFFFFE\r')
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    ✓ Response: {resp.hex().upper()}")
        else:
            print(f"    ✗ No response")
        
        # Position command with higher gains
        print("\n[4] Sending Position Command (1 radian, HIGH GAINS)...")
        # Higher gains: kp=100, kd=3
        ser.write(b't00188A3D7FF666666A7FF\r')
        time.sleep(0.1)
        
        print("    Sending 20 position commands...")
        response_count = 0
        for i in range(20):
            ser.write(b't00188A3D7FF666666A7FF\r')
            time.sleep(0.05)
            
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                response_count += 1
                if i == 0:  # Print first response
                    print(f"    ✓ Response: {data.hex().upper()}")
        
        print(f"    Total responses: {response_count}/20")
        
        if response_count == 0:
            print("\n    ⚠️  Motor is NOT responding to position commands!")
        
        # Disable
        print("\n[5] Disabling motor...")
        ser.write(b't0018FFFFFFFFFFFFFFFD\r')
        time.sleep(0.1)
        
        ser.close()
        
        print("\n" + "="*60)
        if response_count > 0:
            print("DIAGNOSIS: Motor is communicating but may not be moving")
            print("="*60)
            print("\nPossible reasons:")
            print("  1. Motor might be in wrong mode (need to switch to MIT mode)")
            print("  2. Control gains might need adjustment")
            print("  3. Motor might have load/mechanical resistance")
            print("  4. Position feedback might not be calibrated")
            print("\n💡 Try running the Robstride PC software and:")
            print("  - Check if motor moves there")
            print("  - Check motor mode (should be MIT/Position Control)")
            print("  - Calibrate motor encoder if option available")
        else:
            print("DIAGNOSIS: Motor is NOT communicating at all")
            print("="*60)
            print("\nTroubleshooting:")
            print("  1. Power cycle the motor")
            print("  2. Check CANH/CANL wiring")
            print("  3. Verify 120Ω termination resistor")
            print("  4. Close Robstride PC software if running")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_with_responses()

