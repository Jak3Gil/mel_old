#!/usr/bin/env python3
"""
Robstride Motor 14 Control
Production class for motor CAN ID 14
CAN encoding: e8 68
"""

import serial
import time
import math

class RobstrideMotor14:
    """Motor 14 controller."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.can_id = 14
        self.ser = None
        self.connected = False
        self.activated = False
        
        # CAN ID encoding for motor 14 (discovered via scan)
        self.can_id_encoded = bytes([0xe8, 0x74])
    
    def connect(self):
        """Connect and initialize motor 14."""
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.connected = True
            print("    [OK] Connected\n")
            
            # AT+AT handshake
            print("[2] Initializing motor 14...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
            
            # Scan for motor 14
            print("    Detecting motor 14...")
            scan_cmd = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd)
            time.sleep(0.5)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    Motor 14 detected and activated!")
                self.activated = True
            
            print("    [OK] Initialization complete\n")
            time.sleep(1.0)
            
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
            self.connected = False
        print("Disconnected\n")
    
    def jog(self, direction=1, duration=1.0):
        """
        Jog motor 14.
        
        Args:
            direction: 1 for forward, -1 for reverse
            duration: How long to jog (seconds)
        """
        if not self.connected or not self.activated:
            print("Motor not connected/activated!")
            return False
        
        # Build jog command
        if direction > 0:
            # Forward
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            # Reverse
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        
        # Start jog
        self.ser.write(jog_cmd)
        time.sleep(duration)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        
        return True
    
    def jog_forward(self, duration=1.0):
        """Jog forward for specified duration."""
        return self.jog(direction=1, duration=duration)
    
    def jog_reverse(self, duration=1.0):
        """Jog reverse for specified duration."""
        return self.jog(direction=-1, duration=duration)
    
    def stop(self):
        """Immediately stop motor."""
        if not self.connected:
            return False
        
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        return True


def demo_motor14():
    """Demo motor 14 control."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR 14 - DEMO")
    print("=" * 70 + "\n")
    
    motor = RobstrideMotor14(port='COM3')
    
    try:
        # Connect
        if not motor.connect():
            return
        
        # Demo movements
        print("=" * 70)
        print("DEMO MOVEMENTS - Motor 14")
        print("=" * 70 + "\n")
        
        print("[1] Jog forward 2 seconds...")
        motor.jog_forward(2.0)
        print("    Done\n")
        time.sleep(0.5)
        
        print("[2] Jog reverse 2 seconds...")
        motor.jog_reverse(2.0)
        print("    Done\n")
        time.sleep(0.5)
        
        print("[3] Quick forward jog...")
        motor.jog_forward(0.5)
        print("    Done\n")
        time.sleep(0.5)
        
        print("[4] Quick reverse jog...")
        motor.jog_reverse(0.5)
        print("    Done\n")
        
        motor.disconnect()
        
        print("=" * 70)
        print("[SUCCESS] Motor 14 demo complete!")
        print("=" * 70)
        
    except KeyboardInterrupt:
        print("\n\nStopping...")
        motor.stop()
        motor.disconnect()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    print("\nRobstride Motor 14 Control\n")
    print("Settings:")
    print("  Port: COM3")
    print("  Baud: 921600")
    print("  CAN ID: 14")
    print("  Encoding: e8 68\n")
    
    demo_motor14()
    
    print("\n\nUSAGE:")
    print("-" * 70)
    print("""
from robstride_motor14 import RobstrideMotor14

# Create motor 14 instance
motor = RobstrideMotor14(port='COM3')

# Connect
motor.connect()

# Move forward 2 seconds
motor.jog_forward(2.0)

# Move reverse 2 seconds  
motor.jog_reverse(2.0)

# Disconnect
motor.disconnect()
""")

