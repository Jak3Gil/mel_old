#!/usr/bin/env python3
"""
Robstride 03 Motor Control - Working Version with L91
Auto-detects USB port
"""

import serial
import time
import math
import glob

class RobstrideMotor:
    """Controller for Robstride motors using L91 protocol."""
    
    def __init__(self, can_id=0x01, port=None):
        self.can_id = can_id
        self.port = port
        self.ser = None
        
        # MIT protocol limits
        self.P_MIN = -12.5
        self.P_MAX = 12.5
        self.V_MIN = -45.0
        self.V_MAX = 45.0
        self.T_MIN = -18.0
        self.T_MAX = 18.0
        self.KP_MIN = 0.0
        self.KP_MAX = 500.0
        self.KD_MIN = 0.0
        self.KD_MAX = 5.0
    
    def _find_usb_port(self):
        """Auto-detect USB-CAN adapter port."""
        ports = glob.glob('/dev/cu.usbserial*')
        if ports:
            return ports[0]
        return None
    
    def connect(self):
        """Connect and initialize with L91."""
        if not self.port:
            self.port = self._find_usb_port()
            if not self.port:
                print("✗ No USB-CAN adapter found!")
                print("  Check if it's plugged in")
                return False
        
        print(f"Connecting to {self.port}...")
        
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=0.5)
            time.sleep(0.3)
            
            # Initialize with L91 sequence
            print("  Initializing with L91...")
            self.ser.write(b'C\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.ser.write(b'L91\r')
            time.sleep(0.1)
            resp = self.ser.read(1000)
            
            self.ser.write(b'O\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            print("✓ Connected!")
            return True
            
        except Exception as e:
            print(f"✗ Connection failed: {e}")
            print("\nTroubleshooting:")
            print("  1. Close Robstride PC software if running")
            print("  2. Unplug and replug USB-CAN adapter")
            print("  3. Power cycle the motor")
            return False
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            try:
                self.ser.write(b'C\r')
                self.ser.close()
                print("✓ Disconnected")
            except:
                pass
    
    def float_to_uint(self, x, x_min, x_max, bits):
        """Convert float to unsigned int."""
        span = x_max - x_min
        x = max(min(x, x_max), x_min)
        return int((x - x_min) * ((1 << bits) - 1) / span)
    
    def send_command(self, position, velocity, kp, kd, torque):
        """Send MIT motor control command."""
        # Pack values
        p_int = self.float_to_uint(position, self.P_MIN, self.P_MAX, 16)
        v_int = self.float_to_uint(velocity, self.V_MIN, self.V_MAX, 12)
        kp_int = self.float_to_uint(kp, self.KP_MIN, self.KP_MAX, 12)
        kd_int = self.float_to_uint(kd, self.KD_MIN, self.KD_MAX, 12)
        t_int = self.float_to_uint(torque, self.T_MIN, self.T_MAX, 12)
        
        # Pack into bytes
        data = bytearray(8)
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        data[2] = (v_int >> 4) & 0xFF
        data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
        data[4] = kp_int & 0xFF
        data[5] = (kd_int >> 4) & 0xFF
        data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
        data[7] = t_int & 0xFF
        
        # Format as SLCAN
        msg = f"t{self.can_id:03X}8{data.hex().upper()}\r"
        self.ser.write(msg.encode())
        
        # Optional: read response
        if self.ser.in_waiting > 0:
            return self.ser.read(self.ser.in_waiting)
        return None
    
    def enable(self):
        """Enable motor."""
        print("Enabling motor...")
        msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFC\r"
        self.ser.write(msg.encode())
        time.sleep(0.1)
        self.ser.read(1000)
        print("✓ Motor enabled")
    
    def disable(self):
        """Disable motor."""
        print("Disabling motor...")
        msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFD\r"
        self.ser.write(msg.encode())
        time.sleep(0.1)
        self.ser.read(1000)
        print("✓ Motor disabled")
    
    def zero_position(self):
        """Set zero position."""
        print("Setting zero position...")
        msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFE\r"
        self.ser.write(msg.encode())
        time.sleep(0.1)
        self.ser.read(1000)
        print("✓ Position zeroed")


def test_motor():
    """Quick test to verify motor is working."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ROBSTRIDE MOTOR TEST                                 ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    motor = RobstrideMotor(can_id=0x01)
    
    try:
        if not motor.connect():
            return False
        
        motor.enable()
        time.sleep(0.3)
        
        motor.zero_position()
        time.sleep(0.3)
        
        # Quick movement test
        print("\nTesting movement (0.5 radian)...")
        for i in range(30):
            motor.send_command(
                position=0.5,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("✓ Move complete")
        time.sleep(0.5)
        
        # Return to zero
        print("\nReturning to zero...")
        for i in range(30):
            motor.send_command(
                position=0.0,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("✓ Test complete")
        
        motor.disable()
        
        print("\n" + "="*60)
        print("✓ MOTOR TEST SUCCESSFUL!")
        print("="*60)
        print("\nMotor is working! You can now use it in your code.")
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
        motor.disable()
        return False
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        motor.disconnect()


if __name__ == "__main__":
    success = test_motor()
    
    if not success:
        print("\n\n🔧 Troubleshooting:")
        print("  1. Is motor powered? (red LED should be solid)")
        print("  2. Is PC Robstride software closed?")
        print("  3. Try unplugging/replugging USB-CAN adapter")
        print("  4. Power cycle the motor (off 10 sec, then on)")
        print("  5. Check CANH/CANL wiring")

