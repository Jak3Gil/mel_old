# 🚀 Deploy Melvin to Jetson via USB - Fully Automated

## ✅ **Ready to Deploy**

The deployment script is now **fully automated** with password handling.

## Quick Start

```bash
cd /Users/jakegilbert/Desktop/MELVIN/Melvin/deployment
./jetson_deploy.sh
```

That's it! The script will:
- ✅ Automatically connect via SSH (password: `123456`)
- ✅ Transfer all files
- ✅ Install dependencies  
- ✅ Build Melvin
- ✅ Start the service
- ✅ Enable on boot

## Configuration

The script uses these defaults (already set):
- **Username:** `melvin`
- **Password:** `123456`
- **IP:** `192.168.55.1` (Jetson USB network default)

To change the IP, edit `jetson_deploy.sh` line 20:
```bash
JETSON_HOST="your-jetson-ip"
```

## What Gets Deployed

1. **Always-on Vision** - USB camera capture at 10 FPS
2. **Always-on Audio** - USB mic listening continuously  
3. **Always-on Thinking** - Cognitive OS at 50Hz
4. **Always-on Learning** - Feedback loops active
5. **Motor Control** - CAN bus ready for Robstride motors
6. **Systemd Service** - Starts automatically on boot

## After Deployment

Melvin will automatically start. To interact:

```bash
# SSH to Jetson (password: 123456)
sshpass -p '123456' ssh melvin@192.168.55.1

# Check if Melvin is running
sudo systemctl status melvin

# View live logs
journalctl -u melvin -f

# Restart if needed
sudo systemctl restart melvin
```

## Status Check

Once deployed, Melvin will:
- 👁️ Continuously capture from USB cameras
- 🎤 Continuously listen from USB microphone
- 🧠 Process everything through cognitive OS
- 💬 Respond to speech via USB speakers
- 🔄 Learn from experience (Hebbian + feedback)
- 🤖 Control motors via CAN bus (when wired)

---

**Run the deploy script and Melvin will be live in minutes!** 🚀

