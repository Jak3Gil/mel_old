#!/usr/bin/env python3
"""
Move Robstride Motor using Official Library
Motor ID: 15, Port: COM3
"""

import time
import math
from robstride import MotorControlParams, MotorControlMessage, MotorType, configure
from robstride.protocol.slcan import SlcanTransport

def move_motor_official():
    """Use official Robstride library to control motor."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - OFFICIAL LIBRARY")
    print("=" * 60 + "\n")
    
    motor_id = 15
    port = 'COM3'
    
    print(f"Motor ID: {motor_id}")
    print(f"Port: {port}")
    print()
    
    try:
        # Configure transport
        print("[1] Configuring SLCAN transport...")
        transport = SlcanTransport(port=port, baudrate=115200)
        configure(transport=transport)
        print("    [OK] Transport configured\n")
        
        # Open connection
        print("[2] Opening connection...")
        transport.open()
        time.sleep(0.5)
        print("    [OK] Connection opened\n")
        
        # Enable motor
        print("[3] Enabling motor...")
        enable_msg = MotorControlMessage.start_motor(motor_id)
        transport.send(enable_msg)
        time.sleep(0.5)
        
        # Read response
        resp = transport.receive(timeout=1.0)
        if resp:
            print(f"    Response: {resp}")
        print("    [OK] Motor enabled\n")
        
        # Zero position
        print("[4] Setting zero position...")
        zero_msg = MotorControlMessage.zero_position(motor_id)
        transport.send(zero_msg)
        time.sleep(0.5)
        print("    [OK] Zero position set\n")
        
        print("=" * 60)
        print("MOVEMENT TEST")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR! ***\n")
        time.sleep(1)
        
        # Test 1: Move to position
        print("[Test 1] Moving to +1.5 radians (86 degrees)...")
        
        for i in range(100):
            params = MotorControlParams(
                position=1.5,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            msg = MotorControlMessage(motor_id=motor_id, params=params)
            transport.send(msg)
            time.sleep(0.02)
            
            if i % 25 == 0:
                print(f"         Sending... {i}/100")
        
        print("         [OK] Command sequence complete")
        print("         >> Did the motor move? <<\n")
        time.sleep(2)
        
        # Test 2: Opposite direction
        print("[Test 2] Moving to -1.5 radians (-86 degrees)...")
        
        for i in range(100):
            params = MotorControlParams(
                position=-1.5,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            msg = MotorControlMessage(motor_id=motor_id, params=params)
            transport.send(msg)
            time.sleep(0.02)
            
            if i % 25 == 0:
                print(f"         Sending... {i}/100")
        
        print("         [OK] Command sequence complete")
        print("         >> Did it move to opposite side? <<\n")
        time.sleep(2)
        
        # Test 3: Large movement
        print("[Test 3] LARGE movement - +3.0 radians (172 degrees)...")
        print("         Using HIGH gains: Kp=100, Kd=4")
        
        for i in range(120):
            params = MotorControlParams(
                position=3.0,
                velocity=0.0,
                kp=100.0,
                kd=4.0,
                torque=0.0
            )
            msg = MotorControlMessage(motor_id=motor_id, params=params)
            transport.send(msg)
            time.sleep(0.02)
            
            if i % 30 == 0:
                print(f"         Sending... {i}/120")
        
        print("         [OK] Command sequence complete")
        print("         >> This was HUGE! Did you see it? <<\n")
        time.sleep(2)
        
        # Test 4: Sine wave
        print("[Test 4] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        count = 0
        
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 2.0 * math.sin(2 * math.pi * 0.5 * t)
            
            params = MotorControlParams(
                position=position,
                velocity=0.0,
                kp=45.0,
                kd=2.0,
                torque=0.0
            )
            msg = MotorControlMessage(motor_id=motor_id, params=params)
            transport.send(msg)
            time.sleep(0.02)
            
            count += 1
            if count % 50 == 0:
                print(f"         Waving... {t:.1f}s")
        
        print("         [OK] Sine wave complete")
        print("         >> Did you see smooth back-and-forth? <<\n")
        time.sleep(1)
        
        # Return to zero
        print("[Test 5] Returning to zero...")
        
        for i in range(80):
            params = MotorControlParams(
                position=0.0,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            msg = MotorControlMessage(motor_id=motor_id, params=params)
            transport.send(msg)
            time.sleep(0.02)
        
        print("         [OK] Should be at zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[5] Disabling motor...")
        stop_msg = MotorControlMessage.stop_motor(motor_id)
        transport.send(stop_msg)
        time.sleep(0.5)
        print("    [OK] Motor disabled\n")
        
        # Close connection
        print("[6] Closing connection...")
        transport.close()
        print("    [OK] Disconnected\n")
        
        print("=" * 60)
        print("[COMPLETE] ALL TESTS FINISHED!")
        print("=" * 60)
        
        print("\n>>> DID THE MOTOR PHYSICALLY MOVE? <<<")
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        
        print("\n" + "=" * 60)
        print("TROUBLESHOOTING")
        print("=" * 60)
        print("\nIf using official library failed:")
        print("  1. The library might need specific initialization")
        print("  2. Check library documentation")
        print("  3. Motor might need Motor Studio setup first")
        
        try:
            transport.close()
        except:
            pass
        
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Controller")
    print("Using Official Robstride Python Library\n")
    
    success = move_motor_official()
    
    if not success:
        print("\n" + "=" * 60)
        print("ALTERNATIVE APPROACH")
        print("=" * 60)
        print("\nSince motor moves in Motor Studio but not here:")
        print("")
        print("1. Open Motor Studio")
        print("2. Connect to motor ID 15")
        print("3. Enable the motor")
        print("4. Move it manually (jog controls)")
        print("5. Save settings/parameters")
        print("6. Close Motor Studio")
        print("7. Try this script again")
        print("")
        print("Motor Studio may initialize persistent settings")
        print("that are required for programmatic control.")



