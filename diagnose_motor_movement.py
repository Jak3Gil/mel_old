#!/usr/bin/env python3
"""
Motor Movement Diagnostic
Tests if motor is ACTUALLY moving physically
"""

import time
import math
from motor_factory import MotorFactory


def test_actual_movement():
    """Test with very obvious movement that you MUST see."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   MOTOR MOVEMENT DIAGNOSTIC                            ║")
    print("║   Testing if motor ACTUALLY moves physically           ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("Expected Current Draw:")
    print("  • Idle (enabled): 0.1-0.5A")
    print("  • Light movement: 0.5-2A")
    print("  • Active movement: 2-5A")
    print("  • If < 0.1A: Motor not actually enabled\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    
    print("=" * 60)
    print("STEP 1: Connection")
    print("=" * 60)
    if not motor.connect():
        print("✗ Connection failed - check USB-CAN adapter")
        return False
    print("✓ Connected\n")
    
    print("=" * 60)
    print("STEP 2: Enable Motor")
    print("=" * 60)
    print("⚠️  WATCH YOUR AMMETER NOW!")
    print("   Current should jump to ~0.2-0.5A when enabled\n")
    
    if not motor.enable():
        print("✗ Enable failed")
        return False
    
    print("✓ Motor enabled")
    print("\n⏸️  Pausing 3 seconds - CHECK CURRENT DRAW NOW!")
    print("   → Is current > 0.1A?")
    time.sleep(3)
    
    print("\n=" * 60)
    print("STEP 3: Zero Position")
    print("=" * 60)
    motor.calibrate()
    time.sleep(1)
    
    print("\n=" * 60)
    print("STEP 4: LARGE OBVIOUS MOVEMENT")
    print("=" * 60)
    print("\n🔍 WATCH THE MOTOR SHAFT CLOSELY!")
    print("   It should rotate ~172 degrees (3 radians)")
    print("   Current should spike to 1-3A during movement\n")
    
    input("Press ENTER when ready to move motor...")
    
    print("\n▶️  MOVING TO +3 RADIANS (very strong gains)...")
    for i in range(100):
        motor.set_position(
            position=3.0,      # Large movement
            velocity=0.0,
            kp=100.0,         # Very high gains - should be VERY obvious
            kd=3.0,
            torque=0.0
        )
        time.sleep(0.02)
        if i % 20 == 0:
            print(f"   Command {i}/100 sent...")
    
    print("\n⏸️  Holding position for 3 seconds...")
    print("   DID THE MOTOR MOVE? (Y/N)")
    time.sleep(3)
    
    print("\n▶️  MOVING TO -3 RADIANS (opposite direction)...")
    for i in range(100):
        motor.set_position(
            position=-3.0,
            velocity=0.0,
            kp=100.0,
            kd=3.0,
            torque=0.0
        )
        time.sleep(0.02)
        if i % 20 == 0:
            print(f"   Command {i}/100 sent...")
    
    print("\n⏸️  Holding position for 3 seconds...")
    time.sleep(3)
    
    print("\n▶️  RETURNING TO ZERO...")
    for i in range(100):
        motor.set_position(0.0, kp=80.0, kd=3.0)
        time.sleep(0.02)
    
    print("\n=" * 60)
    print("STEP 5: Disable")
    print("=" * 60)
    motor.disable()
    print("✓ Motor disabled")
    print("   Current should drop to ~0A now\n")
    
    motor.disconnect()
    
    print("\n" + "=" * 60)
    print("DIAGNOSTIC COMPLETE")
    print("=" * 60)
    print("\n❓ DID YOU SEE THE MOTOR SHAFT PHYSICALLY ROTATE?")
    print("\n[A] YES - Motor moved back and forth")
    print("    → Motor is working! Maybe previous movements were too small")
    print("\n[B] NO - Motor did not move at all")
    print("    → Problem detected - see troubleshooting below")
    print("\n[C] Motor vibrated/made noise but didn't rotate")
    print("    → Motor is trying but something is blocking it")
    
    print("\n" + "=" * 60)
    print("TROUBLESHOOTING IF NO MOVEMENT (Option B)")
    print("=" * 60)
    print("\n1. CHECK CURRENT DRAW:")
    print("   • If 0-0.1A when enabled:")
    print("     → Motor not receiving CAN commands properly")
    print("     → Check CANH/CANL wiring")
    print("     → Check CAN termination resistor (120Ω)")
    print("\n   • If 0.2-0.5A when enabled but no movement:")
    print("     → Motor is receiving commands but in wrong mode")
    print("     → Needs initial setup with Robstride PC software")
    print("     → Motor may require 'position mode' activation")
    print("\n   • If current spikes when moving commands sent:")
    print("     → Motor is working but mechanically blocked")
    print("     → Check if shaft is free to rotate")
    
    print("\n2. VERIFY WIRING:")
    print("   • Power: 12-48V DC connected?")
    print("   • CAN: CANH and CANL correct (not swapped)?")
    print("   • GND: Common ground between motor and USB-CAN?")
    
    print("\n3. ROBSTRIDE PC SOFTWARE:")
    print("   • Motor may need ONE-TIME initialization")
    print("   • This enables position control mode")
    print("   • Cannot be done via CAN alone for first setup")
    
    print("\n4. MOTOR STATUS LED:")
    print("   • Solid Red + Blinking Blue = Ready")
    print("   • Other patterns = Error state")


def test_with_feedback():
    """Try to read feedback from motor."""
    print("\n\n╔════════════════════════════════════════════════════════╗")
    print("║   FEEDBACK TEST                                        ║")
    print("║   Checking if motor responds to commands               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    import serial
    import glob
    
    port = glob.glob('/dev/cu.usbserial*')
    if not port:
        print("✗ No USB port found")
        return
    
    port = port[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.5)
    
    # Initialize
    print("Initializing...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    
    # Enable motor
    print("Enabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    response = ser.read(1000)
    print(f"Enable response: {response}")
    print(f"Response length: {len(response)} bytes")
    
    if len(response) > 0:
        print("✓ Motor responded to enable command")
    else:
        print("✗ No response from motor - may not be receiving commands")
    
    # Send position command and check for response
    print("\nSending position command...")
    ser.write(b't0018A1467FF666666D7FF\r')  # 2 radians
    time.sleep(0.1)
    response = ser.read(1000)
    print(f"Position response: {response}")
    print(f"Response length: {len(response)} bytes")
    
    # Disable
    print("\nDisabling...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.2)
    ser.read(1000)
    
    ser.close()
    
    print("\n" + "=" * 60)
    if len(response) == 0:
        print("⚠️  WARNING: Motor not responding to CAN commands")
        print("   This suggests:")
        print("   • Wrong CAN ID (not 0x01)")
        print("   • CAN wiring issue")
        print("   • Motor not powered")
        print("   • Motor in error state")
    else:
        print("✓ Motor is responding to CAN bus")
        print("  If motor still not moving, it may need PC software setup")


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("ROBSTRIDE MOTOR MOVEMENT DIAGNOSTIC")
    print("=" * 60)
    print("\nThis will test if your motor ACTUALLY moves physically.")
    print("Have an ammeter ready to check current draw!\n")
    
    input("Press ENTER to start diagnostic...")
    
    try:
        test_actual_movement()
        
        print("\n\n" + "=" * 60)
        run_feedback = input("\nRun feedback test to check CAN communication? (y/n): ")
        if run_feedback.lower() == 'y':
            test_with_feedback()
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

