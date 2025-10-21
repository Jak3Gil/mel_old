#!/usr/bin/env python3
"""
Enable Position Mode on Robstride Motor
Using Official Python Library
"""

import can
from robstride.client import Client, RunMode

print("=" * 60)
print("ENABLING POSITION MODE - OFFICIAL ROBSTRIDE LIBRARY")
print("=" * 60)
print()

# Configuration
MOTOR_ID = 1  # Your motor CAN ID
COM_PORT = "COM4"  # Current adapter port
BAUDRATE = 1000000  # CAN bus speed (1 Mbps)

try:
    # Step 1: Create CAN bus interface
    print("Step 1: Connecting to CAN bus...")
    print(f"  Port: {COM_PORT}")
    print(f"  Baudrate: {BAUDRATE}")
    
    # Use SLCAN interface
    bus = can.interface.Bus(
        bustype='slcan',
        channel=COM_PORT,
        bitrate=BAUDRATE
    )
    print("[OK] CAN bus connected")
    print()
    
    # Step 2: Create Robstride client
    print("Step 2: Creating Robstride client...")
    client = Client(bus)
    print("[OK] Client created")
    print()
    
    # Step 3: Read current run mode
    print("Step 3: Reading current run mode...")
    try:
        current_mode = client.read_param(MOTOR_ID, 'run_mode')
        print(f"[OK] Current mode: {current_mode}")
    except Exception as e:
        print(f"[!] Could not read current mode: {e}")
        current_mode = None
    print()
    
    # Step 4: Enable motor
    print("Step 4: Enabling motor...")
    try:
        response = client.enable(MOTOR_ID)
        print(f"[OK] Motor enabled")
        print(f"  Response: {response}")
    except Exception as e:
        print(f"[!] Enable failed: {e}")
    print()
    
    # Step 5: Set Position Mode
    print("Step 5: Setting Position Mode...")
    try:
        response = client.write_param(
            MOTOR_ID,
            'run_mode',
            RunMode.Position
        )
        print(f"[OK] Position mode set!")
        print(f"  Response: {response}")
        print(f"  Mode: {response.mode}")
        print(f"  Angle: {response.angle}")
        print(f"  Velocity: {response.velocity}")
        print(f"  Temp: {response.temp}")
    except Exception as e:
        print(f"[X] Failed to set position mode: {e}")
        print(f"  This might mean:")
        print(f"    - Motor not responding")
        print(f"    - Wrong CAN ID")
        print(f"    - CAN bus communication issue")
    print()
    
    # Step 6: Verify mode was set
    print("Step 6: Verifying position mode...")
    try:
        new_mode = client.read_param(MOTOR_ID, 'run_mode')
        if new_mode == RunMode.Position:
            print(f"[SUCCESS] Position mode is now active!")
            print(f"  Motor is ready for position control")
        else:
            print(f"[!] Mode is: {new_mode}")
            print(f"   Expected: {RunMode.Position}")
    except Exception as e:
        print(f"[!] Could not verify mode: {e}")
    print()
    
    # Close bus
    bus.shutdown()
    
    print("=" * 60)
    print("DONE")
    print("=" * 60)
    print()
    print("If successful, position mode is now permanently enabled!")
    print("You can now use Motor Studio or Python for position control.")
    print()

except Exception as e:
    print(f"[X] Error: {e}")
    print()
    print("Possible issues:")
    print("  1. Motor not powered or wrong COM port")
    print("  2. CAN adapter not compatible with python-can SLCAN")
    print("  3. Wrong motor CAN ID")
    print()
    import traceback
    traceback.print_exc()

