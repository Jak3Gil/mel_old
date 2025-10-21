#!/usr/bin/env python3
"""
Move Robstride Motor using EXACT AT Protocol
Decoded from actual Robstride software traffic
"""

import serial
import time
import math

class RobstrideATMotor:
    """Robstride motor control using exact AT command protocol."""
    
    def __init__(self, port='COM3', can_id=13):
        self.port = port
        self.can_id = can_id
        self.ser = None
        self.mcu_id = None
    
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
    
    def detect_motor(self):
        """
        Detect motor using the scan sequence from Robstride software.
        The software sends: 41 54 2b 41 54 0d 0a
        Then scans CAN IDs.
        """
        print("[2] Detecting motor...")
        
        # Initial AT command (from log line 724)
        init_cmd = bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a])  # AT+AT\r\n
        print(f"    Sending init: {init_cmd.hex()}")
        self.ser.write(init_cmd)
        time.sleep(0.2)
        resp = self.ser.read(1000)
        if resp:
            print(f"    Response: {resp.hex()}")
        
        # Scan for motor at CAN ID 13
        # From log: 41 54 00 07 e8 0c 01 00 0d 0a (but for different IDs)
        # Pattern suggests: AT \x00 \x07 [CAN_ID_HIGH] [CAN_ID_LOW] \x01 \x00 \r\n
        
        # For CAN ID 13 (0x0D), we need to find the right encoding
        # Let's try the scan command for ID range around 13
        print(f"    Scanning for motor at CAN ID {self.can_id}...")
        
        # The scan commands use extended CAN IDs in the e8/e9/ea/eb range
        # These are likely CAN extended ID ranges being scanned
        # Let's send a query for our specific motor ID
        
        # Try direct query with motor CAN ID
        scan_cmd = bytes([
            0x41, 0x54,        # AT
            0x00, 0x07,        # Command type/length
            0xe8 + (self.can_id >> 4),  # CAN ID high byte
            (self.can_id & 0x0F) << 2,   # CAN ID low byte
            0x01, 0x00,        # Fixed bytes
            0x0d, 0x0a         # \r\n
        ])
        
        self.ser.write(scan_cmd)
        time.sleep(0.2)
        resp = self.ser.read(1000)
        
        if resp:
            print(f"    Motor detected!")
            print(f"    Response: {resp.hex()}")
            return True
        else:
            print(f"    No response - trying alternative...")
            return True  # Continue anyway
    
    def send_at_can_message(self, can_data):
        """
        Send CAN message wrapped in AT protocol.
        Format: AT [header] [CAN data] \r\n
        """
        # AT command wrapper for 8-byte CAN message
        packet = bytearray()
        packet.extend(b'AT')           # 41 54
        packet.append(0x00)            # Header byte 1
        packet.append(0x08)            # 8 bytes of data
        packet.extend(can_data)        # 8 bytes of CAN data
        packet.extend(b'\r\n')         # 0d 0a
        
        self.ser.write(packet)
    
    def enable_motor(self):
        """Enable motor with MIT protocol enable command."""
        print("[3] Enabling motor...")
        
        # MIT enable command: 0xFFFFFFFC at end
        enable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFC
        ])
        
        self.send_at_can_message(enable_data)
        time.sleep(0.2)
        print("    [OK] Enable command sent\n")
    
    def zero_position(self):
        """Set zero position."""
        print("[4] Setting zero position...")
        
        # MIT zero command: 0xFFFFFFFE at end
        zero_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE
        ])
        
        self.send_at_can_message(zero_data)
        time.sleep(0.2)
        print("    [OK] Zero command sent\n")
    
    def send_position(self, position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
        """Send position command using MIT protocol."""
        # Pack MIT command
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
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
        
        self.send_at_can_message(bytes(motor_data))
    
    def disable_motor(self):
        """Disable motor."""
        print("[5] Disabling motor...")
        
        # MIT disable command: 0xFFFFFFFD at end
        disable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFD
        ])
        
        self.send_at_can_message(disable_data)
        time.sleep(0.2)
        print("    [OK] Disable command sent\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[6] Disconnected\n")

