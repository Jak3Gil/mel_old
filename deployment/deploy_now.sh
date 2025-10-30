#!/bin/bash
#
# MELVIN Deployment with credentials
#

set -e

JETSON_USER="melvin"
JETSON_PASS="123456"
JETSON_HOST="192.168.55.1"
JETSON_PATH="/home/melvin/MELVIN"
LOCAL_PATH="$(cd "$(dirname "$0")/.." && pwd)"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - DEPLOYING TO JETSON                     ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "🔍 Testing connection..."

# Test SSH with credentials
if sshpass -p "$JETSON_PASS" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$JETSON_USER@$JETSON_HOST" "echo 'Connected'; hostname" 2>/dev/null; then
    echo "✅ SSH connection successful"
else
    echo "❌ SSH connection failed"
    exit 1
fi

echo ""
echo "📦 Creating deployment package..."

TEMP_DIR=$(mktemp -d)
mkdir -p $TEMP_DIR/MELVIN

# Copy essential files
echo "   Copying core files..."
rsync -a --exclude='build/' --exclude='bin/' --exclude='logs/' \
    --exclude='.git/' --exclude='*.o' --exclude='archive/' --exclude='legacy/' \
    $LOCAL_PATH/core \
    $LOCAL_PATH/cognitive_os \
    $LOCAL_PATH/validator \
    $LOCAL_PATH/Makefile \
    $TEMP_DIR/MELVIN/

# Copy main programs
echo "   Copying programs..."
cp $LOCAL_PATH/melvin_jetson.cpp $TEMP_DIR/MELVIN/ 2>/dev/null || true
cp $LOCAL_PATH/test_*.cpp $TEMP_DIR/MELVIN/ 2>/dev/null || true

# Copy config
if [ -d "$LOCAL_PATH/config" ]; then
    echo "   Copying config..."
    rsync -a $LOCAL_PATH/config $TEMP_DIR/MELVIN/
fi

# Copy small data files only
if [ -d "$LOCAL_PATH/data" ]; then
    DATA_SIZE=$(du -sm "$LOCAL_PATH/data" 2>/dev/null | cut -f1 || echo "0")
    if [ $DATA_SIZE -lt 100 ]; then
        echo "   Copying data files (${DATA_SIZE}MB)..."
        rsync -a $LOCAL_PATH/data $TEMP_DIR/MELVIN/
    else
        echo "   ⚠️  Skipping large data directory (${DATA_SIZE}MB)"
        mkdir -p $TEMP_DIR/MELVIN/data
    fi
else
    mkdir -p $TEMP_DIR/MELVIN/data
fi

# Create setup script
cat > $TEMP_DIR/MELVIN/jetson_setup.sh << 'SETUP'
#!/bin/bash
set -e

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - JETSON SETUP                            ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

echo "📦 Installing dependencies..."
sudo apt-get update -qq || true
sudo apt-get install -y build-essential cmake 2>/dev/null || true

# Check for OpenCV
if ! pkg-config --exists opencv4 2>/dev/null; then
    echo "   Installing OpenCV (this may take a while)..."
    sudo apt-get install -y libopencv-dev 2>/dev/null || echo "   ⚠️  OpenCV install incomplete"
else
    echo "   ✅ OpenCV found"
fi

echo "   Installing audio/video/CAN support..."
sudo apt-get install -y v4l-utils libv4l-dev libasound2-dev alsa-utils can-utils 2>/dev/null || true

echo "✅ Dependencies installed"
echo ""

echo "🔨 Building MELVIN..."
mkdir -p build bin logs data

# Set max performance
echo "   Setting Jetson to max performance..."
sudo nvpmodel -m 0 2>/dev/null || echo "   ⚠️  Could not set nvpmodel"
sudo jetson_clocks 2>/dev/null || echo "   ⚠️  Could not set jetson_clocks"

# Build
echo "   Compiling... (this will take 5-10 minutes)"
make clean 2>/dev/null || true

if make -j$(nproc) melvin_jetson test_cognitive_os test_validator 2>&1 | tee build.log | tail -20; then
    echo ""
    echo "✅ Build successful!"
    ls -lh bin/
else
    echo ""
    echo "❌ Build failed - check build.log for details"
    tail -50 build.log
    exit 1
fi

echo ""
echo "🔧 Configuring hardware..."

# Check cameras
NUM_CAMERAS=$(ls /dev/video* 2>/dev/null | wc -l)
echo "   Cameras found: $NUM_CAMERAS"

# Setup CAN bus if available
if ip link show can0 >/dev/null 2>&1; then
    echo "   Configuring CAN bus..."
    sudo ip link set can0 down 2>/dev/null || true
    sudo ip link set can0 type can bitrate 1000000
    sudo ip link set up can0
    echo "   ✅ CAN bus ready"
else
    echo "   ⚠️  No CAN interface (motors won't work)"
fi

echo ""
echo "🚀 Creating systemd service..."

USERNAME=$(whoami)
WORKING_DIR=$(pwd)

sudo bash -c "cat > /etc/systemd/system/melvin.service << SERVICE
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

[Install]
WantedBy=multi-user.target
SERVICE"

sudo systemctl daemon-reload
sudo systemctl enable melvin

echo "✅ Service created and enabled"
echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     SETUP COMPLETE                                   ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ MELVIN is ready to run!"
echo ""
echo "To start MELVIN:"
echo "  sudo systemctl start melvin"
echo ""
echo "To view logs:"
echo "  journalctl -u melvin -f"
echo ""
echo "To test manually:"
echo "  ./bin/melvin_jetson"
echo ""
SETUP

chmod +x $TEMP_DIR/MELVIN/jetson_setup.sh

echo "✅ Package ready"
echo ""

echo "📤 Transferring to Jetson..."
echo "   This may take 2-5 minutes..."
echo ""

# Create directory
sshpass -p "$JETSON_PASS" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$JETSON_USER@$JETSON_HOST" "mkdir -p $JETSON_PATH" 2>/dev/null

# Transfer with progress
sshpass -p "$JETSON_PASS" rsync -avz --progress -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
    $TEMP_DIR/MELVIN/ $JETSON_USER@$JETSON_HOST:$JETSON_PATH/

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Files transferred successfully"
else
    echo ""
    echo "❌ Transfer failed"
    rm -rf $TEMP_DIR
    exit 1
fi

echo ""
echo "🔧 Running setup on Jetson..."
echo "   (This will take 10-15 minutes for compilation)"
echo ""

sshpass -p "$JETSON_PASS" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$JETSON_USER@$JETSON_HOST" "cd $JETSON_PATH && bash jetson_setup.sh"

if [ $? -eq 0 ]; then
    echo ""
    echo "╔══════════════════════════════════════════════════════╗"
    echo "║                                                      ║"
    echo "║     ✅ DEPLOYMENT COMPLETE!                          ║"
    echo "║                                                      ║"
    echo "╚══════════════════════════════════════════════════════╝"
    echo ""
    echo "✅ MELVIN successfully deployed to Jetson Orin AGX!"
    echo ""
    echo "To start MELVIN:"
    echo "  ssh melvin@192.168.55.1"
    echo "  sudo systemctl start melvin"
    echo ""
    echo "To view logs:"
    echo "  ssh melvin@192.168.55.1"
    echo "  journalctl -u melvin -f"
    echo ""
    echo "Or start now:"
    echo "  sshpass -p '123456' ssh melvin@192.168.55.1 'sudo systemctl start melvin'"
    echo ""
else
    echo ""
    echo "❌ Setup failed on Jetson"
    exit 1
fi

# Cleanup
rm -rf $TEMP_DIR

exit 0
