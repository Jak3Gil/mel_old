#!/usr/bin/env python3
"""
Robstride Motor Boundary Calibration with Position Feedback
Uses actual motor position readings for accurate calibration
"""

import serial
import time
import json
import threading
import struct

class FeedbackCalibration:
    """Calibration using motor position feedback."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = False
        self.can_id_encoded = bytes([0xe8, 0x6c])
        
        self.zero_position = None
        self.max_position = None
        self.min_position = None
        
    def connect(self):
        """Connect and initialize motor."""
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            # Initialize
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Scan
            scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a])
            self.ser.write(scan_cmd)
            time.sleep(0.5)
            self.ser.read(1000)
            
            print("    [OK] Connected\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def read_position(self):
        """Read current motor position from feedback."""
        # Send stop command to get feedback
        cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            
            # Decode position from response
            # Response format: AT [header] [8 bytes data]
            if len(response) >= 10:
                # Find payload (after AT and headers)
                # Looking for the 8-byte motor feedback
                for i in range(len(response) - 8):
                    if i + 8 <= len(response):
                        payload = response[i:i+8]
                        
                        # Try to decode as position feedback
                        # Position is typically in first 2 bytes as signed 16-bit
                        try:
                            pos_raw = struct.unpack('>h', payload[0:2])[0]
                            # This is raw encoder value, needs scaling
                            # Typical range: -32768 to 32767 maps to motor range
                            return pos_raw
                        except:
                            continue
        
        return None
    
    def jog_with_feedback(self, direction=1, duration=0.1):
        """Jog motor and return position."""
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + data + bytes([0x0d, 0x0a])
        
        self.ser.write(cmd)
        time.sleep(duration)
        
        # Read position after movement
        pos = self.read_position()
        return pos
    
    def stop(self):
        """Stop motor."""
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
    
    def wait_for_enter(self):
        """Wait for Enter key."""
        input()
        self.running = False
    
    def calibrate_max(self):
        """Calibrate maximum boundary with position feedback."""
        print("\n" + "=" * 70)
        print("CALIBRATING MAXIMUM BOUNDARY (Forward)")
        print("=" * 70)
        
        # Set zero reference
        print("\n[2] Setting current position as ZERO...")
        self.zero_position = self.read_position()
        print(f"    Zero position (raw): {self.zero_position}")
        time.sleep(1)
        
        print("\nMotor will move FORWARD...")
        print("Press ENTER when motor reaches maximum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING FORWARD - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor position
        last_pos = self.zero_position
        while self.running:
            pos = self.jog_with_feedback(direction=1, duration=0.1)
            if pos is not None:
                delta = pos - self.zero_position
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.05)
        
        self.stop()
        self.max_position = last_pos
        
        print(f"\n\n✓ Maximum boundary set!")
        print(f"  Raw position: {self.max_position}")
        print(f"  Offset from zero: {self.max_position - self.zero_position}")
        
        enter_thread.join(timeout=0.1)
    
    def calibrate_min(self):
        """Calibrate minimum boundary with position feedback."""
        print("\n" + "=" * 70)
        print("CALIBRATING MINIMUM BOUNDARY (Reverse)")
        print("=" * 70)
        
        print("\nReturning to zero...")
        
        # Return to zero using position feedback
        while True:
            current_pos = self.read_position()
            if current_pos is None:
                break
            
            delta = current_pos - self.zero_position
            print(f"  Position: {current_pos:6d} (Δ{delta:+6d})", end='\r')
            
            # Close enough to zero?
            if abs(delta) < 50:  # Within threshold
                print(f"\n  ✓ At zero (Δ{delta:+d})")
                break
            
            # Jog back toward zero
            direction = -1 if delta > 0 else 1
            self.jog_with_feedback(direction=direction, duration=0.05)
            time.sleep(0.05)
        
        self.stop()
        time.sleep(2)
        
        print("\nMotor will move REVERSE...")
        print("Press ENTER when motor reaches minimum safe position")
        print("\nStarting in 3 seconds...")
        
        for i in range(3, 0, -1):
            print(f"  {i}...")
            time.sleep(1)
        
        print("\n>>> MOVING REVERSE - PRESS ENTER TO STOP <<<\n")
        
        # Start Enter listener
        self.running = True
        enter_thread = threading.Thread(target=self.wait_for_enter, daemon=True)
        enter_thread.start()
        
        # Move and monitor position
        last_pos = self.zero_position
        while self.running:
            pos = self.jog_with_feedback(direction=-1, duration=0.1)
            if pos is not None:
                delta = pos - self.zero_position
                print(f"  Position: {pos:6d} (Δ{delta:+6d})", end='\r')
                last_pos = pos
            time.sleep(0.05)
        
        self.stop()
        self.min_position = last_pos
        
        print(f"\n\n✓ Minimum boundary set!")
        print(f"  Raw position: {self.min_position}")
        print(f"  Offset from zero: {self.min_position - self.zero_position}")
        
        enter_thread.join(timeout=0.1)
    
    def return_to_zero(self):
        """Return to zero using position feedback."""
        print("\n" + "=" * 70)
        print("RETURNING TO ZERO")
        print("=" * 70)
        
        while True:
            current_pos = self.read_position()
            if current_pos is None:
                print("\n  Cannot read position!")
                break
            
            delta = current_pos - self.zero_position
            print(f"  Position: {current_pos:6d} (Δ{delta:+6d})", end='\r')
            
            # At zero?
            if abs(delta) < 50:
                print(f"\n  ✓ At zero! (Δ{delta:+d})")
                break
            
            # Jog toward zero
            direction = -1 if delta > 0 else 1
            self.jog_with_feedback(direction=direction, duration=0.05)
            time.sleep(0.05)
        
        self.stop()
    
    def save_boundaries(self):
        """Save boundaries to file."""
        boundaries = {
            'motor_can_id': 13,
            'zero_position_raw': self.zero_position,
            'max_position_raw': self.max_position,
            'min_position_raw': self.min_position,
            'max_offset': self.max_position - self.zero_position,
            'min_offset': self.min_position - self.zero_position,
            'total_range': self.max_position - self.min_position,
            'calibration_date': time.strftime('%Y-%m-%d %H:%M:%S'),
            'notes': 'Calibrated using actual motor position feedback'
        }
        
        with open('motor_boundaries_feedback.json', 'w') as f:
            json.dump(boundaries, f, indent=4)
        
        print(f"\n✓ Boundaries saved to: motor_boundaries_feedback.json")
        return boundaries
    
    def run(self):
        """Run calibration."""
        print("=" * 70)
        print("   MOTOR BOUNDARY CALIBRATION (With Position Feedback)")
        print("=" * 70)
        print("\n⚠️  SAFETY: Watch motor, press Enter to stop anytime")
        
        input("\nPress ENTER when ready...")
        
        if not self.connect():
            return False
        
        try:
            self.calibrate_max()
            time.sleep(2)
            
            self.calibrate_min()
            time.sleep(2)
            
            self.return_to_zero()
            
            # Save
            print("\n" + "=" * 70)
            print("RESULTS")
            print("=" * 70)
            
            boundaries = self.save_boundaries()
            
            print(f"\nBoundaries:")
            print(f"  Zero:    {boundaries['zero_position_raw']:6d}")
            print(f"  Maximum: {boundaries['max_position_raw']:6d} (Δ{boundaries['max_offset']:+6d})")
            print(f"  Minimum: {boundaries['min_position_raw']:6d} (Δ{boundaries['min_offset']:+6d})")
            print(f"  Range:   {boundaries['total_range']:6d}")
            
            print("\n✓ Calibration complete with position feedback!")
            
            return True
            
        except KeyboardInterrupt:
            print("\n\n⚠️  Interrupted!")
            self.stop()
            return False
        
        except Exception as e:
            print(f"\n❌ Error: {e}")
            import traceback
            traceback.print_exc()
            return False
        
        finally:
            if self.ser:
                self.ser.close()
            print("\nDisconnected")


if __name__ == "__main__":
    print("\nRobstride Motor Calibration with Position Feedback\n")
    
    cal = FeedbackCalibration()
    cal.run()

