# 🚀 **MELVIN COGNITIVE OS - JETSON ORIN AGX DEPLOYMENT**

## 📋 **OVERVIEW**

This guide covers deploying MELVIN to a Jetson Orin AGX with full hardware integration.

---

## 🔧 **HARDWARE REQUIREMENTS**

### Jetson Orin AGX
- **RAM**: 32GB (recommended)
- **Storage**: 64GB+ (SSD recommended)
- **JetPack**: 5.0+ installed

### Peripherals
- **2x USB Cameras** (UVC compatible)
- **1x USB Microphone** (ALSA compatible)
- **1x USB Speakers** or 3.5mm audio out
- **Robstride O2/O3 Motors** (via CAN bus transceivers)

### Network
- Ethernet or WiFi connection
- SSH access enabled

---

## 📦 **PRE-DEPLOYMENT CHECKLIST**

On your **local machine**:
- ✅ MELVIN source code compiled and tested
- ✅ Knowledge graph data (data/unified_nodes.bin, data/unified_edges.bin)
- ✅ SSH access to Jetson configured

On the **Jetson**:
- ✅ JetPack 5.0+ installed
- ✅ SSH enabled
- ✅ User account created (default: `melvin`)
- ✅ Cameras, mic, speakers connected
- ✅ CAN transceivers connected (for motors)

---

## 🚀 **QUICK DEPLOYMENT**

### 1. Configure Deployment Script

Edit `deployment/jetson_deploy.sh`:

```bash
JETSON_USER="melvin"              # Your Jetson username
JETSON_HOST="192.168.1.100"       # Your Jetson IP or hostname
JETSON_PATH="/home/melvin/MELVIN" # Installation path
```

### 2. Run Deployment

From your local machine:

```bash
cd /Users/jakegilbert/Desktop/MELVIN/Melvin
chmod +x deployment/jetson_deploy.sh
./deployment/jetson_deploy.sh
```

This will:
1. ✅ Check Jetson connection
2. ✅ Create deployment package
3. ✅ Transfer files to Jetson
4. ✅ Install dependencies
5. ✅ Configure hardware
6. ✅ Build MELVIN
7. ✅ Create systemd service

### 3. Start MELVIN

SSH to Jetson:
```bash
ssh melvin@192.168.1.100
```

Test manually:
```bash
cd /home/melvin/MELVIN
./bin/melvin_jetson
```

Or start as service:
```bash
sudo systemctl start melvin
```

Enable on boot:
```bash
sudo systemctl enable melvin
```

---

## 🔧 **MANUAL DEPLOYMENT**

If automatic deployment fails, follow these steps:

### 1. Transfer Files

```bash
# On local machine
rsync -avz --exclude='build/' --exclude='.git/' \
    /Users/jakegilbert/Desktop/MELVIN/Melvin/ \
    melvin@192.168.1.100:/home/melvin/MELVIN/
```

### 2. Install Dependencies

```bash
# On Jetson
ssh melvin@192.168.1.100

sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    libopencv-dev v4l-utils libv4l-dev \
    libasound2-dev alsa-utils \
    can-utils libsocketcan-dev
```

### 3. Configure CAN Bus

```bash
# Setup CAN0 @ 1Mbps (for Robstride motors)
sudo ip link set can0 type can bitrate 1000000
sudo ip link set up can0

# Verify
ip link show can0
```

To make persistent, add to `/etc/network/interfaces`:
```
auto can0
iface can0 inet manual
    pre-up ip link set can0 type can bitrate 1000000
    up ip link set up can0
```

### 4. Build MELVIN

```bash
cd /home/melvin/MELVIN
mkdir -p build bin logs data

make clean
make -j8 melvin_jetson
```

### 5. Test Hardware

```bash
# Test cameras
v4l2-ctl --list-devices

# Test microphone
arecord -l
arecord -d 5 -f cd test.wav
aplay test.wav

# Test CAN bus
candump can0
```

### 6. Run MELVIN

```bash
./bin/melvin_jetson
```

---

## 🎛️ **HARDWARE CONFIGURATION**

### Cameras

MELVIN expects cameras at `/dev/video0` and `/dev/video1`.

To verify:
```bash
ls -l /dev/video*
v4l2-ctl --list-devices
```

If cameras are on different devices, update the configuration in `melvin_jetson.cpp`.

### Audio

Default ALSA device is used. To specify:
```bash
aplay -L  # List playback devices
arecord -L  # List capture devices
```

### Motors (CAN Bus)

**Recommended: Use CAN bus instead of GPIO**

CAN bus setup:
```bash
sudo ip link set can0 type can bitrate 1000000
sudo ip link set up can0
```

Verify with:
```bash
candump can0
```

If using GPIO (NOT recommended due to finicky nature):
- Requires custom driver/library for Robstride protocol
- Higher latency and potential reliability issues

---

## 📊 **MONITORING**

### System Logs

```bash
# View MELVIN logs
journalctl -u melvin -f

# View last 100 lines
journalctl -u melvin -n 100
```

### Metrics

MELVIN logs KPIs to `logs/kpis.jsonl`:
```bash
tail -f /home/melvin/MELVIN/logs/kpis.jsonl
```

### System Resources

```bash
# CPU/Memory/GPU
sudo tegrastats

# Process info
top -p $(pgrep melvin_jetson)
```

### Hardware Status

```bash
# Cameras
v4l2-ctl --device=/dev/video0 --all

# CAN bus
ip -details link show can0
candump can0

# Audio
arecord -l
aplay -l
```

