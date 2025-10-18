# 🔬 Raw Vision System - No Pre-trained Classification

## ✅ What Changed

### OLD System (YOLO-based):
- Used YOLOv8n pre-trained model
- Objects labeled: "person", "car", "dog", "laptop", etc.
- 80 pre-defined object classes
- **Semantic understanding from day 1**

### NEW System (Raw Vision):
- Uses OpenCV adaptive thresholding + variance-based saliency
- Objects labeled: "object_0", "object_1", "object_2", etc.
- **No pre-defined classes**
- **No semantic labels** - pure visual features

## 🧠 How It Works

1. **Capture Frame** from camera
2. **Detect Visual Regions**:
   - Adaptive thresholding to find contrasts
   - Grid-based variance analysis for interesting regions
   - No object classification
3. **Create Generic Nodes**: `object_0`, `object_1`, etc.
4. **Track Across Frames**: Same visual region = same object ID
5. **Build Connections**:
   - **EXACT edges**: Objects appearing together
   - **LEAP edges**: Melvin discovers patterns through experience

## 📊 Node Structure

```
Node: object_0
Type: visual_region
Features:
  - Bounding box: (x1, y1, x2, y2)
  - Area: pixel count
  - Visual features: color, size, aspect ratio
  - NO semantic label
```

## 🔗 Edge Formation

### EXACT Edges (Co-occurrence):
```
object_0 ←→ object_1  (appeared together)
object_0 ←→ object_2  (appeared together)
```

### LEAP Edges (Discovered Patterns):
Melvin will discover through experience:
- Objects that move together
- Objects with similar visual properties
- Spatial relationships
- Temporal patterns

## 🎯 Why This Matters

**Biological Plausibility**:
- Babies don't have pre-trained object classifiers
- Learn categories from experience
- Build concepts bottom-up from visual features

**True Learning**:
- System must learn what "person" means through exposure
- Discovers object categories naturally
- No bias from pre-training

**Flexibility**:
- Can learn ANY object category
- Not limited to 80 YOLO classes
- Adapts to its environment

## 🚀 Running the System

### Text Output Only (No Voice):
```bash
./melvin_unified
```

### What You'll See:
```
[Frame 50] 👁️ Raw vision... 👁️ 5 regions
   object_0 (conf: 0.95)
   object_1 (conf: 0.87)
   ...

💭 Melvin: I see object_0 and object_1

Knowledge: 25 nodes, 42 edges
```

### Knowledge Growth:
- **Nodes**: object_0, object_1, object_2, ...
- **EXACT edges**: Co-occurrence in same frame
- **LEAP edges**: Discovered patterns
- **No semantic labels** until Melvin learns them!

## 🔬 Learning Process

Over time, if you:
1. Show Melvin a person repeatedly → `object_5` becomes associated with certain features
2. Say "person" when showing them → Could link audio "person" to `object_5`
3. Different people → Melvin discovers `object_5, object_7, object_12` are similar
4. Through LEAP synthesis → Discovers category pattern

**Result**: Melvin learns "person" category from bottom-up experience!

## 📝 Detection Method

```python
# Adaptive thresholding for contrast regions
adaptive_thresh = cv2.adaptiveThreshold(gray, ...)

# Grid-based variance for interesting regions  
for each grid cell:
    if variance > threshold:
        region is "interesting"
        create bounding box
        assign object_ID
```

## 🎉 Benefits

✅ No dependence on YOLO or pre-trained models
✅ Pure visual learning from scratch
✅ Biologically plausible development
✅ Can learn unlimited object categories
✅ Adapts to environment naturally
✅ True tabula rasa learning

## ⚙️ Technical Details

**Detection**: OpenCV adaptive thresholding + variance-based grid
**Tracking**: IoU + visual feature similarity
**Features**: Color, size, aspect ratio, position
**Matching**: 0.3 IoU threshold for same object
**Memory**: Tracked for 10 frames after last seen

## 🔄 Next Steps

1. **Run system**: Accumulate visual nodes
2. **Add audio**: Link words to visual patterns
3. **LEAP synthesis**: Discover object categories
4. **Multi-modal**: Vision + audio → concepts

**You now have a system that truly learns from scratch!** 🧠✨

