#!/usr/bin/env python3
"""
Move Robstride Motor using robstride.Client
CAN ID 13 on COM3
"""

import can
from robstride import Client, RunMode
import time
import math

def move_motor_with_client():
    """Move motor using robstride Client."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - CLIENT LIBRARY")
    print("   CAN ID 13 on COM3")
    print("=" * 60 + "\n")
    
    COM_PORT = "COM3"
    MOTOR_ID = 13  # Your motor CAN ID
    
    try:
        print("[1] Connecting to CAN bus...")
        print(f"    Port: {COM_PORT}")
        print(f"    Motor ID: {MOTOR_ID}")
        
        # Create CAN bus connection with SLCAN
        bus = can.interface.Bus(
            interface='slcan',
            channel=COM_PORT,
            bitrate=1000000
        )
        print("    [OK] CAN bus connected\n")
        
        # Create Robstride client
        client = Client(bus)
        print("[2] Created Robstride client\n")
        
        # Enable motor
        print("[3] Enabling motor...")
        response = client.enable(MOTOR_ID)
        print(f"    [OK] Motor enabled!")
        print(f"    Response: {response}")
        print(f"    Mode: {response.mode}")
        print(f"    Angle: {response.angle:.3f} rad")
        print(f"    Velocity: {response.velocity:.3f} rad/s")
        print(f"    Torque: {response.torque:.3f} Nm")
        print(f"    Temperature: {response.temp}°C")
        print()
        
        time.sleep(0.5)
        
        print("=" * 60)
        print("MOVEMENT TEST SEQUENCE")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR! ***\n")
        
        time.sleep(1)
        
        # First, let's try writing position using MIT protocol
        # The robstride library might use write_param for position control
        
        # Let's try sending raw MIT commands using the bus directly
        print("[Test 1] Trying to send MIT position command...")
        print("         Position: +1.0 radians")
        
        # Pack MIT command manually
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        # Target position parameters
        position = 1.0
        velocity = 0.0
        kp = 50.0
        kd = 2.0
        torque = 0.0
        
        # Pack values
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        v_int = float_to_uint(velocity, -45.0, 45.0, 12)
        kp_int = float_to_uint(kp, 0.0, 500.0, 12)
        kd_int = float_to_uint(kd, 0.0, 5.0, 12)
        t_int = float_to_uint(torque, -18.0, 18.0, 12)
        
        # Pack into bytes
        data = bytearray(8)
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        data[2] = (v_int >> 4) & 0xFF
        data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        data[4] = kp_int & 0xFF
        data[5] = (kd_int >> 4) & 0xFF
        data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
        data[7] = t_int & 0xFF
        
        # Create CAN message
        msg = can.Message(
            arbitration_id=MOTOR_ID,
            data=data,
            is_extended_id=False
        )
        
        print(f"         Sending CAN message:")
        print(f"         ID: 0x{MOTOR_ID:03X}")
        print(f"         Data: {data.hex()}")
        
        # Send position commands
        for i in range(100):
            bus.send(msg)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"         Sent {i}/100 messages...")
        
        print("         [OK] Commands sent")
        print("         >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Try opposite direction
        print("[Test 2] Moving to -1.0 radians...")
        position = -1.0
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        msg.data = data
        
        for i in range(100):
            bus.send(msg)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: Large movement
        print("[Test 3] LARGE MOVEMENT - +2.0 radians...")
        print("         THIS SHOULD BE OBVIOUS!")
        position = 2.0
        kp = 100.0
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        kp_int = float_to_uint(kp, 0.0, 500.0, 12)
        
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        data[4] = kp_int & 0xFF
        msg.data = data
        
        for i in range(120):
            bus.send(msg)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: Return to zero
        print("[Test 4] Returning to zero...")
        position = 0.0
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        msg.data = data
        
        for i in range(100):
            bus.send(msg)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(1)
        
        # Disable motor
        print("[4] Disabling motor...")
        response = client.disable(MOTOR_ID)
        print(f"    [OK] Motor disabled")
        print(f"    Response: {response}")
        print()
        
        bus.shutdown()
        
        print("=" * 60)
        print("[COMPLETE] MOVEMENT SEQUENCE DONE")
        print("=" * 60)
        
        print("\nDid you see the motor move?")
        print("  [YES] - Great! The robstride library works!")
        print("  [NO]  - Motor might need configuration in Robstride software")
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        
        print("\nTroubleshooting:")
        print("  1. Is COM3 the correct port?")
        print("  2. Close Robstride PC software if open")
        print("  3. Check motor is powered")
        print("  4. Verify motor CAN ID is 13")
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Control using Client Library\n")
    
    success = move_motor_with_client()
    
    if success:
        print("\nScript completed successfully!")
    else:
        print("\nScript failed - check error messages above")

