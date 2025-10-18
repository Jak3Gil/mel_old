#!/usr/bin/env python3
"""
Windows CAN Connection Diagnostic
Quick test to see why Windows can't connect
"""

import sys
import time

def test_serial_ports():
    """Test if we can list and open serial ports."""
    print("="*60)
    print("STEP 1: Checking for Serial Ports")
    print("="*60 + "\n")
    
    try:
        import serial.tools.list_ports
        ports = list(serial.tools.list_ports.comports())
        
        if not ports:
            print("✗ No COM ports found!")
            print("\nPossible causes:")
            print("  • USB-CAN adapter not plugged in")
            print("  • Driver not installed")
            print("  • Adapter not recognized by Windows")
            print("\nSolution:")
            print("  1. Check Device Manager (Win+X)")
            print("  2. Look for yellow warning symbols")
            print("  3. Install CH340 driver (most common)")
            print("  4. Restart computer")
            return None
        
        print(f"✓ Found {len(ports)} COM port(s):\n")
        for i, port in enumerate(ports, 1):
            print(f"  [{i}] {port.device}")
            print(f"      Description: {port.description}")
            print(f"      Hardware ID: {port.hwid}")
            print()
        
        return ports
        
    except ImportError:
        print("✗ pyserial not installed!")
        print("\nInstall with:")
        print("  pip install pyserial")
        return None


def test_port_open(port_device):
    """Test if we can open a specific port."""
    print("="*60)
    print(f"STEP 2: Testing Port: {port_device}")
    print("="*60 + "\n")
    
    try:
        import serial
        ser = serial.Serial(port_device, 115200, timeout=1)
        print(f"✓ Successfully opened {port_device}")
        
        # Try version command
        print("  Sending 'V' (version command)...")
        ser.write(b'V\r')
        time.sleep(0.2)
        resp = ser.read(100)
        
        if resp:
            print(f"  ✓ Adapter responds: {resp}")
        else:
            print("  ⚠ No response (but port opened)")
            print("    This is OK - some adapters don't respond to V")
        
        ser.close()
        return True
        
    except PermissionError:
        print(f"✗ Permission denied on {port_device}")
        print("\nSolutions:")
        print("  • Close any programs using this port")
        print("  • Run this script as Administrator")
        print("  • Restart computer")
        return False
        
    except Exception as e:
        print(f"✗ Error: {e}")
        return False


def test_slcan_init(port_device):
    """Test SLCAN initialization."""
    print("\n" + "="*60)
    print("STEP 3: Testing SLCAN Protocol (L91)")
    print("="*60 + "\n")
    
    try:
        import serial
        ser = serial.Serial(port_device, 115200, timeout=1)
        
        print("  Sending 'C' (close CAN)...")
        ser.write(b'C\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.hex()}")
        
        print("  Sending 'L91' (set protocol)...")
        ser.write(b'L91\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.hex()}")
        
        print("  Sending 'O' (open CAN)...")
        ser.write(b'O\r')
        time.sleep(0.1)
        resp = ser.read(1000)
        if resp:
            print(f"    Response: {resp.hex()}")
        
        print("\n✓ SLCAN initialization complete")
        
        # Try motor communication
        print("\n  Testing motor communication...")
        print("  Sending motor enable command...")
        ser.write(b't0018FFFFFFFFFFFFFFFC\r')
        time.sleep(0.3)
        resp = ser.read(1000)
        
        if len(resp) > 0:
            print(f"  ✓ Motor responded: {resp.hex()}")
            print("\n🎉 SUCCESS! Motor is responding on Windows!")
        else:
            print("  ⚠ No motor response")
            print("    (CAN adapter works, but motor not responding)")
        
        ser.close()
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print("\n" + "="*60)
    print("WINDOWS CAN CONNECTION DIAGNOSTIC")
    print("="*60)
    print("\nThis will test:")
    print("  1. Can Windows see COM ports?")
    print("  2. Can we open the ports?")
    print("  3. Does SLCAN protocol work?")
    print("  4. Does motor respond?")
    print("\n" + "="*60 + "\n")
    
    time.sleep(2)
    
    # Step 1: Find ports
    ports = test_serial_ports()
    if not ports:
        return
    
    # Step 2: Test each port
    working_port = None
    for port in ports:
        if test_port_open(port.device):
            working_port = port.device
            break
    
    if not working_port:
        print("\n" + "="*60)
        print("DIAGNOSIS: Cannot open any COM ports")
        print("="*60)
        print("\nThis means:")
        print("  • Driver is installed (ports show up)")
        print("  • But something is blocking access")
        print("\nSolutions:")
        print("  1. Close ALL programs that might use serial ports")
        print("  2. Check Task Manager for:")
        print("     - python.exe")
        print("     - ServoStudio.exe")
        print("     - Any monitoring software")
        print("  3. Restart computer (easiest)")
        print("  4. Run this script as Administrator")
        return
    
    # Step 3: Test SLCAN
    test_slcan_init(working_port)
    
    print("\n" + "="*60)
    print("DIAGNOSTIC COMPLETE")
    print("="*60)
    
    print("\n📋 RESULTS SUMMARY:")
    print(f"  • COM ports found: ✓")
    print(f"  • Working port: {working_port}")
    print(f"  • Port opens: ✓")
    print(f"  • SLCAN protocol: Tested above")
    
    print("\n💡 NEXT STEPS:")
    print("\nIf motor responded:")
    print("  ✓ Everything works!")
    print("  ✓ Use this COM port in Motor Studio")
    print("  ✓ Run: python test_obvious_movement.py")
    
    print("\nIf motor didn't respond:")
    print("  • CAN adapter works on Windows ✓")
    print("  • But motor not responding")
    print("  • This is same issue as Mac had")
    print("  • Motor needs position mode enabled")
    print("  • Use Motor Studio to enable it")
    
    print("\nIf nothing worked:")
    print("  • See WINDOWS_CAN_CONNECTION_FIX.md")
    print("  • Install driver (likely CH340)")
    print("  • Restart computer")
    print("  • Run this test again")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}")
        import traceback
        traceback.print_exc()

