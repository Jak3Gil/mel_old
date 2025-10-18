# 📹 Melvin Camera Executables - Easy Launch Guide

I've created **3 easy executables** to run Melvin's camera vision. Pick whichever you prefer!

---

## ⭐ OPTION 1: Double-Click (Easiest!)

**File:** `MELVIN_CAMERA.command`

**How to use:**
1. Open Finder
2. Navigate to the Melvin folder
3. **Double-click** `MELVIN_CAMERA.command`
4. Camera opens automatically!

**Why this is great:**
- ✅ No terminal needed
- ✅ Auto-detects camera
- ✅ Falls back to test scenes if no camera
- ✅ macOS native (.command extension)

---

## ⭐ OPTION 2: Terminal Executable (Recommended)

**File:** `run_melvin_camera`

**How to use:**
```bash
./run_melvin_camera
```

**Features:**
- ✅ Smart camera detection
- ✅ Shows which camera found
- ✅ Helpful error messages
- ✅ Auto-fallback to test scenes

**From anywhere:**
```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
./run_melvin_camera
```

---

## ⭐ OPTION 3: Ultra-Simple Launcher

**File:** `melvin_camera_launcher.sh`

**How to use:**
```bash
./melvin_camera_launcher.sh
```

**Why this is great:**
- ✅ Shortest script (4 lines)
- ✅ Direct execution
- ✅ Minimal overhead

---

## 🎯 All Three Do the Same Thing!

They all:
1. Find your working camera (Camera 0 or 1)
2. Launch Melvin's vision system
3. Show you what Melvin sees
4. Display attention overlay (cyan heatmap)
5. Track focus (yellow crosshair)

**Pick whichever is easiest for you!**

---

## 🚀 Quick Commands Reference

```bash
# From the Melvin main folder:

# Option 1: Double-click in Finder
# (No command - just double-click MELVIN_CAMERA.command)

# Option 2: Run executable
./run_melvin_camera

# Option 3: Simple launcher
./melvin_camera_launcher.sh

# Option 4: Direct Python (from melvin/ subfolder)
cd melvin
python3 melvin_sees.py

# Option 5: Specify camera manually
cd melvin
python3 melvin_sees.py 0    # Camera 0
python3 melvin_sees.py 1    # Camera 1
```

---

## 📊 What Each Executable Does

### `MELVIN_CAMERA.command` (Double-Click)
```bash
cd melvin/
python3 melvin_sees.py
(with error handling and fallback)
```

### `run_melvin_camera` (Smart Terminal)
```bash
# 1. Detect camera (test cameras 0, 1, 2)
# 2. Launch with working camera
# 3. Show helpful messages
# 4. Fallback to test scenes if no camera
```

### `melvin_camera_launcher.sh` (Minimal)
```bash
cd melvin/
exec python3 melvin_sees.py
```

---

## 🎮 Controls (All Executables)

Once running, use these keys:

| Key | Action |
|-----|--------|
| `q` | Quit |
| `p` | Pause/Resume |
| `s` | Save snapshot |
| `SPACE` | Single step (when paused) |

---

## 👁️ What You'll See

```
┌──────────────────────────────────────┐
│  Melvin's Vision (UCA v1)           │
│  F = 0.45*S + 0.35*G + 0.20*C       │
│                                      │
│    [YOUR CAMERA FEED]               │
│    • Cyan attention heatmap         │
│    • Yellow crosshair on focus      │
│    • Real-time @ 20-30 fps          │
│                                      │
│  S:0.67 G:0.35 C:0.42 F:0.54        │
└──────────────────────────────────────┘
```

**Cyan heatmap** = What Melvin finds interesting  
**Yellow crosshair** = Where Melvin is looking RIGHT NOW

---

## 🎈 Test It Out!

All executables work with:
- ✅ Your built-in camera (1920×1080)
- ✅ External USB cameras
- ✅ Both Camera 0 and Camera 1

**You already saw balloons** - so we know it works! 🎈

Try again:
```bash
./run_melvin_camera
```

Then:
- Wave at Melvin 🖐️
- Move balloons 🎈
- Show colorful objects 🎨
- Watch the crosshair follow!

---

## 🛠️ Troubleshooting

### Camera Not Found?
```bash
# Diagnose cameras
cd melvin
python3 test_camera.py

# Test specific camera
python3 test_camera.py test 0
python3 test_camera.py test 1
```

### Grey Screen?
- Close Zoom, FaceTime, Photo Booth
- Check camera permissions
- Try: `./run_melvin_camera` (has better detection)

### No Camera at All?
```bash
# Use test visualization (no camera needed)
cd melvin
python3 visualize_melvin.py
```

Still shows how Melvin's attention works!

---

## 📁 File Locations

All executables are in:
```
/Users/jakegilbert/Desktop/Melvin/Melvin/

├── MELVIN_CAMERA.command        ✅ Double-click
├── run_melvin_camera            ✅ Terminal executable
├── melvin_camera_launcher.sh    ✅ Simple launcher
└── melvin/
    ├── melvin_sees.py           ✅ Auto-detect script
    ├── visualize_camera.py      ✅ Camera visualization
    └── visualize_melvin.py      ✅ Test visualization
```

---

## 💡 Recommendations

**For casual use:**
- Double-click `MELVIN_CAMERA.command` in Finder

**For development:**
- Run `./run_melvin_camera` in Terminal
- See detailed messages

**For quick tests:**
- `cd melvin && python3 melvin_sees.py`
- Fastest method

**No camera?**
- `cd melvin && python3 visualize_melvin.py`
- Works anywhere!

---

## ✅ Summary

**You now have 3 executables that all:**

✅ Auto-detect your working camera  
✅ Launch Melvin's vision  
✅ Show real-time attention overlay  
✅ Track focus with crosshair  
✅ Display S, G, C, F scores  
✅ Work with HD cameras (1920×1080)  

**Pick your favorite and run it!**

---

## 🎉 Quick Test

Right now, try:

```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin
./run_melvin_camera
```

Or just **double-click** `MELVIN_CAMERA.command` in Finder!

You should see yourself with:
- Cyan attention heatmap
- Yellow crosshair tracking interesting things
- Score bars at bottom
- Real-time updates

**Wave your hand and watch Melvin's attention follow it!** 🖐️👁️

---

**Three executables. All working. Pick one and see what Melvin sees!** ✨

