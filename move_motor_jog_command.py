#!/usr/bin/env python3
"""
Move Robstride Motor using JOG command
Exact command from capture: 05 70 00 00 07 01 83 e0
"""

import serial
import time

class RobstrideJogControl:
    """Robstride motor using JOG commands from capture."""
    
    def __init__(self, port='COM3', can_id=13):
        self.port = port
        self.can_id = can_id
        self.ser = None
        self.can_id_encoded = bytes([0xe8, 0x6c])  # For CAN ID 13
    
    def connect(self):
        """Connect to adapter."""
        print(f"[1] Connecting to {self.port}...")
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=0.5)
            time.sleep(0.3)
            
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            print("    [OK] Connected\n")
            return True
        except Exception as e:
            print(f"    [ERROR] {e}")
            print("\n    *** CLOSE ROBSTRIDE SOFTWARE FIRST! ***\n")
            return False
    
    def send_l91_command(self, data_bytes):
        """
        Send L91 command using exact format from capture.
        Format: AT \x90\x07 e8 6c 08 [8_BYTES] \r\n
        """
        packet = bytearray()
        packet.extend(b'AT')                # 41 54
        packet.append(0x90)                 # L91 command type
        packet.append(0x07)                 # Sub-type
        packet.extend(self.can_id_encoded)  # e8 6c for motor 13
        packet.append(0x08)                 # 8 bytes follow
        packet.extend(data_bytes)           # The command data
        packet.extend(b'\r\n')              # 0d 0a
        
        self.ser.write(packet)
        time.sleep(0.01)
        
        # Read response
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def jog_start(self, speed=1):
        """
        Start jogging motor.
        From capture: 05 70 00 00 07 01 83 e0 (speed=1, flag=1)
        """
        print(f"[2] Starting jog (speed={speed})...")
        
        # Exact bytes from capture for speed=1, flag=1
        jog_data = bytes([
            0x05, 0x70,        # Command bytes
            0x00, 0x00,        # Padding?
            0x07,              # Parameter
            0x01,              # Speed = 1
            0x83, 0xe0         # Checksum or additional params
        ])
        
        resp = self.send_l91_command(jog_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] Jog start sent\n")
    
    def jog_stop(self):
        """
        Stop jogging motor.
        From capture: 05 70 00 00 07 00 7f ff (speed=0, flag=0)
        """
        print("[3] Stopping jog...")
        
        # Exact bytes from capture for speed=0, flag=0
        stop_data = bytes([
            0x05, 0x70,        # Command bytes
            0x00, 0x00,        # Padding?
            0x07,              # Parameter
            0x00,              # Speed = 0
            0x7f, 0xff         # Checksum or additional params
        ])
        
        resp = self.send_l91_command(stop_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] Jog stop sent\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[4] Disconnected\n")

def test_jog_commands():
    """Test motor jog commands."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - JOG COMMANDS")
    print("   Using EXACT commands from Robstride software capture")
    print("=" * 70 + "\n")
    print("IMPORTANT: Close Robstride software before running this!\n")
    print("=" * 70 + "\n")
    
    motor = RobstrideJogControl(port='COM3', can_id=13)
    
    try:
        if not motor.connect():
            return False
        
        print("=" * 70)
        print("JOG TEST SEQUENCE")
        print("=" * 70 + "\n")
        
        # Test 1: Short jog
        print("[Test 1/3] Short jog (2 seconds)...")
        motor.jog_start(speed=1)
        print("           MOTOR SHOULD BE MOVING NOW!")
        time.sleep(2)
        motor.jog_stop()
        print("           >> DID IT MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Longer jog
        print("[Test 2/3] Longer jog (5 seconds)...")
        motor.jog_start(speed=1)
        print("           Motor jogging...")
        time.sleep(5)
        motor.jog_stop()
        print("           [OK] Stopped\n")
        time.sleep(2)
        
        # Test 3: Multiple jogs
        print("[Test 3/3] Multiple short jogs...")
        for i in range(3):
            print(f"           Jog {i+1}/3...")
            motor.jog_start(speed=1)
            time.sleep(1)
            motor.jog_stop()
            time.sleep(1)
        print("           [OK] Complete\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] JOG TEST DONE")
        print("=" * 70)
        
        print("\nDID THE MOTOR MOVE?")
        print("  [YES] - Excellent! Jog commands work!")
        print("  [NO]  - Motor might need initialization sequence")
        
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
        motor.jog_stop()
        motor.disconnect()
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor - JOG Command Test\n")
    print("*** MAKE SURE ROBSTRIDE SOFTWARE IS CLOSED! ***\n")
    time.sleep(1)
    
    success = test_jog_commands()
    
    if not success:
        print("\n" + "=" * 70)
        print("TROUBLESHOOTING")
        print("=" * 70)
        print("\nIf jog didn't work, we need:")
        print("\n1. Full initialization sequence")
        print("   - Capture ALL commands from when you:")
        print("     a) Open Robstride software")
        print("     b) Click 'Connect'")
        print("     c) Motor is detected")
        print("     d) You click to move it")
        print("\n2. Check if motor needs 'enable' first")
        print("   - Before jog, there might be an enable command")
        print("\n3. Verify the CAN ID encoding")
        print("   - We're using e8 6c for motor 13")
        print("   - This might be different for your motor")

