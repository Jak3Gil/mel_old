#!/usr/bin/env python3
"""
Robstride Motor Implementation - K-Scale Style
Implements MotorInterface for Robstride motors
"""

import serial
import time
import glob
from typing import Dict, Optional
from motor_interface import MotorInterface


class RobstrideMotor(MotorInterface):
    """Robstride motor implementation following K-Scale architecture."""
    
    def __init__(self, config: Dict):
        """Initialize Robstride motor with configuration."""
        super().__init__(config)
        
        # Get limits from config
        limits = self.get_limits()
        self.P_MIN = limits.get('position_min', -12.5)
        self.P_MAX = limits.get('position_max', 12.5)
        self.V_MIN = limits.get('velocity_min', -45.0)
        self.V_MAX = limits.get('velocity_max', 45.0)
        self.T_MIN = limits.get('torque_min', -18.0)
        self.T_MAX = limits.get('torque_max', 18.0)
        self.KP_MIN = limits.get('kp_min', 0.0)
        self.KP_MAX = limits.get('kp_max', 500.0)
        self.KD_MIN = limits.get('kd_min', 0.0)
        self.KD_MAX = limits.get('kd_max', 5.0)
        
        # Get control parameters
        control = self.get_control_params()
        self.default_kp = control.get('default_kp', 30.0)
        self.default_kd = control.get('default_kd', 1.5)
        
        # Communication settings
        comm = config.get('communication', {})
        self.port = comm.get('port')
        self.baudrate = comm.get('baudrate', 115200)
        self.timeout = comm.get('timeout', 0.5)
        self.protocol = comm.get('protocol', 'L91')
        
        self.ser = None
    
    def _find_usb_port(self) -> Optional[str]:
        """Auto-detect USB-CAN adapter port."""
        ports = glob.glob('/dev/cu.usbserial*')
        if ports:
            return ports[0]
        ports = glob.glob('/dev/ttyUSB*')
        if ports:
            return ports[0]
        return None
    
    def connect(self) -> bool:
        """Connect and initialize with L91 protocol."""
        if not self.port:
            self.port = self._find_usb_port()
            if not self.port:
                print(f"✗ Motor {self.can_id}: No USB-CAN adapter found")
                return False
        
        print(f"Motor {self.can_id}: Connecting to {self.port}...")
        
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
            time.sleep(0.3)
            
            # Initialize with L91 sequence
            print(f"  Initializing with {self.protocol}...")
            self.ser.write(b'C\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.ser.write(b'L91\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.ser.write(b'O\r')
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.is_connected = True
            print(f"✓ Motor {self.can_id}: Connected")
            return True
            
        except Exception as e:
            print(f"✗ Motor {self.can_id}: Connection failed - {e}")
            return False
    
    def disconnect(self) -> None:
        """Close connection."""
        if self.ser:
            try:
                self.ser.write(b'C\r')
                self.ser.close()
                self.is_connected = False
                print(f"✓ Motor {self.can_id}: Disconnected")
            except:
                pass
    
    def enable(self) -> bool:
        """Enable motor."""
        if not self.is_connected:
            print(f"✗ Motor {self.can_id}: Not connected")
            return False
        
        try:
            print(f"Motor {self.can_id}: Enabling...")
            msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFC\r"
            self.ser.write(msg.encode())
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.is_enabled = True
            print(f"✓ Motor {self.can_id}: Enabled")
            return True
        except Exception as e:
            print(f"✗ Motor {self.can_id}: Enable failed - {e}")
            return False
    
    def disable(self) -> bool:
        """Disable motor."""
        if not self.is_connected:
            return False
        
        try:
            print(f"Motor {self.can_id}: Disabling...")
            msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFD\r"
            self.ser.write(msg.encode())
            time.sleep(0.1)
            self.ser.read(1000)
            
            self.is_enabled = False
            print(f"✓ Motor {self.can_id}: Disabled")
            return True
        except Exception as e:
            print(f"✗ Motor {self.can_id}: Disable failed - {e}")
            return False
    
    def _float_to_uint(self, x: float, x_min: float, x_max: float, bits: int) -> int:
        """Convert float to unsigned int."""
        span = x_max - x_min
        x = max(min(x, x_max), x_min)
        return int((x - x_min) * ((1 << bits) - 1) / span)
    
    def set_position(self, position: float, velocity: float = 0.0,
                    kp: Optional[float] = None, kd: Optional[float] = None,
                    torque: float = 0.0) -> bool:
        """
        Set target position using MIT protocol.
        
        Args:
            position: Target position (radians)
            velocity: Target velocity (rad/s)
            kp: Position gain (uses default if None)
            kd: Derivative gain (uses default if None)
            torque: Feedforward torque (Nm)
        """
        if not self.is_connected or not self.is_enabled:
            return False
        
        # Use defaults if not specified
        if kp is None:
            kp = self.default_kp
        if kd is None:
            kd = self.default_kd
        
        try:
            # Pack values into unsigned ints
            p_int = self._float_to_uint(position, self.P_MIN, self.P_MAX, 16)
            v_int = self._float_to_uint(velocity, self.V_MIN, self.V_MAX, 12)
            kp_int = self._float_to_uint(kp, self.KP_MIN, self.KP_MAX, 12)
            kd_int = self._float_to_uint(kd, self.KD_MIN, self.KD_MAX, 12)
            t_int = self._float_to_uint(torque, self.T_MIN, self.T_MAX, 12)
            
            # Pack into 8 bytes
            data = bytearray(8)
            data[0] = (p_int >> 8) & 0xFF
            data[1] = p_int & 0xFF
            data[2] = (v_int >> 4) & 0xFF
            data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
            data[4] = kp_int & 0xFF
            data[5] = (kd_int >> 4) & 0xFF
            data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
            data[7] = t_int & 0xFF
            
            # Send SLCAN message
            msg = f"t{self.can_id:03X}8{data.hex().upper()}\r"
            self.ser.write(msg.encode())
            
            # Update internal state
            self._current_position = position
            
            # Optional: read response
            if self.ser.in_waiting > 0:
                self.ser.read(self.ser.in_waiting)
            
            return True
        except Exception as e:
            print(f"✗ Motor {self.can_id}: Position command failed - {e}")
            return False
    
    def set_zero_position(self) -> bool:
        """Set current position as zero."""
        if not self.is_connected:
            return False
        
        try:
            print(f"Motor {self.can_id}: Setting zero position...")
            msg = f"t{self.can_id:03X}8FFFFFFFFFFFFFFFE\r"
            self.ser.write(msg.encode())
            time.sleep(0.1)
            self.ser.read(1000)
            
            self._current_position = 0.0
            print(f"✓ Motor {self.can_id}: Position zeroed")
            return True
        except Exception as e:
            print(f"✗ Motor {self.can_id}: Zero failed - {e}")
            return False
    
    def get_position(self) -> float:
        """Get current position (from internal state)."""
        return self._current_position
    
    def get_velocity(self) -> float:
        """Get current velocity (from internal state)."""
        return self._current_velocity
    
    def calibrate(self) -> bool:
        """
        Calibrate motor (for Robstride, this is setting zero position).
        """
        return self.set_zero_position()
    
    def set_current(self, current: float) -> bool:
        """Set motor current (torque mode)."""
        # For Robstride, use position command with high kd and torque feedforward
        return self.set_position(
            position=self.get_position(),
            velocity=0.0,
            kp=0.0,
            kd=0.0,
            torque=current
        )

