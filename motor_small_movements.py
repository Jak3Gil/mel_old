#!/usr/bin/env python3
"""
Robstride Motor - Small Precise Movements
Demonstrates fine motor control with short pulses
"""

from robstride_control import RobstrideMotor
import time

def small_movements():
    """Demo small, precise motor movements."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - SMALL MOVEMENTS")
    print("=" * 70 + "\n")
    
    motor = RobstrideMotor(port='COM3', can_id=13)
    
    try:
        # Connect
        if not motor.connect():
            print("Failed to connect!")
            return
        
        print("Starting small movement demonstrations...\n")
        
        # Test 1: Tiny pulses
        print("[1] Tiny Pulses (100ms each)...")
        for i in range(5):
            print(f"    Pulse {i+1}/5 forward")
            motor.jog_forward(0.1)  # 100ms pulse
            time.sleep(0.3)  # Pause between pulses
        print("    Done\n")
        time.sleep(1)
        
        # Test 2: Very short movements
        print("[2] Very Short Movements (200ms each)...")
        for i in range(3):
            print(f"    {i+1}/3 forward")
            motor.jog_forward(0.2)
            time.sleep(0.5)
        print("    Done\n")
        time.sleep(1)
        
        # Test 3: Alternating tiny movements
        print("[3] Alternating Tiny Movements...")
        for i in range(4):
            print(f"    Forward {i+1}/4")
            motor.jog_forward(0.15)
            time.sleep(0.3)
            print(f"    Reverse {i+1}/4")
            motor.jog_reverse(0.15)
            time.sleep(0.3)
        print("    Done\n")
        time.sleep(1)
        
        # Test 4: Incremental forward movement
        print("[4] Incremental Forward (10 x 150ms)...")
        for i in range(10):
            if i % 2 == 0:
                print(f"    Step {i+1}/10")
            motor.jog_forward(0.15)
            time.sleep(0.2)
        print("    Done\n")
        time.sleep(1)
        
        # Test 5: Micro pulses
        print("[5] Micro Pulses (50ms each)...")
        for i in range(8):
            print(f"    Micro pulse {i+1}/8")
            motor.jog_forward(0.05)  # 50ms - very short!
            time.sleep(0.2)
        print("    Done\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] Small movements demo finished!")
        print("=" * 70)
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        motor.stop()
        motor.disconnect()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()


def interactive_control():
    """Interactive control with adjustable pulse sizes."""
    print("=" * 70)
    print("   INTERACTIVE SMALL MOVEMENT CONTROL")
    print("=" * 70 + "\n")
    
    motor = RobstrideMotor(port='COM3', can_id=13)
    
    if not motor.connect():
        return
    
    print("\nControls:")
    print("  w = Forward small pulse (100ms)")
    print("  s = Reverse small pulse (100ms)")
    print("  W = Forward medium pulse (300ms)")
    print("  S = Reverse medium pulse (300ms)")
    print("  q = Quit")
    print("\nReady for input...\n")
    
    try:
        while True:
            cmd = input("Command (w/s/W/S/q): ").strip()
            
            if cmd == 'w':
                print("  → Forward small")
                motor.jog_forward(0.1)
            elif cmd == 's':
                print("  ← Reverse small")
                motor.jog_reverse(0.1)
            elif cmd == 'W':
                print("  → Forward medium")
                motor.jog_forward(0.3)
            elif cmd == 'S':
                print("  ← Reverse medium")
                motor.jog_reverse(0.3)
            elif cmd == 'q':
                print("Quitting...")
                break
            else:
                print("  Unknown command")
            
            time.sleep(0.1)
    
    except KeyboardInterrupt:
        print("\n\nStopped")
    finally:
        motor.stop()
        motor.disconnect()


if __name__ == "__main__":
    print("\nRobstride Motor - Small Movements\n")
    print("Choose mode:")
    print("  1 - Demo (automated small movements)")
    print("  2 - Interactive control")
    
    try:
        choice = input("\nEnter choice (1/2): ").strip()
        print()
        
        if choice == '1':
            small_movements()
        elif choice == '2':
            interactive_control()
        else:
            print("Running demo by default...\n")
            small_movements()
            
    except KeyboardInterrupt:
        print("\n\nCancelled")

