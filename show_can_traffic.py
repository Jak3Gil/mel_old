#!/usr/bin/env python3
"""
Show CAN Traffic in Real-Time
Displays exactly what's being sent to the motor
"""

import serial
import time
import glob


def decode_slcan_message(msg):
    """Decode SLCAN message to human-readable format."""
    msg = msg.strip()
    
    if msg.startswith('t'):  # Standard CAN frame
        can_id = msg[1:4]
        data_len = msg[4]
        data = msg[5:-1] if msg.endswith('\r') else msg[5:]
        
        # Parse data bytes
        data_bytes = []
        for i in range(0, len(data), 2):
            if i+1 < len(data):
                data_bytes.append(data[i:i+2])
        
        return {
            'type': 'Standard',
            'can_id': f"0x{can_id}",
            'length': data_len,
            'data': ' '.join(data_bytes),
            'data_bytes': data_bytes
        }
    
    elif msg.startswith('T'):  # Extended CAN frame
        can_id = msg[1:9]
        data_len = msg[9]
        data = msg[10:-1] if msg.endswith('\r') else msg[10:]
        
        data_bytes = []
        for i in range(0, len(data), 2):
            if i+1 < len(data):
                data_bytes.append(data[i:i+2])
        
        return {
            'type': 'Extended',
            'can_id': f"0x{can_id}",
            'length': data_len,
            'data': ' '.join(data_bytes),
            'data_bytes': data_bytes
        }
    
    return None


def interpret_command(parsed):
    """Interpret what the CAN command does."""
    if not parsed:
        return "Unknown"
    
    data = parsed['data_bytes']
    
    # Check for specific MIT protocol commands
    if len(data) == 8:
        if all(b == 'FF' for b in data[:-1]):
            last_byte = data[-1]
            if last_byte == 'FC':
                return "🔓 ENABLE MOTOR"
            elif last_byte == 'FD':
                return "🔒 DISABLE MOTOR"
            elif last_byte == 'FE':
                return "🎯 SET ZERO POSITION"
            elif last_byte == '06':
                return "⚙️  CALIBRATE ENCODER"
        
        # Check for magic sequences
        if data == ['AA', '55', 'AA', '55', 'AA', '55', 'AA', '55']:
            return "🔄 RESET SEQUENCE (Magic 1)"
        elif data == ['55', '55', 'AA', 'AA', '55', '55', 'AA', 'AA']:
            return "🔄 RESET SEQUENCE (Magic 2)"
        elif all(b == '00' for b in data):
            return "🧹 CLEAR ALL REGISTERS"
        elif all(b == 'FF' for b in data[:-1]) and data[-1] == '00':
            return "🧹 CLEAR FAULTS"
        elif data[0] != 'FF':
            return "📍 POSITION COMMAND (MIT protocol)"
    
    return "❓ Other command"


def show_traffic():
    """Show live CAN traffic with explanations."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   LIVE CAN BUS TRAFFIC MONITOR                         ║")
    print("║   Showing what's being sent to the motor               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found")
        return
    
    port = ports[0]
    print(f"Port: {port}")
    print(f"💡 Blue LED on adapter = Data being transmitted\n")
    print("="*60)
    
    ser = serial.Serial(port, 115200, timeout=0.5)
    time.sleep(0.5)
    
    print("\n[INIT] Initializing CAN adapter...")
    
    commands = [
        (b'C\r', "Close CAN bus"),
        (b'L91\r', "Set L91 protocol (SLCAN)"),
        (b'O\r', "Open CAN bus for communication"),
    ]
    
    for cmd, desc in commands:
        print(f"  → {desc}")
        print(f"     Sending: {cmd}")
        ser.write(cmd)
        time.sleep(0.2)
        ser.read(1000)
    
    print("\n✓ CAN bus open - ready to send motor commands\n")
    print("="*60)
    print("SENDING MOTOR COMMANDS:")
    print("="*60 + "\n")
    
    # Send and explain various commands
    test_commands = [
        (b't0018FFFFFFFFFFFFFFFC\r', "Enable motor on CAN ID 0x01"),
        (b't0018FFFFFFFFFFFFFFFE\r', "Set zero position"),
        (b't00187FFF7FF666666D7FF\r', "Position: 0 rad, Kp=50, Kd=2"),
        (b't0018A1467FF666666D7FF\r', "Position: 2 rad, Kp=50, Kd=2"),
        (b't0018FFFFFFFFFFFFFFFD\r', "Disable motor"),
    ]
    
    msg_num = 1
    for cmd, description in test_commands:
        print(f"[{msg_num}] {description}")
        print(f"    Raw SLCAN: {cmd.decode().strip()}")
        
        # Decode and show details
        parsed = decode_slcan_message(cmd.decode())
        if parsed:
            print(f"    CAN ID: {parsed['can_id']}")
            print(f"    Data ({parsed['length']} bytes): {parsed['data']}")
            interpretation = interpret_command(parsed)
            print(f"    Meaning: {interpretation}")
        
        # Send it
        print(f"    💡 Blue LED should light up NOW...")
        ser.write(cmd)
        time.sleep(0.3)
        
        # Check for response
        if ser.in_waiting > 0:
            resp = ser.read(ser.in_waiting)
            print(f"    ✓ Motor responded: {resp.hex()}")
        else:
            print(f"    ⚠️  No response from motor")
        
        print()
        msg_num += 1
        time.sleep(0.5)
    
    ser.close()
    
    print("="*60)
    print("TRAFFIC MONITOR COMPLETE")
    print("="*60)
    
    print("\n📊 WHAT YOU SAW:")
    print("\n  Blue LED on USB-CAN adapter:")
    print("    • Blinks/lights when data transmitted")
    print("    • This is NORMAL and GOOD")
    print("    • Means: Commands reaching CAN bus")
    
    print("\n  What's being sent:")
    print("    • SLCAN format messages")
    print("    • 't' = standard CAN frame")
    print("    • Next 3 chars = CAN ID (e.g., 001 = 0x01)")
    print("    • Next char = data length (8 bytes)")
    print("    • Next 16 chars = 8 bytes of data")
    print("    • '\\r' = carriage return (end marker)")
    
    print("\n  Example breakdown:")
    print("    t 001 8 FFFFFFFFFFFFFFFC \\r")
    print("    │  │  │ │               └─ End marker")
    print("    │  │  │ └─────────────────── Data: all FF except last byte")
    print("    │  │  └───────────────────── Length: 8 bytes")
    print("    │  └──────────────────────── CAN ID: 0x001")
    print("    └─────────────────────────── Standard frame")
    
    print("\n  💡 Key insight:")
    print("    • Blue LED = CAN traffic happening ✓")
    print("    • If motor doesn't respond = motor not listening")
    print("    • This suggests motor needs PC software setup")


if __name__ == "__main__":
    try:
        show_traffic()
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

