#!/usr/bin/env python3
"""
Hybrid Calibration
Motor 13: Position feedback (accurate)
Motor 14: Time-based (approximate)
"""

import serial
import time
import json
import threading
import struct

class HybridCalibration:
    """Hybrid calibration using position for M13, time for M14."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
        
        self.motor13 = {}
        self.motor14 = {}
    
    def connect(self):
        """Connect and enable MIT mode."""
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("    [OK] Connected\n")
            
            # Initialize
            print("[2] Initializing and enabling MIT mode...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Activate Motor 13
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Enable MIT mode for Motor 13
            self.ser.write(bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            self.ser.write(bytes([0x41, 0x54, 0x18, 0x07, 0xe8, 0x6c, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Activate Motor 14
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            print("    [OK] Both motors ready\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def read_position_m13(self):
        """Read Motor 13 position."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            if len(response) >= 15:
                payload = response[7:15]
                if len(payload) >= 2:
                    return struct.unpack('<h', payload[0:2])[0]
        return None
    
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
    
    def calibrate_m13_max(self):
        """Calibrate Motor 13 max with position feedback."""
        print("\n" + "=" * 70)
        print("MOTOR 13 - MAXIMUM BOUNDARY")
        print("=" * 70)
        
        zero_pos = self.read_position_m13()
        print(f"\nZero position: {zero_pos}")
        print("\nMotor will move FORWARD - Press ENTER at max safe position")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING - PRESS ENTER <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        last_pos = zero_pos
        while self.running:
            self.jog_motor(self.motor13_encoding, direction=1, duration=0.08)
            pos = self.read_position_m13()
            if pos:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Delta{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.02)
        
        self.stop_motor(self.motor13_encoding)
        
        print(f"\n\n[OK] Max: {last_pos} (Delta{last_pos - zero_pos:+d})")
        
        enter_thread.join(timeout=0.1)
        return zero_pos, last_pos
    
    def calibrate_m13_min(self, zero_pos):
        """Calibrate Motor 13 min with position feedback."""
        print("\n" + "=" * 70)
        print("MOTOR 13 - MINIMUM BOUNDARY")
        print("=" * 70)
        
        print("\nManually move Motor 13 back to zero, then press ENTER...")
        input()
        
        print("\nMotor will move REVERSE - Press ENTER at min safe position")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING - PRESS ENTER <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        last_pos = zero_pos
        while self.running:
            self.jog_motor(self.motor13_encoding, direction=-1, duration=0.08)
            pos = self.read_position_m13()
            if pos:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Delta{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.02)
        
        self.stop_motor(self.motor13_encoding)
        
        print(f"\n\n[OK] Min: {last_pos} (Delta{last_pos - zero_pos:+d})")
        
        enter_thread.join(timeout=0.1)
        return last_pos
    
    def calibrate_m14_simple(self):
        """Calibrate Motor 14 using time (no position feedback)."""
        print("\n" + "=" * 70)
        print("MOTOR 14 - TIME-BASED CALIBRATION")
        print("=" * 70)
        
        input("\nPosition Motor 14 at zero, then press ENTER...")
        
        # Max
        print("\nMotor will move FORWARD - Press ENTER at max safe position")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING - PRESS ENTER <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        start_time = time.time()
        while self.running:
            self.jog_motor(self.motor14_encoding, direction=1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(self.motor14_encoding)
        max_time = time.time() - start_time
        
        print(f"\n\n[OK] Max: {max_time:.2f}s from zero")
        
        enter_thread.join(timeout=0.1)
        
        input("\nManually move Motor 14 back to zero, then press ENTER...")
        
        # Min
        print("\nMotor will move REVERSE - Press ENTER at min safe position")
        print("Starting in 3...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING - PRESS ENTER <<<\n")
        
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        start_time = time.time()
        while self.running:
            self.jog_motor(self.motor14_encoding, direction=-1, duration=0.1)
            elapsed = time.time() - start_time
            print(f"  Time: {elapsed:.1f}s", end='\r')
            time.sleep(0.05)
        
        self.stop_motor(self.motor14_encoding)
        min_time = time.time() - start_time
        
        print(f"\n\n[OK] Min: {min_time:.2f}s from zero")
        
        enter_thread.join(timeout=0.1)
        
        return max_time, min_time
    
    def save_boundaries(self):
        """Save boundaries."""
        boundaries = {
            'motor_13': self.motor13,
            'motor_14': self.motor14,
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S')
        }
        
        with open('motor_boundaries_hybrid.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n[OK] Saved to: motor_boundaries_hybrid.json")
        return boundaries
    
    def run(self):
        """Run hybrid calibration."""
        print("=" * 70)
        print("   HYBRID MOTOR CALIBRATION")
        print("=" * 70)
        print("\nMotor 13: Position feedback (accurate)")
        print("Motor 14: Time-based (approximate)\n")
        
        if not self.connect():
            return False
        
        try:
            # Motor 13 with position feedback
            zero_13, max_13 = self.calibrate_m13_max()
            min_13 = self.calibrate_m13_min(zero_13)
            
            self.motor13 = {
                'can_encoding': 'e8 6c',
                'type': 'position_feedback',
                'zero_position': zero_13,
                'max_position': max_13,
                'min_position': min_13,
                'max_offset': max_13 - zero_13,
                'min_offset': min_13 - zero_13,
                'total_range': max_13 - min_13
            }
            
            print("\n[OK] Motor 13 complete!")
            input("\nManually return Motor 13 to zero, then press ENTER...")
            
            # Motor 14 time-based
            max_14_time, min_14_time = self.calibrate_m14_simple()
            
            self.motor14 = {
                'can_encoding': 'e8 74',
                'type': 'time_based',
                'max_time': max_14_time,
                'min_time': min_14_time,
                'total_range_time': max_14_time + min_14_time
            }
            
            print("\n[OK] Motor 14 complete!")
            
            # Save
            print("\n" + "=" * 70)
            print("RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print("\nMotor 13 (Position Feedback):")
            print(f"  Zero:  {boundaries['motor_13']['zero_position']:6d}")
            print(f"  Max:   {boundaries['motor_13']['max_position']:6d} (Δ{boundaries['motor_13']['max_offset']:+d})")
            print(f"  Min:   {boundaries['motor_13']['min_position']:6d} (Δ{boundaries['motor_13']['min_offset']:+d})")
            print(f"  Range: {boundaries['motor_13']['total_range']:6d} counts")
            
            print("\nMotor 14 (Time-Based):")
            print(f"  Max: {boundaries['motor_14']['max_time']:.2f}s")
            print(f"  Min: {boundaries['motor_14']['min_time']:.2f}s")
            print(f"  Range: {boundaries['motor_14']['total_range_time']:.2f}s")
            
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
    print("\nHybrid Motor Calibration\n")
    print("Motor 13: Uses real position (accurate, no oscillation)")
    print("Motor 14: Uses time (approximate, manual repositioning)\n")
    
    cal = HybridCalibration()
    cal.run()













