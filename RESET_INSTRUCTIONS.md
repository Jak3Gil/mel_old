# How to Factory Reset Robstride Motor

## 🔧 Method 1: Physical Reset Button (RECOMMENDED)

### Step-by-Step:

1. **POWER OFF the motor**
   - Disconnect power supply completely

2. **FIND the reset button:**
   - Look on the motor body near:
     - Power connector
     - CAN port  
     - Motor label/sticker area
   - Button might be:
     - Small recessed hole (need paperclip/pin)
     - Tiny button labeled "RESET" or "R"
     - Under a rubber cap

3. **RESET PROCEDURE:**
   ```
   a. Keep power DISCONNECTED
   b. Press and HOLD reset button
   c. While HOLDING button, reconnect power
   d. Keep holding for 10-15 seconds
   e. Watch LEDs - they should blink/change pattern
   f. Release button when LEDs change
   ```

4. **POWER CYCLE:**
   ```
   - Power OFF motor
   - Wait 30 seconds
   - Power ON motor
   - Wait for LEDs to stabilize (10 seconds)
   ```

5. **TEST:**
   ```bash
   cd /Users/jakegilbert/Desktop/Melvin/Melvin
   python3 robstride_motor.py
   ```

---

## 🔄 Method 2: Multiple Power Cycles

If no reset button is found, try this:

```
1. Power OFF - Wait 60 seconds
2. Power ON - Wait 10 seconds  
3. Power OFF - Wait 60 seconds
4. Power ON - Wait 10 seconds
5. Power OFF - Wait 60 seconds
6. Power ON - Leave it on
7. Wait for LEDs to stabilize
8. Test with Python script
```

This sometimes triggers automatic factory reset on smart motors.

---

## 🌐 Method 3: CAN Command Reset

Run this Python script:

```bash
python3 -c "
import serial, time, glob

port = glob.glob('/dev/cu.usbserial*')[0]
ser = serial.Serial(port, 115200, timeout=1.0)
time.sleep(0.3)

# Init
ser.write(b'C\r')
time.sleep(0.1)
ser.read(1000)
ser.write(b'L91\r')
time.sleep(0.1)
ser.read(1000)
ser.write(b'O\r')
time.sleep(0.1)
ser.read(1000)

# Try reset commands
ser.write(b'T0A0000010\r')  # Reset
time.sleep(0.5)
ser.write(b'T0C0000010\r')  # Clear faults
time.sleep(0.5)

ser.close()
print('Reset commands sent. Power cycle motor now.')
"
```

After running, **power cycle the motor** (OFF 30s, then ON).

---

## 📋 After Reset - What to Expect:

### LED Behavior:
- **Red LED**: Should be solid (motor powered)
- **Blue LED**: May change from flashing to solid after reset

### Testing:
Run the test script:
```bash
python3 robstride_motor.py
```

**If motor moves**: ✅ Success!  
**If still doesn't move**: Motor may need Robstride configuration software

---

## 🚨 If Nothing Works:

The motor may need:

1. **Encoder Calibration** via Robstride PC software
   - This CANNOT be done via CAN alone
   - Requires their official tool

2. **Firmware Update/Recovery**
   - May need special hardware programmer
   - Contact Robstride support

3. **Hardware Issue**
   - Encoder might be damaged
   - Need replacement or repair

---

## 📞 Robstride Support Info:

**When contacting support, tell them:**
- Motor Model: Robstride 03
- Issue: Motor responds to CAN commands (getting 0xFF responses) but doesn't execute them
- Motor moves freely by hand (no mechanical issue)
- Using L91 protocol initialization
- Tried factory reset
- Need: Encoder calibration procedure or configuration tool

---

## 💡 Quick Reference:

```bash
# Test if motor is communicating:
python3 motor_diagnostic.py

# Test motor movement:
python3 robstride_motor.py

# Full diagnosis:
python3 full_diagnosis.py
```

---

**Next Step:** Try Method 1 (Physical Reset Button) first!

