#!/usr/bin/env python3
"""
Robstride Motor Factory Reset - Multiple Methods
"""

import serial
import time
import glob

def factory_reset_via_can():
    """Try factory reset via CAN commands."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ROBSTRIDE MOTOR FACTORY RESET VIA CAN               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.3)
    
    # Initialize
    print("[1] Initializing with L91...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    
    motor_id = 0x01
    
    print("\n[2] Trying CAN Reset Commands...")
    print("="*60)
    
    # Try different reset commands
    reset_commands = [
        (b't0018AA55AA55AA55AA55\r', "Magic reset sequence 1"),
        (b't00185555AAAA5555AAAA\r', "Magic reset sequence 2"),
        (b'T0A0000010\r', "Extended reset command (0x0A)"),
        (b'T0C0000010\r', "Extended clear faults (0x0C)"),
        (b't0018FFFFFFFFFFFFFF00\r', "Clear all command"),
        (b't00180000000000000000\r', "Zero all registers"),
    ]
    
    for cmd, desc in reset_commands:
        print(f"\n  {desc}:")
        print(f"    Sending: {cmd.strip()}")
        ser.write(cmd)
        time.sleep(0.3)
        
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    Response: {resp.hex()}")
        else:
            print(f"    (no response)")
    
    # Try calibration command
    print("\n[3] Trying Encoder Calibration Command...")
    ser.write(b't0018FFFFFFFFFFFF0006\r')  # Calibrate command
    time.sleep(0.5)
    
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"  Response: {resp.hex()}")
    
    ser.close()
    
    print("\n" + "="*60)
    print("CAN RESET ATTEMPTS COMPLETE")
    print("="*60)
    print("\nNow power cycle the motor and test again.")

def print_physical_reset_instructions():
    """Print instructions for physical reset."""
    print("\n\n╔════════════════════════════════════════════════════════╗")
    print("║   PHYSICAL RESET BUTTON METHOD                        ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    print("STEPS:")
    print("-"*60)
    print("\n1. POWER OFF the motor completely")
    print("   - Disconnect power supply\n")
    
    print("2. LOCATE the reset button:")
    print("   - Look for small button/hole on motor body")
    print("   - Usually near power connector or CAN port")
    print("   - Might be labeled 'RESET' or 'R'")
    print("   - May need paperclip/pin to press if recessed\n")
    
    print("3. PRESS AND HOLD reset button:")
    print("   - Hold for 10-15 seconds")
    print("   - Keep holding while reconnecting power")
    print("   - Watch LEDs for changes\n")
    
    print("4. OBSERVE LED pattern:")
    print("   - LEDs should blink or change pattern")
    print("   - This indicates reset in progress\n")
    
    print("5. RELEASE button when:")
    print("   - LEDs finish blinking")
    print("   - Or after 15 seconds\n")
    
    print("6. POWER CYCLE:")
    print("   - Turn OFF motor power")
    print("   - Wait 10 seconds")
    print("   - Turn ON motor power")
    print("   - Wait for LEDs to stabilize\n")
    
    print("7. TEST:")
    print("   - Run: python3 robstride_motor.py")
    print("   - Motor should now respond to commands\n")
    
    print("="*60)
    print("ALTERNATIVE: Power Cycle Multiple Times")
    print("="*60)
    print("\nSome motors reset after multiple power cycles:")
    print("  1. Power OFF")
    print("  2. Wait 30 seconds")
    print("  3. Power ON for 5 seconds")
    print("  4. Power OFF")
    print("  5. Wait 30 seconds")
    print("  6. Power ON for 5 seconds")
    print("  7. Repeat 3-4 times")
    print("  8. Then leave powered ON")

def main():
    """Main reset procedure."""
    print("\n" + "="*60)
    print("ROBSTRIDE MOTOR FACTORY RESET")
    print("="*60)
    print("\nWe'll try TWO methods:\n")
    print("  METHOD 1: CAN command reset (try first)")
    print("  METHOD 2: Physical reset button\n")
    
    choice = input("Try CAN reset first? (y/n): ").lower()
    
    if choice == 'y':
        try:
            factory_reset_via_can()
            
            print("\n\n🔄 NOW POWER CYCLE THE MOTOR:")
            print("  1. Turn OFF motor power")
            print("  2. Wait 30 seconds")
            print("  3. Turn ON motor power")
            print("  4. Wait for LEDs to stabilize")
            
            input("\nPress Enter after power cycle...")
            
            print("\nTesting motor...")
            import subprocess
            result = subprocess.run(['python3', 'robstride_motor.py'], 
                                  capture_output=True, text=True)
            print(result.stdout)
            
            if "MOTOR TEST SUCCESSFUL" in result.stdout:
                print("\n🎉 SUCCESS! Motor reset worked!")
            else:
                print("\n⚠️  Still not working. Try physical reset button...")
                print_physical_reset_instructions()
                
        except Exception as e:
            print(f"\nError: {e}")
            print("\nTry physical reset button method...")
            print_physical_reset_instructions()
    else:
        print_physical_reset_instructions()

if __name__ == "__main__":
    main()

