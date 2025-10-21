#!/usr/bin/env python3
"""
Diagnose Official Robstride USB-CAN Adapter
Find out why Motor Studio can't initialize it
"""

import serial
import time
import sys

def test_robstride_adapter():
    """Test the official Robstride adapter"""
    
    print("=" * 60)
    print("OFFICIAL ROBSTRIDE USB-CAN ADAPTER DIAGNOSTIC")
    print("=" * 60)
    print()
    
    port = "COM3"
    
    # Try different baudrates that Robstride might use
    test_bauds = [115200, 921600, 1000000, 500000, 460800, 230400]
    
    print("Testing adapter at different baudrates...")
    print("-" * 60)
    
    for baud in test_bauds:
        print(f"\nTesting {baud} baud:")
        print("-" * 40)
        
        try:
            ser = serial.Serial(port, baudrate=baud, timeout=0.5)
            time.sleep(0.2)
            
            # Clear any existing data
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Test 1: Simple AT
            print("  Sending: AT", end=" -> ")
            ser.write(b'AT\r\n')
            time.sleep(0.2)
            response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore').strip()
            print(f"Response: '{response}'" if response else "No response")
            
            # Test 2: AT with just \r
            ser.reset_input_buffer()
            print("  Sending: AT\\r", end=" -> ")
            ser.write(b'AT\r')
            time.sleep(0.2)
            response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore').strip()
            print(f"Response: '{response}'" if response else "No response")
            
            # Test 3: SLCAN open command
            ser.reset_input_buffer()
            print("  Sending: O (SLCAN open)", end=" -> ")
            ser.write(b'O\r')
            time.sleep(0.2)
            response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore').strip()
            print(f"Response: '{response}'" if response else "No response")
            
            # Test 4: Version command
            ser.reset_input_buffer()
            print("  Sending: V (version)", end=" -> ")
            ser.write(b'V\r')
            time.sleep(0.2)
            response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore').strip()
            print(f"Response: '{response}'" if response else "No response")
            
            # Test 5: Read any data that might be coming
            time.sleep(0.5)
            if ser.in_waiting > 0:
                extra = ser.read(ser.in_waiting).decode('latin-1', errors='ignore').strip()
                print(f"  Extra data: '{extra}'")
            
            ser.close()
            
        except Exception as e:
            print(f"  Error: {e}")
    
    print()
    print("=" * 60)
    print()
    
    # Now test raw CAN communication (what works with Python)
    print("Testing RAW CAN communication (L91 protocol)...")
    print("-" * 60)
    
    try:
        # Try 115200 which is common for SLCAN/L91
        ser = serial.Serial(port, baudrate=115200, timeout=0.5)
        time.sleep(0.2)
        
        print("  Sending CAN frame in L91 format...")
        # Enable motor command: t 001 8 FF FF FF FF FF FF FF FC
        can_frame = "t0018FFFFFFFFFFFFFFFC\r"
        ser.write(can_frame.encode())
        time.sleep(0.1)
        
        response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
        print(f"  Response: '{response}'" if response else "  No response")
        
        # Try receiving
        time.sleep(0.5)
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
            print(f"  Received data: '{data}'")
        
        ser.close()
        
    except Exception as e:
        print(f"  Error: {e}")
    
    print()
    print("=" * 60)
    print("ANALYSIS")
    print("=" * 60)
    print()


def check_adapter_state():
    """Check what state the adapter is in"""
    
    print("Checking adapter current state...")
    print("-" * 60)
    
    port = "COM3"
    
    try:
        # Open at 115200 (most common)
        ser = serial.Serial(port, baudrate=115200, timeout=1)
        time.sleep(0.5)
        
        # Read any data that's already coming
        print("\nListening for 2 seconds to see if adapter is sending data...")
        time.sleep(2)
        
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            print(f"\n[!] Adapter is sending data unprompted:")
            print(f"    Raw bytes: {data.hex()}")
            print(f"    As text: {data.decode('latin-1', errors='ignore')}")
            print("\n    This might mean adapter is in wrong mode or stuck.")
        else:
            print("\n[OK] No unprompted data (adapter is quiet)")
        
        # Try reset command
        print("\nTrying reset commands...")
        
        commands = [
            (b'ATZ\r', "ATZ (AT reset)"),
            (b'C\r', "C (SLCAN close)"),
            (b'\x00', "NULL byte"),
            (b'\r\n', "CRLF"),
        ]
        
        for cmd, desc in commands:
            ser.reset_input_buffer()
            print(f"  Sending: {desc}", end=" -> ")
            ser.write(cmd)
            time.sleep(0.3)
            
            if ser.in_waiting > 0:
                response = ser.read(ser.in_waiting).decode('latin-1', errors='ignore')
                print(f"Response: '{response}'")
            else:
                print("No response")
        
        ser.close()
        
    except Exception as e:
        print(f"Error: {e}")
    
    print()


if __name__ == "__main__":
    print()
    test_robstride_adapter()
    print()
    check_adapter_state()
    print()
    
    print("=" * 60)
    print("CONCLUSIONS")
    print("=" * 60)
    print()
    print("If adapter doesn't respond to any commands:")
    print("  1. Adapter firmware might be corrupted")
    print("  2. Adapter might be in wrong mode")
    print("  3. Driver issue")
    print("  4. Adapter hardware failure")
    print()
    print("NEXT STEPS:")
    print("  1. Try adapter on different USB port")
    print("  2. Restart computer")
    print("  3. Try adapter on different computer")
    print("  4. Contact Robstride for adapter firmware update")
    print()

