#!/usr/bin/env python3
"""
Try to exit bootloader mode if motor is stuck
"""

import can
import time

print("=" * 60)
print("BOOTLOADER EXIT ATTEMPT")
print("=" * 60)
print()

COM_PORT = "COM4"

try:
    bus = can.interface.Bus(
        interface='slcan',
        channel=COM_PORT,
        bitrate=1000000
    )
    print("[OK] Connected to CAN bus")
    print()
    
    # Try various bootloader exit / reset commands
    commands = [
        # (CAN_ID, data, description)
        (0x000, [0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA], "Bootloader exit pattern 1"),
        (0x001, [0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA], "Bootloader exit ID 1"),
        (0x7FF, [0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55], "Reset pattern"),
        (0x001, [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01], "Mode reset"),
        (0x000, [0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00], "Sync pattern"),
    ]
    
    print("Trying bootloader exit commands...")
    print("-" * 60)
    
    for can_id, data, desc in commands:
        print(f"\nSending: {desc}")
        print(f"  CAN ID: 0x{can_id:03X}, Data: {bytes(data).hex()}")
        
        msg = can.Message(
            arbitration_id=can_id,
            data=data,
            is_extended_id=False
        )
        
        bus.send(msg)
        time.sleep(0.2)
        
        # Check for response
        response = bus.recv(timeout=0.5)
        if response:
            print(f"  [RESPONSE!] ID: 0x{response.arbitration_id:03X}")
            print(f"              Data: {response.data.hex()}")
            print("\n  Motor is responding! Try scanning now!")
            break
        else:
            print("  [No response]")
    
    bus.shutdown()
    
    print()
    print("=" * 60)
    print("NEXT STEPS:")
    print("=" * 60)
    print()
    print("If motor responded:")
    print("  -> Run: python scan_for_robstride_motor.py")
    print()
    print("If no response:")
    print("  -> Try extended power cycle (see instructions)")
    print("  -> Check for hardware reset button on motor")
    print("  -> Contact Robstride for firmware reflash")
    print()
    
except Exception as e:
    print(f"[X] Error: {e}")
    import traceback
    traceback.print_exc()

