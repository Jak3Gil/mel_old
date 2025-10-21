#!/usr/bin/env python3
"""
Very verbose motor test - watch closely for any movement!
"""

import serial
import time

PORT = '/dev/cu.usbserial-110'
BAUD = 921600

print("=" * 70)
print("   VERBOSE MOTOR TEST")
print("=" * 70)
print("\nWATCH THE MOTORS CLOSELY!\n")

try:
    ser = serial.Serial(PORT, BAUD, timeout=2.0)
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print("✓ Connected to", PORT, "at", BAUD, "baud\n")
    
    # Test Motor 13 - LONG movement so you can see it
    print("=" * 70)
    print("TEST 1: Motor 13 - 5 SECOND MOVEMENT")
    print(">>> WATCH MOTOR 13 NOW! <<<")
    print("=" * 70)
    time.sleep(2)
    
    jog_cmd = bytes([
        0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
        0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
        0x0d, 0x0a
    ])
    
    print("Sending command...")
    ser.write(jog_cmd)
    
    for i in range(5):
        print(f"  {i+1}... (watching)", end="\r")
        time.sleep(1)
    print("\n")
    
    stop_cmd = bytes([
        0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
        0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
        0x0d, 0x0a
    ])
    ser.write(stop_cmd)
    print("Stop command sent\n")
    time.sleep(2)
    
    # Test Motor 14 - LONG movement
    print("=" * 70)
    print("TEST 2: Motor 14 - 5 SECOND MOVEMENT")
    print(">>> WATCH MOTOR 14 NOW! <<<")
    print("=" * 70)
    time.sleep(2)
    
    jog_cmd_14 = bytes([
        0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
        0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
        0x0d, 0x0a
    ])
    
    print("Sending command...")
    ser.write(jog_cmd_14)
    
    for i in range(5):
        print(f"  {i+1}... (watching)", end="\r")
        time.sleep(1)
    print("\n")
    
    stop_cmd_14 = bytes([
        0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
        0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
        0x0d, 0x0a
    ])
    ser.write(stop_cmd_14)
    print("Stop command sent\n")
    
    ser.close()
    
    print("=" * 70)
    print("QUESTION: Did EITHER motor move?")
    print("=" * 70)
    print("\nIf YES: The system is working!")
    print("If NO: Motors might need activation in Servo Studio first")
    
except Exception as e:
    print(f"Error: {e}")

