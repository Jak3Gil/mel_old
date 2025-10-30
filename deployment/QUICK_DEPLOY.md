# 🚀 Quick Deploy to Jetson via USB

## Prerequisites

1. **Jetson connected to Mac via USB**
2. **sshpass installed on Mac:**
   ```bash
   brew install hudochenkov/sshpass/sshpass
   ```

## One-Command Deployment

```bash
cd deployment
./jetson_deploy.sh
```

The script will:
1. ✅ Check for sshpass (install if needed)
2. ✅ Connect to Jetson at `192.168.55.1` (default USB network IP)
3. ✅ Transfer all files automatically
4. ✅ Install dependencies (with password automation)
5. ✅ Build Melvin
6. ✅ Create and start systemd service
7. ✅ Enable service to start on boot

## What It Does

- **Username:** `melvin`
- **Password:** `123456` (hardcoded in script)
- **IP:** `192.168.55.1` (Jetson USB network default)

If your Jetson has a different IP, edit `jetson_deploy.sh`:
```bash
JETSON_HOST="your-jetson-ip"
```

## After Deployment

Melvin will be running as a systemd service. To check:

```bash
# SSH to Jetson
sshpass -p '123456' ssh melvin@192.168.55.1

# Check status
sudo systemctl status melvin

# View logs
journalctl -u melvin -f

# Stop/restart
sudo systemctl stop melvin
sudo systemctl start melvin
```

## Troubleshooting

**Connection fails:**
```bash
# Check if Jetson is reachable
ping 192.168.55.1

# Try different IP (if Jetson is on WiFi/network)
# Edit JETSON_HOST in jetson_deploy.sh
```

**sudo password prompts:**
- The script configures passwordless sudo for the `melvin` user
- If it still prompts, the configuration may have failed
- You can manually configure: `echo "melvin ALL=(ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/melvin`

**Build fails:**
- Check if all dependencies installed: `dpkg -l | grep -E "opencv|alsa|can"`
- Re-run deployment script - it's idempotent

---

**That's it! Melvin will be always-on after deployment.** 🎉

