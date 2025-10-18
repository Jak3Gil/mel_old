#!/usr/bin/env python3
"""
Full motor diagnosis - check every aspect
"""

import serial
import time
import glob

def full_diagnosis():
    """Complete motor diagnosis."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   COMPLETE MOTOR DIAGNOSIS                             ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    # Find port
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found!")
        return
    
    port = ports[0]
    print(f"Port: {port}\n")
    print("="*60)
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        print("TEST 1: USB-CAN Adapter")
        print("-"*60)
        
        # Test adapter
        ser.write(b'V\r')
        time.sleep(0.1)
        resp = ser.read(100)
        if resp:
            print(f"✓ Adapter version: {resp.hex()}")
        else:
            print("⚠ No version response")
        
        print("\nTEST 2: CAN Initialization")
        print("-"*60)
        
        # Initialize
        ser.write(b'C\r')
        time.sleep(0.1)
        resp1 = ser.read(100)
        print(f"Close response: {resp1.hex() if resp1 else '(none)'}")
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        resp2 = ser.read(100)
        print(f"L91 response: {resp2.hex() if resp2 else '(none)'}")
        
        ser.write(b'O\r')
        time.sleep(0.1)
        resp3 = ser.read(100)
        print(f"Open response: {resp3.hex() if resp3 else '(none)'}")
        
        if resp1 or resp2 or resp3:
            print("✓ Adapter initializing")
        else:
            print("✗ No initialization responses")
        
        print("\nTEST 3: Motor Enable (0xFC)")
        print("-"*60)
        
        ser.write(b't0018FFFFFFFFFFFFFFFC\r')
        time.sleep(0.2)
        resp = ser.read(1000)
        print(f"Enable response: {resp.hex() if resp else '(none)'}")
        print(f"Response length: {len(resp) if resp else 0} bytes")
        
        if resp:
            print("✓ Motor acknowledges enable")
        else:
            print("✗ No response to enable")
        
        print("\nTEST 4: Motor Zero (0xFE)")
        print("-"*60)
        
        ser.write(b't0018FFFFFFFFFFFFFFFE\r')
        time.sleep(0.2)
        resp = ser.read(1000)
        print(f"Zero response: {resp.hex() if resp else '(none)'}")
        
        print("\nTEST 5: Position Commands")
        print("-"*60)
        print("Sending 5 position commands with MAXIMUM gains...")
        print("(If motor is working, it MUST respond to this)")
        
        # Extreme gains: kp=500 (max), kd=5 (max), position=2 radians
        response_count = 0
        
        for i in range(5):
            # Maximum gain command
            ser.write(b't00188A3D7FFFFFFFFFFF\r')
            time.sleep(0.1)
            
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                response_count += 1
                print(f"  Response {i+1}: {data.hex()}")
        
        print(f"\nResponses received: {response_count}/5")
        
        print("\nTEST 6: Motor Disable (0xFD)")
        print("-"*60)
        
        ser.write(b't0018FFFFFFFFFFFFFFFD\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"Disable response: {resp.hex() if resp else '(none)'}")
        
        ser.close()
        
        print("\n" + "="*60)
        print("DIAGNOSIS RESULTS")
        print("="*60)
        
        if response_count > 0:
            print("\n✓ COMMUNICATION: Working")
            print("✗ MOVEMENT: Not working")
            print("\nThe motor IS receiving commands and responding,")
            print("but it's NOT executing position commands.")
            print("\n🔧 LIKELY CAUSES:")
            print("  1. Motor in FAULT/ERROR state")
            print("  2. Motor encoder not initialized")
            print("  3. Motor needs factory reset")
            print("  4. Motor in bootloader/config mode (not operational)")
            print("\n💡 SOLUTIONS TO TRY:")
            print("  A. POWER CYCLE motor (OFF 30 seconds, then ON)")
            print("  B. Try DIFFERENT motor if you have one")
            print("  C. Check motor documentation for 'factory reset' procedure")
            print("  D. Motor may need initial setup that can't be done via CAN")
        else:
            print("\n✗ Motor not responding at all")
            print("Check wiring and power")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    full_diagnosis()
    
    print("\n\n" + "="*60)
    print("NEXT STEPS")
    print("="*60)
    print("\n1. POWER CYCLE THE MOTOR:")
    print("   - Turn OFF motor power")
    print("   - Wait 30 seconds")
    print("   - Turn ON motor power")
    print("   - Wait for LEDs to settle")
    print("   - Run this test again")
    print("\n2. CHECK MOTOR LEDs:")
    print("   - What is red LED doing? (solid/flash/pattern)")
    print("   - What is blue LED doing? (solid/flash/pattern)")
    print("   - LED patterns indicate motor state")
    print("\n3. PHYSICAL CHECK:")
    print("   - Can you freely rotate motor shaft by hand?")
    print("   - Is there mechanical resistance?")
    print("   - Does motor make any sound when powered?")

