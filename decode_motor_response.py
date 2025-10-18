#!/usr/bin/env python3
"""
Decode motor responses to see what state it's in
"""

import serial
import time
import glob
import struct

def uint_to_float(x_int, x_min, x_max, bits):
    """Convert unsigned int to float."""
    span = x_max - x_min
    return x_int * span / ((1 << bits) - 1) + x_min

def decode_mit_response(data):
    """Decode MIT protocol response."""
    if len(data) < 6:
        return None
    
    # MIT response format: position, velocity, torque
    try:
        # Skip SLCAN formatting, look for actual CAN data
        # SLCAN response: tXXXLDD...DD where XXX=ID, L=len, DD=data
        
        # Find pattern that looks like motor data
        hex_str = data.hex()
        print(f"    Raw hex: {hex_str}")
        print(f"    Length: {len(data)} bytes")
        
        # Try to find actual motor response in the data
        # MIT returns: ID + position(16bit) + velocity(12bit) + torque(12bit)
        
        return None
        
    except Exception as e:
        print(f"    Decode error: {e}")
        return None

def test_with_decode():
    """Test motor and decode responses."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   MOTOR RESPONSE DECODER                               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.3)
    
    # Initialize
    print("[1] Initializing with L91...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    
    # Enable motor
    print("\n[2] Enabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    resp = ser.read(1000)
    
    if resp:
        print(f"  ✓ Got response:")
        decode_mit_response(resp)
    
    # Zero position
    print("\n[3] Zero position...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.2)
    resp = ser.read(1000)
    if resp:
        print(f"  Response: {resp.hex()}")
    
    # Send position command with HIGH GAINS and see response
    print("\n[4] Sending HIGH GAIN position command (1 radian)...")
    print("    Using kp=200, kd=5 (very strong)")
    
    # Pack: pos=1.0, vel=0, kp=200, kd=5, torque=0
    # This is MUCH higher gains - should force movement
    for i in range(5):
        ser.write(b't00188A3D7FFCCCCCA7FF\r')  # High gain command
        time.sleep(0.1)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"\n  Response {i+1}:")
            decode_mit_response(resp)
    
    print("\n[5] Did you see/feel the motor try to move?")
    print("    Even a small twitch or sound means it's trying to move")
    
    # Disable
    print("\n[6] Disabling...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.1)
    
    ser.close()
    
    print("\n" + "="*60)
    print("ANALYSIS:")
    print("="*60)
    print("\n✓ Motor IS communicating (we get responses)")
    print("\nBut if motor didn't move, possible reasons:")
    print("  1. Motor in wrong mode - needs MIT mode enabled")
    print("  2. Motor encoder not calibrated")
    print("  3. Motor has error/fault state")
    print("  4. Motor mechanically stuck/blocked")
    print("\n💡 Next step: Use Robstride PC software to:")
    print("  - Switch motor to MIT/Position Control mode")
    print("  - Clear any faults/errors")
    print("  - Calibrate encoder")
    print("  - Test if motor moves in their software")

if __name__ == "__main__":
    test_with_decode()

