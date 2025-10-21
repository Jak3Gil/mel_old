#!/usr/bin/env python3
"""
Move Robstride Motor - EXACT SEQUENCE
Replicating the exact timing and commands from working capture
"""

import serial
import time

def move_motor_exact():
    """Execute exact sequence from capture."""
    print("=" * 70)
    print("   ROBSTRIDE MOTOR - EXACT SEQUENCE REPLICATION")
    print("   Following exact timing from working capture")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    
    try:
        # Connect
        print("[1] Connecting to COM3...")
        ser = serial.Serial(PORT, 115200, timeout=2.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("    [OK] Connected\n")
        
        # Step 1: AT+AT handshake (line 1248)
        print("[2] Sending AT+AT handshake...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.1)
        
        # Read any response
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
        
        # Step 2: Scan sequence (lines 1249-1375)
        print("\n[3] Scanning for motor (sending scan commands)...")
        
        # We only need to send the one that finds motor 13 (line 1314)
        scan_cmd = bytes([
            0x41, 0x54,        # AT
            0x00, 0x07,        # Scan type
            0xe8, 0x6c,        # For motor 13
            0x01, 0x00,        # Fixed
            0x0d, 0x0a         # \r\n
        ])
        
        print(f"    Sending scan: {scan_cmd.hex()}")
        ser.write(scan_cmd)
        time.sleep(1.0)  # Wait longer for motor response
        
        # Check for motor response - THIS IS CRITICAL!
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    *** MOTOR RESPONDED: {resp.hex()} ***")
            print(f"    Motor detected and activated!")
        else:
            print(f"    WARNING: No motor response detected")
            print(f"    This might be why movement doesn't work")
        
        # Step 3: Wait like Robstride does (2-3 seconds between scan and movement)
        print("\n[4] Waiting 3 seconds (like Robstride software)...")
        time.sleep(3.0)
        
        # Step 4: Send EXACT movement commands (lines 1376-1383)
        print("\n[5] Sending movement commands...")
        print("=" * 70 + "\n")
        
        # Test 1: Forward jog (line 1376-1377)
        print("[Test 1] JOG FORWARD (speed=1)...")
        forward_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        print(f"    Command: {forward_cmd.hex()}")
        ser.write(forward_cmd)
        print("    >>> MOTOR SHOULD BE MOVING FORWARD! <<<")
        time.sleep(2)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd)
        print("    [STOP]")
        time.sleep(1)
        
        # Test 2: Reverse jog (line 1378-1379)
        print("\n[Test 2] JOG REVERSE (speed=-1)...")
        reverse_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, 0xe8, 0x6c, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e,
            0x0d, 0x0a
        ])
        print(f"    Command: {reverse_cmd.hex()}")
        ser.write(reverse_cmd)
        print("    >>> MOTOR SHOULD BE MOVING BACKWARD! <<<")
        time.sleep(2)
        
        # Stop
        ser.write(stop_cmd)
        print("    [STOP]")
        time.sleep(1)
        
        # Test 3: Forward again (line 1382-1383)
        print("\n[Test 3] JOG FORWARD again...")
        ser.write(forward_cmd)
        print("    >>> MOVING FORWARD! <<<")
        time.sleep(2)
        
        # Stop
        ser.write(stop_cmd)
        print("    [STOP]\n")
        
        ser.close()
        
        print("=" * 70)
        print("[COMPLETE] EXACT SEQUENCE FINISHED")
        print("=" * 70)
        
        print("\n🎯 DID THE MOTOR MOVE?")
        print("\n  [YES] - Success! The exact sequence works!")
        print("  [NO]  - Motor not responding. Possible reasons:")
        print("         1. Motor didn't respond to scan (no activation)")
        print("         2. Missing driver initialization")
        print("         3. Motor needs power cycle")
        
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
    print("\nRobstride Motor - Exact Sequence from Working Capture\n")
    print("This replicates EXACTLY what Robstride software does:\n")
    print("  1. AT+AT handshake")
    print("  2. Scan for motor")
    print("  3. Wait 3 seconds")
    print("  4. Send JOG commands\n")
    time.sleep(1)
    
    success = move_motor_exact()
    
    if not success:
        print("\n" + "=" * 70)
        print("CRITICAL: NEED MOTOR RESPONSES")
        print("=" * 70)
        print("\nYour serial monitor shows:")
        print("  -> COM3: ✅ (commands TO motor)")
        print("  <- COM3: ❌ (responses FROM motor) - MISSING!")
        print("\nPlease configure your monitor to show BOTH directions!")
        print("The motor MUST respond to the scan command to activate.")
        print("\nWithout seeing the motor's responses, I cannot")
        print("determine why it's not accepting movement commands.")

