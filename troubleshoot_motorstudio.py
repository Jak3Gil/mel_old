#!/usr/bin/env python3
"""
Motor Studio Connection Troubleshooter
Helps diagnose why Motor Studio won't connect
"""

import serial.tools.list_ports
import subprocess
import sys
import io

# Fix Windows console encoding
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

print("=" * 60)
print("MOTOR STUDIO CONNECTION TROUBLESHOOTER")
print("=" * 60)
print()

# Step 1: Check available COM ports
print("1. CHECKING AVAILABLE COM PORTS")
print("-" * 60)
ports = serial.tools.list_ports.comports()
if not ports:
    print("[X] NO COM PORTS FOUND!")
    print("    -> Check USB-CAN adapter is plugged in")
    print()
else:
    print(f"[OK] Found {len(ports)} COM port(s):")
    for port in ports:
        print(f"   * {port.device}")
        print(f"     Description: {port.description}")
        print(f"     Hardware ID: {port.hwid}")
        print()
    
    # Check if COM3 (from config) exists
    port_devices = [p.device for p in ports]
    if "COM3" in port_devices:
        print("[OK] COM3 (configured in Motor Studio) is available")
    else:
        print("[!] WARNING: COM3 not found, but Motor Studio is configured for COM3")
        if port_devices:
            print(f"    -> Try changing Motor Studio to use: {port_devices[0]}")
    print()

# Step 2: Check for processes using serial ports
print("2. CHECKING FOR PROCESSES USING SERIAL PORTS")
print("-" * 60)
try:
    # Check for Python processes
    result = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq python.exe'], 
                          capture_output=True, text=True, timeout=5)
    python_procs = [line for line in result.stdout.split('\n') if 'python.exe' in line.lower()]
    
    if python_procs:
        print(f"[!] WARNING: Found {len(python_procs)} Python process(es) running:")
        for proc in python_procs:
            print(f"   {proc.strip()}")
        print()
        print("   ACTION NEEDED:")
        print("   -> Close any Python scripts that might be using the motor")
        print("   -> Or run: taskkill /F /IM python.exe")
    else:
        print("[OK] No Python processes found using ports")
    print()
except Exception as e:
    print(f"[!] Could not check processes: {e}")
    print()

# Step 3: Test COM port accessibility
print("3. TESTING COM PORT ACCESSIBILITY")
print("-" * 60)
for port in ports:
    try:
        # Try to open the port briefly
        ser = serial.Serial(port.device, baudrate=921600, timeout=0.1)
        ser.close()
        print(f"[OK] {port.device} - Available (not in use)")
    except serial.SerialException as e:
        if "Access is denied" in str(e) or "PermissionError" in str(e):
            print(f"[X] {port.device} - IN USE by another program!")
            print(f"    -> Close other programs using this port")
        else:
            print(f"[!] {port.device} - {e}")
print()

# Step 4: Check Motor Studio config
print("4. MOTOR STUDIO CONFIGURATION")
print("-" * 60)
config_path = r"C:\Users\Owner\Downloads\motorstudio0.0.8\motorstudio0.0.8\config.xml"
try:
    with open(config_path, 'r', encoding='utf-8') as f:
        config_content = f.read()
        if 'COM3' in config_content:
            print("[OK] Motor Studio configured for COM3")
        if '921600' in config_content:
            print("[OK] Baudrate set to 921600")
except FileNotFoundError:
    print("[!] Could not find Motor Studio config file")
print()

# Step 5: Recommendations
print("5. RECOMMENDED ACTIONS")
print("-" * 60)
print("To fix Motor Studio connection:")
print()
print("[ ] Step 1: Close ALL Python scripts")
print("    -> Run: taskkill /F /IM python.exe")
print()
print("[ ] Step 2: Unplug USB-CAN adapter")
print("    -> Wait 5 seconds")
print("    -> Plug it back in")
print()
print("[ ] Step 3: Check Device Manager")
print("    -> Win+X -> Device Manager -> Ports (COM & LPT)")
print("    -> Note which COM port your adapter is on")
print()
print("[ ] Step 4: Open Motor Studio")
print("    -> Select the correct COM port from dropdown")
print("    -> Baudrate: 921600")
print("    -> Click 'Connect'")
print()
print("[ ] Step 5: If still not working:")
print("    -> Power cycle the motor (turn power supply off/on)")
print("    -> Wait 10 seconds")
print("    -> Try connecting again")
print()

print("=" * 60)
print("Run this script again after trying the fixes to verify")
print("=" * 60)

