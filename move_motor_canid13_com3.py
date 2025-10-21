#!/usr/bin/env python3
"""
Move Robstride Motor with CAN ID 13 on COM3
Based on detected device: CAN ID: 13
"""

import serial
import time
import math

def move_motor_canid13():
    """Connect to COM3 and move motor CAN ID 13."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - CAN ID 13 - COM3")
    print("=" * 60 + "\n")
    
    port = 'COM3'
    motor_id = 0x0D  # CAN ID 13 (decimal) = 0x0D (hex)
    
    print(f"Motor CAN ID: {motor_id} (0x{motor_id:02X} = {motor_id} decimal)")
    print(f"Opening {port}...\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        # Initialize L91
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
        
        # Enable motor
        print("[2] Enabling motor...")
        enable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFC\r"
        print(f"    Sending: {enable_msg.strip()}")
        ser.write(enable_msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Motor enable command sent\n")
        
        # Zero position
        print("[3] Setting zero position...")
        zero_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFE\r"
        print(f"    Sending: {zero_msg.strip()}")
        ser.write(zero_msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Zero position command sent\n")
        
        print("=" * 60)
        print("MOVEMENT TEST SEQUENCE")
        print("=" * 60)
        print("\n*** WATCH THE MOTOR SHAFT! ***\n")
        
        time.sleep(1)
        
        # Helper function to pack MIT command
        def float_to_uint(x, x_min, x_max, bits):
            span = x_max - x_min
            x = max(min(x, x_max), x_min)
            return int((x - x_min) * ((1 << bits) - 1) / span)
        
        def send_position(position, velocity=0.0, kp=50.0, kd=2.0, torque=0.0):
            # Pack values
            p_int = float_to_uint(position, -12.5, 12.5, 16)
            v_int = float_to_uint(velocity, -45.0, 45.0, 12)
            kp_int = float_to_uint(kp, 0.0, 500.0, 12)
            kd_int = float_to_uint(kd, 0.0, 5.0, 12)
            t_int = float_to_uint(torque, -18.0, 18.0, 12)
            
            # Pack into bytes
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
            msg = f"t{motor_id:03X}8{data.hex().upper()}\r"
            ser.write(msg.encode())
            # Read any response
            if ser.in_waiting > 0:
                ser.read(ser.in_waiting)
        
        # Test 1: Move to +2.0 radians (BIG movement)
        print("[Test 1/5] LARGE MOVEMENT - Moving to +2.0 radians (115 degrees)...")
        print("           Using HIGH gains: Kp=100, Kd=5")
        print("           THIS SHOULD BE VERY OBVIOUS!")
        for i in range(100):
            send_position(2.0, kp=100.0, kd=5.0)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"           Sending commands... {i}/100")
        print("           [OK] Should be at +2.0 rad")
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Move to -2.0 radians
        print("[Test 2/5] Moving to -2.0 radians (-115 degrees)...")
        print("           Using HIGH gains: Kp=100, Kd=5")
        for i in range(100):
            send_position(-2.0, kp=100.0, kd=5.0)
            time.sleep(0.02)
            if i % 25 == 0:
                print(f"           Sending commands... {i}/100")
        print("           [OK] Should be at -2.0 rad")
        print("           >> DID IT MOVE TO OPPOSITE SIDE? <<\n")
        time.sleep(2)
        
        # Test 3: Move to +3.0 radians (HUGE)
        print("[Test 3/5] HUGE MOVEMENT - Moving to +3.0 radians (172 degrees)...")
        print("           Using MAXIMUM gains: Kp=150, Kd=5")
        print("           THIS IS ALMOST A HALF ROTATION!")
        for i in range(120):
            send_position(3.0, kp=150.0, kd=5.0)
            time.sleep(0.02)
            if i % 30 == 0:
                print(f"           Sending commands... {i}/120")
        print("           [OK] Should be at +3.0 rad")
        print("           >> THIS WAS ALMOST HALF A ROTATION! Did you see it? <<\n")
        time.sleep(2)
        
        # Test 4: Smooth sine wave
        print("[Test 4/5] Smooth sine wave motion (5 seconds)...")
        print("           Amplitude: 2.0 rad, Frequency: 0.5 Hz")
        start_time = time.time()
        count = 0
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 2.0 * math.sin(2 * math.pi * 0.5 * t)
            send_position(position, kp=80.0, kd=4.0)
            time.sleep(0.02)
            count += 1
            if count % 50 == 0:
                print(f"           Sine wave motion... {t:.1f}s")
        print("           [OK] Sine wave complete")
        print("           >> Did you see smooth back-and-forth motion? <<\n")
        time.sleep(1)
        
        # Test 5: Return to zero
        print("[Test 5/5] Returning to zero position...")
        for i in range(80):
            send_position(0.0, kp=80.0, kd=4.0)
            time.sleep(0.02)
        print("           [OK] Should be at zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[4] Disabling motor...")
        disable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFD\r"
        print(f"    Sending: {disable_msg.strip()}")
        ser.write(disable_msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Motor disabled\n")
        
        ser.close()
        print("[5] Disconnected\n")
        
        print("=" * 60)
        print("[SUCCESS] MOVEMENT SEQUENCE COMPLETE!")
        print("=" * 60)
        
        print("\n" + "=" * 60)
        print("DIAGNOSTIC QUESTIONS:")
        print("=" * 60)
        print("\n1. Did you see the motor shaft physically rotate?")
        print("   [YES] - Great! Motor is working perfectly!")
        print("   [NO]  - Motor might not be powered or wired correctly")
        
        print("\n2. Can you manually rotate the motor shaft now?")
        print("   (Motor should be disabled and free to rotate)")
        print("   [YES] - Normal (motor is disabled)")
        print("   [NO]  - Motor may still be enabled or has brake")
        
        print("\n3. Was the motor making any sound during tests?")
        print("   [YES] - Motor is receiving power (good)")
        print("   [NO]  - Check power supply to motor")
        
        print("\n4. Are there any LEDs on the motor?")
        print("   What color/pattern are they showing?")
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] Cannot open {port}: {e}\n")
        print("Troubleshooting:")
        print("  1. Close any other programs using COM3:")
        print("     - Robstride PC software")
        print("     - Arduino IDE Serial Monitor")
        print("     - PuTTY or other terminal programs")
        print("     - Other Python scripts")
        print("\n  2. Check Windows Device Manager:")
        print("     - Find 'Ports (COM & LPT)'")
        print("     - Verify COM3 is listed")
        print("     - Try a different COM port if needed")
        print("\n  3. Unplug and replug the USB-CAN adapter")
        print("\n  4. Run as Administrator if permission denied")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Controller - CAN ID 13 on COM3\n")
    
    success = move_motor_canid13()
    
    if not success:
        print("\n" + "=" * 60)
        print("TROUBLESHOOTING GUIDE")
        print("=" * 60)
        print("\nIf COM3 is in use:")
        print("  1. Press Ctrl+Alt+Del -> Task Manager")
        print("  2. Find any Python processes and end them")
        print("  3. Close Robstride software if open")
        print("  4. Unplug USB-CAN adapter for 5 seconds")
        print("  5. Plug it back in and try again")
        
        print("\nIf motor doesn't move:")
        print("  1. Check motor power supply (12-48V DC)")
        print("     - Is the power supply ON?")
        print("     - Check voltage with multimeter")
        print("  2. Verify CANH and CANL wiring")
        print("     - CANH (motor) to CANH (adapter)")
        print("     - CANL (motor) to CANL (adapter)")
        print("  3. Ensure 120 ohm termination resistor")
        print("     - Between CANH and CANL at motor end")
        print("  4. Check motor CAN ID with Robstride software")
        print("  5. Try moving motor with Robstride PC software first")
        
        print("\nMotor might be in wrong mode:")
        print("  - Some Robstride motors need to be set to MIT mode")
        print("  - Use Robstride PC software to configure mode")
        print("  - Make sure motor firmware is up to date")

