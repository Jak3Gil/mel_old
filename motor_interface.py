#!/usr/bin/env python3
"""
Motor Interface - K-Scale Labs Style
Generic interface for motor control
"""

from abc import ABC, abstractmethod
from typing import Dict, Optional, Tuple


class MotorInterface(ABC):
    """
    Abstract base class for motor control.
    Based on K-Scale Labs architecture.
    """
    
    def __init__(self, config: Dict):
        """Initialize motor with configuration dictionary."""
        self.config = config
        self.can_id = config.get('can_id', 0x01)
        self.is_connected = False
        self.is_enabled = False
        self._current_position = 0.0
        self._current_velocity = 0.0
    
    @abstractmethod
    def connect(self) -> bool:
        """
        Connect to the motor.
        Returns: True if successful, False otherwise
        """
        pass
    
    @abstractmethod
    def disconnect(self) -> None:
        """Disconnect from the motor."""
        pass
    
    @abstractmethod
    def enable(self) -> bool:
        """
        Enable the motor for operation.
        Returns: True if successful, False otherwise
        """
        pass
    
    @abstractmethod
    def disable(self) -> bool:
        """
        Disable the motor.
        Returns: True if successful, False otherwise
        """
        pass
    
    @abstractmethod
    def set_position(self, position: float, velocity: float = 0.0, 
                    kp: Optional[float] = None, kd: Optional[float] = None,
                    torque: float = 0.0) -> bool:
        """
        Set target position with optional velocity, gains, and feedforward torque.
        
        Args:
            position: Target position (radians)
            velocity: Target velocity (rad/s)
            kp: Position gain (optional, uses default if None)
            kd: Derivative gain (optional, uses default if None)
            torque: Feedforward torque (Nm)
        
        Returns: True if command sent successfully
        """
        pass
    
    @abstractmethod
    def set_zero_position(self) -> bool:
        """
        Set current position as zero.
        Returns: True if successful
        """
        pass
    
    @abstractmethod
    def get_position(self) -> float:
        """
        Get current position.
        Returns: Position in radians
        """
        pass
    
    @abstractmethod
    def get_velocity(self) -> float:
        """
        Get current velocity.
        Returns: Velocity in rad/s
        """
        pass
    
    @abstractmethod
    def calibrate(self) -> bool:
        """
        Calibrate the motor (implementation specific).
        Returns: True if successful
        """
        pass
    
    def get_state(self) -> Dict:
        """
        Get current motor state.
        Returns: Dictionary with motor state information
        """
        return {
            'can_id': self.can_id,
            'connected': self.is_connected,
            'enabled': self.is_enabled,
            'position': self._current_position,
            'velocity': self._current_velocity
        }
    
    def get_limits(self) -> Dict:
        """Get motor limits from configuration."""
        return self.config.get('limits', {})
    
    def get_control_params(self) -> Dict:
        """Get default control parameters."""
        return self.config.get('control', {})

