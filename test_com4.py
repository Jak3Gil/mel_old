#!/usr/bin/env python3
import serial
import time

print("Testing COM4 (new cable port)...")
print()

try:
    ser = serial.Serial("COM4", baudrate=115200, timeout=1)
    print("[OK] COM4 opened successfully!")
    
    # Send enable command
    print("Sending CAN enable command...")
    can_frame = "t0018FFFFFFFFFFFFFFFC\r"
    ser.write(can_frame.encode())
    print(f"Sent: {can_frame.strip()}")
    
    time.sleep(0.3)
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting)
        print(f"[OK] Motor responded: {response.hex()}")
        print()
        print("==> NEW CABLE WORKS! CAN communication is GOOD!")
    else:
        print("[!] No response (but port opened)")
        print("    Cable works, check motor power/wiring")
    
    ser.close()
    
except Exception as e:
    print(f"[X] Error: {e}")

