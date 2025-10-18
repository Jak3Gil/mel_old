# 👁️ Melvin Visual Perception - Quick Reference Card

## 🚀 Setup (One Command)

```bash
./setup_perception.sh
```

---

## 🏃 Quick Start

### Test Without Image
```bash
./test_perception
```

### Test With Image
```bash
./test_perception path/to/image.jpg
```

---

## 💻 Minimal Code Example

```cpp
#include "melvin/io/melvin_perception.h"

int main() {
    melvin::Storage storage;
    melvin::perception::VisualPerception perception(&storage);
    melvin::perception::YOLODetector detector;
    
    perception.process_image_file("photo.jpg", detector);
    storage.save("nodes.melvin", "edges.melvin");
    
    return 0;
}
```

---

## 📦 What You Get

| Component | What It Does |
|-----------|--------------|
| **Nodes** | Each detected object = SENSORY node |
| **EXACT Edges** | Objects in SAME frame (spatial) |
| **LEAP Edges** | Objects in DIFFERENT frames (temporal) |

---

## ⚙️ Common Configurations

### High Accuracy
```cpp
YOLODetector::Config config;
config.model_name = "yolov8m.pt";       // Medium model
config.confidence_threshold = 0.4f;     // Higher confidence
```

### Real-time Speed
```cpp
YOLODetector::Config config;
config.model_name = "yolov8n.pt";       // Nano model (fastest)
config.confidence_threshold = 0.25f;    // Standard
```

### Video Tracking
```cpp
VisualPerception::Config config;
config.temporal_window = 10;            // Track 10 frames
config.track_object_persistence = true; // IoU matching
config.create_inter_frame_edges = true; // LEAP edges
```

---

## 📊 Statistics

```cpp
perception.print_stats();
```

**Output:**
```
Frames processed:     100
Objects detected:     1,247
Nodes created:        1,247
EXACT edges (intra):  5,432  ← Same frame
LEAP edges (inter):   2,198  ← Different frames
Avg objects/frame:    12.47
```

---

## 🐍 Python-Only Detection

```bash
python3 melvin/io/detect_objects.py image.jpg
```

**Output (JSON):**
```json
[
  {"label": "person", "confidence": 0.95, "x1": 100, "y1": 200, "x2": 300, "y2": 400},
  {"label": "dog", "confidence": 0.87, "x1": 350, "y1": 180, "x2": 500, "y2": 320}
]
```

---

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| "ultralytics not found" | `pip3 install ultralytics` |
| "No objects detected" | Lower `confidence_threshold` to 0.15 |
| Build fails | `make -f Makefile.perception clean && make -f Makefile.perception` |
| Python script not found | Set `config.python_script_path = "full/path/to/detect_objects.py"` |

---

## 📁 Key Files

| File | Purpose |
|------|---------|
| `melvin/io/detect_objects.py` | YOLO detector |
| `melvin/io/melvin_perception.h` | C++ API |
| `melvin/io/melvin_perception.cpp` | Implementation |
| `melvin/demos/test_visual_perception.cpp` | Tests |
| `Makefile.perception` | Build system |
| `README_PERCEPTION.md` | Full docs |

---

## 🎯 YOLO Models

| Model | Size | Speed | Accuracy | Use Case |
|-------|------|-------|----------|----------|
| yolov8n | 6 MB | **Fastest** | Good | Real-time |
| yolov8s | 22 MB | Fast | Better | Balanced |
| yolov8m | 52 MB | Medium | Best | High accuracy |
| yolov8l | 87 MB | Slow | Excellent | Research |
| yolov8x | 136 MB | Slowest | **Best** | Max accuracy |

---

## 🔥 One-Liners

### Build
```bash
make -f Makefile.perception
```

### Clean
```bash
make -f Makefile.perception clean
```

### Run Demo
```bash
./demo_perception_workflow.sh
```

### Install Deps
```bash
pip3 install ultralytics opencv-python
```

---

## 📖 Full Documentation

- **User Guide**: `README_PERCEPTION.md`
- **Complete Summary**: `VISUAL_PERCEPTION_COMPLETE.md`
- **Examples**: `melvin/demos/example_perception_usage.cpp`

---

## 💡 Quick Tips

1. **Start with tests**: Run `./test_perception` first
2. **Use verbose mode** while learning: `config.verbose = true`
3. **Track video sequences**: Enable `create_inter_frame_edges`
4. **Filter noise**: Set `min_confidence` and `min_box_area`
5. **Monitor performance**: Call `perception.print_stats()`

---

## ✅ Checklist

- [ ] Installed Python dependencies
- [ ] Built C++ module
- [ ] Ran tests successfully
- [ ] Processed first image
- [ ] Checked graph with `storage.print_stats()`
- [ ] Saved graph with `storage.save()`

---

**That's it! You're ready to give Melvin eyes! 👁️**


