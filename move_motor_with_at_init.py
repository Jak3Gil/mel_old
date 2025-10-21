#!/usr/bin/env python3
"""
Move Robstride Motor with proper AT initialization
Includes CH340 driver AT command setup
"""

import serial
import time

class RobstrideWithATInit:
    """Robstride motor with full AT initialization."""
    
    def __init__(self, port='COM3', can_id=13):
        self.port = port
        self.can_id = can_id
        self.ser = None
        self.can_id_encoded = bytes([0xe8, 0x6c])  # For CAN ID 13
    
    def connect_and_init(self):
        """Connect and perform full AT initialization."""
        print(f"[1] Connecting to {self.port}...")
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=1.0)
            time.sleep(0.5)
            
            # Clear buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            
            print("    [OK] Connected\n")
            
            # Perform AT initialization sequence
            print("[2] Initializing with AT commands...")
            
            # From your capture, the first command is: AT+AT
            init_commands = [
                (b'AT+AT\r\n', "AT+AT - Initial handshake"),
                (b'AT\r\n', "AT - Basic AT command"),
                (b'ATE0\r\n', "ATE0 - Echo off"),
                (b'ATZ\r\n', "ATZ - Reset"),
                (b'AT+BAUD=115200\r\n', "Set baudrate"),
                (b'AT+CAN=1000\r\n', "Set CAN speed to 1Mbps"),
                (b'AT+MODE=1\r\n', "Set mode"),
            ]
            
            for i, (cmd, desc) in enumerate(init_commands, 1):
                print(f"    [{i}] {desc}")
                print(f"        Sending: {cmd.decode('ascii', errors='ignore').strip()}")
                self.ser.write(cmd)
                time.sleep(0.3)
                
                # Read response
                if self.ser.in_waiting > 0:
                    resp = self.ser.read(self.ser.in_waiting)
                    print(f"        Response: {resp.hex()} ({resp.decode('ascii', errors='ignore').strip()})")
                else:
                    print(f"        No response")
            
            print("    [OK] AT initialization complete\n")
            
            # Now detect the motor
            print("[3] Detecting motor...")
            detect_cmd = bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a])  # AT+AT
            self.ser.write(detect_cmd)
            time.sleep(0.3)
            resp = self.ser.read(1000)
            if resp:
                print(f"    Response: {resp.hex()}")
            print("    [OK] Motor detection sent\n")
            
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def send_l91_command(self, data_bytes):
        """Send L91 command."""
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
        
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def enable_motor(self):
        """Enable motor with MIT protocol."""
        print("[4] Enabling motor...")
        
        # MIT enable command
        enable_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFC
        ])
        
        resp = self.send_l91_command(enable_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        time.sleep(0.2)
        print("    [OK] Enable sent\n")
    
    def zero_position(self):
        """Set zero position."""
        print("[5] Setting zero position...")
        
        zero_data = bytes([
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFE
        ])
        
        resp = self.send_l91_command(zero_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        time.sleep(0.2)
        print("    [OK] Zero sent\n")
    
    def jog_start(self, speed=1):
        """Start jogging."""
        print(f"[6] Starting JOG (speed={speed})...")
        
        # Exact bytes from your capture
        jog_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x01,              # Speed
            0x83, 0xe0
        ])
        
        resp = self.send_l91_command(jog_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] JOG started - MOTOR SHOULD BE MOVING!\n")
    
    def jog_stop(self):
        """Stop jogging."""
        print("[7] Stopping JOG...")
        
        stop_data = bytes([
            0x05, 0x70,
            0x00, 0x00,
            0x07,
            0x00,              # Speed = 0
            0x7f, 0xff
        ])
        
        resp = self.send_l91_command(stop_data)
        if resp:
            print(f"    Response: {resp.hex()}")
        print("    [OK] JOG stopped\n")
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("[8] Disconnected\n")

def test_with_full_init():
    """Test motor with full AT initialization."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - WITH AT INITIALIZATION")
    print("   Including CH340 driver setup")
    print("=" * 70 + "\n")
    
    motor = RobstrideWithATInit(port='COM3', can_id=13)
    
    try:
        if not motor.connect_and_init():
            return False
        
        # Enable motor
        motor.enable_motor()
        time.sleep(0.5)
        
        # Zero position
        motor.zero_position()
        time.sleep(0.5)
        
        print("=" * 70)
        print("MOVEMENT TEST")
        print("=" * 70 + "\n")
        
        # Test 1: Short jog
        print("[Test 1/2] Short jog (3 seconds)...")
        motor.jog_start(speed=1)
        print("           >>> WATCH THE MOTOR! <<<")
        time.sleep(3)
        motor.jog_stop()
        print("           >> DID IT MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Longer jog
        print("[Test 2/2] Longer jog (5 seconds)...")
        motor.jog_start(speed=1)
        time.sleep(5)
        motor.jog_stop()
        print("           [OK] Stopped\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[COMPLETE] TEST DONE")
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
    print("\nRobstride Motor - With Full AT Initialization\n")
    print("This includes CH340 driver AT command setup\n")
    time.sleep(1)
    
    success = test_with_full_init()
    
    if not success:
        print("\n" + "=" * 70)
        print("NEXT STEP")
        print("=" * 70)
        print("\nIf motor still doesn't move, I need to see:")
        print("\n1. What AT commands does Robstride software send to 'connect'?")
        print("   - Capture from the moment you click 'Connect'")
        print("   - Show me ALL AT commands before the motor is 'ready'")
        print("\n2. Are there specific CH340 initialization commands?")
        print("   - The CH340 driver might need special setup")
        print("\n3. What do you see in Robstride after clicking 'Connect'?")
        print("   - Does it show 'Connected', 'Enabled', 'Ready'?")
        print("   - Are there any buttons you need to click?")

