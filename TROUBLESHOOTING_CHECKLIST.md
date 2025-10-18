# Robstride Motor Troubleshooting Checklist
## Motor Worked Yesterday, Now Doesn't Connect to Anything

---

## 🔴 CRITICAL CHECKS (Do These First!)

### 1. Motor Power
- [ ] Is motor powered ON?
- [ ] Check power supply voltage with multimeter
- [ ] Is power supply still outputting correct voltage?
- [ ] Check power connections - any loose wires?
- [ ] Try different power outlet

**LED Check:**
- Red LED status: ________________
- Blue LED status: ________________

### 2. Physical Connections
Walk through each connection:

**At Motor:**
- [ ] CANH wire still connected firmly?
- [ ] CANL wire still connected firmly?
- [ ] GND wire still connected?
- [ ] Power wires (+ and -) still connected?

**At USB-CAN Adapter:**
- [ ] CANH wire still connected?
- [ ] CANL wire still connected?
- [ ] GND wire still connected?
- [ ] USB cable plugged in?

### 3. What Changed Yesterday?
Think carefully:
- [ ] Did you move the motor?
- [ ] Did you disconnect/reconnect anything?
- [ ] Did power go out?
- [ ] Did you change any settings in software?
- [ ] Did motor get hot during testing?
- [ ] Did you power cycle it many times?

---

## 🔧 SYSTEMATIC TESTS

### Test 1: Motor Power
```bash
Turn motor power OFF
Wait 10 seconds
Turn motor power ON
Watch LEDs carefully for 30 seconds
```

**What do LEDs do when powering on?**
- Immediately after power on: ________________
- After 5 seconds: ________________
- After 30 seconds: ________________

**Normal bootup should be:**
- Red LED: Solid (means powered)
- Blue LED: Flashing or solid (means ready)

### Test 2: USB Adapter
```bash
# Unplug USB adapter from Mac
# Wait 10 seconds
# Plug back in
# Check if it appears:
ls -la /dev/cu.usbserial*
```

**Result:** ________________

### Test 3: CAN Bus Voltage
**With multimeter, measure:**
- CANH to GND: Should be ~3.5V: ______ V
- CANL to GND: Should be ~1.5V: ______ V
- CANH to CANL: Should be ~2.0V: ______ V

**If voltages are wrong:** CAN bus problem

### Test 4: Try Different CAN ID
Maybe motor ID changed:

```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
python3 scan_for_motor.py
```

This scans ALL CAN IDs to find motor.

---

## 🚨 COMMON CAUSES

### Cause 1: Motor in Fault State
**Symptom:** Red LED flashing pattern
**Fix:** 
1. Power OFF motor
2. Wait 60 seconds
3. Power ON motor
4. If still in fault, needs factory reset

### Cause 2: Wire Came Loose
**Symptom:** Everything looks connected but doesn't work
**Fix:**
1. Unscrew each terminal
2. Check wire condition (not broken/frayed)
3. Re-insert wire firmly
4. Tighten terminal screw

### Cause 3: Motor Protection Activated
**Symptom:** Motor was working, suddenly stopped
**Fix:**
- Motor may have overheated
- Or detected overcurrent
- Needs power cycle + wait 5 minutes to cool

### Cause 4: USB-CAN Adapter Issue
**Symptom:** Can't see /dev/cu.usbserial*
**Fix:**
1. Try different USB port
2. Try different USB cable
3. Restart Mac

### Cause 5: CAN Bus Termination Problem
**Symptom:** Worked before, suddenly stopped
**Fix:**
- Check 120Ω resistor still connected
- Resistor might have come loose
- Or broken

---

## 🔍 DIAGNOSTIC COMMANDS

### Check if USB adapter visible:
```bash
ls -la /dev/cu.usb*
system_profiler SPUSBDataType | grep -i serial
```

### Test CAN communication:
```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
python3 test_adapter.py
```

### Scan for motor at any ID:
```bash
python3 scan_for_motor.py
```

### Full diagnostic:
```bash
python3 full_diagnosis.py
```

---

## 💡 SPECIFIC SCENARIOS

### Scenario A: PC Software Can't Connect Either
**Likely cause:** 
- Motor power issue
- Or CAN wiring issue
- NOT a software issue

**Steps:**
1. Check motor has power (LED on)
2. Check all physical wires
3. Power cycle motor
4. Try on PC again

### Scenario B: USB Adapter Not Showing Up
**Likely cause:** USB issue

**Steps:**
1. Try different USB port
2. Unplug/replug
3. Restart computer
4. Check cable

### Scenario C: Everything Looks Fine But Doesn't Work
**Likely cause:** Motor in fault/protection mode

**Steps:**
1. Power OFF motor
2. Wait 5 minutes (let it cool)
3. Power ON motor
4. Watch LED pattern carefully
5. Look for error blink codes

---

## 📋 NEXT STEPS

### If Motor Power LED is OFF:
→ Power supply problem

### If Motor LEDs Show Error Pattern:
→ Motor in fault state, needs reset

### If No USB Device Found:
→ USB adapter or cable problem

### If Everything Connected But No Communication:
→ CAN wiring or termination issue

---

## 🎯 WHAT TO CHECK RIGHT NOW

**Answer these questions:**

1. **Motor Power LED (Red):**
   - [ ] Solid
   - [ ] Flashing
   - [ ] OFF
   - [ ] Pattern: ________________

2. **Motor Status LED (Blue):**
   - [ ] Solid
   - [ ] Flashing slowly
   - [ ] Flashing fast
   - [ ] OFF
   - [ ] Pattern: ________________

3. **Can you see USB device?**
   ```bash
   ls -la /dev/cu.usb*
   ```
   - [ ] YES - shows device
   - [ ] NO - no device found

4. **Physical check:**
   - [ ] All wires tight at motor end
   - [ ] All wires tight at adapter end
   - [ ] 120Ω resistor still connected

---

## 🔑 KEY INSIGHT

If motor worked yesterday and both:
- Python code can't connect
- PC software can't connect

Then it's **100% a hardware issue**, not software:
- Power
- Wiring  
- Motor fault state
- CAN bus termination

**Start with LED check and power cycle!**




