# Install CH340 Driver on macOS

## Why You Need This

The USB-CAN adapter uses a CH340 chip. Without the driver, the serial port appears (`/dev/cu.usbserial-110`) but **doesn't actually work** - which is exactly the issue we're seeing!

---

## Quick Install Steps

### Method 1: Homebrew (Recommended for macOS)

```bash
brew tap mengbo/ch340g-ch34g-ch34x-mac-os-x-driver https://github.com/mengbo/ch340g-ch34g-ch34x-mac-os-x-driver
brew install mengbo/ch340g-ch34g-ch34x-mac-os-x-driver/ch34xvcp
```

### Method 2: Manual Download

1. **Download the driver:**
   - Go to: https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
   - Click "Releases" → Download latest version for your macOS
   
   OR
   
   - Official WCH: https://www.wch.cn/downloads/CH341SER_MAC_ZIP.html

2. **Install:**
   - Open the downloaded `.pkg` file
   - Follow installation wizard
   - **IMPORTANT:** Click "Allow" when System Settings asks about system extension

3. **After Installation:**
   - Go to **System Settings** → **Privacy & Security**
   - Look for message about blocked system extension
   - Click **"Allow"** next to the CH34x extension
   - **Restart your Mac**

---

## Verify Installation

After restart, run:

```bash
kextstat | grep ch34
```

Should show something like:
```
com.wch.usbserial    (1.5.0)
```

---

## Then Test Motors

Once driver is installed and Mac is restarted:

```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
python3 dual_motor_mac.py
```

The motors should work! 🎉

---

## Common Issues

### "System Extension Blocked" Message
- Go to System Settings → Privacy & Security
- Scroll down, click "Allow" button
- Restart Mac

### Still Not Working After Install
- Unplug USB adapter
- Wait 5 seconds
- Plug back in
- Run: `ls /dev/cu.usbserial*` (should show device)

### Conflicting Drivers
If you have old CH340 drivers:
```bash
sudo rm -rf /Library/Extensions/usbserial.kext
sudo kextcache -i /
```
Then reinstall the new driver.

---

## Quick Test Command

After driver is installed and Mac restarted:

```bash
python3 -c "import serial; s=serial.Serial('/dev/cu.usbserial-110', 921600); s.write(b'AT+AT\r\n'); import time; time.sleep(0.3); print(s.read(s.in_waiting))"
```

Should output: `b'OK\r\n'`

---

## Status

- ❌ **Current:** Driver not installed - adapter not responding
- ✅ **After Install:** Driver working - motors will respond to commands

The Python scripts are ready to go! Just need the driver installed.



