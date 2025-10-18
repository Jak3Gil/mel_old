# Robstride Motor Power Diagnosis
## LEDs Work But Motor Has Zero Response

---

## 🔴 CRITICAL CLUE: Two Power Circuits!

Robstride motors typically have **TWO power systems**:

1. **Logic Power (5V-12V)**: Powers LEDs, processor, CAN interface
   - ✓ WORKING (LEDs are on, CAN works)

2. **Motor Driver Power (24-48V)**: Powers motor coils
   - ❌ POSSIBLY NOT WORKING (no sound/vibration/movement)

---

## 🔧 POWER SUPPLY CHECKS

### Check 1: Voltage at Motor Terminals
**With multimeter, measure DC voltage:**

**At motor power input:**
- V+ to GND: Should be 24-48V (check your power supply spec)
- Your reading: ______ V

**Expected voltages for Robstride 03:**
- Typical: 24V or 48V
- Minimum: 18V
- Maximum: 50V

**If voltage is low (<18V):** Power supply problem!

### Check 2: Power Supply Current
**Check power supply specifications:**
- Voltage output: ______ V
- Maximum current: ______ A
- Is LED on power supply lit? ______

**Robstride 03 requirements:**
- Voltage: 24-48V DC
- Current: Minimum 3-5A, peak can be 10-20A

**If power supply is <5A:** May not have enough current!

### Check 3: Power Supply Under Load
**Test sequence:**
1. Measure voltage with motor idle: ______ V
2. Run motor command (python script)
3. Measure voltage while commands running: ______ V

**If voltage drops significantly:** Power supply can't handle load!

### Check 4: Power Connections
**Physical inspection:**
- [ ] V+ wire firmly connected to motor?
- [ ] GND wire firmly connected to motor?
- [ ] Power supply output terminals tight?
- [ ] Any loose connections?
- [ ] Wires intact (not broken/frayed)?

---

## 🚨 COMMON POWER ISSUES

### Issue 1: Power Supply Too Weak
**Symptoms:**
- LEDs work (they need <1W)
- Motor doesn't work (needs 50-500W)
- No sound/vibration

**Solution:**
- Check power supply rating
- Robstride 03 needs: 24-48V, 5-10A minimum
- If power supply is <5A, it's too weak!

### Issue 2: Motor Power Disabled
**Symptoms:**
- Logic works, motor doesn't
- No current draw from power supply

**Possible causes:**
- Motor driver in safety lock
- Overcurrent protection triggered
- Motor needs enable via PC software

**Test:** Measure current draw
- Idle: <0.5A (just logic)
- Commanding: Should be 1-5A
- If stays <0.5A: Motor driver not engaging!

### Issue 3: Blown Fuse or Protection
**Some motors have:**
- Internal fuse
- Overcurrent protection
- Overheat protection

**If triggered yesterday:**
- Motor locks out until reset
- Needs power cycle or special reset

### Issue 4: Power Polarity
**Double check:**
- Is V+ connected to V+ terminal?
- Is GND connected to GND terminal?
- Reversed polarity = protection mode activated

---

## 🔬 DIAGNOSTIC TESTS

### Test 1: Current Draw Test
**With multimeter in series (or clamp meter):**

1. Measure current draw when idle: ______ A
   - Expected: 0.1-0.5A (logic only)

2. Run python script and measure current: ______ A
   - Expected: 2-10A (motor trying to move)
   - If <0.5A: Motor driver not engaging!

### Test 2: Power Supply Load Test
**Temporarily connect a known load:**
- If you have a 12V light bulb or fan
- Connect to power supply
- Does it work? ______

**If load doesn't work:** Power supply is dead/broken!

### Test 3: Alternative Power Source
**If you have another power supply:**
- Same voltage (24V or 48V)
- At least 5A capacity
- Try powering motor with it
- Does it work? ______

**If works with different supply:** Original supply is faulty!

---

## 💡 WHAT TO CHECK RIGHT NOW

### Priority 1: Measure Voltage
```
1. Get multimeter
2. Set to DC voltage
3. Measure at motor V+ and GND terminals
4. Should read 24-48V (whatever your supply is rated)
5. If <18V or 0V: Power supply problem!
```

### Priority 2: Check Power Supply Rating
```
Look at power supply label:
- Output voltage: _____ V
- Output current: _____ A

Robstride 03 needs:
- 24-48V
- Minimum 5A (10A+ recommended)

If your supply is <5A: TOO WEAK!
```

### Priority 3: Look for Protection Indicators
```
On motor or power supply:
- Any LED indicators showing fault/error?
- Red blinking LED?
- Fuse visible (blown)?
```

---

## 🎯 LIKELY SCENARIOS

### Scenario A: Power Supply Too Weak
**Evidence:**
- LEDs work
- CAN works  
- No motor response
- Supply is <5A rated

**Solution:** Get stronger power supply (10A+)

### Scenario B: Motor Driver Disabled
**Evidence:**
- Voltage correct (24-48V)
- Current draw stays <0.5A even when commanding
- No sound/vibration

**Solution:** Needs Robstride PC software to enable

### Scenario C: Protection Mode Active
**Evidence:**
- Was working yesterday
- Suddenly stopped
- No response to any commands

**Solution:** Power OFF 5 minutes, check for reset procedure

### Scenario D: Hardware Fault
**Evidence:**
- Voltage correct
- Everything else checks out
- But still no response

**Solution:** Motor may need repair/replacement

---

## 📋 ACTION PLAN

**Step 1:** Measure voltage at motor terminals
- If <18V: Power supply issue
- If >18V: Go to Step 2

**Step 2:** Check power supply rating
- If <5A: Too weak, need stronger supply
- If >5A: Go to Step 3

**Step 3:** Measure current during operation
- Run script, measure current
- If <0.5A: Motor driver not engaging
- If >1A: Driver working, other issue

**Step 4:** Based on measurements:
- Bad power supply → Replace
- Low current draw → Needs PC software unlock
- Everything checks out → Hardware fault

---

## 🔑 KEY QUESTIONS TO ANSWER

1. **What voltage does your power supply output?**
   - ______ V

2. **What is power supply current rating?**
   - ______ A

3. **Voltage measured at motor terminals:**
   - ______ V

4. **Current draw when idle:**
   - ______ A

5. **Current draw when commanding motor:**
   - ______ A

---

## 💡 QUICK TEST

**If you don't have multimeter:**
- When running commands, does power supply get warm/hot?
- Does motor get warm after 1 minute of commands?
- Does power supply LED dim when commanding?

**If NO to all:** Power not reaching motor driver!




