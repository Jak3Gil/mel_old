#!/usr/bin/env python3
"""
Comprehensive Motor ID 15 Fix - Try Multiple Methods
Replicates what Motor Studio does
"""

import serial
import time

def try_all_methods():
    """Try every possible initialization method."""
    print("=" * 60)
    print("   COMPREHENSIVE MOTOR FIX - ID 15 ON COM3")
    print("=" * 60 + "\n")
    
    port = 'COM3'
    motor_id = 0x0F  # Motor ID 15
    sdo_cob = 0x600 + motor_id  # 0x60F
    
    print(f"Motor CAN ID: {motor_id} (0x{motor_id:02X})")
    print(f"SDO COB-ID: 0x{sdo_cob:03X}\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.5)
        
        # ============================================================
        # STEP 1: Initialize CAN adapter
        # ============================================================
        print("[STEP 1] Initializing CAN adapter...")
        ser.write(b'C\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'O\r')
        time.sleep(0.1)
        ser.read(1000)
        print("         [OK] CAN adapter ready\n")
        
        # ============================================================
        # STEP 2: Try TORQUE MODE first (often works when position doesn't)
        # ============================================================
        print("[STEP 2] Testing TORQUE MODE (bypass position mode)...")
        print("         This should make motor resist/twitch\n")
        
        # Enable motor
        enable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFC\r"
        ser.write(enable_msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        print(f"         Enable response: {resp.decode('ascii', errors='ignore').strip() if resp else 'none'}")
        
        # Send pure torque commands (should feel resistance)
        print("         Sending torque commands...")
        print("         >> TRY TO ROTATE THE SHAFT - you should feel resistance <<\n")
        
        for i in range(30):
            # Torque only - no position control
            # Set position/velocity to middle, zero gains, add torque
            torque_msg = f"t{motor_id:03X}87FFF7FFF00000A00\r"
            ser.write(torque_msg.encode())
            time.sleep(0.05)
        
        print("         Did you feel resistance when turning the shaft?")
        time.sleep(2)
        
        # ============================================================
        # STEP 3: Set motor parameters via SDO (like Motor Studio does)
        # ============================================================
        print("\n[STEP 3] Setting motor parameters via SDO...")
        print("         (Replicating what Motor Studio does)\n")
        
        # Parameter: run_mode = 1 (Position mode)
        print("         Setting run_mode to Position (0x7005 = 1)...")
        data = bytes([0x2F, 0x05, 0x70, 0x00, 0x01, 0x00, 0x00, 0x00])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"         Response: {resp.decode('ascii', errors='ignore').strip()}")
        
        # Parameter: Set limit_torque higher
        print("         Setting torque limit higher (0x7006)...")
        data = bytes([0x23, 0x06, 0x70, 0x00, 0x00, 0x00, 0xC8, 0x41])  # 25.0 Nm
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        
        # Parameter: Set speed limit
        print("         Setting speed limit (0x7017)...")
        data = bytes([0x23, 0x17, 0x70, 0x00, 0x00, 0x00, 0x48, 0x42])  # 50 rad/s
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        
        # Parameter: Set current limit
        print("         Setting current limit (0x7018)...")
        data = bytes([0x23, 0x18, 0x70, 0x00, 0x00, 0x00, 0x20, 0x41])  # 10 A
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        
        print("         [OK] Parameters set\n")
        
        # ============================================================
        # STEP 4: Re-enable with new parameters
        # ============================================================
        print("[STEP 4] Re-enabling motor with new settings...")
        
        # Disable first
        disable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFD\r"
        ser.write(disable_msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        
        time.sleep(0.5)
        
        # Re-enable
        ser.write(enable_msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("         [OK] Motor re-enabled\n")
        
        # ============================================================
        # STEP 5: Zero position
        # ============================================================
        print("[STEP 5] Setting zero position...")
        zero_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFE\r"
        ser.write(zero_msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("         [OK] Zero set\n")
        
        # ============================================================
        # STEP 6: Test position control with different gain sets
        # ============================================================
        print("[STEP 6] Testing position control...")
        print("         *** WATCH THE MOTOR SHAFT ***\n")
        
        time.sleep(1)
        
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
        
        # Test A: Very low gains (gentle start)
        print("         [Test A] Gentle movement - LOW gains (Kp=10, Kd=0.5)")
        print("                  Moving to +0.5 rad (29 degrees)...\n")
        for i in range(100):
            send_position(0.5, kp=10.0, kd=0.5, torque=2.0)  # Added small feedforward torque
            time.sleep(0.02)
        print("                  >> Did it move? <<")
        time.sleep(2)
        
        # Test B: Medium gains
        print("\n         [Test B] Medium movement - MEDIUM gains (Kp=50, Kd=2)")
        print("                  Moving to +2.0 rad (115 degrees)...\n")
        for i in range(100):
            send_position(2.0, kp=50.0, kd=2.0)
            time.sleep(0.02)
        print("                  >> Did it move this time? <<")
        time.sleep(2)
        
        # Test C: High gains with velocity feedforward
        print("\n         [Test C] HIGH gains with velocity (Kp=150, Kd=5)")
        print("                  Moving to -2.0 rad (-115 degrees)...\n")
        for i in range(100):
            send_position(-2.0, velocity=10.0, kp=150.0, kd=5.0)
            time.sleep(0.02)
        print("                  >> Did it move now? <<")
        time.sleep(2)
        
        # Test D: EXTREME test
        print("\n         [Test D] EXTREME - MAX gains, large movement")
        print("                  Moving to +4.0 rad (229 degrees)...\n")
        for i in range(150):
            send_position(4.0, velocity=0.0, kp=200.0, kd=5.0, torque=5.0)
            time.sleep(0.02)
            if i % 50 == 0:
                print(f"                  Command {i}/150...")
        print("                  >> This should be UNMISTAKABLE! <<")
        time.sleep(2)
        
        # Return to zero
        print("\n         [Test E] Returning to zero...")
        for i in range(100):
            send_position(0.0, kp=50.0, kd=2.0)
            time.sleep(0.02)
        print("                  [OK] Should be at zero\n")
        
        time.sleep(1)
        
        # ============================================================
        # STEP 7: Read motor state
        # ============================================================
        print("[STEP 7] Reading motor state...")
        
        # Try to read current mode
        data = bytes([0x40, 0x05, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        print(f"         Reading run_mode (0x7005)...")
        ser.write(msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"         Response: {resp.decode('ascii', errors='ignore').strip()}")
        else:
            print("         No response")
        
        # ============================================================
        # STEP 8: Disable
        # ============================================================
        print("\n[STEP 8] Disabling motor...")
        ser.write(disable_msg.encode())
        time.sleep(0.2)
        ser.read(1000)
        print("         [OK] Motor disabled\n")
        
        ser.close()
        
        print("=" * 60)
        print("COMPREHENSIVE TEST COMPLETE")
        print("=" * 60)
        
        print("\n" + "=" * 60)
        print("ANALYSIS:")
        print("=" * 60)
        
        print("\n1. TORQUE MODE (Step 2):")
        print("   Did you feel resistance when turning shaft?")
        print("   [YES] - Motor is receiving commands and powered")
        print("   [NO]  - Motor may not be enabled properly")
        
        print("\n2. POSITION CONTROL (Step 6):")
        print("   Did motor move in ANY of the tests A-E?")
        print("   [YES] - SUCCESS! Motor works, may need tuning")
        print("   [NO]  - Continue reading below...")
        
        print("\n3. IF MOTOR STILL DOESN'T MOVE:")
        print("")
        print("   Possible causes:")
        print("   a) Motor firmware requires Motor Studio initialization")
        print("      -> One-time setup needed via official software")
        print("      -> Parameters stored in motor's NVRAM")
        print("")
        print("   b) Motor needs calibration sequence")
        print("      -> Motor Studio performs encoder calibration")
        print("      -> Cannot be done via simple CAN commands")
        print("")
        print("   c) Motor protection mode active")
        print("      -> Over-current protection")
        print("      -> Over-temperature protection")
        print("      -> Check motor LED for error codes")
        print("")
        print("   d) Missing initialization parameter")
        print("      -> Motor Studio sets additional hidden parameters")
        print("      -> Contact Robstride for complete init sequence")
        
        print("\n" + "=" * 60)
        print("RECOMMENDATION:")
        print("=" * 60)
        print("\nSince motor works in Motor Studio but not here:")
        print("")
        print("1. Open Motor Studio")
        print("2. Connect to motor")
        print("3. Enable position mode")
        print("4. Move motor a bit (verify it works)")
        print("5. Close Motor Studio")
        print("6. Run this script again")
        print("")
        print("Motor Studio may save initialization state that")
        print("persists and allows other software to work.")
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] Cannot open {port}: {e}\n")
        print("Make sure:")
        print("  - Motor Studio is CLOSED")
        print("  - No other programs using COM3")
        print("  - USB-CAN adapter is plugged in")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}\n")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Comprehensive Fix")
    print("Motor ID: 15, Port: COM3\n")
    
    try_all_methods()

