#!/usr/bin/env python3
"""
Dual Motor Control - Control Motors 13 and 14 Together
Synchronized movement of both motors
"""

import serial
import time

class DualMotorController:
    """Controller for both Motor 13 and Motor 14."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.connected = False
        
        # Motor 13 and 14 CAN ID encodings
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
    
    def connect(self):
        """Connect and initialize both motors."""
        print("=" * 70)
        print("   DUAL MOTOR CONTROLLER")
        print("=" * 70 + "\n")
        
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("    [OK] Connected\n")
            
            # AT+AT handshake
            print("[2] Initializing...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
            
            # Scan for Motor 13
            print("    Detecting Motor 13...")
            scan_cmd_13 = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd_13)
            time.sleep(0.3)
            if self.ser.in_waiting > 0:
                self.ser.read(self.ser.in_waiting)
                print("    [OK] Motor 13 activated")
            
            # Scan for Motor 14
            print("    Detecting Motor 14...")
            scan_cmd_14 = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd_14)
            time.sleep(0.3)
            if self.ser.in_waiting > 0:
                self.ser.read(self.ser.in_waiting)
                print("    [OK] Motor 14 activated")
            
            print("    [OK] Both motors initialized\n")
            self.connected = True
            time.sleep(1.0)
            
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def send_jog(self, motor_encoding, direction=1, duration=1.0):
        """Send jog command to specific motor."""
        if not self.connected:
            return False
        
        # Build jog command
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + motor_encoding + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        
        self.ser.write(jog_cmd)
        time.sleep(duration)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + motor_encoding + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        
        return True
    
    def motor13_forward(self, duration=1.0):
        """Move motor 13 forward."""
        return self.send_jog(self.motor13_encoding, direction=1, duration=duration)
    
    def motor13_reverse(self, duration=1.0):
        """Move motor 13 reverse."""
        return self.send_jog(self.motor13_encoding, direction=-1, duration=duration)
    
    def motor14_forward(self, duration=1.0):
        """Move motor 14 forward."""
        return self.send_jog(self.motor14_encoding, direction=1, duration=duration)
    
    def motor14_reverse(self, duration=1.0):
        """Move motor 14 reverse."""
        return self.send_jog(self.motor14_encoding, direction=-1, duration=duration)
    
    def both_forward(self, duration=1.0):
        """Move both motors forward simultaneously."""
        if not self.connected:
            return False
        
        # Send start commands to both motors
        jog_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        
        # Start motor 13
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor13_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        self.ser.write(cmd_13)
        time.sleep(0.01)
        
        # Start motor 14
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor14_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        self.ser.write(cmd_14)
        
        # Wait
        time.sleep(duration)
        
        # Stop both
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_13 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor13_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        stop_14 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor14_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.write(stop_13)
        time.sleep(0.01)
        self.ser.write(stop_14)
        
        return True
    
    def both_reverse(self, duration=1.0):
        """Move both motors reverse simultaneously."""
        if not self.connected:
            return False
        
        # Send start commands to both motors
        jog_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        # Start motor 13
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor13_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        self.ser.write(cmd_13)
        time.sleep(0.01)
        
        # Start motor 14
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor14_encoding + bytes([0x08]) + jog_data + bytes([0x0d, 0x0a])
        self.ser.write(cmd_14)
        
        # Wait
        time.sleep(duration)
        
        # Stop both
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_13 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor13_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        stop_14 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor14_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        
        self.ser.write(stop_13)
        time.sleep(0.01)
        self.ser.write(stop_14)
        
        return True
    
    def stop_all(self):
        """Stop both motors immediately."""
        if not self.connected:
            return False
        
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        
        # Stop motor 13
        stop_13 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor13_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        self.ser.write(stop_13)
        
        # Stop motor 14
        stop_14 = bytes([0x41, 0x54, 0x90, 0x07]) + self.motor14_encoding + bytes([0x08]) + stop_data + bytes([0x0d, 0x0a])
        self.ser.write(stop_14)
        
        return True
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
            self.connected = False
        print("\nDisconnected")


def demo_dual_motors():
    """Demo controlling both motors."""
    print("\nDual Motor Control Demo\n")
    
    motors = DualMotorController()
    
    try:
        # Connect
        if not motors.connect():
            return
        
        print("=" * 70)
        print("DUAL MOTOR DEMONSTRATIONS")
        print("=" * 70 + "\n")
        
        # Test 1: Individual motors
        print("[1] Testing individual motors...")
        print("    Motor 13 forward...")
        motors.motor13_forward(1.5)
        time.sleep(0.5)
        
        print("    Motor 14 forward...")
        motors.motor14_forward(1.5)
        time.sleep(0.5)
        print("    Done\n")
        
        # Test 2: Both motors together
        print("[2] Both motors forward (synchronized)...")
        motors.both_forward(2.0)
        print("    Done\n")
        time.sleep(0.5)
        
        print("[3] Both motors reverse (synchronized)...")
        motors.both_reverse(2.0)
        print("    Done\n")
        time.sleep(0.5)
        
        # Test 3: Alternating
        print("[4] Alternating motors...")
        for i in range(3):
            print(f"    Round {i+1}/3")
            motors.motor13_forward(0.5)
            time.sleep(0.2)
            motors.motor14_forward(0.5)
            time.sleep(0.2)
        print("    Done\n")
        
        # Test 4: Opposite directions
        print("[5] Opposite directions...")
        print("    Motor 13 forward, Motor 14 reverse...")
        
        # Start motor 13 forward
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        motors.ser.write(cmd_13)
        
        # Start motor 14 reverse
        jog_rev = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_rev + bytes([0x0d, 0x0a])
        motors.ser.write(cmd_14)
        
        time.sleep(2.0)
        motors.stop_all()
        print("    Done\n")
        
        motors.disconnect()
        
        print("=" * 70)
        print("[SUCCESS] Dual motor demo complete!")
        print("=" * 70)
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        motors.stop_all()
        motors.disconnect()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    demo_dual_motors()
    
    print("\n\nUSAGE EXAMPLE:")
    print("=" * 70)
    print("""
from dual_motor_control import DualMotorController

# Create controller
motors = DualMotorController()

# Connect
motors.connect()

# Move individual motors
motors.motor13_forward(2.0)
motors.motor14_forward(2.0)

# Move both together
motors.both_forward(2.0)
motors.both_reverse(2.0)

# Stop all
motors.stop_all()

# Disconnect
motors.disconnect()
""")

