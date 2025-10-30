#!/bin/bash
#
# MELVIN Cognitive OS - Jetson Orin AGX Deployment Script
#
# This script deploys MELVIN to a Jetson Orin AGX with:
# - 2 USB cameras
# - USB microphone/speakers
# - Robstride O2/O3 motors (via CAN bus recommended)
#

set -e  # Exit on error

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CONFIGURATION
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

JETSON_USER="melvin"
JETSON_PASS="123456"
# For USB connection via ssh over USB network interface
JETSON_HOST="192.168.55.1"  # Default Jetson USB network IP, or jetson-orin.local
JETSON_PATH="/home/melvin/MELVIN"

LOCAL_PATH="$(cd "$(dirname "$0")/.." && pwd)"

# Check for sshpass (needed for password automation)
if ! command -v sshpass &> /dev/null; then
    echo "📦 Installing sshpass for password automation..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if command -v brew &> /dev/null; then
            brew install hudochenkov/sshpass/sshpass
        else
            echo "❌ sshpass not found. Install with: brew install hudochenkov/sshpass/sshpass"
            exit 1
        fi
    else
        echo "❌ sshpass not found. Install with: Herbert install sshpass"
        exit 1
    fi
fi

# SSH function with password
ssh_with_pass() {
    sshpass -p "$JETSON_PASS" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$@"
}

# SCP function with password
scp_with_pass() {
    sshpass -p "$JETSON_PASS" scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$@"
}

# RSYNC function with password
rsync_with_pass() {
    SSHPASS="$JETSON_PASS" sshpass -e rsync -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" "$@"
}

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN COGNITIVE OS - JETSON DEPLOYMENT          ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "📍 Local:  $LOCAL_PATH"
echo "📍 Jetson: $JETSON_USER@$JETSON_HOST:$JETSON_PATH"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CHECK JETSON CONNECTION
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔍 Checking Jetson connection..."
if ! ssh_with_pass $JETSON_USER@$JETSON_HOST "echo 'Connected'" 2>&1; then
    echo "❌ Cannot connect to Jetson at $JETSON_HOST"
    echo "   Please check:"
    echo "   1. Jetson is powered on and connected via USB"
    echo "   2. SSH is enabled on Jetson"
    echo "   3. IP address is correct (check: ping $JETSON_HOST)"
    exit 1
