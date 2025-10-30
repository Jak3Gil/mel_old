#!/bin/bash
#
# MELVIN Cognitive OS - Jetson Orin AGX Deployment Script (USB Connection)
#
# For Jetson connected via USB in USB Device Mode
#

set -e  # Exit on error

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CONFIGURATION
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# USB Device Mode - Jetson appears as 192.168.55.1 (default)
JETSON_USER="nvidia"  # Default Jetson user (or your username)
JETSON_HOST="192.168.55.1"  # Default USB device mode IP
JETSON_PATH="/home/nvidia/MELVIN"

LOCAL_PATH="$(cd "$(dirname "$0")/.." && pwd)"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN COGNITIVE OS - JETSON DEPLOYMENT (USB)    ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "📍 Local:  $LOCAL_PATH"
echo "📍 Jetson: $JETSON_USER@$JETSON_HOST:$JETSON_PATH"
echo ""
echo "ℹ️  Jetson connected via USB Device Mode"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CHECK USB CONNECTION
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔍 Checking USB connection to Jetson..."

# Check if USB network interface exists
if ! ifconfig | grep -q "192.168.55"; then
    echo "⚠️  USB network interface not found"
    echo ""
    echo "Please ensure:"
    echo "  1. Jetson is connected via USB-C cable"
    echo "  2. Jetson is powered on"
    echo "  3. Jetson is in USB Device Mode"
    echo ""
    echo "To enable USB Device Mode on Jetson:"
    echo "  sudo systemctl enable nv-l4t-usb-device-mode.service"
    echo "  sudo systemctl start nv-l4t-usb-device-mode.service"
    echo ""
    echo "Your Mac should see a new network interface (usually 192.168.55.100)"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Try SSH connection
echo "Testing SSH connection..."
if ! ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no $JETSON_USER@$JETSON_HOST "echo 'Connected'" 2>/dev/null; then
    echo "❌ Cannot connect via SSH"
    echo ""
    echo "Setup SSH access:"
    echo ""
    echo "1. Connect to Jetson via serial console or direct connection"
    echo "2. Enable SSH:"
    echo "   sudo systemctl enable ssh"
    echo "   sudo systemctl start ssh"
    echo ""
    echo "3. Setup SSH keys from your Mac:"
    echo "   ssh-copy-id $JETSON_USER@$JETSON_HOST"
    echo ""
    echo "4. Or set password for user:"
    echo "   sudo passwd $JETSON_USER"
    echo ""
    exit 1
fi

echo "✅ USB connection active"
echo "✅ SSH working"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CREATE DEPLOYMENT PACKAGE
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "📦 Creating deployment package..."

# Create temp directory
TEMP_DIR=$(mktemp -d)
mkdir -p $TEMP_DIR/MELVIN

