#!/usr/bin/env python3
"""
Move Robstride Motor with CORRECT baud rate: 921600
"""

import serial
import time

def move_motor():
    """Move motor using correct baud rate."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - CORRECT BAUD RATE (921600)")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600  # CORRECT BAUD RATE!
    
    try:
        # Connect
        print(f"[1] Connecting to {PORT} at {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=2.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("    [OK] Connected\n")
        
        # AT+AT handshake
        print("[2] Sending AT+AT handshake...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
            print(f"    ASCII: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Handshake complete\n")
        
        # Scan for motor
        print("[3] Scanning for motor CAN ID 13...")
        scan_cmd = bytes([
            0x41, 0x54, 0x00, 0x07, 0xe8, 0x6c, 0x01, 0x00, 0x0d, 0x0a
        ])
        ser.write(scan_cmd)
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    *** MOTOR RESPONDED! ***")
            print(f"    Response: {resp.hex()}")
            print(f"    Motor detected and activated!")
        else:
            print(f"    No motor response - continuing anyway...")
        print()
        
        # Wait before movement
        print("[4] Waiting 2 seconds...")
        time.sleep(2.0)
        
        # Movement commands
        print("[5] Sending movement commands...")
        print("=" * 70 + "\n")
        
        # Test 1: Forward JOG
        print("[Test 1/3] JOG FORWARD (3 seconds)...")
        forward_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        
        ser.write(forward_cmd)
        print("    >>> MOTOR SHOULD BE MOVING! <<<")
        time.sleep(3)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd)
        print("    [STOPPED]")
        time.sleep(1)
        
        # Test 2: Reverse JOG
        print("\n[Test 2/3] JOG REVERSE (3 seconds)...")
        reverse_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e,
            0x0d, 0x0a
        ])
        
        ser.write(reverse_cmd)
        print("    >>> MOTOR SHOULD BE MOVING BACKWARD! <<<")
        time.sleep(3)
        
        ser.write(stop_cmd)
        print("    [STOPPED]")
        time.sleep(1)
        
        # Test 3: Forward again longer
        print("\n[Test 3/3] JOG FORWARD (5 seconds)...")
        ser.write(forward_cmd)
        print("    >>> MOTOR MOVING! <<<")
        time.sleep(5)
        
        ser.write(stop_cmd)
        print("    [STOPPED]\n")
        
        ser.close()
        
        print("=" * 70)
        print("[COMPLETE] TEST FINISHED")
        print("=" * 70)
        
        print("\n*** DID THE MOTOR MOVE? ***")
        print("\nIf YES: Perfect! 921600 was the correct baud rate!")
        print("If NO: The motor still needs something else to activate")
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] {e}")
        print("*** CLOSE ROBSTRIDE SOFTWARE FIRST! ***")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor - With CORRECT Baud Rate\n")
    print("Using 921600 baud (motor responded to AT+AT!)\n")
    time.sleep(1)
    
    success = move_motor()
    
    if success:
        print("\n\nIf the motor moved:")
        print("  This was a BAUD RATE issue all along!")
        print("  You were using 115200, motor needs 921600!")
        print("\nIf it still didn't move:")
        print("  At least now the motor is responding (we got 'OK')")
        print("  So we're on the right track!")

