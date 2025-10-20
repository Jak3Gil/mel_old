# 🎥 MELVIN v2 - Live Camera Vision Quick Start

## See MELVIN's Evolved Vision in Real-Time!

This shows you **exactly** how genome-driven vision works with a live camera feed.

---

## Quick Start (2 Minutes)

### 1. Install Dependencies

```bash
pip install opencv-python numpy
```

### 2. Run Live Camera

```bash
python3 melvin/v2/tools/live_camera_vision.py
```

### 3. What You'll See

**Real-time camera feed with overlays showing:**

- 📦 **Detected objects** (gray boxes)
- 🎯 **Focused object** (yellow box with crosshair)
- 📊 **Saliency scores** (genome-weighted: edge + motion + color)
- 🧬 **Genome parameters** (edge_weight, motion_weight, color_weight)
- 🧪 **Neuromodulator bars** (DA/NE/ACh levels)
- 💭 **Attention scores** (α·S + β·R + γ·C)

---

## Controls

| Key | Action |
|-----|--------|
| **q** | Quit |
| **e** | Toggle edge weight (0.1 ↔ 0.9) |
| **m** | Toggle motion weight (0.1 ↔ 0.9) |
| **c** | Toggle color weight (0.1 ↔ 0.9) |
| **s** | Save genome (future) |

---

## What It Demonstrates

### Genome Controls Vision

**Press 'c' to toggle color weight:**

```
color_weight = 0.1 (low):
  → Red objects less salient
  → Focus shifts to edges/motion
  → Different behavior!

color_weight = 0.9 (high):
  → Red objects highly salient
  → Focus locks on colorful items
  → Completely different attention!
```

**This proves: Genome genes directly control what MELVIN notices!**

### Neuromodulators Respond

**Watch the DA bar:**

```
When tracking successfully:
  DA bar grows → Learning faster

When losing track:
  DA bar shrinks → Exploring more
  NE bar grows → Unexpected uncertainty
```

**This proves: Visual prediction errors modulate learning!**

### Attention Formula Visible

**Top of each object shows:**

```
S: 0.73  ← Saliency score
  = 0.30×edges + 0.40×motion + 0.30×color
  = 0.30×0.45 + 0.40×0.80 + 0.30×0.90
  = 0.73
```

**Focused object gets yellow box + crosshair**

**This proves: Genome weights determine what gets attention!**

---

## Experiment: Evolution in Real-Time

### Test Different Genomes

**1. High color weight (good for colorful scenes):**

```bash
# Edit the script or press 'c' multiple times
color_weight: 0.9
edge_weight: 0.05
motion_weight: 0.05

Result: Locks onto red/blue/green objects
        Ignores gray/white objects
```

**2. High edge weight (good for structured scenes):**

```bash
edge_weight: 0.9
motion_weight: 0.05
color_weight: 0.05

Result: Locks onto corners, edges, text
        Ignores smooth surfaces
```

**3. High motion weight (good for dynamic scenes):**

```bash
motion_weight: 0.9
edge_weight: 0.05
color_weight: 0.05

Result: Locks onto moving objects
        Ignores stationary items
```

**See the difference in real-time!**

---

## Using Custom Genome

### 1. Create Custom Genome

```bash
# Run genome demo to create one
make v2-demo

# Genome saved to /tmp/demo_genome.json
```

### 2. Load in Camera Viewer

```bash
# Edit live_camera_vision.py line 235:
genome_path = "/tmp/demo_genome.json"

# Or modify the genome
vim /tmp/demo_genome.json
# Change vision.color_weight to 0.95
```

### 3. See Your Genome's Vision!

```bash
python3 melvin/v2/tools/live_camera_vision.py
```

**The camera will use YOUR genome's vision parameters!**

---

## Troubleshooting

### "Cannot open camera"

```bash
# Check camera permissions (Mac)
# System Settings → Privacy & Security → Camera

# Try different camera index
# Edit script line 245: camera_index = 1
```

