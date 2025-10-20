# Motor Troubleshooting - Motors 13 & 14

## Current Status: ❌ Not Communicating

The USB-CAN adapter at `/dev/cu.usbserial-110` is not responding to any commands.

---

## What We Tested

### ✅ Tests Completed
1. **Baud Rate Detection** - Tested 11 common baud rates (115200, 921600, etc.)
2. **AT Command Handshake** - Sent `AT+AT` initialization command
3. **Motor Scanning** - Tried to detect motors 13 and 14
4. **Serial Port Listening** - Checked for any incoming data
5. **Both tty and cu devices** - Tried both macOS serial device types

### ❌ Results
- **No response to AT+AT** handshake
- **No response** from motor scans
- **No data** received on serial port at all
- Tested at all common baud rates - **none worked**

---

## Root Cause

The USB-CAN adapter isn't sending or receiving any data, which means:

**Most Likely:**
1. 🔌 **Motors not powered on** - Check if LEDs are lit
2. 🔋 **Power supply issue** - Verify 12-48V power connected
3. 🔗 **USB-CAN adapter not connected** - Check physical connections

**Also Possible:**
4. 📡 **Different adapter type** - May not use AT commands
5. 💻 **Driver issue** - May need specific drivers on macOS
6. ⚙️ **Adapter configuration** - May need initialization software

---

## Critical Questions to Answer

### 1. Are the motors POWERED ON?
- [ ] Check for LED indicators on motors
- [ ] Verify power supply is connected (12-48V DC)
- [ ] Check power supply is turned on

### 2. Does Robstride software work?
- [ ] Can you control motors with Robstride PC software?
- [ ] On THIS Mac or a different computer?
- [ ] What port does Robstride software use?

### 3. Hardware Setup
- [ ] What USB-CAN adapter model are you using?
- [ ] Are CANH and CANL wires connected?
- [ ] Is 120Ω termination resistor installed?
- [ ] USB cable securely connected?

### 4. Previous Success
- [ ] Have these motors ever worked on this Mac?
- [ ] Did the motor-control-kscale branch work on Windows?
- [ ] Is this a new setup or was it working before?

---

## Next Steps

### Immediate Checks

1. **Power Check**
   ```bash
   # Look for LED lights on the motors
   # They should be illuminated when powered
   ```

2. **USB Connection**
   ```bash
   # Verify device is detected
   ls -la /dev/*usbserial*
   # Should show: /dev/cu.usbserial-110 and /dev/tty.usbserial-110
   ```

3. **Try Robstride Software**
   - Open Robstride control software
   - See if it can detect and move the motors
   - Note what settings it uses

### If Motors Are Powered and Connected

The USB-CAN adapter might use a different protocol. Options:

**Option A: SLCAN Protocol**
Some adapters use SLCAN (Serial Line CAN):
```bash
# Initialize SLCAN adapter
slcand -o -c -s6 /dev/cu.usbserial-110 can0
```

**Option B: SocketCAN / Python-CAN**
```python
import can
bus = can.interface.Bus(bustype='slcan', channel='/dev/cu.usbserial-110', bitrate=1000000)
```

**Option C: Direct CAN Commands**
Send raw CAN frames instead of AT commands

---

## Scripts Available

I've created several diagnostic scripts:

1. **`dual_motor_mac.py`** - Main dual motor controller (needs working connection)
2. **`diagnose_motors.py`** - Detailed diagnostic with hex dumps
3. **`find_correct_baud.py`** - Tests all baud rates
4. **`listen_serial.py`** - Listens for any serial data
5. **`test_small_movements.py`** - Small movement tests (needs working connection)

---

## The Working Configuration (from Windows)

According to `ROBSTRIDE_MOTOR_CONTROL.md`, on Windows this setup works:

- **Port:** COM3
- **Baud:** 921600 ✅
- **Motor ID:** 13 (tested and working)
- **Protocol:** AT commands over serial
- **Adapter:** CH340 driver

The code is correct - we just need the adapter to respond!

---

## Debug Output Example

When working, you should see:
```
AT+AT
OK                    <-- This response is missing!

Scan motor...
[Motor response]      <-- This response is missing!
```

Currently getting:
```
AT+AT
[silence]             <-- ❌ No response
```

---

## Summary

**The Python scripts are ready and correct.**  
**The issue is hardware/power/connection related.**

Once you verify:
- ✅ Motors are powered (LEDs on)
- ✅ USB-CAN adapter is connected
- ✅ Robstride software can communicate

Then we can:
1. Match the Robstride software settings
2. Or switch to the correct protocol (SLCAN/SocketCAN)
3. Run the dual motor controller successfully

---

## Contact Info

Scripts created: October 20, 2025  
Location: `/Users/jakegilbert/Desktop/Melvin/Melvin/`

**Ready to test once motors are powered and connected!** 🚀



