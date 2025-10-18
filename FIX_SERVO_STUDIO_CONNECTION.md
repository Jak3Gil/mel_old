# Fix Robstride Servo Studio Connection

## 🔍 **What We've Confirmed:**

✅ Motor is powered (LEDs on)  
✅ Motor is on CAN bus (responds to commands)  
✅ USB-CAN adapter works (blue LED lights up)  
✅ CAN wiring is correct (getting responses)  
✅ Motor CAN ID is 0x01  
❌ Motor won't execute position commands (needs mode activation)

**Conclusion:** Motor firmware requires PC software to enable position control mode.

---

## 🛠️ **Step-by-Step: Fix Servo Studio Connection**

### **Step 1: Close Everything**

```bash
# Kill any Python scripts using the port
pkill -f python
pkill -f Python

# Wait 5 seconds, then unplug USB-CAN adapter
# Wait 5 seconds, then plug it back in
```

### **Step 2: Find Your COM Port**

**On Mac:**
```bash
ls -la /dev/cu.*
```
Look for: `/dev/cu.usbserial-XXXXX` or `/dev/cu.wchusbserial-XXXXX`

**On Windows:**
- Open Device Manager
- Expand "Ports (COM & LPT)"
- Note the COM port number (e.g., COM3, COM5)

### **Step 3: Check USB-CAN Adapter Drivers**

Your adapter type determines the driver needed:

**CH340/CH341 adapters** (most common cheap adapters):
- Mac: https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
- Windows: https://www.wch-ic.com/downloads/CH341SER_EXE.html

**FTDI adapters** (better quality):
- https://ftdichip.com/drivers/vcp-drivers/

**After installing driver: RESTART YOUR COMPUTER**

### **Step 4: Configure Servo Studio**

When you open Robstride Servo Studio:

1. **COM Port Selection:**
   - Try EVERY port in the dropdown
   - On Mac: Select the `/dev/cu.usbserial-XXXXX` one
   - On Windows: Try COM3, COM4, COM5, etc. systematically

2. **Baudrate:**
   - Try: `1000000` (1 Mbps) first
   - If that fails: `500000` (500 kbps)
   - If that fails: `250000` (250 kbps)

3. **Protocol:**
   - Try: `SLCAN`
   - If option exists: `L91`
   - Or: `CANopen`

4. **Motor ID:**
   - Set to: `1` or `0x01`

5. **Click "Connect" or "Scan"**

### **Step 5: If Still Won't Connect - Alternative Methods**

**A) Try Different CAN Adapter:**
- Your current adapter might not be compatible
- Robstride may require specific adapter
- Ask Robstride support: "Which USB-CAN adapter works with Servo Studio?"

**B) Try Robstride Mobile App:**
- Check iOS App Store or Google Play
- Search: "RobStride" or "Robstride Servo"
- Mobile app might use Bluetooth instead of CAN
- Some motors have dual interfaces

**C) Use Windows Instead of Mac (if on Mac):**
- Servo Studio might work better on Windows
- Try on different computer
- Or use Windows VM on Mac

**D) Try Older Version of Servo Studio:**
- Newer versions might have bugs
- Ask Robstride for previous version downloads
- Try v1.0, v2.0 systematically

---

## 📞 **Contact Robstride Support**

**Email template you can send:**

```
Subject: Robstride 03 Motor - Servo Studio Won't Connect

Hello,

I have a Robstride 03 motor that I'm trying to configure using 
Servo Studio, but the software won't connect.

Hardware Setup:
- Motor: Robstride 03
- Power: 12V 80A supply
- Connection: USB-CAN adapter (SLCAN compatible)
- Motor LEDs: Solid Red + Blinking Blue (appears ready)
- CAN ID: 0x01

What I've Confirmed:
- Motor responds to CAN commands (I can see responses)
- USB-CAN adapter works (tested with Python scripts)
- Motor won't execute position commands
- Need to enable Position Control Mode / MIT Mode

Problem:
- Servo Studio won't connect to the motor
- Tried all COM ports, different baudrates, protocols

Questions:
1. Which USB-CAN adapter model is officially supported?
2. What are the correct connection settings (baudrate, protocol)?
3. Is there an alternative way to enable position mode?
4. Can you provide step-by-step connection instructions?
5. Is there a mobile app alternative?

My Setup:
- OS: [Mac/Windows/Linux]
- Servo Studio Version: [version]
- USB-CAN Adapter: [model if known]

Thank you for your help!
```

---

## 🔧 **Advanced: Check If Adapter is Actually Working**

Run this to verify adapter responds properly:

```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
python3 -c "
import serial, glob
port = glob.glob('/dev/cu.usbserial*')[0]
print(f'Port: {port}')
ser = serial.Serial(port, 115200, timeout=1)
ser.write(b'V\r')  # Version command
import time; time.sleep(0.2)
print(f'Adapter response: {ser.read(100)}')
ser.close()
"
```

**Expected:** Should return version string like `V1301` or similar  
**If no response:** Adapter might be faulty or wrong baudrate

---

## 🎯 **What Happens After You Connect?**

Once Servo Studio connects, you need to:

1. **Read motor parameters** (should auto-load)
2. **Find "Mode" or "Control Mode" setting**
3. **Change to "Position Mode" or "MIT Mode"**
4. **Save configuration to motor**
5. **Motor may need to restart**
6. **Test with Servo Studio first** (should be able to move motor)
7. **Then Python code will work!**

---

## 📋 **Quick Checklist**

Before trying Servo Studio again:

- [ ] Closed all Python scripts
- [ ] Unplugged and replugged USB-CAN adapter  
- [ ] Installed correct driver for adapter
- [ ] Restarted computer
- [ ] Motor is powered ON (12V supply on)
- [ ] Checked all COM ports in dropdown
- [ ] Tried different baudrates (1000000, 500000, 250000)
- [ ] Tried different protocols (SLCAN, CANopen)
- [ ] Motor ID set to 1
- [ ] Running Servo Studio as Administrator (Windows)

---

## 💡 **Important Note**

**This is NOT unusual!** Many smart motors (Robstride, Dynamixel, etc.) 
require initial configuration via manufacturer software before they 
accept position commands. This is a security/safety feature.

Once you get past this ONE-TIME setup, your Python K-Scale architecture 
code will work perfectly for all future control!

---

## 🆘 **If Nothing Works**

Last resort options:

1. **Buy Official Robstride Adapter:**
   - They may sell USB-CAN adapter that's guaranteed compatible
   - Worth the investment if you're stuck

2. **Send Motor Back:**
   - If under warranty, ask them to pre-configure it
   - Request they enable MIT/Position mode before shipping

3. **Find Someone with Working Setup:**
   - Local robotics community?
   - University lab with Robstride motors?
   - They can configure it for you

4. **Try Linux:**
   - Some adapters work better on Linux
   - Live USB boot if you don't have Linux installed

---

## ✅ **Success Indicators**

You'll know Servo Studio connected when:
- Connection status changes to "Connected" or "Online"
- Motor parameters load and display
- You can see motor position/status update in real-time
- Motor responds to test movements in software

---

**Good luck! Once you get connected, it's a 5-minute configuration process!**

