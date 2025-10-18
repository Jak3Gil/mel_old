#!/usr/bin/env python3
"""
CAN ID Scanner
Tries to find what CAN ID your motor is actually using
"""

import serial
import time
import glob


def scan_can_ids():
    """Scan for motor on different CAN IDs."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   CAN ID SCANNER                                       ║")
    print("║   Finding your motor's actual CAN ID                   ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found")
        return
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=0.3)
    time.sleep(0.5)
    
    # Initialize
    print("Initializing CAN adapter...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    print("✓ CAN bus ready\n")
    
    print("Scanning CAN IDs 0x01 to 0x0F...")
    print("(This will try to enable motor on each ID)\n")
    
    found_ids = []
    
    for can_id in range(0x01, 0x10):
        # Try enable command on this ID
        msg = f"t{can_id:03X}8FFFFFFFFFFFFFFFC\r"
        ser.write(msg.encode())
        time.sleep(0.2)
        
        response = ser.read(1000)
        
        if len(response) > 0:
            print(f"  CAN ID 0x{can_id:02X}: ✓ RESPONSE RECEIVED!")
            print(f"             Response: {response}")
            found_ids.append(can_id)
        else:
            print(f"  CAN ID 0x{can_id:02X}: No response")
    
    # Try to disable on found IDs
    for can_id in found_ids:
        msg = f"t{can_id:03X}8FFFFFFFFFFFFFFFD\r"
        ser.write(msg.encode())
        time.sleep(0.1)
        ser.read(1000)
    
    ser.close()
    
    print("\n" + "=" * 60)
    print("SCAN COMPLETE")
    print("=" * 60)
    
    if found_ids:
        print(f"\n✓ Found motor(s) on CAN ID(s): {[f'0x{i:02X}' for i in found_ids]}")
        print("\n📝 UPDATE YOUR CONFIG:")
        print(f"   Edit motor_config.yaml and change:")
        print(f"   can_id: 0x{found_ids[0]:02X}")
    else:
        print("\n✗ NO MOTOR RESPONDED on any CAN ID 0x01-0x0F")
        print("\n  Possible causes:")
        print("  1. Motor needs PC software initialization first")
        print("  2. CANH/CANL wiring issue")
        print("  3. Motor powered off or in error state")
        print("  4. CAN termination missing (120Ω resistor)")
        print("  5. Motor using non-standard CAN ID > 0x0F")
        
        print("\n  🔧 IMMEDIATE CHECKS:")
        print("     • Is 12V power supply ON and showing 12V?")
        print("     • Does ammeter show ANY current when motor powered?")
        print("     • Can you rotate motor shaft by hand easily?")
        print("     • LED status: Solid red + blinking blue?")


if __name__ == "__main__":
    try:
        scan_can_ids()
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

