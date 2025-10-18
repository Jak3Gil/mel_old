# Robstride Servo Studio - Debug Connection

## 🔍 **Systematic Connection Troubleshooting**

### **Step 1: Verify USB-CAN Adapter**

```bash
# Check if adapter is detected
ls -la /dev/cu.* /dev/tty.*

# You should see something like:
# /dev/cu.usbserial-XXXXX  or  /dev/cu.wchusbserial-XXXXX

# Test adapter responds:
python3 -c "
import serial, glob
port = glob.glob('/dev/cu.usbserial*')[0]
print(f'Testing: {port}')
ser = serial.Serial(port, 115200, timeout=1)
ser.write(b'V\r')  # Version command
import time; time.sleep(0.2)
resp = ser.read(100)
print(f'Response: {resp}')
ser.close()
"

# Expected: Some response (version string or data)
# If no response: Adapter might be faulty
```

---

### **Step 2: Check Adapter Driver**

**Mac:**
```bash
# Check system info
system_profiler SPUSBDataType | grep -A 10 "USB Serial"

# Common drivers needed:
# - CH340/CH341: github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
# - FTDI: ftdichip.com/drivers/vcp-drivers
# - CP2102: silabs.com/developers/usb-to-uart-bridge-vcp-drivers
```

**Windows:**
```
Device Manager → Ports (COM & LPT)
- Look for "USB Serial Port (COMX)"
- If yellow exclamation mark: Driver issue
- Right-click → Update Driver
```

**After installing driver: RESTART COMPUTER!**

---

### **Step 3: Close ALL Programs Using Port**

```bash
# Mac/Linux:
lsof | grep usbserial

# Kill any Python processes:
pkill -9 -f python
pkill -9 -f Python

# Unplug USB-CAN adapter
# Wait 10 seconds
# Plug back in
# Wait 5 seconds
```

---

### **Step 4: Try Servo Studio with Different Settings**

**Open Servo Studio and try EACH of these systematically:**

#### **Port Selection:**
- Try EVERY port in dropdown
- Don't just try the first one
- Some show as "COM3", "ttyUSB0", "/dev/cu.usbserial-XXX"

#### **Baudrate (try each):**
- 1000000 (1 Mbps) ← Try first
- 500000 (500 kbps)
- 250000 (250 kbps)  
- 115200
- 9600

#### **Protocol (try each):**
- SLCAN
- L91
- CANopen
- Auto
- CAN 2.0

#### **Motor ID:**
- Set to: 1
- Or: 0x01
- Or: 01

#### **Additional Settings:**
- Disable auto-scan if option exists
- Enable "Force Connect" if available
- Try "Manual Configuration"

---

### **Step 5: Servo Studio Specific Checks**

**Before Opening:**
1. Close ALL other programs
2. Disable antivirus temporarily
3. Run as Administrator (Windows) / sudo (Mac)

**In Servo Studio:**
1. Go to: Settings/Preferences/Configuration
2. Look for: Communication/Port/Adapter settings
3. Check: Any "adapter mode" or "protocol mode" switches
4. Look for: Firmware update option for adapter

---

### **Step 6: Try Different Servo Studio Versions**

Contact Robstride:
```
"Hi, Servo Studio v[X.X] won't connect to my motor.
 - What is the most stable version for [Mac/Windows]?
 - Can you provide download link for older versions?
 - My adapter: Generic SLCAN USB-CAN adapter
 - Motor: Robstride 03, CAN ID 0x01"
```

---

### **Step 7: Try on Different Computer**

