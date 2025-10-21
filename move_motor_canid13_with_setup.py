#!/usr/bin/env python3
"""
Move Robstride Motor CAN ID 13 on COM3
WITH PROPER MODE CONFIGURATION
"""

import serial
import time
import math

def move_motor_with_config():
    """Connect to COM3 and move motor CAN ID 13 with proper setup."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR - CAN ID 13 - COM3")
    print("   WITH MODE CONFIGURATION")
    print("=" * 60 + "\n")
    
    port = 'COM3'
    motor_id = 0x0D  # CAN ID 13
    sdo_cob_tx = 0x600 + motor_id  # SDO transmit: 0x60D
    sdo_cob_rx = 0x580 + motor_id  # SDO receive: 0x58D
    
    print(f"Motor CAN ID: {motor_id} (0x{motor_id:02X})")
    print(f"SDO TX COB-ID: 0x{sdo_cob_tx:03X}")
    print(f"SDO RX COB-ID: 0x{sdo_cob_rx:03X}")
    print(f"Opening {port}...\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        # Clear any pending data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
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
        
        # CRITICAL: Configure motor mode using SDO
        print("[2] Configuring motor for MIT control mode...")
        
        # Read current run mode (0x7005)
        print("    Reading current run_mode (0x7005)...")
        read_data = bytes([
            0x40,  # SDO read request
            0x05,  # Index low byte (0x7005)
            0x70,  # Index high byte
            0x00,  # Subindex
            0x00, 0x00, 0x00, 0x00  # Reserved
        ])
        read_msg = f"t{sdo_cob_tx:03X}8{read_data.hex().upper()}\r"
        print(f"    Sending: {read_msg.strip()}")
        ser.write(read_msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        
        # Write run_mode to MIT/Motion mode (mode 1)
        print("    Setting run_mode to MIT mode (0x01)...")
        write_data = bytes([
            0x2F,  # Write 1 byte
            0x05,  # Index low byte (0x7005)
            0x70,  # Index high byte
            0x00,  # Subindex
            0x01,  # Value: 1 = Motion/MIT mode
            0x00, 0x00, 0x00  # Padding
        ])
        write_msg = f"t{sdo_cob_tx:03X}8{write_data.hex().upper()}\r"
        print(f"    Sending: {write_msg.strip()}")
        ser.write(write_msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] MIT mode configured\n")
        
        # Set control parameters
        print("[3] Setting control parameters...")
        
        # Set Max velocity (0x7017) - 10 rad/s
        print("    Setting max velocity to 10.0 rad/s...")
        velocity_bytes = int(10.0 * 100).to_bytes(2, byteorder='little', signed=True)
        vel_data = bytes([
            0x2B,  # Write 2 bytes
            0x17,  # Index low byte (0x7017)
            0x70,  # Index high byte
            0x00,  # Subindex
        ]) + velocity_bytes + bytes([0x00, 0x00])
        vel_msg = f"t{sdo_cob_tx:03X}8{vel_data.hex().upper()}\r"
        ser.write(vel_msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        
        # Set Max current (0x7018) - 5A
        print("    Setting max current to 5.0A...")
        current_bytes = int(5.0 * 100).to_bytes(2, byteorder='little', signed=True)
        curr_data = bytes([
            0x2B,  # Write 2 bytes
            0x18,  # Index low byte (0x7018)
            0x70,  # Index high byte
            0x00,  # Subindex
        ]) + current_bytes + bytes([0x00, 0x00])
        curr_msg = f"t{sdo_cob_tx:03X}8{curr_data.hex().upper()}\r"
        ser.write(curr_msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        print("    [OK] Parameters set\n")
        
        # Enable motor
        print("[4] Enabling motor...")
        enable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFC\r"
        print(f"    Sending: {enable_msg.strip()}")
        ser.write(enable_msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.decode('ascii', errors='ignore').strip()}")
        print("    [OK] Motor enabled\n")
        
        # Zero position
        print("[5] Setting zero position...")
        zero_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFE\r"
        ser.write(zero_msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK] Position zeroed\n")
        
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
            # Read response
            if ser.in_waiting > 0:
                ser.read(ser.in_waiting)
        
        # Test 1: Small movement first
        print("[Test 1/6] Small movement - Moving to +0.5 radians (29 degrees)...")
        print("           Using moderate gains: Kp=50, Kd=2")
        for i in range(60):
            send_position(0.5, kp=50.0, kd=2.0)
            time.sleep(0.02)
        print("           [OK] Should be at +0.5 rad")
        print("           >> DID THE MOTOR MOVE? <<\n")
        time.sleep(2)
        
        # Test 2: Move to +1.5 radians
        print("[Test 2/6] Medium movement - Moving to +1.5 radians (86 degrees)...")
        for i in range(80):
            send_position(1.5, kp=60.0, kd=2.5)
            time.sleep(0.02)
        print("           [OK] Should be at +1.5 rad\n")
        time.sleep(2)
        
        # Test 3: Move to -1.5 radians
        print("[Test 3/6] Moving to -1.5 radians (-86 degrees)...")
        for i in range(80):
            send_position(-1.5, kp=60.0, kd=2.5)
            time.sleep(0.02)
        print("           [OK] Should be at -1.5 rad\n")
        time.sleep(2)
        
        # Test 4: LARGE movement
        print("[Test 4/6] LARGE MOVEMENT - Moving to +2.5 radians (143 degrees)...")
        print("           Using HIGH gains: Kp=100, Kd=4")
        for i in range(120):
            send_position(2.5, kp=100.0, kd=4.0)
            time.sleep(0.02)
        print("           [OK] Should be at +2.5 rad")
        print("           >> THIS WAS A BIG MOVEMENT! Did you see it? <<\n")
        time.sleep(2)
        
        # Test 5: Smooth sine wave
        print("[Test 5/6] Smooth sine wave motion (5 seconds)...")
        print("           Amplitude: 1.5 rad, Frequency: 0.5 Hz")
        start_time = time.time()
        count = 0
        while (time.time() - start_time) < 5.0:
            t = time.time() - start_time
            position = 1.5 * math.sin(2 * math.pi * 0.5 * t)
            send_position(position, kp=60.0, kd=3.0)
            time.sleep(0.02)
            count += 1
            if count % 50 == 0:
                print(f"           Sine wave motion... {t:.1f}s")
        print("           [OK] Sine wave complete\n")
        time.sleep(1)
        
        # Test 6: Return to zero
        print("[Test 6/6] Returning to zero position...")
        for i in range(80):
            send_position(0.0, kp=60.0, kd=2.5)
            time.sleep(0.02)
        print("           [OK] Should be at zero\n")
        time.sleep(1)
        
        # Disable motor
        print("[6] Disabling motor...")
        disable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFD\r"
        ser.write(disable_msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        print("    [OK] Motor disabled\n")
        
        ser.close()
        print("[7] Disconnected\n")
        
        print("=" * 60)
        print("[SUCCESS] MOVEMENT SEQUENCE COMPLETE!")
        print("=" * 60)
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] Cannot open {port}: {e}\n")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Controller with Configuration - CAN ID 13\n")
    
    success = move_motor_with_config()
    
    if not success:
        print("\n" + "=" * 60)
        print("STILL NOT WORKING?")
        print("=" * 60)
        print("\nTry this:")
        print("  1. Open Robstride PC software")
        print("  2. Connect to the motor")
        print("  3. Check what 'Control Mode' it's using")
        print("  4. Try setting it to 'MIT Mode' or 'Motion Mode'")
        print("  5. Move the motor in the software to verify")
        print("  6. Close the software and try this script again")

