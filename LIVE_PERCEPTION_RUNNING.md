# 🎥 LIVE VISUAL PERCEPTION - NOW RUNNING!

## ✅ System Status: ACTIVE

Your Melvin Visual Perception Engine is **LIVE** and processing your USB camera feed in real-time!

---

## 🎯 What's Happening Right Now

### Camera Feed
- **Camera**: UVC Camera (USB camera, index 1)
- **Model**: YOLOv8 Nano (yolov8n.pt) - optimized for speed
- **Confidence**: 0.3 threshold
- **Status**: ✅ RUNNING

### Real-Time Processing

Every frame is:
1. **Captured** from your USB camera
2. **Analyzed** by YOLO for object detection
3. **Converted** into knowledge graph nodes and edges
4. **Displayed** with bounding boxes and labels

---

## 📺 What You See

### Video Window: "Melvin Visual Perception"

Shows:
- **Live camera feed** from your USB camera
- **Green bounding boxes** around detected objects
- **Labels** with object name and confidence (e.g., "person 0.95")
- **Stats overlay** at the top:
  - Frame count
  - Objects in current frame
  - Total nodes in graph
  - Processing FPS

### Example Detection:
```
┌─────────────────────────────────────┐
│ Frame: 142 | Objects: 3 | Nodes: 487 | FPS: 12.4 │
│                                     │
│   ┌──────────┐                     │
│   │ person   │  0.95                │
│   │          │                      │
│   └──────────┘                      │
│                                     │
│        ┌────────┐                   │
│        │ laptop │ 0.87              │
│        └────────┘                   │
│                                     │
│                  ┌──────┐           │
│                  │ cup  │ 0.72      │
│                  └──────┘           │
└─────────────────────────────────────┘
```

---

## ⌨️ Controls

| Key | Action |
|-----|--------|
| **q** | Quit and show final statistics |
| **s** | Show current stats in terminal |
| **ESC** | Close window |

---

## 🧠 Knowledge Graph Being Built

### Nodes Created
Every detected object becomes a **SENSORY node**:

```
Node ID: 487
Content: "object:person:142:0"
         └──┬──┘ └──┬─┘ └┬┘ └┘
            │      │     │  └─ Object index in frame
            │      │     └──── Frame number
            │      └────────── Object label
            └───────────────── Node type
Type: SENSORY
Activation: 0.95 (confidence)
```

### Edges Created

#### EXACT Edges (Intra-Frame)
Objects in the **same frame** are connected:

```
Frame 142: [person, laptop, cup]

person ←──EXACT──→ laptop  (appeared together)
person ←──EXACT──→ cup
laptop ←──EXACT──→ cup

Meaning: "These objects co-occurred in space"
```

#### LEAP Edges (Inter-Frame)
Same object across **different frames**:

```
Frame 141: [person]
              │
              └──LEAP──→ Frame 142: [person]

Meaning: "Person persisted across time"
```

---

## 📊 Real-Time Statistics

### In-Terminal Stats (Press 's')

```
📊 Current Stats:
  Frames: 142
  Objects detected: 487
  Nodes in graph: 487
  Edges in graph: 1,249
  EXACT edges: 823
  LEAP edges: 426
  Avg objects/frame: 3.43
  FPS: 12.4
```

---

## 💾 What Gets Saved

### When You Press 'q'

The system will:
1. Stop the camera
2. Print **final statistics**
3. Save **graph snapshot** to `perception_graph_snapshot.json`

### Snapshot Contains:

```json
{
  "metadata": {
    "frames": 142,
    "total_objects": 487,
    "duration": 11.45
  },
  "nodes": [
    {
      "id": 0,
      "content": "object:person:1:0",
      "type": "SENSORY",
      "activation": 0.95
    },
    ...
  ],
  "edges": [
    {
      "from": 0,
      "to": 1,
      "type": "EXACT",
      "weight": 1.0
    },
    ...
  ]
}
```

---

## 🔍 Example Session Output

When you quit (press 'q'), you'll see:

```
👋 Stopping...

╔════════════════════════════════════════════════════════════════╗
║  👁️  LIVE PERCEPTION SESSION COMPLETE                          ║
╚════════════════════════════════════════════════════════════════╝

📊 Final Statistics:
  Duration:            11.45 seconds
  Frames processed:    142
  Objects detected:    487
  Nodes created:       487
  Edges created:       1,249
    ├─ EXACT (intra):  823  ← Same-frame co-occurrence
    └─ LEAP (inter):   426  ← Cross-frame temporal links
  Avg objects/frame:   3.43
  Processing FPS:      12.40

💾 Graph snapshot saved to: perception_graph_snapshot.json
   (In production, this would be saved to nodes.melvin/edges.melvin)
```

