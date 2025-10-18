#!/usr/bin/env python3
"""
Test OBVIOUS Motor Movement
No interaction needed - just watch the motor!
"""

import time
from motor_factory import MotorFactory


def main():
    print("╔════════════════════════════════════════════════════════╗")
    print("║   OBVIOUS MOVEMENT TEST                                ║")
    print("║   Testing with LARGE movements you MUST see            ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("Expected Current Draw:")
    print("  • Idle (enabled): 0.1-0.5A")
    print("  • Light movement: 0.5-2A")
    print("  • Active movement: 2-5A")
    print("  • Heavy load: 5-10A")
    print("  • If seeing < 0.1A: Motor NOT enabled/connected\n")
    
    print("⚠️  WATCH YOUR AMMETER AND THE MOTOR SHAFT!\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    
    print("[1/6] Connecting...")
    motor.connect()
    time.sleep(0.5)
    
    print("\n[2/6] Enabling motor...")
    print("      → Current should jump to ~0.2-0.5A NOW")
    motor.enable()
    time.sleep(2)
    print("      ✓ Enabled - did current increase?")
    
    print("\n[3/6] Setting zero position...")
    motor.calibrate()
    time.sleep(1)
    
    print("\n[4/6] 🔥 LARGE MOVEMENT TEST - WATCH THE SHAFT!")
    print("      Moving to +3 radians (~172°)")
    print("      Using VERY HIGH GAINS (Kp=150)")
    print("      → Current should spike to 1-5A during movement\n")
    
    time.sleep(1)
    print("      ▶️  MOVING NOW...")
    
    for i in range(150):
        motor.set_position(
            position=3.0,     # Very large angle
            velocity=0.0,
            kp=150.0,        # EXTREMELY high gain
            kd=5.0,          # High damping
            torque=0.0
        )
        time.sleep(0.02)
        
        if i % 30 == 0:
            print(f"         Sending command {i}/150... (CHECK MOTOR!)")
    
    print("\n      ⏸️  Holding at +3 rad for 3 seconds...")
    print("         → Motor shaft should be rotated ~172° from start")
    print("         → Did you see it move? Is it holding position?")
    time.sleep(3)
    
    print("\n[5/6] 🔥 MOVING TO OPPOSITE DIRECTION")
    print("      Moving to -3 radians (~-172°)")
    print("      This is a HUGE swing - you MUST see this!\n")
    
    time.sleep(1)
    print("      ▶️  MOVING NOW...")
    
    for i in range(150):
        motor.set_position(
            position=-3.0,
            velocity=0.0,
            kp=150.0,
            kd=5.0,
            torque=0.0
        )
        time.sleep(0.02)
        
        if i % 30 == 0:
            print(f"         Sending command {i}/150... (CHECK MOTOR!)")
    
    print("\n      ⏸️  Holding at -3 rad for 3 seconds...")
    time.sleep(3)
    
    print("\n[6/6] Returning to zero...")
    for i in range(100):
        motor.set_position(0.0, kp=100.0, kd=3.0)
        time.sleep(0.02)
    
    time.sleep(1)
    
    print("\n      Disabling motor...")
    print("      → Current should drop to ~0A now")
    motor.disable()
    motor.disconnect()
    
    print("\n" + "=" * 60)
    print("TEST COMPLETE")
    print("=" * 60)
    
    print("\n📊 DIAGNOSIS:")
    print("\n┌─────────────────────────────────────────────────────┐")
    print("│ DID YOU SEE THE MOTOR PHYSICALLY ROTATE?           │")
    print("└─────────────────────────────────────────────────────┘")
    
    print("\n✓ YES - Motor moved back and forth:")
    print("    → Motor is working perfectly!")
    print("    → Previous movements may have been too small to notice")
    
    print("\n✗ NO - Motor did NOT move:")
    print("\n    Check your ammeter readings:")
    print("\n    A) Current was 0-0.1A entire time:")
    print("       → Motor NOT receiving CAN commands")
    print("       → Check CANH/CANL wiring")
    print("       → Verify CAN bus termination (120Ω)")
    print("       → Check motor CAN ID (should be 0x01)")
    
    print("\n    B) Current jumped to 0.2-0.5A when enabled:")
    print("       → Motor is connected but not in position mode")
    print("       → Requires Robstride PC software initialization")
    print("       → This is a ONE-TIME setup that cannot be done via CAN")
    
    print("\n    C) Current spiked during movement commands:")
    print("       → Motor is trying to move but mechanically blocked")
    print("       → Check if shaft is free to rotate")
    print("       → Remove any mechanical load")
    
    print("\n⚡ MOTOR LED STATUS:")
    print("    • Solid Red + Blinking Blue = Ready for commands")
    print("    • Other patterns = Error state (check Robstride docs)")
    
    print("\n🔧 WIRING CHECKLIST:")
    print("    • Power: 12-48V DC connected and ON?")
    print("    • CAN High: Connected to CANH on motor?")
    print("    • CAN Low: Connected to CANL on motor?")
    print("    • Ground: Common GND between motor and USB-CAN?")
    print("    • Termination: 120Ω resistor across CANH/CANL?")
    
    print("\n" + "=" * 60)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted")
    except Exception as e:
        print(f"\n✗ Error occurred: {e}")
        import traceback
        traceback.print_exc()
        
        print("\n⚠️  If error occurred during connection:")
        print("    • Check USB-CAN adapter is plugged in")
        print("    • Close any other software using the adapter")
        print("    • Try unplugging/replugging the adapter")