---

## 🧪 **VALIDATION**

### Run Validator

After deployment, run the validator:

```bash
cd /home/melvin/MELVIN
./bin/test_validator --duration 600 --report deployment_report.md
```

This will test:
- ✅ Timing & scheduler
- ✅ Field health
- ✅ Reasoning quality
- ✅ Stress resilience
- ✅ Safety response
- ✅ Memory hygiene

### Expected Results

For a successful deployment:
```
✅ Timing ......... PASS (jitter < 2ms)
✅ Scheduler ...... PASS
✅ Field Health ... PASS
✅ Reasoning ...... PASS (accuracy > 90%)
✅ Stress Tests ... PASS
✅ Safety ......... PASS
✅ Memory ......... PASS

OVERALL: ✅ READY FOR DEPLOYMENT
```

---

## 🔧 **TUNING**

### Performance Optimization

For maximum performance on Jetson:

```bash
# Set to max performance mode
sudo nvpmodel -m 0
sudo jetson_clocks

# Verify
sudo tegrastats
```

### Genome Tuning

Edit genome parameters in `config/melvin_config.json`:

```json
{
  "reasoning": {
    "temperature": 0.7,
    "semantic_threshold": 0.5,
    "exploration_rate": 0.3
  },
  "field": {
    "beta": 0.5,
    "theta": 0.1,
    "decay_rate": 0.95
  }
}
```

### Service Frequencies

Adjust in `cognitive_os/cognitive_os.cpp`:

```cpp
// Current frequencies
Cognition:      30 Hz (every 2 ticks)
Attention:      60 Hz (every tick)
Working Memory: 30 Hz (every 2 ticks)
Learning:       10 Hz (every 5 ticks)
Reflection:      5 Hz (every 10 ticks)
```

---

## 🛡️ **SAFETY**

### Watchdog

MELVIN includes automatic restart on failure:

```bash
# Check service status
sudo systemctl status melvin

# Restart if needed
sudo systemctl restart melvin
```

### Backpressure

If CPU overloaded, MELVIN automatically:
- Reduces non-critical service frequencies
- Applies k-WTA sparsity to activation field
- Pauses learning temporarily

### Emergency Stop

To stop MELVIN:
```bash
sudo systemctl stop melvin
```

Or kill process:
```bash
pkill -SIGTERM melvin_jetson
```

---

## 🐛 **TROUBLESHOOTING**

### Build Fails

```bash
# Check dependencies
dpkg -l | grep opencv
pkg-config --modversion opencv4

# Clean build
make clean
rm -rf build/*
make -j8 melvin_jetson
```

### Cameras Not Found

```bash
# List all video devices
ls -l /dev/video*

# Check permissions
sudo usermod -aG video melvin

# Test camera
v4l2-ctl --device=/dev/video0 --stream-mmap
```

### CAN Bus Issues

```bash
# Check interface
ip link show can0

# Verify bitrate
ip -details link show can0

# Restart CAN
sudo ip link set down can0
sudo ip link set can0 type can bitrate 1000000
sudo ip link set up can0

# Test
cansend can0 123#DEADBEEF
```

### High CPU Usage

```bash
# Check current mode
sudo nvpmodel -q

# Set to lower power mode
sudo nvpmodel -m 2

# Reduce service frequencies in code
```

### Memory Leaks

```bash
# Monitor memory
watch -n 1 free -h

# Check MELVIN process
top -p $(pgrep melvin_jetson)

# Run validator to check growth
./bin/test_validator --duration 1800
```

---

## 📁 **FILE STRUCTURE**

```
/home/melvin/MELVIN/
├── bin/
│   ├── melvin_jetson          # Main executable
│   └── test_validator         # Validation tool
├── core/                      # Core intelligence
├── cognitive_os/              # Always-on OS
├── validator/                 # Validation suite
├── data/
│   ├── unified_nodes.bin      # Knowledge graph nodes
│   └── unified_edges.bin      # Knowledge graph edges
├── logs/
│   └── kpis.jsonl            # System metrics
└── config/
    └── melvin_config.json     # Configuration
```

---

## 🎯 **EXPECTED PERFORMANCE**

### Jetson Orin AGX (32GB)

| Metric | Expected |
|--------|----------|
| Scheduler Frequency | 50 Hz |
| Cognition Latency | < 30 ms |
| Field Update | < 20 ms |
| Memory Usage | 2-4 GB |
| CPU Usage | 40-60% |
| Power Draw | 15-30W |

### After 1 Week

- **Nodes**: 50,000 - 100,000
- **Edges**: 500,000 - 1,000,000
- **Response Time**: < 100ms for queries
- **Accuracy**: > 85% on familiar tasks

---

## 📞 **SUPPORT**

If issues persist:

1. Check logs: `journalctl -u melvin -n 1000`
2. Run validator: `./bin/test_validator --duration 600`
3. Monitor resources: `sudo tegrastats`
4. Review hardware: `v4l2-ctl -L`, `candump can0`

---

## ✅ **DEPLOYMENT CHECKLIST**

- [ ] Jetson powered on and accessible via SSH
- [ ] All peripherals connected (cameras, mic, speakers, motors)
- [ ] Dependencies installed
- [ ] CAN bus configured (if using motors)
- [ ] MELVIN built successfully
- [ ] Hardware test passed
- [ ] Validator passed
- [ ] Systemd service created
- [ ] MELVIN running continuously
- [ ] Metrics being logged

**Once all checked: MELVIN is live! 🎉**

