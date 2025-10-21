#!/usr/bin/env python3
"""
Move Robstride Motor - WITH MCU ID ACTIVATION
Uses both MCU ID (0x1137239c20303d3c) and CAN ID (13)
"""

import serial
import time
import struct

class RobstrideWithMCU:
    """Robstride motor with MCU ID activation."""
    
    def __init__(self, port='COM3', can_id=13, mcu_id=0x1137239c20303d3c):
        self.port = port
        self.can_id = can_id
        self.mcu_id = mcu_id
        self.ser = None
        self.can_id_encoded = bytes([0xe8, 0x6c])  # For CAN ID 13
    
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
    
    def detect_and_activate(self):
        """Detect motor and activate using MCU ID."""
        print("[2] Detecting and activating motor...")
        print(f"    MCU ID: 0x{self.mcu_id:016X}")
        print(f"    CAN ID: {self.can_id}")
        
        # Step 1: AT+AT handshake
        print("\n    [Step 1] Sending AT+AT handshake...")
        handshake = bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a])
        self.ser.write(handshake)
        time.sleep(0.3)
        
        if self.ser.in_waiting > 0:
            resp = self.ser.read(self.ser.in_waiting)
            print(f"    Response: {resp.hex()}")
        
        # Step 2: Send scan command
        print("\n    [Step 2] Scanning for motor...")
        scan_cmd = bytes([
            0x41, 0x54,        # AT
            0x00, 0x07,        # Scan command
            0xe8, 0x6c,        # CAN ID encoded
            0x01, 0x00,        # Fixed
            0x0d, 0x0a         # \r\n
        ])
        
        self.ser.write(scan_cmd)
        time.sleep(0.5)
        
        if self.ser.in_waiting > 0:
            resp = self.ser.read(self.ser.in_waiting)
            print(f"    Scan response: {resp.hex()}")
        
        # Step 3: ACTIVATE using MCU ID
        # The software says "Activate device: Lingzu Motor 13"
        # This might require sending the MCU ID
        print("\n    [Step 3] ACTIVATING motor with MCU ID...")
        
        # Try activation command with MCU ID
        # Format might be: AT [CMD] [MCU_ID] [CAN_ID]
        activate_cmd = bytearray()
        activate_cmd.extend(b'AT')
        activate_cmd.append(0x00)          # Command type
        activate_cmd.append(0x09)          # 9 bytes follow? (8 for MCU ID + 1 for CAN ID)
        
        # Add MCU ID (8 bytes, little-endian)
        mcu_bytes = struct.pack('<Q', self.mcu_id)  # 64-bit little-endian
        activate_cmd.extend(mcu_bytes)
        
        # Add CAN ID
        activate_cmd.append(self.can_id)
        
        activate_cmd.extend(b'\r\n')
        
        print(f"    Activation command: {activate_cmd.hex()}")
        self.ser.write(activate_cmd)
        time.sleep(0.5)
        
        if self.ser.in_waiting > 0:
            resp = self.ser.read(self.ser.in_waiting)
            print(f"    Activation response: {resp.hex()}")
        
        print("    [OK] Activation sequence complete\n")
        return True
    
    def send_movement_command(self, data_bytes):
        """Send L91 movement command."""
        packet = bytearray()
        packet.extend(b'AT')
        packet.append(0x90)
        packet.append(0x07)
        packet.extend(self.can_id_encoded)
        packet.append(0x08)
        packet.extend(data_bytes)
        packet.extend(b'\r\n')
        
        self.ser.write(packet)
        time.sleep(0.01)
        
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def enable_motor(self):
        """Enable motor with MIT protocol."""
        print("[3] Enabling motor...")
        
        enable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFC
        ])
        
        resp = self.send_movement_command(enable_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        time.sleep(0.2)
        print("    [OK] Enable sent\n")
    
    def jog_start(self, speed=1):
        """Start jogging."""
        print(f"[4] Starting JOG (speed={speed})...")
        
        jog_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x01,
            0x83, 0xe0
        ])
        
        resp = self.send_movement_command(jog_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] JOG started - MOTOR SHOULD BE MOVING!\n")
    
    def jog_stop(self):
        """Stop jogging."""
        print("[5] Stopping JOG...")
        
        stop_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x00,
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
        print("[6] Disconnected\n")

def test_with_mcu():
    """Test with MCU ID activation."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - WITH MCU ID ACTIVATION")
    print("   MCU ID: 0x1137239c20303d3c")
    print("   CAN ID: 13")
    print("=" * 70 + "\n")
    
    motor = RobstrideWithMCU(
        port='COM3',
        can_id=13,
        mcu_id=0x1137239c20303d3c
    )
    
    try:
        # Connect
        if not motor.connect():
            return False
        
        # Detect and ACTIVATE
        if not motor.detect_and_activate():
            return False
        
        # Enable
        motor.enable_motor()
        
        print("=" * 70)
        print("MOVEMENT TEST")
        print("=" * 70 + "\n")
        
        # Test 1
        print("[Test 1/2] JOG for 3 seconds...")
        motor.jog_start(speed=1)
        print("           >>> WATCH THE MOTOR! <<<")
        time.sleep(3)
        motor.jog_stop()
        print("           >> DID IT MOVE? <<\n")
        time.sleep(2)
        
        # Test 2
        print("[Test 2/2] JOG for 5 seconds...")
        motor.jog_start(speed=1)
        time.sleep(5)
        motor.jog_stop()
        print("           [OK] Stopped\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] TEST FINISHED")
        print("=" * 70)
        
        print("\nDID THE MOTOR MOVE?")
        
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
    print("\nRobstride Motor - WITH MCU ID\n")
    print("Using BOTH identifiers:")
    print("  MCU ID: 0x1137239c20303d3c (64-bit unique identifier)")
    print("  CAN ID: 13 (CAN bus address)\n")
    time.sleep(1)
    
    success = test_with_mcu()
    
    if not success:
        print("\n" + "=" * 70)
        print("STILL NO MOVEMENT?")
        print("=" * 70)
        print("\nI need to see the motor's RESPONSE during activation.")
        print("\nIn your serial capture, show me:")
        print("  -> COM3: [commands sent TO motor]")
        print("  <- COM3: [responses FROM motor]  ← THIS IS WHAT I NEED!")
        print("\nSpecifically the response after line 1186 (the scan that finds motor 13)")

