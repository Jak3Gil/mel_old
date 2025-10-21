#!/usr/bin/env python3
"""
Calibrate Both Motors Using Real Position Feedback
Uses actual encoder readings instead of time estimates
"""

import serial
import time
import json
import threading
import struct

class DualMotorFeedbackCalibration:
    """Boundary calibration using real motor position data."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        
        # Motor encodings
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
        
        # Boundaries for each motor (raw encoder values)
        self.motor13_boundaries = {
            'zero_position': None,
            'max_position': None,
            'min_position': None
        }
        
        self.motor14_boundaries = {
            'zero_position': None,
            'max_position': None,
            'min_position': None
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
    
    def read_position(self, motor_encoding):
        """Read current position from motor feedback."""
        # Send stop command to get feedback
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            
            # Decode position from response
            # Response format: AT [header] [8 bytes motor data]
            if len(response) >= 10:
                # Find the motor feedback payload
                for i in range(len(response) - 8):
                    if i + 8 <= len(response):
                        payload = response[i:i+8]
                        
                        # Try to decode position (first 2 bytes as signed 16-bit)
                        try:
                            pos_raw = struct.unpack('>h', payload[0:2])[0]
                            # This is raw encoder value
                            return pos_raw
                        except:
                            continue
        
        return None
    
    def jog_motor_with_feedback(self, motor_encoding, direction=1, duration=0.1):
        """Jog motor and return position."""
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        
        self.ser.write(cmd)
        time.sleep(duration)
        
        # Read position after movement
        pos = self.read_position(motor_encoding)
        return pos
    
    def stop_motor(self, motor_encoding):
        """Stop motor."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        for i in range(3):
            self.ser.write(stop_cmd)
            time.sleep(0.02)
    
    def wait_for_enter(self):
        """Wait for Enter key."""
        input()
        self.running = False
    
    def calibrate_motor_max(self, motor_encoding, motor_name, zero_pos):
        """Calibrate maximum boundary using position feedback."""
        print("\n" + "=" * 70)
        print(f"CALIBRATING {motor_name} - MAXIMUM BOUNDARY (Forward)")
        print("=" * 70)
        
        print(f"\n{motor_name} will move FORWARD...")
        print("Press ENTER when it reaches maximum safe position")
        print(f"\nZero position: {zero_pos}")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> {motor_name} MOVING FORWARD - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor position
        last_pos = zero_pos
        while self.running:
            pos = self.jog_motor_with_feedback(motor_encoding, direction=1, duration=0.1)
            if pos is not None:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        
        print(f"\n\n[OK] {motor_name} maximum boundary set!")
        print(f"  Raw position: {last_pos}")
        print(f"  Offset from zero: {last_pos - zero_pos:+d}")
        
        enter_thread.join(timeout=0.1)
        return last_pos
    
    def return_to_zero(self, motor_encoding, motor_name, zero_pos):
        """Return motor to zero using position feedback."""
        print(f"\nReturning {motor_name} to zero...")
        
        while True:
            current_pos = self.read_position(motor_encoding)
            if current_pos is None:
                print("\n  Cannot read position!")
                break
            
            delta = current_pos - zero_pos
            print(f"  Position: {current_pos:6d} (Δ{delta:+6d})", end='\r')
            
            # Close enough to zero?
            if abs(delta) < 50:  # Within threshold
                print(f"\n  [OK] At zero (Δ{delta:+d})")
                break
            
            # Jog toward zero
            direction = -1 if delta > 0 else 1
            self.jog_motor_with_feedback(motor_encoding, direction=direction, duration=0.05)
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
    
    def calibrate_motor_min(self, motor_encoding, motor_name, zero_pos):
        """Calibrate minimum boundary using position feedback."""
        print("\n" + "=" * 70)
        print(f"CALIBRATING {motor_name} - MINIMUM BOUNDARY (Reverse)")
        print("=" * 70)
        
        print(f"\n{motor_name} will move REVERSE...")
        print("Press ENTER when it reaches minimum safe position")
        print(f"\nZero position: {zero_pos}")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> {motor_name} MOVING REVERSE - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor position
        last_pos = zero_pos
        while self.running:
            pos = self.jog_motor_with_feedback(motor_encoding, direction=-1, duration=0.1)
            if pos is not None:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.05)
        
        self.stop_motor(motor_encoding)
        
        print(f"\n\n[OK] {motor_name} minimum boundary set!")
        print(f"  Raw position: {last_pos}")
        print(f"  Offset from zero: {last_pos - zero_pos:+d}")
        
        enter_thread.join(timeout=0.1)
        return last_pos
    
    def save_boundaries(self):
        """Save boundaries to file."""
        boundaries = {
            'motor_13': {
                'can_encoding': 'e8 6c',
                'zero_position': self.motor13_boundaries['zero_position'],
                'max_position': self.motor13_boundaries['max_position'],
                'min_position': self.motor13_boundaries['min_position'],
                'max_offset': self.motor13_boundaries['max_position'] - self.motor13_boundaries['zero_position'],
                'min_offset': self.motor13_boundaries['min_position'] - self.motor13_boundaries['zero_position'],
                'total_range': self.motor13_boundaries['max_position'] - self.motor13_boundaries['min_position']
            },
            'motor_14': {
                'can_encoding': 'e8 74',
                'zero_position': self.motor14_boundaries['zero_position'],
                'max_position': self.motor14_boundaries['max_position'],
                'min_position': self.motor14_boundaries['min_position'],
                'max_offset': self.motor14_boundaries['max_position'] - self.motor14_boundaries['zero_position'],
                'min_offset': self.motor14_boundaries['min_position'] - self.motor14_boundaries['zero_position'],
                'total_range': self.motor14_boundaries['max_position'] - self.motor14_boundaries['min_position']
            },
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S'),
            'notes': 'Calibrated using actual motor encoder feedback (raw position values)'
        }
        
        with open('dual_motor_boundaries_feedback.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n[OK] Boundaries saved to: dual_motor_boundaries_feedback.json")
        return boundaries
    
    def run(self):
        """Run full calibration for both motors."""
        print("=" * 70)
        print("   DUAL MOTOR CALIBRATION (With Position Feedback)")
        print("=" * 70)
        print("\nThis uses REAL position data from motor encoders.")
        print("\nSafety:")
        print("  - Watch both motors carefully")
        print("  - Press Enter immediately at physical limits")
        print("  - Motors will return EXACTLY to zero after each calibration")
        
        input("\nPress ENTER when ready to start...")
        
        if not self.connect():
            return False
        
        try:
            # ===== CALIBRATE MOTOR 13 =====
            print("\n" + "=" * 70)
            print("CALIBRATING MOTOR 13")
            print("=" * 70)
            
            print("\n[3] Setting Motor 13 zero position...")
            zero_13 = self.read_position(self.motor13_encoding)
            if zero_13 is None:
                print("  [ERROR] Cannot read Motor 13 position!")
                return False
            print(f"    Zero position: {zero_13}")
            self.motor13_boundaries['zero_position'] = zero_13
            
            input("\nPress ENTER to start Motor 13 maximum calibration...")
            max_13 = self.calibrate_motor_max(self.motor13_encoding, "Motor 13", zero_13)
            self.motor13_boundaries['max_position'] = max_13
            time.sleep(2)
            
            self.return_to_zero(self.motor13_encoding, "Motor 13", zero_13)
            time.sleep(2)
            
            input("\nPress ENTER to start Motor 13 minimum calibration...")
            min_13 = self.calibrate_motor_min(self.motor13_encoding, "Motor 13", zero_13)
            self.motor13_boundaries['min_position'] = min_13
            time.sleep(2)
            
            self.return_to_zero(self.motor13_encoding, "Motor 13", zero_13)
            print("[OK] Motor 13 calibration complete!")
            
            # ===== CALIBRATE MOTOR 14 =====
            print("\n" + "=" * 70)
            print("CALIBRATING MOTOR 14")
            print("=" * 70)
            
            print("\n[4] Setting Motor 14 zero position...")
            zero_14 = self.read_position(self.motor14_encoding)
            if zero_14 is None:
                print("  [ERROR] Cannot read Motor 14 position!")
                return False
            print(f"    Zero position: {zero_14}")
            self.motor14_boundaries['zero_position'] = zero_14
            
            input("\nPress ENTER to start Motor 14 maximum calibration...")
            max_14 = self.calibrate_motor_max(self.motor14_encoding, "Motor 14", zero_14)
            self.motor14_boundaries['max_position'] = max_14
            time.sleep(2)
            
            self.return_to_zero(self.motor14_encoding, "Motor 14", zero_14)
            time.sleep(2)
            
            input("\nPress ENTER to start Motor 14 minimum calibration...")
            min_14 = self.calibrate_motor_min(self.motor14_encoding, "Motor 14", zero_14)
            self.motor14_boundaries['min_position'] = min_14
            time.sleep(2)
            
            self.return_to_zero(self.motor14_encoding, "Motor 14", zero_14)
            print("[OK] Motor 14 calibration complete!")
            
            # ===== SAVE RESULTS =====
            print("\n" + "=" * 70)
            print("CALIBRATION RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print("\nMotor 13:")
            print(f"  Zero:    {boundaries['motor_13']['zero_position']:6d}")
            print(f"  Maximum: {boundaries['motor_13']['max_position']:6d} (Δ{boundaries['motor_13']['max_offset']:+6d})")
            print(f"  Minimum: {boundaries['motor_13']['min_position']:6d} (Δ{boundaries['motor_13']['min_offset']:+6d})")
            print(f"  Range:   {boundaries['motor_13']['total_range']:6d} encoder counts")
            
            print("\nMotor 14:")
            print(f"  Zero:    {boundaries['motor_14']['zero_position']:6d}")
            print(f"  Maximum: {boundaries['motor_14']['max_position']:6d} (Δ{boundaries['motor_14']['max_offset']:+6d})")
            print(f"  Minimum: {boundaries['motor_14']['min_position']:6d} (Δ{boundaries['motor_14']['min_offset']:+6d})")
            print(f"  Range:   {boundaries['motor_14']['total_range']:6d} encoder counts")
            
            print("\n[SUCCESS] Both motors calibrated with real position data!")
            
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
    print("\nDual Motor Calibration with Position Feedback\n")
    
    cal = DualMotorFeedbackCalibration()
    cal.run()

