# Robstride Motor Control Guide

**Successfully Tested - Motor ID 13 on COM3**

## Quick Start

```python
python robstride_control.py
```

The motor will run a demo showing forward/reverse movements.

---

## Critical Settings

| Setting | Value | Notes |
|---------|-------|-------|
| **Baud Rate** | `921600` | ⚠️ CRITICAL - Motor won't respond at 115200! |
| **COM Port** | `COM3` | Check Device Manager if different |
| **CAN ID** | `13` | Your motor's CAN bus address |
| **MCU ID** | `0x1137239c20303d3c` | Unique motor identifier |

---

## Hardware Setup

### Requirements
- Robstride Motor (Lingzu Motor 13)
- USB-CAN adapter (CH340 driver)
- 12-48V DC power supply
- CANH/CANL wiring
- 120Ω termination resistor

### Connections
```
PC USB ──> USB-CAN Adapter ──> Motor
                    │
                    └──> CANH to Motor CANH
                    └──> CANL to Motor CANL
                    └──> GND to Motor GND
```

---

## Software Installation

### 1. Install Python Dependencies
```bash
pip install pyserial
```

### 2. Verify COM Port
- Open Windows Device Manager
- Look under "Ports (COM & LPT)"
- Find your USB-Serial adapter
- Note the COM port number

### 3. Close Robstride Software
⚠️ **IMPORTANT:** Close Robstride PC software before running Python scripts - only one program can use COM port at a time!

---

## Usage

### Basic Control

```python
from robstride_control import RobstrideMotor

# Create motor instance
motor = RobstrideMotor(port='COM3', can_id=13)

# Connect and initialize
motor.connect()

# Move forward for 2 seconds
motor.jog_forward(2.0)

# Move reverse for 2 seconds  
motor.jog_reverse(2.0)

# Stop immediately
motor.stop()

# Disconnect
motor.disconnect()
```

### Custom Movements

```python
# Jog with custom direction and duration
motor.jog(direction=1, duration=3.0)   # Forward 3 seconds
motor.jog(direction=-1, duration=1.5)  # Reverse 1.5 seconds

# Quick pulse
motor.jog_forward(0.1)  # 100ms forward pulse
```

---

## Protocol Details

### Communication Protocol

The Robstride motor uses a custom AT command protocol over serial:

1. **Initialization Sequence:**
   ```
   AT+AT\r\n                    # Handshake
   AT\x00\x07 [CAN_ID] ...      # Scan for motor
   ```

2. **Movement Command Format:**
   ```
   AT\x90\x07 [CAN_ID_ENCODED] \x08 [8_BYTES_DATA] \r\n
   ```

3. **JOG Commands:**
   - Forward: `05 70 00 00 07 01 83 e0`
   - Reverse: `05 70 00 00 07 01 7c 1e`
   - Stop: `05 70 00 00 07 00 7f ff`

### CAN ID Encoding

For CAN ID 13, the encoding is `0xe8 0x6c`:
- This is used in movement commands to address the specific motor

---

## Troubleshooting

### Motor Doesn't Move

**Problem:** Motor doesn't respond to commands
**Solution:** Check baud rate - MUST be **921600**

```python
# Wrong (won't work):
motor = RobstrideMotor(port='COM3', baud=115200)  # ❌

# Correct:
motor = RobstrideMotor(port='COM3', baud=921600)  # ✅
```

### COM Port Permission Error

**Problem:** `PermissionError: Access is denied`
**Solution:** 
1. Close Robstride PC software
2. Close any other serial monitor tools
3. Unplug and replug USB adapter
4. Wait 5 seconds, then try again

### Motor Not Detected

**Problem:** No response during scan
**Solution:**
1. Check motor power (LED should be on)
2. Verify CANH/CANL wiring
3. Ensure 120Ω termination resistor installed
4. Confirm motor works with Robstride software
5. Check correct COM port in Device Manager

### Wrong Baud Rate Symptoms

If using wrong baud rate (115200 instead of 921600):
- ❌ No response to AT+AT handshake
- ❌ Motor scan returns no data
- ❌ Motor never activates
- ❌ Movement commands ignored

**Test Baud Rate:**
```bash
python test_baud_rates.py
```

---

## Technical Discovery

### How We Found the Solution

The motor control issue was solved through systematic debugging:

1. **Initial Problem:** Motor worked with Robstride software but not custom scripts
2. **Protocol Analysis:** Captured serial traffic showing AT commands
3. **Key Discovery:** Motor wasn't responding to commands
4. **Root Cause:** Wrong baud rate (115200 vs 921600)
5. **Solution:** Changed to 921600 baud → Motor responded → Success!

### Serial Traffic Capture

Example of working communication:
```
→ COM3: 41 54 2b 41 54 0d 0a              # AT+AT handshake
← COM3: 4f 4b 0d 0a                       # "OK" response

→ COM3: 41 54 00 07 e8 6c 01 00 0d 0a     # Scan for motor 13
← COM3: 41 54 00 00 6f f4 08 ...          # Motor responds with MCU ID

→ COM3: 41 54 90 07 e8 6c 08 05 70 ...    # JOG forward command
✓ Motor moves!
```

---

## Files

- `robstride_control.py` - Production motor control class
- `test_baud_rates.py` - Test different baud rates
- `move_motor_correct_baud.py` - Working test script
- `ROBSTRIDE_MOTOR_CONTROL.md` - This document

---

## API Reference

### RobstrideMotor Class

```python
class RobstrideMotor:
    def __init__(self, port='COM3', can_id=13, baud=921600)
    def connect() -> bool
    def disconnect() -> None
    def jog(direction=1, duration=1.0) -> bool
    def jog_forward(duration=1.0) -> bool
    def jog_reverse(duration=1.0) -> bool
    def stop() -> bool
```

### Methods

**`connect()`**
- Connects to motor and performs initialization
- Returns: `True` if successful, `False` otherwise

**`jog(direction, duration)`**
- Jogs motor in specified direction
- `direction`: `1` for forward, `-1` for reverse
- `duration`: How long to move (seconds)

**`jog_forward(duration)`**
- Convenience method to jog forward
- `duration`: How long to move (seconds)

**`jog_reverse(duration)`**
- Convenience method to jog reverse
- `duration`: How long to move (seconds)

**`stop()`**
- Immediately stops motor

**`disconnect()`**
- Closes serial connection

---

## Safety Notes

⚠️ **Important Safety Information:**

1. **Power Supply:** Ensure proper 12-48V DC supply rated for motor
2. **Emergency Stop:** Keep physical access to power switch
3. **Clear Workspace:** Ensure motor shaft can rotate freely
4. **Testing:** Start with short durations (0.5-1 second)
5. **Disconnect:** Always call `motor.disconnect()` when done

---

## Future Enhancements

Potential improvements:
- [ ] Position control mode (MIT protocol)
- [ ] Velocity feedback reading
- [ ] Torque control
- [ ] Multiple motor support
- [ ] Auto-detect COM port
- [ ] Configuration file support

---

## Credits

**Motor:** Robstride Lingzu Motor (CAN ID 13)  
**Protocol:** AT command over serial @ 921600 baud  
**Tested:** October 2025  
**Status:** ✅ Working

---

## License

This is part of the Melvin project.

---

## Support

If you encounter issues:
1. Verify baud rate is **921600** (most common issue!)
2. Check COM port is correct
3. Ensure Robstride software is closed
4. Test motor works with Robstride software first
5. Run `test_baud_rates.py` to verify communication

**Key Takeaway:** The motor MUST communicate at **921600 baud** - this is non-negotiable!

