#!/usr/bin/env python3
"""
Full Motor Scan - Find ALL motors on CAN bus
Tests the complete scan range used by Robstride software
"""

import serial
import time

def full_scan():
    """Comprehensive scan for all motors."""
    print("=" * 70)
    print("   COMPREHENSIVE MOTOR SCAN")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    try:
        print(f"Connecting to {PORT} at {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("[OK] Connected\n")
        
        # Initialize
        print("Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        print("[OK]\n")
        
        print("=" * 70)
        print("SCANNING ALL POSSIBLE ENCODINGS")
        print("=" * 70)
        print("\nThis will take about 30 seconds...")
        print("Testing encodings: e8 0c through eb fc\n")
        
        found_motors = []
        test_count = 0
        
        # Full scan pattern from Robstride logs
        # e8 0c, e8 4c, e8 8c, e8 cc, e9 0c, e9 4c, etc.
        for high_byte in [0xe8, 0xe9, 0xea, 0xeb]:
            for low_byte in [0x0c, 0x4c, 0x8c, 0xcc,
                           0x14, 0x54, 0x94, 0xd4,
                           0x1c, 0x5c, 0x9c, 0xdc,
                           0x24, 0x64, 0xa4, 0xe4,
                           0x2c, 0x6c, 0xac, 0xec,
                           0x34, 0x74, 0xb4, 0xf4,
                           0x3c, 0x7c, 0xbc, 0xfc,
                           0x44, 0x84, 0xc4,
                           0x04]:  # All patterns from logs
                
                test_count += 1
                
                # Build scan command
                scan_cmd = bytes([
                    0x41, 0x54, 0x00, 0x07,
                    high_byte, low_byte,
                    0x01, 0x00, 0x0d, 0x0a
                ])
                
                # Send
                ser.reset_input_buffer()
                ser.write(scan_cmd)
                time.sleep(0.12)  # Wait for response
                
                # Check response
                if ser.in_waiting > 0:
                    response = ser.read(ser.in_waiting)
                    
                    # Extract MCU ID from response
                    # Format: AT \x00\x00 [header] [MCU_ID 8 bytes]
                    if len(response) >= 15:
                        # Try to find MCU ID (8 bytes)
                        mcu_id = None
                        for i in range(len(response) - 8):
                            if i >= 6:  # Skip header
                                potential_mcu = response[i:i+8]
                                # Check if it looks like an MCU ID (not all zeros)
                                if potential_mcu != b'\x00'*8:
                                    mcu_id = potential_mcu.hex()
                                    break
                        
                        # Check if this is motor 13 (known MCU ID)
                        motor_id = "?"
                        if b'\x3c\x3d\x30\x20\x9c\x23\x37\x11' in response:
                            motor_id = "13"
                        elif mcu_id and mcu_id != '0000000000000000':
                            motor_id = "14?"  # Potentially motor 14!
                        
                        print(f"  [{test_count:3d}] {high_byte:02x} {low_byte:02x} -> RESPONSE! Motor {motor_id}")
                        if mcu_id:
                            print(f"         MCU ID: {mcu_id}")
                        
                        found_motors.append({
                            'encoding': f'{high_byte:02x}{low_byte:02x}',
                            'high': high_byte,
                            'low': low_byte,
                            'motor_id': motor_id,
                            'mcu_id': mcu_id,
                            'response': response.hex()
                        })
                
                # Progress indicator
                if test_count % 20 == 0:
                    print(f"  ... tested {test_count} encodings")
        
        print(f"\n[OK] Scan complete - tested {test_count} encodings\n")
        
        ser.close()
        
        # Results
        print("=" * 70)
        print("SCAN RESULTS")
        print("=" * 70)
        
        if found_motors:
            print(f"\nFound {len(found_motors)} responding encoding(s):\n")
            
            motor13_encodings = []
            motor14_encodings = []
            unknown_encodings = []
            
            for motor in found_motors:
                if motor['motor_id'] == "13":
                    motor13_encodings.append(motor)
                elif motor['motor_id'] == "14?":
                    motor14_encodings.append(motor)
                else:
                    unknown_encodings.append(motor)
            
            if motor13_encodings:
                print(f"Motor 13 found at {len(motor13_encodings)} encoding(s):")
                for m in motor13_encodings:
                    print(f"  • {m['encoding']} - MCU: {m['mcu_id']}")
                print()
            
            if motor14_encodings:
                print(f"*** POSSIBLE MOTOR 14 found at {len(motor14_encodings)} encoding(s): ***")
                for m in motor14_encodings:
                    print(f"  • {m['encoding']} - MCU: {m['mcu_id']}")
                print()
            
            if unknown_encodings:
                print(f"Unknown motor(s) at {len(unknown_encodings)} encoding(s):")
                for m in unknown_encodings:
                    print(f"  • {m['encoding']}")
                print()
            
            # If only motor 13 found
            if motor14_encodings == [] and unknown_encodings == []:
                print("⚠️  ONLY MOTOR 13 DETECTED")
                print("\nThis means either:")
                print("  1. Motor 14 is not powered")
                print("  2. Motor 14 is not on the CAN bus")
                print("  3. Motor 14 is on a different COM port")
                print("  4. You only have one motor")
                print("\nCheck:")
                print("  - Does Robstride software show 2 motors?")
                print("  - Is Motor 14 power LED on?")
                print("  - Are both motors on same CAN bus?")
            
            return found_motors
            
        else:
            print("\n⚠️  NO MOTORS FOUND!")
            print("\nCheck:")
            print("  - Motor power")
            print("  - CAN wiring (CANH/CANL)")
            print("  - Correct COM port")
            print("  - Motor works with Robstride software")
            
            return []
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return []


if __name__ == "__main__":
    print("\nFull Motor Scanner\n")
    print("This will scan the complete range of CAN encodings")
    print("used by Robstride software to find all motors.\n")
    
    input("Press ENTER to start scan...")
    print()
    
    motors = full_scan()
    
    if motors:
        # Check if multiple motors found
        unique_mcus = set(m['mcu_id'] for m in motors if m['mcu_id'])
        
        print("\n" + "=" * 70)
        print("SUMMARY")
        print("=" * 70)
        print(f"\nUnique motors detected: {len(unique_mcus)}")
        
        if len(unique_mcus) == 1:
            print("\n⚠️  Only ONE motor detected on CAN bus")
            print("If you have two motors, check the second motor's power and wiring")
        elif len(unique_mcus) >= 2:
            print("\n✓ Multiple motors detected!")
            print("Motor 14 is present on the CAN bus")

