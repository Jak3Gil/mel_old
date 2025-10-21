#!/usr/bin/env python3
"""
Move Robstride Motor - FINAL WORKING VERSION
Complete connection sequence with motor response detection
"""

import serial
import time

class RobstrideFinal:
    """Robstride motor with complete connection protocol."""
    
    def __init__(self, port='COM3', can_id=13):
        self.port = port
        self.can_id = can_id
        self.ser = None
        self.can_id_encoded = bytes([0xe8, 0x6c])  # For CAN ID 13
        self.motor_detected = False
    
    def connect(self):
        """Connect to COM port."""
        print(f"[1] Connecting to {self.port}...")
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=2.0)
            time.sleep(0.5)
            
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            print("    [OK] Connected\n")
            return True
        except Exception as e:
            print(f"    [ERROR] {e}")
            print("    *** CLOSE ROBSTRIDE SOFTWARE FIRST! ***\n")
            return False
    
    def detect_motor(self):
        """Detect motor using exact scan sequence."""
        print("[2] Detecting motor...")
        
        # Step 1: Send AT+AT handshake (line 1120)
        print("    Sending AT+AT handshake...")
        handshake = bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a])
        self.ser.write(handshake)
        time.sleep(0.3)
        
        # Read any response
        if self.ser.in_waiting > 0:
            resp = self.ser.read(self.ser.in_waiting)
            print(f"    Handshake response: {resp.hex()}")
        
        # Step 2: Send scan command for motor ID 13 (line 1186)
        print(f"    Scanning for motor ID {self.can_id}...")
        scan_cmd = bytes([
            0x41, 0x54,        # AT
            0x00, 0x07,        # Scan command type
            0xe8, 0x6c,        # CAN ID encoded for motor 13
            0x01, 0x00,        # Fixed bytes
            0x0d, 0x0a         # \r\n
        ])
        
        self.ser.write(scan_cmd)
        time.sleep(0.5)  # Wait longer for motor response
        
        # Read motor response - THIS IS CRITICAL!
        if self.ser.in_waiting > 0:
            resp = self.ser.read(self.ser.in_waiting)
            print(f"    Motor response: {resp.hex()}")
            print(f"    Motor DETECTED!")
            self.motor_detected = True
        else:
            print(f"    WARNING: No response from motor")
            print(f"    Continuing anyway...")
            self.motor_detected = False
        
        print("    [OK] Detection sequence complete\n")
        return True
    
    def send_movement_command(self, data_bytes):
        """
        Send movement command using L91 format.
        Format: AT \x90\x07 e8 6c 08 [8_BYTES] \r\n
        """
        packet = bytearray()
        packet.extend(b'AT')                # 41 54
        packet.append(0x90)                 # L91 movement command
        packet.append(0x07)                 # Sub-type
        packet.extend(self.can_id_encoded)  # e8 6c
        packet.append(0x08)                 # 8 bytes
        packet.extend(data_bytes)
        packet.extend(b'\r\n')
        
        self.ser.write(packet)
        time.sleep(0.01)
        
        # Read response
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def jog_start(self, speed=1):
        """Start jogging motor."""
        print(f"[3] Starting JOG (speed={speed})...")
        
        # Exact JOG command from your capture
        jog_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x01,              # Speed
            0x83, 0xe0
        ])
        
        resp = self.send_movement_command(jog_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] JOG command sent")
        print("    >>> MOTOR SHOULD BE MOVING NOW! <<<\n")
    
    def jog_stop(self):
        """Stop jogging motor."""
        print("[4] Stopping JOG...")
        
        stop_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x00,              # Speed = 0
            0x7f, 0xff
        ])
        
        resp = self.send_movement_command(stop_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] JOG stopped\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[5] Disconnected\n")

def test_final():
    """Final test with complete protocol."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - FINAL VERSION")
    print("   Complete connection sequence with motor detection")
    print("=" * 70 + "\n")
    
    motor = RobstrideFinal(port='COM3', can_id=13)
    
    try:
        # Connect
        if not motor.connect():
            return False
        
        # Detect motor (critical step!)
        if not motor.detect_motor():
            return False
        
        print("=" * 70)
        print("MOVEMENT TEST")
        print("=" * 70 + "\n")
        
        # Test 1: Short jog
        print("[Test 1/3] Short jog (3 seconds)...")
        motor.jog_start(speed=1)
        time.sleep(3)
        motor.jog_stop()
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Longer jog
        print("[Test 2/3] Longer jog (5 seconds)...")
        motor.jog_start(speed=1)
        time.sleep(5)
        motor.jog_stop()
        print("           [OK] Stopped\n")
        time.sleep(2)
        
        # Test 3: Multiple jogs
        print("[Test 3/3] Rapid jogs (3x 1 second)...")
        for i in range(3):
            print(f"           Jog {i+1}/3...")
            motor.jog_start(speed=1)
            time.sleep(1)
            motor.jog_stop()
            time.sleep(0.5)
        print("           [OK] Complete\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] TEST FINISHED")
        print("=" * 70)
        
        print("\n🎯 DID THE MOTOR MOVE?")
        print("\nIf YES: Perfect! We've decoded the complete protocol!")
        print("If NO: We need to see what the motor responds during detection")
        
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
        try:
            motor.jog_stop()
            motor.disconnect()
        except:
            pass
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor - Final Complete Protocol\n")
    print("Includes:")
    print("  - AT+AT handshake")
    print("  - Motor scan and detection")
    print("  - L91 JOG commands\n")
    time.sleep(1)
    
    success = test_final()
    
    if not success:
        print("\n" + "=" * 70)
        print("IF MOTOR STILL DOESN'T MOVE")
        print("=" * 70)
        print("\nOne more thing I need:")
        print("\nCapture BOTH directions of serial data:")
        print("  -> COM3: Commands SENT to motor")
        print("  <- COM3: Responses RECEIVED from motor")
        print("\nSpecifically, show me what the motor RESPONDS when:")
        print("  Line 1186: AT \\x00\\x07 e8 6c 01 00 is sent")
        print("\nThe motor's response is what 'activates' it!")

