#!/usr/bin/env python3
"""
Smooth Motion Demo - K-Scale Architecture
Demonstrates smooth, continuous motor movement
"""

import time
import math
from motor_factory import MotorFactory


def smooth_sine_wave():
    """Move motor in smooth sine wave pattern."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   SMOOTH SINE WAVE MOTION DEMO                         ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    print("Moving in smooth sine wave pattern...")
    print("(Press Ctrl+C to stop)\n")
    
    try:
        amplitude = 1.5  # radians (~86 degrees)
        frequency = 0.5  # Hz
        duration = 10    # seconds
        
        start_time = time.time()
        
        while (time.time() - start_time) < duration:
            t = time.time() - start_time
            position = amplitude * math.sin(2 * math.pi * frequency * t)
            
            motor.set_position(position)
            
            # Print status every 0.5 seconds
            if int(t * 2) % 1 == 0 and t - int(t * 2) / 2 < 0.02:
                print(f"  t={t:.1f}s  position={position:+.3f} rad  ({math.degrees(position):+.1f}°)")
            
            time.sleep(0.02)
        
        print("\nReturning to zero...")
        for _ in range(50):
            motor.set_position(0.0)
            time.sleep(0.02)
        
        print("✓ Demo complete!")
        
    except KeyboardInterrupt:
        print("\n\nStopping...")
        for _ in range(30):
            motor.set_position(0.0)
            time.sleep(0.02)
    
    finally:
        motor.disable()
        motor.disconnect()


def back_and_forth():
    """Simple back and forth motion."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   BACK AND FORTH MOTION DEMO                           ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    positions = [1.5, -1.5, 1.5, -1.5, 0.0]
    
    print("Moving back and forth...\n")
    
    for i, target in enumerate(positions):
        print(f"[{i+1}/{len(positions)}] Moving to {target:+.1f} rad ({math.degrees(target):+.1f}°)")
        
        for _ in range(60):
            motor.set_position(target, kp=40.0, kd=2.0)
            time.sleep(0.02)
        
        time.sleep(0.3)
    
    print("\n✓ Demo complete!")
    
    motor.disable()
    motor.disconnect()


def velocity_demo():
    """Demonstrate velocity control."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   VELOCITY DEMO                                        ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    motor = MotorFactory.create_from_file('motor_config.yaml', 'main_motor')
    motor.connect()
    motor.enable()
    motor.calibrate()
    
    print("Slow movement (low Kp, emphasizing velocity)...")
    for _ in range(100):
        motor.set_position(2.0, velocity=1.0, kp=5.0, kd=0.8)
        time.sleep(0.02)
    
    time.sleep(0.5)
    
    print("Fast movement (high Kp, quick position snap)...")
    for _ in range(100):
        motor.set_position(-2.0, velocity=0.0, kp=80.0, kd=3.0)
        time.sleep(0.02)
    
    time.sleep(0.5)
    
    print("Returning to zero...")
    for _ in range(60):
        motor.set_position(0.0)
        time.sleep(0.02)
    
    print("✓ Demo complete!")
    
    motor.disable()
    motor.disconnect()


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python3 demo_smooth_motion.py <demo_type>")
        print("\nAvailable demos:")
        print("  sine     - Smooth sine wave motion")
        print("  back     - Back and forth motion")
        print("  velocity - Velocity control demo")
        print("\nExample: python3 demo_smooth_motion.py sine")
        sys.exit(1)
    
    demo = sys.argv[1].lower()
    
    try:
        if demo == "sine":
            smooth_sine_wave()
        elif demo == "back":
            back_and_forth()
        elif demo == "velocity":
            velocity_demo()
        else:
            print(f"Unknown demo: {demo}")
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()

