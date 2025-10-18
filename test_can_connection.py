#!/usr/bin/env python3
"""
Quick test to verify CAN bus connection before running motor control.
"""

import can
import time

def test_connection():
    """Test basic CAN bus connectivity."""
    print("Testing CAN bus connection...")
    print("-" * 50)
    
    channel = '/dev/cu.usbserial-10'
    
    try:
        print(f"\n[1] Opening serial device: {channel}")
        
        # Try to connect with SLCAN
        bus = can.interface.Bus(
            channel=channel,
            bustype='slcan',
            bitrate=1000000,
            ttyBaudrate=115200  # Serial baudrate for communication with adapter
        )
        
        print("    ✓ CAN bus interface created successfully")
        print(f"    ✓ Bitrate: 1 Mbps")
        print(f"    ✓ Interface: SLCAN")
        
        print("\n[2] Testing CAN message transmission...")
        
        # Send a test message (this won't affect the motor if it's disabled)
        test_msg = can.Message(
            arbitration_id=0x01,
            data=[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
            is_extended_id=False
        )
        
        bus.send(test_msg)
        print("    ✓ Test message sent successfully")
        
        print("\n[3] Listening for responses (2 seconds)...")
        start_time = time.time()
        msg_count = 0
        
        while time.time() - start_time < 2.0:
            msg = bus.recv(timeout=0.1)
            if msg:
                msg_count += 1
                print(f"    ✓ Received message: ID=0x{msg.arbitration_id:03X}, Data={msg.data.hex()}")
        
        if msg_count == 0:
            print("    ⓘ No messages received (motor might be disabled or not responding)")
        
        bus.shutdown()
        print("\n" + "="*50)
        print("✓ CAN CONNECTION TEST PASSED")
        print("="*50)
        print("\nYou can now run: python3 robstride_control.py")
        return True
        
    except Exception as e:
        print(f"\n✗ Connection test failed: {e}")
        print("\nTroubleshooting:")
        print("1. Ensure the USB-CAN adapter is properly connected")
        print("2. Check that the device path is correct: ls -la /dev/cu.usbserial*")
        print("3. Try unplugging and replugging the USB-CAN adapter")
        print("4. Verify the adapter supports SLCAN protocol")
        return False

if __name__ == "__main__":
    test_connection()

