#!/usr/bin/env python3
"""
Test Motors 13 & 14 with K-Scale Actuator SDK - Working Version
"""

from actuator import (
    CH341TransportWrapper,
    RobstrideActuator,
    RobstrideActuatorCommand,
    RobstrideActuatorConfig,
    RobstrideConfigureRequest
)
import time

def main():
    print("=" * 70)
    print("   K-SCALE MOTORS 13 & 14 TEST")
    print("=" * 70)
    print()
    
    port = "/dev/cu.usbserial-110"
    
    try:
        # Create transport
        print("[1] Setting up transport...")
        transport = CH341TransportWrapper(port)
        print(f"    ✓ Transport created for {port}\n")
        
        # Create actuator configs for motors 13 and 14
        print("[2] Configuring actuators...")
        actuators_config = [
            (13, RobstrideActuatorConfig(0)),  # Motor 13
            (14, RobstrideActuatorConfig(0)),  # Motor 14
        ]
        print("    ✓ Motor 13 configured")
        print("    ✓ Motor 14 configured\n")
        
        # Create actuator manager
        print("[3] Initializing actuator manager...")
        actuator = RobstrideActuator([transport], actuators_config)
        print("    ✓ Actuator manager ready\n")
        
        # Skip explicit configuration - motors should be ready from init
        print("[4] Motors ready to command\n")
        
        print("=" * 70)
        print("MOVEMENT TESTS")
        print("=" * 70)
        print()
        
        # Test 1: Motor 13
        print("[Test 1] Motor 13 movement...")
        print(">>> WATCH MOTOR 13! <<<")
        
        cmd = RobstrideActuatorCommand(13)
        cmd.position = 0.5
        cmd.velocity = 1.0
        cmd.torque = 0.0
        actuator.command_actuators([cmd])
        time.sleep(2)
        
        # Return to zero
        cmd_zero = RobstrideActuatorCommand(13)
        cmd_zero.position = 0.0
        cmd_zero.velocity = 1.0
        cmd_zero.torque = 0.0
        actuator.command_actuators([cmd_zero])
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 2: Motor 14
        print("[Test 2] Motor 14 movement...")
        print(">>> WATCH MOTOR 14! <<<")
        
        cmd = RobstrideActuatorCommand(14)
        cmd.position = 0.5
        cmd.velocity = 1.0
        cmd.torque = 0.0
        actuator.command_actuators([cmd])
        time.sleep(2)
        
        cmd_zero = RobstrideActuatorCommand(14)
        cmd_zero.position = 0.0
        cmd_zero.velocity = 1.0
        cmd_zero.torque = 0.0
        actuator.command_actuators([cmd_zero])
        print("    ✓ Done\n")
        time.sleep(1)
        
        # Test 3: Both motors together
        print("[Test 3] BOTH motors together...")
        print(">>> WATCH BOTH MOTORS! <<<")
        
        cmd_13 = RobstrideActuatorCommand(13)
        cmd_13.position = 0.5
        cmd_13.velocity = 1.0
        cmd_13.torque = 0.0
        
        cmd_14 = RobstrideActuatorCommand(14)
        cmd_14.position = 0.5
        cmd_14.velocity = 1.0
        cmd_14.torque = 0.0
        
        actuator.command_actuators([cmd_13, cmd_14])
        time.sleep(2)
        
        cmd_13_zero = RobstrideActuatorCommand(13)
        cmd_13_zero.position = 0.0
        cmd_13_zero.velocity = 1.0
        cmd_13_zero.torque = 0.0
        
        cmd_14_zero = RobstrideActuatorCommand(14)
        cmd_14_zero.position = 0.0
        cmd_14_zero.velocity = 1.0
        cmd_14_zero.torque = 0.0
        
        actuator.command_actuators([cmd_13_zero, cmd_14_zero])
        print("    ✓ Done\n")
        
        print("=" * 70)
        print("[SUCCESS] Tests complete!")
        print("=" * 70)
        print("\nDid the motors move?")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()

