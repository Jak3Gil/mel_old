# 👁️ Melvin Visual Perception Engine (YOLO Integration)

## Overview

This module integrates **YOLOv8 object detection** directly into Melvin's knowledge graph. Each detected bounding box becomes a node, with edges representing spatial and temporal relationships.

### Architecture

```
Camera/Image → YOLO Detection → Objects (bbox) → Knowledge Graph
                                                  ├─ Nodes (SENSORY type)
                                                  ├─ EXACT edges (same frame)
                                                  └─ LEAP edges (across frames)
```

## Key Concepts

### 1. **Object → Node Mapping**
- Each detected object becomes a **SENSORY node**
- Node content: `"object:<label>:<frame_num>:<obj_idx>"`
- Node activation = detection confidence

### 2. **Edge Types**
- **EXACT edges**: Connect objects in the **same frame** (spatial co-occurrence)
  - Example: `person` ↔ `dog` in frame 1
- **LEAP edges**: Connect objects **across frames** (temporal inference)
  - Example: `person` in frame 1 → `person` in frame 2

### 3. **Temporal Tracking**
- Maintains history of recent frames (default: 5 frames)
- Links similar objects across time
- Optional IoU-based tracking for object persistence

---

## File Structure

```
melvin/
├── io/
│   ├── detect_objects.py       # Python YOLO detector
│   ├── melvin_perception.h     # C++ header
│   └── melvin_perception.cpp   # C++ implementation
├── demos/
│   └── test_visual_perception.cpp  # Test harness
└── Makefile.perception         # Build system
```

---

## Installation

### 1. Install Python Dependencies

```bash
pip install -r requirements.txt
```

This installs:
- `ultralytics` (YOLOv8)
- `opencv-python` (image/video processing)

### 2. Build C++ Perception Module

```bash
make -f Makefile.perception
```

This creates the `test_perception` executable.

---

## Usage

### Quick Test (No Image Required)

Run unit tests with mock data:

```bash
./test_perception
```

**Output:**
- ✅ Single frame processing
- ✅ Multi-frame temporal processing
- ✅ Edge type verification (EXACT vs LEAP)
- ✅ Graph persistence (save/load)

### Full Pipeline Test (With Image)

Test YOLO detection on a real image:

```bash
./test_perception path/to/image.jpg
```

**What happens:**
1. Python script runs YOLO on the image
2. Detections are parsed from JSON
3. Objects become nodes in the graph
4. Edges are created between objects
5. Graph is persisted to `nodes.melvin` / `edges.melvin`

---

## Python API

### Standalone Object Detection

```bash
python3 melvin/io/detect_objects.py image.jpg
```

**Output (JSON):**
```json
[
  {
    "label": "person",
    "confidence": 0.95,
    "x1": 100, "y1": 200,
    "x2": 300, "y2": 400
  },
  {
    "label": "dog",
    "confidence": 0.87,
    "x1": 350, "y1": 180,
    "x2": 500, "y2": 320
  }
]
```

### Custom Model

```bash
python3 melvin/io/detect_objects.py image.jpg yolov8m.pt 0.5
#                                              ↑ model   ↑ confidence
```

Models:
- `yolov8n.pt` - Nano (fastest, smallest)
- `yolov8s.pt` - Small
- `yolov8m.pt` - Medium (default)
- `yolov8l.pt` - Large
- `yolov8x.pt` - Extra large (most accurate)

---

## C++ API

### Basic Usage

```cpp
#include "melvin/io/melvin_perception.h"

using namespace melvin::perception;

int main() {
    // Create storage
    melvin::Storage storage;
    
    // Create perception engine
    VisualPerception perception(&storage);
    
    // Process an image
    YOLODetector detector;
    perception.process_image_file("image.jpg", detector);
    
    // Print stats
    perception.print_stats();
    storage.print_stats();
    
    // Save graph
    storage.save("nodes.melvin", "edges.melvin");
    
    return 0;
}
```

### Advanced Configuration

```cpp
VisualPerception::Config config;

// Node creation
config.create_object_nodes = true;
config.create_frame_summary_node = false;

// Edge creation
config.create_intra_frame_edges = true;   // EXACT edges (same frame)
config.create_inter_frame_edges = true;   // LEAP edges (across frames)
config.intra_frame_weight = 1.0f;
config.inter_frame_weight = 0.5f;

// Temporal tracking
config.temporal_window = 5;               // Track last 5 frames
config.track_object_persistence = true;   // Use IoU matching
config.iou_threshold = 0.5f;              // Minimum IoU for tracking

// Filtering
config.min_confidence = 0.25f;            // Min detection confidence
config.min_box_area = 100;                // Min bbox area (px²)

config.verbose = true;

VisualPerception perception(&storage, config);
```

### Process Multiple Frames

```cpp
YOLODetector detector;

for (int i = 1; i <= 10; i++) {
    std::string img = "frame_" + std::to_string(i) + ".jpg";
    perception.process_image_file(img, detector);
}

// Temporal LEAP edges will connect objects across all frames
```

