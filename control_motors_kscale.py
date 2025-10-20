#!/usr/bin/env python3
"""
Control Motors 13 & 14 using K-Scale Actuator SDK
This is the proper way to control Robstride motors on macOS!
"""

from actuator import RobstrideMotorControlParams, RobstrideMotorsSupervisor

def main():
    print("=" * 70)
    print("   K-SCALE ACTUATOR SDK - Motors 13 & 14")
    print("=" * 70)
    print()
    
    # Configure motors
    print("[1] Configuring motors...")
    motor_infos = {
        13: RobstrideMotorControlParams(
            Serial_Number=13,
            zero_position=0.0,
            P_min=-12.5,
            P_max=12.5,
            V_min=-50.0,
            V_max=50.0,
            Kp_min=0.0,
            Kp_max=500.0,
            Kd_min=0.0,
            Kd_max=5.0,
            T_min=-12.0,
            T_max=12.0,
        ),
        14: RobstrideMotorControlParams(
            Serial_Number=14,
            zero_position=0.0,
            P_min=-12.5,
            P_max=12.5,
            V_min=-50.0,
            V_max=50.0,
            Kp_min=0.0,
            Kp_max=500.0,
            Kd_min=0.0,
            Kd_max=5.0,
            T_min=-12.0,
            T_max=12.0,
        ),
    }
    print("    ✓ Motor 13 configured")
    print("    ✓ Motor 14 configured\n")
    
    # Initialize supervisor
    print("[2] Connecting to motors...")
    port = "/dev/cu.usbserial-110"
    
    try:
        supervisor = RobstrideMotorsSupervisor(
            port_name=port,
            motor_infos=motor_infos,
        )
        print(f"    ✓ Connected to {port}\n")
        
        # Test movements
        print("=" * 70)
        print("MOTOR MOVEMENT TESTS")
        print("=" * 70)
        print()
        
        # Test 1: Motor 13
        print("[Test 1] Motor 13 - Small movement...")
        print(">>> WATCH MOTOR 13 NOW! <<<")
        supervisor.send_motor_command(
            motor_id=13,
            position=0.5,  # Move to position 0.5 radians
            velocity=1.0,
            kp=10.0,
            kd=1.0,
            torque=0.0
        )
        import time
        time.sleep(2)
        
        # Return to zero
        supervisor.send_motor_command(
            motor_id=13,
            position=0.0,
            velocity=1.0,
            kp=10.0,
            kd=1.0,
            torque=0.0
        )
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 2: Motor 14
        print("[Test 2] Motor 14 - Small movement...")
        print(">>> WATCH MOTOR 14 NOW! <<<")
        supervisor.send_motor_command(
            motor_id=14,
            position=0.5,
            velocity=1.0,
            kp=10.0,
            kd=1.0,
            torque=0.0
        )
        time.sleep(2)
        
        # Return to zero
        supervisor.send_motor_command(
            motor_id=14,
            position=0.0,
            velocity=1.0,
            kp=10.0,
            kd=1.0,
            torque=0.0
        )
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 3: Both motors together
        print("[Test 3] BOTH motors together...")
        print(">>> WATCH BOTH MOTORS! <<<")
        supervisor.send_motor_command(motor_id=13, position=0.5, velocity=1.0, kp=10.0, kd=1.0, torque=0.0)
        supervisor.send_motor_command(motor_id=14, position=0.5, velocity=1.0, kp=10.0, kd=1.0, torque=0.0)
        time.sleep(2)
        
        # Return to zero
        supervisor.send_motor_command(motor_id=13, position=0.0, velocity=1.0, kp=10.0, kd=1.0, torque=0.0)
        supervisor.send_motor_command(motor_id=14, position=0.0, velocity=1.0, kp=10.0, kd=1.0, torque=0.0)
        print("    ✓ Done\n")
        
        print("=" * 70)
        print("[SUCCESS] Motor control complete!")
        print("=" * 70)
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        print("\nTroubleshooting:")
        print("  - Check motors are powered on")
        print("  - Check USB connection")
        print("  - Try different port: /dev/tty.usbserial-110")

if __name__ == "__main__":
    main()

