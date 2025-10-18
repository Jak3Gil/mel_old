#!/usr/bin/env python3
"""
Simple Example: Using K-Scale Architecture
Quick reference for motor control
"""

import time
from motor_factory import MotorFactory


def simple_movement():
    """Simplest possible motor control example."""
    print("Simple Movement Example\n")
    
    # Create motor from config
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    
    # Connect and enable
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    # Move to position (uses default gains from config)
    print("Moving to 1 radian...")
    for _ in range(50):
        motor.set_position(1.0)
        time.sleep(0.02)
    
    time.sleep(1)
    
    # Return to zero
    print("Returning to zero...")
    for _ in range(50):
        motor.set_position(0.0)
        time.sleep(0.02)
    
    # Cleanup
    motor.disable()
    motor.disconnect()
    print("Done!")


def custom_gains_example():
    """Example with custom control gains."""
    print("Custom Gains Example\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    # Move with strong position hold (high Kp)
    print("Moving with high Kp (stiff)...")
    for _ in range(50):
        motor.set_position(
            position=0.5,
            kp=80.0,  # Very stiff
            kd=3.0
        )
        time.sleep(0.02)
    
    time.sleep(0.5)
    
    # Move with soft position hold (low Kp)
    print("Moving with low Kp (compliant)...")
    for _ in range(50):
        motor.set_position(
            position=-0.5,
            kp=10.0,  # Very soft
            kd=0.5
        )
        time.sleep(0.02)
    
    time.sleep(0.5)
    
    # Return to zero
    for _ in range(50):
        motor.set_position(0.0)
        time.sleep(0.02)
    
    motor.disable()
    motor.disconnect()
    print("Done!")


def state_monitoring_example():
    """Example showing state monitoring."""
    print("State Monitoring Example\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    # Move and check state
    positions = [0.5, 1.0, 0.5, 0.0]
    
    for target_pos in positions:
        print(f"\nMoving to {target_pos} rad...")
        
        for _ in range(50):
            motor.set_position(target_pos)
            time.sleep(0.02)
        
        # Check state
        state = motor.get_state()
        print(f"  State:")
        print(f"    Connected: {state['connected']}")
        print(f"    Enabled: {state['enabled']}")
        print(f"    Position: {state['position']:.3f} rad")
        
        time.sleep(0.5)
    
    motor.disable()
    motor.disconnect()
    print("\nDone!")


def multi_motor_example():
    """Example for when you have multiple motors."""
    print("Multi-Motor Example\n")
    
    # Create all motors from config
    motors = MotorFactory.create_from_file('motor_config.yaml')
    
    print(f"Found {len(motors)} motor(s)")
    
    # Connect all
    for name, motor in motors.items():
        print(f"Connecting {name}...")
        motor.connect()
        motor.enable()
        motor.calibrate()
    
    # Coordinated movement
    print("\nCoordinated movement...")
    for _ in range(50):
        for motor in motors.values():
            motor.set_position(0.5)
        time.sleep(0.02)
    
    time.sleep(0.5)
    
    # Return all to zero
    print("Returning all to zero...")
    for _ in range(50):
        for motor in motors.values():
            motor.set_position(0.0)
        time.sleep(0.02)
    
    # Cleanup all
    for motor in motors.values():
        motor.disable()
        motor.disconnect()
    
    print("Done!")


def configuration_example():
    """Show how to access configuration."""
    print("Configuration Example\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    
    # Get limits
    limits = motor.get_limits()
    print("Motor Limits:")
    print(f"  Position: [{limits['position_min']}, {limits['position_max']}] rad")
    print(f"  Velocity: [{limits['velocity_min']}, {limits['velocity_max']}] rad/s")
    print(f"  Torque: [{limits['torque_min']}, {limits['torque_max']}] Nm")
    
    # Get control parameters
    control = motor.get_control_params()
    print("\nDefault Control Parameters:")
    print(f"  Kp: {control['default_kp']}")
    print(f"  Kd: {control['default_kd']}")
    
    # Get full config
    print("\nFull Configuration:")
    print(f"  CAN ID: 0x{motor.can_id:02X}")
    print(f"  Type: {motor.config['type']}")


if __name__ == "__main__":
    import sys
    
    print("╔════════════════════════════════════════════════════════╗")
    print("║   K-SCALE ARCHITECTURE USAGE EXAMPLES                  ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    if len(sys.argv) < 2:
        print("Usage: python3 example_kscale_usage.py <example_number>")
        print("\nAvailable examples:")
        print("  1 - Simple movement")
        print("  2 - Custom gains")
        print("  3 - State monitoring")
        print("  4 - Multi-motor (if configured)")
        print("  5 - Configuration access")
        print("\nExample: python3 example_kscale_usage.py 1")
        sys.exit(1)
    
    example = sys.argv[1]
    
    try:
        if example == "1":
            simple_movement()
        elif example == "2":
            custom_gains_example()
        elif example == "3":
            state_monitoring_example()
        elif example == "4":
            multi_motor_example()
        elif example == "5":
            configuration_example()
        else:
            print(f"Unknown example: {example}")
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()