---

## 🎯 Real-World Example

Let's say you're sitting at your desk with a laptop and coffee cup:

### Frame 1 (t=0s):
**Detected**: person, laptop, cup

**Graph created**:
```
Nodes:
- object:person:1:0 (conf=0.95)
- object:laptop:1:1 (conf=0.87)
- object:cup:1:2 (conf=0.72)

EXACT Edges (same frame):
- person ↔ laptop (weight=1.0)
- person ↔ cup (weight=1.0)
- laptop ↔ cup (weight=1.0)
```

### Frame 30 (t=2.5s):
**Detected**: person, laptop, cup (still there)

**Additional graph**:
```
Nodes:
- object:person:30:0 (conf=0.96)
- object:laptop:30:1 (conf=0.88)
- object:cup:30:2 (conf=0.75)

EXACT Edges (same frame):
- person ↔ laptop
- person ↔ cup
- laptop ↔ cup

LEAP Edges (across frames):
- Frame1:person → Frame30:person (weight=0.5)
- Frame1:laptop → Frame30:laptop (weight=0.5)
- Frame1:cup → Frame30:cup (weight=0.5)
```

**What Melvin learned**:
- "Person, laptop, and cup appear together" (EXACT edges)
- "These objects persist over time" (LEAP edges)
- "Person is likely working with laptop and drinking from cup"

---

## 🚀 Performance

### Current Configuration

- **Model**: YOLOv8 Nano (fastest)
- **Confidence**: 0.3 (balanced)
- **Expected FPS**: 10-20 fps (CPU) or 60+ fps (GPU)

### Optimization Tips

**For Speed** (real-time):
```bash
python3 live_camera_perception.py --camera 1 --model yolov8n.pt --confidence 0.4
```

**For Accuracy**:
```bash
python3 live_camera_perception.py --camera 1 --model yolov8m.pt --confidence 0.25
```

---

## 📋 Quick Commands

### Check Status
```bash
./check_perception_status.sh
```

### Stop System
Press **'q'** in the video window

### Restart System
```bash
python3 live_camera_perception.py --camera 1
```

### View Different Camera
```bash
python3 live_camera_perception.py --camera 0  # FaceTime HD
python3 live_camera_perception.py --camera 1  # USB Camera (current)
python3 live_camera_perception.py --camera 2  # iPhone Camera
```

---

## 🎨 Object Classes Detected

YOLO can detect **80 object classes**:

**Common objects**:
- People: person
- Vehicles: car, truck, bus, motorcycle, bicycle
- Animals: dog, cat, bird, horse, cow, sheep
- Furniture: chair, couch, bed, table
- Electronics: laptop, mouse, keyboard, cell phone, tv, monitor
- Kitchen: cup, bottle, bowl, knife, spoon, fork
- And 60+ more!

---

## 💡 What This Enables

### Immediate Applications

1. **Scene Understanding**
   - "What objects are in my workspace?"
   - "What items appear together frequently?"

2. **Activity Detection**
   - "When did I use my laptop?"
   - "How long was the cup on my desk?"

3. **Object Tracking**
   - "Where did the dog go?"
   - "Is my phone on the desk?"

4. **Pattern Recognition**
   - "Person always appears with laptop"
   - "Cup appears near keyboard 80% of the time"

### Future Integration

Once integrated with Melvin's reasoning engine:
- **Query**: "What objects are usually near me when I work?"
  - **Answer**: laptop, cup, keyboard (via EXACT edges)
  
- **Query**: "What objects persist the longest?"
  - **Answer**: laptop, monitor (via many LEAP edges)

---

## 🎯 Next Steps

### While Running
1. ✅ Let it run and detect objects in your environment
2. ✅ Move things around to test temporal tracking
3. ✅ Press 's' to see live stats
4. ✅ Press 'q' when done to save the graph

### After Session
1. View the graph snapshot: `cat perception_graph_snapshot.json`
2. Analyze patterns in the data
3. Integrate with Melvin's reasoning engine
4. Run queries on the knowledge graph!

---

## ✨ Summary

You have a **fully functional real-time visual perception system** that:

✅ Captures live video from your USB camera  
✅ Detects objects using state-of-the-art YOLO  
✅ Builds a knowledge graph in real-time  
✅ Creates spatial relationships (EXACT edges)  
✅ Tracks temporal patterns (LEAP edges)  
✅ Displays live visualization  
✅ Saves complete graph snapshots  

**This is Melvin's "eyes" - now working in real-time!** 👁️🧠

---

**Enjoy watching Melvin see and understand your world!** 🎉


