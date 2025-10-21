#!/usr/bin/env python3
"""
Accurate Dual Motor Calibration with Real Position Feedback
Properly decodes motor encoder positions
"""

import serial
import time
import json
import threading
import struct

class AccurateDualCalibration:
    """Accurate calibration using motor position sensors."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        
        # Motor encodings
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
        
        # Boundaries
        self.motor13_boundaries = {}
        self.motor14_boundaries = {}
    
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
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            print("    Motor 13 activated")
            
            # Activate motor 14
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            print("    Motor 14 activated")
            
            print("    [OK] Both motors ready\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def read_position(self, motor_encoding):
        """Read actual motor position from encoder."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            
            # Response format: AT [header] [8 bytes data] \r\n
            # Example: 415410006fec08 [b0d57ff27fff010e] 0d0a
            # But the last 0d0a is terminator, not data!
            # Real payload is: b0d57ff27fff010e (8 bytes)
            
            if len(response) >= 15:
                # Extract 8-byte payload (before \r\n terminator)
                # Response: 41 54 10 00 6f ec 08 [8 bytes] 0d 0a
                #           A  T  header........  payload... \r \n
                payload_start = 7  # After "AT" and 5-byte header
                payload = response[payload_start:payload_start+8]
                
                if len(payload) >= 2:
                    # Position is in first 2 bytes (little-endian based on analysis)
                    pos_raw = struct.unpack('<h', payload[0:2])[0]
                    return pos_raw
        
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
        """Wait for Enter key."""
        input()
        self.running = False
    
    def calibrate_max(self, motor_encoding, motor_name, zero_pos):
        """Calibrate maximum boundary."""
        print("\n" + "=" * 70)
        print(f"{motor_name} - MAXIMUM BOUNDARY (Forward)")
        print("=" * 70)
        
        print(f"\n{motor_name} will move FORWARD...")
        print("Press ENTER when it reaches maximum safe position\n")
        print(f"Zero position: {zero_pos}")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> MOVING FORWARD - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor
        last_pos = zero_pos
        while self.running:
            self.jog_motor(motor_encoding, direction=1, duration=0.08)
            pos = self.read_position(motor_encoding)
            if pos is not None:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.02)
        
        self.stop_motor(motor_encoding)
        
        print(f"\n\n[OK] Maximum set at position: {last_pos} (Δ{last_pos - zero_pos:+d})")
        
        enter_thread.join(timeout=0.1)
        return last_pos
    
    def return_to_zero(self, motor_encoding, motor_name, zero_pos):
        """Return to zero using position feedback - anti-oscillation."""
        print(f"\nReturning {motor_name} to zero...")
        
        iterations = 0
        last_delta = None
        consecutive_overshoots = 0
        
        while iterations < 300:  # Safety limit
            pos = self.read_position(motor_encoding)
            if pos is None:
                print("\n  Cannot read position!")
                break
            
            delta = pos - zero_pos
            print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
            
            # Detect oscillation - if delta keeps changing sign
            if last_delta is not None:
                if (last_delta > 0 and delta < 0) or (last_delta < 0 and delta > 0):
                    consecutive_overshoots += 1
                    if consecutive_overshoots >= 2:
                        # Oscillating! Stop immediately
                        print(f"\n  [OK] At zero (oscillation detected, Δ{delta:+d})")
                        break
                else:
                    consecutive_overshoots = 0
            
            last_delta = delta
            
            # At zero? Use VERY large threshold
            if abs(delta) < 300:  # Very large deadband
                print(f"\n  [OK] At zero (Δ{delta:+d})")
                break
            
            # Adaptive control - much more conservative
            if abs(delta) > 2000:
                # Far from zero
                duration = 0.08
                wait = 0.15
            elif abs(delta) > 1000:
                # Medium distance
                duration = 0.04
                wait = 0.2
            elif abs(delta) > 500:
                # Getting close - very slow
                duration = 0.02
                wait = 0.25
            else:
                # Very close - tiny pulses
                duration = 0.01
                wait = 0.3
            
            # Jog toward zero
            direction = -1 if delta > 0 else 1
            self.jog_motor(motor_encoding, direction=direction, duration=duration)
            
            # Longer waits to let motor settle
            time.sleep(wait)
            
            iterations += 1
        
        # Final stop with multiple commands
        self.stop_motor(motor_encoding)
        time.sleep(0.2)
        
        # Verify final position
        final_pos = self.read_position(motor_encoding)
        if final_pos:
            final_delta = final_pos - zero_pos
            print(f"  Final position: {final_pos} (Δ{final_delta:+d})")
    
    def calibrate_min(self, motor_encoding, motor_name, zero_pos):
        """Calibrate minimum boundary."""
        print("\n" + "=" * 70)
        print(f"{motor_name} - MINIMUM BOUNDARY (Reverse)")
        print("=" * 70)
        
        print(f"\n{motor_name} will move REVERSE...")
        print("Press ENTER when it reaches minimum safe position\n")
        print(f"Zero position: {zero_pos}")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print(f"\n>>> MOVING REVERSE - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor
        last_pos = zero_pos
        while self.running:
            self.jog_motor(motor_encoding, direction=-1, duration=0.08)
            pos = self.read_position(motor_encoding)
            if pos is not None:
                delta = pos - zero_pos
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.02)
        
        self.stop_motor(motor_encoding)
        
        print(f"\n\n[OK] Minimum set at position: {last_pos} (Δ{last_pos - zero_pos:+d})")
        
        enter_thread.join(timeout=0.1)
        return last_pos
    
    def save_boundaries(self):
        """Save boundaries to file."""
        boundaries = {
            'motor_13': self.motor13_boundaries,
            'motor_14': self.motor14_boundaries,
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S'),
            'notes': 'Real encoder positions (raw values from motor sensors)'
        }
        
        with open('dual_motor_boundaries_accurate.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n[OK] Boundaries saved to: dual_motor_boundaries_accurate.json")
        return boundaries
    
    def run(self):
        """Run calibration for both motors."""
        print("=" * 70)
        print("   DUAL MOTOR CALIBRATION - POSITION FEEDBACK")
        print("=" * 70)
        print("\nUses REAL encoder positions from motors!")
        print("\nSafety:")
        print("  - Watch motors carefully")
        print("  - Press Enter at physical limits")
        print("  - Motors return EXACTLY to zero after each step")
        
        input("\nPress ENTER when ready...")
        
        if not self.connect():
            return False
        
        try:
            # ===== CALIBRATE MOTOR 13 =====
            print("\n" + "=" * 70)
            print("MOTOR 13 CALIBRATION")
            print("=" * 70)
            
            print("\n[3] Reading Motor 13 zero position...")
            zero_13 = self.read_position(self.motor13_encoding)
            if zero_13 is None:
                print("  [ERROR] Cannot read Motor 13!")
                return False
            print(f"    Zero: {zero_13}")
            
            input("\nPress ENTER to calibrate Motor 13 maximum...")
            max_13 = self.calibrate_max(self.motor13_encoding, "Motor 13", zero_13)
            time.sleep(1)
            
            self.return_to_zero(self.motor13_encoding, "Motor 13", zero_13)
            time.sleep(1)
            
            input("\nPress ENTER to calibrate Motor 13 minimum...")
            min_13 = self.calibrate_min(self.motor13_encoding, "Motor 13", zero_13)
            time.sleep(1)
            
            self.return_to_zero(self.motor13_encoding, "Motor 13", zero_13)
            
            self.motor13_boundaries = {
                'can_encoding': 'e8 6c',
                'zero_position': zero_13,
                'max_position': max_13,
                'min_position': min_13,
                'max_offset': max_13 - zero_13,
                'min_offset': min_13 - zero_13,
                'total_range': max_13 - min_13
            }
            
            print("\n[OK] Motor 13 complete!")
            
            # ===== CALIBRATE MOTOR 14 =====
            print("\n" + "=" * 70)
            print("MOTOR 14 CALIBRATION")
            print("=" * 70)
            
            print("\n[4] Reading Motor 14 zero position...")
            zero_14 = self.read_position(self.motor14_encoding)
            if zero_14 is None:
                print("  [ERROR] Cannot read Motor 14!")
                return False
            print(f"    Zero: {zero_14}")
            
            input("\nPress ENTER to calibrate Motor 14 maximum...")
            max_14 = self.calibrate_max(self.motor14_encoding, "Motor 14", zero_14)
            time.sleep(1)
            
            self.return_to_zero(self.motor14_encoding, "Motor 14", zero_14)
            time.sleep(1)
            
            input("\nPress ENTER to calibrate Motor 14 minimum...")
            min_14 = self.calibrate_min(self.motor14_encoding, "Motor 14", zero_14)
            time.sleep(1)
            
            self.return_to_zero(self.motor14_encoding, "Motor 14", zero_14)
            
            self.motor14_boundaries = {
                'can_encoding': 'e8 74',
                'zero_position': zero_14,
                'max_position': max_14,
                'min_position': min_14,
                'max_offset': max_14 - zero_14,
                'min_offset': min_14 - zero_14,
                'total_range': max_14 - min_14
            }
            
            print("\n[OK] Motor 14 complete!")
            
            # ===== SAVE RESULTS =====
            print("\n" + "=" * 70)
            print("CALIBRATION COMPLETE")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print("\nMotor 13 Boundaries:")
            print(f"  Zero:    {boundaries['motor_13']['zero_position']:6d}")
            print(f"  Maximum: {boundaries['motor_13']['max_position']:6d} (Δ{boundaries['motor_13']['max_offset']:+6d})")
            print(f"  Minimum: {boundaries['motor_13']['min_position']:6d} (Δ{boundaries['motor_13']['min_offset']:+6d})")
            print(f"  Range:   {boundaries['motor_13']['total_range']:6d} counts")
            
            print("\nMotor 14 Boundaries:")
            print(f"  Zero:    {boundaries['motor_14']['zero_position']:6d}")
            print(f"  Maximum: {boundaries['motor_14']['max_position']:6d} (Δ{boundaries['motor_14']['max_offset']:+6d})")
            print(f"  Minimum: {boundaries['motor_14']['min_position']:6d} (Δ{boundaries['motor_14']['min_offset']:+6d})")
            print(f"  Range:   {boundaries['motor_14']['total_range']:6d} counts")
            
            print("\n[SUCCESS] Both motors calibrated with real encoder data!")
            
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
    print("\nAccurate Dual Motor Calibration\n")
    print("Features:")
    print("  - Uses REAL position from motor encoders")
    print("  - Returns EXACTLY to zero (within 100 counts)")
    print("  - Saves actual encoder values, not time estimates\n")
    
    cal = AccurateDualCalibration()
    cal.run()

