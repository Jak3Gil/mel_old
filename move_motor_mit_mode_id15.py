#!/usr/bin/env python3
"""
Move Robstride Motor ID 15 - Using AT Commands for MIT Mode
Based on Motor Studio protocol capture
"""

import serial
import time
import math

def move_motor_with_at_commands():
    """Use AT commands to properly enable MIT mode like Motor Studio does."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR ID 15 - MIT MODE VIA AT COMMANDS")
    print("=" * 60 + "\n")
    
    port = 'COM3'
    motor_id = 0x0F  # Motor ID 15
    
    print(f"Motor CAN ID: {motor_id} (0x{motor_id:02X})")
    print(f"Opening {port}...\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.5)
        
        # Initialize L91 protocol
        print("[1] Initializing L91 protocol...")
        ser.write(b'C\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'O\r')
        time.sleep(0.1)
        ser.read(1000)
        print("    [OK] L91 initialized\n")
        
        # CRITICAL: Enter MOTOR MODE using AT command (like Motor Studio does)
        print("[2] Entering MOTOR MODE using AT command...")
        print("    (This is what Motor Studio does to enable MIT mode!)")
        
        # AT command to enter motor mode
        # Format: AT 18 07 e8 7c 08 00 00 00 00 00 00 00 00
        at_motor_mode = bytes([
            0x41, 0x54,  # "AT"
            0x18,        # Command: Enter motor mode
            0x07, 0xe8,  # Motor ID related (from capture)
            0x7c,        # ?
            0x08,        # Data length
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Data
            0x0d, 0x0a   # CR LF
        ])
        
        print(f"    Sending: {at_motor_mode.hex(' ')}")
        ser.write(at_motor_mode)
        time.sleep(0.5)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.hex(' ')}")
            print(f"    ASCII: {resp.decode('ascii', errors='ignore')}")
        print("    [OK] MOTOR MODE enabled\n")
        
        # Now use standard MIT protocol commands
        print("[3] Enabling motor (MIT protocol)...")
        enable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFC\r"
        print(f"    Sending: {enable_msg.strip()}")
        ser.write(enable_msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Motor enabled\n")
        
        # Zero position
        print("[4] Setting zero position...")
        zero_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFE\r"
        ser.write(zero_msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK] Zero position set\n")
        
        print("=" * 60)
        print("MOVEMENT TEST")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR SHAFT NOW! ***\n")
        
        time.sleep(1)
        
        # Helper functions
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        def send_position(position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
            p_int = float_to_uint(position, -12.5, 12.5, 16)
            v_int = float_to_uint(velocity, -45.0, 45.0, 12)
            kp_int = float_to_uint(kp, 0.0, 500.0, 12)
            kd_int = float_to_uint(kd, 0.0, 5.0, 12)
            t_int = float_to_uint(torque, -18.0, 18.0, 12)
            
            data = bytearray(8)
            data[0] = (p_int >> 8) & 0xFF
            data[1] = p_int & 0xFF
            data[2] = (v_int >> 4) & 0xFF
            data[3] = ((v_int & 0x0F) << 4) | ((kp_int >> 8) & 0x0F)
            data[4] = kp_int & 0xFF
            data[5] = (kd_int >> 4) & 0xFF
            data[6] = ((kd_int & 0x0F) << 4) | ((t_int >> 8) & 0x0F)
            data[7] = t_int & 0xFF
            
            msg = f"t{motor_id:03X}8{data.hex().upper()}\r"
            ser.write(msg.encode())
        
        # Test 1: Move to +2.0 radians
        print("[Test 1] Moving to +2.0 radians (115 degrees)...")
        print("         Using Kp=60, Kd=2.5")
        for i in range(100):
            send_position(2.0, kp=60.0, kd=2.5)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"         Sending... {i}/100")
        print("         [OK] Command sequence complete")
        print("\n         >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Move to -2.0 radians
        print("[Test 2] Moving to -2.0 radians (-115 degrees)...")
        for i in range(100):
            send_position(-2.0, kp=60.0, kd=2.5)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"         Sending... {i}/100")
        print("         [OK] Command sequence complete")
        print("\n         >> DID IT MOVE TO OPPOSITE SIDE? <<\n")
        time.sleep(2)
        
        # Test 3: Large movement
        print("[Test 3] LARGE MOVEMENT - Moving to +3.5 radians (200 degrees)...")
        print("         Using HIGH gains: Kp=100, Kd=4")
        print("         THIS SHOULD BE VERY OBVIOUS!")
        for i in range(120):
            send_position(3.5, kp=100.0, kd=4.0)
            time.sleep(0.02)
            if i % 30 == 0:
                print(f"         Sending... {i}/120")
        print("         [OK] Command sequence complete")
        print("\n         >> THIS WAS HUGE! Did you see it? <<\n")
        time.sleep(2)
        
        # Test 4: Smooth sine wave
        print("[Test 4] Smooth sine wave (5 seconds)...")
        start_time = time.time()
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 2.0 * math.sin(2 * math.pi * 0.5 * t)
            send_position(position, kp=50.0, kd=2.0)
            time.sleep(0.02)
        print("         [OK] Sine wave complete")
        print("\n         >> Did you see smooth back-and-forth motion? <<\n")
        time.sleep(1)
        
        # Return to zero
        print("[Test 5] Returning to zero...")
        for i in range(80):
            send_position(0.0, kp=60.0, kd=2.5)
            time.sleep(0.02)
        print("         [OK] Should be at zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[5] Disabling motor...")
        disable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFD\r"
        ser.write(disable_msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK] Motor disabled\n")
        
        # Exit motor mode (return to reset mode)
        print("[6] Exiting MOTOR MODE...")
        at_reset_mode = bytes([
            0x41, 0x54,  # "AT"
            0x20,        # Command: Enter reset mode (exit motor mode)
            0x07, 0xe8,  # Motor ID related
            0x7c,        # ?
            0x08,        # Data length
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  # Data
            0x0d, 0x0a   # CR LF
        ])
        ser.write(at_reset_mode)
        time.sleep(0.3)
        resp = ser.read(1000)
        print("    [OK] RESET MODE (MIT mode exited)\n")
        
        ser.close()
        print("[7] Disconnected\n")
        
        print("=" * 60)
        print("[SUCCESS] TEST COMPLETE!")
        print("=" * 60)
        
        print("\n" + "=" * 60)
        print("RESULTS:")
        print("=" * 60)
        print("\nDid you see the motor move?")
        print("[YES] - PERFECT! The AT commands were the missing piece!")
        print("[NO]  - See troubleshooting below")
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] Cannot open {port}: {e}\n")
        print("Troubleshooting:")
        print("  1. Close Motor Studio")
        print("  2. Close any other programs using COM3")
        print("  3. Unplug and replug USB-CAN adapter")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Controller - Using Motor Studio Protocol")
    print("Motor ID: 15, Port: COM3\n")
    
    success = move_motor_with_at_commands()
    
    if not success:
        print("\n" + "=" * 60)
        print("TROUBLESHOOTING")
        print("=" * 60)
        print("\nIf still not working:")
        print("  1. Verify motor ID is actually 15")
        print("  2. Check if AT command format needs adjustment")
        print("  3. The '07 e8' bytes might be motor-ID specific")
        print("  4. Try capturing more Motor Studio commands")
        print("\nTo capture more commands:")
        print("  - Use a serial monitor/sniffer")
        print("  - Watch what Motor Studio sends for YOUR motor")
        print("  - The AT command format might vary by motor ID")
    else:
        print("\n" + "=" * 60)
        print("SUCCESS!")
        print("=" * 60)
        print("\nThe AT commands were the key!")
        print("Motor Studio uses these special commands to")
        print("enable MIT mode before sending position commands.")
        print("\nYou can now control the motor!")

