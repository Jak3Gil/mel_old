#!/usr/bin/env python3
"""
Move Robstride Motor with Hybrid Initialization
1. Use serial port to send AT commands (like Robstride software)
2. Then use python-can for motor control
"""

import serial
import can
from robstride import Client
import time
import math

def initialize_adapter_serial(port):
    """Initialize adapter using direct serial AT commands."""
    print("[1] Initializing adapter with AT commands...")
    
    try:
        # Open as serial port first
        ser = serial.Serial(port, 115200, timeout=0.5)
        time.sleep(0.3)
        
        # Clear buffers
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Try various AT initialization sequences
        commands = [
            b'C\r',          # Close CAN
            b'S8\r',         # Set CAN speed to 1Mbps (S8 = 1Mbps for some adapters)
            b'O\r',          # Open CAN
        ]
        
        for cmd in commands:
            print(f"    Sending: {cmd.decode('ascii', errors='ignore').strip()}")
            ser.write(cmd)
            time.sleep(0.1)
            resp = ser.read(1000)
            if resp:
                print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        
        ser.close()
        print("    [OK] AT initialization complete\n")
        time.sleep(0.5)
        return True
        
    except Exception as e:
        print(f"    [ERROR] {e}\n")
        return False

def move_motor_hybrid():
    """Move motor with hybrid initialization."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - HYBRID INIT")
    print("   CAN ID 13 on COM3")
    print("=" * 60 + "\n")
    
    COM_PORT = "COM3"
    MOTOR_ID = 13
    
    try:
        # Step 1: Initialize adapter with AT commands
        if not initialize_adapter_serial(COM_PORT):
            print("[ERROR] Adapter initialization failed")
            return False
        
        # Step 2: Connect with python-can
        print("[2] Connecting to CAN bus...")
        bus = can.interface.Bus(
            interface='slcan',
            channel=COM_PORT,
            bitrate=1000000,
            ttyBaudrate=115200
        )
        print("    [OK] CAN bus connected\n")
        
        # Step 3: Create client
        client = Client(bus)
        print("[3] Created Robstride client\n")
        
        # Step 4: Enable motor
        print("[4] Enabling motor...")
        try:
            response = client.enable(MOTOR_ID)
            print(f"    [OK] Motor enabled!")
            print(f"    Angle: {response.angle:.3f} rad")
            print(f"    Velocity: {response.velocity:.3f} rad/s")
            print(f"    Temperature: {response.temp}°C")
        except Exception as e:
            print(f"    [WARNING] Enable error: {e}")
            print("    Continuing anyway...")
        print()
        
        time.sleep(0.5)
        
        print("=" * 60)
        print("MOVEMENT TEST")
        print("=" * 60 + "\n")
        
        # Helper function
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        def send_mit_position(position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
            """Send MIT position command via CAN."""
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
            
            # Send CAN message
            msg = can.Message(
                arbitration_id=MOTOR_ID,
                data=data,
                is_extended_id=False
            )
            bus.send(msg)
        
        # Test 1: Move to +1.0 radian
        print("[Test 1] Moving to +1.0 radians...")
        for i in range(100):
            send_mit_position(1.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 2: Move to -1.0 radian
        print("[Test 2] Moving to -1.0 radians...")
        for i in range(100):
            send_mit_position(-1.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: LARGE movement
        print("[Test 3] LARGE MOVEMENT - +2.5 radians...")
        print("         THIS SHOULD BE VERY OBVIOUS!")
        for i in range(120):
            send_mit_position(2.5, kp=120.0, kd=4.0)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: Back to zero
        print("[Test 4] Returning to zero...")
        for i in range(100):
            send_mit_position(0.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("         [OK] Commands sent\n")
        time.sleep(1)
        
        # Disable
        print("[5] Disabling motor...")
        try:
            response = client.disable(MOTOR_ID)
            print(f"    [OK] Motor disabled")
        except Exception as e:
            print(f"    Disable response: {e}")
        print()
        
        bus.shutdown()
        
        print("=" * 60)
        print("[COMPLETE] TEST SEQUENCE DONE")
        print("=" * 60)
        
        print("\n📋 DIAGNOSTIC CHECKLIST:")
        print("\n1. Did you see the motor shaft rotate?")
        print("   [ ] YES - Motor is working!")
        print("   [ ] NO  - Continue troubleshooting")
        
        print("\n2. When the Robstride software moves the motor:")
        print("   - Does the motor shaft turn freely?")
        print("   - Or does it have resistance/holding torque?")
        
        print("\n3. What happens if you try to manually turn the motor NOW:")
        print("   - Free to rotate = motor disabled (good)")
        print("   - Resistance = motor still enabled or has brake")
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Control - Hybrid Initialization\n")
    
    success = move_motor_hybrid()
    
    if not success:
        print("\n" + "=" * 60)
        print("STILL NOT WORKING?")
        print("=" * 60)
        print("\nThe motor works with Robstride software but not this script.")
        print("This means:")
        print("\n1. Motor hardware is fine")
        print("2. Wiring is correct")
        print("3. Something about how Robstride software initializes is different")
        print("\n💡 NEXT STEPS:")
        print("\nPlease run the Robstride software and:")
        print("  1. Connect to the motor")
        print("  2. Check what 'Mode' the motor is in")
        print("  3. Look for any 'Protocol' or 'Communication' settings")
        print("  4. Take a screenshot of the motor configuration")
        print("\nThen we can match those exact settings!")

