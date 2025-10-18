# 📹 Camera Fix - See Yourself!

## Your Issue: Grey Background

You're seeing a grey background because the camera frame isn't displaying properly. Here's how to fix it:

---

## ✅ QUICK FIX (Try These First)

### Option 1: Use Auto-Detect Script
```bash
cd melvin
python3 melvin_sees.py
```

This automatically finds a working camera and starts Melvin!

### Option 2: Pick Camera Manually
```bash
cd melvin
python3 test_camera.py test 0    # Test camera 0
# OR
python3 test_camera.py test 1    # Test camera 1
```

Watch the preview. If you see yourself, that camera works!

### Option 3: Use Camera 1 Instead
```bash
python3 melvin_sees.py 1
```

Your Mac has 2 cameras (0 and 1). Try both!

---

## 🔍 Diagnose the Problem

### Step 1: Find Working Cameras
```bash
cd melvin
python3 test_camera.py
```

This shows all available cameras and tests them.

### Step 2: Test Each Camera Live
```bash
# Test camera 0
python3 test_camera.py test 0

# If that doesn't work, try camera 1
python3 test_camera.py test 1
```

Press 'q' to quit the test.

---

## 🛠️ Common Issues & Fixes

### Issue 1: Camera Permission Denied

**macOS:**
1. Go to **System Preferences** → **Security & Privacy** → **Camera**
2. Make sure **Terminal** (or your IDE) has camera access
3. Restart Terminal
4. Try again

### Issue 2: Another App Using Camera

Close these apps:
- Zoom
- FaceTime
- Skype
- Photo Booth
- Any video chat apps

Then try again.

### Issue 3: Wrong Camera Selected

You have 2 cameras:
- **Camera 0**: Usually built-in webcam
- **Camera 1**: External or Continuity camera

Try both:
```bash
python3 melvin_sees.py 0   # Try built-in
python3 melvin_sees.py 1   # Try other
```

### Issue 4: High Resolution Issues

Your camera is 1920x1080 (HD). The new script handles this automatically by using larger patches (64×64 instead of 32×32).

---

## ✨ Updated Scripts (Fixed for You!)

I've created these new tools:

1. **melvin_sees.py** - Smart launcher (auto-detects camera)
   ```bash
   python3 melvin_sees.py
   ```

2. **test_camera.py** - Diagnostic tool
   ```bash
   python3 test_camera.py          # Find cameras
   python3 test_camera.py test 0   # Live test
   ```

3. **pick_camera.sh** - Interactive menu
   ```bash
   ./pick_camera.sh
   ```

4. **visualize_camera.py** - Updated (better error handling)
   - Now handles high-res cameras (1920x1080)
   - Better error messages
   - Auto-reconnect if frame drops

---

## 🎯 Recommended Steps

### Step 1: Quick Test
```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin/melvin
python3 test_camera.py test 0
```

**Do you see yourself?**
- ✅ YES → Press 'q', then run: `python3 melvin_sees.py 0`
- ❌ NO → Try: `python3 test_camera.py test 1`

### Step 2: Run Melvin Vision
Once you find a working camera:
```bash
python3 melvin_sees.py 0    # or 1, whichever works
```

You should now see:
- ✅ **Yourself** in the video
- ✅ **Cyan attention heatmap** overlay
- ✅ **Yellow crosshair** on focus point
- ✅ **Score bars** at bottom

### Step 3: Interact!
- 🖐️ **Wave your hand** - watch crosshair follow movement
- 🎨 **Show colorful objects** - high saliency
- 📱 **Move your phone** - motion detection
- 👀 **Move your face closer/away** - attention shifts

---

## 🧪 Test Results from Your Mac

Your diagnostic showed:
```
✅ Camera 0 working - 1920x1080 @ 30fps
✅ Camera 1 working - 1920x1080 @ 1fps
```

**Recommendation:** Use Camera 0 (it's 30fps, much smoother!)

```bash
python3 melvin_sees.py 0
```

---

## 📊 What You Should See

```
┌─────────────────────────────────────────────┐
│  Melvin's Vision (UCA v1)                  │
│  F = 0.45*S + 0.35*G + 0.20*C              │
│                                             │
│         [YOUR FACE HERE!]                   │
│    with cyan heatmap showing attention     │
│    and yellow crosshair on focus          │
│                                             │
└─────────────────────────────────────────────┘
```

If you see **grey** instead of your face:
1. Camera permissions issue
2. Wrong camera selected
3. Another app using camera

---

## 🆘 Still Not Working?

### Try This:
```bash
# 1. Kill any background processes
killall Python python3

# 2. Test camera directly
python3 test_camera.py test 0

# 3. If you see yourself, run:
python3 melvin_sees.py 0
```

### Check Camera Permissions:
```bash
# macOS
open "x-apple.systempreferences:com.apple.preference.security?Privacy_Camera"
```

Make sure Terminal has camera access!

### Still grey? Use Test Visualization Instead:
```bash
python3 visualize_melvin.py
```

This uses synthetic scenes (no camera needed) and still shows how Melvin's attention works!

---

## 🎮 Controls (Once Working)

- `q` - Quit
- `p` - Pause/Resume
- `s` - Save snapshot
- `SPACE` - Single step

---

## 📝 Quick Command Reference

```bash
# Auto-detect and run
python3 melvin_sees.py

# Specify camera
python3 melvin_sees.py 0    # Camera 0
python3 melvin_sees.py 1    # Camera 1

# Test cameras
python3 test_camera.py          # Find all
python3 test_camera.py test 0   # Live test cam 0

# Interactive menu
./pick_camera.sh

# Test visualization (no camera)
python3 visualize_melvin.py
```

---

## ✅ Expected Result

Once working, you'll see:
- **Your face/body** in the video
- **Cyan heatmap** showing Melvin's attention
- **Yellow crosshair** tracking interesting regions
- **Real-time updates** (~20-30 fps)
- **Scores** showing S, G, C, F values

**Wave at Melvin and watch the crosshair follow your hand!** 🖐️👁️

---

Need more help? Run:
```bash
python3 melvin_sees.py --help
```

