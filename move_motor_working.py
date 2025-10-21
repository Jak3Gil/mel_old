#!/usr/bin/env python3
"""
Move Robstride Motor - WORKING VERSION
Using exact command format from captured movement commands
"""

import serial
import time
import math

class RobstrideMotorWorking:
    """Robstride motor using exact working AT protocol."""
    
    def __init__(self, port='COM3', can_id=13):
        self.port = port
        self.can_id = can_id
        self.ser = None
        
        # For CAN ID 13, the encoding is: e8 6c
        # This appears to be: 0xe800 | (can_id << 2)
        # e8 6c = 0xe86c = 59500 decimal
        # Let's calculate: 0xe800 + (13 << 2) = 0xe800 + 52 = 0xe834... not quite
        # Actually looking at the pattern: e8 6c for ID 13
        # Let me try: (0xe800 | (can_id << 2)) + offset
        # For now, hardcode for ID 13
        self.can_id_encoded = bytes([0xe8, 0x6c])
    
    def connect(self):
        """Connect to adapter."""
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
    
    def send_motor_command(self, can_data):
        """
        Send motor command using exact format from capture.
        Format: AT \x90\x07 [CAN_ID_ENCODED] \x08 [8_BYTES_DATA] \r\n
        """
        packet = bytearray()
        packet.extend(b'AT')                # 41 54
        packet.append(0x90)                 # Command type for movement
        packet.append(0x07)                 # Sub-command
        packet.extend(self.can_id_encoded)  # e8 6c for ID 13
        packet.append(0x08)                 # 8 bytes of data
        packet.extend(can_data)             # The 8 bytes
        packet.extend(b'\r\n')              # 0d 0a
        
        self.ser.write(packet)
        
        # Read any response
        time.sleep(0.001)
        if self.ser.in_waiting > 0:
            self.ser.read(self.ser.in_waiting)
    
    def enable_motor(self):
        """Enable motor."""
        print("[2] Enabling motor...")
        
        # MIT enable: 0xFFFFFFFC at end
        enable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFC
        ])
        
        self.send_motor_command(enable_data)
        time.sleep(0.2)
        print("    [OK] Enable command sent\n")
    
    def zero_position(self):
        """Set zero position."""
        print("[3] Setting zero position...")
        
        # MIT zero: 0xFFFFFFFE at end
        zero_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE
        ])
        
        self.send_motor_command(zero_data)
        time.sleep(0.2)
        print("    [OK] Zero command sent\n")
    
    def send_position(self, position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
        """Send position command."""
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        # Pack MIT protocol values
        p_int = float_to_uint(position, -12.5, 12.5, 16)
        v_int = float_to_uint(velocity, -45.0, 45.0, 12)
        kp_int = float_to_uint(kp, 0.0, 500.0, 12)
        kd_int = float_to_uint(kd, 0.0, 5.0, 12)
        t_int = float_to_uint(torque, -18.0, 18.0, 12)
        
        # Pack into 8 bytes
        motor_data = bytearray(8)
        motor_data[0] = (p_int >> 8) & 0xFF
        motor_data[1] = p_int & 0xFF
        motor_data[2] = (v_int >> 4) & 0xFF
        motor_data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        motor_data[4] = kp_int & 0xFF
        motor_data[5] = (kd_int >> 4) & 0xFF
        motor_data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
        motor_data[7] = t_int & 0xFF
        
        self.send_motor_command(bytes(motor_data))
    
    def disable_motor(self):
        """Disable motor."""
        print("[4] Disabling motor...")
        
        # MIT disable: 0xFFFFFFFD at end
        disable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFD
        ])
        
        self.send_motor_command(disable_data)
        time.sleep(0.2)
        print("    [OK] Disable command sent\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[5] Disconnected\n")

