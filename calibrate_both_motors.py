#!/usr/bin/env python3
"""
Calibrate Boundaries for Both Motors (13 and 14)
Sets physical limits for each motor independently
"""

import serial
import time
import json
import threading

class DualMotorCalibration:
    """Boundary calibration for both motors."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        
        # Motor encodings
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
        
        # Boundaries for each motor
        self.motor13_boundaries = {
            'zero_time': 0,
            'max_time': 0,
            'min_time': 0
        }
        
        self.motor14_boundaries = {
            'zero_time': 0,
            'max_time': 0,
            'min_time': 0
        }
    
    def connect(self):
        """Connect to both motors."""
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("    [OK] Connected\n")
            
            # Initialize
            print("[2] Initializing both motors...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Activate motor 13
            scan_13 = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a])
            self.ser.write(scan_13)
            time.sleep(0.3)
            self.ser.read(1000)
            print("    Motor 13 activated")
            
            # Activate motor 14
            scan_14 = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a])
            self.ser.write(scan_14)
            time.sleep(0.3)
            self.ser.read(1000)
            print("    Motor 14 activated")
            
            print("    [OK] Both motors ready\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def jog_motor(self, motor_encoding, direction=1, duration=0.1):
        """Jog specific motor."""
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        self.ser.write(cmd)
        time.sleep(duration)
    
    def stop_motor(self, motor_encoding):
        """Stop specific motor."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        # Send stop multiple times to ensure it stops
        for i in range(3):
            self.ser.write(stop_cmd)
            time.sleep(0.02)
    
    def wait_for_enter(self):
        """Wait for Enter key."""
        input()
        self.running = False
    
    def calibrate_motor_max(self, motor_num, motor_encoding, motor_name):
        """Calibrate maximum boundary for a motor."""
        print("\n" + "=" * 70)
        print(f"CALIBRATING {motor_name} - MAXIMUM BOUNDARY (Forward)")
        print("=" * 70)
        
        print(f"\n{motor_name} will move FORWARD...")
        print("Press ENTER when it reaches maximum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> {motor_name} MOVING FORWARD - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and track time
        start_time = time.time()
        while self.running:
            self.jog_motor(motor_encoding, direction=1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        max_time = time.time() - start_time
        
        print(f"\n\n[OK] {motor_name} maximum boundary set!")
        print(f"  Time from zero: {max_time:.2f} seconds")
        
        enter_thread.join(timeout=0.1)
        return max_time
    
    def calibrate_motor_min(self, motor_num, motor_encoding, motor_name, max_time):
        """Calibrate minimum boundary for a motor."""
        print("\n" + "=" * 70)
        print(f"CALIBRATING {motor_name} - MINIMUM BOUNDARY (Reverse)")
        print("=" * 70)
        
        print(f"\nFirst, returning {motor_name} to zero position...")
        print(f"  Reversing for {max_time:.2f} seconds...")
        
        # Return to zero
        start = time.time()
        while (time.time() - start) < max_time:
            self.jog_motor(motor_encoding, direction=-1, duration=0.1)
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        print("  [OK] Back at zero\n")
        time.sleep(2)
        
        print(f"{motor_name} will move REVERSE...")
        print("Press ENTER when it reaches minimum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> {motor_name} MOVING REVERSE - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and track time
        start_time = time.time()
        while self.running:
            self.jog_motor(motor_encoding, direction=-1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        min_time = -(time.time() - start_time)  # Negative for reverse
        
        print(f"\n\n[OK] {motor_name} minimum boundary set!")
        print(f"  Time from zero: {abs(min_time):.2f} seconds")
        
        enter_thread.join(timeout=0.1)
        return min_time
    
    def return_to_zero(self, motor_encoding, motor_name, min_time):
        """Return motor to zero position."""
        print(f"\nReturning {motor_name} to zero...")
        print(f"  Moving forward for {abs(min_time):.2f} seconds...")
        
        start = time.time()
        while (time.time() - start) < abs(min_time):
            self.jog_motor(motor_encoding, direction=1, duration=0.1)
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        print("  [OK] At zero\n")
    
    def save_boundaries(self):
        """Save boundaries to file."""
        boundaries = {
            'motor_13': {
                'can_encoding': 'e8 6c',
                'max_time': self.motor13_boundaries['max_time'],
                'min_time': self.motor13_boundaries['min_time'],
                'total_range': self.motor13_boundaries['max_time'] + abs(self.motor13_boundaries['min_time'])
            },
            'motor_14': {
                'can_encoding': 'e8 74',
                'max_time': self.motor14_boundaries['max_time'],
                'min_time': self.motor14_boundaries['min_time'],
                'total_range': self.motor14_boundaries['max_time'] + abs(self.motor14_boundaries['min_time'])
            },
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S'),
            'notes': 'Boundaries in seconds of jog movement from zero position'
        }
        
        with open('dual_motor_boundaries.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n[OK] Boundaries saved to: dual_motor_boundaries.json")
        return boundaries
    
    def run(self):
        """Run full calibration for both motors."""
        print("=" * 70)
        print("   DUAL MOTOR BOUNDARY CALIBRATION")
        print("=" * 70)
        print("\nThis will calibrate physical limits for BOTH motors.")
        print("\nSafety:")
        print("  - Watch both motors carefully")
        print("  - Press Enter immediately at physical limits")
        print("  - Keep hand near power switch")
        
        input("\nPress ENTER when ready to start...")
        
        if not self.connect():
            return False
        
        try:
            # Calibrate Motor 13
            print("\n" + "=" * 70)
            print("CALIBRATING MOTOR 13")
            print("=" * 70)
            input("\nPress ENTER to start Motor 13 calibration...")
            
            max_13 = self.calibrate_motor_max(13, self.motor13_encoding, "Motor 13")
            time.sleep(2)
            
            min_13 = self.calibrate_motor_min(13, self.motor13_encoding, "Motor 13", max_13)
            time.sleep(2)
            
            self.return_to_zero(self.motor13_encoding, "Motor 13", min_13)
            
            self.motor13_boundaries = {
                'max_time': max_13,
                'min_time': min_13
            }
            
            # Calibrate Motor 14
            print("\n" + "=" * 70)
            print("CALIBRATING MOTOR 14")
            print("=" * 70)
            input("\nPress ENTER to start Motor 14 calibration...")
            
            max_14 = self.calibrate_motor_max(14, self.motor14_encoding, "Motor 14")
            time.sleep(2)
            
            min_14 = self.calibrate_motor_min(14, self.motor14_encoding, "Motor 14", max_14)
            time.sleep(2)
            
            self.return_to_zero(self.motor14_encoding, "Motor 14", min_14)
            
            self.motor14_boundaries = {
                'max_time': max_14,
                'min_time': min_14
            }
            
            # Save results
            print("\n" + "=" * 70)
            print("CALIBRATION RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print("\nMotor 13 Boundaries:")
            print(f"  Maximum: +{boundaries['motor_13']['max_time']:.2f}s")
            print(f"  Minimum: {boundaries['motor_13']['min_time']:.2f}s")
            print(f"  Range:   {boundaries['motor_13']['total_range']:.2f}s")
            
            print("\nMotor 14 Boundaries:")
            print(f"  Maximum: +{boundaries['motor_14']['max_time']:.2f}s")
            print(f"  Minimum: {boundaries['motor_14']['min_time']:.2f}s")
            print(f"  Range:   {boundaries['motor_14']['total_range']:.2f}s")
            
            print("\n[SUCCESS] Calibration complete for both motors!")
            
            return True
            
        except KeyboardInterrupt:
            print("\n\nInterrupted!")
            self.stop_motor(self.motor13_encoding)
            self.stop_motor(self.motor14_encoding)
            return False
            
        except Exception as e:
            print(f"\n[ERROR] {e}")
            import traceback
            traceback.print_exc()
            return False
            
        finally:
            if self.ser:
                self.ser.close()
            print("\nDisconnected")


if __name__ == "__main__":
    print("\nDual Motor Boundary Calibration\n")
    
    cal = DualMotorCalibration()
    cal.run()

