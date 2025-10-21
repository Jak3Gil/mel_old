#!/usr/bin/env python3
"""
Move Robstride 03 Motor on COM3
Simple script to connect and move the motor
"""

import serial
import time
import math

class RobstrideMotorCOM3:
    """Controller for Robstride motor on COM3."""
    
    def __init__(self, can_id=0x01, port='COM3'):
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
    
    def connect(self):
        """Connect and initialize with L91."""
        print(f"Connecting to {self.port}...")
        
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=0.5)
            time.sleep(0.3)
            
            # Initialize with L91 sequence
            print("  Initializing with L91 protocol...")
            self.ser.write(b'C\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.ser.write(b'L91\r')
            time.sleep(0.1)
            resp = self.ser.read(1000)
            
            self.ser.write(b'O\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            print("[OK] Connected!")
            return True
            
        except Exception as e:
            print(f"[ERROR] Connection failed: {e}")
            print("\nTroubleshooting:")
            print("  1. Close Robstride PC software if running")
            print("  2. Unplug and replug USB-CAN adapter")
            print("  3. Power cycle the motor")
            print("  4. Check that COM3 is the correct port")
            return False
    
    def disconnect(self):
        """Close connection."""
        if self.ser:
            try:
                self.ser.write(b'C\r')
                self.ser.close()
                print("[OK] Disconnected")
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
        print("[OK] Motor enabled")
    
    def disable(self):
        """Disable motor."""
        print("Disabling motor...")
        msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFD\r"
        self.ser.write(msg.encode())
        time.sleep(0.1)
        self.ser.read(1000)
        print("[OK] Motor disabled")
    
    def zero_position(self):
        """Set zero position."""
        print("Setting zero position...")
        msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFE\r"
        self.ser.write(msg.encode())
        time.sleep(0.1)
        self.ser.read(1000)
        print("[OK] Position zeroed")


def move_motor():
    """Connect to COM3 and move the Robstride 03 motor."""
    print("=" * 60)
    print("   ROBSTRIDE 03 MOTOR - COM3")
    print("=" * 60 + "\n")
    
    motor = RobstrideMotorCOM3(can_id=0x01, port='COM3')
    
    try:
        if not motor.connect():
            return False
        
        motor.enable()
        time.sleep(0.3)
        
        motor.zero_position()
        time.sleep(0.3)
        
        # Movement sequence
        print("\n" + "="*60)
        print("MOVEMENT SEQUENCE")
        print("="*60)
        
        # Move 1: +1.0 radian (about 57 degrees)
        print("\n[1/4] Moving to +1.0 rad (57 degrees)...")
        for i in range(60):
            motor.send_command(
                position=1.0,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("[OK] Position reached")
        time.sleep(0.5)
        
        # Move 2: -1.0 radian
        print("\n[2/4] Moving to -1.0 rad (-57 degrees)...")
        for i in range(60):
            motor.send_command(
                position=-1.0,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("[OK] Position reached")
        time.sleep(0.5)
        
        # Move 3: Smooth sine wave (5 seconds)
        print("\n[3/4] Smooth sine wave motion (5 seconds)...")
        start_time = time.time()
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 1.2 * math.sin(2 * math.pi * 0.5 * t)
            motor.send_command(
                position=position,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("[OK] Sine wave complete")
        time.sleep(0.3)
        
        # Move 4: Return to zero
        print("\n[4/4] Returning to zero...")
        for i in range(60):
            motor.send_command(
                position=0.0,
                velocity=0.0,
                kp=30.0,
                kd=1.5,
                torque=0.0
            )
            time.sleep(0.02)
        print("[OK] Back to zero")
        
        motor.disable()
        
        print("\n" + "="*60)
        print("[SUCCESS] MOVEMENT SEQUENCE COMPLETE!")
        print("="*60)
        return True
        
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        print("Safely stopping motor...")
        motor.disable()
        return False
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False
    finally:
        motor.disconnect()


if __name__ == "__main__":
    print("Robstride 03 Motor Control - COM3\n")
    
    success = move_motor()
    
    if not success:
        print("\n\nTroubleshooting:")
        print("  1. Is motor powered? (red LED should be solid)")
        print("  2. Is PC Robstride software closed?")
        print("  3. Try unplugging/replugging USB-CAN adapter")
        print("  4. Power cycle the motor (off 10 sec, then on)")
        print("  5. Check CANH/CANL wiring")
        print("  6. Verify COM3 is correct port (check Device Manager)")

