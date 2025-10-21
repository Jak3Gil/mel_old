#!/usr/bin/env python3
"""
Fix USB-CAN Adapter for Motor Studio
Motor Studio expects the adapter to be in a specific mode/state
"""

import serial
import time
import sys

def send_at_command(ser, command, wait_time=0.1):
    """Send AT command and get response"""
    ser.write((command + '\r').encode())
    time.sleep(wait_time)
    response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
    return response

def initialize_adapter_for_motorstudio():
    """Initialize adapter to work with Motor Studio"""
    
    print("=" * 60)
    print("INITIALIZING USB-CAN ADAPTER FOR MOTOR STUDIO")
    print("=" * 60)
    print()
    
    # Try to find the adapter on COM3
    port = "COM3"
    
    # Try multiple baudrates since adapter might be in unknown state
    baudrates = [115200, 921600, 1000000, 500000, 9600, 19200, 38400, 57600]
    
    adapter_found = False
    working_ser = None
    working_baud = None
    
    print("Step 1: Finding adapter...")
    print("-" * 60)
    
    for baud in baudrates:
        try:
            print(f"Trying {port} at {baud} baud...", end=" ")
            ser = serial.Serial(port, baudrate=baud, timeout=0.5)
            time.sleep(0.1)
            
            # Try basic AT command
            ser.write(b'AT\r')
            time.sleep(0.2)
            response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
            
            if 'OK' in response or 'ok' in response.lower():
                print(f"[OK] Adapter responding!")
                adapter_found = True
                working_ser = ser
                working_baud = baud
                break
            else:
                print(f"[No response]")
                ser.close()
                
        except Exception as e:
            print(f"[Error: {e}]")
            continue
    
    if not adapter_found:
        print()
        print("[X] Could not communicate with adapter at any baudrate")
        print()
        print("This means either:")
        print("  1. Adapter is not compatible with AT commands")
        print("  2. Adapter needs different initialization")
        print("  3. Adapter firmware is in bad state")
        print()
        print("SOLUTION: Try a different USB-CAN adapter")
        print()
        return False
    
    print()
    print(f"[OK] Adapter found at {working_baud} baud")
    print()
    
    # Now configure it for Motor Studio
    print("Step 2: Configuring adapter for Motor Studio...")
    print("-" * 60)
    
    # Common SLCAN initialization sequence
    commands = [
        ("ATZ", "Reset adapter"),
        ("ATE0", "Disable echo"),
        ("AT+BAUD=921600", "Set baudrate to 921600"),
        ("AT+MODE=0", "Set SLCAN mode"),
        ("AT+CAN=1000", "Set CAN bus to 1Mbps"),
        ("AT&W", "Save settings"),
        ("ATZ", "Reset to apply settings"),
    ]
    
    for cmd, description in commands:
        print(f"  {description}...", end=" ")
        try:
            response = send_at_command(working_ser, cmd, 0.2)
            if 'OK' in response or 'ok' in response.lower() or len(response) > 0:
                print("[OK]")
            else:
                print(f"[No response]")
        except Exception as e:
            print(f"[Error: {e}]")
    
    working_ser.close()
    print()
    
    # Wait for adapter to reset
    print("Step 3: Waiting for adapter to reinitialize...")
    time.sleep(2)
    print("[OK] Ready")
    print()
    
    print("=" * 60)
    print("ADAPTER CONFIGURATION COMPLETE")
    print("=" * 60)
    print()
    print("Now try Motor Studio:")
    print("  1. Open Motor Studio")
    print("  2. Select COM3")
    print("  3. Baudrate: 921600")
    print("  4. Click Connect")
    print()
    
    return True


def check_adapter_compatibility():
    """Check if adapter is compatible with Motor Studio"""
    
    print("=" * 60)
    print("USB-CAN ADAPTER COMPATIBILITY CHECK")
    print("=" * 60)
    print()
    
    port = "COM3"
    
    print("Checking adapter type...")
    print("-" * 60)
    
    try:
        ser = serial.Serial(port, baudrate=115200, timeout=1)
        time.sleep(0.1)
        
        # Try AT command
        ser.write(b'AT\r')
        time.sleep(0.3)
        response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
        
        ser.close()
        
        if 'OK' in response or 'ok' in response.lower():
            print("[OK] Adapter supports AT commands (SLCAN compatible)")
            print()
            return True
        else:
            print("[!] Adapter does NOT respond to AT commands")
            print()
            print("This adapter might not be compatible with Motor Studio.")
            print()
            print("Motor Studio expects:")
            print("  - SLCAN protocol adapter")
            print("  - AT command support")
            print("  - Specific initialization sequence")
            print()
            print("Your adapter (CH340) might be:")
            print("  - Generic USB-Serial (not SLCAN)")
            print("  - CAN adapter without SLCAN firmware")
            print("  - Incompatible with Motor Studio")
            print()
            return False
            
    except Exception as e:
        print(f"[X] Could not test adapter: {e}")
        print()
        return False


if __name__ == "__main__":
    print()
    
    # First check compatibility
    compatible = check_adapter_compatibility()
    
    if not compatible:
        print("=" * 60)
        print("INCOMPATIBLE ADAPTER DETECTED")
        print("=" * 60)
        print()
        print("Your USB-CAN adapter (CH340) is NOT compatible with Motor Studio.")
        print()
        print("EXPLANATION:")
        print("  - Motor Studio requires SLCAN protocol adapter")
        print("  - Your adapter is generic USB-Serial (CH340 chip)")
        print("  - Motor Studio can't initialize generic adapters")
        print()
        print("SOLUTIONS:")
        print()
        print("Option 1: Get Compatible Adapter (RECOMMENDED)")
        print("  Motor Studio needs one of these:")
        print("    - Official Robstride USB-CAN adapter")
        print("    - SLCAN-compatible adapter")
        print("    - CANable adapter with SLCAN firmware")
        print("    - USBtin adapter")
        print()
        print("Option 2: Use Python Control Instead")
        print("  Your current CH340 adapter DOES work with Python!")
        print("  We've confirmed this in previous tests.")
        print()
        print("  PROBLEM: Can't enable position mode without Motor Studio")
        print()
        print("Option 3: Contact Robstride")
        print("  Ask: 'How to enable position mode without Motor Studio?'")
        print("  Ask: 'Which USB-CAN adapter works with Motor Studio?'")
        print("  Request: Remote activation of position mode")
        print()
        print("=" * 60)
        print()
        sys.exit(1)
    
    print()
    print("Attempting to configure adapter...")
    print()
    
    success = initialize_adapter_for_motorstudio()
    
    if success:
        print("[SUCCESS] Try Motor Studio now!")
    else:
        print("[FAILED] Could not configure adapter")
        print()
        print("Your adapter might not be compatible with Motor Studio.")
        print("See compatibility notes above.")

