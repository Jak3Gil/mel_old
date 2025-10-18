#!/usr/bin/env python3
"""
Enable Position Control Mode via CAN
Try to activate MIT/Position mode without PC software
"""

import serial
import time
import glob


def try_enable_position_mode():
    """Try various methods to enable position control mode."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ENABLE POSITION MODE VIA CAN                         ║")
    print("║   Trying to activate position control without PC SW    ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found")
        return False
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=0.5)
    time.sleep(0.5)
    
    # Initialize
    print("[1/10] Initializing CAN adapter...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    print("       ✓ CAN ready\n")
    
    motor_id = 0x01
    
    # Try CANopen SDO to set modes
    print("[2/10] Trying CANopen SDO commands...")
    print("       Setting Modes of Operation (0x6060 = Position Mode)\n")
    
    # SDO write to 0x6060 (Modes of Operation) = 0x01 (Position mode)
    sdo_commands = [
        # Format: [cmd, index_low, index_high, sub, data...]
        (bytes([0x2F, 0x60, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00]), "Set Mode = Profile Position"),
        (bytes([0x2F, 0x60, 0x60, 0x00, 0x08, 0x00, 0x00, 0x00]), "Set Mode = Cyclic Sync Position"),
        (bytes([0x2F, 0x60, 0x60, 0x00, 0x0A, 0x00, 0x00, 0x00]), "Set Mode = Cyclic Sync Torque"),
    ]
    
    sdo_cob = 0x600 + motor_id
    for data, desc in sdo_commands:
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        print(f"       {desc}")
        print(f"       Sending: {msg.strip()}")
        ser.write(msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if len(resp) > 0:
            print(f"       Response: {resp.hex()}")
        print()
    
    # Try to set controlword
    print("[3/10] Setting CANopen Controlword sequence...")
    print("       (This enables the drive)\n")
    
    controlwords = [
        (0x06, "Shutdown"),
        (0x07, "Switch On"),
        (0x0F, "Enable Operation"),
    ]
    
    for val, desc in controlwords:
        data = bytes([0x2F, 0x40, 0x60, 0x00, val, 0x00, 0x00, 0x00])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        print(f"       {desc} (0x{val:02X})")
        ser.write(msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if len(resp) > 0:
            print(f"       Response: {resp.hex()}")
    
    print("\n[4/10] Trying MIT Mode enable sequence...")
    
    # MIT protocol enable
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    resp = ser.read(1000)
    print(f"       Enable response: {resp.hex() if resp else 'none'}")
    
    # Try motor mode commands
    print("\n[5/10] Trying motor mode selection commands...")
    
    mode_commands = [
        (b't0018FFFFFFFFFFFF0001\r', "Mode 1 (Position)"),
        (b't0018FFFFFFFFFFFF0008\r', "Mode 8 (Position)"),
        (b't0018FFFFFFFFFFFF000A\r', "Mode 10 (Torque)"),
    ]
    
    for cmd, desc in mode_commands:
        print(f"       {desc}")
        ser.write(cmd)
        time.sleep(0.3)
        resp = ser.read(1000)
        if len(resp) > 0:
            print(f"       Response: {resp.hex()}")
    
    # Try encoder calibration
    print("\n[6/10] Forcing encoder calibration...")
    ser.write(b't0018FFFFFFFFFFFF0006\r')
    time.sleep(0.5)
    resp = ser.read(1000)
    print(f"       Calibration response: {resp.hex() if resp else 'none'}")
    
    # Try to read motor parameters
    print("\n[7/10] Trying to read motor status...")
    
    # Read SDO requests
    read_commands = [
        (bytes([0x40, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00]), "Read Modes of Operation"),
        (bytes([0x40, 0x41, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00]), "Read Statusword"),
    ]
    
    for data, desc in read_commands:
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        print(f"       {desc}")
        ser.write(msg.encode())
        time.sleep(0.3)
        resp = ser.read(1000)
        if len(resp) > 0:
            print(f"       Response: {resp.hex()}")
    
    # Try alternative MIT enable sequences
    print("\n[8/10] Trying alternative enable sequences...")
    
    alt_enables = [
        b't0018FFFFFFFFFFFFFC00\r',
        b't0018FCFFFFFFFFFFFFFF\r',
        b't00187FFFFFFFFFFFFFFC\r',
    ]
    
    for cmd in alt_enables:
        ser.write(cmd)
        time.sleep(0.2)
        resp = ser.read(1000)
        if len(resp) > 0 and resp != b'ffffffff7fffffffffffffffffffffffffffffffff':
            print(f"       Different response: {resp.hex()}")
    
    # Set zero and try position
    print("\n[9/10] Testing position control...")
    print("       Setting zero...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.3)
    ser.read(1000)
    
    print("       Sending position command (1 radian)...")
    print("       🔍 WATCH THE MOTOR - does it move?\n")
    
    for i in range(50):
        # Position: 1 rad, Kp=100, Kd=3
        ser.write(b't0018901D7FF8CCCCD7FF\r')
        time.sleep(0.02)
    
    time.sleep(1)
    
    print("       Returning to zero...")
    for i in range(50):
        ser.write(b't00187FFF7FF8CCCCD7FF\r')
        time.sleep(0.02)
    
    # Disable
    print("\n[10/10] Disabling...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.2)
    ser.read(1000)
    
    ser.close()
    
    print("\n" + "="*60)
    print("POSITION MODE ENABLE ATTEMPTS COMPLETE")
    print("="*60)
    
    print("\n❓ DID THE MOTOR MOVE during step [9/10]?")
    print("\n  ✓ YES - Success! Position mode is now active")
    print("  ✗ NO - Motor still not in position mode")
    
    return True


def troubleshoot_servo_studio():
    """Help troubleshoot Servo Studio connection."""
    print("\n\n╔════════════════════════════════════════════════════════╗")
    print("║   TROUBLESHOOTING ROBSTRIDE SERVO STUDIO               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("Common issues with Servo Studio not connecting:\n")
    
    print("1️⃣  WRONG COM PORT SELECTED:")
    print("   • Servo Studio might be looking at wrong port")
    print("   • Try ALL available COM ports in dropdown")
    print("   • On Mac: Look for /dev/cu.usbserial-*")
    print("   • On Windows: Try COM3, COM4, COM5, etc.\n")
    
    print("2️⃣  USB-CAN ADAPTER DRIVER:")
    print("   • Servo Studio might need specific driver")
    print("   • Install CH340/CH341 driver if using cheap adapter")
    print("   • Or FTDI driver for FTDI-based adapters")
    print("   • Restart computer after driver install\n")
    
    print("3️⃣  SOFTWARE COMPATIBILITY:")
    print("   • Try running as Administrator (Windows)")
    print("   • Try older version of Servo Studio")
    print("   • Check if software supports your adapter\n")
    
    print("4️⃣  CAN SETTINGS IN SOFTWARE:")
    print("   • Baudrate: Try 1000000 (1Mbps) or 500000")
    print("   • Protocol: Select SLCAN or CANopen")
    print("   • Motor ID: Set to 1 (0x01)\n")
    
    print("5️⃣  PYTHON SCRIPT INTERFERENCE:")
    print("   • Close ALL Python scripts first")
    print("   • Make sure no other program is using the port")
    print("   • Unplug and replug USB-CAN adapter")
    print("   • Then try Servo Studio again\n")
    
    print("6️⃣  TRY ROBSTRIDE PHONE APP:")
    print("   • Some Robstride motors work with mobile app")
    print("   • Look for 'RobStride' app on iOS/Android")
    print("   • Might be easier than PC software\n")
    
    print("="*60)
    print("\n💡 ALTERNATIVE: Contact Robstride Support")
    print("\nIf Servo Studio won't connect, email Robstride:")
    print("  • Explain: Motor responds on CAN but won't move")
    print("  • Ask: How to enable position mode via CAN")
    print("  • Request: Alternative configuration method")
    print("  • Mention: Using USB-CAN adapter, motor responds")
    print("           but doesn't execute position commands")


if __name__ == "__main__":
    print("\n" + "="*60)
    print("ROBSTRIDE POSITION MODE ACTIVATION")
    print("="*60)
    print("\nSince Servo Studio won't connect, we'll try to")
    print("enable position mode via CAN commands directly.\n")
    
    time.sleep(2)
    
    try:
        success = try_enable_position_mode()
        
        if success:
            troubleshoot_servo_studio()
            
            print("\n\n" + "="*60)
            print("FINAL RECOMMENDATIONS")
            print("="*60)
            
            print("\n1. If motor MOVED:")
            print("   → Test with: python3 test_obvious_movement.py")
            print("   → Use K-Scale architecture for control")
            
            print("\n2. If motor STILL doesn't move:")
            print("   → Your motor firmware requires PC software unlock")
            print("   → This is a hardware/firmware limitation")
            print("   → Options:")
            print("     a) Fix Servo Studio connection (see troubleshooting)")
            print("     b) Contact Robstride support for alternative")
            print("     c) Try mobile app if available")
            
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

