#!/usr/bin/env python3
"""
Robstride Motor - AT Command Mode Initialization
Try AT commands to enable position control mode
"""

import serial
import time
import glob


def send_at_command(ser, cmd, description="", wait=0.5):
    """Send AT command and return response."""
    print(f"  → {cmd:<30} {description}")
    
    # Try both \r and \r\n endings
    for ending in [b'\r', b'\r\n', b'\n']:
        ser.write(cmd.encode() + ending)
        time.sleep(wait)
        
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting)
            if len(response) > 0 and response != b'\r' and response != b'\n':
                print(f"     Response: {response}")
                return response
    
    print(f"     (no response)")
    return None


def try_at_commands():
    """Try comprehensive set of AT commands."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║   ROBSTRIDE AT COMMAND MODE INITIALIZATION             ║")
    print("║   Trying to enable position mode via AT commands       ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        print("✗ No USB-CAN adapter found")
        return False
    
    port = ports[0]
    print(f"Port: {port}\n")
    
    # Try different baudrates - AT commands might need different rate
    baudrates = [115200, 9600, 19200, 38400, 57600]
    
    for baud in baudrates:
        print(f"\n{'='*60}")
        print(f"TRYING BAUDRATE: {baud}")
        print('='*60 + "\n")
        
        try:
            ser = serial.Serial(port, baud, timeout=1.0)
            time.sleep(0.5)
            
            print("[1] Basic AT Commands:")
            responses = []
            
            # Basic test commands
            basic_commands = [
                ("AT", "Basic test"),
                ("AT?", "Help/Query"),
                ("ATE0", "Echo off"),
                ("ATE1", "Echo on"),
                ("ATI", "Info/Version"),
                ("ATZ", "Reset"),
            ]
            
            for cmd, desc in basic_commands:
                resp = send_at_command(ser, cmd, desc)
                if resp:
                    responses.append((cmd, resp))
            
            print("\n[2] Mode Configuration Commands:")
            
            mode_commands = [
                ("AT+MODE=0", "Set mode 0"),
                ("AT+MODE=1", "Set mode 1 (Position)"),
                ("AT+MODE=2", "Set mode 2"),
                ("AT+MODE=3", "Set mode 3"),
                ("AT+MODE=MIT", "Set MIT mode"),
                ("AT+MODE=POS", "Set Position mode"),
                ("AT+POSMODE", "Position mode"),
                ("AT+MITMODE", "MIT mode"),
                ("AT+MODE?", "Query current mode"),
            ]
            
            for cmd, desc in mode_commands:
                resp = send_at_command(ser, cmd, desc)
                if resp:
                    responses.append((cmd, resp))
            
            print("\n[3] Enable/Configuration Commands:")
            
            config_commands = [
                ("AT+ENABLE", "Enable motor"),
                ("AT+ENABLE=1", "Enable=1"),
                ("AT+EN", "Enable (short)"),
                ("AT+CFG?", "Query config"),
                ("AT+CFG=POS", "Config position"),
                ("AT+CTRL=1", "Control mode 1"),
                ("AT+CTRL=POS", "Control position"),
                ("AT+INIT", "Initialize"),
                ("AT+START", "Start motor"),
            ]
            
            for cmd, desc in config_commands:
                resp = send_at_command(ser, cmd, desc)
                if resp:
                    responses.append((cmd, resp))
            
            print("\n[4] Save/Apply Commands:")
            
            save_commands = [
                ("AT+SAVE", "Save configuration"),
                ("AT+WRITE", "Write config"),
                ("AT+APPLY", "Apply settings"),
                ("AT&W", "Write to EEPROM"),
                ("AT&F", "Factory defaults"),
            ]
            
            for cmd, desc in save_commands:
                resp = send_at_command(ser, cmd, desc)
                if resp:
                    responses.append((cmd, resp))
            
            print("\n[5] Motor-Specific Commands:")
            
            motor_commands = [
                ("AT+MOTOR?", "Motor status"),
                ("AT+MOTOR=1", "Motor enable"),
                ("AT+CAL", "Calibrate"),
                ("AT+ZERO", "Set zero"),
                ("AT+ID?", "Query ID"),
                ("AT+ID=1", "Set ID=1"),
            ]
            
            for cmd, desc in motor_commands:
                resp = send_at_command(ser, cmd, desc)
                if resp:
                    responses.append((cmd, resp))
            
            ser.close()
            
            # If we got responses, this might be the right baudrate
            if responses:
                print(f"\n{'='*60}")
                print(f"✓ GOT RESPONSES AT BAUDRATE {baud}!")
                print('='*60)
                print("\nSuccessful Commands:")
                for cmd, resp in responses:
                    print(f"  {cmd}: {resp}")
                
                # Try to test motor now
                print(f"\n\nAttempting to test motor at baudrate {baud}...")
                test_motor_after_at(port, baud)
                return True
            
        except Exception as e:
            print(f"Error at baudrate {baud}: {e}")
            continue
    
    print("\n" + "="*60)
    print("NO AT COMMAND RESPONSES RECEIVED")
    print("="*60)
    return False


def test_motor_after_at(port, baudrate):
    """Test if motor responds after AT commands."""
    print("\n[TESTING] Trying to move motor after AT init...")
    
    ser = serial.Serial(port, baudrate, timeout=0.5)
    time.sleep(0.5)
    
    # Switch to CAN mode
    print("  Switching to CAN mode (L91)...")
    ser.write(b'C\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'L91\r')
    time.sleep(0.1)
    ser.read(1000)
    ser.write(b'O\r')
    time.sleep(0.1)
    ser.read(1000)
    
    # Try to enable and move
    print("  Enabling motor...")
    ser.write(b't0018FFFFFFFFFFFFFFFC\r')
    time.sleep(0.3)
    ser.read(1000)
    
    print("  Setting zero...")
    ser.write(b't0018FFFFFFFFFFFFFFFE\r')
    time.sleep(0.3)
    ser.read(1000)
    
    print("  Sending position command (2 radians)...")
    print("  🔍 WATCH THE MOTOR!\n")
    
    for i in range(60):
        ser.write(b't0018A1467FF8CCCCD7FF\r')
        time.sleep(0.02)
    
    time.sleep(1)
    
    print("  Returning to zero...")
    for i in range(60):
        ser.write(b't00187FFF7FF8CCCCD7FF\r')
        time.sleep(0.02)
    
    print("  Disabling...")
    ser.write(b't0018FFFFFFFFFFFFFFFD\r')
    time.sleep(0.2)
    ser.read(1000)
    
    ser.close()
    
    print("\n  ❓ DID THE MOTOR MOVE?")


def try_direct_at_mode_switch():
    """Try direct AT commands to switch mode without L91."""
    print("\n\n╔════════════════════════════════════════════════════════╗")
    print("║   DIRECT AT MODE COMMANDS (No L91 first)               ║")
    print("╚════════════════════════════════════════════════════════╝\n")
    
    ports = glob.glob('/dev/cu.usbserial*')
    if not ports:
        return
    
    port = ports[0]
    
    print("Trying AT commands directly at 115200 baud...\n")
    
    ser = serial.Serial(port, 115200, timeout=1.0)
    time.sleep(0.5)
    
    # Comprehensive AT command list
    print("[Testing Extended AT Commands]\n")
    
    extended_commands = [
        # Hayes-style AT commands
        "AT",
        "ATE1",
        "ATI",
        "AT&V",
        "AT&F",
        "ATZ",
        
        # Mode commands
        "AT+MODE=1",
        "AT+MODE=POS",
        "AT+MODE=POSITION",
        "AT+MODE=MIT",
        "AT+POSMODE=1",
        "AT+MITMODE=1",
        "AT+CONTROL=POSITION",
        
        # Enable commands  
        "AT+ENABLE=1",
        "AT+EN=1",
        "AT+MOTOR=ON",
        "AT+MOTOR=1",
        
        # Configuration
        "AT+CFG=POS",
        "AT+CONFIG=POSITION",
        "AT+SETUP=1",
        
        # Robstride specific (guessing)
        "AT+RS_MODE=1",
        "AT+RS_EN=1",
        "AT+ROBSTRIDE=POS",
        
        # Query commands
        "AT+MODE?",
        "AT+STATUS?",
        "AT+VER?",
        "AT+INFO?",
        
        # Save/commit
        "AT+SAVE",
        "AT+WRITE",
        "AT+COMMIT",
        "AT&W",
    ]
    
    got_response = False
    for cmd in extended_commands:
        for ending in ['\r', '\r\n', '\n']:
            ser.write((cmd + ending).encode())
            time.sleep(0.3)
            
            if ser.in_waiting > 0:
                resp = ser.read(ser.in_waiting)
                if len(resp) > 0:
                    print(f"  ✓ {cmd:<30} → {resp}")
                    got_response = True
                    break
    
    ser.close()
    
    if got_response:
        print("\n✓ Got AT command responses!")
        print("  Try running: python3 test_obvious_movement.py")
    else:
        print("\n✗ No AT command responses")


if __name__ == "__main__":
    print("\n" + "="*60)
    print("ROBSTRIDE AT COMMAND INITIALIZATION")
    print("="*60)
    print("\nMotor controllers often use AT commands for configuration.")
    print("This will try comprehensive AT command sequences.\n")
    
    time.sleep(2)
    
    try:
        # Try with different baudrates first
        success = try_at_commands()
        
        # Also try direct AT commands
        try_direct_at_mode_switch()
        
        print("\n\n" + "="*60)
        print("SUMMARY")
        print("="*60)
        
        if success:
            print("\n✓ Motor responded to AT commands!")
            print("\nNext steps:")
            print("  1. Test motor: python3 test_obvious_movement.py")
            print("  2. If works: You're done!")
            print("  3. If not: Power cycle motor and test again")
        else:
            print("\n✗ No AT command responses detected")
            print("\nPossibilities:")
            print("  1. Motor doesn't use standard AT commands")
            print("  2. AT commands require specific adapter mode")
            print("  3. AT commands disabled in your motor firmware")
            print("  4. Different command syntax needed")
            print("\n  → Still need Robstride PC software")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()

