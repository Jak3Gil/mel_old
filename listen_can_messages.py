#!/usr/bin/env python3
"""
Listen to CAN bus messages on COM3
Use this while Robstride software is running to see what it sends
"""

import can
import time
from datetime import datetime

def listen_can_bus():
    """Listen to all CAN messages and display them."""
    print("=" * 70)
    print("   CAN BUS MESSAGE SNIFFER - COM3")
    print("=" * 70)
    print()
    print("This will listen to all CAN messages on the bus.")
    print()
    print("INSTRUCTIONS:")
    print("  1. Start this script")
    print("  2. Open Robstride PC software")
    print("  3. Connect to the motor")
    print("  4. Move the motor")
    print("  5. Watch what CAN messages appear here")
    print()
    print("Press Ctrl+C to stop")
    print()
    print("=" * 70)
    print()
    
    COM_PORT = "COM3"
    
    try:
        print(f"[1] Connecting to CAN bus on {COM_PORT}...")
        bus = can.interface.Bus(
            interface='slcan',
            channel=COM_PORT,
            bitrate=1000000,
            ttyBaudrate=115200
        )
        print("    [OK] Listening...\n")
        print("-" * 70)
        print(f"{'TIME':<12} {'CAN ID':<10} {'DLC':<5} {'DATA':<25} {'ASCII':<20}")
        print("-" * 70)
        
        message_count = 0
        start_time = time.time()
        
        while True:
            # Receive message (with timeout)
            msg = bus.recv(timeout=1.0)
            
            if msg is not None:
                message_count += 1
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
                
                # Format data as hex
                data_hex = ' '.join(f'{b:02X}' for b in msg.data)
                
                # Try to decode as ASCII
                try:
                    data_ascii = ''.join(chr(b) if 32 <= b < 127 else '.' for b in msg.data)
                except:
                    data_ascii = ''
                
                # Print message
                print(f"{timestamp:<12} 0x{msg.arbitration_id:03X} ({msg.arbitration_id:3d}) "
                      f"{msg.dlc:<5} {data_hex:<25} {data_ascii:<20}")
                
                # Special highlighting for motor ID 13
                if msg.arbitration_id == 0x0D or msg.arbitration_id == 13:
                    print("         ^^^ Motor ID 13 message! ^^^")
            
            # Print stats every 10 seconds
            if time.time() - start_time > 10:
                print(f"\n[STATS] Received {message_count} messages in last 10 seconds\n")
                message_count = 0
                start_time = time.time()
        
    except KeyboardInterrupt:
        print("\n\n" + "=" * 70)
        print("STOPPED BY USER")
        print("=" * 70)
        print(f"\nTotal messages received: {message_count}")
        print()
        print("What did you see?")
        print("  - If you saw messages: Great! The CAN bus is active")
        print("  - If NO messages: Either:")
        print("    * Robstride software wasn't running/connected")
        print("    * Motor communication happens on different interface")
        print("    * Adapter needs special mode to see traffic")
        
    except Exception as e:
        print(f"\n[ERROR] {e}")
        import traceback
        traceback.print_exc()
        
        print("\nTroubleshooting:")
        print("  1. Close other programs using COM3")
        print("  2. Unplug/replug USB-CAN adapter")
        print("  3. Check adapter drivers are installed")
    
    finally:
        try:
            bus.shutdown()
        except:
            pass

if __name__ == "__main__":
    print("\nCAN Bus Message Sniffer\n")
    listen_can_bus()

