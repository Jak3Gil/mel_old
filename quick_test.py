#!/usr/bin/env python3
import serial
import time
import glob

port = glob.glob('/dev/cu.usbserial*')[0]
print(f"Testing: {port}\n")

ser = serial.Serial(port, 115200, timeout=1.0)
time.sleep(0.3)

print("[1] Sending L91...")
ser.write(b'C\r')
time.sleep(0.1)
resp1 = ser.read(100)

ser.write(b'L91\r')
time.sleep(0.1)
resp2 = ser.read(100)

ser.write(b'O\r')
time.sleep(0.1)
resp3 = ser.read(100)

if resp1 or resp2 or resp3:
    print(f"✓ Adapter responding!")
    print(f"  C: {resp1.hex() if resp1 else 'none'}")
    print(f"  L91: {resp2.hex() if resp2 else 'none'}")
    
    print("\n[2] Testing motor enable...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    resp = ser.read(100)
    
    if resp:
        print(f"✓✓✓ MOTOR RESPONDING!")
        print(f"  Response: {resp.hex()}")
        print("\n🎉 SUCCESS! Ready to control motor!")
    else:
        print("✗ Motor not responding")
else:
    print("✗ Adapter still not responding")

ser.close()

