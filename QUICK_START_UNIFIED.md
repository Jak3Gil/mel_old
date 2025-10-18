# 🚀 Quick Start: Unified Melvin Brain

## ONE Command to Rule Them All

```bash
# Build
make -f Makefile.unified

# Run
./melvin_unified
```

That's it! Melvin will start seeing, hearing, and thinking.

---

## What You'll See

```
🧠 Initializing Unified Melvin Brain...
📂 Loaded existing knowledge: X nodes, Y edges
✅ All brain components initialized

🚀 Starting Unified Melvin...
✅ Audio stream started
✅ Camera opened

🧠 Melvin is now perceiving and thinking...
   (Press Ctrl+C to stop)

[Processing camera frames + microphone...]
💭 Melvin: I see dog and person

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 Status Update (t=10s)
   Frames: 100
   Knowledge: 1547 nodes, 4289 edges
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

💾 Knowledge saved (1547 nodes, 4289 edges)
```

Press `Ctrl+C` to stop.

---

## What's Happening

### Every Frame (~10ms):
1. **Camera** captures image → YOLO detects objects → nodes created
2. **Microphone** captures audio → Whisper transcribes → nodes created
3. **Brain Pipeline** runs:
   - Energy injection at inputs
   - Hopfield diffusion (pattern completion)
   - LEAP synthesis (analogies)
   - GNN predictions
   - Adaptive weight updates
   - Reasoning & episodic memory
4. **Self-reflection**: Output text → tokenized → back to graph
5. **Auto-save** every 30s to `melvin_nodes.bin` + `melvin_edges.bin`

### Result:
- Vision + audio concepts linked automatically
- Repeated observations strengthen connections
- Outputs become part of memory
- Persistent learning across sessions

---

## File Locations

| File | Purpose |
|------|---------|
| `melvin_nodes.bin` | All concepts (objects, words, outputs) |
| `melvin_edges.bin` | All connections (spatial, temporal, cross-modal) |
| `melvin_unified.cpp` | Main application code |
| `Makefile.unified` | Build system |
| `UNIFIED_MELVIN_COMPLETE.md` | Full documentation |

---

## Troubleshooting

### Camera not working?
```bash
# Check camera
ls /dev/video*
# Should show: /dev/video0

# Test with Python
python3 -c "import cv2; cap = cv2.VideoCapture(0); print('OK' if cap.isOpened() else 'FAIL')"
```

### Microphone not working?
```bash
# List audio devices
pactl list sources short

# Test recording
arecord -d 3 -f cd test.wav && aplay test.wav
```

### YOLO not installed?
```bash
pip install ultralytics
python3 -c "from ultralytics import YOLO; print('OK')"
```

### Build errors?
```bash
# Install dependencies
sudo apt-get install g++ libopencv-dev portaudio19-dev

# Clean and rebuild
make -f Makefile.unified clean
make -f Makefile.unified all
```

---

## Example Use Cases

### 1. Object-Word Association
- Show Melvin a dog
- Say "dog"
- Melvin links visual and audio "dog"
- Repeat → connection strengthens

### 2. Scene Understanding
- Show multiple objects
- Melvin creates spatial edges between co-occurring objects
- Learns: "person" often with "chair", "dog" often with "person"

### 3. Self-Learning
- Melvin generates thoughts: "I see dog and person"
- These thoughts feed back into memory
- Repeated thoughts strengthen (like human self-talk)

### 4. Continuous Learning
- Run Melvin for hours/days
- Knowledge accumulates
- Restart → loads previous knowledge
- Continues learning from where it left off

---

## What Makes This Special

### ✅ ONE Unified Brain
- Not separate vision/audio systems
- ALL perceptions flow through same cognitive pipeline
- Energy → Hopfield → LEAPs → GNN → Adaptive → Reasoning → Episodic

### ✅ Self-Reflective
- Outputs become inputs
- Learns from own thoughts
- Builds meta-knowledge

### ✅ Cross-Modal
- Automatically links sight and sound
- "dog" (visual) ↔ "dog" (audio)
- Strengthens with repeated co-occurrence

### ✅ Complete Cognitive Architecture
- EnergyField (attention)
- HopfieldDiffusion (pattern completion)
- LeapSynthesis (analogies)
- GNNPredictor (predictions)
- AdaptiveWeighting (learning)
- EpisodicMemory (temporal organization)

---

## Next Steps

1. **Run it**: `./melvin_unified`
2. **Watch it learn**: Show objects, say words
3. **Check knowledge**: Nodes and edges grow
4. **Stop and restart**: Knowledge persists
5. **Read docs**: `UNIFIED_MELVIN_COMPLETE.md`

---

## Commands Summary

```bash
# Build
make -f Makefile.unified

# Run
./melvin_unified

# Clean
make -f Makefile.unified clean

# Info
make -f Makefile.unified info

# Help
make -f Makefile.unified help
```

---

**Ready to run! Melvin can now see, hear, think, and learn!** 🧠👁️🎤

Press Ctrl+C when done. Knowledge auto-saves.

