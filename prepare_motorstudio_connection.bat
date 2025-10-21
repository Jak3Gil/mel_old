@echo off
echo ============================================================
echo PREPARING FOR MOTOR STUDIO CONNECTION
echo ============================================================
echo.

echo Step 1: Killing any Python processes...
taskkill /F /IM python.exe >nul 2>&1
if %errorlevel%==0 (
    echo [OK] Closed Python processes
) else (
    echo [OK] No Python processes were running
)
echo.

echo Step 2: Waiting for COM port to be fully released...
timeout /t 3 /nobreak >nul
echo [OK] Ready
echo.

echo ============================================================
echo NOW DO THESE STEPS:
echo ============================================================
echo.
echo 1. Unplug the USB-CAN adapter from your PC
echo    Press any key when done...
pause >nul
echo    [OK] Adapter unplugged
echo.

echo 2. Turn OFF the motor power supply (12V supply)
echo    Press any key when done...
pause >nul
echo    [OK] Motor power OFF
echo.

echo 3. Wait 10 seconds...
timeout /t 10 /nobreak
echo    [OK] Reset complete
echo.

echo 4. Turn ON the motor power supply (12V supply)
echo    Press any key when motor LEDs are on...
pause >nul
echo    [OK] Motor powered
echo.

echo 5. Plug in the USB-CAN adapter to PC
echo    Press any key when plugged in...
pause >nul
echo    [OK] Adapter connected
echo.

echo 6. Wait for Windows to recognize device...
timeout /t 5 /nobreak
echo    [OK] Device should be ready
echo.

echo ============================================================
echo READY TO CONNECT!
echo ============================================================
echo.
echo Motor Studio Settings:
echo   - COM Port: COM3
echo   - Baudrate: 921600
echo   - Protocol: SLCAN or L91
echo.
echo Now open Motor Studio and click Connect!
echo.
pause

