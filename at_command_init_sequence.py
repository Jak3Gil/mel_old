#!/usr/bin/env python3
"""
Robstride AT Command - Specific Init Sequence
Based on actual AT responses, try specific sequence
"""

import serial
import time
import glob


def at_init_sequence():
    """Send specific AT command initialization sequence."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ROBSTRIDE AT COMMAND INITIALIZATION SEQUENCE         ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No adapter found")
        return
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.5)
    
    print("="*60)
    print("STEP 1: AT MODE INITIALIZATION")
    print("="*60 + "\n")
    
    # Specific initialization sequence
    init_sequence = [
        ("ATZ", "Reset device", 1.0),
        ("ATE0", "Echo off", 0.3),
        ("AT&F", "Factory defaults", 0.5),
        ("AT+MODE=1", "Set mode 1 (Position)", 0.5),
        ("AT+POSMODE", "Enable position mode", 0.5),
        ("AT+MIT MODE", "Enable MIT mode", 0.5),
        ("AT+ENABLE=1", "Enable motor control", 0.5),
        ("AT+MOTOR=1", "Enable motor", 0.5),
        ("AT+CTRL=1", "Control mode 1", 0.5),
        ("AT+CFG=POS", "Config position", 0.5),
        ("AT+INIT", "Initialize", 0.5),
        ("AT+CAL", "Calibrate", 1.0),
        ("AT+SAVE", "Save configuration", 0.5),
        ("AT&W", "Write to EEPROM", 0.5),
        ("AT+APPLY", "Apply settings", 0.5),
        ("AT+START", "Start motor", 0.5),
    ]
    
    for i, (cmd, desc, wait) in enumerate(init_sequence, 1):
        print(f"[{i}/{len(init_sequence)}] {desc}")
        print(f"     Command: {cmd}")
        
        ser.write((cmd + '\r').encode())
        time.sleep(wait)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"     Response: {resp.hex()}")
        else:
            print(f"     (no response)")
        print()
    
    ser.close()
    
    print("\n" + "="*60)
    print("AT INITIALIZATION COMPLETE")
    print("="*60)
    
    print("\n⚡ CRITICAL NEXT STEP:")
    print("\n  POWER CYCLE THE MOTOR NOW!")
    print("\n  1. Turn OFF 12V power supply")
    print("  2. Wait 10 seconds")
    print("  3. Turn ON 12V power supply")
    print("  4. Wait for LEDs to stabilize (10 seconds)")
    print("\n  AT command settings often require power cycle to activate!")


def test_after_power_cycle():
    """Test motor after user power cycles it."""
    print("\n\n" + "="*60)
    print("TESTING MOTOR AFTER POWER CYCLE")
    print("="*60 + "\n")
    
    input("Press ENTER after you've power cycled the motor...")
    
    print("\nTesting motor movement...")
    import subprocess
    result = subprocess.run(
        ['python3', 'test_obvious_movement.py'],
        cwd='/Users/jakegilbert/Desktop/Melvin/Melvin'
    )
    
    return result.returncode == 0


def check_current_mode():
    """Try to query what mode motor is in."""
    print("\n\n" + "="*60)
    print("QUERYING MOTOR MODE")
    print("="*60 + "\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        return
    
    port = ports[0]
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.5)
    
    query_commands = [
        "AT+MODE?",
        "AT+STATUS?",
        "AT+INFO?",
        "AT+CFG?",
        "ATI",
        "AT&V",
    ]
    
    print("Querying motor status:\n")
    
    for cmd in query_commands:
        print(f"  {cmd}...")
        ser.write((cmd + '\r').encode())
        time.sleep(0.5)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            # Try to decode
            try:
                decoded = resp.decode('ascii', errors='ignore')
                print(f"     ASCII: {decoded}")
            except:
                pass
            print(f"     HEX: {resp.hex()}")
            print(f"     Bytes: {list(resp)}")
        print()
    
    ser.close()


if __name__ == "__main__":
    print("\n" + "="*60)
    print("ROBSTRIDE AT COMMAND INITIALIZATION")
    print("="*60)
    print("\nThis will send a specific AT command sequence to")
    print("try to enable position control mode.\n")
    
    print("NOTE: Motor responses were mostly 0xFF bytes, which")
    print("typically means commands are being received but either:")
    print("  • Settings will apply after power cycle")
    print("  • Motor is in wrong state to accept them")
    print("  • Different command syntax needed\n")
    
    time.sleep(2)
    
    try:
        # Send AT initialization
        at_init_sequence()
        
        # Try to query mode
        check_current_mode()
        
        # Prompt for power cycle and test
        test_result = test_after_power_cycle()
        
        print("\n\n" + "="*60)
        print("FINAL RESULT")
        print("="*60)
        
        if test_result:
            print("\n🎉 SUCCESS! Motor is now working!")
            print("\n  The AT command initialization worked!")
            print("  You can now use the K-Scale architecture for control.")
            print("\n  Test with:")
            print("    python3 example_kscale_usage.py 1")
        else:
            print("\n❌ Motor still not moving")
            print("\n  The AT commands didn't enable position mode.")
            print("\n  Possible reasons:")
            print("    1. AT commands are for adapter, not motor")
            print("    2. Motor uses different AT syntax")
            print("    3. Motor requires proprietary command sequence")
            print("\n  → Still need Robstride PC software")
            
            print("\n  📋 What to tell Robstride Support:")
            print("     • Motor responds to AT commands (got 0xFF responses)")
            print("     • Tried: AT+MODE=1, AT+POSMODE, AT+SAVE, etc.")
            print("     • Motor won't execute position commands")
            print("     • Ask: Correct AT command sequence for position mode")
        
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