**Priority order:**
1. **Windows PC** (if you're on Mac)
2. **Mac** (if you're on Windows)  
3. **Linux** (Ubuntu/Debian)
4. **Friend's computer**
5. **Work/School computer**

Some combinations just work better:
- Servo Studio often more stable on Windows
- Some adapters work better on Mac
- Linux sometimes has better USB-serial support

---

### **Step 8: Adapter Compatibility Test**

**Your adapter might not be compatible with Servo Studio.**

**Contact Robstride:**
```
"Which USB-CAN adapter models are officially supported by Servo Studio?
 
 Currently using: Generic SLCAN adapter
 OS: [Mac/Windows/Linux]
 Servo Studio Version: [X.X]
 
 Can you recommend/provide:
 - Specific adapter model that works
 - Purchase link for official adapter
 - Configuration guide for my adapter"
```

**Known Compatible Adapters:**
- PEAK PCAN-USB (expensive but reliable)
- Kvaser Leaf Light
- Canable.io adapters
- Official Robstride adapter (if they sell one)

---

### **Step 9: Check Servo Studio Logs**

**Look for error logs:**

**Mac:**
```bash
# Check console logs while trying to connect
log stream --predicate 'process == "ServoStudio"' --level debug

# Or check crash logs:
~/Library/Logs/
```

**Windows:**
```
C:\Users\[YourName]\AppData\Local\Robstride\
C:\ProgramData\Robstride\
Event Viewer → Application logs
```

**Look for:**
- Port access errors
- Driver errors
- Timeout messages
- Permission denied
- USB device errors

---

### **Step 10: Manual Port Test**

**Test if Servo Studio can see the port at all:**

```bash
# While Servo Studio is CLOSED, run:
python3 -c "
import serial
import glob

# Find port
ports = glob.glob('/dev/cu.*serial*') + glob.glob('/dev/tty.*serial*')
print('Available ports:')
for p in ports:
    print(f'  {p}')
    try:
        s = serial.Serial(p, 115200, timeout=1)
        print(f'    ✓ Can open')
        s.close()
    except Exception as e:
        print(f'    ✗ Error: {e}')
"
```

If Python can open but Servo Studio can't: **Permission issue**

**Mac - Fix permissions:**
```bash
sudo chmod 666 /dev/cu.usbserial*
```

**Linux - Add user to dialout group:**
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

---

### **Step 11: Capture Communication**

**See what Servo Studio is trying to send:**

```bash
# Mac - Install Wireshark or use tcpdump
# This won't work for USB-serial, but we can monitor with Python:

# Run this BEFORE opening Servo Studio:
python3 -c "
import serial
import time

port = '/dev/cu.usbserial-110'  # Your port
ser = serial.Serial(port, 115200, timeout=0.1)

print('Monitoring port... Open Servo Studio now.')
print('Press Ctrl+C to stop')
print()

try:
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            print(f'Received: {data.hex()}')
        time.sleep(0.1)
except KeyboardInterrupt:
    print('Stopped')
    ser.close()
"

# This will show if Servo Studio is even trying to communicate
```

---

### **Step 12: Virtual Machine Option**

**If nothing else works:**

1. **Install VirtualBox** (free)
2. **Create Windows VM** (or Linux)
3. **Install Servo Studio in VM**
4. **Connect USB adapter to VM**
5. **Try again in clean environment**

Sometimes VM has better USB passthrough than native.

---

### **Step 13: Alternative Tools**

**Try other CAN software to verify adapter works:**

**Python-CAN:**
```bash
pip3 install python-can

python3 -c "
import can

# Try to create bus
bus = can.interface.Bus(
    bustype='slcan',
    channel='/dev/cu.usbserial-110',
    bitrate=1000000
)
print('✓ python-can can open adapter')
bus.shutdown()
"
```

**If this works but Servo Studio doesn't:** 
→ Servo Studio compatibility issue with your adapter

---

### **Step 14: Contact Robstride - Detailed Report**

**Send them this:**

```
Subject: Servo Studio Connection Issue - Detailed Report

Motor: Robstride 03
OS: [Mac/Windows/Linux + version]
Servo Studio Version: [X.X.X]
USB-CAN Adapter: Generic SLCAN adapter
Port: /dev/cu.usbserial-110 (Mac) or COM3 (Windows)

VERIFIED WORKING:
• Adapter communicates (tested with Python)
• Motor responds on CAN bus (CAN ID 0x01)
• Python scripts can send/receive CAN commands
• Motor acknowledges all commands
• Blue LED on adapter confirms transmission

SERVO STUDIO ISSUE:
• Won't connect to motor
• Tried all ports in dropdown: [list them]
• Tried baudrates: 1000000, 500000, 250000, 115200
• Tried protocols: SLCAN, CANopen, Auto
• Tried on [multiple computers/OSes]
• Checked/installed drivers: [which ones]
• Run as Administrator/sudo: Yes
• Antivirus disabled: Yes
• No other programs using port: Confirmed

ERROR MESSAGES: 
[Include any error messages or "Connection failed" dialogs]

QUESTIONS:
1. Which USB-CAN adapter models are officially supported?
2. Are there specific driver requirements?
3. Is there a diagnostic mode in Servo Studio?
4. Can you provide connection logs from working setup?
5. Is there alternative software to activate position mode?

LOGS ATTACHED:
[If you found any log files]

Thank you for your help!
```

---

## 🎯 **Most Likely Issues:**

### **80% Chance: Adapter Incompatibility**
- Your adapter works for basic CAN
- But Servo Studio needs specific adapter
- Solution: Get official/recommended adapter

### **15% Chance: Driver/Permission Issue**
- Correct driver not installed
- Or wrong version
- Or insufficient permissions
- Solution: Reinstall driver, restart, check permissions

### **5% Chance: Servo Studio Bug**
- Software version incompatibility
- Bug on your OS version
- Solution: Try different version, different OS

---

## ✅ **Success Checklist:**

Once Servo Studio connects, you'll see:
- ✓ "Connected" or "Online" status
- ✓ Motor parameters populate in interface
- ✓ Real-time position/status updates
- ✓ Ability to send test movements
- ✓ Configuration/settings become available

Then:
1. Find "Mode" or "Control Mode" setting
2. Change to "Position Mode" or "MIT Mode"
3. Save/Write to motor
4. Motor might restart (LEDs will blink)
5. Test movement in Servo Studio
6. If works in Studio → Python will work too!

---

**Don't give up! This is solvable. It's just a software connection issue.**

