#!/usr/bin/env python3
"""
Serial Port Traffic Sniffer for COM3
Shows exact hex bytes being sent/received

IMPORTANT: This can't run at same time as Robstride software
Instead, we need a different approach - checking existing logs
or using a serial port monitor tool
"""

import serial
import time
from datetime import datetime

def monitor_serial_raw():
    """Monitor raw serial traffic."""
    print("=" * 70)
    print("   SERIAL PORT RAW MONITOR - COM3")
    print("=" * 70)
    print()
    print("This will show ALL bytes sent/received on COM3")
    print()
    print("NOTE: Cannot run simultaneously with Robstride software!")
    print()
    print("INSTRUCTIONS FOR CAPTURING ROBSTRIDE TRAFFIC:")
    print("  1. Download 'Free Serial Port Monitor' or similar tool")
    print("  2. Set it to monitor COM3")
    print("  3. Start monitoring")
    print("  4. Open Robstride software and move motor")
    print("  5. Copy the hex output")
    print("  6. Send me the captured bytes")
    print()
    print("OR we can test if motor responds to our AT commands:")
    print("=" * 70)
    print()
    
    response = input("Do you want to test sending AT commands? (y/n): ")
    
    if response.lower() != 'y':
        print("\nOk! To capture Robstride traffic:")
        print("  1. Get a serial port monitor tool")
        print("  2. Monitor COM3 while using Robstride software")
        print("  3. Share the captured hex bytes")
        return
    
    print("\n[1] Opening COM3...")
    
    try:
        ser = serial.Serial('COM3', 115200, timeout=0.5)
        time.sleep(0.3)
        print("    [OK] Connected\n")
        
        # Clear buffers
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        print("[2] Testing various AT command formats...\n")
        
        # Test different AT command possibilities
        test_commands = [
            # Standard AT commands
            (b'AT\r\n', "Standard AT"),
            (b'AT\r', "AT with CR only"),
            (b'ATZ\r\n', "Reset"),
            (b'ATE0\r\n', "Echo off"),
            
            # Motor enable variations (ID 13 = 0x0D)
            (b'AT\x00\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFC\r\n', "Enable (format 1)"),
            (b'AT\x0D\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFC\r\n', "Enable with ID (format 2)"),
            
            # Position command (1.0 rad, Kp=50, Kd=2)
            (b'AT\x00\x08\x8A\x3C\x00\x06\x66\x03\x33\x00\r\n', "Position 1.0 rad (format 1)"),
            (b'AT\x0D\x08\x8A\x3C\x00\x06\x66\x03\x33\x00\r\n', "Position 1.0 rad with ID (format 2)"),
        ]
        
        for i, (cmd, desc) in enumerate(test_commands, 1):
            print(f"[Test {i}/{len(test_commands)}] {desc}")
            print(f"    Hex: {cmd.hex()}")
            print(f"    Sending...")
            
            ser.write(cmd)
            time.sleep(0.2)
            
            # Check for response
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                print(f"    Response: {resp.hex()}")
                try:
                    ascii_resp = resp.decode('ascii', errors='ignore')
                    if ascii_resp.strip():
                        print(f"    ASCII: {ascii_resp}")
                except:
                    pass
            else:
                print("    No response")
            print()
            
            time.sleep(0.3)
        
        ser.close()
        
        print("=" * 70)
        print("TEST COMPLETE")
        print("=" * 70)
        print("\nDid you see the motor move during any of these tests?")
        print("If YES, tell me which test number!")
        print("If NO, we need to capture what Robstride software actually sends.")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("\nSerial Traffic Monitor\n")
    monitor_serial_raw()

