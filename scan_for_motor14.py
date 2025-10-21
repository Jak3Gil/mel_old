#!/usr/bin/env python3
"""
Scan to find Motor 14's correct CAN ID encoding
Systematically test all encodings in the scan range
"""

import serial
import time

def scan_for_motors():
    """Scan CAN bus for all motors and their encodings."""
    print("=" * 70)
    print("   SCANNING FOR ALL MOTORS")
    print("=" * 70 + "\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    try:
        # Connect
        print(f"Connecting to {PORT} at {BAUD} baud...")
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("[OK] Connected\n")
        
        # AT+AT handshake
        print("Initializing...")
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        resp = ser.read(1000)
        print(f"Handshake: {resp.decode('ascii', errors='ignore').strip()}\n")
        
        print("=" * 70)
        print("SCANNING FOR MOTORS")
        print("=" * 70)
        print("\nTesting encodings in the e8/e9/ea/eb range...\n")
        
        found_motors = []
        
        # Scan the full range like Robstride software does
        # From the logs, we saw: e8 0c, e8 4c, e8 8c, e8 cc, e9 0c, etc.
        # Pattern: increments of 0x40 in low byte
        
        for high_byte in [0xe8, 0xe9, 0xea, 0xeb]:
            for low_byte_base in [0x0c, 0x4c, 0x8c, 0xcc]:
                for offset in range(0, 64, 4):  # Try small variations
                    low_byte = low_byte_base + offset
                    if low_byte > 0xff:
                        continue
                    
                    # Build scan command
                    scan_cmd = bytes([
                        0x41, 0x54, 0x00, 0x07, 
                        high_byte, low_byte, 
                        0x01, 0x00, 0x0d, 0x0a
                    ])
                    
                    # Send
                    ser.reset_input_buffer()
                    ser.write(scan_cmd)
                    time.sleep(0.15)  # Longer wait for response
                    
                    # Check response
                    if ser.in_waiting > 0:
                        response = ser.read(ser.in_waiting)
                        
                        # Parse response to get MCU ID and CAN ID
                        if len(response) >= 15:
                            # Look for MCU ID pattern
                            if b'\x3c\x3d\x30\x20\x9c\x23\x37\x11' in response:
                                # This is motor 13 (we know this MCU ID)
                                print(f"  {high_byte:02x} {low_byte:02x} -> Motor 13 (MCU: 1137239c20303d3c)")
                                found_motors.append({
                                    'encoding': f'{high_byte:02x} {low_byte:02x}',
                                    'can_id': 13,
                                    'response': response.hex()
                                })
                            else:
                                # Different motor!
                                print(f"  {high_byte:02x} {low_byte:02x} -> FOUND DIFFERENT MOTOR!")
                                print(f"      Response: {response.hex()}")
                                
                                # Try to extract CAN ID from response
                                # Response format: AT \x00\x00 [data] [MCU_ID]
                                found_motors.append({
                                    'encoding': f'{high_byte:02x} {low_byte:02x}',
                                    'can_id': '?',
                                    'response': response.hex()
                                })
        
        ser.close()
        
        # Results
        print("\n" + "=" * 70)
        print("SCAN RESULTS")
        print("=" * 70)
        
        if found_motors:
            print(f"\nFound {len(found_motors)} motor(s):\n")
            for i, motor in enumerate(found_motors, 1):
                print(f"  [{i}] Encoding: {motor['encoding']}")
                print(f"      CAN ID: {motor['can_id']}")
                print(f"      Response: {motor['response'][:40]}...")
                print()
            
            # Ask user which one to test
            if len(found_motors) > 1:
                print("Multiple motors found!")
                print("\nTo identify which is motor 14:")
                print("  Run this script and watch which motor moves")
                
        else:
            print("\nNo motors found in scan range!")
            print("\nMake sure:")
            print("  - Motor 14 is powered on")
            print("  - Motor 14 is on CAN bus")
            print("  - CANH/CANL connected")
        
        return found_motors
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        return []


def test_encoding(encoding_str):
    """Test a specific encoding to see which motor moves."""
    high, low = [int(x, 16) for x in encoding_str.split()]
    encoding = bytes([high, low])
    
    print(f"\n{'='*70}")
    print(f"TESTING ENCODING: {encoding_str}")
    print(f"{'='*70}\n")
    
    PORT = 'COM3'
    BAUD = 921600
    
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
        time.sleep(0.5)
        ser.reset_input_buffer()
        
        # Init
        ser.write(bytes([0x41, 0x54, 0x2b, 0x41, 0x54, 0x0d, 0x0a]))
        time.sleep(0.3)
        ser.read(1000)
        
        # Scan
        scan_cmd = bytes([0x41, 0x54, 0x00, 0x07, high, low, 0x01, 0x00, 0x0d, 0x0a])
        ser.write(scan_cmd)
        time.sleep(0.5)
        ser.read(1000)
        
        print("Watch the motors carefully!")
        print("Press Ctrl+C if you see which one moves\n")
        
        # Forward movement
        print("Moving FORWARD for 2 seconds...")
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, high, low, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x83, 0xe0,
            0x0d, 0x0a
        ])
        ser.write(jog_cmd)
        time.sleep(2)
        
        # Stop
        stop_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, high, low, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x00, 0x7f, 0xff,
            0x0d, 0x0a
        ])
        ser.write(stop_cmd)
        print("[STOPPED]\n")
        
        time.sleep(1)
        
        # Reverse movement
        print("Moving REVERSE for 2 seconds...")
        jog_cmd = bytes([
            0x41, 0x54, 0x90, 0x07, high, low, 0x08,
            0x05, 0x70, 0x00, 0x00, 0x07, 0x01, 0x7c, 0x1e,
            0x0d, 0x0a
        ])
        ser.write(jog_cmd)
        time.sleep(2)
        
        ser.write(stop_cmd)
        print("[STOPPED]\n")
        
        ser.close()
        
        # Ask user which motor moved
        response = input("Which motor moved? (13/14/none): ").strip()
        
        if response == '14':
            print(f"\n✓ Motor 14 encoding found: {encoding_str}")
            
            # Save to file
            with open('motor14_encoding.txt', 'w') as f:
                f.write(f"Motor 14 CAN ID encoding: {encoding_str}\n")
                f.write(f"High byte: 0x{high:02x}\n")
                f.write(f"Low byte: 0x{low:02x}\n")
            
            print(f"  Saved to: motor14_encoding.txt")
        elif response == '13':
            print(f"\n✗ This encoding controls motor 13, not 14")
        else:
            print(f"\n✗ No motor moved with this encoding")
        
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        ser.write(stop_cmd)
        ser.close()
    except Exception as e:
        print(f"\n[ERROR] {e}")


if __name__ == "__main__":
    print("\nRobstride Motor Scanner - Find Motor 14\n")
    
    print("This will scan for all motors on the CAN bus")
    print("and help identify which encoding controls motor 14.\n")
    
    input("Press ENTER to start scan...")
    
    found = scan_for_motors()
    
    if found and len(found) > 0:
        print("\n" + "=" * 70)
        print("TESTING EACH ENCODING")
        print("=" * 70)
        print("\nWe'll test each encoding to see which motor moves.")
        print("Watch your motors carefully!\n")
        
        for motor in found:
            response = input(f"\nTest encoding {motor['encoding']}? (y/n): ").strip()
            if response.lower() == 'y':
                test_encoding(motor['encoding'])

