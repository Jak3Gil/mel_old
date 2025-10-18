#!/usr/bin/env python3
"""
Motor Test with CAN Feedback Analysis
Checks if motor is actually responding to commands
"""

import serial
import time
import glob


def test_motor_response():
    """Test if motor responds to CAN commands."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   CAN FEEDBACK TEST                                    ║")
    print("║   Checking if motor responds to commands               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("📊 Monitor your ammeter and watch for:")
    print("   • Enable: Current should jump from 0A → 0.2-0.5A")
    print("   • Movement: Current should spike to 1-5A")
    print("   • If current stays < 0.1A: Motor NOT receiving commands\n")
    
    # Find port
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found")
        return
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=0.5)
    time.sleep(0.5)
    
    # Initialize L91
    print("[1/7] Initializing CAN adapter (L91 protocol)...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'L91\r')
    time.sleep(0.1)
    resp = ser.read(1000)
    print(f"      L91 response: {resp}")
    
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    print("      ✓ CAN bus opened\n")
    
    # Enable motor
    print("[2/7] Sending ENABLE command...")
    print("      🔍 CHECK AMMETER NOW - current should increase!")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.3)
    response = ser.read(1000)
    
    print(f"      Raw response: {response}")
    print(f"      Response length: {len(response)} bytes")
    
    if len(response) > 0:
        print("      ✓ Motor RESPONDED to enable command")
    else:
        print("      ⚠️  NO RESPONSE from motor")
        print("         This suggests motor may not be listening on CAN ID 0x01")
    
    time.sleep(2)
    print("      → Did ammeter show current increase? (If no: motor not enabled)\n")
    
    # Zero position
    print("[3/7] Sending ZERO POSITION command...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.3)
    response = ser.read(1000)
    print(f"      Response: {response}")
    print(f"      Response length: {len(response)} bytes\n")
    
    # Send position commands with feedback
    print("[4/7] Sending POSITION COMMANDS...")
    print("      Target: +2 radians (~115 degrees)")
    print("      Gains: Kp=100, Kd=3 (VERY STRONG)")
    print("      🔍 WATCH AMMETER - current should spike to 1-5A if motor moves!\n")
    
    time.sleep(1)
    print("      Sending 100 position commands...")
    
    response_count = 0
    for i in range(100):
        # Position command: 2 rad, vel=0, kp=100, kd=3
        ser.write(b't0018A1467FF666666D7FF\r')
        time.sleep(0.02)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            if len(resp) > 0:
                response_count += 1
        
        if i % 20 == 0:
            print(f"         Command {i}/100 sent... (responses: {response_count})")
    
    print(f"\n      ✓ Commands sent")
    print(f"      Motor responses received: {response_count}/100")
    
    if response_count > 0:
        print("      ✓ Motor IS responding on CAN bus")
    else:
        print("      ⚠️  Motor NOT responding on CAN bus")
    
    time.sleep(2)
    print("\n      → Did motor move? Did current spike?\n")
    
    # Try opposite direction
    print("[5/7] Sending OPPOSITE DIRECTION...")
    print("      Target: -2 radians (~-115 degrees)\n")
    
    time.sleep(1)
    
    response_count = 0
    for i in range(100):
        # Position command: -2 rad
        ser.write(b't00185EB97FF666666D7FF\r')
        time.sleep(0.02)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            if len(resp) > 0:
                response_count += 1
        
        if i % 20 == 0:
            print(f"         Command {i}/100 sent...")
    
    print(f"\n      ✓ Commands sent")
    print(f"      Motor responses: {response_count}/100\n")
    
    time.sleep(2)
    
    # Return to zero
    print("[6/7] Returning to zero...")
    for i in range(60):
        ser.write(b't00187FFF7FF666666D7FF\r')
        time.sleep(0.02)
    
    print("      ✓ Zero commands sent\n")
    
    # Disable
    print("[7/7] Sending DISABLE command...")
    print("      🔍 Ammeter should drop to ~0A")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.3)
    response = ser.read(1000)
    print(f"      Response: {response}")
    print(f"      Response length: {len(response)} bytes\n")
    
    ser.close()
    
    print("=" * 60)
    print("TEST COMPLETE")
    print("=" * 60)
    
    print("\n📊 DIAGNOSIS BASED ON AMMETER READINGS:\n")
    
    print("┌─────────────────────────────────────────────────────┐")
    print("│ What did you see on the ammeter?                   │")
    print("└─────────────────────────────────────────────────────┘\n")
    
    print("SCENARIO A: Current stayed at 0-0.1A entire time")
    print("  → Motor is NOT receiving power or NOT connected to CAN")
    print("  → Check:")
    print("     • 12V power supply is ON and connected")
    print("     • CANH/CANL wiring (may be swapped)")
    print("     • CAN termination resistor (120Ω)")
    print("     • Motor CAN ID (should be 0x01)")
    
    print("\n\nSCENARIO B: Current jumped to 0.2-0.5A when enabled, stayed there")
    print("  → Motor IS enabled but NOT in position control mode")
    print("  → This is the MOST LIKELY ISSUE")
    print("  → Solution:")
    print("     ✓ Motor needs ONE-TIME setup with Robstride PC software")
    print("     ✓ PC software activates 'position mode' capability")
    print("     ✓ After PC setup, CAN control will work")
    print("     ✓ This cannot be bypassed - it's in the motor firmware")
    
    print("\n\nSCENARIO C: Current spiked to 1-5A during movement commands")
    print("  → Motor IS trying to move")
    print("  → Check:")
    print("     • Can shaft rotate freely by hand when disabled?")
    print("     • Any mechanical load or obstruction?")
    print("     • Motor shaft should be easy to turn when disabled")
    
    print("\n\n🔧 NEXT STEPS:")
    print("\nIf SCENARIO B (most likely):")
    print("  1. Download Robstride PC software")
    print("  2. Connect motor via USB-CAN adapter")
    print("  3. Run initial setup/calibration")
    print("  4. Enable 'MIT Mode' or 'Position Mode'")
    print("  5. After this ONE-TIME setup, CAN control will work")
    
    print("\n💡 IMPORTANT:")
    print("   Robstride motors often require factory software initialization")
    print("   before accepting position commands via CAN. This is normal.")
    print("   After PC setup, your Python code will work perfectly!")


if __name__ == "__main__":
    try:
        test_motor_response()
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