# Copy essential files
echo "   Copying source files..."
rsync -a --exclude='build/' --exclude='bin/' --exclude='logs/' \
    --exclude='.git/' --exclude='*.o' --exclude='archive/' \
    --exclude='legacy/' \
    $LOCAL_PATH/core \
    $LOCAL_PATH/cognitive_os \
    $LOCAL_PATH/validator \
    $LOCAL_PATH/Makefile \
    $LOCAL_PATH/*.cpp \
    $LOCAL_PATH/*.md \
    $TEMP_DIR/MELVIN/

# Copy config
if [ -d "$LOCAL_PATH/config" ]; then
    echo "   Copying config..."
    rsync -a $LOCAL_PATH/config $TEMP_DIR/MELVIN/
fi

# Copy data files if they exist
if [ -d "$LOCAL_PATH/data" ]; then
    echo "   Copying knowledge graph data..."
    # Check size first
    DATA_SIZE=$(du -sm "$LOCAL_PATH/data" | cut -f1)
    if [ $DATA_SIZE -gt 500 ]; then
        echo "   ⚠️  Data directory is ${DATA_SIZE}MB - this may take a while over USB"
        read -p "   Continue? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rsync -a $LOCAL_PATH/data $TEMP_DIR/MELVIN/
        else
            echo "   Skipping data directory (will need to transfer separately)"
        fi
    else
        rsync -a $LOCAL_PATH/data $TEMP_DIR/MELVIN/
    fi
fi

# Create setup script
cat > $TEMP_DIR/MELVIN/deploy_setup.sh << 'DEPLOY_SCRIPT'
#!/bin/bash
#
# Jetson setup script (runs on Jetson)
#

set -e

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - JETSON SETUP                            ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# INSTALL DEPENDENCIES
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "📦 Installing dependencies..."

# Update package lists
sudo apt-get update

# Install build tools
echo "   Installing build tools..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# Install OpenCV (if not already installed via JetPack)
if ! pkg-config --exists opencv4; then
    echo "   Installing OpenCV..."
    sudo apt-get install -y \
        libopencv-dev \
        python3-opencv
else
    echo "   ✅ OpenCV already installed"
fi

# Install V4L2 for camera access
echo "   Installing camera support..."
sudo apt-get install -y \
    v4l-utils \
    libv4l-dev

# Install ALSA for audio
echo "   Installing audio support..."
sudo apt-get install -y \
    libasound2-dev \
    alsa-utils

# Install CAN bus tools (for Robstride motors)
echo "   Installing CAN bus support..."
sudo apt-get install -y \
    can-utils \
    libsocketcan-dev

echo "✅ Dependencies installed"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CONFIGURE HARDWARE
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔧 Configuring hardware..."

# Check cameras
echo "   Checking USB cameras..."
NUM_CAMERAS=$(ls /dev/video* 2>/dev/null | wc -l)
echo "   Found $NUM_CAMERAS camera devices"

if [ $NUM_CAMERAS -lt 2 ]; then
    echo "   ⚠️  Warning: Expected 2 cameras, found $NUM_CAMERAS"
    echo "   Camera devices:"
    ls -l /dev/video* 2>/dev/null || echo "   No cameras found"
fi

# Check audio devices
echo "   Checking audio devices..."
if arecord -l > /dev/null 2>&1; then
    echo "   ✅ Microphone detected"
    arecord -l | grep -i "card"
else
    echo "   ⚠️  No microphone detected"
fi

if aplay -l > /dev/null 2>&1; then
    echo "   ✅ Speakers detected"
    aplay -l | grep -i "card"
else
    echo "   ⚠️  No speakers detected"
fi

# Setup CAN bus for motors (if interface exists)
echo "   Checking CAN bus..."
if ip link show can0 > /dev/null 2>&1; then
    echo "   Setting up CAN bus..."
    sudo ip link set can0 down 2>/dev/null || true
    sudo ip link set can0 type can bitrate 1000000
    sudo ip link set up can0
    echo "   ✅ CAN bus configured (can0 @ 1Mbps)"
elif ls /sys/class/net/ | grep -q "can"; then
    # CAN interface exists but different name
    CAN_IF=$(ls /sys/class/net/ | grep can | head -1)
    echo "   Found CAN interface: $CAN_IF"
    sudo ip link set $CAN_IF down 2>/dev/null || true
    sudo ip link set $CAN_IF type can bitrate 1000000
    sudo ip link set up $CAN_IF
    echo "   ✅ CAN bus configured ($CAN_IF @ 1Mbps)"
else
    echo "   ⚠️  No CAN interface found"
    echo "      Motors will not be available"
    echo "      You may need to enable CAN in device tree"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# BUILD MELVIN
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔨 Building MELVIN..."

# Create directories
mkdir -p build bin logs data

# Set performance mode for faster build
echo "   Setting max performance..."
sudo nvpmodel -m 0 2>/dev/null || true
sudo jetson_clocks 2>/dev/null || true

# Build using all cores
CORES=$(nproc)
echo "   Building with $CORES cores..."

make clean
if make -j$CORES melvin_jetson test_cognitive_os test_validator 2>&1 | tee build.log; then
    echo "✅ Build successful"
else
    echo "❌ Build failed - see build.log"
    tail -50 build.log
    exit 1
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CREATE SYSTEMD SERVICE
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🚀 Creating systemd service..."

USERNAME=$(whoami)
WORKING_DIR=$(pwd)

cat > /tmp/melvin.service << SERVICE
[Unit]
Description=MELVIN Cognitive OS
After=network.target

[Service]
Type=simple
User=$USERNAME
WorkingDirectory=$WORKING_DIR
ExecStart=$WORKING_DIR/bin/melvin_jetson
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Resource limits
MemoryMax=8G
CPUQuota=800%

# Environment
Environment="LD_LIBRARY_PATH=/usr/local/lib:/usr/lib"

[Install]
WantedBy=multi-user.target
SERVICE

sudo mv /tmp/melvin.service /etc/systemd/system/
sudo systemctl daemon-reload

echo "✅ Systemd service created"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# SUMMARY
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "╔══════════════════════════════════════════════════════╗"
echo "║     SETUP COMPLETE                                   ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ MELVIN successfully installed on Jetson"
echo ""
echo "To start MELVIN:"
echo "  sudo systemctl start melvin"
echo ""
echo "To enable on boot:"
echo "  sudo systemctl enable melvin"
echo ""
echo "To view logs:"
echo "  journalctl -u melvin -f"
echo ""
echo "To test manually:"
echo "  ./bin/melvin_jetson"
echo ""
echo "To run validator:"
echo "  ./bin/test_validator --duration 600 --report report.md"
echo ""

DEPLOY_SCRIPT

chmod +x $TEMP_DIR/MELVIN/deploy_setup.sh

echo "✅ Deployment package created"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# TRANSFER TO JETSON (OVER USB)
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "📤 Transferring to Jetson over USB..."
echo "   This may take a few minutes depending on package size..."
echo ""

# Create directory on Jetson
ssh -o StrictHostKeyChecking=no $JETSON_USER@$JETSON_HOST "mkdir -p $JETSON_PATH"

# Transfer files with progress
# Use compression for faster transfer over USB
rsync -avz --progress --compress-level=6 \
    $TEMP_DIR/MELVIN/ \
    $JETSON_USER@$JETSON_HOST:$JETSON_PATH/

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Files transferred successfully"
else
    echo ""
    echo "❌ Transfer failed"
    exit 1
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# RUN SETUP ON JETSON
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔧 Running setup on Jetson..."
echo ""

ssh -o StrictHostKeyChecking=no $JETSON_USER@$JETSON_HOST "cd $JETSON_PATH && bash deploy_setup.sh"

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     DEPLOYMENT COMPLETE                              ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ MELVIN deployed to Jetson Orin AGX via USB"
echo ""
echo "Next steps:"
echo ""
echo "  1. Connect to Jetson:"
echo "     ssh $JETSON_USER@$JETSON_HOST"
echo ""
echo "  2. Test MELVIN:"
echo "     cd $JETSON_PATH"
echo "     ./bin/melvin_jetson"
echo ""
echo "  3. Start as service:"
echo "     sudo systemctl start melvin"
echo ""
echo "  4. View logs:"
echo "     journalctl -u melvin -f"
echo ""
echo "  5. Run validator:"
echo "     ./bin/test_validator --duration 600"
echo ""
echo "📊 The Jetson is accessible at: $JETSON_HOST"
echo ""

# Cleanup
rm -rf $TEMP_DIR

exit 0

