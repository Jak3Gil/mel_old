# 👁️ Melvin Vision - See What Melvin Sees

## Quick Start

### Option 1: Test Visualization (No Camera Needed)
```bash
cd melvin
python3 visualize_melvin.py
```

**Shows:**
- Moving objects, shape changes, multiple objects
- Real-time attention heatmap
- Focus selection with crosshair
- Score breakdown (S, G, C, F)
- Melvin's thoughts

**Controls:**
- `q` - Quit
- `p` - Pause/Resume
- `SPACE` - Single step (when paused)

---

### Option 2: Live Camera View
```bash
cd melvin
python3 visualize_camera.py
```

**Shows:**
- Your real camera feed
- What Melvin pays attention to
- Attention heatmap overlay
- Focus tracking (crosshair follows interesting things)
- Score visualization

**Controls:**
- `q` - Quit
- `p` - Pause/Resume
- `s` - Save snapshot
- `SPACE` - Single step (when paused)

---

## What You're Seeing

### The Display Layout

**Test Visualization (`visualize_melvin.py`):**
```
┌─────────────────────────────────────────────────────────┐
│  Input Image  │  Patch Grid  │  Attention Heatmap       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│           Melvin's Focus (with crosshair)              │
│                                                         │
├─────────────────────────────────────────────────────────┤
│    Spacer    │  Score Panel  │  Thought Panel          │
└─────────────────────────────────────────────────────────┘
```

**Camera View (`visualize_camera.py`):**
```
┌─────────────────────────────────────────────┐
│  Melvin's Vision (UCA v1)                  │
│  F = 0.45*S + 0.35*G + 0.20*C              │
│                                             │
│         [Live Camera Feed]                  │
│                                             │
│    with attention heatmap overlay          │
│    and focus crosshair                     │
│                                             │
│  Frame: 123                                │
└─────────────────────────────────────────────┘
```

---

## Understanding Melvin's Attention

### The Attention Formula

```
F = α·S + β·G + γ·C
  = 0.45·Saliency + 0.35·Goal + 0.20·Curiosity
```

### Components Explained

**🟢 Saliency (S)** - Bottom-up attention
- What naturally stands out?
- High contrast, edges, variation
- "This catches my eye"

**🟡 Goal (G)** - Top-down attention
- What's relevant to current task?
- Matches active concepts
- "This is what I'm looking for"

**🟣 Curiosity (C)** - Novelty detection
- What's unexpected or new?
- High edge density, prediction errors
- "This is interesting!"

**🔵 Focus (F)** - Combined score
- Where Melvin actually looks
- Winner-take-all selection
- Crosshair shows current focus

---

## Visual Elements

### Attention Heatmap
- **Cyan/Teal colors** show attention intensity
- Brighter = higher attention
- Darker = lower attention

### Focus Crosshair
- **Yellow crosshair** marks current focus
- Circle radius ~50 pixels
- Only ONE focus at a time (like human attention)

### Focus Trail
- **Cyan line** shows where focus has been
- Last 20-30 positions tracked
- Shows attention shifts over time

### Patch Grid
- **Gray lines** divide image into 32×32 patches
- Each patch scored independently
- Mimics V1 receptive fields

### Score Bars (Test Visualization)
- **Green** - Saliency
- **Yellow** - Goal relevance
- **Purple** - Curiosity
- **Cyan** - Combined focus

---

## What to Look For

### Attention Behaviors

**1. Saliency-Driven (No Active Goal)**
- Focus jumps to high-contrast regions
- Edges, motion, bright colors
- Pure bottom-up attention

**2. Goal-Directed (With Active Concepts)**
- Focus biased toward relevant objects
- Less distraction from salient but irrelevant items
- Top-down modulation working

**3. Curiosity-Driven (Novelty Detection)**
- Focus drawn to unexpected changes
- New objects appearing
- Prediction errors

**4. Inhibition of Return (IOR)**
- Focus doesn't immediately return to same spot
- 0.8 second suppression
- Forces exploration

**5. Focus Inertia**
- Current focus gets 15% boost
- Prevents constant jumping
- Stable tracking

---

## Interesting Things to Try

### With Test Visualization
1. **Watch the moving object** (frames 0-50)
   - See how focus tracks motion
   - Saliency + curiosity high

