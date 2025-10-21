#!/usr/bin/env python3
"""
Connect to Robstride Motor 14
Test connection and basic movement
"""

import serial
import time

class Motor14Controller:
    """Controller for Motor ID 14."""
    
    def __init__(self, port='COM3', baud=921600):
        self.port = port
        self.baud = baud
        self.can_id = 14
        self.ser = None
        
        # CAN ID encoding for motor 14
        # Pattern from motor 13: 0xe8 0x6c
        # For motor 14, we need to calculate the encoding
        # The pattern seems to be in the scan range
        self.can_id_encoded = bytes([0xe8, 0xac])  # Estimated for motor 14
    
    def connect(self):
        """Connect to motor 14."""
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
            
            # Scan for motor 14
            print(f"    Scanning for motor CAN ID {self.can_id}...")
            
            # Try the calculated encoding
            scan_cmd = bytes([
                0x41, 0x54, 0x00, 0x07, 0xe8, 0xac, 0x01, 0x00, 0x0d, 0x0a
            ])
            self.ser.write(scan_cmd)
            time.sleep(0.5)
            
            if self.ser.in_waiting > 0:
                resp = self.ser.read(self.ser.in_waiting)
                print(f"    *** MOTOR 14 RESPONDED! ***")
                print(f"    Response: {resp.hex()}")
                print(f"    Motor detected and activated!")
                return True
            else:
                print(f"    No response with encoding e8 ac")
                print(f"    Trying to find correct encoding...")
                
                # Try different encodings in the e8/e9/ea/eb range
                for high_byte in [0xe8, 0xe9, 0xea, 0xeb]:
                    for low_offset in range(0, 256, 4):
                        low_byte = 0x0c + low_offset
                        if low_byte > 0xff:
                            break
                        
                        test_cmd = bytes([
                            0x41, 0x54, 0x00, 0x07, high_byte, low_byte, 0x01, 0x00, 0x0d, 0x0a
                        ])
                        self.ser.write(test_cmd)
                        time.sleep(0.1)
                        
                        if self.ser.in_waiting > 0:
                            resp = self.ser.read(self.ser.in_waiting)
                            print(f"\n    *** FOUND MOTOR 14! ***")
                            print(f"    Encoding: {high_byte:02x} {low_byte:02x}")
                            print(f"    Response: {resp.hex()}")
                            self.can_id_encoded = bytes([high_byte, low_byte])
                            return True
                
                print(f"\n    Could not detect motor 14")
                return False
            
        except Exception as e:
            print(f"    [ERROR] {e}")
            return False
    
    def jog_forward(self, duration=1.0):
        """Jog motor 14 forward."""
        if not self.ser:
            print("Not connected!")
            return False
        
        # Forward jog command
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0, 0x0d, 0x0a
        ])
        
        self.ser.write(jog_cmd)
        time.sleep(duration)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff, 0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        
        return True
    
    def jog_reverse(self, duration=1.0):
        """Jog motor 14 reverse."""
        if not self.ser:
            print("Not connected!")
            return False
        
        # Reverse jog command
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e, 0x0d, 0x0a
        ])
        
        self.ser.write(jog_cmd)
        time.sleep(duration)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07
        ]) + self.can_id_encoded + bytes([
            0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff, 0x0d, 0x0a
        ])
        self.ser.write(stop_cmd)
        
        return True
    
    def stop(self):
        """Stop motor."""
        if self.ser:
            stop_cmd = bytes([
                0x41, 0x54, 0x90, 0x07
            ]) + self.can_id_encoded + bytes([
                0x08, 0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff, 0x0d, 0x0a
            ])
            self.ser.write(stop_cmd)
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            self.ser.close()
        print("\nDisconnected")


def test_motor14():
    """Test motor 14 connection and movement."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR 14 - CONNECTION TEST")
    print("=" * 70 + "\n")
    
    motor = Motor14Controller(port='COM3', baud=921600)
    
    try:
        # Connect
        if not motor.connect():
            print("\nFailed to connect to motor 14!")
            print("\nPossible reasons:")
            print("  1. Motor 14 not powered")
            print("  2. Wrong COM port")
            print("  3. Motor ID is not actually 14")
            print("  4. Motor not on CAN bus")
            return False
        
        print("\n[3] Testing movement...")
        time.sleep(1)
        
        print("    Testing forward...")
        motor.jog_forward(2.0)
        print("    Done")
        time.sleep(1)
        
        print("    Testing reverse...")
        motor.jog_reverse(2.0)
        print("    Done")
        
        motor.disconnect()
        
        print("\n" + "=" * 70)
        print("[SUCCESS] Motor 14 is working!")
        print("=" * 70)
        
        print(f"\nMotor 14 CAN ID encoding: {motor.can_id_encoded.hex()}")
        print("You can now use this motor with the control scripts!")
        
        return True
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        motor.stop()
        motor.disconnect()
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    print("\nRobstride Motor 14 - Connection Test\n")
    print("This will:")
    print("  1. Connect to motor 14")
    print("  2. Find the correct CAN ID encoding")
    print("  3. Test forward/reverse movement\n")
    
    time.sleep(1)
    test_motor14()

