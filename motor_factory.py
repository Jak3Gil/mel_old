#!/usr/bin/env python3
"""
Motor Factory - K-Scale Style
Creates motor objects based on configuration
"""

import yaml
from typing import Dict, Optional
from motor_interface import MotorInterface
from robstride_kscale import RobstrideMotor


class MotorFactory:
    """
    Factory for creating motor objects based on configuration.
    Follows K-Scale Labs architecture.
    """
    
    # Motor type registry
    MOTOR_TYPES = {
        'robstride': RobstrideMotor,
        # Add more motor types here:
        # 'bionic': BionicMotor,
        # 'dynamixel': DynamixelMotor,
    }
    
    @staticmethod
    def load_config(config_path: str) -> Dict:
        """Load configuration from YAML file."""
        with open(config_path, 'r') as f:
            return yaml.safe_load(f)
    
    @staticmethod
    def create_motor(motor_name: str, config: Dict) -> Optional[MotorInterface]:
        """
        Create a motor instance based on configuration.
        
        Args:
            motor_name: Name of the motor in config
            config: Full configuration dictionary
        
        Returns:
            Motor instance or None if motor type not found
        """
        motors_config = config.get('motors', {})
        
        if motor_name not in motors_config:
            print(f"✗ Motor '{motor_name}' not found in configuration")
            return None
        
        motor_config = motors_config[motor_name]
        motor_type = motor_config.get('type', '').lower()
        
        if motor_type not in MotorFactory.MOTOR_TYPES:
            print(f"✗ Motor type '{motor_type}' not supported")
            print(f"  Supported types: {list(MotorFactory.MOTOR_TYPES.keys())}")
            return None
        
        # Create motor instance
        motor_class = MotorFactory.MOTOR_TYPES[motor_type]
        return motor_class(motor_config)
    
    @staticmethod
    def create_all_motors(config: Dict) -> Dict[str, MotorInterface]:
        """
        Create all motors defined in configuration.
        
        Args:
            config: Full configuration dictionary
        
        Returns:
            Dictionary mapping motor names to motor instances
        """
        motors = {}
        motors_config = config.get('motors', {})
        
        for motor_name in motors_config:
            motor = MotorFactory.create_motor(motor_name, config)
            if motor:
                motors[motor_name] = motor
        
        return motors
    
    @staticmethod
    def create_from_file(config_path: str, motor_name: Optional[str] = None):
        """
        Load config file and create motor(s).
        
        Args:
            config_path: Path to YAML configuration file
            motor_name: Specific motor name, or None to create all motors
        
        Returns:
            Single motor instance or dictionary of motors
        """
        config = MotorFactory.load_config(config_path)
        
        if motor_name:
            return MotorFactory.create_motor(motor_name, config)
        else:
            return MotorFactory.create_all_motors(config)

