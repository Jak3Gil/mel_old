#!/usr/bin/env python3
"""
Robstride Motor Position Control
Uses MIT protocol for precise position control (not JOG)
"""

import serial
import time
import struct

class MotorPositionControl:
    """Position control using MIT protocol."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
    
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
            
            # Activate both motors
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            print("    [OK] Both motors activated\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def read_position(self, motor_encoding):
        """Read motor position."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.reset_input_buffer()
        self.ser.write(cmd)
        time.sleep(0.05)
        
        if self.ser.in_waiting > 0:
            response = self.ser.read(self.ser.in_waiting)
            if len(response) >= 15:
                payload = response[7:15]
                if len(payload) >= 2:
                    pos_raw = struct.unpack('<h', payload[0:2])[0]
                    return pos_raw
        
        return None
    
    def send_mit_position(self, motor_encoding, target_position_raw, kp=30.0, kd=1.0):
        """
        Send MIT position control command.
        This tells motor to GO TO a specific position (not JOG continuously).
        """
        # Pack MIT protocol values
        # Position: Use raw encoder value directly
        p_int = target_position_raw & 0xFFFF  # 16-bit position
        
        # Velocity: 0 (let motor choose speed)
        v_int = 0  # 12-bit
        
        # Kp and Kd gains (12-bit each)
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        kp_int = float_to_uint(kp, 0.0, 500.0, 12)
        kd_int = float_to_uint(kd, 0.0, 5.0, 12)
        t_int = 0  # No feedforward torque
        
        # Pack into 8 bytes (MIT protocol format)
        motor_data = bytearray(8)
        motor_data[0] = (p_int >> 8) & 0xFF
        motor_data[1] = p_int & 0xFF
        motor_data[2] = (v_int >> 4) & 0xFF
        motor_data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        motor_data[4] = kp_int & 0xFF
        motor_data[5] = (kd_int >> 4) & 0xFF
        motor_data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
        motor_data[7] = t_int & 0xFF
        
        # Send as L91 command
        cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoding + bytes([0x08]) + motor_data + bytes([0x0d, 0x0a])
        self.ser.write(cmd)
    
    def go_to_position(self, motor_encoding, motor_name, target_pos, current_pos=None, duration=2.0):
        """Move to target position and hold."""
        if current_pos is None:
            current_pos = self.read_position(motor_encoding)
        
        if current_pos is None:
            print(f"Cannot read {motor_name} position!")
            return False
        
        delta = target_pos - current_pos
        print(f"{motor_name}: Going from {current_pos} to {target_pos} (Delta{delta:+d})")
        
        # Send position commands continuously for duration
        start_time = time.time()
        while (time.time() - start_time) < duration:
            self.send_mit_position(motor_encoding, target_pos, kp=50.0, kd=2.0)
            time.sleep(0.02)
        
        # Verify
        final_pos = self.read_position(motor_encoding)
        if final_pos:
            final_delta = final_pos - target_pos
            print(f"{motor_name}: Arrived at {final_pos} (target {target_pos}, error {final_delta:+d})")
        
        return True
    
    def test_position_control(self):
        """Test MIT position control."""
        print("=" * 70)
        print("MIT POSITION CONTROL TEST")
        print("=" * 70 + "\n")
        
        # Read starting positions
        print("[1] Reading starting positions...")
        zero_13 = self.read_position(self.motor13_encoding)
        time.sleep(0.2)
        zero_14 = self.read_position(self.motor14_encoding)
        
        print(f"    Motor 13: {zero_13}")
        print(f"    Motor 14: {zero_14}")
        
        if zero_13 is None or zero_14 is None:
            print("\n[ERROR] Cannot read motor positions!")
            print("Motor 14 might need different response decoding")
            return
        
        print()
        
        time.sleep(1)
        
        # Test motor 13 position control
        print("[2] Testing Motor 13 position control...")
        print("    Moving +1000 counts...")
        self.go_to_position(self.motor13_encoding, "Motor 13", zero_13 + 1000, zero_13, duration=2.0)
        time.sleep(1)
        
        print("    Returning to zero...")
        self.go_to_position(self.motor13_encoding, "Motor 13", zero_13, duration=2.0)
        print("    Done\n")
        time.sleep(1)
        
        # Test motor 14 position control
        print("[3] Testing Motor 14 position control...")
        print("    Moving +1000 counts...")
        self.go_to_position(self.motor14_encoding, "Motor 14", zero_14 + 1000, zero_14, duration=2.0)
        time.sleep(1)
        
        print("    Returning to zero...")
        self.go_to_position(self.motor14_encoding, "Motor 14", zero_14, duration=2.0)
        print("    Done\n")
        
        print("=" * 70)
        print("TEST COMPLETE")
        print("=" * 70)
        print("\nDid the motors return to zero smoothly without oscillating?")
        print("  [YES] - MIT position control works!")
        print("  [NO]  - JOG is the only option, need better tuning")


if __name__ == "__main__":
    print("\nMIT Position Control Test\n")
    
    controller = MotorPositionControl()
    
    if controller.connect():
        try:
            controller.test_position_control()
        except KeyboardInterrupt:
            print("\n\nStopped")
        finally:
            if controller.ser:
                controller.ser.close()
            print("\nDisconnected")

