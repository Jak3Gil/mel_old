#!/usr/bin/env python3
"""
Quick Test for Motor ID 15
Now that Motor Studio enabled position mode, let's test!
"""

import time
from motor_factory import MotorFactory


def test_motor():
    """Test motor with correct CAN ID."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   MOTOR ID 15 TEST                                     ║")
    print("║   Testing with correct motor ID from Motor Studio     ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("Motor Configuration:")
    print("  • CAN ID: 15 (0x0F)")
    print("  • Position mode: Enabled via Motor Studio ✓")
    print("  • Ready to move!\n")
    
    print("="*60)
    print("[1/6] Loading Configuration...")
    
    try:
        motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
        print(f"     ✓ Motor created (CAN ID: 0x{motor.can_id:02X})")
    except Exception as e:
        print(f"     ✗ Error: {e}")
        return False
    
    print("\n[2/6] Connecting to motor...")
    if not motor.connect():
        print("     ✗ Connection failed")
        return False
    print("     ✓ Connected")
    
    print("\n[3/6] Enabling motor...")
    if not motor.enable():
        print("     ✗ Enable failed")
        return False
    print("     ✓ Enabled")
    time.sleep(0.3)
    
    print("\n[4/6] Setting zero position...")
    if not motor.calibrate():
        print("     ✗ Calibrate failed")
        return False
    print("     ✓ Zero set")
    time.sleep(0.3)
    
    print("\n[5/6] 🎯 MOVING MOTOR!")
    print("     Target: 1 radian (~57 degrees)")
    print("     Watch the motor shaft...\n")
    
    for i in range(50):
        motor.set_position(1.0)
        time.sleep(0.02)
    
    print("     ✓ Move complete!")
    print("     → Did you see the motor move?\n")
    time.sleep(1)
    
    print("[6/6] Returning to zero...")
    for i in range(50):
        motor.set_position(0.0)
        time.sleep(0.02)
    
    print("     ✓ Return complete\n")
    
    motor.disable()
    motor.disconnect()
    
    print("="*60)
    print("✓ TEST COMPLETE!")
    print("="*60)
    
    print("\n🎉 If the motor moved, everything is working!")
    print("\nYou can now use:")
    print("  • example_kscale_usage.py (usage examples)")
    print("  • test_kscale_approach.py (full test suite)")
    print("  • Your own code with the K-Scale architecture!")
    
    return True


if __name__ == "__main__":
    print("\n" + "="*60)
    print("ROBSTRIDE MOTOR - ID 15 TEST")
    print("="*60)
    print("\nTesting motor after Motor Studio configuration...")
    print("Make sure motor is powered and connected!\n")
    
    time.sleep(2)
    
    try:
        success = test_motor()
        
        if not success:
            print("\n⚠️ Test had issues. Check:")
            print("  • Is motor powered?")
            print("  • Is USB-CAN adapter plugged in?")
            print("  • Is motor ID really 15 in Motor Studio?")
            print("  • Did Motor Studio save the position mode?")
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

