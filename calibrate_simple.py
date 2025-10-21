#!/usr/bin/env python3
"""
Simple Motor Calibration - No Return to Zero
Just set max/min limits without trying to return precisely
"""

import serial
import time
import json
import threading

class SimpleCalibration:
    """Simple boundary calibration."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
        
        self.motor13_max_time = 0
        self.motor13_min_time = 0
        self.motor14_max_time = 0
        self.motor14_min_time = 0
    
    def connect(self):
        """Connect to motors."""
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("    [OK] Connected\n")
            
            # Initialize
            print("[2] Initializing...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Activate both
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            print("    [OK] Both motors ready\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def jog_motor(self, motor_encoding, direction=1, duration=0.1):
        """Jog motor."""
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        self.ser.write(cmd)
        time.sleep(duration)
    
    def stop_motor(self, motor_encoding):
        """Stop motor."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        for i in range(5):
            self.ser.write(stop_cmd)
            time.sleep(0.02)
    
    def wait_for_enter(self):
        """Wait for Enter."""
        input()
        self.running = False
    
    def calibrate_max(self, motor_encoding, motor_name):
        """Calibrate maximum boundary."""
        print("\n" + "=" * 70)
        print(f"{motor_name} - MAXIMUM BOUNDARY")
        print("=" * 70)
        print(f"\n{motor_name} will move FORWARD")
        print("Press ENTER at maximum safe position\n")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> MOVING - PRESS ENTER TO STOP <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        start_time = time.time()
        while self.running:
            self.jog_motor(motor_encoding, direction=1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        max_time = time.time() - start_time
        
        print(f"\n\n[OK] Maximum at {max_time:.2f} seconds from zero")
        
        enter_thread.join(timeout=0.1)
        return max_time
    
    def calibrate_min(self, motor_encoding, motor_name, max_time):
        """Calibrate minimum boundary."""
        print("\n" + "=" * 70)
        print(f"{motor_name} - RETURN TO ZERO")
        print("=" * 70)
        print(f"\nMoving {motor_name} back toward zero...")
        print(f"Will reverse for approximately {max_time:.2f} seconds\n")
        
        # Just reverse for roughly the same time
        start = time.time()
        while (time.time() - start) < max_time:
            self.jog_motor(motor_encoding, direction=-1, duration=0.1)
            elapsed = time.time() - start
            print(f"  Time: {elapsed:.1f}s / {max_time:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        print(f"\n[OK] Approximately at zero (may be slightly off)")
        
        time.sleep(2)
        
        # Now calibrate minimum
        print("\n" + "=" * 70)
        print(f"{motor_name} - MINIMUM BOUNDARY")
        print("=" * 70)
        print(f"\n{motor_name} will move REVERSE")
        print("Press ENTER at minimum safe position\n")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> MOVING - PRESS ENTER TO STOP <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        start_time = time.time()
        while self.running:
            self.jog_motor(motor_encoding, direction=-1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        min_time = time.time() - start_time
        
        print(f"\n\n[OK] Minimum at {min_time:.2f} seconds from zero")
        
        enter_thread.join(timeout=0.1)
        return min_time
    
    def save_boundaries(self):
        """Save boundaries."""
        boundaries = {
            'motor_13': {
                'can_encoding': 'e8 6c',
                'max_time': self.motor13_max_time,
                'min_time': self.motor13_min_time,
                'total_range': self.motor13_max_time + self.motor13_min_time,
                'note': 'Times in seconds from zero. Approximate.'
            },
            'motor_14': {
                'can_encoding': 'e8 74',
                'max_time': self.motor14_max_time,
                'min_time': self.motor14_min_time,
                'total_range': self.motor14_max_time + self.motor14_min_time,
                'note': 'Times in seconds from zero. Approximate.'
            },
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S')
        }
        
        with open('motor_boundaries_simple.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n[OK] Saved to: motor_boundaries_simple.json")
        return boundaries
    
    def run(self):
        """Run calibration."""
        print("=" * 70)
        print("   SIMPLE MOTOR CALIBRATION")
        print("=" * 70)
        print("\nNo auto-return to zero (avoids oscillation)")
        print("You'll manually position motors between steps\n")
        
        if not self.connect():
            return False
        
        try:
            # Motor 13
            print("\n" + "=" * 70)
            print("MOTOR 13")
            print("=" * 70)
            input("\nPosition Motor 13 at its starting point, then press ENTER...")
            
            print("\nCalibrating Motor 13 maximum...")
            self.motor13_max_time = self.calibrate_max(self.motor13_encoding, "Motor 13")
            time.sleep(1)
            
            print("\nCalibrating Motor 13 minimum...")
            self.motor13_min_time = self.calibrate_min(self.motor13_encoding, "Motor 13", self.motor13_max_time)
            
            print("\n[OK] Motor 13 complete!")
            input("\nManually position Motor 13 back at starting point, then press ENTER...")
            
            # Motor 14
            print("\n" + "=" * 70)
            print("MOTOR 14")
            print("=" * 70)
            input("\nPosition Motor 14 at its starting point, then press ENTER...")
            
            print("\nCalibrating Motor 14 maximum...")
            self.motor14_max_time = self.calibrate_max(self.motor14_encoding, "Motor 14")
            time.sleep(1)
            
            print("\nCalibrating Motor 14 minimum...")
            self.motor14_min_time = self.calibrate_min(self.motor14_encoding, "Motor 14", self.motor14_max_time)
            
            print("\n[OK] Motor 14 complete!")
            
            # Save
            print("\n" + "=" * 70)
            print("RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print("\nMotor 13:")
            print(f"  Max: {boundaries['motor_13']['max_time']:.2f}s")
            print(f"  Min: {boundaries['motor_13']['min_time']:.2f}s")
            print(f"  Range: {boundaries['motor_13']['total_range']:.2f}s")
            
            print("\nMotor 14:")
            print(f"  Max: {boundaries['motor_14']['max_time']:.2f}s")
            print(f"  Min: {boundaries['motor_14']['min_time']:.2f}s")
            print(f"  Range: {boundaries['motor_14']['total_range']:.2f}s")
            
            print("\n[SUCCESS] Calibration complete!")
            
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
    print("\nSimple Motor Calibration\n")
    print("Note: You'll manually reposition motors between steps")
    print("This avoids oscillation issues with auto-return\n")
    
    cal = SimpleCalibration()
    cal.run()

