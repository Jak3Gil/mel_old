#!/usr/bin/env python3
"""
Run Both Motors Simultaneously
Synchronized control of Motors 13 and 14
"""

import serial
import time

class DualMotorSync:
    """Synchronized control of both motors."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.ser = None
        
        self.motor13_encoding = bytes([0xe8, 0x6c])
        self.motor14_encoding = bytes([0xe8, 0x74])
    
    def connect(self):
        """Connect and initialize both motors."""
        print("=" * 70)
        print("   DUAL MOTOR SYNCHRONIZED CONTROL")
        print("=" * 70 + "\n")
        
        print(f"[1] Connecting to {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=2.0)
            time.sleep(0.5)
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            print("    [OK] Connected\n")
            
            # Initialize
            print("[2] Initializing...")
            self.ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            print("    [OK]\n")
            
            # Activate Motor 13
            print("[3] Activating Motor 13...")
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Enable MIT mode for Motor 13
            self.ser.write(bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            self.ser.write(bytes([0x41, 0x54, 0x18, 0x07, 0xe8, 0x6c, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            print("    [OK] Motor 13 ready\n")
            
            # Activate Motor 14
            print("[4] Activating Motor 14...")
            self.ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            
            # Enable MIT mode for Motor 14
            self.ser.write(bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            self.ser.write(bytes([0x41, 0x54, 0x18, 0x07, 0xe8, 0x74, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0a]))
            time.sleep(0.3)
            self.ser.read(1000)
            print("    [OK] Motor 14 ready\n")
            
            print("[OK] Both motors initialized and ready!\n")
            return True
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def jog_both_forward(self, duration=1.0):
        """Move both motors forward simultaneously."""
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        
        # Send to Motor 13
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        self.ser.write(cmd_13)
        
        # Send to Motor 14 immediately after
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        self.ser.write(cmd_14)
        
        # Wait
        time.sleep(duration)
        
        # Stop both
        self.stop_both()
    
    def jog_both_reverse(self, duration=1.0):
        """Move both motors reverse simultaneously."""
        jog_rev = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e])
        
        # Send to Motor 13
        cmd_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_rev + bytes([0x0d, 0x0a])
        self.ser.write(cmd_13)
        
        # Send to Motor 14
        cmd_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_rev + bytes([0x0d, 0x0a])
        self.ser.write(cmd_14)
        
        # Wait
        time.sleep(duration)
        
        # Stop both
        self.stop_both()
    
    def motor13_forward(self, duration=1.0):
        """Motor 13 only forward."""
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        
        self.ser.write(cmd)
        time.sleep(duration)
        
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        for i in range(3):
            self.ser.write(stop_cmd)
            time.sleep(0.02)
    
    def motor14_forward(self, duration=1.0):
        """Motor 14 only forward."""
        jog_fwd = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0])
        cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + jog_fwd + bytes([0x0d, 0x0a])
        
        self.ser.write(cmd)
        time.sleep(duration)
        
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        stop_cmd = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        for i in range(3):
            self.ser.write(stop_cmd)
            time.sleep(0.02)
    
    def stop_both(self):
        """Stop both motors."""
        stop_data = bytes([0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff])
        
        # Stop Motor 13
        stop_13 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        self.ser.write(stop_13)
        
        # Stop Motor 14
        stop_14 = bytes([0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08]) + stop_data + bytes([0x0d, 0x0a])
        self.ser.write(stop_14)
        
        # Send multiple times
        for i in range(3):
            self.ser.write(stop_13)
            self.ser.write(stop_14)
            time.sleep(0.02)
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("\nDisconnected")


def demo_synchronized():
    """Demo synchronized motor control."""
    motors = DualMotorSync()
    
    if not motors.connect():
        return
    
    try:
        print("=" * 70)
        print("SYNCHRONIZED MOVEMENT DEMO")
        print("=" * 70 + "\n")
        
        # Test 1: Both forward
        print("[1] Both motors FORWARD together (2 seconds)...")
        motors.jog_both_forward(2.0)
        print("    Done\n")
        time.sleep(1)
        
        # Test 2: Both reverse
        print("[2] Both motors REVERSE together (2 seconds)...")
        motors.jog_both_reverse(2.0)
        print("    Done\n")
        time.sleep(1)
        
        # Test 3: Alternating
        print("[3] Alternating (Motor 13, then Motor 14)...")
        for i in range(3):
            print(f"    Round {i+1}/3: Motor 13...")
            motors.motor13_forward(0.5)
            time.sleep(0.2)
            print(f"    Round {i+1}/3: Motor 14...")
            motors.motor14_forward(0.5)
            time.sleep(0.2)
        print("    Done\n")
        
        # Test 4: Synchronized longer movement
        print("[4] Both motors FORWARD (5 seconds)...")
        motors.jog_both_forward(5.0)
        print("    Done\n")
        
        motors.disconnect()
        
        print("=" * 70)
        print("[SUCCESS] Both motors working together!")
        print("=" * 70)
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        motors.stop_both()
        motors.disconnect()
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    print("\nDual Motor Synchronized Control\n")
    print("Both motors will move together!\n")
    time.sleep(1)
    
    demo_synchronized()







