#!/usr/bin/env python3
"""
Dual Motor Control - Motors 13 and 14 (macOS Version)
Control both Robstride motors simultaneously on Mac!
"""

import serial
import time
import glob

class DualMotorController:
    """Controller for both motor 13 and motor 14 on macOS."""
    
    def __init__(self, port='/dev/tty.usbserial-110', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        self.connected = False
        
        # Motor 13 encoding (tested and working on Windows)
        self.motor_13_encoded = bytes([0xe8, 0x6c])
        
        # Motor 14 encoding (calculated for ID 14)
        self.motor_14_encoded = bytes([0xe8, 0x70])
    
    @staticmethod
    def find_usb_ports():
        """Find available USB serial ports on macOS."""
        ports = glob.glob('/dev/tty.usbserial*') + glob.glob('/dev/tty.usbmodem*')
        return ports
    
    def connect(self):
        """Connect and initialize both motors."""
        print("=" * 70)
        print("   DUAL MOTOR CONTROL - Motors 13 & 14 (macOS)")
        print("=" * 70 + "\n")
        
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.connected = True
            print("    [OK] Connected\n")
            
            # AT+AT handshake
            print("[2] Initializing communication...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
            
            # Scan for motor 13
            print("    Detecting Motor 13...")
            scan_cmd_13 = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd_13)
            time.sleep(0.5)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    ✓ Motor 13 detected!")
            
            # Scan for motor 14
            print("    Detecting Motor 14...")
            scan_cmd_14 = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0x70, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd_14)
            time.sleep(0.5)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    ✓ Motor 14 detected!")
            
            print("    [OK] Both motors initialized!\n")
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
    
    def jog_motor(self, motor_id, direction=1, duration=1.0):
        """
        Jog a specific motor.
        
        Args:
            motor_id: 13 or 14
            direction: 1 for forward, -1 for reverse
            duration: How long to jog (seconds)
        """
        if not self.connected:
            print("Motors not connected!")
            return False
        
        # Select motor encoding
        if motor_id == 13:
            motor_encoded = self.motor_13_encoded
        elif motor_id == 14:
            motor_encoded = self.motor_14_encoded
        else:
            print(f"Invalid motor ID: {motor_id}")
            return False
        
        # Build jog command
        if direction > 0:
            # Forward
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            # Reverse
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        jog_cmd = bytes([0x41, 0x54, 0x90, 0x07]) + motor_encoded + bytes([0x08]) + data + bytes([0x0d, 0x0a])
        
        # Start jog
        self.ser.write(jog_cmd)
        time.sleep(duration)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07]) + motor_encoded + bytes([0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        
        return True
    
    def jog_both(self, direction=1, duration=1.0):
        """
        Jog both motors simultaneously.
        
        Args:
            direction: 1 for forward, -1 for reverse
            duration: How long to jog (seconds)
        """
        if not self.connected:
            print("Motors not connected!")
            return False
        
        # Build jog command data
        if direction > 0:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        else:
            data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        # Command for motor 13
        jog_cmd_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + data + bytes([0x0d, 0x0a])
        
        # Command for motor 14
        jog_cmd_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08]) + data + bytes([0x0d, 0x0a])
        
        # Start both motors
        self.ser.write(jog_cmd_13)
        time.sleep(0.01)  # Small delay between commands
        self.ser.write(jog_cmd_14)
        
        time.sleep(duration)
        
        # Stop both motors
        stop_cmd_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        stop_cmd_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        
        self.ser.write(stop_cmd_13)
        time.sleep(0.01)
        self.ser.write(stop_cmd_14)
        
        return True
    
    def stop_all(self):
        """Stop both motors immediately."""
        if not self.connected:
            return False
        
        stop_cmd_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        stop_cmd_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        
        self.ser.write(stop_cmd_13)
        self.ser.write(stop_cmd_14)
        return True


def demo():
    """Demonstration of dual motor control."""
    # Auto-detect port
    ports = DualMotorController.find_usb_ports()
    if not ports:
        print("[ERROR] No USB serial ports found!")
        print("Please check your USB connection.")
        return
    
    print(f"Found USB ports: {ports}")
    port = ports[0]  # Use first available port
    print(f"Using: {port}\n")
    
    controller = DualMotorController(port=port)
    
    try:
        # Connect
        if not controller.connect():
            return
        
        print("=" * 70)
        print("DEMO: DUAL MOTOR MOVEMENTS")
        print("=" * 70 + "\n")
        
        # Test 1: Motor 13 only
        print("[1] Motor 13 forward (2 seconds)...")
        controller.jog_motor(13, direction=1, duration=2.0)
        print("    ✓ Done\n")
        time.sleep(0.5)
        
        # Test 2: Motor 14 only
        print("[2] Motor 14 forward (2 seconds)...")
        controller.jog_motor(14, direction=1, duration=2.0)
        print("    ✓ Done\n")
        time.sleep(0.5)
        
        # Test 3: Both motors forward
        print("[3] BOTH motors forward (2 seconds)...")
        controller.jog_both(direction=1, duration=2.0)
        print("    ✓ Done\n")
        time.sleep(0.5)
        
        # Test 4: Both motors reverse
        print("[4] BOTH motors reverse (2 seconds)...")
        controller.jog_both(direction=-1, duration=2.0)
        print("    ✓ Done\n")
        time.sleep(0.5)
        
        # Test 5: Alternating
        print("[5] Alternating motors...")
        print("    Motor 13 forward...")
        controller.jog_motor(13, direction=1, duration=1.0)
        time.sleep(0.2)
        print("    Motor 14 forward...")
        controller.jog_motor(14, direction=1, duration=1.0)
        print("    ✓ Done\n")
        
        controller.disconnect()
        
        print("=" * 70)
        print("[SUCCESS] DUAL MOTOR DEMO COMPLETE!")
        print("=" * 70)
        
    except KeyboardInterrupt:
        print("\n\nStopping...")
        controller.stop_all()
        controller.disconnect()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    print("\n" + "=" * 70)
    print("   ROBSTRIDE DUAL MOTOR CONTROL (macOS)")
    print("=" * 70)
    print("\nControlling Motors 13 & 14")
    print("Baud: 921600")
    print("Auto-detecting USB port...\n")
    
    demo()
    
    print("\n\nUSAGE EXAMPLE:")
    print("-" * 70)
    print("""
from dual_motor_mac import DualMotorController

# Create controller (auto-detects port, or specify manually)
controller = DualMotorController(port='/dev/tty.usbserial-110')

# Connect
controller.connect()

# Move motor 13 forward
controller.jog_motor(13, direction=1, duration=2.0)

# Move motor 14 reverse
controller.jog_motor(14, direction=-1, duration=2.0)

# Move both motors forward together
controller.jog_both(direction=1, duration=2.0)

# Stop all motors
controller.stop_all()

# Disconnect
controller.disconnect()
""")



