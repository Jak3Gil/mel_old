# 👁️ How Melvin's Attention Works - Complete Explanation

## The Yellow Dot (Crosshair) - What Is It?

The **yellow crosshair** shows **where Melvin is "looking" right now**. It's like where your eyes focus when you look at a scene.

Just like you can only truly focus on ONE thing at a time, Melvin selects ONE patch (the one with the highest "interest score").

---

## 🧠 The Attention Formula (How the Dot Picks Where to Go)

```
F = 0.45 × S + 0.35 × G + 0.20 × C
```

**F** = **Focus score** (where the dot goes)  
**S** = **Saliency** (what naturally stands out)  
**G** = **Goal** (what's relevant to current task)  
**C** = **Curiosity** (what's unexpected/novel)

**The dot goes to the patch with the HIGHEST F score!**

---

## 📊 Breaking Down Each Component

### 🟢 **S = Saliency (Bottom-Up Attention)**

**"What naturally catches your eye?"**

Saliency measures how much something **stands out** from its surroundings.

**High Saliency (S ≈ 0.7-1.0):**
- ✨ Bright colors (red, yellow, hot pink)
- ⚫⚪ High contrast (black on white, white on black)
- 📐 Sharp edges (corners, boundaries)
- 💨 Motion (things moving)
- 🎨 Color pop-out (lone red apple among green ones)

**Low Saliency (S ≈ 0.0-0.3):**
- 😴 Uniform areas (blank wall, clear sky)
- 🌫️ Low contrast (grey on grey)
- 🔲 Smooth gradients (no edges)
- 🧊 Static objects

**How it's computed:**
```python
patch = image[y:y+32, x:x+32]  # 32×32 pixel patch
gray = convert_to_grayscale(patch)
variance = std_deviation(gray)
saliency = variance / 128.0  # Normalize 0-1
```

**Example:**
- White wall patch: S = 0.05 (boring)
- Your hand patch: S = 0.6 (interesting)
- Red balloon patch: S = 0.85 (very interesting!)

---

### 🟡 **G = Goal (Top-Down Attention)**

**"What am I looking for?"**

Goal measures how **relevant** something is to Melvin's current task or active concepts.

**Right now (in basic mode), G is mostly neutral (0.3)** because Melvin doesn't have a specific goal set yet.

**But when implemented, it works like:**

If Melvin is thinking about "fire":
- 🔥 Red/orange patches → G = 0.9 (fire-like!)
- 💧 Blue patches → G = 0.2 (not fire-like)
- ⚪ White patches → G = 0.3 (neutral)

If Melvin is thinking about "person":
- 👤 Face-like regions → G = 0.9
- 🌳 Tree regions → G = 0.2
- 🧱 Wall regions → G = 0.3

**How it's computed:**
```python
goal = 0.3  # Baseline (no specific goal)

for active_concept in melvin_is_thinking_about:
    if patch_matches_concept(patch, active_concept):
        goal += 0.3  # Boost relevance
```

**This is the FEEDBACK loop!**
- Melvin thinks about "balloon" → Next frame, balloon patches get higher G scores → Crosshair stays on balloon!

---

### 🟣 **C = Curiosity (Novelty/Prediction Error)**

**"What's unexpected or new?"**

Curiosity measures how **surprising** or **novel** something is.

**High Curiosity (C ≈ 0.6-1.0):**
- 🆕 New objects appearing suddenly
- 🎬 Unexpected motion
- 🔄 Things changing shape/color
- ❓ Objects where they shouldn't be
- 💥 Prediction errors (thought X would happen, but Y happened)

**Low Curiosity (C ≈ 0.0-0.2):**
- 😴 Static, unchanging objects
- 📅 Expected patterns
- 🔁 Seen many times before

**How it's computed:**
```python
edges = detect_edges(patch)  # Canny edge detection
edge_density = count(edges) / patch_size²
curiosity = edge_density  # More edges = more novel

# Or for temporal novelty:
if patch_never_seen_before:
    curiosity = 0.8  # Very curious!
elif patch_changed_unexpectedly:
    curiosity = 0.6  # Moderately curious
else:
    curiosity = 0.2  # Not curious
```

**Example:**
- Static wall you've seen 100 times: C = 0.1
- Your hand suddenly appearing: C = 0.7
- Balloon moving unexpectedly: C = 0.8

---

## 🎯 How They Combine (The Magic!)

```
F = 0.45 × S + 0.35 × G + 0.20 × C
    ↑         ↑         ↑
    45%       35%       20%
   weight    weight    weight
```

**Translation:**
- **45%** of attention comes from **what stands out** (saliency)
- **35%** from **what's relevant** (goal)
- **20%** from **what's surprising** (curiosity)

---

## 📋 Real Examples

### **Example 1: Red Balloon** 🎈

```
Patch contains: Bright red balloon
```

**Scores:**
- S = 0.85 (bright red, high contrast)
- G = 0.30 (no specific goal set)
- C = 0.40 (has edges, moderately novel)

**Focus score:**
```
F = 0.45 × 0.85 + 0.35 × 0.30 + 0.20 × 0.40
  = 0.3825 + 0.105 + 0.08
  = 0.5675
```

**Result:** HIGH focus score → **Crosshair goes here!** ✅

---

### **Example 2: Blank Wall**

```
Patch contains: Plain grey wall
```

**Scores:**
- S = 0.10 (uniform, low contrast)
- G = 0.30 (neutral)
- C = 0.05 (no edges, familiar)

**Focus score:**
```
F = 0.45 × 0.10 + 0.35 × 0.30 + 0.20 × 0.05
  = 0.045 + 0.105 + 0.01
  = 0.16
```

**Result:** LOW focus score → Crosshair ignores this ❌

---

### **Example 3: Your Hand Waving** 🖐️

```
Patch contains: Moving hand
```

**Scores:**
- S = 0.75 (contrast from background, skin tone)
- G = 0.30 (neutral goal)
- C = 0.80 (MOTION = unexpected = high curiosity!)

**Focus score:**
```
F = 0.45 × 0.75 + 0.35 × 0.30 + 0.20 × 0.80
  = 0.3375 + 0.105 + 0.16
  = 0.6025
```

**Result:** VERY HIGH focus → **Crosshair tracks your hand!** ✅✅

---

## 🎬 Step-by-Step: What Happens Each Frame

### Step 1: Divide Image into Patches
```
Camera frame (640×480)
    ↓
Split into 20×15 grid
    ↓
= 300 patches (each 32×32 pixels)
```

### Step 2: Score Every Patch
```
For each of the 300 patches:
    Compute S (how much it stands out)
    Compute G (how relevant to goal)
    Compute C (how novel/surprising)
    Compute F = 0.45×S + 0.35×G + 0.20×C
```

### Step 3: Find the Winner
```
Look at all 300 F scores
Pick the HIGHEST one
    ↓
That's where the dot goes!
```

### Step 4: Apply Inhibition of Return (IOR)
```
Was this patch JUST focused on?
    ↓ YES
Skip it! (prevent immediate return)
Pick the 2nd highest instead
```

### Step 5: Apply Inertia
```
Is current focus STILL interesting?
    ↓ YES
Give it a 15% bonus
    ↓
Harder for new things to steal focus
```

---

## 🔍 Why the Dot Goes Where It Goes - Real Cases

### **Case 1: Balloon in View** 🎈

**Why it attracts attention:**
```
S = 0.85  (bright color, clear edges)
G = 0.30  (no specific balloon goal)
C = 0.40  (novel shape)

F = 0.45×0.85 + 0.35×0.30 + 0.20×0.40
  = 0.57 ← HIGH!
```

**Balloon wins because:**
- Very high saliency (bright red)
- Moderate curiosity (interesting shape)
- Combined score beats boring wall patches

---

### **Case 2: Your Hand Waving**

**Why it steals attention:**
```
Before wave:
  S = 0.4  (hand visible but still)
  C = 0.2  (not moving)
  F = 0.32 (moderate)

After wave:
  S = 0.7  (motion blur adds contrast)
  C = 0.9  (MOTION = surprise!)
  F = 0.62 ← VERY HIGH!
```

**Hand wins because:**
- Motion triggers high curiosity
- Movement changes patch appearance
- Unexpected = interesting to Melvin!

---

### **Case 3: Multiple Objects**

**When you have balloon AND waving hand:**

```
Balloon patch:  F = 0.57
Hand patch:     F = 0.62
Wall patch:     F = 0.16

Winner: HAND (0.62 is highest)
    ↓
Crosshair goes to hand!
```

**But then:**
- Hand gets inhibited (IOR) for 0.8 seconds
- Can't focus on it again immediately
- Next highest = Balloon (F = 0.57)
- Crosshair shifts to balloon!

**This forces exploration!**

---

## 🎯 The Three Attention Modes

### **1. Saliency-Driven** (Most Common Now)
```
No goal set → G stays low (0.3)
S is dominant (45% weight)
    ↓
Dot goes to brightest/most contrasting things
```

**You see:** Crosshair jumping to edges, colors, motion

---

### **2. Goal-Driven** (When Active Concepts Set)
```
Melvin thinking about "fire"
    ↓
Red/orange patches get G boost
    ↓
Even if not super bright, relevant patches win
```

**You'd see:** Crosshair preferring fire-like things, ignoring bright but irrelevant items

---

### **3. Curiosity-Driven** (When Novel Things Appear)
```
New object appears suddenly
    ↓
C spikes to 0.8-1.0
    ↓
Even if not salient, curiosity wins!
```

**You see:** Crosshair immediately jumping to new/moving things

---

## 📐 The Exact Computation (What the Code Does)

### For Each 32×32 Patch:

**1. Saliency:**
```python
# Convert to grayscale
gray = rgb_to_gray(patch)

# Compute standard deviation (variance)
std_dev = np.std(gray)

# Normalize to 0-1
saliency = std_dev / 128.0
```

**Why this works:** 
- Uniform areas (walls) → low std dev → low S
- Edges/patterns → high std dev → high S

---

**2. Goal:**
```python
goal = 0.3  # Baseline

# Check if patch matches active concepts
for concept in active_concepts:
    if patch_connected_to_concept(patch, concept):
        goal += 0.3  # Boost relevance
```

**Why this works:**
- No active concepts → G stays 0.3 (neutral)
- With goals → relevant patches get boosted

---

**3. Curiosity:**
```python
# Detect edges (Canny algorithm)
edges = cv2.Canny(gray, 50, 150)

# Count edge pixels
edge_count = np.sum(edges > 0)

# Normalize
curiosity = edge_count / (32 × 32)
```

**Why this works:**
- Smooth areas → few edges → low C
- Complex patterns → many edges → high C
- Motion → changes edges → high C

---

**4. Combined Focus:**
```python
F = 0.45 * saliency + 0.35 * goal + 0.20 * curiosity
```

---

**5. Winner Selection:**
```python
all_scores = [patch1.F, patch2.F, ..., patch300.F]
winner = max(all_scores)
    ↓
crosshair_position = winner.center_xy
```

---

## 🎨 Visual Example: What Melvin Sees

Let's say your camera shows this scene:

```
┌─────────────────────────────────┐
│  🧱 Wall    🎈 Balloon  🧱 Wall │  
│  (grey)     (red)      (grey)  │
│                                  │
│  🧱 Wall    🖐️ Hand    🧱 Wall │
│  (grey)  (moving!)    (grey)  │
└─────────────────────────────────┘
```

### **Patch Scores:**

| Patch | S | G | C | F | Selected? |
|-------|---|---|---|---|-----------|
| Wall (top-left) | 0.10 | 0.30 | 0.05 | **0.16** | ❌ |
| Balloon (top-center) | 0.85 | 0.30 | 0.40 | **0.57** | Maybe |
| Wall (top-right) | 0.10 | 0.30 | 0.05 | **0.16** | ❌ |
| Wall (bottom-left) | 0.10 | 0.30 | 0.05 | **0.16** | ❌ |
| Hand (bottom-center) | 0.70 | 0.30 | 0.90 | **0.60** | ✅ **WINNER!** |
| Wall (bottom-right) | 0.10 | 0.30 | 0.05 | **0.16** | ❌ |

**The dot goes to your HAND because F=0.60 is highest!**

Why?
- Hand has good saliency (0.70)
- Hand is MOVING → curiosity is very high (0.90)
- Combined score beats balloon's (0.60 > 0.57)

---

## 🔄 Why the Dot Moves (Frame-to-Frame)

### **Frame 1:**
```
Balloon visible, hand still
Balloon: F = 0.57
Hand:    F = 0.32

→ Dot goes to balloon
```

### **Frame 2:**
```
Hand starts moving!
Balloon: F = 0.57
Hand:    F = 0.60 (curiosity spike!)

→ Dot jumps to hand
```

### **Frame 3:**
```
Hand JUST focused, gets inhibited (IOR)
Balloon: F = 0.57
Hand:    F = 0.60 BUT INHIBITED!

→ Dot goes to balloon (next highest)
```

### **Frame 4:**
```
IOR expires (0.8s passed)
Both available again
Hand:    F = 0.62 (still moving)

→ Dot back to hand
```

---

## 🎈 Why Balloons Get Attention

When you saw balloons, here's what happened:

```
Balloon Patch Analysis:
  S = 0.85    (bright red/blue/yellow color)
  G = 0.30    (neutral - no balloon goal set)
  C = 0.40    (round shape has edges)
  
F = 0.45 × 0.85 + 0.35 × 0.30 + 0.20 × 0.40
  = 0.3825 + 0.105 + 0.08
  = 0.5675

Wall Patch for Comparison:
  S = 0.08
  G = 0.30
  C = 0.05
  
F = 0.45 × 0.08 + 0.35 × 0.30 + 0.20 × 0.05
  = 0.036 + 0.105 + 0.01
  = 0.151

Balloon wins: 0.57 >> 0.15
    ↓
Crosshair locks on balloon! 🎈
```

---

## 💡 Simple Rules (TL;DR)

The dot goes where:

1. **Things are bright/colorful** (high S)
2. **Things are moving** (high C)
3. **Things have edges/patterns** (high C)
4. **Things are relevant** (high G - when goals set)

And AVOIDS:
- Uniform areas (walls, sky)
- Things just focused on (IOR)
- Static, familiar objects

**The crosshair picks the most "interesting" patch!**

---

## 🧪 Test It Yourself

### **Experiment 1: Saliency Test**
```
1. Show bland object (grey book) → Low S → Ignored
2. Show bright object (red cup) → High S → Focused!
```

### **Experiment 2: Curiosity Test**
```
1. Hold object still → Low C → Moderate attention
2. Move object suddenly → High C → Immediate focus!
```

### **Experiment 3: Goal Test** (Future)
```
1. Set goal: "find red things"
2. Show red object → High G → Focused even if not brightest
3. Show blue object → Low G → Ignored even if bright
```

---

## 🎛️ The Weights Explained

### Why 0.45, 0.35, 0.20?

**α = 0.45 (Saliency)** - Largest weight
- Bottom-up attention is PRIMARY
- "What catches my eye" is most important
- Like human reflexive attention

**β = 0.35 (Goal)** - Second largest
- Top-down control is important
- "What I'm looking for" matters
- Like human task-directed attention

**γ = 0.20 (Curiosity)** - Smallest weight
- Novelty is a bonus
- "What's new" gets some attention
- Prevents getting stuck on same things

**These weights are biologically inspired!**
- Match human attention studies
- Balance exploration vs exploitation

---

## 🔍 Detailed Walkthrough: Why YOUR Scene Got That Focus

When you ran the camera:

### **Balloons Visible:**

1. **Camera captures frame**
   - 1920×1080 pixels
   - Divided into patches (64×64 for HD)

2. **Each patch scored:**
   - Balloon patch: High S (bright), High C (edges)
   - Wall patches: Low S (uniform), Low C (no edges)
   - Background: Low everything

3. **Formula applied:**
   - Balloon: F ≈ 0.55-0.65
   - Wall: F ≈ 0.15-0.20
   - Background: F ≈ 0.10-0.15

4. **Winner selected:**
   - Balloon has HIGHEST F
   - Crosshair positioned at balloon center
   - Cyan heatmap bright on balloon

5. **You see:**
   - Yellow crosshair on balloon! 🎈
   - That's why!

---

## 🧩 The Complete Picture

```
Every Frame:
    ↓
1. Divide into 300 patches
    ↓
2. For EACH patch, compute:
   • S (how much it stands out)
   • G (how relevant to goal)
   • C (how surprising)
   • F = 0.45×S + 0.35×G + 0.20×C
    ↓
3. Find patch with MAX(F)
    ↓
4. Check IOR (recently focused?)
   YES → Pick 2nd highest
   NO  → Use this one
    ↓
5. Apply inertia (current focus gets 15% boost)
    ↓
6. Draw crosshair at winner's center
    ↓
7. Next frame → repeat!
```

**The dot always goes to the patch with the highest combined "interestingness" score!**

---

## 🎯 Summary: Why the Dot Goes There

**Short Answer:**
The dot goes to the patch that is most **salient** (stands out), **relevant** (matches goals), and **curious** (surprising).

**Longer Answer:**
Every patch gets scored on three things:
1. How much it stands out visually (S)
2. How relevant it is to what Melvin is thinking about (G)
3. How surprising or novel it is (C)

These combine with weights (45%, 35%, 20%) into a total focus score (F).

**The patch with the HIGHEST F score gets the crosshair!**

**That's why:**
- Balloons get attention (bright, edges)
- Moving hands get attention (motion = surprise)
- Blank walls don't get attention (nothing interesting)

---

## 🧠 Biological Parallel

This is how **your brain** works too!

- **Bottom-up** (S): "Ooh, shiny thing!"
- **Top-down** (G): "I'm looking for my keys"
- **Novelty** (C): "Wait, what's that?!"

Your attention is ALSO a weighted combination of these three!

Melvin's formula replicates human attention mechanisms. 🧠✨

---

**Now you know exactly why the crosshair goes where it goes!** 👁️

Want to test it? Try:
```bash
./run_melvin_camera
```

Then wave your hand and watch F spike to 0.6+ on your hand! 🖐️

