#!/bin/bash
#
# Deploy MELVIN motor control to Jetson
#

set -e

JETSON_USER="melvin"
JETSON_HOST="192.168.55.1"
JETSON_PASS="123456"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN MOTOR CONTROL - JETSON DEPLOYMENT         ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

echo "📦 Creating motor control package..."
tar czf /tmp/melvin_motors.tar.gz \
    core/motor/ \
    test_motors.cpp

echo "📤 Transferring to Jetson..."
sshpass -p "$JETSON_PASS" scp /tmp/melvin_motors.tar.gz $JETSON_USER@$JETSON_HOST:/home/melvin/MELVIN/

echo "📥 Extracting on Jetson..."
sshpass -p "$JETSON_PASS" ssh $JETSON_USER@$JETSON_HOST "cd /home/melvin/MELVIN && tar xzf melvin_motors.tar.gz && rm melvin_motors.tar.gz"

echo ""
echo "🔧 Building on Jetson..."
sshpass -p "$JETSON_PASS" ssh $JETSON_USER@$JETSON_HOST << 'REMOTE'
cd /home/melvin/MELVIN

# Build motor test
echo "Compiling motor control..."
g++ -std=c++17 -O2 -pthread -I. \
    test_motors.cpp \
    core/motor/can_motor_controller.cpp \
    -o bin/test_motors

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    ls -lh bin/test_motors
else
    echo "❌ Build failed"
    exit 1
fi

# Configure CAN bus
echo ""
echo "🔧 Configuring CAN bus..."
echo "123456" | sudo -S modprobe can 2>/dev/null || true
echo "123456" | sudo -S modprobe can_raw 2>/dev/null || true
echo "123456" | sudo -S modprobe mttcan 2>/dev/null || true

echo "123456" | sudo -S ip link set can0 down 2>/dev/null || true
echo "123456" | sudo -S ip link set can0 type can bitrate 500000
echo "123456" | sudo -S ip link set can0 up

if ip link show can0 > /dev/null 2>&1; then
    echo "✅ CAN bus configured"
    ip -details link show can0 | grep -i "can\|bitrate"
else
    echo "⚠️  CAN bus not available"
fi

REMOTE

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     DEPLOYMENT COMPLETE                              ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ Motor control system deployed!"
echo ""
echo "To test motors:"
echo "  ssh $JETSON_USER@$JETSON_HOST"
echo "  cd /home/melvin/MELVIN"
echo "  ./bin/test_motors"
echo ""
echo "Motors configured:"
echo "  • Motor 13 (0x0D)"
echo "  • Motor 14 (0x0E)"
echo "  • CAN bus @ 500 kbps"
echo ""

rm /tmp/melvin_motors.tar.gz

