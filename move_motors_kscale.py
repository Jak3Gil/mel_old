#!/usr/bin/env python3
"""
Control Motors 13 & 14 using K-Scale Actuator SDK - CORRECT VERSION
"""

from actuator import RobstrideActuator, RobstrideActuatorConfig, RobstrideActuatorCommand
import time

def main():
    print("=" * 70)
    print("   K-SCALE ACTUATOR SDK - Motors 13 & 14")
    print("=" * 70)
    print()
    
    port = "/dev/cu.usbserial-110"
    
    try:
        # Configure and connect to Motor 13
        print("[1] Connecting to Motor 13...")
        config_13 = RobstrideActuatorConfig(
            can_id=13,
            gear_ratio=6.0,  # Typical for Robstride motors
            max_torque=12.0,
        )
        motor_13 = RobstrideActuator(port_name=port, config=config_13)
        print("    ✓ Motor 13 connected\n")
        
        # Configure and connect to Motor 14
        print("[2] Connecting to Motor 14...")
        config_14 = RobstrideActuatorConfig(
            can_id=14,
            gear_ratio=6.0,
            max_torque=12.0,
        )
        motor_14 = RobstrideActuator(port_name=port, config=config_14)
        print("    ✓ Motor 14 connected\n")
        
        print("=" * 70)
        print("MOTOR MOVEMENT TESTS")
        print("=" * 70)
        print()
        
        # Test 1: Motor 13
        print("[Test 1] Motor 13 - Small movement...")
        print(">>> WATCH MOTOR 13 NOW! <<<")
        
        cmd = RobstrideActuatorCommand(
            position=0.5,  # 0.5 radians
            velocity=0.0,
            torque=0.0,
            kp=50.0,
            kd=1.0,
        )
        motor_13.send_command(cmd)
        time.sleep(2)
        
        # Return to zero
        cmd_zero = RobstrideActuatorCommand(position=0.0, velocity=0.0, torque=0.0, kp=50.0, kd=1.0)
        motor_13.send_command(cmd_zero)
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 2: Motor 14
        print("[Test 2] Motor 14 - Small movement...")
        print(">>> WATCH MOTOR 14 NOW! <<<")
        
        motor_14.send_command(cmd)
        time.sleep(2)
        
        motor_14.send_command(cmd_zero)
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 3: Both motors
        print("[Test 3] BOTH motors together...")
        print(">>> WATCH BOTH MOTORS! <<<")
        
        motor_13.send_command(cmd)
        motor_14.send_command(cmd)
        time.sleep(2)
        
        motor_13.send_command(cmd_zero)
        motor_14.send_command(cmd_zero)
        print("    ✓ Done\n")
        
        print("=" * 70)
        print("[SUCCESS] Motor control complete!")
        print("=" * 70)
        print("\nDid the motors move?")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        print("\nTroubleshooting:")
        print("  - Check motors are powered on (LEDs lit)")
        print("  - Check CAN bus wiring (CANH/CANL)")
        print("  - Check termination resistor (120Ω)")
        print("  - Try different port: /dev/tty.usbserial-110")

if __name__ == "__main__":
    main()