fi
echo "✅ Connected to Jetson"
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
    --exclude='.git/' --exclude='*.o' \
    $LOCAL_PATH/core \
    $LOCAL_PATH/cognitive_os \
    $LOCAL_PATH/validator \
    $LOCAL_PATH/Makefile \
    $LOCAL_PATH/*.md \
    $TEMP_DIR/MELVIN/

# Copy data files if they exist
if [ -d "$LOCAL_PATH/data" ]; then
    echo "   Copying knowledge graph data..."
    rsync -a $LOCAL_PATH/data $TEMP_DIR/MELVIN/
fi

# Create deployment scripts
cat > $TEMP_DIR/MELVIN/deploy_setup.sh << DEPLOY_SCRIPT
#!/bin/bash
#
# Jetson setup script (runs on Jetson)
#

set -e

JETSON_PASS="$JETSON_PASS"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - JETSON SETUP                            ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# INSTALL DEPENDENCIES
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "📦 Installing dependencies..."

# Configure passwordless sudo for deployment (first command needs password)
echo "$JETSON_PASS" | sudo -S bash -c "echo 'melvin ALL=(ALL) NOPASSWD: ALL' > /etc/sudoers.d/melvin-deploy && chmod 0440 /etc/sudoers.d/melvin-deploy" || {
    echo "⚠️  Could not configure passwordless sudo. Commands will prompt for password."
    echo "   You may need to run: echo '$JETSON_PASS' | sudo -S <command>"
}

# Update package lists
sudo -n apt-get update

# Install build tools
sudo -n apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# Install OpenCV (if not already installed via JetPack)
if ! pkg-config --exists opencv4; then
    echo "   Installing OpenCV..."
    sudo -n apt-get install -y \
        libopencv-dev \
        python3-opencv
fi

# Install V4L2 for camera access
sudo -n apt-get install -y \
    v4l-utils \
    libv4l-dev

# Install ALSA for audio
sudo -n apt-get install -y \
    libasound2-dev \
    alsa-utils

# Install CAN bus tools (for Robstride motors)
sudo -n apt-get install -y \
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
fi

# Check audio devices
echo "   Checking audio devices..."
arecord -l > /dev/null 2>&1 && echo "   ✅ Microphone detected" || echo "   ⚠️  No microphone detected"
aplay -l > /dev/null 2>&1 && echo "   ✅ Speakers detected" || echo "   ⚠️  No speakers detected"

# Setup CAN bus for motors (if not already configured)
if ! ip link show can0 > /dev/null 2>&1; then
    echo "   Setting up CAN bus..."
    sudo -n ip link set can0 type can bitrate 1000000 || true
    sudo -n ip link set up can0 || true
    echo "   ✅ CAN bus configured (can0 @ 1Mbps)"
else
    echo "   ✅ CAN bus already configured"
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# BUILD MELVIN
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔨 Building MELVIN..."

# Create directories
mkdir -p build bin logs data

# Build
make clean
make -j$(nproc) all

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# CREATE SYSTEMD SERVICE
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🚀 Creating systemd service..."

cat > /tmp/melvin.service << 'SERVICE'
[Unit]
Description=MELVIN Cognitive OS
After=network.target

[Service]
Type=simple
User=melvin
WorkingDirectory=/home/melvin/MELVIN
ExecStart=/home/melvin/MELVIN/bin/melvin_jetson
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Resource limits
MemoryMax=4G
CPUQuota=400%

[Install]
WantedBy=multi-user.target
SERVICE

sudo -n mv /tmp/melvin.service /etc/systemd/system/
sudo -n systemctl daemon-reload

echo "✅ Systemd service created"
echo ""
echo "To start MELVIN:"
echo "  sudo systemctl start melvin"
echo ""
echo "To enable on boot:"
echo "  sudo systemctl enable melvin"
echo ""
echo "To view logs:"
echo "  journalctl -u melvin -f"

DEPLOY_SCRIPT

chmod +x $TEMP_DIR/MELVIN/deploy_setup.sh

echo "✅ Deployment package created"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# TRANSFER TO JETSON
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "📤 Transferring to Jetson..."

# Create directory on Jetson
ssh_with_pass $JETSON_USER@$JETSON_HOST "mkdir -p $JETSON_PATH"

# Transfer files
rsync_with_pass -avz --progress $TEMP_DIR/MELVIN/ $JETSON_USER@$JETSON_HOST:$JETSON_PATH/

echo "✅ Files transferred"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# RUN SETUP ON JETSON
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

echo "🔧 Running setup on Jetson..."
echo "   (This may take several minutes...)"

ssh_with_pass $JETSON_USER@$JETSON_HOST "cd $JETSON_PATH && JETSON_PASS='$JETSON_PASS' bash deploy_setup.sh"

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     DEPLOYMENT COMPLETE                              ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ MELVIN deployed to Jetson Orin AGX"
echo ""
echo "🚀 Starting MELVIN service..."
ssh_with_pass $JETSON_USER@$JETSON_HOST "sudo -n systemctl start melvin && sudo -n systemctl enable melvin" || true

echo ""
echo "Next steps:"
echo "  1. SSH to Jetson: sshpass -p '$JETSON_PASS' ssh $JETSON_USER@$JETSON_HOST"
echo "  2. Test MELVIN:   cd $JETSON_PATH && ./bin/test_cognitive_os"
echo "  3. Check status:  sudo systemctl status melvin"
echo "  4. View logs:     journalctl -u melvin -f"
echo ""
echo "📊 Run validator:"
echo "  ./bin/test_validator --duration 600 --report deployment_report.md"
echo ""

# Cleanup
rm -rf $TEMP_DIR

exit 0

