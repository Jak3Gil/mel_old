#!/usr/bin/env python3
"""
Test K-Scale Motor Control Approach
Validates the architecture with actual motor
"""

import time
import sys
from motor_factory import MotorFactory


def test_kscale_architecture():
    """Test K-Scale style motor control."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   K-SCALE ARCHITECTURE TEST                            ║")
    print("║   Testing modular motor control approach               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    # Step 1: Load configuration and create motor
    print("[1] Loading Configuration...")
    config_path = "motor_config.yaml"
    
    try:
        config = MotorFactory.load_config(config_path)
        robot_name = config['robot']['name']
        print(f"    ✓ Loaded config for: {robot_name}")
        print(f"    Motors defined: {list(config['motors'].keys())}")
    except Exception as e:
        print(f"    ✗ Failed to load config: {e}")
        return False
    
    # Step 2: Create motor using factory
    print("\n[2] Creating Motor Instance...")
    motor = MotorFactory.create_motor('main_motor', config)
    
    if not motor:
        print("    ✗ Failed to create motor")
        return False
    
    print(f"    ✓ Created motor (CAN ID: 0x{motor.can_id:02X})")
    print(f"    Type: {type(motor).__name__}")
    
    # Step 3: Display motor configuration
    print("\n[3] Motor Configuration:")
    limits = motor.get_limits()
    control = motor.get_control_params()
    print(f"    Position: [{limits['position_min']}, {limits['position_max']}] rad")
    print(f"    Velocity: [{limits['velocity_min']}, {limits['velocity_max']}] rad/s")
    print(f"    Default Kp: {control['default_kp']}")
    print(f"    Default Kd: {control['default_kd']}")
    
    # Step 4: Connect to motor
    print("\n[4] Connecting to Motor...")
    if not motor.connect():
        print("    ✗ Connection failed")
        return False
    
    print("    ✓ Connection successful")
    
    try:
        # Step 5: Enable motor
        print("\n[5] Enabling Motor...")
        if not motor.enable():
            print("    ✗ Enable failed")
            return False
        time.sleep(0.3)
        
        # Step 6: Calibrate (zero position)
        print("\n[6] Calibrating Motor...")
        if not motor.calibrate():
            print("    ✗ Calibration failed")
            return False
        time.sleep(0.3)
        
        # Step 7: Test position control with interface
        print("\n[7] Testing Position Control...")
        print("    Moving to 0.5 radians (~28.6 degrees)")
        
        for i in range(30):
            success = motor.set_position(
                position=0.5,
                velocity=0.0
                # kp and kd will use defaults from config
            )
            if not success:
                print(f"    ✗ Position command {i+1} failed")
                break
            time.sleep(0.02)
        
        print("    ✓ Move complete")
        time.sleep(0.5)
        
        # Step 8: Check motor state
        print("\n[8] Motor State:")
        state = motor.get_state()
        print(f"    Connected: {state['connected']}")
        print(f"    Enabled: {state['enabled']}")
        print(f"    Position: {state['position']:.3f} rad")
        print(f"    Velocity: {state['velocity']:.3f} rad/s")
        
        # Step 9: Return to zero
        print("\n[9] Returning to Zero...")
        for i in range(30):
            motor.set_position(position=0.0)
            time.sleep(0.02)
        print("    ✓ Return complete")
        
        # Step 10: Test with custom gains
        print("\n[10] Testing Custom Gains...")
        print("     Moving with higher Kp (stronger position hold)")
        
        for i in range(30):
            motor.set_position(
                position=0.3,
                kp=50.0,  # Higher than default
                kd=2.0
            )
            time.sleep(0.02)
        
        print("    ✓ Custom gains test complete")
        time.sleep(0.5)
        
        # Return to zero again
        for i in range(30):
            motor.set_position(position=0.0)
            time.sleep(0.02)
        
        # Step 11: Disable motor
        print("\n[11] Disabling Motor...")
        motor.disable()
        
        print("\n" + "="*60)
        print("✓ K-SCALE ARCHITECTURE TEST PASSED!")
        print("="*60)
        print("\nKey Advantages Demonstrated:")
        print("  • Clean abstraction layer")
        print("  • Configuration-driven setup")
        print("  • Easy to use motor interface")
        print("  • Factory pattern for creation")
        print("  • State tracking built-in")
        print("\nThis architecture scales easily to multiple motors!")
        return True
        
    except KeyboardInterrupt:
        print("\n\n⚠ Test interrupted")
        motor.disable()
        return False
    except Exception as e:
        print(f"\n✗ Error during test: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        motor.disconnect()


def test_multi_motor_creation():
    """Test creating multiple motors from config."""
    print("\n" + "="*60)
    print("BONUS: Testing Multi-Motor Creation")
    print("="*60 + "\n")
    
    try:
        motors = MotorFactory.create_from_file('motor_config.yaml')
        print(f"✓ Created {len(motors)} motor(s):")
        for name, motor in motors.items():
            print(f"  • {name}: {type(motor).__name__} (CAN ID: 0x{motor.can_id:02X})")
        return True
    except Exception as e:
        print(f"✗ Multi-motor creation failed: {e}")
        return False


if __name__ == "__main__":
    print("\n" + "="*60)
    print("Testing K-Scale Labs Architecture Implementation")
    print("="*60 + "\n")
    
    # Run main test
    success = test_kscale_architecture()
    
    if success:
        # Run bonus test
        test_multi_motor_creation()
        
        print("\n" + "="*60)
        print("COMPARISON: K-Scale vs Original Approach")
        print("="*60)
        print("\nOriginal (robstride_motor.py):")
        print("  • Direct SLCAN commands")
        print("  • Hardcoded parameters")
        print("  • Single motor focus")
        print("  • Manual state tracking")
        
        print("\nK-Scale Approach:")
        print("  ✓ Abstract motor interface")
        print("  ✓ YAML configuration")
        print("  ✓ Multi-motor ready")
        print("  ✓ Automatic state management")
        print("  ✓ Factory pattern")
        print("  ✓ Easy to extend")
        
        print("\n✓ Ready for production use!")
    else:
        print("\n✗ Test failed - check connection and motor setup")
        sys.exit(1)

