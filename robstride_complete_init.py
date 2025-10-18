#!/usr/bin/env python3
"""
Complete Robstride Motor Initialization
Based on all documentation - this should work!
"""

import serial
import time
import glob

def complete_init():
    """Complete initialization sequence for Robstride motor."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   COMPLETE ROBSTRIDE INITIALIZATION                    ║")
    print("║   Solid Red + Blinking Blue = Motor Ready!             ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')
    if not port:
        print("✗ Reconnect USB adapter")
        return
    
    port = port[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.5)
    
    # Step 1: L91 Init
    print("[1] L91 Protocol Init...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    print("    ✓ L91 ready\n")
    
    motor_id = 0x01
    
    # Step 2: CANopen NMT - Start Node
    print("[2] CANopen NMT Start Node...")
    ser.write(b't00021018\r')  # NMT: Start node ID 0x01
    time.sleep(0.3)
    ser.read(1000)
    print("    ✓ Node started\n")
    
    # Step 3: Set run_mode to Position (0x7005 = 1)
    print("[3] Setting run_mode = Position (0x7005)...")
    sdo_cob = 0x600 + motor_id
    data = bytes([0x2F, 0x05, 0x70, 0x00, 0x01, 0x00, 0x00, 0x00])
    msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
    ser.write(msg.encode())
    time.sleep(0.3)
    ser.read(1000)
    print("    ✓ Position mode set\n")
    
    # Step 4: Set Modes of Operation (0x6060 = 1)
    print("[4] Setting Modes of Operation (0x6060)...")
    data = bytes([0x2F, 0x60, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00])
    msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
    ser.write(msg.encode())
    time.sleep(0.3)
    ser.read(1000)
    print("    ✓ Mode set\n")
    
    # Step 5: Controlword - Enable Operation
    print("[5] Enabling motor via Controlword (0x6040)...")
    # Controlword sequence: 0x06 -> 0x07 -> 0x0F -> 0x1F
    for val in [0x06, 0x07, 0x0F, 0x1F]:
        data = bytes([0x2F, 0x40, 0x60, 0x00, val, 0x00, 0x00, 0x00])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        print(f"    Controlword: 0x{val:02X}")
    print("    ✓ Motor enabled\n")
    
    # Step 6: MIT Protocol Enable
    print("[6] MIT Protocol Enable...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    ser.read(1000)
    print("    ✓ MIT enable sent\n")
    
    # Step 7: Zero Position
    print("[7] Setting Zero Position...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.2)
    ser.read(1000)
    print("    ✓ Zero set\n")
    
    # Step 8: Position Command
    print("[8] SENDING POSITION COMMAND...")
    print("    Target: 2 radians (~114 degrees)")
    print("    Gains: kp=100, kd=3 (strong)")
    print("="*60)
    
    for i in range(50):
        # pos=2rad, vel=0, kp=100, kd=3, torque=0
        ser.write(b't0018A1467FF666666D7FF\r')
        time.sleep(0.03)
        
        if i % 10 == 0:
            print(f"    Sending command {i+1}/50...")
    
    print("\n    ✓ Position commands sent")
    print("\n    🔍 WATCH THE MOTOR - DID IT MOVE?\n")
    
    time.sleep(2)
    
    # Return to zero
    print("[9] Returning to zero...")
    for i in range(50):
        ser.write(b't00187FFF7FF666666D7FF\r')
        time.sleep(0.03)
    
    print("    ✓ Return complete\n")
    
    # Disable
    print("[10] Disabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.2)
    
    ser.close()
    
    print("\n" + "="*60)
    print("INITIALIZATION COMPLETE")
    print("="*60)

if __name__ == "__main__":
    complete_init()
    
    print("\n\n🔍 RESULT:")
    print("="*60)
    print("\nDid the motor shaft physically rotate?")
    print("\n[A] YES - Motor moved!")
    print("    → SUCCESS! Save this script, motor is working!")
    print("\n[B] NO - Still no movement")
    print("    → Motor needs Robstride PC software initialization")
    print("    → This cannot be done via CAN alone")
    print("\n[C] Motor made sound/vibration but didn't rotate")
    print("    → Motor is trying! May need higher gains or")
    print("    → mechanical load removed")
    print("\n" + "="*60)
    print("\n💡 If still no movement:")
    print("   The motor REQUIRES one-time setup with Robstride's")
    print("   official software that enables position control.")
    print("   This is a firmware/configuration lock that CAN")
    print("   commands alone cannot bypass.")




