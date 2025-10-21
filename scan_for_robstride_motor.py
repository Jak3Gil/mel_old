#!/usr/bin/env python3
"""
Scan CAN bus for Robstride motors
Try to find what ID the motor is on
"""

import can
from robstride.client import Client
import time

print("=" * 60)
print("SCANNING FOR ROBSTRIDE MOTORS ON CAN BUS")
print("=" * 60)
print()

COM_PORT = "COM4"
BAUDRATE = 1000000

try:
    print("Connecting to CAN bus...")
    bus = can.interface.Bus(
        interface='slcan',
        channel=COM_PORT,
        bitrate=BAUDRATE
    )
    print("[OK] Connected\n")
    
    client = Client(bus)
    
    print("Scanning CAN IDs 1-15...")
    print("-" * 60)
    
    found_motors = []
    
    for motor_id in range(1, 16):
        print(f"  Trying ID {motor_id}...", end=" ", flush=True)
        
        try:
            # Try to enable motor at this ID
            response = client.enable(motor_id)
            
            # If we get a response, motor exists!
            print(f"[FOUND!]")
            print(f"    Response: {response}")
            print(f"    Mode: {response.mode}")
            print(f"    Angle: {response.angle}")
            print(f"    Velocity: {response.velocity}")
            print(f"    Temp: {response.temp}°C")
            print(f"    Errors: {response.errors}")
            print()
            
            found_motors.append(motor_id)
            
        except Exception as e:
            print(f"[No response]")
            continue
    
    print()
    print("=" * 60)
    print("SCAN COMPLETE")
    print("=" * 60)
    print()
    
    if found_motors:
        print(f"[SUCCESS] Found {len(found_motors)} motor(s):")
        for motor_id in found_motors:
            print(f"  - Motor ID: {motor_id}")
        print()
        print("Update your scripts to use the correct motor ID!")
    else:
        print("[X] NO MOTORS FOUND")
        print()
        print("This means:")
        print("  1. Motor not powered")
        print("  2. CAN wiring disconnected/wrong")
        print("  3. CAN termination missing")
        print("  4. Motor in wrong mode/stuck")
        print("  5. Adapter not actually on CAN bus")
        print()
        print("CHECK:")
        print("  - Motor LEDs: Should be Solid Red + Flashing Blue")
        print("  - CANH wire: Connected motor CANH to adapter CANH")
        print("  - CANL wire: Connected motor CANL to adapter CANL")
        print("  - GND wire: Common ground between motor and adapter")
        print("  - 120Ω resistor: Between CANH and CANL")
        print()
    
    bus.shutdown()
    
except Exception as e:
    print(f"[X] Error: {e}")
    import traceback
    traceback.print_exc()

