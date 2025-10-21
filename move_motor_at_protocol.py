#!/usr/bin/env python3
"""
Move Robstride Motor using AT Command Protocol
Based on captured traffic: 41 54 00 07 ea 4c 01 00 0d 0a
"""

import serial
import time
import struct
import math

class RobstrideATProtocol:
    """Robstride motor control using AT command protocol."""
    
    def __init__(self, port='COM3', motor_id=13):
        self.port = port
        self.motor_id = motor_id
        self.ser = None
    
    def connect(self):
        """Connect to the adapter."""
        print(f"[1] Connecting to {self.port}...")
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=0.5)
            time.sleep(0.3)
            
            # Clear buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            print("    [OK] Connected\n")
            return True
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def send_at_command(self, data_bytes):
        """
        Send AT command in the format seen in logs:
        41 54 00 07 ea 4c 01 00 0d 0a
        AT \x00 \x07 [data...] \x0d \x0a
        """
        # Build AT command packet
        packet = bytearray()
        packet.extend(b'AT')  # 41 54
        packet.extend(data_bytes)
        packet.extend(b'\r\n')  # 0d 0a
        
        self.ser.write(packet)
        
        # Read response
        time.sleep(0.01)
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def enable_motor(self):
        """Enable the motor."""
        print("[2] Enabling motor...")
        
        # Try standard enable command
        # Format: AT command for motor enable
        enable_data = bytes([
            0x00, 0x07,  # Command length/type?
            0xFF, 0xFF, 0xFF, 0xFF,  # Enable signature
            0xFF, 0xFF, 0xFF, 0xFC   # Enable with ID
        ])
        
        self.send_at_command(enable_data)
        time.sleep(0.2)
        print("    [OK] Enable command sent\n")
    
    def zero_position(self):
        """Set zero position."""
        print("[3] Setting zero position...")
        
        zero_data = bytes([
            0x00, 0x07,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE   # Zero position signature
        ])
        
        self.send_at_command(zero_data)
        time.sleep(0.2)
        print("    [OK] Zero command sent\n")
    
    def send_position(self, position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
        """Send position command via AT protocol."""
        # Pack MIT command data
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        v_int = float_to_uint(velocity, -45.0, 45.0, 12)
        kp_int = float_to_uint(kp, 0.0, 500.0, 12)
        kd_int = float_to_uint(kd, 0.0, 5.0, 12)
        t_int = float_to_uint(torque, -18.0, 18.0, 12)
        
        # Pack into bytes
        motor_data = bytearray(8)
        motor_data[0] = (p_int >> 8) & 0xFF
        motor_data[1] = p_int & 0xFF
        motor_data[2] = (v_int >> 4) & 0xFF
        motor_data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        motor_data[4] = kp_int & 0xFF
        motor_data[5] = (kd_int >> 4) & 0xFF
        motor_data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
        motor_data[7] = t_int & 0xFF
        
        # Wrap in AT command format
        at_data = bytes([0x00, 0x08]) + motor_data  # 0x08 = 8 bytes of data
        self.send_at_command(at_data)
    
    def disable_motor(self):
        """Disable the motor."""
        print("[4] Disabling motor...")
        
        disable_data = bytes([
            0x00, 0x07,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFD   # Disable signature
        ])
        
        self.send_at_command(disable_data)
        time.sleep(0.2)
        print("    [OK] Disable command sent\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[5] Disconnected\n")

def move_motor_at():
    """Move motor using AT protocol."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - AT PROTOCOL")
    print("   CAN ID 13 on COM3")
    print("=" * 60 + "\n")
    
    motor = RobstrideATProtocol(port='COM3', motor_id=13)
    
    try:
        if not motor.connect():
            return False
        
        motor.enable_motor()
        time.sleep(0.3)
        
        motor.zero_position()
        time.sleep(0.3)
        
        print("=" * 60)
        print("MOVEMENT TEST SEQUENCE")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR! ***\n")
        time.sleep(1)
        
        # Test 1: Move to +1.0 radian
        print("[Test 1/5] Moving to +1.0 radians (57 degrees)...")
        for i in range(100):
            motor.send_position(1.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"           Sending commands... {i}/100")
        print("           [OK] Commands sent")
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Move to -1.0 radian
        print("[Test 2/5] Moving to -1.0 radians (-57 degrees)...")
        for i in range(100):
            motor.send_position(-1.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: LARGE movement
        print("[Test 3/5] LARGE MOVEMENT - +2.5 radians (143 degrees)...")
        print("           THIS SHOULD BE VERY OBVIOUS!")
        for i in range(120):
            motor.send_position(2.5, kp=120.0, kd=4.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: Sine wave
        print("[Test 4/5] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        count = 0
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 1.5 * math.sin(2 * math.pi * 0.5 * t)
            motor.send_position(position, kp=70.0, kd=3.0)
            time.sleep(0.02)
            count += 1
            if count % 50 == 0:
                print(f"           {t:.1f}s...")
        print("           [OK] Sine wave complete\n")
        time.sleep(1)
        
        # Test 5: Return to zero
        print("[Test 5/5] Returning to zero...")
        for i in range(100):
            motor.send_position(0.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("           [OK] Back to zero\n")
        time.sleep(1)
        
        motor.disable_motor()
        motor.disconnect()
        
        print("=" * 60)
        print("[COMPLETE] MOVEMENT SEQUENCE DONE")
        print("=" * 60)
        
        print("\nDid you see the motor move?")
        print("  [YES] - Excellent! AT protocol works!")
        print("  [NO]  - We may need to decode the exact AT format")
        
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        motor.disable_motor()
        motor.disconnect()
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Control - AT Protocol\n")
    
    success = move_motor_at()
    
    if not success:
        print("\n" + "=" * 60)
        print("ALTERNATIVE APPROACH")
        print("=" * 60)
        print("\nIf AT protocol didn't work, let's capture exact commands.")
        print("\nRun this to see what Robstride software actually sends:")
        print("  python listen_for_can_traffic.py")
        print("\nThen:")
        print("  1. Keep that running")
        print("  2. Open Robstride software")
        print("  3. Move the motor")
        print("  4. Copy the hex output")
        print("  5. We'll decode it and replicate exactly!")

