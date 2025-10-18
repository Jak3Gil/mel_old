# Windows Setup - Quick Start

## 🚨 **Current Issue: Windows Can't Connect to CAN**

Your Mac successfully connects to the motor, but Windows can't. This is a **Windows driver issue**, not a motor problem.

---

## ✅ **Quick Fix - Start Here:**

### **1. Install Python Dependencies**
```cmd
pip install pyserial pyyaml
```

### **2. Run Diagnostic**
```cmd
python windows_can_test.py
```

This will tell you exactly what's wrong!

---

## 📋 **Most Likely Issues & Fixes:**

### **Issue 1: No COM Ports Found**

**Symptom:** Diagnostic says "No COM ports found"

**Fix:**
1. Open Device Manager (`Win + X`)
2. Install CH340 driver: https://www.wch-ic.com/downloads/CH341SER_EXE.html
3. Restart computer
4. Run `windows_can_test.py` again

---

### **Issue 2: Port Won't Open**

**Symptom:** Diagnostic sees ports but can't open them

**Fix:**
1. Close ALL programs that might use serial:
   - Close Servo Studio
   - Close other Python scripts
   - Check Task Manager for `python.exe`
2. Or just restart computer (easiest)
3. Run `windows_can_test.py` again

---

### **Issue 3: Port Opens But No Motor Response**

**Symptom:** Diagnostic opens port successfully but motor doesn't respond

**This is GOOD!** It means:
- ✓ Windows CAN communication works
- ✓ Same issue as Mac (motor needs position mode enabled)
- ✓ Ready for Motor Studio

**Next:** Use Motor Studio to enable position mode

---

## 🎯 **Step-by-Step Windows Setup:**

### **Step 1: Get CAN Working**
```cmd
# Run diagnostic
python windows_can_test.py

# Keep running until you see:
# "✓ Successfully opened COM3" (or similar)
```

**If it fails:** See `WINDOWS_CAN_CONNECTION_FIX.md`

---

### **Step 2: Download Motor Studio**

Search: "Robstride Motor Studio Windows download"

Or contact Robstride support:
- Ask: "Where to download Motor Studio for Windows?"
- Mention: You have Robstride 03 motor

---

### **Step 3: Connect in Motor Studio**

**Settings:**
- **Port:** The COM port that worked in diagnostic (e.g., COM3)
- **Baudrate:** 1000000 (try 500000 if that fails)
- **Protocol:** SLCAN or L91
- **Motor ID:** 1

Click "Connect" or "Scan"

---

### **Step 4: Enable Position Mode**

Once connected:
1. Find "Mode" or "Control Mode" setting
2. Change to **"Position Mode"** or **"MIT Mode"**
3. Click **"Save"** or **"Write to Motor"**
4. Motor may restart (LEDs blink)

---

### **Step 5: Test with Python**

```cmd
python test_obvious_movement.py
```

**If motor moves:** 🎉 **SUCCESS!**

---

## 📁 **All Files in This Repo:**

### **For Diagnosis:**
- `windows_can_test.py` - **START HERE!**
- `WINDOWS_CAN_CONNECTION_FIX.md` - Detailed troubleshooting

### **For Setup:**
- `WINDOWS_PC_SETUP.md` - Complete Windows guide
- `motor_config.yaml` - Motor configuration

### **For Testing:**
- `test_obvious_movement.py` - Large movement test
- `test_kscale_approach.py` - Full test suite
- `example_kscale_usage.py` - Usage examples

### **Motor Control Code:**
- `motor_interface.py` - Abstract interface
- `robstride_kscale.py` - Implementation
- `motor_factory.py` - Factory pattern

---

## 🔍 **Why Mac Works But Windows Doesn't:**

| System | Status | Why |
|--------|--------|-----|
| **Mac** | ✓ Works | Drivers built-in |
| **Windows** | ✗ Broken | Needs manual driver |

**Solution:** Install CH340 driver on Windows → restart → works!

---

## 💡 **Common Windows Errors:**

### **"No module named 'serial'"**
```cmd
pip install pyserial
```

### **"Port COM3 not found"**
- Wrong COM port number
- Run `windows_can_test.py` to find correct one

### **"Access is denied"**
- Another program using port
- Restart computer or run as Administrator

### **"Device not recognized"**
- Driver not installed
- Install CH340 driver
- Restart computer

---

## 🎯 **TL;DR - Three Commands:**

```cmd
# 1. Install dependencies
pip install pyserial pyyaml

# 2. Diagnose issue
python windows_can_test.py

# 3. If it works, test motor
python test_obvious_movement.py
```

---

## 📞 **Need Help?**

1. **Run diagnostic first:** `python windows_can_test.py`
2. **Read the results:** It tells you exactly what's wrong
3. **Follow the fix:** Usually just installing driver
4. **Still stuck?** See `WINDOWS_CAN_CONNECTION_FIX.md`

---

## ✅ **Success Indicators:**

You'll know Windows CAN is working when:
- ✓ `windows_can_test.py` shows "✓ Successfully opened COM3"
- ✓ SLCAN initialization completes without errors
- ✓ Motor responds with hex data (even if 0xFF)

Then you're ready for Motor Studio!

---

**Remember: The code works, the motor works, you just need to fix the Windows driver issue first!** 🚀

