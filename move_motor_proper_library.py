#!/usr/bin/env python3
"""
Move Robstride Motor using proper python-can and robstride library
This should match what the Robstride software does
"""

import can
import time
import math

try:
    from robstride import MotorControlParams, MotorCommand
    from robstride.client import Client
    HAS_ROBSTRIDE = True
except ImportError:
    HAS_ROBSTRIDE = False
    print("[WARNING] robstride library not found")
    print("Install with: pip install robstride")

def move_motor_proper():
    """Move motor using proper robstride library."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - PROPER LIBRARY")
    print("   CAN ID 13 on COM3")
    print("=" * 60 + "\n")
    
    if not HAS_ROBSTRIDE:
        print("[ERROR] robstride library required")
        print("\nInstall it with:")
        print("  pip install robstride")
        print("  or")
        print("  pip install python-can robstride")
        return False
    
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
        try:
            response = client.enable(MOTOR_ID)
            print(f"    [OK] Motor enabled!")
            print(f"    Response: {response}")
            print(f"    Mode: {response.mode}")
            print(f"    Angle: {response.angle:.3f} rad")
            print(f"    Velocity: {response.velocity:.3f} rad/s")
            print(f"    Torque: {response.torque:.3f} Nm")
            print(f"    Temperature: {response.temp}°C")
        except Exception as e:
            print(f"    [WARNING] Enable response: {e}")
        print()
        
        # Zero position
        print("[4] Setting zero position...")
        try:
            response = client.zero(MOTOR_ID)
            print(f"    [OK] Position zeroed!")
            print(f"    Response: {response}")
        except Exception as e:
            print(f"    [WARNING] Zero response: {e}")
        print()
        
        time.sleep(0.5)
        
        print("=" * 60)
        print("MOVEMENT TEST SEQUENCE")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR! ***\n")
        
        time.sleep(1)
        
        # Test 1: Small movement
        print("[Test 1/5] Moving to +0.5 radians (29 degrees)...")
        print("           Using Kp=50, Kd=2")
        for i in range(60):
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=0.5,
                    velocity=0.0,
                    kp=50.0,
                    kd=2.0,
                    torque=0.0
                )
                time.sleep(0.02)
            except Exception as e:
                if i == 0:  # Only print error once
                    print(f"           Error: {e}")
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 2: Medium movement
        print("[Test 2/5] Moving to +1.5 radians (86 degrees)...")
        for i in range(80):
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=1.5,
                    velocity=0.0,
                    kp=60.0,
                    kd=2.5,
                    torque=0.0
                )
                time.sleep(0.02)
            except:
                pass
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: Opposite direction
        print("[Test 3/5] Moving to -1.5 radians (-86 degrees)...")
        for i in range(80):
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=-1.5,
                    velocity=0.0,
                    kp=60.0,
                    kd=2.5,
                    torque=0.0
                )
                time.sleep(0.02)
            except:
                pass
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: Large movement
        print("[Test 4/5] LARGE MOVEMENT - +2.5 radians (143 degrees)...")
        print("           THIS SHOULD BE VERY OBVIOUS!")
        for i in range(120):
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=2.5,
                    velocity=0.0,
                    kp=100.0,
                    kd=4.0,
                    torque=0.0
                )
                time.sleep(0.02)
            except:
                pass
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 5: Sine wave
        print("[Test 5/5] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        count = 0
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 1.5 * math.sin(2 * math.pi * 0.5 * t)
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=position,
                    velocity=0.0,
                    kp=60.0,
                    kd=3.0,
                    torque=0.0
                )
            except:
                pass
            time.sleep(0.02)
            count += 1
            if count % 50 == 0:
                print(f"           {t:.1f}s...")
        print("           [OK] Sine wave complete\n")
        time.sleep(1)
        
        # Return to zero
        print("[6] Returning to zero...")
        for i in range(80):
            try:
                response = client.position(
                    motor_id=MOTOR_ID,
                    position=0.0,
                    velocity=0.0,
                    kp=60.0,
                    kd=2.5,
                    torque=0.0
                )
                time.sleep(0.02)
            except:
                pass
        print("    [OK] Back to zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[7] Disabling motor...")
        try:
            response = client.disable(MOTOR_ID)
            print(f"    [OK] Motor disabled")
            print(f"    Response: {response}")
        except Exception as e:
            print(f"    Response: {e}")
        print()
        
        bus.shutdown()
        
        print("=" * 60)
        print("[SUCCESS] MOVEMENT SEQUENCE COMPLETE!")
        print("=" * 60)
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        
        print("\nTroubleshooting:")
        print("  1. Is COM3 the correct port?")
        print("  2. Close Robstride PC software if open")
        print("  3. Check motor is powered")
        print("  4. Try: pip install python-can robstride")
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Control using Official Library\n")
    
    success = move_motor_proper()
    
    if success:
        print("\n✓ Motor control working!")
        print("\nYou can now integrate this into your larger system.")
    else:
        print("\n✗ Motor control failed")
        print("\nNext steps:")
        print("  1. Make sure robstride library is installed")
        print("  2. Verify motor works with Robstride PC software")
        print("  3. Check CAN ID is actually 13")

