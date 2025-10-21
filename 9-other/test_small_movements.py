#!/usr/bin/env python3
"""
Test motors with VERY small movements to verify they're working.
"""

import serial
import time

def test_small_movements():
    """Test motors with tiny, visible movements."""
    PORT = '/dev/tty.usbserial-110'
    BAUD = 921600
    
    print("=" * 70)
    print("   TESTING SMALL MOTOR MOVEMENTS")
    print("=" * 70 + "\n")
    
    try:
        # Connect
        print(f"[1] Connecting to {PORT} at {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=2.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("    ✓ Connected\n")
        
        # AT+AT handshake
        print("[2] Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        
        # Scan for motor 13
        print("    Scanning Motor 13...")
        scan_cmd_13 = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a])
        ser.write(scan_cmd_13)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    ✓ Motor 13 responded: {len(resp)} bytes")
        else:
            print(f"    ⚠ Motor 13 no response")
        
        # Scan for motor 14
        print("    Scanning Motor 14...")
        scan_cmd_14 = bytes([0x41, 0x54, 0x00, 0x07, 0xe8, 0x70, 0x01, 0x00, 0x0d, 0x0a])
        ser.write(scan_cmd_14)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    ✓ Motor 14 responded: {len(resp)} bytes")
        else:
            print(f"    ⚠ Motor 14 no response")
        
        print("\n" + "=" * 70)
        print("MOVEMENT TESTS - WATCH CLOSELY!")
        print("=" * 70 + "\n")
        
        time.sleep(1)
        
        # Test Motor 13 - Very short pulse
        print("[Test 1] Motor 13 - SHORT PULSE (0.3 seconds)...")
        print(">>> WATCH MOTOR 13 NOW! <<<")
        jog_cmd_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        ser.write(jog_cmd_13)
        print("Command sent...")
        time.sleep(0.3)
        
        # Stop motor 13
        stop_cmd_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd_13)
        print("Stop sent\n")
        time.sleep(2)
        
        # Test Motor 14 - Very short pulse
        print("[Test 2] Motor 14 - SHORT PULSE (0.3 seconds)...")
        print(">>> WATCH MOTOR 14 NOW! <<<")
        jog_cmd_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        ser.write(jog_cmd_14)
        print("Command sent...")
        time.sleep(0.3)
        
        # Stop motor 14
        stop_cmd_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd_14)
        print("Stop sent\n")
        time.sleep(2)
        
        # Test both motors - Very short pulse
        print("[Test 3] BOTH MOTORS - SHORT PULSE (0.3 seconds)...")
        print(">>> WATCH BOTH MOTORS NOW! <<<")
        ser.write(jog_cmd_13)
        time.sleep(0.01)
        ser.write(jog_cmd_14)
        print("Commands sent to both motors...")
        time.sleep(0.3)
        
        ser.write(stop_cmd_13)
        ser.write(stop_cmd_14)
        print("Stop sent to both\n")
        time.sleep(2)
        
        # Very tiny movements - 0.1 second pulses
        print("[Test 4] Motor 13 - TINY PULSE (0.1 seconds)...")
        print(">>> WATCH FOR TINY MOVEMENT! <<<")
        ser.write(jog_cmd_13)
        time.sleep(0.1)
        ser.write(stop_cmd_13)
        print("Done\n")
        time.sleep(2)
        
        print("[Test 5] Motor 14 - TINY PULSE (0.1 seconds)...")
        print(">>> WATCH FOR TINY MOVEMENT! <<<")
        ser.write(jog_cmd_14)
        time.sleep(0.1)
        ser.write(stop_cmd_14)
        print("Done\n")
        time.sleep(2)
        
        # Reverse direction test
        print("[Test 6] Motor 13 - REVERSE (0.3 seconds)...")
        print(">>> WATCH FOR REVERSE MOVEMENT! <<<")
        reverse_cmd_13 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e,
            0x0d, 0x0a
        ])
        ser.write(reverse_cmd_13)
        time.sleep(0.3)
        ser.write(stop_cmd_13)
        print("Done\n")
        time.sleep(2)
        
        print("[Test 7] Motor 14 - REVERSE (0.3 seconds)...")
        print(">>> WATCH FOR REVERSE MOVEMENT! <<<")
        reverse_cmd_14 = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x70, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e,
            0x0d, 0x0a
        ])
        ser.write(reverse_cmd_14)
        time.sleep(0.3)
        ser.write(stop_cmd_14)
        print("Done\n")
        
        ser.close()
        
        print("=" * 70)
        print("TEST COMPLETE")
        print("=" * 70)
        print("\nDid you see ANY movement?")
        print("  - If YES: Great! The motors are working")
        print("  - If NO: Motors may need different commands or activation")
        print("\nThings to check:")
        print("  1. Are the motors powered? (LEDs on?)")
        print("  2. Can you hear any sound from the motors?")
        print("  3. Does Robstride software move them?")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("\nSmall Movement Test for Motors 13 & 14\n")
    print("Starting test in 2 seconds... Watch the motors!\n")
    time.sleep(2)
    test_small_movements()