### "opencv not found"

```bash
pip install opencv-python
# or
pip3 install opencv-python
```

### "No objects detected"

- Move to brighter area
- Wave your hand (motion detection)
- Show colorful objects
- Adjust genome weights (press e/m/c)

---

## Example Session

```bash
$ python3 melvin/v2/tools/live_camera_vision.py

======================================================================
🎥 MELVIN v2 - Live Camera Vision (Genome-Driven)
======================================================================

✓ Loading genome from /tmp/demo_genome.json

GENOME VISION PARAMETERS:
Edge: threshold=0.30, weight=0.30
Motion: sensitivity=0.50, weight=0.40
Color: threshold=0.20, weight=0.30
Attention: α=0.40, β=0.30, γ=0.20

✓ Camera 0 opened

CONTROLS:
  q - Quit
  e/m/c - Toggle weights
  
Starting camera feed...

[Camera window opens showing:]
  • Real-time video
  • Objects outlined in gray
  • Focused object in yellow with crosshair
  • Genome parameters displayed
  • Neuromodulator bars
  
[Press 'c' to boost color]
Color weight: 0.90

[Watch focus shift to red mug!]
  
[Press 'e' to boost edges]
Edge weight: 0.90

[Watch focus shift to book corners!]

[Press 'q']
👋 Quitting...
✓ Processed 847 frames
```

---

## Advanced: Evolution Comparison

### Compare 3 Genomes Side-by-Side

**Run 3 instances with different genomes:**

```bash
# Terminal 1: Color-focused
python3 melvin/v2/tools/live_camera_vision.py
# (Press 'c' several times)

# Terminal 2: Edge-focused  
python3 melvin/v2/tools/live_camera_vision.py
# (Press 'e' several times)

# Terminal 3: Motion-focused
python3 melvin/v2/tools/live_camera_vision.py
# (Press 'm' several times)
```

**Wave a red ball in front of all 3:**
- Color genome: ✅ Tracks ball (color = 0.9)
- Edge genome: ❌ Ignores ball (smooth surface)
- Motion genome: ⚠️ Tracks but distracted by hand

**This visually proves different genomes → different vision!**

---

## What This Proves

✅ **Genome controls perception** - Different weights → different focus  
✅ **Real-time demonstration** - See gene effects immediately  
✅ **Neuromodulator response** - DA/NE bars react to tracking  
✅ **Evolution substrate** - Change genes → different behavior  
✅ **Task optimization** - Can evolve for specific visual tasks  

---

## Next Steps

### Connect to Full v2 Brain

```python
# Instead of standalone, integrate with UnifiedLoopV2:

import subprocess
import json

# Send frame to C++ brain
result = subprocess.run(
    ['./melvin_v2_brain', '--frame', 'frame.png'],
    capture_output=True
)

# Get back: focus, neuromod, WM state
brain_state = json.loads(result.stdout)

# Display on camera view
```

### Run Evolution

```python
# Evaluate multiple genomes on camera task
for genome in population:
    accuracy = test_genome_vision(genome, camera, duration=60)
    fitness[genome] = accuracy

# Select best → breed → mutate → repeat
```

---

## Summary

**The live camera viewer shows:**

1. **Real camera feed** (your webcam)
2. **Genome-driven object detection** (edge/motion/color genes)
3. **Genome-weighted attention** (α·S + β·R + γ·C formula)
4. **Focus selection** (yellow crosshair on chosen object)
5. **Neuromodulator dynamics** (DA/NE/ACh bars)
6. **Interactive gene editing** (e/m/c keys to change weights)

**You can SEE evolution working!**

Press 'c' → color weight changes → focus shifts → different behavior!

**This is genome-driven vision in action!** 🎥🧬🧠

---

**Run it now:** `python3 melvin/v2/tools/live_camera_vision.py`

