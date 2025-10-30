#!/bin/bash
#
# Automatic deployment with password handling
#

set -e

JETSON_USER="nvidia"
JETSON_HOST="192.168.55.1"
JETSON_PATH="/home/nvidia/MELVIN"
LOCAL_PATH="$(cd "$(dirname "$0")/.." && pwd)"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - AUTOMATIC USB DEPLOYMENT                ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# Check if sshpass is available
if ! command -v sshpass &> /dev/null; then
    echo "📦 Installing sshpass for automatic password handling..."
    
    # Check if Homebrew is available
    if command -v brew &> /dev/null; then
        brew install hudochenkov/sshpass/sshpass || {
            echo "⚠️  Could not install sshpass via brew"
            echo "   Falling back to manual password entry..."
        }
    else
        echo "⚠️  Homebrew not found, sshpass not available"
        echo "   You'll need to enter password manually for each step"
    fi
fi

echo ""
echo "🔍 Testing connection to Jetson..."
echo "   Host: $JETSON_HOST"
echo ""

# Function to run SSH command
run_ssh() {
    if command -v sshpass &> /dev/null; then
        sshpass -p "nvidia" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$JETSON_USER@$JETSON_HOST" "$@"
    else
        ssh -o StrictHostKeyChecking=no "$JETSON_USER@$JETSON_HOST" "$@"
    fi
}

# Test connection
echo "Testing SSH (password: nvidia)..."
if run_ssh "echo 'Connected'; hostname" 2>/dev/null; then
    echo "✅ SSH connection successful"
else
    echo "❌ SSH connection failed"
    echo ""
    echo "Please ensure:"
    echo "  1. Jetson is powered on"
    echo "  2. USB connection is active"
    echo "  3. SSH is enabled on Jetson"
    echo "  4. Password is 'nvidia' (default)"
    echo ""
    exit 1
fi

echo ""
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
    $TEMP_DIR/MELVIN/ 2>/dev/null || true

# Copy config
if [ -d "$LOCAL_PATH/config" ]; then
    rsync -a $LOCAL_PATH/config $TEMP_DIR/MELVIN/ 2>/dev/null || true
fi

# Copy small data files only
if [ -d "$LOCAL_PATH/data" ]; then
    DATA_SIZE=$(du -sm "$LOCAL_PATH/data" 2>/dev/null | cut -f1 || echo "0")
    if [ $DATA_SIZE -lt 100 ]; then
        echo "   Copying data files..."
        rsync -a $LOCAL_PATH/data $TEMP_DIR/MELVIN/ 2>/dev/null || true
    else
        echo "   Skipping large data directory (${DATA_SIZE}MB)"
    fi
fi

# Create setup script
cat > $TEMP_DIR/MELVIN/jetson_setup.sh << 'SETUP'
#!/bin/bash
set -e

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN - JETSON SETUP                            ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

echo "📦 Installing dependencies..."
sudo apt-get update -qq
sudo apt-get install -y -qq build-essential cmake || true

# Check for OpenCV
if ! pkg-config --exists opencv4 2>/dev/null; then
    echo "   Installing OpenCV..."
    sudo apt-get install -y -qq libopencv-dev 2>/dev/null || true
fi

echo "   Installing audio/video support..."
sudo apt-get install -y -qq v4l-utils libasound2-dev can-utils 2>/dev/null || true

echo "✅ Dependencies installed"
echo ""

echo "🔨 Building MELVIN..."
mkdir -p build bin logs data

# Set max performance
sudo nvpmodel -m 0 2>/dev/null || true
sudo jetson_clocks 2>/dev/null || true

# Build
make clean 2>/dev/null || true
if make -j$(nproc) melvin_jetson test_cognitive_os 2>&1 | tee build.log; then
    echo "✅ Build successful!"
else
    echo "❌ Build failed - check build.log"
    exit 1
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

[Install]
WantedBy=multi-user.target
SERVICE"

sudo systemctl daemon-reload

echo "✅ Service created"
echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     SETUP COMPLETE                                   ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "To start MELVIN:"
echo "  sudo systemctl start melvin"
echo ""
echo "To view logs:"
echo "  journalctl -u melvin -f"
SETUP

chmod +x $TEMP_DIR/MELVIN/jetson_setup.sh

echo "✅ Package created"
echo ""

echo "📤 Transferring to Jetson..."
echo "   This may take a few minutes..."
echo ""

# Create directory on Jetson
run_ssh "mkdir -p $JETSON_PATH" 2>/dev/null

# Transfer files
if command -v sshpass &> /dev/null; then
    rsync -avz --progress -e "sshpass -p nvidia ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" \
        $TEMP_DIR/MELVIN/ $JETSON_USER@$JETSON_HOST:$JETSON_PATH/
else
    echo "   (You may need to enter password: nvidia)"
    rsync -avz --progress \
        $TEMP_DIR/MELVIN/ $JETSON_USER@$JETSON_HOST:$JETSON_PATH/
fi

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Files transferred"
else
    echo ""
    echo "❌ Transfer failed"
    exit 1
fi

echo ""
echo "🔧 Running setup on Jetson..."
echo ""

run_ssh "cd $JETSON_PATH && bash jetson_setup.sh"

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     DEPLOYMENT COMPLETE                              ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ MELVIN successfully deployed to Jetson!"
echo ""
echo "To start MELVIN:"
echo "  ssh $JETSON_USER@$JETSON_HOST"
echo "  sudo systemctl start melvin"
echo ""
echo "To view logs:"
echo "  ssh $JETSON_USER@$JETSON_HOST"
echo "  journalctl -u melvin -f"
echo ""

# Cleanup
rm -rf $TEMP_DIR

