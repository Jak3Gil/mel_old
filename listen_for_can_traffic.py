#!/usr/bin/env python3
"""
Listen for ANY CAN traffic on the bus
"""

import can
import time

print("=" * 60)
print("LISTENING FOR CAN TRAFFIC")
print("=" * 60)
print()

COM_PORT = "COM4"

try:
    print(f"Connecting to {COM_PORT}...")
    bus = can.interface.Bus(
        interface='slcan',
        channel=COM_PORT,
        bitrate=1000000
    )
    print("[OK] Connected")
    print()
    print("Listening for CAN messages for 5 seconds...")
    print("(Motor should send periodic status messages)")
    print("-" * 60)
    
    bus.set_filters([{"can_id": 0x000, "can_mask": 0x000, "extended": False}])  # Accept all
    
    message_count = 0
    start_time = time.time()
    
    while time.time() - start_time < 5:
        msg = bus.recv(timeout=0.1)
        if msg:
            message_count += 1
            print(f"[{message_count}] ID: 0x{msg.arbitration_id:03X} Data: {msg.data.hex()}")
    
    print("-" * 60)
    print()
    
    if message_count > 0:
        print(f"[SUCCESS] Received {message_count} CAN message(s)!")
        print("Motor IS on the CAN bus!")
    else:
        print("[X] NO CAN messages received")
        print()
        print("This means:")
        print("  1. Motor not sending data")
        print("  2. Still a wiring issue")
        print("  3. CAN termination missing")
        print("  4. GND connection missing")
        print()
        print("Try:")
        print("  - Power cycle motor (OFF 10 sec, ON)")
        print("  - Verify GND wire connected")
        print("  - Check for 120 ohm termination")
    
    bus.shutdown()
    
except Exception as e:
    print(f"[X] Error: {e}")
    import traceback
    traceback.print_exc()

