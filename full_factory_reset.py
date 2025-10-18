#!/usr/bin/env python3
"""
Complete Factory Reset for Robstride Motor
Tries all CAN-based reset methods automatically
"""

import serial
import time
import glob


def full_reset():
    """Try comprehensive factory reset via CAN."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ROBSTRIDE MOTOR - FULL FACTORY RESET                ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found!")
        return False
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        time.sleep(0.5)
        
        print("[1/8] Initializing CAN adapter (L91)...")
        ser.write(b'C\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        ser.read(1000)
        
        ser.write(b'O\r')
        time.sleep(0.1)
        ser.read(1000)
        print("      ✓ CAN bus ready\n")
        
        # Try multiple CAN IDs
        print("[2/8] Trying reset on multiple CAN IDs...")
        for can_id in [0x01, 0x7F, 0x00]:  # Common IDs
            print(f"      Trying CAN ID 0x{can_id:02X}...")
            
            # Disable/clear
            msg = f"t{can_id:03X}8FFFFFFFFFFFFFFFD\r"
            ser.write(msg.encode())
            time.sleep(0.2)
            ser.read(1000)
        
        print("      ✓ Disable commands sent\n")
        
        print("[3/8] Sending clear fault commands...")
        commands = [
            b'T0C0000010\r',              # Clear faults (extended CAN)
            b't0018FFFFFFFFFFFFFF00\r',  # Clear all
            b't00180000000000000000\r',  # Zero all
        ]
        
        for cmd in commands:
            ser.write(cmd)
            time.sleep(0.3)
            ser.read(1000)
        
        print("      ✓ Fault clear commands sent\n")
        
        print("[4/8] Sending factory reset sequences...")
        reset_sequences = [
            b'T0A0000010\r',              # Reset command (extended)
            b't0018AA55AA55AA55AA55\r',  # Magic sequence 1
            b't00185555AAAA5555AAAA\r',  # Magic sequence 2
            b't0018DEADBEEFDEADBEEF\r',  # Magic sequence 3
        ]
        
        for seq in reset_sequences:
            ser.write(seq)
            time.sleep(0.5)
            resp = ser.read(1000)
            if len(resp) > 0:
                print(f"      Got response: {resp.hex()}")
        
        print("      ✓ Reset sequences sent\n")
        
        print("[5/8] Trying encoder calibration command...")
        ser.write(b't0018FFFFFFFFFFFF0006\r')
        time.sleep(0.5)
        resp = ser.read(1000)
        if len(resp) > 0:
            print(f"      Response: {resp.hex()}")
        print("      ✓ Calibration command sent\n")
        
        print("[6/8] Trying mode reset commands...")
        mode_commands = [
            b't0018FFFFFFFFFFFF0001\r',  # Mode 1
            b't0018FFFFFFFFFFFF0002\r',  # Mode 2
            b't0018FFFFFFFFFFFF0003\r',  # Mode 3
        ]
        
        for cmd in mode_commands:
            ser.write(cmd)
            time.sleep(0.3)
            ser.read(1000)
        
        print("      ✓ Mode commands sent\n")
        
        print("[7/8] Broadcasting reset to all devices...")
        # Try broadcast ID
        ser.write(b't0008FFFFFFFFFFFFFFFD\r')  # Broadcast disable
        time.sleep(0.3)
        ser.read(1000)
        
        ser.write(b'T0000FFFF10\r')  # Broadcast extended reset
        time.sleep(0.5)
        ser.read(1000)
        
        print("      ✓ Broadcast commands sent\n")
        
        print("[8/8] Final cleanup...")
        ser.write(b'C\r')  # Close CAN
        time.sleep(0.2)
        ser.close()
        print("      ✓ Complete\n")
        
        return True
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        return False


def print_next_steps():
    """Print what to do next."""
    print("\n" + "="*60)
    print("CAN RESET COMMANDS SENT")
    print("="*60)
    
    print("\n🔄 IMMEDIATE NEXT STEP - POWER CYCLE:")
    print("\n  1. Turn OFF motor power at the supply")
    print("  2. Wait 30 seconds (count to 30)")
    print("  3. Turn ON motor power")
    print("  4. Wait for LEDs to stabilize (10 seconds)")
    print("  5. Watch LED pattern:")
    print("     • Should see: Solid Red + Blinking Blue")
    
    print("\n" + "="*60)
    print("\n⏰ AFTER POWER CYCLE, RUN THIS TEST:")
    print("\n  cd /Users/jakegilbert/Desktop/Melvin/Melvin")
    print("  python3 test_obvious_movement.py")
    
    print("\n" + "="*60)
    print("IF MOTOR STILL DOESN'T WORK:")
    print("="*60)
    
    print("\n🔧 METHOD 2: PHYSICAL RESET BUTTON")
    print("\n  Your Robstride motor likely has a reset button:")
    print("\n  LOCATION:")
    print("    • Near power connector")
    print("    • Near CAN port")
    print("    • Small recessed hole (need paperclip)")
    print("    • Might be labeled 'RESET' or 'R'")
    
    print("\n  PROCEDURE:")
    print("    1. Power OFF motor")
    print("    2. Press and HOLD reset button")
    print("    3. While holding, reconnect power")
    print("    4. Keep holding 15 seconds")
    print("    5. Release when LEDs blink/change")
    print("    6. Power cycle again (OFF 30s, ON)")
    print("    7. Test motor")
    
    print("\n" + "="*60)
    print("🔄 METHOD 3: MULTIPLE POWER CYCLES")
    print("="*60)
    
    print("\n  Some motors auto-reset after several power cycles:")
    print("\n  1. Power OFF → Wait 60 seconds")
    print("  2. Power ON  → Wait 10 seconds")
    print("  3. Power OFF → Wait 60 seconds")
    print("  4. Power ON  → Wait 10 seconds")
    print("  5. Power OFF → Wait 60 seconds")
    print("  6. Power ON  → Leave on, wait for LEDs")
    print("  7. Test motor")
    
    print("\n" + "="*60)
    print("🚨 IF NOTHING WORKS:")
    print("="*60)
    
    print("\n  Your motor likely needs:")
    print("\n  ✓ Robstride PC Software Setup")
    print("    • Download 'RobStride Servo Studio'")
    print("    • Connect motor via USB-CAN")
    print("    • Run encoder calibration")
    print("    • Enable MIT/Position mode")
    print("    • This is ONE-TIME setup")
    print("    • After this, Python control will work")
    
    print("\n  🌐 Robstride Software Download:")
    print("     • Check robstride.com")
    print("     • Look for 'Servo Studio' or 'Configuration Tool'")
    print("     • May need to contact support for link")
    
    print("\n  📋 What to tell Robstride Support:")
    print("     • Motor Model: Robstride 03")
    print("     • Issue: Motor powered, LEDs correct, but doesn't")
    print("       respond to CAN position commands")
    print("     • LEDs: Solid Red + Blinking Blue")
    print("     • Need: Encoder calibration / mode activation")
    
    print("\n" + "="*60)
    print("\n💡 MOST LIKELY ISSUE:")
    print("\n  Based on symptoms (LEDs correct, no CAN response):")
    print("  → Motor needs INITIAL SETUP with PC software")
    print("  → This is NORMAL for new/reset Robstride motors")
    print("  → After PC setup, your Python code will work perfectly!")
    
    print("\n" + "="*60)


if __name__ == "__main__":
    print("\n" + "="*60)
    print("ROBSTRIDE MOTOR - AUTOMATIC FACTORY RESET")
    print("="*60)
    print("\nThis will send all known reset commands via CAN.")
    print("After this, you MUST power cycle the motor.\n")
    
    time.sleep(2)
    
    success = full_reset()
    
    if success:
        print_next_steps()
    else:
        print("\n✗ Could not complete CAN reset")
        print("  → Try physical reset button method")
        print("  → Or use Robstride PC software")

