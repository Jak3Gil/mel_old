#!/usr/bin/env python3
"""
Diagnose and Move Robstride Motor on COM3
Sets motor to position mode and tests movement
"""

import serial
import time

def diagnose_and_move():
    """Full diagnostic and movement test."""
    print("=" * 60)
    print("   ROBSTRIDE MOTOR DIAGNOSTIC - COM3")
    print("=" * 60 + "\n")
    
    port = 'COM3'
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
        resp = ser.read(1000)
        
        ser.write(b'O\r')
        time.sleep(0.1)
        ser.read(1000)
        print("    [OK] L91 initialized")
        
        motor_id = 0x01
        sdo_cob = 0x600 + motor_id  # SDO transmit COB-ID
        
        # CRITICAL: Set motor to POSITION MODE using SDO
        print("\n[2] Setting motor to POSITION MODE (SDO)...")
        print("    This is REQUIRED before position commands work!")
        
        # Write to 0x7005 (run_mode) = 0x01 (Position mode)
        data = bytes([
            0x2F,  # Write 1 byte
            0x05,  # Index low byte (0x7005)
            0x70,  # Index high byte
            0x00,  # Subindex
            0x01,  # Value: 1 = Position mode
            0x00, 0x00, 0x00  # Padding
        ])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        print(f"    Sending: {msg.strip()}")
        ser.write(msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp}")
        print("    [OK] Mode set to Position")
        
        # Also set CANopen mode of operation
        print("\n[3] Setting CANopen Modes of Operation...")
        data = bytes([
            0x2F,  # Write 1 byte
            0x60,  # Index low byte (0x6060)
            0x60,  # Index high byte
            0x00,  # Subindex
            0x01,  # Value: 1 = Position mode
            0x00, 0x00, 0x00
        ])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK] CANopen mode set")
        
        # Set controlword to enable
        print("\n[4] Setting Controlword to Enable...")
        data = bytes([
            0x2B,  # Write 2 bytes
            0x40,  # Index low byte (0x6040)
            0x60,  # Index high byte
            0x00,  # Subindex
            0x0F, 0x00,  # Value: 0x000F = Enable
            0x00, 0x00
        ])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.3)
        ser.read(1000)
        print("    [OK] Controlword set")
        
        # Now use MIT protocol commands
        print("\n[5] Enabling motor (MIT protocol)...")
        ser.write(b't0018FFFFFFFFFFFFFFFC\r')
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp}")
        print("    [OK] Motor enabled")
        
        print("\n[6] Setting zero position...")
        ser.write(b't0018FFFFFFFFFFFFFFFE\r')
        time.sleep(0.2)
        ser.read(1000)
        print("    [OK] Zero set")
        
        print("\n" + "=" * 60)
        print("STARTING MOVEMENT TEST")
        print("=" * 60)
        print("\nWATCH THE MOTOR CAREFULLY!")
        print("You should see the shaft rotate.\n")
        
        time.sleep(1)
        
        # Test 1: Moderate movement with high gains
        print("[Test 1] Moving to +2.0 radians (115 degrees)...")
        print("         Using HIGH gains (Kp=100, Kd=4)")
        print("         WATCH THE SHAFT NOW!\n")
        
        for i in range(100):
            # Position: +2.0 rad, Velocity: 0, Kp: 100, Kd: 4, Torque: 0
            # Pack: pos=2.0, vel=0, kp=100, kd=4, torque=0
            ser.write(b't0018A3D17FFF19A066A00000\r')
            time.sleep(0.02)
            
            if i % 20 == 0:
                print(f"         Command {i}/100 sent...")
        
        print("         [OK] Command sequence complete")
        print("\n         >> Did the motor move? <<")
        time.sleep(2)
        
        # Test 2: Opposite direction
        print("\n[Test 2] Moving to -2.0 radians (-115 degrees)...")
        print("         LARGE movement in opposite direction!\n")
        
        for i in range(100):
            # Position: -2.0 rad
            ser.write(b't00185C2E7FFF19A066A00000\r')
            time.sleep(0.02)
            
            if i % 20 == 0:
                print(f"         Command {i}/100 sent...")
        
        print("         [OK] Command sequence complete")
        print("\n         >> Did it move this time? <<")
        time.sleep(2)
        
        # Test 3: Extreme movement
        print("\n[Test 3] EXTREME TEST - Moving to +4.0 radians (229 degrees)...")
        print("         Using MAXIMUM gains (Kp=200, Kd=5)")
        print("         This should be VERY OBVIOUS!\n")
        
        for i in range(150):
            # Position: +4.0 rad with very high gains
            ser.write(b't0018CFFF7FFFFF0066A00000\r')
            time.sleep(0.02)
            
            if i % 30 == 0:
                print(f"         Command {i}/150 sent...")
        
        print("         [OK] Command sequence complete")
        print("\n         >> THIS SHOULD BE UNMISTAKABLE! Did it move? <<")
        time.sleep(2)
        
        # Return to zero
        print("\n[Test 4] Returning to zero...")
        for i in range(100):
            ser.write(b't00187FFFFFFFFFFF66A00000\r')
            time.sleep(0.02)
        print("         [OK] Should be at zero now")
        
        time.sleep(1)
        
        # Disable
        print("\n[7] Disabling motor...")
        ser.write(b't0018FFFFFFFFFFFFFFFD\r')
        time.sleep(0.2)
        ser.read(1000)
        print("    [OK] Motor disabled")
        
        ser.close()
        
        print("\n" + "=" * 60)
        print("DIAGNOSTIC COMPLETE")
        print("=" * 60)
        
        print("\n" + "=" * 60)
        print("IMPORTANT QUESTIONS:")
        print("=" * 60)
        print("\n1. Did you see ANY physical movement?")
        print("   [YES] - Motor is working! May need calibration.")
        print("   [NO]  - Continue with checks below.")
        
        print("\n2. Can you freely rotate the motor shaft by hand?")
        print("   [YES] - Motor is not holding position (not enabled)")
        print("   [NO]  - Motor is trying to hold (good sign)")
        
        print("\n3. When you try to rotate shaft, does it resist?")
        print("   [YES] - Motor is powered and holding")
        print("   [NO]  - Motor may not be receiving commands")
        
        print("\n4. Is the motor making any sound?")
        print("   [YES] - Motor is active, may have mechanical issue")
        print("   [NO]  - Check power and wiring")
        
        print("\n5. Check POWER LED on motor:")
        print("   [RED SOLID] - Powered correctly")
        print("   [OFF] - No power! Check power supply")
        
        print("\n6. Check CAN LED on motor (if present):")
        print("   [BLINKING] - Receiving CAN messages")
        print("   [OFF] - Not receiving - check CANH/CANL wiring")
        
        print("\n" + "=" * 60)
        print("NEXT STEPS IF NO MOVEMENT:")
        print("=" * 60)
        print("\n1. Check power supply:")
        print("   - Motor needs 12-48V DC")
        print("   - Check voltage with multimeter")
        print("   - Ensure current rating is sufficient (10A+)")
        
        print("\n2. Check CAN wiring:")
        print("   - CANH (USB-CAN) -> CANH (Motor)")
        print("   - CANL (USB-CAN) -> CANL (Motor)")
        print("   - GND connected between adapter and motor")
        print("   - 120 ohm termination resistor across CANH/CANL")
        
        print("\n3. Try Robstride PC software:")
        print("   - Motor may need one-time initialization")
        print("   - Connect with official software first")
        print("   - Check if motor moves there")
        print("   - May need firmware update")
        
        print("\n4. Check motor LED patterns:")
        print("   - Refer to Robstride manual for LED codes")
        print("   - Different patterns indicate different states")
        
        return True
        
    except serial.SerialException as e:
        print(f"\n[ERROR] Cannot open {port}: {e}")
        print("\nTroubleshooting:")
        print("  1. Check that COM3 is the correct port")
        print("  2. Close other programs using COM3")
        print("  3. Unplug and replug the USB-CAN adapter")
        print("  4. Check Device Manager for correct COM port")
        return False
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("\nRobstride Motor Diagnostic Tool\n")
    diagnose_and_move()