def test_motor():
    """Test motor movement."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - WORKING VERSION")
    print("   Using exact command format: AT \\x90\\x07 [CAN_ID] \\x08 [DATA]")
    print("=" * 70 + "\n")
    
    motor = RobstrideMotorWorking(port='COM3', can_id=13)
    
    try:
        if not motor.connect():
            return False
        
        motor.enable_motor()
        time.sleep(0.5)
        
        motor.zero_position()
        time.sleep(0.5)
        
        print("=" * 70)
        print("MOVEMENT TEST")
        print("=" * 70)
        print("\n*** WATCH THE MOTOR CLOSELY! ***\n")
        time.sleep(1)
        
        # Test 1: Small movement with HIGH gains
        print("[Test 1/6] Moving to +0.5 radians (29 degrees)...")
        print("           Using HIGH gains: Kp=100, Kd=4")
        for i in range(100):
            motor.send_position(0.5, kp=100.0, kd=4.0)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"           Sending... {i}/100")
        print("           [OK] Commands sent")
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Bigger movement
        print("[Test 2/6] Moving to +1.5 radians (86 degrees)...")
        print("           Using Kp=120, Kd=5")
        for i in range(120):
            motor.send_position(1.5, kp=120.0, kd=5.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: Opposite direction
        print("[Test 3/6] Moving to -1.5 radians (-86 degrees)...")
        for i in range(120):
            motor.send_position(-1.5, kp=120.0, kd=5.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: HUGE movement
        print("[Test 4/6] HUGE MOVEMENT - +3.0 radians (172 degrees)...")
        print("           MAXIMUM gains: Kp=200, Kd=5")
        print("           THIS SHOULD BE EXTREMELY OBVIOUS!")
        for i in range(150):
            motor.send_position(3.0, kp=200.0, kd=5.0)
            time.sleep(0.02)
            if i % 50 == 0:
                print(f"           Commanding... {i}/150")
        print("           [OK] Commands sent")
        print("           >> THIS WAS ALMOST A HALF ROTATION! <<\n")
        time.sleep(3)
        
        # Test 5: Rapid oscillation
        print("[Test 5/6] Rapid oscillation (2 seconds)...")
        print("           Moving between +1 and -1 radians rapidly")
        for i in range(20):
            target = 1.0 if i % 2 == 0 else -1.0
            for j in range(5):
                motor.send_position(target, kp=150.0, kd=5.0)
                time.sleep(0.01)
            time.sleep(0.05)
        print("           [OK] Oscillation complete\n")
        time.sleep(1)
        
        # Test 6: Return to zero
        print("[Test 6/6] Returning to zero...")
        for i in range(120):
            motor.send_position(0.0, kp=100.0, kd=4.0)
            time.sleep(0.02)
        print("           [OK] Back to zero\n")
        time.sleep(1)
        
        motor.disable_motor()
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] TEST SEQUENCE DONE")
        print("=" * 70)
        
        print("\nDID YOU SEE THE MOTOR MOVE?")
        print("  [YES] - Perfect! The AT protocol is working!")
        print("  [NO]  - There might be an initialization step we're missing")
        
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
        motor.disable_motor()
        motor.disconnect()
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Control - Working Version\n")
    print("Command format: AT \\x90\\x07 e8 6c 08 [DATA] \\r\\n\n")
    
    success = test_motor()
    
    if not success:
        print("\n" + "=" * 70)
        print("IF MOTOR STILL DOESN'T MOVE")
        print("=" * 70)
        print("\nWe're using the exact format from Robstride software.")
        print("If it still doesn't work, we need to check:")
        print("\n1. Is there an initialization sequence before movement?")
        print("   - Capture commands from when you FIRST open Robstride")
        print("   - Before detecting the motor")
        print("\n2. Does motor need to be put in a specific mode?")
        print("   - In Robstride software, check motor settings")
        print("   - Look for 'Control Mode' or 'Protocol' settings")
        print("\n3. Try the 'MOVE_JOG' command format directly:")
        print("   - We saw: Send L91 Command MOVE_JOG")
        print("   - Data: 05 70 00 00 07 01 83 e0")
        print("   - This might be a jog command, not position control")

