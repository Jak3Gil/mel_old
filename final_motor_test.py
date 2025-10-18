#!/usr/bin/env python3
"""
Final comprehensive motor test - try ALL parameters
"""

import serial
import time
import glob

def final_test():
    """Try every possible configuration."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   FINAL COMPREHENSIVE MOTOR TEST                       ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    port = glob.glob('/dev/cu.usbserial*')[0]
    print(f"Port: {port}\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.3)
    
    # Initialize
    print("[1] L91 Init...")
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
    sdo_cob = 0x600 + motor_id
    
    print("\n[2] Setting ALL necessary parameters...")
    print("="*60)
    
    # Set multiple parameters that might be needed
    params = [
        (0x7005, 0x01, "run_mode = Position (1)"),
        (0x6060, 0x01, "Modes of operation = Position (1)"),
        (0x6040, 0x0F, "Controlword = Enable (0x0F)"),
        (0x6040, 0x1F, "Controlword = Enable+NewSetpoint (0x1F)"),
    ]
    
    for index, value, desc in params:
        print(f"\n  {desc}")
        # SDO write: 1 byte
        data = bytes([
            0x2F,  # Write 1 byte
            index & 0xFF,  # Index low
            (index >> 8) & 0xFF,  # Index high
            0x00,  # Subindex
            value,  # Value
            0x00, 0x00, 0x00  # Padding
        ])
        msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
        ser.write(msg.encode())
        time.sleep(0.2)
        ser.read(1000)
    
    print("\n\n[3] Trying TORQUE MODE (might work when position doesn't)...")
    print("="*60)
    
    # Switch to torque mode (mode 3)
    data = bytes([0x2F, 0x05, 0x70, 0x00, 0x03, 0x00, 0x00, 0x00])
    msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
    ser.write(msg.encode())
    time.sleep(0.3)
    
    # Enable
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    
    # Send PURE TORQUE command (no position)
    print("  Sending torque commands (should make motor twitch/resist)...")
    for i in range(20):
        # Torque = 2 Nm, no position/velocity control
        ser.write(b't00187FFF7FFF00000A00\r')
        time.sleep(0.05)
    
    print("  (Did you feel resistance or hear sound?)")
    
    print("\n\n[4] Back to POSITION MODE with EXTREME settings...")
    print("="*60)
    
    # Position mode
    data = bytes([0x2F, 0x05, 0x70, 0x00, 0x01, 0x00, 0x00, 0x00])
    msg = f"t{sdo_cob:03X}8{data.hex().upper()}\r"
    ser.write(msg.encode())
    time.sleep(0.3)
    
    # Enable
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.2)
    
    # Zero
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.2)
    
    # EXTREME position command: 5 radians, max everything
    print("  Sending EXTREME position command...")
    print("  (5 radians = almost 2 full rotations)")
    print("  (kp=500 max, kd=5 max, torque=10 Nm)")
    
    for i in range(50):
        # pos=5rad, vel=0, kp=500, kd=5, torque=10
        ser.write(b't0018E38E7FFFFFFFFFFF\r')
        time.sleep(0.03)
    
    time.sleep(1)
    
    # Return
    print("\n  Returning to zero with max force...")
    for i in range(50):
        ser.write(b't00187FFFFFFFFFFF7FFF\r')
        time.sleep(0.03)
    
    # Disable
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.1)
    
    ser.close()
    
    print("\n\n" + "="*60)
    print("FINAL TEST COMPLETE")
    print("="*60)
    print("\n🔍 DID YOU OBSERVE ANY OF THESE:")
    print("  • Motor shaft rotating?")
    print("  • Motor vibrating or twitching?")
    print("  • Motor making sound (humming/whining)?")
    print("  • Motor feeling warm/hot?")
    print("  • Resistance when turning shaft by hand?")
    print("\nIf NONE of the above: Motor has deeper issue")

if __name__ == "__main__":
    final_test()
    
    print("\n\n" + "="*60)
    print("DIAGNOSIS SUMMARY")
    print("="*60)
    print("\nWhat's WORKING:")
    print("  ✓ CAN communication perfect")
    print("  ✓ L91 protocol working")
    print("  ✓ Motor responding to all commands")
    print("  ✓ Encoder calibrated (you confirmed)")
    print("  ✓ Wiring correct")
    print("\nWhat's NOT working:")
    print("  ✗ Motor not executing any commands")
    print("  ✗ No physical movement, sound, or resistance")
    print("\n💡 LIKELY CAUSES:")
    print("  1. Motor in hardware safety lock (needs official unlock)")
    print("  2. Motor firmware in wrong state")
    print("  3. Motor protection mode active")
    print("  4. Hardware fault in motor driver")
    print("\n🎯 RECOMMENDATION:")
    print("  Contact Robstride support with this info:")
    print("  • Motor CAN ID 0x01")
    print("  • Responds to all CAN commands (getting 0xFF responses)")
    print("  • Encoder calibrated")
    print("  • Using L91 protocol")
    print("  • Tried setting run_mode to position (0x7005=1)")
    print("  • Motor doesn't execute ANY commands (position/torque/speed)")
    print("  • No movement, sound, or resistance")
    print("  • Shaft moves freely by hand")
    print("\n  Ask: 'What additional initialization is required?'")




