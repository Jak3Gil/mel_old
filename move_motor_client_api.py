#!/usr/bin/env python3
"""
Move Robstride Motor using Client API
Motor ID: 15, Port: COM3
"""

import time
import math
import can
from robstride import Client, RunMode

def move_motor_client_api():
    """Use robstride Client API to control motor."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - CLIENT API")
    print("=" * 60 + "\n")
    
    motor_id = 15
    port = 'COM3'
    
    print(f"Motor ID: {motor_id}")
    print(f"Port: {port}")
    print()
    
    try:
        # Create CAN bus using SLCAN
        print("[1] Creating CAN bus (SLCAN)...")
        bus = can.interface.Bus(
            bustype='slcan',
            channel=port,
            bitrate=1000000  # 1 Mbps
        )
        print("    [OK] CAN bus created\n")
        
        # Create Robstride client
        print("[2] Creating Robstride client...")
        client = Client(bus)
        print("    [OK] Client created\n")
        
        # Enable motor
        print("[3] Enabling motor...")
        response = client.enable(motor_id)
        print(f"    Response: {response}")
        print("    [OK] Motor enabled\n")
        
        time.sleep(0.5)
        
        # Set to position mode
        print("[4] Setting position mode...")
        try:
            response = client.write_param(motor_id, 'run_mode', RunMode.Position)
            print(f"    Response: {response}")
            print(f"    Mode: {response.mode}")
            print(f"    Position: {response.angle} rad")
            print(f"    Velocity: {response.velocity} rad/s")
            print(f"    Torque: {response.torque} Nm")
            print(f"    Temp: {response.temp} C")
            print("    [OK] Position mode set\n")
        except Exception as e:
            print(f"    [WARNING] Mode set error: {e}")
            print("    Continuing anyway...\n")
        
        time.sleep(0.5)
        
        # Zero position
        print("[5] Setting zero position...")
        response = client.zero(motor_id)
        print(f"    Response: {response}")
        print("    [OK] Zero position set\n")
        
        time.sleep(0.5)
        
        print("=" * 60)
        print("MOVEMENT TEST")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR SHAFT! ***\n")
        time.sleep(1)
        
        # Test 1: Small movement
        print("[Test 1] Moving to +0.5 radians (29 degrees)...")
        for i in range(60):
            response = client.position(
                motor_id,
                position=0.5,
                velocity=0.0,
                kp=40.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("         [OK] Command sent")
        print("         >> Did it move? <<\n")
        time.sleep(2)
        
        # Test 2: Medium movement
        print("[Test 2] Moving to +1.5 radians (86 degrees)...")
        for i in range(80):
            response = client.position(
                motor_id,
                position=1.5,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            time.sleep(0.02)
            
            if i % 20 == 0:
                print(f"         Sending... {i}/80")
        print("         [OK] Command sent")
        print("         >> Did it move? <<\n")
        time.sleep(2)
        
        # Test 3: Opposite direction
        print("[Test 3] Moving to -1.5 radians (-86 degrees)...")
        for i in range(100):
            response = client.position(
                motor_id,
                position=-1.5,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            time.sleep(0.02)
            
            if i % 25 == 0:
                print(f"         Sending... {i}/100")
        print("         [OK] Command sent")
        print("         >> Did it move to opposite side? <<\n")
        time.sleep(2)
        
        # Test 4: Large movement
        print("[Test 4] LARGE movement - +3.0 radians (172 degrees)...")
        print("         Using HIGH gains: Kp=100, Kd=4")
        for i in range(120):
            response = client.position(
                motor_id,
                position=3.0,
                velocity=0.0,
                kp=100.0,
                kd=4.0,
                torque=0.0
            )
            time.sleep(0.02)
            
            if i % 30 == 0:
                print(f"         Sending... {i}/120")
        print("         [OK] Command sent")
        print("         >> This was HUGE! Did you see it? <<\n")
        time.sleep(2)
        
        # Test 5: Sine wave
        print("[Test 5] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        count = 0
        
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 2.0 * math.sin(2 * math.pi * 0.5 * t)
            
            response = client.position(
                motor_id,
                position=position,
                velocity=0.0,
                kp=45.0,
                kd=2.0,
                torque=0.0
            )
            time.sleep(0.02)
            
            count += 1
            if count % 50 == 0:
                print(f"         Waving... {t:.1f}s")
        
        print("         [OK] Sine wave complete")
        print("         >> Did you see smooth motion? <<\n")
        time.sleep(1)
        
        # Return to zero
        print("[Test 6] Returning to zero...")
        for i in range(80):
            response = client.position(
                motor_id,
                position=0.0,
                velocity=0.0,
                kp=50.0,
                kd=2.0,
                torque=0.0
            )
            time.sleep(0.02)
        print("         [OK] At zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[6] Disabling motor...")
        response = client.disable(motor_id)
        print(f"    Response: {response}")
        print("    [OK] Motor disabled\n")
        
        # Close bus
        print("[7] Closing CAN bus...")
        bus.shutdown()
        print("    [OK] Disconnected\n")
        
        print("=" * 60)
        print("[COMPLETE] ALL TESTS FINISHED!")
        print("=" * 60)
        
        print("\n>>> DID THE MOTOR PHYSICALLY MOVE? <<<")
        print("\nIf YES - Great! The official library works!")
        print("If NO  - Motor may need Motor Studio initialization first")
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        
        try:
            bus.shutdown()
        except:
            pass
        
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Controller")
    print("Using Official robstride.Client API\n")
    
    time.sleep(1)
    
    success = move_motor_client_api()
    
    if not success:
        print("\n" + "=" * 60)
        print("FINAL RECOMMENDATION")
        print("=" * 60)
        print("\nSince motor works in Motor Studio but not programmatically:")
        print("")
        print("The motor likely needs ONE-TIME initialization via Motor Studio.")
        print("")
        print("Steps:")
        print("1. Open Motor Studio")
        print("2. Connect to motor (ID 15, COM3)")
        print("3. Enable motor and jog it around")
        print("4. Click 'Save Settings' or 'Write Parameters'")
        print("5. Close Motor Studio")
        print("6. Run this script again")
        print("")
        print("This saves initialization state to motor's NVRAM")
        print("which persists and allows programmatic control.")