---

## Examples

### Example 1: Security Camera

Detect people and vehicles in surveillance footage:

```cpp
Storage storage;
VisualPerception::Config config;
config.temporal_window = 10;  // Track across 10 frames
config.track_object_persistence = true;

VisualPerception perception(&storage, config);
YOLODetector detector;

// Process video frames
for (int frame = 1; frame <= 100; frame++) {
    std::string path = "security_cam_frame_" + std::to_string(frame) + ".jpg";
    perception.process_image_file(path, detector);
}

// Save knowledge graph
storage.save("security_nodes.melvin", "security_edges.melvin");
```

### Example 2: Traffic Analysis

Track cars, trucks, and pedestrians:

```cpp
YOLODetector::Config yolo_config;
yolo_config.model_name = "yolov8m.pt";     // More accurate model
yolo_config.confidence_threshold = 0.4f;   // Higher confidence

YOLODetector detector(yolo_config);

VisualPerception::Config perc_config;
perc_config.min_box_area = 500;  // Ignore small detections

VisualPerception perception(&storage, perc_config);

// Process traffic camera images...
```

### Example 3: Scene Understanding

Analyze relationships between objects in a photo:

```cpp
perception.process_image_file("living_room.jpg", detector);

// Graph now contains:
// - person (node)
// - couch (node)
// - tv (node)
// - person ←→ couch (EXACT edge: spatial co-occurrence)
// - person ←→ tv (EXACT edge: spatial co-occurrence)
// - couch ←→ tv (EXACT edge: spatial co-occurrence)

// Query: "What objects are near the person?"
// Answer: couch, tv (via EXACT edges)
```

---

## Statistics & Debugging

### Print Statistics

```cpp
perception.print_stats();
```

**Output:**
```
╔════════════════════════════════════════════════════════════════╗
║  👁️  VISUAL PERCEPTION STATISTICS                              ║
╚════════════════════════════════════════════════════════════════╝

Frames processed:        10
Objects detected:        47
Nodes created:           47
EXACT edges (intra):     210
LEAP edges (inter):      83
Avg objects/frame:       4.70
Avg processing time:     23.45 ms/frame
```

### Verbose Mode

```cpp
config.verbose = true;
```

**Output:**
```
[YOLO] Running: python3 detect_objects.py image.jpg yolov8n.pt 0.25
[YOLO] Detection output: [{"label": "person", ...}]
[Perception] Created node 1 for person (conf=0.95)
[Perception] Created node 2 for dog (conf=0.87)
[Perception] Created 2 intra-frame EXACT edges for frame 1
[Perception] Frame 1: Detected 2 objects, created 2 nodes
```

---

## Performance

### Speed Benchmarks

| Model     | Size | Speed (CPU) | Speed (GPU) | Accuracy |
|-----------|------|-------------|-------------|----------|
| yolov8n   | 6 MB | ~50 ms      | ~5 ms       | Good     |
| yolov8s   | 22 MB| ~80 ms      | ~8 ms       | Better   |
| yolov8m   | 52 MB| ~150 ms     | ~12 ms      | Best     |

*Tested on Intel i7 / NVIDIA RTX 3060*

### Optimization Tips

1. **Use smaller models** for real-time processing (`yolov8n`)
2. **Increase confidence threshold** to reduce false positives
3. **Limit temporal window** to reduce inter-frame edge creation
4. **Filter by area** to ignore tiny detections

---

## Troubleshooting

### "ultralytics not found"

```bash
pip install ultralytics
```

### "YOLO detection script failed"

Check Python script path:
```cpp
YOLODetector::Config config;
config.python_script_path = "melvin/io/detect_objects.py";  // Adjust path
```

### "No objects detected"

- Lower confidence threshold: `config.confidence_threshold = 0.15f`
- Try a larger model: `config.model_name = "yolov8m.pt"`
- Check image quality

### Build Errors

Ensure all core modules are built:
```bash
cd melvin
make clean
make
cd ..
make -f Makefile.perception
```

---

## Integration with Melvin Reasoning

Once objects are in the graph, you can query them using Melvin's reasoning engine:

```cpp
// After processing frames...
melvin::reasoning::Reasoner reasoner(&storage);

// Query: "What objects appear together?"
// (Uses EXACT edges between objects in same frames)

// Query: "What objects appear sequentially?"
// (Uses LEAP edges between objects across frames)
```

---

## Future Enhancements

- [ ] Real-time camera stream processing
- [ ] Object tracking with persistent IDs
- [ ] Semantic scene graphs (relations like "sitting on", "holding")
- [ ] Integration with language models for captioning
- [ ] Multi-modal reasoning (vision + text)

---

## Credits

- **YOLO**: [Ultralytics YOLOv8](https://github.com/ultralytics/ultralytics)
- **Melvin**: Knowledge graph reasoning engine

---

## License

Part of the Melvin project. See main LICENSE file.


