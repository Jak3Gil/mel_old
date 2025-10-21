#!/usr/bin/env python3
"""
Diagnose motor responses on COM3
Check if motor is responding to commands
"""

import serial
import time

def diagnose_motor():
    """Connect and check motor responses."""
    print("=" * 60)
    print("   MOTOR DIAGNOSTIC - CAN ID 13 - COM3")
    print("=" * 60 + "\n")
    
    port = 'COM3'
    motor_id = 0x0D  # CAN ID 13
    
    try:
        ser = serial.Serial(port, 115200, timeout=1.0)
        time.sleep(0.3)
        
        print("[1] Clearing buffers...")
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        print("[2] Initializing L91...")
        ser.write(b'C\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    C response: {resp}")
        
        ser.write(b'L91\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    L91 response: {resp}")
        
        ser.write(b'O\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        print(f"    O response: {resp}")
        print()
        
        print("[3] Sending ENABLE command and listening for response...")
        enable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFC\r"
        print(f"    Sending: {enable_msg.strip()}")
        ser.write(enable_msg.encode())
        
        # Wait and collect ALL responses
        time.sleep(0.5)
        all_data = b""
        while ser.in_waiting > 0:
            all_data += ser.read(ser.in_waiting)
            time.sleep(0.05)
        
        if all_data:
            print(f"    Raw bytes: {all_data}")
            print(f"    ASCII: {all_data.decode('ascii', errors='ignore')}")
            print(f"    Length: {len(all_data)} bytes")
        else:
            print("    [WARNING] NO RESPONSE from motor!")
        print()
        
        print("[4] Sending ZERO POSITION command...")
        zero_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFE\r"
        print(f"    Sending: {zero_msg.strip()}")
        ser.write(zero_msg.encode())
        
        time.sleep(0.5)
        all_data = b""
        while ser.in_waiting > 0:
            all_data += ser.read(ser.in_waiting)
            time.sleep(0.05)
        
        if all_data:
            print(f"    Raw bytes: {all_data}")
            print(f"    ASCII: {all_data.decode('ascii', errors='ignore')}")
        else:
            print("    [WARNING] NO RESPONSE from motor!")
        print()
        
        print("[5] Sending POSITION command (1.0 radian)...")
        # Pack a simple position command
        position = 1.0
        # Convert to uint16
        p_min, p_max = -12.5, 12.5
        p_int = int((position - p_min) * 65535 / (p_max - p_min))
        
        # Simple command: position only, zero everything else
        data = bytearray(8)
        data[0] = (p_int >> 8) & 0xFF
        data[1] = p_int & 0xFF
        # Rest zeros (which encodes to specific Kp/Kd values)
        
        msg = f"t{motor_id:03X}8{data.hex().upper()}\r"
        print(f"    Sending: {msg.strip()}")
        
        # Send command multiple times
        for i in range(10):
            ser.write(msg.encode())
            time.sleep(0.02)
        
        # Check for response
        time.sleep(0.3)
        all_data = b""
        while ser.in_waiting > 0:
            all_data += ser.read(ser.in_waiting)
            time.sleep(0.05)
        
        if all_data:
            print(f"    Response count: {len(all_data)} bytes")
            print(f"    Sample: {all_data[:100]}")
            
            # Try to decode motor feedback
            ascii_data = all_data.decode('ascii', errors='ignore')
            print(f"    ASCII: {ascii_data[:200]}")
            
            # Look for 't' frames (motor responses)
            if 't' in ascii_data:
                print("    [OK] Motor is responding!")
            else:
                print("    [WARNING] No 't' frames in response")
        else:
            print("    [WARNING] NO RESPONSE from motor!")
        print()
        
        print("[6] Trying different CAN IDs to see if motor responds...")
        for test_id in [0x01, 0x0D, 0x0F, 13, 15]:
            enable_msg = f"t{test_id:03X}8FFFFFFFFFFFFFFFC\r"
            print(f"    Testing ID {test_id} (0x{test_id:02X}): {enable_msg.strip()}", end="")
            ser.write(enable_msg.encode())
            time.sleep(0.2)
            
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                print(f" -> GOT RESPONSE: {resp[:50]}")
            else:
                print(" -> No response")
        print()
        
        print("[7] Disabling motor...")
        disable_msg = f"t{motor_id:03X}8FFFFFFFFFFFFFFFD\r"
        ser.write(disable_msg.encode())
        time.sleep(0.2)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp}")
        
        ser.close()
        
        print("\n" + "=" * 60)
        print("DIAGNOSIS COMPLETE")
        print("=" * 60)
        
        print("\nWhat did we learn?")
        print("  - If you saw responses: Motor is listening!")
        print("  - If NO responses: Motor might be:")
        print("    * In wrong mode (needs Robstride software to configure)")
        print("    * Using different protocol")
        print("    * On different CAN ID than 13")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    diagnose_motor()

