#!/usr/bin/env python3
"""
Enable MIT Control Mode on Motors
Required before MIT position control will work
Based on captured commands from Robstride software
"""

import serial
import time

def enable_mit_mode():
    """Enable MIT control mode on both motors."""
    print("=" * 70)
    print("   ENABLE MIT CONTROL MODE")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    motor13_encoding = bytes([0xe8, 0x6c])
    motor14_encoding = bytes([0xe8, 0x74])
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=2.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("[1] Connected\n")
        
        # Initialize
        print("[2] Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        # Activate motors
        ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        ser.write(bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x74, 0x01, 0x00, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK]\n")
        
        time.sleep(1)
        
        # CRITICAL: Set motors to MIT control mode
        print("[3] Setting Motor 13 to MIT control mode...")
        
        # From capture line 2310: SET_RUN_MODE
        # AT \x90\x07 e8 6c 08 [05 70 00 00 00 00 00 00]
        set_run_mode = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0d, 0x0a
        ])
        print(f"    Sending SET_RUN_MODE: {set_run_mode.hex()}")
        ser.write(set_run_mode)
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
        
        # From capture line 2311: SET_MODE
        # AT \x18\x07 e8 6c 08 [00 00 00 00 00 00 00 00]
        set_mode = bytes([
            0x41, 0x54, 0x18, 0x07, 0xe8, 0x6c, 0x08,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0d, 0x0a
        ])
        print(f"    Sending SET_MODE: {set_mode.hex()}")
        ser.write(set_mode)
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
        
        print("    [OK] Motor 13 set to MIT mode\n")
        
        # Same for Motor 14
        print("[4] Setting Motor 14 to MIT control mode...")
        
        set_run_mode_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x74, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0d, 0x0a
        ])
        ser.write(set_run_mode_14)
        time.sleep(0.3)
        ser.read(1000)
        
        set_mode_14 = bytes([
            0x41, 0x54, 0x18, 0x07, 0xe8, 0x74, 0x08,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0d, 0x0a
        ])
        ser.write(set_mode_14)
        time.sleep(0.3)
        ser.read(1000)
        
        print("    [OK] Motor 14 set to MIT mode\n")
        
        ser.close()
        
        print("=" * 70)
        print("[SUCCESS] Both motors now in MIT control mode!")
        print("=" * 70)
        print("\nYou can now use MIT position control commands.")
        print("This setting persists until motors are power cycled.")
        
        return True
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nEnable MIT Control Mode\n")
    print("This sets motors to accept MIT position/velocity commands")
    print("instead of just JOG commands.\n")
    
    enable_mit_mode()