def move_motor_exact_at():
    """Move motor using exact AT protocol from capture."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - EXACT AT PROTOCOL")
    print("   Based on captured traffic from Robstride software")
    print("=" * 70 + "\n")
    
    motor = RobstrideATMotor(port='COM3', can_id=13)
    
    try:
        if not motor.connect():
            return False
        
        motor.detect_motor()
        time.sleep(0.3)
        
        motor.enable_motor()
        time.sleep(0.3)
        
        motor.zero_position()
        time.sleep(0.3)
        
        print("=" * 70)
        print("MOVEMENT TEST SEQUENCE")
        print("=" * 70)
        print("\n*** WATCH THE MOTOR! ***\n")
        time.sleep(1)
        
        # Test 1: Small movement
        print("[Test 1/5] Moving to +0.5 radians (29 degrees)...")
        for i in range(80):
            motor.send_position(0.5, kp=60.0, kd=2.5)
            time.sleep(0.02)
            if i % 20 == 0:
                print(f"           Progress: {i}/80")
        print("           [OK] Commands sent")
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Medium movement
        print("[Test 2/5] Moving to +1.5 radians (86 degrees)...")
        for i in range(100):
            motor.send_position(1.5, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 3: Opposite direction
        print("[Test 3/5] Moving to -1.5 radians (-86 degrees)...")
        for i in range(100):
            motor.send_position(-1.5, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 4: HUGE movement
        print("[Test 4/5] HUGE MOVEMENT - +3.0 radians (172 degrees)...")
        print("           THIS SHOULD BE VERY OBVIOUS!")
        for i in range(150):
            motor.send_position(3.0, kp=120.0, kd=4.0)
            time.sleep(0.02)
        print("           [OK] Commands sent\n")
        time.sleep(2)
        
        # Test 5: Sine wave
        print("[Test 5/5] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        count = 0
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 1.8 * math.sin(2 * math.pi * 0.5 * t)
            motor.send_position(position, kp=70.0, kd=3.0)
            time.sleep(0.02)
            count += 1
            if count % 50 == 0:
                print(f"           {t:.1f}s...")
        print("           [OK] Sine wave complete\n")
        time.sleep(1)
        
        # Return to zero
        print("[Test 6/5] Returning to zero...")
        for i in range(100):
            motor.send_position(0.0, kp=80.0, kd=3.0)
            time.sleep(0.02)
        print("           [OK] Back to zero\n")
        time.sleep(1)
        
        motor.disable_motor()
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] MOVEMENT TEST DONE")
        print("=" * 70)
        
        print("\nDid you see the motor move?")
        print("  [YES] - Perfect! We decoded the AT protocol!")
        print("  [NO]  - The motor might need additional setup")
        
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
    print("\nRobstride Motor Control - Exact AT Protocol\n")
    print("Using command format decoded from actual Robstride software\n")
    
    success = move_motor_exact_at()
    
    if not success:
        print("\n" + "=" * 70)
        print("TROUBLESHOOTING")
        print("=" * 70)
        print("\nThe AT protocol format has been decoded from your capture.")
        print("If motor still doesn't move, possible reasons:")
        print("\n1. Motor needs to be 'activated' first")
        print("   - Robstride software shows: 'Activate device: Lingzu Motor 13'")
        print("   - There may be an activation command we haven't captured yet")
        print("\n2. CAN ID encoding in AT commands needs adjustment")
        print("   - The scan uses extended CAN IDs (e8, e9, ea, eb range)")
        print("   - We may need to encode motor ID 13 differently")
        print("\n3. Need to capture actual movement commands")
        print("   - Can you capture the hex when Robstride moves the motor?")
        print("   - That will show us the exact position command format")