2. **Observe shape changes** (frames 50-100)
   - Pulsing square triggers curiosity
   - Focus locks on change

3. **Multiple objects** (frames 100+)
   - Focus shifts between objects
   - IOR prevents revisiting
   - Exploration behavior

### With Camera
1. **Move your hand in view**
   - Motion creates saliency
   - Focus follows movement

2. **Show high-contrast objects**
   - Books, colorful items
   - Edge-rich things

3. **Stay still, then move suddenly**
   - Triggers curiosity spike
   - Unexpected = interesting

4. **Wave multiple objects**
   - Watch focus shift between them
   - IOR forces sampling

---

## Technical Details

### Frame Processing
- **Resolution**: 640×480 (default)
- **Patch size**: 32×32 pixels
- **Grid**: 20×15 = 300 patches
- **Processing**: ~1-5ms per frame

### Attention Computation
```python
# For each patch:
saliency = std_deviation(patch) / 128.0
goal = 0.3  # From active concepts
curiosity = edge_density(patch)

focus = 0.45*saliency + 0.35*goal + 0.20*curiosity
```

### Focus Selection
```python
# Winner-take-all:
best_patch = max(all_patches, key=lambda p: p.focus)

# With IOR suppression:
if patch in recently_focused:
    exclude from selection

# Apply focus:
draw_crosshair(best_patch.center)
```

---

## Biological Parallels

| Visual Element | Brain Analog | Function |
|----------------|--------------|----------|
| Patch grid | V1 receptive fields | Local feature detection |
| Saliency map | Posterior parietal | Bottom-up attention |
| Goal signals | Prefrontal cortex | Top-down control |
| Focus selection | FEF + Superior Colliculus | Saccade generation |
| IOR | Pulvinar thalamus | Novelty detection |
| Focus trail | Oculomotor history | Scanpath |

---

## Troubleshooting

### Camera Not Working?
```bash
# Check if camera exists
ls /dev/video*

# Try different camera ID
python3 visualize_camera.py  # Edit camera_id=0 to 1, 2, etc.

# Use test visualization instead
python3 visualize_melvin.py
```

### Slow Performance?
```bash
# Reduce resolution in script:
self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)

# Or increase patch size:
self.patch_size = 64  # Fewer patches = faster
```

### No OpenCV/NumPy?
```bash
pip3 install opencv-python numpy
```

---

## Integration with C++ UCA

These visualizations show what Melvin's C++ UCA system does internally:

```cpp
// In unified_mind.cpp:

// 1. Vision tokenization
auto frame_id = vision_->ingest_frame(fm, rgb);

// 2. Attention scoring
auto candidates = vision_->score_regions(frame_id);
// Each candidate has S, G, C scores

// 3. Focus selection
auto focus = focus_->select(candidates);
// Implements F = α·S + β·G + γ·C with IOR

// 4. Reasoning from focus
auto thought = reason_->infer_from_focus(focus.node_id);

// 5. Feedback loop
active_concepts_ = reason_->active_concepts();
vision_->set_active_concepts(active_concepts_);
// Next frame's G scores biased by thoughts!
```

The Python visualizations replicate this pipeline for human viewing.

---

## Next Steps

1. **Understand the formulas**
   - Watch how S, G, C combine
   - See F = 0.45·S + 0.35·G + 0.20·C in action

2. **Observe behaviors**
   - Saliency-driven exploration
   - Goal-directed search (when implemented)
   - Curiosity about novelty

3. **Integrate with C++ UCA**
   - Run `./build/melvin_uca` alongside visualization
   - See same attention patterns in graph
   - Match focus decisions

4. **Extend the system**
   - Add concept recognition
   - Implement goal-setting
   - Track objects over time
   - Add speech output

---

## Quick Commands

```bash
# Test visualization (synthetic scenes)
python3 visualize_melvin.py

# Camera visualization (live feed)
python3 visualize_camera.py

# Save snapshots (in camera mode)
# Press 's' key while running

# Pause and step through
# Press 'p' to pause, SPACE to step
```

---

**Enjoy watching Melvin see! 👁️🧠✨**

*The visualizations show the same attention mechanisms that run in the C++ UCA core.*


