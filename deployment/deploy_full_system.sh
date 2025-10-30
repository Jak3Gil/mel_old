#!/bin/bash
#
# Deploy MELVIN Full Cognitive System to Jetson
# Vision + Audio + Motors + Unified Reasoning
#

set -e

JETSON_USER="melvin"
JETSON_HOST="192.168.55.1"
JETSON_PASS="123456"

echo "╔══════════════════════════════════════════════════════╗"
echo "║     MELVIN FULL SYSTEM - JETSON DEPLOYMENT           ║"
echo "║     Vision + Motors + Reasoning                      ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

echo "📦 Creating deployment package..."
# Include only files that exist
if [ -f data/unified_nodes.dat ]; then
    tar czf /tmp/melvin_full.tar.gz \
        core/ \
        melvin_simple_full.cpp \
        data/unified_nodes.dat \
        data/unified_edges.dat \
        data/token_map.bin
else
    echo "   (No existing graph data - will create new)"
    tar czf /tmp/melvin_full.tar.gz \
        core/ \
        melvin_simple_full.cpp
fi

echo "📤 Transferring to Jetson (this may take a moment)..."
sshpass -p "$JETSON_PASS" scp -C /tmp/melvin_full.tar.gz $JETSON_USER@$JETSON_HOST:/home/melvin/MELVIN/

echo "📥 Extracting on Jetson..."
sshpass -p "$JETSON_PASS" ssh $JETSON_USER@$JETSON_HOST "cd /home/melvin/MELVIN && tar xzf melvin_full.tar.gz && rm melvin_full.tar.gz"

echo ""
echo "🔧 Building on Jetson..."
sshpass -p "$JETSON_PASS" ssh $JETSON_USER@$JETSON_HOST << 'REMOTE'
cd /home/melvin/MELVIN

# Ensure directories exist
mkdir -p bin logs /mnt/melvin_ssd/melvin_data

# Copy existing graph data to SSD if it exists
if [ -f data/unified_nodes.dat ]; then
    echo "📊 Copying graph data to SSD..."
    cp data/unified_nodes.dat /mnt/melvin_ssd/melvin_data/
    cp data/unified_edges.dat /mnt/melvin_ssd/melvin_data/
    cp data/token_map.bin /mnt/melvin_ssd/melvin_data/
fi

# Configure CAN bus for Robstride motors (921600 bps)
echo ""
echo "🔧 Configuring CAN bus for Robstride motors..."
echo "123456" | sudo -S modprobe can 2>/dev/null || true
echo "123456" | sudo -S modprobe can_raw 2>/dev/null || true
echo "123456" | sudo -S modprobe mttcan 2>/dev/null || true

echo "123456" | sudo -S ip link set can0 down 2>/dev/null || true
echo "123456" | sudo -S ip link set can0 type can bitrate 921600
echo "123456" | sudo -S ip link set can0 up

echo "123456" | sudo -S ip link set can1 down 2>/dev/null || true
echo "123456" | sudo -S ip link set can1 type can bitrate 921600
echo "123456" | sudo -S ip link set can1 up

if ip link show can0 > /dev/null 2>&1; then
    echo "✅ CAN bus configured"
else
    echo "⚠️  CAN bus not available"
fi

# Build full system
echo ""
echo "🔨 Compiling full cognitive system..."
echo "   This includes:"
echo "   • Unified Intelligence"
echo "   • Cognitive OS"
echo "   • Vision Service (OpenCV)"
echo "   • Motor Service (CAN)"
echo "   • Activation Field"
echo "   • Graph Storage"
echo ""

g++ -std=c++17 -O2 -pthread -I. \
    $(pkg-config --cflags opencv4) \
    melvin_simple_full.cpp \
    core/graph_api.cpp \
    core/motor/can_motor_controller.cpp \
    $(pkg-config --libs opencv4) \
    -o bin/melvin_full

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ BUILD SUCCESSFUL!"
    echo ""
    ls -lh bin/melvin_full
else
    echo "❌ Build failed"
    exit 1
fi

# Create startup script
cat > start_melvin.sh << 'STARTUP'
#!/bin/bash
cd /home/melvin/MELVIN
./bin/melvin_full 2>&1 | tee logs/melvin_$(date +%Y%m%d_%H%M%S).log
STARTUP

chmod +x start_melvin.sh

echo ""
echo "✅ Startup script created: start_melvin.sh"

REMOTE

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║     DEPLOYMENT COMPLETE                              ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""
echo "✅ Full cognitive system deployed!"
echo ""
echo "🧠 System includes:"
echo "   • Vision processing (camera)"
echo "   • Motor control (CAN bus, motors 13 & 14)"
echo "   • Unified reasoning"
echo "   • Knowledge graph learning"
echo "   • Cognitive OS scheduler"
echo ""
echo "🚀 To start MELVIN:"
echo "   ssh $JETSON_USER@$JETSON_HOST"
echo "   cd /home/melvin/MELVIN"
echo "   ./start_melvin.sh"
echo ""
echo "Or run directly:"
echo "   ./bin/melvin_full"
echo ""
echo "📊 All nodes and edges will be saved to:"
echo "   /mnt/melvin_ssd/melvin_data/"
echo ""

rm /tmp/melvin_full.tar.gz

