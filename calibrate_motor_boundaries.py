#!/usr/bin/env python3
"""
Robstride Motor Boundary Calibration
Sets the physical limits (min/max angles) for the motor
"""

from robstride_control import RobstrideMotor
import time
import json
import threading

class BoundaryCalibration:
    """Calibrate motor movement boundaries."""
    
    def __init__(self):
        self.motor = RobstrideMotor(port='COM3', can_id=13)
        self.running = False
        self.max_angle = 0.0
        self.min_angle = 0.0
        self.current_time = 0.0
        
    def wait_for_enter(self):
        """Wait for Enter key press."""
        input()  # Blocks until Enter is pressed
        self.running = False
    
    def calibrate_max(self):
        """Calibrate maximum (positive) boundary."""
        print("\n" + "=" * 70)
        print("CALIBRATING MAXIMUM BOUNDARY (Positive Direction)")
        print("=" * 70)
        print("\nMotor will start moving FORWARD...")
        print("Press ENTER when motor reaches maximum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOTOR MOVING FORWARD - PRESS ENTER TO STOP <<<\n")
        
        # Start listening for Enter key in separate thread
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Start timer
        start_time = time.time()
        
        # Continuously jog forward until Enter is pressed
        while self.running:
            self.motor.jog_forward(0.1)  # Small pulses
            time.sleep(0.05)  # Very responsive
            
            # Update elapsed time
            self.current_time = time.time() - start_time
            if int(self.current_time * 10) % 10 == 0:  # Print every second
                print(f"  Time: {self.current_time:.1f}s", end='\r')
        
        # Stop motor
        self.motor.stop()
        self.max_angle = self.current_time  # Using time as proxy for angle
        
        print(f"\n\n✓ Maximum boundary set!")
        print(f"  Time from zero: {self.max_angle:.2f} seconds")
        print(f"  (Approximate angle based on jog duration)")
        
        # Wait for Enter thread to finish
        enter_thread.join(timeout=0.1)
    
    def calibrate_min(self):
        """Calibrate minimum (negative) boundary."""
        print("\n" + "=" * 70)
        print("CALIBRATING MINIMUM BOUNDARY (Negative Direction)")
        print("=" * 70)
        print("\nFirst, returning to zero position...")
        
        # Return to zero by jogging back for the time we went forward
        return_time = self.max_angle
        print(f"  Reversing for {return_time:.2f} seconds...")
        
        start = time.time()
        while (time.time() - start) < return_time:
            self.motor.jog_reverse(0.1)
            time.sleep(0.05)
        
        self.motor.stop()
        print("  ✓ Back at zero\n")
        time.sleep(2)
        
        print("Motor will start moving REVERSE...")
        print("Press ENTER when motor reaches minimum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOTOR MOVING REVERSE - PRESS ENTER TO STOP <<<\n")
        
        # Start listening for Enter key
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Start timer
        start_time = time.time()
        
        # Continuously jog reverse until Enter is pressed
        while self.running:
            self.motor.jog_reverse(0.1)
            time.sleep(0.05)
            
            # Update elapsed time
            self.current_time = time.time() - start_time
            if int(self.current_time * 10) % 10 == 0:
                print(f"  Time: {self.current_time:.1f}s", end='\r')
        
        # Stop motor
        self.motor.stop()
        self.min_angle = -self.current_time  # Negative for reverse direction
        
        print(f"\n\n✓ Minimum boundary set!")
        print(f"  Time from zero: {abs(self.min_angle):.2f} seconds")
        print(f"  (Approximate angle based on jog duration)")
        
        enter_thread.join(timeout=0.1)
    
    def save_boundaries(self):
        """Save boundaries to file."""
        boundaries = {
            'motor_can_id': 13,
            'max_angle_time': self.max_angle,
            'min_angle_time': self.min_angle,
            'total_range_time': self.max_angle + abs(self.min_angle),
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S'),
            'notes': 'Angles approximated by jog duration. Positive = forward, Negative = reverse.'
        }
        
        filename = 'motor_boundaries.json'
        with open(filename, 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n✓ Boundaries saved to: {filename}")
        
        return boundaries
    
    def run_calibration(self):
        """Run full calibration sequence."""
        print("=" * 70)
        print("   ROBSTRIDE MOTOR BOUNDARY CALIBRATION")
        print("=" * 70)
        print("\nThis will help you set safe movement limits for your motor.")
        print("\n⚠️  SAFETY:")
        print("  - Ensure motor shaft can rotate freely")
        print("  - Watch for obstacles or limits")
        print("  - Keep hand near power switch")
        print("  - Press Enter immediately if anything goes wrong")
        
        input("\nPress ENTER when ready to start...")
        
        # Connect to motor
        print("\n[1] Connecting to motor...")
        if not self.motor.connect():
            print("Failed to connect!")
            return False
        
        print("\n[2] Starting from ZERO position")
        print("    Current position is now considered ZERO")
        time.sleep(2)
        
        try:
            # Calibrate max
            self.calibrate_max()
            time.sleep(2)
            
            # Calibrate min
            self.calibrate_min()
            
            # Return to zero
            print("\n" + "=" * 70)
            print("RETURNING TO ZERO")
            print("=" * 70)
            print(f"\nReversing for {abs(self.min_angle):.2f} seconds...")
            
            start = time.time()
            while (time.time() - start) < abs(self.min_angle):
                self.motor.jog_forward(0.1)
                time.sleep(0.05)
            
            self.motor.stop()
            print("✓ Back at zero\n")
            
            # Save results
            print("\n" + "=" * 70)
            print("CALIBRATION RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print(f"\nMotor Boundaries:")
            print(f"  Maximum (Forward):  +{boundaries['max_angle_time']:.2f} seconds")
            print(f"  Minimum (Reverse):  {boundaries['min_angle_time']:.2f} seconds")
            print(f"  Total Range:        {boundaries['total_range_time']:.2f} seconds")
            
            print("\n✓ Calibration complete!")
            
            return True
            
        except KeyboardInterrupt:
            print("\n\n⚠️  Calibration interrupted!")
            self.motor.stop()
            return False
        
        except Exception as e:
            print(f"\n❌ Error: {e}")
            import traceback
            traceback.print_exc()
            return False
        
        finally:
            self.motor.disconnect()


def view_boundaries():
    """View saved boundaries."""
    try:
        with open('motor_boundaries.json', 'r') as f:
            boundaries = json.load(f)
        
        print("\n" + "=" * 70)
        print("SAVED MOTOR BOUNDARIES")
        print("=" * 70)
        print(f"\nCalibration Date: {boundaries['calibration_date']}")
        print(f"\nBoundaries:")
        print(f"  Maximum (Forward):  +{boundaries['max_angle_time']:.2f} seconds")
        print(f"  Minimum (Reverse):  {boundaries['min_angle_time']:.2f} seconds")
        print(f"  Total Range:        {boundaries['total_range_time']:.2f} seconds")
        print(f"\nNotes: {boundaries['notes']}")
        
    except FileNotFoundError:
        print("\n⚠️  No calibration file found. Run calibration first!")
    except Exception as e:
        print(f"\n❌ Error reading boundaries: {e}")


if __name__ == "__main__":
    print("\nRobstride Motor Boundary Calibration\n")
    print("Options:")
    print("  1 - Run calibration")
    print("  2 - View saved boundaries")
    
    try:
        choice = input("\nEnter choice (1/2): ").strip()
        
        if choice == '1':
            cal = BoundaryCalibration()
            cal.run_calibration()
        elif choice == '2':
            view_boundaries()
        else:
            print("Invalid choice")
            
    except KeyboardInterrupt:
        print("\n\nCancelled")

