#!/bin/bash
# Test if CH340 driver is working after installation

echo "======================================================================"
echo "   CH340 Driver Test"
echo "======================================================================"
echo ""

echo "[1] Checking if driver is loaded..."
if kextstat | grep -i ch34 > /dev/null 2>&1; then
    echo "    ✓ CH340 driver is loaded!"
else
    echo "    ✗ Driver not loaded yet"
    echo "    Did you restart your Mac?"
fi

echo ""
echo "[2] Checking USB serial ports..."
ls -la /dev/cu.usbserial* 2>/dev/null || echo "    No USB serial ports found"

echo ""
echo "[3] Testing communication at 921600 baud..."
cd /Users/jakegilbert/Desktop/Melvin/Melvin
python3 find_correct_baud.py

echo ""
echo "======================================================================"
echo "If you see '✓ SUCCESS!' above, the driver is working!"
echo "Then run: python3 dual_motor_mac.py"
echo "======================================================================"


